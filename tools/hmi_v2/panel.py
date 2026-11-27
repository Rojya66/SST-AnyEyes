"""Run a Windows commissioning panel over ARM Modbus TCP."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import csv
from datetime import datetime
import os
from pathlib import Path
import time
import tkinter as tk
from tkinter import messagebox, simpledialog, ttk
from uuid import uuid4

from protocol import (PARAMETER_COUNT, PARAMETER_START, VERSION_LABEL, ModbusTcp, Panel,
                      ProtocolError, SnapshotChanged, load_points)


PHASES = "ABC"
STATE_NAMES = ("停止", "预充", "等 KT1", "等 PLL", "运行", "故障")
FAN_CONFIG_STATES = ("未开始", "等待 UNIT 确认", "已确认", "配置失败", "使用默认阈值或未启用分发")
PHASE_FIELDS = (
    ("PM_ACTIVE", "投入运行"),
    ("UNIT_ONLINE", "501 在线"),
    ("UDC_USABLE", "Udc 可用"),
    ("UNIT_OFFLINE", "当前失联"),
    ("UNIT_TRIP_LOSS", "跳闸失联"),
    ("FO_ENABLE", "701 使能"),
    ("FO_LIVE", "701 实时"),
    ("FO_FIRST", "701 首锁存"),
)
FAULT_NAMES = Path(__file__).with_name("fault_codes.csv")
LOG_PATH = Path(os.environ.get("LOCALAPPDATA", Path.home())) / "SST_CHB" / "fault_history.csv"


def read_fault_names():
    with FAULT_NAMES.open(encoding="utf-8-sig", newline="") as file:
        return {int(row["code"]): row["name"] for row in csv.DictReader(file)}


def bitmap_modules(mask):
    return "、".join(str(n + 1) for n in range(14) if mask & (1 << n)) or "—"


def enum_name(value, choices):
    return choices[value] if value < len(choices) else str(value)


def tree(parent, columns, height):
    frame = ttk.Frame(parent)
    frame.pack(fill="both", expand=True)
    widget = ttk.Treeview(frame, columns=tuple(c[0] for c in columns), show="headings", height=height)
    for name, width in columns:
        widget.heading(name, text=name)
        widget.column(name, width=width, minwidth=40, stretch=True)
    scroll = ttk.Scrollbar(frame, orient="vertical", command=widget.yview)
    widget.configure(yscrollcommand=scroll.set)
    scroll.pack(side="right", fill="y")
    widget.pack(side="left", fill="both", expand=True)
    return widget


def rows(widget, values):
    for index, cells in enumerate(values):
        item = str(index)
        if widget.exists(item):
            widget.item(item, values=cells)
        else:
            widget.insert("", "end", iid=item, values=cells)
    extra = widget.get_children()[len(values):]
    if extra:
        widget.delete(*extra)


class HistoryJournal:
    def __init__(self, path):
        self.path = path
        self.new_session()

    def new_session(self):
        self.session = uuid4().hex
        self.last = 0
        self.uptime = None

    def update_uptime(self, uptime):
        reset = self.uptime is not None and uptime < self.uptime
        if reset:
            self.new_session()
        self.uptime = uptime
        return reset

    def append(self, records):
        latest = self.last
        incoming = []
        for row in reversed(records):
            sequence = row[13] | (row[14] << 16)
            if sequence > self.last:
                incoming.append(row)
                latest = sequence
        if not incoming:
            return
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self.path.open("a", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            if file.tell() == 0:
                writer.writerow(("received_time", "connection_session", "code", "level", "core", "port",
                                 "type", "AD_source", "FB", "FO_A", "FO_B", "FO_C", "module_dispose",
                                 "tick_low", "tick_high", "event_low", "event_high", "501_A", "501_B", "501_C"))
            timestamp = datetime.now().astimezone().isoformat(timespec="seconds")
            for row in incoming:
                writer.writerow((timestamp, self.session, *row))
        self.last = latest


class App:
    def __init__(self, root, args, points, faults):
        self.root = root
        self.points = points
        self.faults = faults
        self.args = args
        self.parameter_points = [p for p in points.values()
                                 if PARAMETER_START <= p.address < PARAMETER_START + PARAMETER_COUNT]
        self.executor = ThreadPoolExecutor(max_workers=1)
        self.panel = None
        self.future = None
        self.pending = None
        self.closing = False
        self.command_uncertain = False
        self.last_live_change = 0.0
        self.latest = {}
        self.parameter_values = None
        self.parameter_edits = {}
        self.history_epoch = None
        self.journal = HistoryJournal(args.journal)
        self.actions = []

        root.title("SST CHB · 调试屏")
        root.geometry("1220x770")
        root.minsize(900, 560)
        root.protocol("WM_DELETE_WINDOW", self.close)
        style = ttk.Style(root)
        style.configure("Treeview", rowheight=26)
        style.configure("Title.TLabel", font=("Microsoft YaHei UI", 15, "bold"))

        nav = ttk.Frame(root, padding=(12, 8))
        nav.pack(fill="x")
        ttk.Label(nav, text="SST CHB 调试屏", style="Title.TLabel").pack(side="left", padx=(0, 16))
        ttk.Label(nav, text="ARM 地址").pack(side="left")
        self.host = ttk.Entry(nav, width=20)
        self.host.insert(0, args.host or "")
        self.host.pack(side="left", padx=5)
        self.connect_button = ttk.Button(nav, text="连接", command=self.connect)
        self.connect_button.pack(side="left")
        self.link = tk.StringVar(value="未连接")
        ttk.Label(nav, textvariable=self.link).pack(side="left", padx=10)
        self.feedback = tk.StringVar(value=f"等待读取 DSP {VERSION_LABEL}")
        ttk.Label(root, textvariable=self.feedback, padding=(12, 4), anchor="w").pack(side="bottom", fill="x")
        self.pages = ttk.Notebook(root, padding=(8, 4))
        self.pages.pack(fill="both", expand=True)
        self.live_page = self.page("运行与开关")
        self.module_page = self.page("模块诊断")
        self.param_page = self.page("遥调")
        self.fault_page = self.page("故障与历史")
        self.build_live()
        self.build_modules()
        self.build_parameters()
        self.build_faults()
        self.enable_actions(False)
        root.after(25, self.reap)
        root.after(200, self.poll)

    def page(self, title):
        page = ttk.Frame(self.pages, padding=10)
        self.pages.add(page, text=title)
        return page

    def button(self, box, label, action):
        button = ttk.Button(box, text=label, command=action)
        button.pack(side="left", padx=(0, 6), pady=4)
        self.actions.append(button)

    def enable_actions(self, enabled):
        for button in self.actions:
            button.state(("!disabled",) if enabled else ("disabled",))

    def actions_available(self):
        return (self.panel is not None and self.pending is None and not self.command_uncertain
                and time.monotonic() - self.last_live_change < 1.0)

    def controls(self, box, labels):
        for label, op in labels:
            self.button(box, label, lambda op=op, label=label: self.control(op, label))

    def build_live(self):
        self.overview = tk.StringVar(value="等待 DSP")
        ttk.Label(self.live_page, textvariable=self.overview, style="Title.TLabel").pack(anchor="w", pady=(0, 6))
        running = ttk.Frame(self.live_page)
        running.pack(fill="x")
        self.controls(running, (("启动 CHB", 1), ("停止 CHB", 2), ("启动 DAB", 3), ("停止 DAB", 4),
                                ("整机停止", 5), ("复位故障", 6)))
        switching = ttk.Frame(self.live_page)
        switching.pack(fill="x")
        self.controls(switching, (("允许手动合闸", 11), ("禁止手动合闸", 12), ("合 QF1", 7), ("分 QF1", 8),
                                  ("合 KT1", 9), ("释放 KT1", 10)))
        self.switching = tk.StringVar(value="")
        ttk.Label(self.live_page, textvariable=self.switching).pack(anchor="w", pady=5)
        self.ready = tk.StringVar(value="")
        ttk.Label(self.live_page, textvariable=self.ready, wraplength=1020).pack(anchor="w")
        self.live_tree = tree(self.live_page, (("地址", 85), ("当前量", 320), ("值", 115), ("单位", 90)), 16)

    def build_modules(self):
        choices = ttk.Frame(self.module_page)
        choices.pack(fill="x")
        self.phase = tk.StringVar(value="A")
        ttk.Label(choices, text="观察相").pack(side="left")
        ttk.Combobox(choices, state="readonly", width=4, values=tuple(PHASES),
                     textvariable=self.phase).pack(side="left", padx=6)
        self.mask_fields = []
        for phase in PHASES:
            ttk.Label(choices, text=f"{phase} 相观察掩码").pack(side="left", padx=(8, 3))
            entry = ttk.Entry(choices, width=8)
            entry.pack(side="left")
            self.mask_fields.append(entry)
        self.button(choices, "提交选点", self.select_modules)
        self.button(choices, "观察全部投入", self.select_all_modules)
        self.module_info = tk.StringVar(value="首次刷新显示已采用的观察掩码")
        ttk.Label(self.module_page, textvariable=self.module_info).pack(anchor="w", pady=6)
        self.module_tree = tree(self.module_page, (
            ("UNIT", 60), ("已投入", 70), ("501", 70), ("Udc 可用", 75), ("701 首报", 75),
            ("Udc (V)", 95), ("Ir (A)", 95), ("Ig (A)", 95), ("PT100 (°C)", 100),
        ), 15)
        self.module_masks = tk.StringVar(value="")
        ttk.Label(self.module_page, textvariable=self.module_masks, wraplength=1050).pack(anchor="w", pady=8)

    def build_parameters(self):
        panel = ttk.Frame(self.param_page)
        panel.pack(fill="x")
        self.button(panel, "读取活动参数", lambda: self.parameter(4))
        self.button(panel, "应用编辑", lambda: self.parameter(1))
        self.button(panel, "保存至 EEPROM", lambda: self.parameter(2))
        self.button(panel, "加载 EEPROM", lambda: self.parameter(3))
        ttk.Label(panel, text="筛选").pack(side="left", padx=(12, 4))
        self.filter = tk.StringVar()
        ttk.Entry(panel, width=22, textvariable=self.filter).pack(side="left")
        self.filter.trace_add("write", lambda *_: self.show_parameters())
        ttk.Label(self.param_page, text="先读取活动参数。双击编辑值，应用后查看 DSP 回读；保存仅保存活动值。").pack(anchor="w", pady=5)
        self.fan_config = tk.StringVar(value="等待风扇均压配置状态")
        ttk.Label(self.param_page, textvariable=self.fan_config, wraplength=1030).pack(anchor="w", pady=5)
        self.param_tree = tree(self.param_page, (
            ("地址", 80), ("参数", 260), ("活动值", 100), ("编辑值", 100), ("单位", 75), ("原始范围", 155),
        ), 20)
        self.param_tree.bind("<Double-1>", self.edit_parameter)
        self.param_tree.bind("<Return>", self.edit_parameter)

    def build_faults(self):
        ttk.Label(self.fault_page, text=f"本机历史：{self.args.journal}", wraplength=1050).pack(anchor="w")
        self.fault_head = tk.StringVar(value="等待读取故障状态")
        ttk.Label(self.fault_page, textvariable=self.fault_head, wraplength=1030).pack(anchor="w", pady=6)
        current = ttk.LabelFrame(self.fault_page, text="当前故障", padding=6)
        current.pack(fill="both")
        self.current_faults = tree(current, (("故障码", 100), ("故障含义", 460)), 4)
        history = ttk.LabelFrame(self.fault_page, text="DSP 历史（最近 32 条）", padding=6)
        history.pack(fill="both", expand=True, pady=(10, 0))
        self.history_tree = tree(history, (("事件", 55), ("DSP 时间 s", 80), ("代码", 65), ("故障含义", 220),
                                          ("核心", 50), ("403 锁存", 90), ("701 A/B/C", 165), ("501 A/B/C", 165)), 12)

    def schedule(self, call, callback, action=False):
        if self.future is not None:
            if self.pending is None and action:
                self.pending = call, callback, action
                self.enable_actions(False)
            return
        self.future = self.executor.submit(call), callback, action
        if action:
            self.enable_actions(False)

    def connect(self):
        if self.future is not None and self.future[2]:
            return
        if self.panel is not None:
            panel = self.panel

            def disconnected(_):
                self.panel = None
                self.clear_live()
                self.connect_button.configure(text="连接")
                self.link.set("未连接")

            self.schedule(panel.close, disconnected, True)
            return
        host = self.host.get().strip()
        if not host:
            self.feedback.set("输入 ARM 地址后连接")
            return

        def open_panel():
            panel = Panel(ModbusTcp(host, self.args.port, self.args.unit), self.args.timeout)
            try:
                panel.wait_running()
            except (OSError, ProtocolError):
                panel.close()
                raise
            return panel

        def connected(panel):
            self.panel = panel
            self.command_uncertain = False
            self.last_live_change = 0.0
            self.history_epoch = None
            self.journal.new_session()
            for entry in self.mask_fields:
                entry.delete(0, "end")
            self.link.set(f"{host}:{self.args.port} · Unit ID {self.args.unit}")
            self.connect_button.configure(text="断开")
            self.feedback.set(f"{VERSION_LABEL} 已识别，等待读取实时状态")

        self.schedule(open_panel, connected, True)

    def clear_live(self):
        self.latest = {}
        self.overview.set("等待 DSP")
        self.switching.set("")
        self.ready.set("")
        self.module_info.set("等待 DSP")
        self.module_masks.set("")
        self.fault_head.set("等待读取故障状态")
        self.fan_config.set("等待风扇均压配置状态")
        self.parameter_values = None
        self.parameter_edits.clear()
        for widget in (self.live_tree, self.module_tree, self.current_faults, self.history_tree, self.param_tree):
            rows(widget, [])

    def reap(self):
        if self.future is not None and self.future[0].done():
            result, callback, action = self.future
            self.future = None
            try:
                callback(result.result())
            except SnapshotChanged as exc:
                self.link.set(str(exc))
            except (OSError, ProtocolError) as exc:
                if isinstance(exc, OSError) and self.panel is not None:
                    self.panel.close()
                    self.panel = None
                    self.pending = None
                    self.clear_live()
                    self.link.set("连接已断开")
                    self.connect_button.configure(text="连接")
                prefix = "操作未确认" if action else "读取失败"
                if action and self.panel is not None:
                    self.command_uncertain = True
                    self.feedback.set(f"{prefix} · {exc} · 核对设备状态后重连")
                else:
                    self.feedback.set(f"{prefix} · {exc}")
            self.enable_actions(self.actions_available())
            if self.pending:
                queued = self.pending
                self.pending = None
                self.schedule(*queued)
        self.root.after(25, self.reap)

    def poll(self):
        if not self.closing and self.panel is not None and self.future is None:
            panel = self.panel
            phase = PHASES.index(self.phase.get())
            tab = self.pages.select()
            history_epoch = self.history_epoch

            def read_pages():
                live = panel.live()
                try:
                    module = panel.modules(phase) if tab == str(self.module_page) else None
                except SnapshotChanged:
                    module = None
                uptime = self.points["UPTIME_10MS"].decode(live)
                previous_epoch = None if self.journal.uptime is not None and uptime < self.journal.uptime else history_epoch
                try:
                    history = panel.history(previous_epoch)
                except SnapshotChanged:
                    history = (None, None)
                return live, module, history

            def display(data):
                live, module, history = data
                uptime = self.points["UPTIME_10MS"].decode(live)
                if uptime != self.journal.uptime:
                    self.last_live_change = time.monotonic()
                if self.journal.update_uptime(uptime):
                    self.clear_live()
                    self.history_epoch = None
                    for entry in self.mask_fields:
                        entry.delete(0, "end")
                    self.feedback.set("DSP 已重新上电；请重新读取活动参数")
                self.latest = live
                alive = time.monotonic() - self.last_live_change < 1.0
                self.link.set(f"{self.host.get().strip()} · {VERSION_LABEL} {'在线' if alive else '实时节拍未更新'}")
                self.show_live()
                self.show_fan_config()
                if module is not None and phase == PHASES.index(self.phase.get()):
                    self.show_modules(phase, module)
                self.show_faults(live, history)

            if not self.actions_available():
                self.enable_actions(False)
            self.schedule(read_pages, display)
        self.root.after(250, self.poll)

    def value(self, name, words=None):
        return self.points[name].decode(self.latest if words is None else words)

    def show_live(self):
        state = self.value("CHB_STATE")
        requests = self.value("BRIDGE_REQUEST")
        bridge = self.value("BRIDGE_STATUS")
        mode = enum_name(self.value("CHB_MODE_ACTIVE"), ("开环", "闭环"))
        first = self.value("fault_first_code")
        self.overview.set(f"{enum_name(state, STATE_NAMES)} · {mode}    "
                          f"CHB 请求 {requests & 1} / PWM {bridge & 1}    "
                          f"DAB 请求 {(requests >> 1) & 1} / PWM {(bridge >> 1) & 1}    "
                          f"总关断 {(bridge >> 4) & 1}    " + (f"故障 {first}" if first else ""))
        target = self.value("SW_TARGET")
        feedback = self.value("SW_FEEDBACK")
        if self.value("BUILD_IO_CAPABILITY") & 2:
            contact = f"DI7 合位 {feedback & 1} · DI8 分位 {(feedback >> 1) & 1} · DI6 KT1 {(feedback >> 2) & 1}"
        else:
            contact = "开关反馈未启用"
        self.switching.set(f"QF1 目标 {target & 1} · KT1 目标 {(target >> 1) & 1} · {contact}    "
                           f"{'自动' if target & 4 else '手动'} · 手动合闸 {'允许' if target & 8 else '禁止'}")
        if state == 1:
            flags = self.value("PRECHARGE_READY")
            labels = ("触点", "全部在线", "Udc 可用", "无超时", "701/403", "电压", "风扇配置")
            elapsed = self.value("STATE_ELAPSED") * 0.01
            stable = self.value("PRECHARGE_STABLE") * 10
            settings = ""
            if self.parameter_values is not None:
                timeout = self.value("PRECHARGE_TIMEOUT_10MS", self.parameter_values) * 0.01
                target = self.value("PRECHARGE_STABLE_10MS", self.parameter_values) * 10
                settings = f" / 总超时 {timeout:g} s · 连续定值 {target} ms"
            conditions = "  ".join(f"{n} {'满足' if flags & (1 << i) else '未满足/待查'}"
                                   for i, n in enumerate(labels))
            self.ready.set(f"预充总耗时 {elapsed:.2f} s · 连续达标 {stable} ms{settings}\n{conditions}")
        else:
            self.ready.set("")
        active = self.value("PHASE_CAPABILITY")
        required = {phase: self.value("PM_ACTIVE_" + phase) for phase in PHASES}
        voltage_ready = {phase: pm != 0 and self.value("UDC_USABLE_" + phase) == pm
                         for phase, pm in required.items()}
        all_voltage_ready = all(voltage_ready[phase] for i, phase in enumerate(PHASES) if active & (1 << i))
        data = []
        for p in self.points.values():
            if not (0x0516 <= p.address <= 0x0525 or 0x0540 <= p.address <= 0x056D or 0x059E <= p.address <= 0x05A9):
                continue
            label, value = p.label, p.format(self.latest)
            if 0x0540 <= p.address <= 0x0556 and p.name[-2:] in ("_A", "_B", "_C"):
                phase = PHASES.index(p.name[-1])
                if not active & (1 << phase):
                    value = "未参与"
                if p.name == "U_RMS_A":
                    label = "Uan 有效值" if active == 1 else "Uab 有效值"
                if p.name.startswith("UDC_PHASE_") and active & (1 << phase):
                    if not required[p.name[-1]]:
                        value = "未投入"
                    elif not voltage_ready[p.name[-1]]:
                        value += "（未齐）"
            if p.name in ("UDC_CONTROL_MEAN", "UDC_PHASE_MIN") and not all_voltage_ready:
                value += "（未齐）"
            data.append((f"0x{p.address:04X}", label, value, p.unit))
        rows(self.live_tree, data)

    def show_modules(self, phase, words):
        letter = PHASES[phase]
        enabled = self.value("PM_ACTIVE_" + letter)
        online = self.value("UNIT_ONLINE_" + letter)
        udc = self.value("UDC_USABLE_" + letter)
        fo_first = self.value("FO_FIRST_" + letter)
        published_valid = self.value("OBS_VALID_" + letter, words)
        valid = published_valid & online & udc
        applied = self.value("OBS_APPLIED_" + letter, words)
        permitted = self.value("PHASE_CAPABILITY") & (1 << phase)
        # Initialize text fields from applied state once per connection; preserve edits.
        for i, entry in enumerate(self.mask_fields):
            if not entry.get():
                selected = self.value("OBS_APPLIED_" + PHASES[i], words)
                entry.insert(0, f"0x{selected:04X}")
        recent = self.points["TMAX_OBS_" + letter].format(words) if valid and valid == published_valid else "—"
        self.module_info.set(f"{letter} 相 · 已采用 0x{applied:04X} · 有效采样 0x{valid:04X} · 所选最高温 {recent} °C"
                             if permitted else f"{letter} 相未参与当前构建")
        data = []
        for i in range(14):
            bit = 1 << i
            slot = f"{letter}{i + 1:02}"
            values = [self.points[f"{slot}_{field}"].format(words) if valid & bit else "—"
                      for field in ("UDC", "IR", "IG", "TEMP")]
            data.append((slot, "是" if enabled & bit else "—", "在线" if online & bit else "—",
                         "是" if udc & bit else "—", "故障" if fo_first & bit else "—", *values))
        rows(self.module_tree, data if permitted else [])
        details = []
        for prop, label in PHASE_FIELDS:
            value = self.value(prop + "_" + letter)
            if prop == "UNIT_TRIP_LOSS" and self.value("fault_control_reason") != 8:
                details.append(f"{label} 未判定")
            else:
                details.append(f"{label} {bitmap_modules(value)}")
        self.module_masks.set("   |   ".join(details) if permitted else "")

    def show_fan_config(self):
        state = enum_name(self.value("FAN_CONFIG_STATE"), FAN_CONFIG_STATES)
        off = self.value("FAN_CONFIG_OFF")
        on = self.value("FAN_CONFIG_ON")
        generation = self.value("FAN_CONFIG_GENERATION")
        phases = self.value("PHASE_CAPABILITY")
        details = []
        for i, phase in enumerate(PHASES):
            if not phases & (1 << i):
                continue
            required = self.value("PM_ACTIVE_" + phase)
            confirmed = self.value("FAN_CONFIG_CONFIRMED_" + phase)
            rejected = self.value("FAN_CONFIG_REJECTED_" + phase)
            pending = required & ~(confirmed | rejected)
            details.append(f"{phase} 相 已确认 {bitmap_modules(confirmed)} / "
                           f"拒绝 {bitmap_modules(rejected)} / 未确认 {bitmap_modules(pending)}")
        detail = "   ".join(details)
        self.fan_config.set(f"风扇均压 {state} · 事务请求 OFF {off} / ON {on} count · 代号 {generation}\n{detail}")

    def show_parameters(self):
        if self.parameter_values is None:
            rows(self.param_tree, [])
            return
        query = self.filter.get().strip().lower()
        entries = []
        self.visible_parameters = []
        for p in self.parameter_points:
            address = f"0x{p.address:04X}"
            if query and query not in f"{address} {p.label} {p.name}".lower():
                continue
            self.visible_parameters.append(p)
            entries.append((address, p.label, p.format(self.parameter_values),
                            self.parameter_edits.get(p.address, ""), p.unit, p.raw_range))
        rows(self.param_tree, entries)

    def edit_parameter(self, event):
        selected = self.param_tree.focus()
        if not selected or self.parameter_values is None:
            return
        p = self.visible_parameters[int(selected)]
        value = simpledialog.askstring(p.label,
                                      f"{p.label} ({p.unit})\n{p.note}\n\n原始范围 {p.raw_range}",
                                      initialvalue=self.parameter_edits.get(p.address, p.format(self.parameter_values)))
        if value is not None:
            try:
                p.encode(value.strip())
            except ValueError as exc:
                self.feedback.set(str(exc))
                return
            self.parameter_edits[p.address] = value.strip()
            self.show_parameters()

    def show_faults(self, live, history):
        core = self.value("fault_control_reason", live)
        count = min(self.value("fault_active_count", live), 32)
        def positions(prefix):
            return "   ".join(f"{phase} 相 {bitmap_modules(self.value(prefix + phase, live))}" for phase in PHASES)

        link_trip = positions("UNIT_TRIP_LOSS_") if core == 8 else "未判定"
        core_name = "9 风扇阈值配置失败" if core == 9 else str(core)
        self.fault_head.set(f"核心 {core_name} · 总故障字 0x{self.value('fault_summary', live):04X} · "
                            f"403 首锁存 0x{self.value('FB_FIRST', live):04X} · "
                            f"202 首锁存 0x{self.value('AD_FIRST', live):04X}\n"
                            f"701 首锁存：{positions('FO_FIRST_')}   |   501 跳闸失联：{link_trip}")
        rows(self.current_faults, [(code, self.faults.get(code, "设备报码")) for code in
                                   (live[0x0720 + i] for i in range(count))])
        new_epoch, records = history
        if records is not None:
            try:
                self.journal.append(records)
            except OSError as exc:
                self.feedback.set(f"故障历史归档失败 · {exc}")
            else:
                self.history_epoch = new_epoch
            data = []
            for r in records:
                seq = r[13] | (r[14] << 16)
                seconds = (r[11] | (r[12] << 16)) / 100
                fo = "/".join(f"{v:04X}" for v in r[7:10])
                lost = "/".join(f"{v:04X}" for v in r[15:18]) if r[2] in (1, 8) else "未判定"
                data.append((seq, f"{seconds:.2f}", r[0], self.faults.get(r[0], "设备报码"),
                             r[2], f"{r[6]:04X}", fo, lost))
            rows(self.history_tree, data)

    def control(self, operation, label, observation=(0, 0, 0)):
        if not self.actions_available():
            return
        if operation in (1, 3, 7, 9, 11) and not messagebox.askyesno("确认控制请求", f"发出“{label}”请求？"):
            return
        panel = self.panel
        self.feedback.set(f"{label} · 等待 DSP 受理回执")
        self.schedule(lambda: panel.control(operation, observation),
                      lambda receipt: self.feedback.set(f"{label} · {receipt.text()}"), True)

    def select_modules(self):
        try:
            numbers = tuple(int(entry.get(), 16 if entry.get().lower().startswith("0x") else 10)
                            for entry in self.mask_fields)
            if any(not 0 <= n <= 0x3FFF for n in numbers):
                raise ValueError
        except ValueError:
            self.feedback.set("A、B、C 观察点请填 0x0000–0x3FFF 或十进制数")
            return
        self.control(13, "设置观察掩码", numbers)

    def select_all_modules(self):
        if not self.latest:
            return
        values = tuple(self.value("PM_ACTIVE_" + phase) for phase in PHASES)
        for entry, value in zip(self.mask_fields, values):
            entry.delete(0, "end")
            entry.insert(0, f"0x{value:04X}")
        self.control(13, "观察全部投入", values)

    def parameter(self, operation):
        if not self.actions_available():
            return
        if operation == 1 and not self.parameter_edits:
            self.feedback.set("读取活动值并修改至少一个参数后应用")
            return
        if operation != 1 and self.parameter_edits and not messagebox.askyesno(
                "参数还有未应用编辑", "本次操作结束后会用 DSP 活动回读覆盖编辑。继续？"):
            return
        label = ("", "应用编辑", "保存至 EEPROM", "加载 EEPROM", "读取活动参数")[operation]
        if operation in (1, 2, 3) and not messagebox.askyesno("确认遥调", f"发出“{label}”请求？"):
            return
        values = None
        if operation == 1:
            values = tuple(p.encode(self.parameter_edits[p.address]) if p.address in self.parameter_edits
                           else self.parameter_values[p.address] for p in self.parameter_points)
        panel = self.panel
        self.feedback.set(f"{label} · 等待 DSP 受理回执")

        def done(result):
            receipt, active = result
            self.parameter_values = active
            if operation != 1 or receipt.result == 0:
                self.parameter_edits.clear()
            self.feedback.set(f"{label} · {receipt.text()}")
            self.show_parameters()

        self.schedule(lambda: panel.parameter(operation, values), done, True)

    def close(self):
        self.closing = True
        self.pending = None
        self.enable_actions(False)
        if self.future is not None:
            self.root.after(50, self.close)
            return
        if self.panel is not None:
            self.panel.close()
        self.executor.shutdown(wait=False, cancel_futures=True)
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", nargs="?", help="ARM IPv4 地址或主机名")
    parser.add_argument("--port", type=int, default=502)
    parser.add_argument("--unit", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=10.0, help="等待 DSP 事务结果的秒数")
    parser.add_argument("--journal", type=Path, default=LOG_PATH, help="本机故障历史 CSV 路径")
    parser.add_argument("--probe", action="store_true", help="只读核对 V2 与当前状态后退出")
    args = parser.parse_args()
    if args.probe:
        if not args.host:
            parser.error("--probe 需要 ARM 地址")
        try:
            panel = Panel(ModbusTcp(args.host, args.port, args.unit), args.timeout)
            try:
                panel.wait_running()
                values = panel.live()
                print(f"{VERSION_LABEL} | CHB state={values[0x0510]} | fault={values[0x0702]}")
            finally:
                panel.close()
        except (OSError, ProtocolError) as exc:
            parser.exit(1, f"{exc}\n")
        return
    root = tk.Tk()
    App(root, args, load_points(), read_fault_names())
    root.mainloop()


if __name__ == "__main__":
    main()
