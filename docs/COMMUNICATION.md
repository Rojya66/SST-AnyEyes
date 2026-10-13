# 调试通信

通信由运行期主循环中的 `TaskSlice_Service` 显式调度。`SstSerial` 唯一拥有 UART2 寄存器、RS485 方向和整帧发送队列；`SstDebug` 负责所选AnyEyes/VOFA协议。PSC、GPIO 和引脚复用由板级初始化先完成，随后 `SstDebug_Init` 调用 `SstSerial_Init` 配置 UART2 的 115200 8N1 及 FIFO，关闭该 UART 的 RX/TX 中断，不关闭全局中断。快速 TAD 和 FAULT ISR 不发送串口、不解析参数协议、不写 EEPROM。2026-10-13已移除485的A5 5A MaintenanceProtocol，维护改用独立并口DPRAM扩展。

AnyEyes 的 1 ms 时基沿用 `SstBoard_GetTick1ms` 接口，组合实现位于 `CpuTask.c`，读取已有 Timer2 计数和控制核心的 10 ms 软件节拍，不新增中断或改变 TAD 时序。`SstDebug_Service` 保留兼容入口，活动调度不调用该包装，避免重复执行通信和存储服务。

## 构建选择

| 宏 | 默认 | 效果 |
|---|---:|---|
| SST_DEBUG_PROTOCOL | 1 | 1 AnyEyes，2 VOFA，互斥 |
| SST_ENABLE_DPRAM | 1 | 独立启用原固定块与现代并口维护扩展 |
| SST_SIMULATOR_MODE | 0 | 0允许远控；1由仿真器直接控制，普通远程运行命令不接管 |
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

该通信段之前完成原DPRAM遥控/遥测服务，并调用 `SstDpramMaintenance_Service`；扩展发布每轮最多搬运16个数据字及两个版本标记，不忙等、不关闭中断。参数保存位于第二次串口发送之前。VOFA 使用首次发送前捕获的 `now`。该通信段之后才调用后台 `Port_RealTimeParameter`，保留本轮上报观察到的原测量顺序。

每轮最多读取 64 字节；每次发送服务向 FIFO 推进最多 16 字节，一轮调用两次，因此最多推进 32 字节，仍受 FIFO 可用空间和发送完成条件限制，不忙等 FIFO 或 TEMT。8 个整帧 TX 槽，每槽最多 260 字节。入队先完整复制；物理串口严格按整帧顺序发送，GPIO104 在整帧发完且 TEMT 置位后转接收，不在 VOFA、AnyEyes 中分别控制方向。队列满返回背压并计数，遥测为回复留至少一个槽；接收也在回复槽不足时停止本批读取。

AnyEyes帧以6B起始，长度字段包含指令+负载，CRC8覆盖头到负载（poly07/init00/xoroutFF）。接收以完整已识别AnyEyes帧为边界；100ms半帧超时，越界长度丢弃。VOFA构建仅发送JustFloat观测，不解析远程维护请求。A5 5A帧不再分流、执行或生成回复；旧地址式UART/透明EMIF桥没有作为第三协议接入。

AnyEyes和DPRAM均调用统一参数表、命令与存储服务。EEPROM使用页6/7的v2镜像：108字节、50个持久化字，包含37个现代参数、5个持久化命令/给定和8个原保护/模块巡检参数；兼容读取v1的92字节/42字镜像，v1未包含的8项保留当前默认值。run/dab/复位及临时软件保护总使能不保存。后台每10ms最多写1字节，最后提交magic并逐字节回读验证，不屏蔽中断。页4/5旧维护镜像不再作为活动维护服务加载。

DPRAM原遥控块及扩展EXEC的START/STOP、HF/LF_ON/OFF、FAULT_CLEAR通过 `SstCommand` 进入CHB控制命令，不调用旧 `RemoteControl_Dispose` 的直接门极操作。旧手动开关/端口模式不恢复；AC开闭环模式使用现代参数点或AnyEyes组0索引2。CHB运行、DAB使能和复位的单点写仍独立作用，不能把独立使能误解为必然联动。

所有来源通过 `SstParameterStore_IsBusy` 共享统一保存忙状态；`SstParams_IsStorageBusy` 保留兼容查询。启动和修改拒绝或过滤，停止/关脉冲仍有效。EEPROM加载先校验版本、长度、CRC和全部50字（v1为42字）范围，再整体提交，不恢复启停/复位命令，硬件采样周期仅核验。原Block2的V/Q兼容入口只更新旧RAM缓存/FPGA镜像，自动Page1写入已取消，不可据此认定现代给定已变化。

485仅有所选AnyEyes或VOFA格式，DPRAM维护回复不会进入UART队列。总线为RS485半双工，AnyEyes主机应请求-响应并遵守发送间隔；实板应验证收发方向、电气回显和RX溢出计数。

## DPRAM并口地址与覆盖

启用DPRAM并允许远控需 `SST_ENABLE_DPRAM=1,SST_SIMULATOR_MODE=0`，当前源码默认即此配置。保留遥控 `0x660001E0`、遥调 `0x660003FE`、遥测 `0x66000A00`及插件区，按16位字访问；它们不是AnyEyes参数组地址。

尾部新增Request=`0x66002C00`、Response=`0x66002D00`、现代遥测=`0x66002E00`、故障历史=`0x66002F00`。扩展维护覆盖54点：9个命令/总使能、37个现代参数（含PI、快速保护和预充）、8个原软件保护/模块电压巡检参数。五位故障码、等级、首故障和32条历史以现代快照上报。旧公共故障字仍为16位位图，旧PI字段仍不作为现代参数入口。

扩展请求用左邮箱21提交，完整回复用右邮箱21通知；每笔带序号、CRC16和结果码。现代遥测/历史按偶数版本戳读一致快照，不额外覆盖右邮箱。精确地址、R/W、编码和操作步骤见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)；原区见 [DPRAM_MAPPING.md](DPRAM_MAPPING.md)、[DPRAM_ADDRESSES.csv](DPRAM_ADDRESSES.csv)、[DPRAM_BITS.csv](DPRAM_BITS.csv)，原程序对比见 [DPRAM_LEGACY_COMPARISON.md](DPRAM_LEGACY_COMPARISON.md)。

## 验证边界

`tests/protocol_regression.py` 独立编译真实协议、参数表、存储、命令、VOFA、串口和调试模块，仅替换硬件边界；校验CRC、权限、编码、坏帧恢复、范围、存储、RX64/TX16和背压。`serial_protocol_scope.py` 检查485不再执行维护帧，`parameter_store_migration.py` 检查v1/v2镜像兼容；DPRAM扩展另核验点表、序号、CRC、互锁和分片发布。本轮软件结果以 [dpram_maintenance_validation.json](evidence/dpram_maintenance_validation.json) 为准，旧 [software_validation.json](evidence/software_validation.json) 记录先前框架版本，不能代替本轮验证。

编译、链接及假硬件回归不能代替实板通信和时序联调。实际 AnyEyes 客户端、RS485 线缆、C6748 板卡、50 Hz 波形清晰度、EEPROM 掉电读回须按台架步骤核验。参数/波形具体表见 [ANYEYES_MAPPING.md](ANYEYES_MAPPING.md) 和 [ANYEYES_PARAMETERS.csv](ANYEYES_PARAMETERS.csv)。

