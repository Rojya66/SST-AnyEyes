# 调试通信

通信由运行期主循环中的 `TaskSlice_Service` 显式调度。`SstSerial` 唯一拥有 UART2 寄存器、RS485 方向和整帧发送队列；`SstDebug` 负责所选AnyEyes/VOFA协议。PSC、GPIO 和引脚复用由板级初始化先完成，随后 `SstDebug_Init` 调用 `SstSerial_Init` 配置 UART2 的 115200 8N1 及 FIFO，关闭该 UART 的 RX/TX 中断，不关闭全局中断。快速 TAD 和 FAULT ISR 不发送串口、不解析参数协议、不写 EEPROM。2026-10-13已移除485的A5 5A MaintenanceProtocol，维护改用独立并口DPRAM扩展。

AnyEyes 的 1 ms 时基沿用 `SstBoard_GetTick1ms` 接口，组合实现位于 `CpuTask.c`，读取已有 Timer2 计数和控制核心的 10 ms 软件节拍，不新增中断或改变 TAD 时序。`SstDebug_Service` 保留兼容入口，活动调度不调用该包装，避免重复执行通信和存储服务。

## 构建选择

| 宏 | 默认 | 效果 |
|---|---:|---|
| SST_DEBUG_PROTOCOL | 1 | 1 AnyEyes，2 VOFA，互斥 |
| SST_ENABLE_DPRAM | 0 | 当前用户配置关闭；置1独立启用原固定块与现代并口维护扩展 |
| SST_SIMULATOR_MODE | 1 | 当前由仿真器直接控制；置0允许所选远控源接管 |
| SST_BENCH_INPUT_SIM | 1 | 当前用户配置开启临时输入注入；要求SIMULATOR=1、DPRAM=0、EEPROM加载=0 |
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

## 电子标签与握手

AnyEyes首先发送`FF`及4字节电子标签，UNIT回`7F`及`55/AA`结果和6字节版本。四标签和六段版本统一配置在[SoftVersion.h](../Source/04_App/00_Version/SoftVersion.h)，当前标签为十进制`255/43/1/0`，版本为`255.43.1.0.1.0`。这些匹配值按用户确认保留；参考类`CommVersion`实际指硬件平台，`McuType`实际指其他区分标记，不代表SST改用了F28377S硬件。

精确连接请求为`6B 05 FF FF 2B 01 00 48`，成功响应为`6B 08 7F 55 FF 2B 01 00 01 00 85`；全FF通用连接请求为`6B 05 FF FF FF FF FF FD`。业务访问需先连接；握手成功不改变仿真器/DPRAM/串口控制权限。PC建议每1s发送`6B 01 FE 37`维持连接；超过30s没有合法命令则断链并停止通信订阅，再握手才能恢复业务访问，断链不自动停机。

本轮补齐SST扩展`FA/7A`：连接后发送`6B 01 FA 2B`，UNIT以6字节版本应答`6B 07 7A FF 2B 01 00 01 00 C8`。参考工程仅设置FA状态标志、没有7A打包；新增版本查询不改FF/7F及参数、波形、状态地址。详细字段、失败响应、CRC与CCS台架步骤见[ANYEYES_HANDSHAKE.md](ANYEYES_HANDSHAKE.md)。

AnyEyes和DPRAM均调用统一参数表、命令与存储服务。EEPROM使用页6/7的v2镜像：108字节、50个持久化字，包含37个现代参数、5个持久化命令/给定和8个原保护/模块巡检参数；兼容读取v1的92字节/42字镜像，v1未包含的8项保留当前默认值。run/dab/复位及临时软件保护总使能不保存。后台每10ms最多写1字节，最后提交magic并逐字节回读验证，不屏蔽中断。页4/5旧维护镜像不再作为活动维护服务加载。

DPRAM原遥控块及扩展EXEC的START/STOP、HF/LF_ON/OFF、FAULT_CLEAR通过 `SstCommand` 进入CHB控制命令，不调用旧 `RemoteControl_Dispose` 的直接门极操作。旧手动开关/端口模式不恢复；AC开闭环模式使用现代参数点或AnyEyes组0索引2。CHB运行、DAB使能和复位的单点写仍独立作用，不能把独立使能误解为必然联动。

所有来源通过 `SstParameterStore_IsBusy` 共享统一保存忙状态；`SstParams_IsStorageBusy` 保留兼容查询。启动和修改拒绝或过滤，停止/关脉冲仍有效。EEPROM加载先校验版本、长度、CRC和全部50字（v1为42字）范围，再整体提交，不恢复启停/复位命令，硬件采样周期仅核验。原Block2的V/Q兼容入口只更新旧RAM缓存/FPGA镜像，自动Page1写入已取消，不可据此认定现代给定已变化。

485仅有所选AnyEyes或VOFA格式，DPRAM维护回复不会进入UART队列。总线为RS485半双工，AnyEyes主机应请求-响应并遵守发送间隔；实板应验证收发方向、电气回显和RX溢出计数。

## DPRAM并口地址与覆盖

启用DPRAM并允许远控需 `SST_ENABLE_DPRAM=1,SST_SIMULATOR_MODE=0`；当前用户源码为临时输入测试配置`SST_BENCH_INPUT_SIM=1,SST_SIMULATOR_MODE=1,SST_ENABLE_DPRAM=0`，切换并口远控前需关闭测试宏并重新编译。保留遥控 `0x660001E0`、遥调 `0x660003FE`、遥测 `0x66000A00`及插件区，按16位字访问；它们不是AnyEyes参数组地址。

尾部新增Request=`0x66002C00`、Response=`0x66002D00`、现代遥测=`0x66002E00`、故障历史=`0x66002F00`。扩展维护覆盖54点：9个命令/总使能、37个现代参数（含PI、快速保护和预充）、8个原软件保护/模块电压巡检参数。五位故障码、等级、首故障和32条历史以现代快照上报。旧公共故障字仍为16位位图，旧PI字段仍不作为现代参数入口。

扩展请求用左邮箱21提交，完整回复用右邮箱21通知；每笔带序号、CRC16和结果码。现代遥测/历史按偶数版本戳读一致快照，不额外覆盖右邮箱。精确地址、R/W、编码和操作步骤见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)；原区见 [DPRAM_MAPPING.md](DPRAM_MAPPING.md)、[DPRAM_ADDRESSES.csv](DPRAM_ADDRESSES.csv)、[DPRAM_BITS.csv](DPRAM_BITS.csv)，原程序对比见 [DPRAM_LEGACY_COMPARISON.md](DPRAM_LEGACY_COMPARISON.md)。

## 验证边界

`tests/protocol_regression.py` 独立编译真实协议、参数表、存储、命令、VOFA、串口和调试模块，仅替换硬件边界；校验CRC、权限、编码、坏帧恢复、范围、存储、RX64/TX16和背压。`serial_protocol_scope.py` 检查485不再执行维护帧，`parameter_store_migration.py` 检查v1/v2镜像兼容；DPRAM扩展另核验点表、序号、CRC、互锁和分片发布。[dpram_maintenance_validation.json](evidence/dpram_maintenance_validation.json)记录此前并口维护版本，[software_validation.json](evidence/software_validation.json)记录此前框架版本，两者均不能代替本轮电子标签及版本查询验证。

编译、链接及假硬件回归不能代替实板通信和时序联调。实际 AnyEyes 客户端、RS485 线缆、C6748 板卡、50 Hz 波形清晰度、EEPROM 掉电读回须按台架步骤核验。参数/波形具体表见 [ANYEYES_MAPPING.md](ANYEYES_MAPPING.md) 和 [ANYEYES_PARAMETERS.csv](ANYEYES_PARAMETERS.csv)。

