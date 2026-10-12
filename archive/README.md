# 不参与编译的历史代码

活动源码仅位于 `Source/`。CLI 与 CCS 的源发现均不包含本目录或 `Legacy/FrameworkArchive/`。

| 保留文件 | 原因及当前实现 |
|---|---|
| `legacy/main_uart_key_history.c`、`board_uart_history.c` | 保留停用 UART 帧、按键及旧 UART ISR 代码；新串口只经 `SstSerial` 后台有界轮询 |
| `legacy/ChbApplication.c/.h` | 保留原启动、等待及后台协调；当前为 `SystemInit`、`CpuTask`、`TaskSlice`、`SstDpramHost` |
| `legacy/MainHeader.H` | 保留原聚合类型、宏及非 HVAC 历史声明，供等价审计回查；活动模块包含所属窄头 |
| `legacy/main_entry_and_uart_macros.h` | 保留原入口声明及未活动的串口范围常量 |
| `../Legacy/FrameworkArchive/PluginUnit.C` | 原外围函数完整参照；活动实现已分入 FPGA IO、调制、FT3 和 DAC 模块 |
| `../Legacy/FrameworkArchive/Stateflow.C/.H` | 保留非活动启动流程；活动端口状态在 `SstPortStatus`，统一启停在 `ChbDabControl` |

归档包含原可执行标记，不代表这些路径仍在固件中生效。不得通过增加 Source entry 或 UART 注册恢复历史逻辑。固定基线标签为 `baseline-pre-framework-2026-10-12`；完整搬迁及归档核对由 `tests/refactor_equivalence.py` 生成。
