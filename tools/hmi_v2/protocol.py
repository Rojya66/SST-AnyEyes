"""SST CHB V2.1 Modbus TCP access for the panel and bench client."""

import csv
from dataclasses import dataclass
from decimal import Decimal, ROUND_HALF_UP
from pathlib import Path
import socket
import struct
import time


SIGNATURE = (0x5353, 0x0201)
VERSION_LABEL = f"V{SIGNATURE[1] >> 8}.{SIGNATURE[1] & 0xFF}"
CONTROL_START = 0x0100
PARAMETER_COMMAND = 0x01FF
PARAMETER_START = 0x0200
PARAMETER_COUNT = 63
CONTROL_RECEIPT = 0x0504
PARAMETER_RECEIPT = 0x0507
LEFT_MAILBOX = 0x1FFE
LIVE_EPOCH = 0x0502
MODULE_EPOCH = 0x0503
HISTORY_EPOCH = 0x0800
HISTORY_START = 0x0810
HISTORY_RECORD_WORDS = 18
HISTORY_CAPACITY = 32

ROOT = Path(__file__).resolve().parents[2]
POINTS_CSV = ROOT / "docs" / "DPRAM_Screen_V2" / "DPRAM_MODBUS_POINTS.csv"


class ProtocolError(Exception):
    """The peer did not provide a usable response."""


class SnapshotChanged(ProtocolError):
    """The DSP is replacing the snapshot; try a fresh read."""


@dataclass(frozen=True)
class Receipt:
    count: int
    operation: int
    result: int
    error_address: int = 0xFFFF

    def text(self):
        descriptions = (
            "已受理", "地址错误", "只读", "超范围或约束不满足",
            "忙或未完全停机", "联锁或权限拒绝", "存储失败", "操作不支持",
        )
        result = descriptions[self.result] if self.result < len(descriptions) else str(self.result)
        detail = f"，出错地址 0x{self.error_address:04X}" if self.error_address != 0xFFFF else ""
        return f"DSP 完成计数 {self.count} · 操作 {self.operation} · {result}{detail}"


@dataclass(frozen=True)
class Point:
    address: int
    name: str
    label: str
    encoding: str
    width: int
    unit: str
    scale: Decimal
    raw_range: str
    note: str

    def decode(self, words):
        low = words[self.address]
        if self.encoding in ("U32", "F32"):
            raw = low | (words[self.address + 1] << 16)
            if self.encoding == "F32":
                return struct.unpack(">f", struct.pack(">I", raw))[0]
            return raw
        if self.encoding == "I16" and low & 0x8000:
            low -= 0x10000
        if self.scale == 1:
            return low
        return Decimal(low) / self.scale

    def format(self, words):
        value = self.decode(words)
        if self.encoding == "BITS":
            return f"0x{value:04X}"
        if self.encoding == "F32":
            return f"{value:.6g}"
        return str(value)

    def encode(self, text):
        try:
            if self.encoding == "BITS" and text.lower().startswith("0x"):
                raw = int(text, 16)
            else:
                number = Decimal(text)
                raw = int((number * self.scale).to_integral_value(rounding=ROUND_HALF_UP))
        except (ValueError, ArithmeticError) as exc:
            raise ValueError(f"{self.label} 需要合法数值") from exc
        bounds = {
            "I16": (-32768, 32767),
            "U16": (0, 65535),
            "BITS": (0, 65535),
        }
        minimum, maximum = bounds[self.encoding]
        if not minimum <= raw <= maximum:
            raise ValueError(f"{self.label} 超过 {self.encoding} 编码范围")
        return raw & 0xFFFF


def load_points(path=POINTS_CSV):
    with open(path, encoding="utf-8-sig", newline="") as file:
        return {
            fields["符号"]: Point(
                int(fields["字地址 HEX"], 16),
                fields["符号"],
                fields["中文点名"],
                fields["类型"],
                int(fields["占字"]),
                fields["单位"],
                Decimal(fields["编码乘数"]),
                fields["原始范围／枚举"],
                fields["说明／有效条件"],
            )
            for fields in csv.DictReader(file)
        }


class ModbusTcp:
    def __init__(self, host, port=502, unit=1, timeout=1.0):
        self.socket = socket.create_connection((host, port), timeout)
        self.unit = unit
        self.sequence = 0

    def close(self):
        self.socket.close()

    def _receive(self, length):
        data = bytearray()
        while len(data) < length:
            chunk = self.socket.recv(length - len(data))
            if not chunk:
                raise ProtocolError("ARM 关闭了 TCP 连接")
            data.extend(chunk)
        return data

    def _request(self, pdu):
        self.sequence = (self.sequence + 1) & 0xFFFF
        header = struct.pack(">HHHB", self.sequence, 0, len(pdu) + 1, self.unit)
        self.socket.sendall(header + pdu)
        sequence, protocol, length, unit = struct.unpack(">HHHB", self._receive(7))
        if (sequence, protocol, unit) != (self.sequence, 0, self.unit) or not 2 <= length <= 254:
            raise ProtocolError("Modbus TCP 回复头不符合请求")
        payload = self._receive(length - 1)
        if len(payload) == 2 and payload[0] == (pdu[0] | 0x80):
            raise ProtocolError(f"ARM 返回 Modbus 异常 {payload[1]:02X}")
        if payload[0] != pdu[0]:
            raise ProtocolError("Modbus TCP 回复功能码不符")
        return payload[1:]

    def read(self, address, count):
        if count < 1 or count > 125 or not 0 <= address < address + count <= 0x2000:
            raise ValueError("FC03 范围或长度错误")
        payload = self._request(struct.pack(">BHH", 3, address, count))
        if len(payload) != 1 + count * 2 or payload[0] != count * 2:
            raise ProtocolError("FC03 返回的寄存器长度不符")
        return struct.unpack(f">{count}H", payload[1:])

    def write_word(self, address, value):
        pdu = struct.pack(">BHH", 6, address, value)
        if not 0 <= address < 0x2000:
            raise ValueError("FC06 地址超出 DPRAM")
        if self._request(pdu) != pdu[1:]:
            raise ProtocolError("FC06 回显不符")

    def write_words(self, address, words):
        count = len(words)
        if count < 1 or count > 123 or not 0 <= address < address + count <= 0x1FFE:
            raise ValueError("FC10 范围、长度或邮箱位置错误")
        pdu = struct.pack(f">BHHB{count}H", 0x10, address, count, count * 2, *words)
        if self._request(pdu) != pdu[1:5]:
            raise ProtocolError("FC10 回显不符")


class Panel:
    """One requester per DSP. Completion counters distinguish DSP receipt from ARM ACK."""

    def __init__(self, bus, command_timeout=10.0):
        self.bus = bus
        self.command_timeout = command_timeout

    def close(self):
        self.bus.close()

    def check_version(self):
        version = self.bus.read(0x0500, 2)
        if version != SIGNATURE:
            raise ProtocolError(f"DSP 接口签名或版本不匹配，读到 0x{version[0]:04X}/0x{version[1]:04X}")

    def wait_running(self, timeout=1.0):
        self.check_version()
        first = None
        limit = time.monotonic() + timeout
        while time.monotonic() < limit:
            try:
                uptime = self.snapshot(LIVE_EPOCH, ((0x050C, 2),))
            except SnapshotChanged:
                continue
            if first is not None and uptime != first:
                return
            first = uptime
            time.sleep(0.03)
        raise ProtocolError(f"DSP {VERSION_LABEL} 签名已就绪，实时节拍未更新")

    def snapshot(self, epoch, spans):
        for _ in range(4):
            before = self.bus.read(epoch, 1)[0]
            if not before & 1:
                words = {}
                for address, count in spans:
                    words.update(enumerate(self.bus.read(address, count), address))
                if self.bus.read(epoch, 1)[0] == before:
                    return words
            time.sleep(0.01)
        raise SnapshotChanged("等待 DSP 发布下一份完整快照")

    def live(self):
        return self.snapshot(LIVE_EPOCH, (
            (0x050C, 0x056E - 0x050C),
            (0x0580, 0x0598 - 0x0580),
            (0x059E, 0x05B4 - 0x059E),
            (0x0700, 0x0740 - 0x0700),
        ))

    def modules(self, phase):
        if not 0 <= phase < 3:
            raise ValueError("相序须为 0、1 或 2")
        return self.snapshot(MODULE_EPOCH, ((0x056E + phase, 1), (0x0598, 6),
                                            (0x0600 + phase * 56, 56)))

    def history(self, previous_epoch):
        for _ in range(4):
            epoch = self.bus.read(HISTORY_EPOCH, 1)[0]
            if epoch == previous_epoch and not epoch & 1:
                return epoch, None
            if not epoch & 1:
                count = self.bus.read(HISTORY_EPOCH + 1, 1)[0]
                if count > HISTORY_CAPACITY:
                    raise ProtocolError("DSP 故障历史条数超出范围")
                rows = []
                for start in range(0, count, 6):
                    size = min(count - start, 6) * HISTORY_RECORD_WORDS
                    address = HISTORY_START + start * HISTORY_RECORD_WORDS
                    words = self.bus.read(address, size)
                    rows.extend(words[i:i + HISTORY_RECORD_WORDS] for i in range(0, size, HISTORY_RECORD_WORDS))
                if self.bus.read(HISTORY_EPOCH, 1)[0] == epoch:
                    return epoch, rows
            time.sleep(0.01)
        raise SnapshotChanged("等待 DSP 发布故障历史")

    def _submit(self, mailbox_value, operation, before):
        self.bus.write_word(LEFT_MAILBOX, mailbox_value)
        start = time.monotonic()
        parameter = mailbox_value == 2
        address = PARAMETER_RECEIPT if parameter else CONTROL_RECEIPT
        while time.monotonic() - start < self.command_timeout:
            words = self.bus.read(address, 5 if parameter else 3)
            count, last_op, result = words[:3]
            if count != before and (not parameter or words[4] == 0):
                if last_op != operation:
                    raise ProtocolError("DSP 回执操作号不符，请确认没有第二个写入端")
                return Receipt(count, last_op, result, words[3] if parameter else 0xFFFF)
            time.sleep(0.025)
        raise ProtocolError("DSP 回执超时，动作结果未知，请读取状态和回执后判断")

    def control(self, operation, observation=(0, 0, 0)):
        if operation not in range(1, 14):
            raise ValueError("控制操作号错误")
        if len(observation) != 3 or any(not 0 <= mask <= 0x3FFF for mask in observation):
            raise ValueError("观察掩码须为三个 14 位值")
        self.check_version()
        before = self.bus.read(CONTROL_RECEIPT, 1)[0]
        self.bus.write_words(CONTROL_START, (operation, *observation))
        return self._submit(1, operation, before)

    def parameter(self, operation, words=None):
        if operation not in (1, 2, 3, 4):
            raise ValueError("遥调操作号错误")
        if operation == 1 and (words is None or len(words) != PARAMETER_COUNT):
            raise ValueError(f"遥调应用需要完整的 {PARAMETER_COUNT} 个设置字")
        self.check_version()
        before, _, _, _, busy = self.bus.read(PARAMETER_RECEIPT, 5)
        if busy:
            raise ProtocolError("DSP 遥调忙")
        if operation == 1:
            self.bus.write_words(PARAMETER_COMMAND, (operation, *words))
        else:
            self.bus.write_word(PARAMETER_COMMAND, operation)
        receipt = self._submit(2, operation, before)
        active = self.bus.read(PARAMETER_START, PARAMETER_COUNT)
        return receipt, dict(enumerate(active, PARAMETER_START))
