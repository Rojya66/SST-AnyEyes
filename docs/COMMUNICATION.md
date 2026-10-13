# 调试通信

通信由运行期主循环中的 `TaskSlice_Service` 显式调度。`SstSerial` 唯一拥有 UART2 寄存器、RS485 方向和整帧发送队列；`SstDebug` 负责所选协议协调和字节分流。PSC、GPIO 和引脚复用由板级初始化先完成，随后 `SstDebug_Init` 调用 `SstSerial_Init` 配置 UART2 的 115200 8N1 及 FIFO，关闭该 UART 的 RX/TX 中断，不关闭全局中断。快速 TAD 和 FAULT ISR 不发送串口、不解析参数协议、不写 EEPROM。

AnyEyes 的 1 ms 时基沿用 `SstBoard_GetTick1ms` 接口，组合实现位于 `CpuTask.c`，读取已有 Timer2 计数和控制核心的 10 ms 软件节拍，不新增中断或改变 TAD 时序。`SstDebug_Service` 保留兼容入口，活动调度不调用该包装，避免重复执行通信和存储服务。

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

`TaskSlice_Service` 的通信段顺序为：

```text
捕获本轮10ms节拍 now
  → SstSerial_TxService
  → SstDebug_ReceiveService
  → SstDebug_ProtocolService(now)
  → SstParameterStore_Service
  → SstSerial_TxService
```

维护协议处理在所选 AnyEyes/VOFA 服务之前，参数保存仍位于第二次发送之前。VOFA 使用首次发送前捕获的 `now`，不因分层重新取时而改变周期边界。该通信段之后才调用后台 `Port_RealTimeParameter`，保留本轮上报观察到的原测量顺序。

每轮最多读取 64 字节；每次发送服务向 FIFO 推进最多 16 字节，一轮调用两次，因此最多推进 32 字节，仍受 FIFO 可用空间和发送完成条件限制，不忙等 FIFO 或 TEMT。8 个整帧 TX 槽，每槽最多 260 字节。入队先完整复制；物理串口严格按整帧顺序发送，GPIO104 在整帧发完且 TEMT 置位后转接收，不在 VOFA、维护、AnyEyes 中分别控制方向。队列满返回背压并计数，遥测为回复留至少一个槽；接收也在回复槽不足时停止本批读取。

AnyEyes帧以6B起始，长度字段包含指令+负载，CRC8覆盖头到负载（poly07/init00/xoroutFF）。维护帧以A5 5A起始，版本01、flags、sequence、command、16位小端length、payload、CRC16/IBM。RX分流以**完整已识别帧**为边界，负载中的6B/A5/5A不会触发另一协议；100ms半帧超时，越界长度丢弃。维护帧先完整收齐才提交原解析器，避免半维护帧污染后续帧。AnyEyes/VOFA与维护回复进入同一TX队列。

`SST_ENABLE_MAINTENANCE=1` 保留旧对象/旧参数语义及 A5 5A 协议；其旧参数并不自动等于新的 CHB 参数表。参数写和故障复位检查停机条件，启停及脉冲命令按对应控制源权限和原互锁处理。旧维护显式 SAVE 仍用页4/5，后台每 10 ms 写1字节、提交 magic 后回读，再发送原完成回复；不屏蔽中断。页6/7为 SST 参数独立版本图像，不覆盖旧维护参数。旧地址式 UART/透明 EMIF 桥没有作为第三协议接入；使用 AnyEyes 或维护帧控制，避免多个 UART 消费者。

维护 START/STOP、HF/LF_ON/OFF、FAULT_CLEAR 和 CONTROL 对象中的启停/脉冲/清故障字段通过 `SstCommand` 的原策略进入 CHB 控制命令，不调用旧 `RemoteControl_Dispose` 的直接门极操作。旧 MODE/manual SWITCH 命令返回不允许，AC 开闭环模式用 AnyEyes 组0索引2。固定参数表的 CHB 运行、DAB 使能和复位写入也转交窄命令接口，两个使能字段仍独立写入，未改为联动操作。

所有来源通过 `SstParameterStore_IsBusy` 共享 SST 参数保存与可选维护保存的忙状态；`SstParams_IsStorageBusy` 保留兼容查询。启动和修改拒绝或过滤，停止/关脉冲保留原优先规则。旧 PARAMETER/REGULATE/PLUGIN 写仅停机且无保存任务时接受。EEPROM 加载在格式、CRC 和 42 字范围整体校验成功后内部提交，不逐项调用公开参数写接口，不恢复启停/复位命令，硬件采样周期仅核验。

启用两种回复格式意味着PC端须能识别两者；单协议VOFA/AnyEyes客户端不能保证识别另一种回复，只有发送相应维护请求才会出现维护响应。总线为RS485半双工，主机应请求-响应并遵守发送间隔，不能在从机连续发送期间无条件灌入；实板应验证收发方向、电气回显和RX溢出计数。

## DPRAM并口地址与覆盖

启用DPRAM并允许远控需 `SST_ENABLE_DPRAM=1,SST_SIMULATOR_MODE=0`。当前并口仍为旧固定块及邮箱协议：遥控 `0x660001E0`、遥调 `0x660003FE`、遥测 `0x66000A00`，按16位字访问；不是AnyEyes参数组地址。

当前启停、CHB/DAB脉冲与停机故障复位已接入。旧遥调块虽然定义PI及保护使能字段，其当前消费代码被注释；新的PI、保护及预充参数没有DPRAM映射。五位SST故障码/等级/历史也未写入DPRAM，其公共故障字段仍是旧16位位图。旧 `PORT_V_SET/STATCOM_Q_SET`只改变旧缓存和FPGA镜像，不更新现代CHB直流目标/Iq给定。

完整逐字地址、主机R/W方向、编码、有效字段和缺口见 [DPRAM_MAPPING.md](DPRAM_MAPPING.md)、[DPRAM_ADDRESSES.csv](DPRAM_ADDRESSES.csv) 和 [DPRAM_BITS.csv](DPRAM_BITS.csv)。

## 验证边界

`tests/protocol_regression.py` 分别编译真实协议、参数表、存储、命令、VOFA、串口和调试模块再链接，仅替换硬件寄存器和 EEPROM 边界，验证 CRC、握手与响应字节、权限、参数缩放、坏帧恢复、范围、保存 CRC、RX64/TX16、背压及非阻塞整帧发送。维护、命令及参数精度另有回归；启动/调度/ISR 轨迹验证默认、DPRAM、EEPROM+维护 3 种配置各 17 个场景。最终软件证据见 [software_validation.json](evidence/software_validation.json) 和 `build/delivery_manifest.json`。

编译、链接及假硬件回归不能代替实板通信和时序联调。实际 AnyEyes 客户端、RS485 线缆、C6748 板卡、50 Hz 波形清晰度、EEPROM 掉电读回须按台架步骤核验。参数/波形具体表见 [ANYEYES_MAPPING.md](ANYEYES_MAPPING.md) 和 [ANYEYES_PARAMETERS.csv](ANYEYES_PARAMETERS.csv)。

