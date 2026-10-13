# 迁移与架构说明

## 来源与目标

功能来源：`D:/Code/SST/Chb_DabPrim_vofa`。框架与 PcDebug 来源：`D:/Code/CCSv12/F28377S_Plat`。新工程：`D:/Code/SST/AnyEyesVersion`。原两个工程未修改。

项目保留 TMS320C6748、C6000 8.3.12、FPGA 的采样/PWM/保护接口、EMIFA 16 位总线、GPIO69 的 TAD 中断和原寄存器链接映射。参考框架的 C2000 启动、CLA、ePWM、PIE、库和链接文件不属于这个工程。

## 模块分层及迁移对应

| 新目录 | 源文件或内容 | 职责 |
|---|---|---|
| `00_Device` | include、system_config | 器件定义、DSP 中断分发与汇编向量 |
| `01_Drive` | drivers | TI 原平台驱动 |
| `01_ComLib` | CommonProgram、SstTypes、SstWireTypes、ControlAlgorithmTypes、TransformData | 公共算法、基础类型、共享线上字段、坐标变换兼容状态 |
| `02_Peripheral/00_FPGA` | FpgaRegister、Ft3RecorderData、FpgaIo、FpgaModulation、Ft3Recorder、DacOutput | 固定寄存器布局、映射对象与按职责分开的外围访问 |
| `02_Peripheral/01_DPRAM` | DpramRegister、DpramConfig、DPRAM、SstDpramExtension | 既有地址布局、映射对象、扩展固定区与块读写 |
| `02_Peripheral/02_Storage` | EEProm、X1226_RW | 原 EEPROM/RTC 硬件接口 |
| `02_Peripheral/03_Serial` | SstSerial | UART2/RS485、整帧队列、有界收发 |
| `03_System/00_Board` | SstBoard、Hardware、EMIFAPinmuxSetup | PSC、引脚、EMIFA、GPIO及Timer2硬件配置 |
| `03_System/01_OS` | Main、SystemInit、TaskSlice、CpuTask、SoftTimeData | 程序入口、启动顺序、后台顺序、心跳、毫秒时基与软件节拍 |
| `03_System/02_ISR` | Redef_Interrupt | 中断注册/映射及Timer2、FPGA、FAULT、DPRAM的原中断正文 |
| `04_App/00_Config` | ChbBuildConfig、SstControlConfig；main.H 仅兼容 | 构建选择与控制常量；活动模块不依赖万能头 |
| `04_App/01_FuncCode` | SstParams、SstParameterTable、SstParameterStore、SstLegacyParameters、SstParamTypes | 固定地址权限、现代/原保护参数和 EEPROM 事务 |
| `04_App/02_Measurement` | ADC_Sample、MeasurementData、MeasurementInit、MeasurementService | 标定、快速采样、滤波/RMS数据、初始化与后台整理 |
| `04_App/03_PcDebug` | Comm2PcDebug、SstDebug、PcDebugDispData、VOFA、LegacySerialData | AnyEyes/VOFA协议选择、显示/波形映射、历史兼容符号 |
| `04_App/04_Control` | ChbDabControl、HVAC_Strategy、ControlData、ControlInit、PortStatusData、SstPortStatus、PwmGate | CHB/DAB 核心、控制历史、初始化、端口联锁及 PWM 动作 |
| `04_App/08_Fault` | FaultCheck、ModuleVoltageCheck、ProtectionData/Types/Config、SstFault | 快速保护、模块电压巡检、保护数据与后台报告 |
| `04_App/10_Host` | SstDpramHost、SstDpramMaintenance、DpramHostData | DPRAM旧区兼容、54点维护事务及分片快照 |

重构前首版仍把数据和中断放在原主文件；本轮按 [FRAMEWORK_REFACTOR_PLAN.md](FRAMEWORK_REFACTOR_PLAN.md) 将这些职责归回所属模块。原 `main_485test.c` 从活动源码移除，必要数据保留原符号、类型、初值和硬件布局；不用另建一个全局变量集合文件替代它。`main.H` 仅供历史兼容，活动模块使用明确的类型、数据及函数接口。原控制算法不改为新的对象模型，也不批量修改 `volatile/static` 语义。

初始化实现进一步拆为 `RuntimeStateInit`、`ProtectionDefaults`、`FpgaInit`、`PortStatusInit`、`DpramDefaults`、`ControlDefaults`、`MeasurementInit`，由 `ControlInit` 按原有序段协调；每段保持原赋值及循环表达式。实际目录、旧文件到新文件对应及当次验证状态见 [FRAMEWORK_REFACTOR_RESULT.md](FRAMEWORK_REFACTOR_RESULT.md)。本文不把首版测试结果当作重构版本已经验证的证据。

## 初始化和执行顺序

`Main.main` → `CpuTask_InitBootFlags` → `SystemInit_Hardware` → `SystemInit_WaitPowerOnDelay` → `SystemInit_Runtime` → 无限调用 `TaskSlice_Service`。

硬件阶段保持PSC/引脚/EMIFA/Timer2和指示灯初值；上电等待保持20秒。运行阶段完成变量初始化与复位、故障锁存清除、CHB/DAB核心、故障报告、DPRAM扩展与通信初始化、可选SST参数加载，最后开放GPIO TAD/FAULT中断。原有域初始化仍按原有序段执行，同一域多次出现时保持多个有序段。

运行对象与安全输出在 TAD/FAULT 注册前初始化。Timer2 在上电等待期间保持原 10 ms 节拍。通信不在等待期间执行启停。旧485维护页4/5的加载及协议已移除；统一SST参数镜像在核心初始化后可选加载，避免被默认初始化覆盖。v2镜像同时保存现代参数和8项原保护/模块巡检参数。

`TaskSlice_Service` 显式列出：看门狗/LED → CHB/DAB慢状态机 → 故障汇总 → 可选DPRAM遥控/遥测 → DPRAM维护分片服务 → 捕获本轮原10ms节拍 → 串口发送 → 有界接收 → 所选调试协议 → 参数持久化 → 串口发送 → 原实时参数整理。DPRAM扩展每轮至多搬运16个数据字及两个版本标记；CRC、维护、快照和持久化均在后台。

## 保留的控制行为

- CHB 开环：默认 50 Hz、调制比 0.20，直接启停，保持原独立 CHB/DAB 调试路径。
- 闭环：停机 → 预充 → 旁路延时 → PLL 等待 → 运行，故障进入锁存停机。
- 两种闭环启动策略：兼容原版和 Id 斜坡后接管电压环。默认使用当前源代码的兼容策略。
- 保留电压外环、dq 电流内环、前馈、增益符号、反馈极性、相位补偿和限幅。
- DAB 原边：初始占空计数 350，目标 2500，每个 TAD 增加 1，按原使能与停机规则执行。
- 原独立硬件故障路径不受软件保护调试开关影响；开环的软件 OC/OV 条件保持原行为。
- 上电 `run_enable=0`、`dab_enable=0`，故障复位不自动运行。

### 必须按当前硬件配置核对

当前源配置 A1=0x0001、A2=0，B/C 两组分别 0x007F/0x003F，即 A 相 1 个、B/C 相各 13 个。迁移保留这些位图，整机模块数需要按实际安装修改 `ChbDabControl.h`，不可依据旧文档假设三相均为 13 个。

当前所谓最低直流电压取三相相均值的最小值，未增加每个单元最低值检测。原预充、断路器和旁路接触器输出仍遵循源代码 IO 映射。

## 10 kHz 约束

`FPGAIsr` 归 `Redef_Interrupt.c`，保持原采样 → 计算 → 控制 → 保护 → 状态/录波 → DAC 链路。迁移首版已删除原 VOFA 降采样计数和通知标志，通信依赖后台时基；本轮框架整理以首版 Git 基线为比较对象，完整保留中断正文。INT5 映射 GPIO Bank4，GPIO69 上升沿触发保持一致。DPRAM关闭时，既有FT3录波中的故障字从本拍本地`Port_RealFault`读取，避免已关闭的CS5访问；DPRAM启用时来源保持原代码。

新通信不注册 UART RX/TX 中断，主循环限预算轮询 UART2；不会新增低优先级 UART ISR 阻挡 TAD。新故障编码、历史与报文处理不加入 TAD。新后台运行路径不全局禁止 DSP 中断。

TI 原中断入口不自动允许低优先级 ISR 内被高优先级抢占，因此原 Timer2/FAULT ISR 的最大执行时间仍需板级核验。软件回归及编译确认结构与功能等价，不能证明物理外部触发的最大延迟。测量要求见 BOARD_ACCEPTANCE.md。

## 地址与兼容约束

- `C6748.cmd` 原 FPGA/DPRAM section 地址与位域结构保持。
- PcDebug 帧头 0x6B，F0～FF 请求、70～7F 应答、CRC8、功能码组和组内序号、显示索引维持参考格式。
- 将参考 C28x 的依赖类型替换为明确的 16 位类型，传输显式编码高低字节，禁止通过结构体原始内存迁移通信。
- 波形与状态使用 16 位传输的观测适配，浮点控制参数以映射表中的量纲/比例接入。
- VOFA 使用原小端 float32 和 JustFloat 帧尾，默认后台限频，发送结束等待改为非阻塞检查。
- DPRAM 的启用和控制权限由独立宏明确；仿真器模式不接受 DPRAM 或串口远程启动。
- 原CS5地址对象保持链接地址；原区及4个尾区扩展section均为`NOINIT`，避免C运行库在EMIFA初始化前清零外部通信区。DPRAM关闭时后台不读写、GPIO中断不注册；启用后在后台初始化/收发。普通RAM中的`PORT_*_Buf`仍初始化，串口不依赖映射DPRAM对象。

DPRAM控制块仍使用原地址及字位图：START/STOP的bit0，PLUSE_ON/OFF的bit0为DAB、bit1为CHB，FAULT_CLEAR的bit0请求停机复位；其余位保留。STOP优先于START，OFF优先于同一路ON。`SstCommand`只修改核心命令，PWM由原控制状态机驱动。旧直接开关/门极入口不恢复；REGULATE的V/Q仅更新旧缓存及FPGA镜像，取消自动EEPROM Page1保存。现代调参通过AnyEyes、DPRAM54点扩展或仿真器，详见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)。

AnyEyes与DPRAM使用同一参数存储忙状态；存储期间启动和修改拒绝/过滤，停止和封波仍有效。新显式SAVE使用页6/7的v2镜像108字节/50字，兼容读取v1的92字节/42字，不保存启停、复位和临时软件保护总使能。仿真器直接写RAM无法拦截；保存时检测到运行则中止并报告忙。

2026-10-13调整默认配置为AnyEyes、DPRAM=1、仿真器=0；485只走AnyEyes或VOFA，切换VOFA仍可并口维护。原并口能力对比见 [DPRAM_LEGACY_COMPARISON.md](DPRAM_LEGACY_COMPARISON.md)。当前变更的验证以 [dpram_maintenance_validation.json](evidence/dpram_maintenance_validation.json) 为准；先前框架证据描述固定历史版本。

## 验证层次和交付边界

已提供 TI 编译脚本、协议测试、故障测试、原/新算法对照测试及实时链路静态检查。当前构建结果与软件测试记录见 VALIDATION.md。板级验收包含外部中断延迟、输出引脚、EEPROM、AnyEyes 客户端和故障注入，需按 BOARD_ACCEPTANCE.md 实施后记录实测值。
