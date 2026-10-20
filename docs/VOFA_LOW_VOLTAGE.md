# 100 V 单相试验 VOFA 观察点

此版通过 UART2/RS485 单向上报运行量。选择 `SST_DEBUG_PROTOCOL=SST_DEBUG_PROTOCOL_VOFA`，VOFA+ 选择 **JustFloat、460800、8N1、12 通道**。上报周期 `SST_TELEMETRY_PERIOD_10MS=1`，即 10 ms / 100 帧每秒。VOFA 不接收启动、停止或参数写入命令；仿真器控制仍通过 `g_chb_dab_command`。

| VOFA 通道（从 0 起） | 变量 | 单位 / 含义 |
|---:|---|---|
| 0 | `g_chb_dab_status.state` | 状态枚举转 float |
| 1 | `g_chb_dab_status.fault_code` | 控制故障枚举转 float，非五位故障上报码 |
| 2 | `g_chb_dab_status.dc_voltage_average_v` | V，A 相 6 个模块的直流均压；不是总母线电压 |
| 3 | `g_chb_dab_status.id_reference_a` | A，正在执行的 Idref |
| 4 | `g_chb_dab_status.id_feedback_a` | A，虚拟 dq 的 d 轴反馈 |
| 5 | `g_chb_dab_status.iq_feedback_a` | A，虚拟 dq 的 q 轴反馈 |
| 6 | `g_chb_dab_status.startup_phase` | 启动阶段枚举转 float |
| 7 | `g_chb_dab_status.startup_voltage_reference_v` | V/模块，斜坡后的实际电压环参考 |
| 8 | `g_chb_dab_status.modulation_actual` | 无量纲，A 相实际调制绝对值 |
| 9 | `g_chb_dab_status.pll_angle_rad` | PLL 相角，rad；锁相标志改用 CCS 观察 |
| 10 | `HA_Port_Volt_A` | V，AN 标定和既有采样低通后的瞬时量；不是 RMS，也不是 SOGI α |
| 11 | `HA_Port_Current_A` | A，501 `Avg_IG/10` 的 A 相瞬时电流；不是 SOGI α |

状态码：`0 STOPPED`、`1 PRECHARGE`、`2 BYPASS_DELAY`、`3 WAIT_PLL`、`4 RUNNING`、`5 FAULT`。

启动阶段：`0 IDLE`、`1 ID_RAMP_PHASE`、`2 VOLTAGE_HANDOVER`、`3 NORMAL`。

控制故障码：`0 NONE`、`1 PRECHARGE_TIMEOUT`、`2 PLL_TIMEOUT`（包括单相/三相闭环运行连续失锁）、`3 AC_OVERCURRENT`、`4 DC_OVERVOLTAGE`、`5 HARDWARE`、`6 BAD_CONFIGURATION`、`7 SWITCH_FEEDBACK`（五位上报29108）、`8 CONTROL_RUNTIME`（五位上报29109）。完整故障及五位故障码仍按现有 `FAULT_CODES.md`，AnyEyes 的参数、显示与波形地址未改。

上述 CH2/CH8 说明针对单相低压模式；三相模式中 CH2 为三相模块均压的算术平均，CH8 为三相调制绝对值的最大值。编码与通道顺序相同。

每帧为 12 个 IEEE754 float（每个 4 字节、小端）加 `00 00 80 7F` 帧尾，共 52 字节，无帧头和校验。460800 / 8N1 时每帧约 1.13 ms，100 Hz 约占 52 kbps。串口整帧在后台入队，UART 每轮最多推进 16 字节；10 kHz FPGA/TAD ISR 中没有增加串口、遥测拷贝或关中断。

各字段由后台顺序采集到本地数组；采集时 TAD 可正常抢占，因此一帧可能包含相邻 TAD 的数据，不能当作同拍记录。100 Hz 发送适合观察预充、锁相、Idref 斜坡和 Udc 收敛，不能重建 50 Hz 工频波形或判定电流环瞬态稳定性；细节用 CCS 缓冲或示波器观察。`SstDebug_GetTxDropped()` 可在 CCS 观察累计发送丢帧数，正常 100 Hz 条件下应保持不增长。

验证命令：`python tests/vofa_telemetry.py`。该主机测试编译真实后台协议和串口队列代码，检查通道顺序、物理量选择、52 字节帧及帧尾、周期门控、满队列丢帧计数和原四通道编码兼容性；RS485 电气层和板上 100 µs 时限需按 `BOARD_ACCEPTANCE.md` 实测。

50 A峰值低压限幅与开关量反馈的新增CCS观察字段、现场配置边界，见 [LOAD_CURRENT_AND_SWITCH_IO.md](LOAD_CURRENT_AND_SWITCH_IO.md)。VOFA十二通道顺序保持，新字段未占用既有AnyEyes/DPRAM地址。
