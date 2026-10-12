# 调试通信

只允许runtime主循环调用 `SstDebug_Service`，它统一拥有UART2。PSC/GPIO/引脚复用由原board初始化先完成；`SstDebug_Init`配置UART2时钟/115200 8N1并关闭该UART RX/TX中断，不关闭全局中断。快速TAD和FAULT ISR不发送串口、不读参数协议、不写EEPROM。AnyEyes 1ms时基调用 `SstBoard_GetTick1ms`，读取已有Timer2计数器，不新增中断或改变TAD时序。

## 构建选择

| 宏 | 默认 | 效果 |
|---|---:|---|
| SST_DEBUG_PROTOCOL | 1 | 1 AnyEyes，2 VOFA，互斥 |
| SST_ENABLE_MAINTENANCE | 0 | 1增加A5 5A维护协议分流 |
| SST_ENABLE_DPRAM | 0 | 独立保留DPRAM通路 |
| SST_SIMULATOR_MODE | 1 | 模拟器直接控制，串口启动/停止/复位指令不接管 |
| SST_ALLOW_SERIAL_COMMANDS | 派生 | !SIMULATOR && !DPRAM；DPRAM启用时为其保留控制权 |
| SST_UART_BAUD | 115200 | UART与波形限频使用同一值 |
| SST_UART_INPUT_CLOCK_HZ | 228000000 | 保留原PLL实际UART2输入时钟 |
| SST_TELEMETRY_PERIOD_10MS | 2 | VOFA默认50Hz |
| CHB_LOAD_EEPROM_PARAMETERS | 0 | 可显式启用上电载入已保存SST参数 |

默认输出 AnyEyes 协议，不能同时在同一串口连续输出VOFA JustFloat。VOFA模式保留原4通道顺序与语义：`PORT_RealTime_DATA.PORT_V_A`、`g_chb_dab_status.dc_voltage_average_v`、`(float)Port_State`、`g_chb_dab_status.pll_angle_rad`；每通道IEEE float32小端，尾00 00 80 7F，总20字节。原VOFA_Init/Process保留空兼容接口，不再直接访问UART。

## 调度与帧仲裁

每次service最多读64字节；发FIFO每次最多16字节（调用起止各一次，最多32字节/service），不会等待FIFO或TEMT。8个整帧TX槽，每槽最多260字节。入队先完整复制；物理串口严格按整帧FIFO发送，GPIO104在整帧发完且TEMT置位后转接收，不在VOFA/维护/AnyEyes里各自控制方向。队列满返回背压并计数，遥测为回复留至少一个槽；接收也在回复槽不足时停止当次读取。

AnyEyes帧以6B起始，长度字段包含指令+负载，CRC8覆盖头到负载（poly07/init00/xoroutFF）。维护帧以A5 5A起始，版本01、flags、sequence、command、16位小端length、payload、CRC16/IBM。RX分流以**完整已识别帧**为边界，负载中的6B/A5/5A不会触发另一协议；100ms半帧超时，越界长度丢弃。维护帧先完整收齐才提交原解析器，避免半维护帧污染后续帧。AnyEyes/VOFA与维护回复进入同一TX队列。

`SST_ENABLE_MAINTENANCE=1`保留旧对象/旧参数语义及A5 5A协议；其旧参数并不自动等于新的CHB参数表。控制/参数写需停机和对应控制源授权。旧维护显式SAVE仍用4/5页，后台每10ms写1字节、提交magic后回读，再发送原完成回复；不屏蔽中断。6/7页为SST参数独立版本图像，不覆盖旧维护参数。旧地址式UART/透明EMIF桥没有作为第三协议接入；必须使用AnyEyes或维护帧控制，避免多个UART消费者。

维护START/STOP、HF/LF_ON/OFF与FAULT_CLEAR只进入现有CHB控制命令，关闭旧RemoteControl_Dispose直接门极操作；旧MODE/manual SWITCH命令返回不允许，AC开闭环模式用AnyEyes 0/2。维护CONTROL对象写仍接收旧字段，但只消费启停/脉冲/清故障字段。保存期间所有来源共享SstParams_IsStorageBusy互锁，启动/修改拒绝或过滤，stop/off仍可执行。旧PARAMETER/REGULATE/PLUGIN写仅停机且无保存任务时接受。

启用两种回复格式意味着PC端须能识别两者；单协议VOFA/AnyEyes客户端不能保证识别另一种回复，只有发送相应维护请求才会出现维护响应。总线为RS485半双工，主机应请求-响应并遵守发送间隔，不能在从机连续发送期间无条件灌入；实板应验证收发方向、电气回显和RX溢出计数。

## 验证边界

`tests/protocol_regression.py`编译真实协议、参数、VOFA、TX/RX调度代码，替换硬件寄存器/EEPROM，验证CRC、握手与响应字节、权限、参数缩放、坏帧恢复、范围、保存CRC和非阻塞整帧TX。编译器/链接检查只说明软件可构建；实际AnyEyes客户端工程、RS485线缆、C6748板卡、50Hz波形清晰度、EEPROM掉电读回仍须台架联调。参数/波形具体表见 [ANYEYES_MAPPING.md](ANYEYES_MAPPING.md) 和 [ANYEYES_PARAMETERS.csv](ANYEYES_PARAMETERS.csv)。

