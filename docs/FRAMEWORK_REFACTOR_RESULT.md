# F28377S_Plat 框架整理交付说明

2026-10-13后续并口调整：删除485 MaintenanceProtocol及其宏，默认DPRAM=1、仿真器=0；增加54点维护、现代遥测/历史和16字分片发布，统一存储支持v2/v1镜像。当前接口见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)，当前证据见 [dpram_maintenance_validation.json](evidence/dpram_maintenance_validation.json)。本文历史对象数、token数、宏组合和ISR尺寸表对应2026-10-12框架发布，不作为本轮扩展构建结果。

日期：2026-10-12。工程：`D:/Code/SST/AnyEyesVersion`；CCS 名称仍为 `AnyEyesVersion`，固件名称仍为 `SST_AnyEyes.out`。本次落实 [实施前方案](FRAMEWORK_REFACTOR_PLAN.md) 中的职责划分，不迁入 F28377S 的 PIE、CLA、ePWM 或 C2000 工具链。硬件仍为 C6748 + FPGA。

2026-10-12框架交付完成入口、中断、后台任务、各域数据、初始化、通信、存储和输出控制的职责拆分，以及TI编译、CCS Clean Build和回归。该阶段摘要为 [software_validation.json](evidence/software_validation.json)；当前并口变更使用本页开头的新证据，避免把固定历史版本的日志当作当前结果。

## 1. 阅读入口与职责

| 阅读顺序 | 模块 | 负责内容 |
|---|---|---|
| 1 | `03_System/01_OS/Main.c` | 启动阶段和永久后台循环；没有业务数据定义或 ISR |
| 2 | `SystemInit.c/.h` | 协调硬件、原 20 秒等待及运行初始化，最后开放 TAD/FAULT |
| 3 | `TaskSlice.c/.h` | 一次后台调度的明确顺序，收发预算和存储服务可直接看见 |
| 4 | `CpuTask.c/.h`、`SoftTimeData.c/.h` | 上电标志、看门狗/运行灯及原 Timer2 软件节拍 |
| 5 | `03_System/02_ISR/Redef_Interrupt.c/.h` | 四个中断正文、DSP 中断控制器初始化和中断源注册；快速链保持原名称和调用次序 |
| 6 | `03_System/00_Board/SstBoard.c/.h` | PSC、引脚、EMIFA、GPIO 和 Timer2 硬件配置；毫秒时基的组合实现归 `CpuTask` |

旧 `main_485test.c`、`ChbApplication.c/.h`、`ChbPlatform.c` 从活动源码移除。入口变短的同时，原入口文件中的变量、初始化、中断和通信历史分别获得实际归属；没有建立一个新的全局变量集合文件。

## 2. 启动及后台调用关系

正常构建的启动关系为：

```text
Main.main
  ├─ CpuTask_InitBootFlags
  ├─ SystemInit_Hardware
  │    └─ PSC → 系统解锁 → EMIFA → GPIO → DSP INT → Timer2 → 灯/看门狗
  ├─ SystemInit_WaitPowerOnDelay                  原20秒等待
  ├─ SystemInit_Runtime
  │    ├─ Initial_Variable → Variable_RST
  │    ├─ 原故障锁存清除
  │    ├─ CHB/DAB核心 → 故障报告 → DPRAM扩展 → 通信初始化
  │    ├─ 可选统一SST参数装载
  │    └─ GPIO TAD/FAULT开放 → 可选DPRAM mailbox处理
  └─ for(;;) TaskSlice_Service
```

`CHB_OPEN_LOOP_PULSE_TEST` 保留既有兼容分支，不作为普通开环逆变入口；普通开环仍通过 `g_chb_dab_command.ac_mode=0`。

一次后台调度为：

```text
CpuTask_ServiceHeartbeat
  → ChbDabControl_Service
  → SstFault_Service
  → 可选 SstDpramHost_ServiceRemoteControl / ServiceTelemetry
  → SstDpramMaintenance_Service                  后台16数据字分片
  → 捕获原10ms节拍
  → SstSerial_TxService
  → SstDebug_ReceiveService
  → SstDebug_ProtocolService
  → SstParameterStore_Service
  → SstSerial_TxService
  → Port_RealTimeParameter
```

原先藏在 `SstDebug_Service` 内的 UART、协议及持久化顺序现在由调度文件逐项列出。UART 仍为后台有界轮询：每批最多 64 字节接收、每次最多 16 字节发送，8 个整帧槽；不恢复旧 UART 中断入口。

## 3. 数据所有权与头文件

原主文件中前部的 516 条对象声明（含原同一文件的重复暂定声明）搬到 11 个所属数据模块。对象名字、类型、初值、条件分支及固定 section 保持；普通 RAM 链接地址可能因拆分改变，不能当作稳定通信地址。

| 所属模块 | 数据域 |
|---|---|
| `FpgaRegister` | FPGA 映射窗口、控制字与参考核对量 |
| `Ft3RecorderData` | FT3 映射窗口、录波状态及采样序号 |
| `DpramRegister`、`SstDpramExtension` | 原固定地址对象及4个尾区扩展对象；均NOINIT |
| `DpramHostData` | DPRAM 遥控、遥调、轮询的普通 RAM 缓存 |
| `PortStatusData` | 端口运行、开关流程及兼容镜像 |
| `ControlData` | 旧控制系数、PLL/PI/谐振历史和参考量 |
| `MeasurementData` | 采样、噪声、滤波、RMS/功率窗口及模块电压数据 |
| `ProtectionData` | 使能、阈值、计数、锁存、欠压/高压判断及故障策略 |
| `SoftTimeData` | Timer2 的等待、节拍和指示标志 |
| `TransformData` | 旧坐标变换结果与控制器计算共享状态 |
| `LegacySerialData` | 历史UART兼容符号；不恢复485维护协议或旧ISR |

机器可读的旧声明行号及新所有者见 [REFACTOR_OBJECT_MOVES.json](REFACTOR_OBJECT_MOVES.json)。定义只存在于所属 `.c`；共享访问通过对应 `.h` 的必要 `extern`，不批量改变 `volatile/static/const` 或调试器可见符号。

基础类型归 `SstTypes.h`，算法结构归 `ControlAlgorithmTypes.h`，构建与控制常量归 `ChbBuildConfig.h`、`SstControlConfig.h`，保护默认宏归 `ProtectionConfig.h`。端口业务位图归 `PortStatusTypes.h`，故障业务位图归 `ProtectionTypes.h`。

`XXG_MainStatus`、`XXG_PORT_STATUS`、`XXG_PORT_FAULT` 同时是 FPGA 寄存器或 DPRAM 线上字段，完整放在 `01_ComLib/SstWireTypes.h`；该窄头仅定义字段，既不拥有硬件对象也不包含应用状态。外围布局和应用类型头单向包含它，硬件层不反向依赖 App。全部字段宽度、顺序和位域表达式保持。

`main.H` 保留为兼容聚合头，活动源码不引用它。`PluginUnit.H`、`Stateflow.H` 保留窄兼容入口，新的调用者直接使用 `FpgaIo/FpgaModulation/Ft3Recorder/DacOutput` 或 `SstPortStatus`。兼容头不包含函数正文、不定义对象。

## 4. 初始化按原顺序分域

`ControlInit` 保留用户及旧调用路径需要的 `Initial_Variable`、`Variable_RST` 符号，正文成为有序协调。不是把全部赋值换个文件继续集中维护，也不是按模块名重新排序原初始化。

`Initial_Variable` 的有序段如下：

1. `RuntimeStateInit_ResetObservationAndModes`
2. `RuntimeStateInit_ResetTimeFlags`
3. `ProtectionDefaults_ResetIndicators`
4. `RuntimeStateInit_ResetLegacySerial`
5. `FpgaInit_SetSafeOutputs`
6. `PortStatusInit_DefaultState`
7. `DpramDefaults_InitRegulation`
8. `FpgaInit_ConfigureModuleLayout`
9. `ControlDefaults_InitLimits`
10. `ControlDefaults_InitCoefficients`
11. `FpgaInit_InitTimingDuty`
12. `FpgaInit_InitOutputControls`
13. `MeasurementInit_ResetFilters`
14. `ControlDefaults_InitPllState`
15. `MeasurementInit_ResetAverages`
16. `FpgaInit_InitRecorderFanStatus`

每段保留原语句、表达式与循环，同一所属域的不同时间段保持分开调用。`Variable_RST` 转入 `ControlDefaults_ResetRegulators`，仍保留启动之外的原 PWM 开锁调用语义；不能改为仅上电执行。

初始化等价审计按上述调用顺序展开全部域函数：`Initial_Variable` 的 3,120 个执行 token 和 14 条预处理指令、`Variable_RST` 的 175 个执行 token 和 2 条预处理指令均与 Git 基线严格一致。局部变量只在所属段内使用，类型保持；审计拒绝跨段传值、增加局部初始化或调整原语句次序。

## 5. 外围、控制、通信及保护边界

| 原综合模块 | 本轮实际拆分 |
|---|---|
| `PluginUnit.C` | `FpgaIo` 维护开关/硬件输入输出；`FpgaModulation` 维护参考/调制；`Ft3Recorder` 维护录波；`DacOutput` 维护 DAC |
| `Stateflow.C` | 活动端口状态及联锁归 `SstPortStatus`，非活动旧启动流程在 `Source` 外归档 |
| `FaultCheck.C` 的正常 PWM 动作 | `PwmGate` 维护安全初始化、CHB/DAB 开锁及封锁；快速故障检测留 `FaultCheck` |
| `SstDebug` | 协议选择、分流及服务；UART/RS485寄存器、队列和有界收发归 `SstSerial` |
| `SstParams` | 固定地址读取/写入权限入口；唯一参数表归 `SstParameterTable`，显示/波形归 `PcDebugDispData`，存储事务归 `SstParameterStore` |
| DPRAM 应用处理 | `SstDpramHost`负责原区兼容；`SstDpramMaintenance`负责54点事务与分片快照；`DPRAM/SstDpramExtension`提供固定映射 |

`ChbDabControl` 保持控制算法和私有历史；`SstCommand`统一AnyEyes、原DPRAM及并口扩展的命令策略、停机判断和急停。参数单点写明确目标、权限及存储互锁，不恢复旧遥控的直接门极/开关操作。`SstLegacyParameters`提供原保护和模块巡检8点，`SstParameterStore`统一显式保存现代及原参数。快速保护仍在原快链动作，`SstFault`在后台整理五位码、等级、当前/历史及上报，不把通信、存储或故障字符串放进FPGA中断。

AnyEyes/VOFA仍二选一、DPRAM独立开关；保留仿真器、开环、闭环、保存互锁及STOP优先。当前默认改为AnyEyes+DPRAM远控；485维护协议删除，VOFA构建仍可并口维护。PcDebug命令、地址、200字显示表和16波形通道保持，DPRAM新增区独立定义，不重新分配AnyEyes地址。

## 6. 历史代码与注释

历史 UART/按键、非活动旧启动路径位于 `archive/legacy/`，不置于 `Source` 内。构建脚本递归枚举 `Source`；归档 `.c` 不进入固件。归档原因和保留路径随等价审计说明，不能以归档名为理由恢复其原中断或直接门极控制。

所有自定义函数按参考工程保留“函数功能、输入参数、输出参数、返回值、执行说明”注释，变量及字段保留真实中文说明。多变量声明的每个具名尾注紧邻声明；条件编译不能把说明隔到下一段。新增协调接口注释写明执行上下文及副作用。

后续维护直接修改所属源码模块。构建和审计工具读取当前文件及固定 Git 基线，不把历史缓存自动覆盖回工程；工具职责和复核方法见 [REFACTOR_VALIDATION_TOOLS.md](REFACTOR_VALIDATION_TOOLS.md)。

## 7. 历史框架验证结果与适用边界

| 验证项 | 结果 | 证据位置 |
|---|---|---|
| 原主文件 516 条对象、条件及初值保持 | 通过；新增接口变化有独立合同及回归 | `docs/REFACTOR_OBJECT_MOVES.json`、`build/refactor_equivalence.json` |
| 三个共享线上位域类型保持 | 通过，与 Git 基线执行 token 一致 | `SstWireTypes.h`、源等价报告 |
| 自定义函数、变量及字段中文注释 | 通过；覆盖范围和最终计数随源码指纹记录 | `build/comment_inventory/verification.json` |
| 模块边界及独立 C/C++ 头编译 | 通过；公开头无包含环，无隐式函数声明 | `build/framework_structure/result.json` |
| TI Debug / Release、11 种宏配置 | 通过；每个构建编译 85 个活动源文件 | `build/Debug/`、`build/Release/`、`build/matrix_result.json` |
| 实际 CCS 导入及 Debug / Release Clean Build | 通过；清理及构建均返回 0 | `build/ccs_validation/result.json` |
| 协议、命令、存储、故障、控制及结构回归 | 通过 | `build/host_validation/result.json` |
| 初始化与真实启动/后台/ISR 假硬件轨迹 | 通过；初始化严格展开等价，3 种配置各 17 个场景 | `build/control_init_equivalence.json`、`build/firmware_trace/result.json` |
| 四个 ISR 指令、直接调用、栈及函数尺寸 | 通过；同一 TI Debug 配置对照基线 | `build/isr_binary_audit/result.json` |
| 快链源码、向量及 DPRAM 固定映射 | 通过 | 源等价报告、实时审计及 `build/delivery_manifest.json` |

最终各项计数、构建输入指纹及固件哈希以 [随版本验证摘要](evidence/software_validation.json) 和 `build/delivery_manifest.json` 为准。完整日志与固件属于可重新生成产物，不提交生成的 `build/Debug/Release` 目录。

源等价检查基于固定 Git 标签，按函数、对象、类型、宏及硬件 section 的执行标记和条件上下文比较。机械搬迁无需豁免；接口/协调变化需列出具体原因、旧新精确哈希及相应回归证据。`FPGAIsr/FAULTIsr/Timer2Isr/DPRAMIsr`、TAD常量和硬件section不能通过例外掩盖变化。初始化分段还需核对展开后的原语句顺序。

ISR 二进制审计比较 TI 生成的符号汇编指令流，只规范化编译器局部标签、排除调试指令，保留操作数及调用符号。四个函数的指令流、直接调用顺序、栈帧和 ELF 尺寸均一致：

| ISR | 基线/重构函数尺寸（字节） | 基线/重构栈帧（字节） |
|---|---:|---:|
| `FPGAIsr` | 572 / 572 | 8 / 8 |
| `FAULTIsr` | 132 / 132 | 8 / 8 |
| `Timer2Isr` | 1,104 / 1,104 | 8 / 8 |
| `DPRAMIsr` | 104 / 104 | 8 / 8 |

文件拆分改变普通 RAM 和函数链接位置，因此已链接 ELF 的字节哈希允许因地址重定位而不同；审计不把地址差异当作算法改变，也不声称固件二进制完全相同。软件编译和上述对照不证明物理 100 μs 周期、外部触发到响应延迟或中断最大执行时间。TI 原入口的非嵌套语义继续保留，实际板卡按 [BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md) 测量重构前后快链、Timer2 和故障最大时序。

## 8. GitHub版本标记

专用私有仓库：[Rojya66/SST-AnyEyes](https://github.com/Rojya66/SST-AnyEyes)。

| 版本 | 名称 | 用途 |
|---|---|---|
| 重构前程序 | `baseline-pre-framework-2026-10-12` | 固定原始版本，提交 `ddaeb56f21a81a70cdb76085728f91b67dc3c7a0` |
| 框架整理 | `refactor/f28377s-framework` 分支 | 本次重构及后续维护 |
| 重构发布版本 | `framework-refactored-2026-10-12` | 本次验证通过的交付标签 |

基线标签固定，不改名覆盖。源码提交包含轻量验证摘要，不包含生成的 `build/Debug/Release` 目录；构建日志及固件由当前版本命令生成。
