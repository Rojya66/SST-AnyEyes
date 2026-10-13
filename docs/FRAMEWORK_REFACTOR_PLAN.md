# SST 工程按 F28377S_Plat 框架整理的完整方案

本文是2026-10-12实施前的历史规划，已由 [框架交付说明](FRAMEWORK_REFACTOR_RESULT.md) 记录实施结果。2026-10-13进一步移除485 MaintenanceProtocol，维护恢复为并口并新增54点/现代快照，默认DPRAM=1、仿真器=0；当前通信、地址和配置以 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)、[DPRAM_MAPPING.md](DPRAM_MAPPING.md)、[COMMUNICATION.md](COMMUNICATION.md) 为准，不按本文旧维护协议或旧默认值接线/配置。

审查日期：2026-10-12。对象：`D:/Code/SST/AnyEyesVersion`。参考：`D:/Code/CCSv12/F28377S_Plat`。

本文是实施前方案，本轮没有修改固件源码、工程配置或重新生成固件。文件及函数行号来自本次读取的当前代码；词法清单与源码 SHA256 见 [FRAMEWORK_REFACTOR_INVENTORY.json](/D:/Code/SST/AnyEyesVersion/docs/FRAMEWORK_REFACTOR_INVENTORY.json)。清单涵盖 46 个自定义源文件/头文件，排除 TI 的 `00_Device`、`01_Drive`；其中包含条件编译分支，不能把文本统计当作活动代码数量。

## 1. 总体判断与调整目标

用户提出的问题成立。现有工程完成了六层目录组织、功能迁移和部分接口适配，但仍保留原 SST 的集中变量定义、万能头文件和跨模块直接访问。增加 `ChbApplication` 后，入口函数变短了，许多内部职责却仍在原主文件或新的综合服务文件里。

需要落实参考工程的三个核心原则：

1. 入口、中断、后台调度、硬件访问和业务逻辑分别拥有明确职责。
2. 每组数据由一个模块定义和初始化；其他模块只依赖明确声明的必要接口。
3. 从主程序和任务调度文件就能读出启动顺序、后台顺序和快慢任务关系，不必先遍历几千行全局代码。

保留 C6748 + FPGA、现有 C6000 工具链和现有 C/C++ 混合工程。沿用参考工程的职责划分、接口与注释习惯；不要求把全部 C 模块改成 C++ 类。现有 PcDebug C++ 协议模块可以继续使用。

这次重构的第一目标是改善组织方式并保持行为。算法、硬件布局、采样周期、后台周期、保护策略、参数范围的改变必须另列变更，不能夹在“整理框架”中。

## 2. 参考工程实际上提供了什么

| 框架规则 | 参考工程证据 | 在 SST 的应用 |
|---|---|---|
| Main 只协调启动和运行 | [Main.cpp:59](/D:/Code/CCSv12/F28377S_Plat/Source/03_System/01_OS/Main.cpp:59) | 独立 `Main.c`，不再定义其他模块的数据或 ISR |
| 后台任务明确列出，调度规则集中 | [TaskSlice.cpp:90](/D:/Code/CCSv12/F28377S_Plat/Source/03_System/01_OS/TaskSlice.cpp:90) | 独立 `TaskSlice.c/.h`，保留 SST 原服务顺序 |
| 时基、任务对象和依赖具有归属 | [TaskSlice.hpp:60](/D:/Code/CCSv12/F28377S_Plat/Source/03_System/01_OS/TaskSlice.hpp:60) | 用 C 模块私有状态和必要接口表达同样边界 |
| CPU、看门狗等系统任务独立 | [CpuTask.cpp:172](/D:/Code/CCSv12/F28377S_Plat/Source/03_System/01_OS/CpuTask.cpp:172) | 系统时基、看门狗和运行灯归系统任务 |
| ISR 与程序入口分离 | [Redef_Interrupt.c:212](/D:/Code/CCSv12/F28377S_Plat/Source/03_System/02_ISR/Redef_Interrupt.c:212) | 独立 `03_System/02_ISR`，清楚列出 TAD/FAULT/Timer2/DPRAM |
| 控制按命令、给定、采样、输出组织 | [CoreController.hpp:198](/D:/Code/CCSv12/F28377S_Plat/Source/04_App/04_Control/CoreController.hpp:198) | 命令授权、测量、控制核心、输出和保护分别维护 |

参考工程的 PIE、CLA、ePWM/SCI 中断和 CPU Timer 规则属于 F28377S，不能搬到 C6748。参考中的 PcDebug 1 ms、故障任务 50 ms等调度周期也不能直接替换 SST 的当前触发规则。

## 3. 当前结构问题与处理优先级

优先级 A 是首先实施的结构问题；B 是随后收敛的后台接口；C 是在等价重构稳定后整理的历史分支和通用算法接口。

| 优先级 | 当前位置及问题 | 调整结果 |
|---|---|---|
| A | [main_485test.c:110](/D:/Code/SST/AnyEyesVersion/Source/03_System/01_OS/main_485test.c:110)：4144 行文件混放映射对象、运行量、初始化、保护及 ISR；`main()` 本身已经较短 | 分离入口、中断、映射、各域数据与初始化，旧主文件退出编译 |
| A | [main.H:12](/D:/Code/SST/AnyEyesVersion/Source/04_App/00_Config/main.H:12)：2284 行，缺少包含保护，混放类型、宏、布局、extern、外设与 ISR 接口 | 所属模块头文件替代万能头；最终活动代码不再包含它 |
| A | [ChbApplication.c:39](/D:/Code/SST/AnyEyesVersion/Source/03_System/01_OS/ChbApplication.c:39)：系统组织层直接配置寄存器、处理 DPRAM 业务及遥测块轮询 | 分到板级、启动协调、任务调度、DPRAM 应用适配 |
| A | [Stateflow.C:352](/D:/Code/SST/AnyEyesVersion/Source/04_App/04_Control/Stateflow.C:352)：活动状态联锁与历史启动流程混在一起 | 活动状态代码保留为 `SstPortStatus`；历史启动流程单独归档 |
| B | [SstParams.c:157](/D:/Code/SST/AnyEyesVersion/Source/04_App/01_FuncCode/SstParams.c:157)：参数访问、启停权限、遥测表、EEPROM 事务混合 | 参数、遥测、命令和参数存储分别维护 |
| B | [SstDebug.c:103](/D:/Code/SST/AnyEyesVersion/Source/04_App/03_PcDebug/SstDebug.c:103)：UART 寄存器、RS485、队列、协议分流及存储调度混合 | 串口传输归外围；协议归 PcDebug；存储由 TaskSlice 调度 |
| B | [SstCommand.c:25](/D:/Code/SST/AnyEyesVersion/Source/04_App/04_Control/SstCommand.c:25)：命令依赖参数模块查询停机/存储状态，参数模块也执行命令权限 | 单一命令策略入口；存储状态由独立接口提供 |
| B | [FaultCheck.C:3097](/D:/Code/SST/AnyEyesVersion/Source/04_App/08_Fault/FaultCheck.C:3097)：故障检测文件包含正常 PWM 启停执行逻辑 | 分出控制输出/发波模块，故障负责决定关断动作 |
| C | [ADC_Sample.C:612](/D:/Code/SST/AnyEyesVersion/Source/04_App/02_Measurement/ADC_Sample.C:612)：测量整理还更新外部电源与欠压判据 | 先标明快慢边界，后分离测量结果和运行判据 |
| C | [CommonProgram.C:94](/D:/Code/SST/AnyEyesVersion/Source/01_ComLib/CommonProgram.C:94)：变换与 PI 用全局量传递输入输出 | 后续改为显式参数与状态；第一轮保留算法接口 |
| C | `PluginUnit.C`、`DPRAM.C`、`FaultCheck.C`、`HVAC_Strategy.C` 含多端口历史分支 | 基于活动调用图划分模块；逐项归档，不按文件长度删除 |

### 活动代码与历史代码必须区分

- `Uart2_send()` 在块注释中；`UARTReceiveDispose()` 在 `#if 0` 中。它们不是当前正在处理收发的主链路。
- `UART2Isr()` 仍编译，但当前启动调用链不注册它。`UARTInterruptInit()` 如果被调用，确实会注册并使能 UART 中断，不能只依据注释认定其为空操作。
- 当前主运行状态机是 `ChbDabControl_Service()`。未发现旧 `NormalRun_flow/Start_flow/Start_flow_Manual/Stop_flow/SwitchOperation_Dispose` 的非注释活动调用，不能说当前运行着两套启动状态机。
- `Port_StateUpdate_Read/Write` 和 `Port_AllStopFlag_RST` 仍有活动调用。这部分不能随旧 Stateflow 一起移出编译。
- `CHB_OPEN_LOOP_PULSE_TEST` 在配置头中被 `#error` 禁止。保留的开环逆变是 `ac_mode=0`，两者不是同一功能。

## 4. 目标文件布局与职责

以下是拟实施的目标。名称可在实施时随现有命名习惯小幅调整，但责任、数据归属和执行上下文必须落实。相同文件名统一大小写，头文件统一 `.h`/`.hpp`，不能继续依赖 Windows 大小写不敏感行为。

```text
Source/
├─00_Device/                         TI 芯片库，保留
├─01_Drive/                          TI 外设库，保留
├─01_ComLib/
│  ├─SstTypes.h                      基础类型；不包含任何硬件/业务状态
│  └─CommonProgram.C/.H              首轮保持算法接口；后续按变换/PI/滤波整理
├─02_Peripheral/
│  ├─00_FPGA/
│  │  ├─FpgaRegister.c/.h            FPGA 固定映射对象与寄存器布局
│  │  ├─FpgaIo.c/.h                  原 PluginUnit 的 GPIO/状态/模块硬件访问
│  │  ├─FpgaModulation.c/.h          CHB/DAB 指令下发，保留旧 API
│  │  ├─Ft3Recorder.c/.h             FT3 映射、读写和录波数据接口
│  │  └─DacOutput.c/.h               DAC 硬件输出
│  ├─01_DPRAM/
│  │  ├─DpramRegister.c/.h           19 个映射对象与既有协议布局
│  │  └─DpramTransport.c/.h          块读写、字复制和 mailbox 访问
│  ├─02_Storage/
│  │  ├─EEProm.C/.H                 保留 EEPROM/IIC 底层访问
│  │  └─X1226_RW.C/.H               保留并逐步隔离旧存储适配
│  └─03_Serial/
│     └─SstSerial.c/.h              UART2/RS485、8 槽整帧队列、有界轮询
├─03_System/
│  ├─00_Board/
│  │  ├─SstBoard.c/.h               PSC、引脚、EMIFA、GPIO、Timer2 硬件配置
│  │  ├─SstBoardConfig.h            板级引脚/时钟/中断源常量
│  │  └─Hardware/EMIFAPinmuxSetup   现有 pinmux 实现保留
│  ├─01_OS/
│  │  ├─Main.c                      启动、上电等待、进入主循环
│  │  ├─SystemInit.c/.h             协调初始化顺序，不拥有各域状态
│  │  ├─TaskSlice.c/.h              后台任务列表及明确的调度顺序
│  │  └─CpuTask.c/.h                系统时基、看门狗和指示灯服务
│  └─02_ISR/
│     └─Redef_Interrupt.c/.h        中断注册及完整 ISR 正文
└─04_App/
   ├─00_Config/
   │  ├─ChbBuildConfig.h            协议、DPRAM、仿真器等选择宏
   │  ├─SstControlConfig.h          原控制常量/默认值，保持数值表达式
   │  └─main.H                     仅过渡兼容，最终活动代码不再引用
   ├─01_FuncCode/
   │  ├─SstParams.c/.h              固定参数表、量纲、范围、读写规则
   │  ├─SstParameterStore.c/.h      参数持久化事务与结果
   │  └─FuncCode.hpp                原 PcDebug 功能码接口适配
   ├─02_Measurement/
   │  ├─ADC_Sample.C/.H             快速采样接口保留
   │  ├─MeasurementData.c/.h        采样/滤波/RMS 累积量及初始化
   │  └─MeasurementService.c/.h     后台测量整理
   ├─03_PcDebug/
   │  ├─Comm2PcDebug.cpp/.hpp       保留参考协议、命令处理和地址访问
   │  ├─SstDebug.c/.h              协议选择、分流、服务入口
   │  ├─PcDebugDispData.c/.h        参数/波形/状态的 SST 映射与显示表
   │  ├─vofa.c/.h                  保留 JustFloat 编码
   │  └─MaintenanceProtocol.C/.H   可选兼容协议
   ├─04_Control/
   │  ├─ChbDabControl.c/.h         现有慢状态机与快速算法核心
   │  ├─SstCommand.c/.h            命令来源、权限、互锁和停止优先
   │  ├─ControlData.c/.h           旧控制/PLL/PI 兼容量的实际归属
   │  ├─ControlInit.c/.h           控制默认值与运行态复位
   │  ├─SstPortStatus.c/.h         活动端口状态、停机联锁和兼容镜像
   │  └─05_ControlOutput/
   │     └─PwmGate.c/.h            PWM 开锁/封锁策略与输出动作
   ├─08_Fault/
   │  ├─FaultCheck.C/.H            快速故障检查与动作决策
   │  ├─ModuleVoltageCheck.c/.h    Udc_check 及其巡检状态
   │  ├─ProtectionData.c/.h        原保护计数、锁存、阈值和初始化
   │  └─SstFault.c/.h              五位故障码、等级、当前/历史和后台汇总
   └─10_Host/
      └─SstDpramHost.c/.h          遥控/遥调语义与遥测轮询适配
```

旧启动流程、停用 UART 实现和非 HVAC 分支的归档目录应在 `Source` 外，例如 `archive/legacy/`。先保留清单及迁移理由，再从编译中排除。`tools/build.py` 会递归编译 `Source` 下的源文件，不能只把未用 `.c` 移进 `Source/Legacy` 就认为它不会参与构建。

这些模块不是全部同时新建。先搬入口/ISR/映射/状态定义，接着拆后台混合职责，最后整理外围和历史文件。单一职责的现有模块继续沿用。

## 5. main_485test.c 的具体处理

| 原内容 | 当前位置 | 首轮归属 | 约束 |
|---|---|---|---|
| `main()` | 1908 | `01_OS/Main.c` | 原启动流程和无限循环保持；不定义业务全局量 |
| `Timer2Isr/FPGAIsr/FAULTIsr/DPRAMIsr` | 3441/3783/4097/4131 | `02_ISR/Redef_Interrupt.c` | 完整搬运，保留名、正文、预处理条件及调用次序 |
| FPGA_Interface、FT3 对象及 pragma | 61/103/110/129 | `FpgaRegister/Ft3Recorder` | 对象与 pragma 一起迁移，符号名和布局不变 |
| DPRAM 19 个映射对象及 pragma | 82/146/165/172/178 | `DpramRegister` | 保留 `volatile`、长度、地址和 `NOINIT` |
| `Initial_Variable()` | 897 | 首轮 `ControlInit` 中保留完整函数；随后分域 | 建立逐语句归属表后按原序调用各域初始化 |
| `Variable_RST()` | 1659 | `ControlInit` | 不视为仅上电初始化：PWM 开锁路径也会调用 |
| `Udc_check()` | 1705 | `ModuleVoltageCheck` | 闭环条件、巡检与保护位置保持 |
| 时基计数与标志 | 约 250 起 | `CpuTask` 或实际业务域 | 保护计数归保护；系统计时归系统，不能全塞 CpuTask |
| 采样/滤波/功率量 | 多组 | `MeasurementData` | 保留单一写入者、累积窗口和必要共享声明 |
| 控制/PLL/PI/状态量 | 多组 | `ControlData/SstPortStatus` | 必要旧镜像保留；不得同时保留两个定义 |
| 保护/故障量 | 多组 | `ProtectionData/SstFault` | 检测、锁存、输出动作、上报码的职责分开 |
| UART 历史缓冲、CRC、按键、未注册 ISR | 1945/2008/3320/3363/3648 | 逐项引用审查后兼容/归档 | 不恢复旧 UART 路径，不把 `#if 0` 恢复成活动代码 |

不新建 `AllVariables.c`、`GlobalData.c` 或巨型 `LegacyData.c` 来容纳原主文件全部变量。过渡期仍需公开的符号，也必须在所属模块定义，只公开必要声明。

目标入口示意（接口名称是拟议名称，尚未实施）：

```c
int main(void)
{
    SystemInit_Hardware();
    SystemInit_WaitPowerOnDelay();
    SystemInit_Runtime();
    TaskSlice_Run();
    return 0;
}
```

文件长度不是验收指标。即使 ISR 因保留完整快速链而较长，只要文件专门维护中断、调用清楚且无协议/存储业务，就比为了缩短函数增加间接派发更合适。

## 6. 数据所有权与头文件规范

### 6.1 数据由谁维护

| 数据域 | 唯一所有者 | 对外接口与兼容方式 |
|---|---|---|
| FPGA/FT3 MMIO 对象 | FPGA/FT3 外围 | 保留固定符号与窄布局头，其他模块不得重复定义 |
| DPRAM MMIO 对象 | DpramRegister | 保留 19 个 section；协议缓存与硬件窗口区别明确 |
| 系统时基、上电等待、看门狗 | CpuTask | 时间读取/心跳服务；首轮保留旧标志类型 |
| 采样、滤波、RMS 累积 | MeasurementData | 测量接口；快慢双方允许访问的数据明确标注 |
| 命令、控制参数与核心状态 | ChbDabControl/SstCommand | 保留 `g_chb_dab_command/params/status` 仿真器接口 |
| 旧端口/运行标志和状态镜像 | SstPortStatus/ControlData | 仅保留活动兼容接口，注明镜像更新者 |
| 保护阈值、计数、锁存 | ProtectionData | 快速检查/复位接口；后台只获取报告输入 |
| 故障码、等级与历史 | SstFault | 只负责现有上报机制，不取代快速关断 |
| 参数地址和转换规则 | SstParams | 受控读写，协议逻辑地址保持 |
| EEPROM 图像与事务状态 | SstParameterStore | 请求/服务/忙标志/结果；不依赖当前所选协议 |
| UART 队列和 RS485 状态 | SstSerial | 整帧入队、有界接收/发送、剩余容量 |
| 协议解析状态 | 各协议模块 | 不暴露 UART 寄存器，不直接控制 PWM |
| 显示表/波形映射 | PcDebugDispData | 固定索引、比例和读接口，保留原可观察语义 |

首轮不把所有数据改为结构体，也不立即批量增加/删除 `volatile`、`static`、`const`。这些会改变符号可见性、链接或编译行为，必须按真实写入者和调用者单独处理。算法私有状态能留在当前 `ChbDabControl.c` 中，不为了对齐目录把一个计算过程的历史量散落到多个文件。

仿真器长期使用的符号保持可见。接口收敛优先针对模块内部量，不能破坏用户现有 Watch/Expressions 调试操作。硬件 MMIO 地址和 PcDebug 逻辑地址保持；普通 RAM 变量的物理链接地址在文件拆分后可能改变，不能把原 RAM 地址作为稳定接口。

### 6.2 main.H 如何拆

1. 基础类型归 `SstTypes.h`。第一轮保留等价的类型宽度和符号，是否把宏别名改成 typedef 另列审查。
2. 板级常量归 `SstBoardConfig.h`；算法常量归 `SstControlConfig.h`；构建选择保留在 `ChbBuildConfig.h`。
3. FPGA/FT3/DPRAM 位域、联合体与数组布局归各外围布局头。保留成员顺序、宽度、打包方式和大小核对。
4. 测量、控制、状态、保护声明放回所属模块头。需要跨文件共享时使用窄的数据接口头，不继续包括所有模块。
5. ISR 声明归 `Redef_Interrupt.h`；板级配置接口归 `SstBoard.h`；不再从应用配置头获得板级函数。
6. 过渡聚合 `main.H` 加包含保护，只包含新所属头；新写/已整理模块不得引用它。最终活动代码中引用计数应为零。

公开头独立可编译；包含保护、必要 `extern "C"`、类型/函数声明完整。硬件布局头不包含应用状态头，通用算法头不包含芯片寄存器头。不用头文件互相包含来掩盖循环依赖。

DPRAM 插件布局目前复用 FPGA 的 ID、PM 控制和开关位域类型。拆头时让 DPRAM 布局单向包含窄的公共硬件字段头，FPGA 不反向包含 DPRAM。`PortRunFLAG/ControlFLAG/FaultTypeFLAG` 等应用业务位图归应用状态/保护头，不能为了复用把它们一起下沉硬件层。

## 7. 启动与调用关系

### 7.1 启动顺序保持

```text
Main
  ├─ 原 Flag_testT/rtt 置零（交回所属模块，仍先于硬件初始化）
  ├─ SystemInit_Hardware
  │    ├─ PSC 与系统配置解锁
  │    ├─ EMIFA → GPIO pinmux → GPIO 方向/初值
  │    ├─ DSP 中断控制器 → Timer2 配置 → Timer2 中断绑定
  │    └─ 上电等待灯/看门狗初值
  ├─ SystemInit_WaitPowerOnDelay
  │    └─ 原 20 s 等待；此阶段不消费远程启停
  ├─ SystemInit_Runtime
  │    ├─ Initial_Variable → Variable_RST
  │    ├─ 可选旧维护参数加载 → FaultLatch_Clear(0, 1)
  │    ├─ 端口初始状态 → ChbDabControl_Init
  │    ├─ SstFault_Init → 调试通信初始化
  │    ├─ 可选 SST 参数加载
  │    ├─ TAD/FAULT 和可选 DPRAM 的 GPIO 中断绑定/使能
  │    └─ 可选 DPRAM mailbox 读取及通知引脚初值
  └─ TaskSlice_Run
```

板级层负责寄存器配置；系统层负责调用顺序；每个业务域负责自己的初始化值。`Initial_Variable` 的分域不能把原交错发生的寄存器写入随意前移/后移，不能让参数恢复先于核心默认初始化，也不能提前开放 TAD/FAULT。

当前 `SstBoard_GetTick1ms()` 读取 `g_chb_dab_status.tick_10ms`，使板级层反向依赖控制状态。目标是由系统时间模块组合 Timer2 硬件计数与软件时基，板级只提供计数读取。首轮先迁移现有实现与接口，再处理时间归属；不引入新的 Timer2 行为。

### 7.2 后台顺序必须可读

当前一轮正常后台的展开次序如下；`TaskSlice` 应明确列出，不藏在某个协议服务的尾部：

```text
看门狗/LED
  → ChbDabControl_Service
  → SstFault_Service
  → [DPRAM 遥控/遥调服务 → DPRAM 遥测轮询]
  → 读取本轮通信 now 时基
  → 串口 TX 服务（第一次）
  → 有界 RX 与协议分流
  → [维护协议处理]
  → AnyEyes 服务 / VOFA 周期发送（二选一）
  → 参数保存服务
  → 串口 TX 服务（第二次）
  → Port_RealTimeParameter
```

整理后存储由调度器调用，但首次外提仍在第二次 TX 之前，保持本轮返回码、队列和副作用顺序。不能在“拆后台”时顺便增加一个调度周期，或把测量整理提前改变报文观察到的数据。

当前 `SstDebug_Service` 在第一次 TX 之前采集本轮 `now`，VOFA 周期判断使用该值。拆分时保留这个采样位置，不让各子任务分别重新取时基而改变节拍边界行为。

当前 RX 预算为每轮最多 64 字节。TX 单次服务预算 16 字节，一轮服务调用两次，因此最多推进 32 字节，仍受 FIFO 与发送完成条件限制。保留 8 个整帧槽、请求应答背压、预留容量、UART FIFO 写入方式和 RS485 TEMT 完成判断。重构说明不能误写为“每轮总 TX 16 字节”。

DPRAM 原插件轮询顺序、每次一个插件块和跳过规则保留，宏关闭时不访问对应地址、不注册通知中断。

### 7.3 通信、命令与存储的依赖

```text
UART2/RS485 ←→ SstSerial ←→ SstDebug
                               ├─ Comm2PcDebug → SstParams / PcDebugDispData / SstCommand
                               ├─ VOFA → PcDebugDispData
                               └─ MaintenanceProtocol → 参数/命令/旧存储适配

DpramTransport ←→ SstDpramHost → SstCommand / 测量与兼容状态

SstParams → SstCommand（操作权限）/ 参数存储请求接口
SstCommand → 控制状态接口 / 存储忙接口
TaskSlice → SstParameterStore_Service → EEPROM 底层
```

Peripheral 的 DPRAM 模块只做块传输，遥控/遥调的业务语义放到 `04_App/10_Host`。不把应用命令授权下沉进 DPRAM 驱动。EEPROM 驱动只处理字节/页访问，SST 参数图像和事务放在应用存储模块。

`SstCommand` 是运行命令策略的唯一入口，保持仿真器/串口/DPRAM 来源规则、STOP 高于 START、OFF 高于 ON、停机复位和存储互锁。参数普通写入也使用同一操作许可规则，避免两个模块分别实现后发生分歧。停止/急停不被存储忙拒绝；普通启动/修改继续拒绝。仿真器直接改 RAM 的现有使用方式保留。

## 8. PcDebug、参数、故障与控制如何进一步拆

### 8.1 PcDebug 和遥测

- 保留 `Comm2PcDebug` 的解析、参数访问、波形、状态、命令/应答和 CRC 处理；本轮方案不提出改写参考通信协议。
- 硬件收发与队列从 `SstDebug` 移到 `SstSerial`；通信适配只依赖窄收发接口。
- `SstDisplay_Refresh/Read` 和 `SstTelemetry_ReadFloat` 归 `PcDebugDispData`，接口名首轮保留，更新 `.h` 所属。
- 固定功能码 30×34 布局、200 字显示索引、16 通道波形选择、比例/符号/范围、F0～FF及对应应答码、CRC8、故障显示和历史接口保持。
- AnyEyes/VOFA 仍由 `SST_DEBUG_PROTOCOL` 二选一；DPRAM、维护协议独立宏门控。VOFA 自身不增加虚构的控制报文。
- 当前上报是后台读取的实时标量组合，不能在重构时声称已升级为整组原子快照。若以后需要一致性快照，必须单独设计并评估 TAD 额外复制成本。

### 8.2 参数与存储

`SstParams` 保留参数索引、转换、范围和停机写规则。`SstParameterStore` 拥有 EEPROM 图像、CRC、pending/result/position/tick 和写入/读回验证。保持现有页地址、长度、格式、每个 10 ms 最多一个事务步骤、先失效标记后提交标记，以及保存中检测到运行后的处理。

为避免参数 → 命令 → 存储 → 参数的循环调用，参数模块维护唯一映射表，并提供窄的内部“42 项快照导出/整体范围校验/提交”接口；存储模块只接收快照、处理现有 92 字节镜像并返回镜像读取结果，不反向调用公开参数写接口。加载由现有兼容入口或启动协调层依次完成读取、格式/CRC 校验、整组参数校验和提交，保持原结果码与顺序。保存中停机判据使用控制层的只读状态查询，不能回调可能再次查询存储忙的命令操作入口。

不能用 `SstParams_Write()` 逐项恢复 EEPROM：普通写权限、串口来源和存储忙条件会改变现有加载语义。必须保留当前先验证整个镜像、再内部提交的行为，采样周期只核验不恢复覆盖，不加载运行使能/DAB使能/复位命令。

旧维护保存与 SST 参数保存仍需共享忙语义。初次分离只使用查询/请求接口保持现有互锁；引入新通用事务锁或改变协议返回时机是后续独立行为变更。

### 8.3 故障与发波

快速检测/锁存/关断与后台故障码汇总必须分开。保留当前五位故障码、0～4 等级、当前/历史容量、旧组19历史访问及复位行为。重构不重新编号、不改变故障位复用规则。

`PWMPluseEnable_Initial/Enable/Disable` 包含故障处置状态和控制状态更新，不能机械搬到外围后继续让底层依赖所有应用全局量。建议将这些原函数先原样归到应用 `05_ControlOutput/PwmGate`，保留 API；外围 `FpgaIo/FpgaModulation` 只提供实际寄存器动作。随后再细分“输出决策”与“硬件写入”。

`FaultDispose` 的动作及写寄存器序列不在第一轮合并。`Variable_RST` 在正常开锁中也有调用，不能把它变成只允许启动时执行的初始化函数。

### 8.4 控制、测量与旧状态

当前 `ChbDabControl` 已有私有历史状态、慢 `Service` 和快 `FastStep`，应保留这个边界。先收敛旧数据和硬件输出依赖；PLL、电压环、电流环、开环和 DAB 不因文件 1642 行就立即拆成独立对象。

测量阶段先明确哪些函数在 TAD、哪些在后台，保留采样窗口、累积顺序、符号和状态判据更新时机。之后若要整理 `Port_RealTimeParameter` 的运行判据，将其作为单独步骤，核对是否影响下轮控制和报文。

`Port_StateUpdate_Write()` 当前被注释成“回写 DPRAM”，实际经 `PortController_Communication()` 写 FPGA 主控的 `HVAC_STATUS`，并有故障位、LED和通知引脚副作用。它必须保留在原快速链位置，不能因为名字含通信就移动到后台。

通用 Clarke/Park/PI 的显式输入输出改造放在最后。现有历史量和浮点运算顺序保持；接口纯化不与算法调整同时实施。

## 9. 10 kHz FPGA 外部中断的重构边界

### 第一阶段保持的真实顺序

```text
FPGAIsr
  → 禁用 GPIO bank4 / 清 DSP event / 判断并清 GPIO69 状态
  → GPIO43、45 测时拉高
  → Port_StateUpdate_Read
  → Port_AD_Sample(0)
  → HVAC_RealTimeParameter
  → SVG_Udc_Average
  → ChbDabControl_FastStep
  → 按现有模式执行 Udc_check / 模块及 OC/OV 保护
  → 停机兼容动作 / Port_StateUpdate_Write / 故障状态处理
  → 原 FT3 录波及相关条件分支
  → DAC_Output
  → GPIO43、45 拉低
  → 重新使能 bank4
```

第一轮将完整 `FPGAIsr` 搬到专用 ISR 文件。不增加包装调用、函数指针派发、虚函数、日志、CRC、串口轮询、报文打包或 EEPROM 操作。`ChbDabControl_FastStep` 的调用者仍只有 `FPGAIsr`。

GPIO69 上升沿、bank4、INT5、`SYS_INT_GPIO_B4INT` 保持；硬件故障 GPIO142/bank8/INT4 保持。DPRAM 通知只在宏启用时绑定。`ControlTAD/TAD_PLL`、FPGA 周期写入和快速闭环保护条件保持。

Timer2 中断已有 `ChbDabControl_Tick10ms`、`FaultLatch_Clear`、等待状态和欠压/OC/OV 计数处理，第一轮原样迁移。把它进一步缩成只置标志会改变执行时间与语义，需要后续单独评估。

TI 当前 ISR 包装不会主动重开全局中断，Timer2/FAULT/DPRAM 执行期间可能延迟 TAD。源码等价和编译通过能证明结构与软件约束，不能单独证明每次外部边沿都满足 100 μs。文件搬运还可能影响链接布局/cache，所以最终需要对比板上重构前后 GPIO43/45 脉宽、边沿到服务延迟、漏触发和抖动。已有快速测时 GPIO 保留，优先使用它们。

## 10. 注释及文件内部组织

各自定义文件按参考风格组织：文件职责与执行上下文 → 公共/私有包含 → 私有宏 → 私有声明 → 本模块数据 → 公共接口 → 私有实现 → 文件结束。公开头列出接口契约，不宣告其他模块所有变量。

注释必须说明实际语义：

- 函数：功能、输入、输出、副作用、返回值、调用上下文、允许调用者及互锁条件。
- 变量：含义、单位、初值、所有者、写入者/读取者、ISR/后台共享情况、是否映射硬件。
- 配置：单位、范围、默认值、生效时机；位图写出对应组/模块，而非泛称“配置变量”。
- 中断：触发源、应答次序、快速任务、禁止添加的后台处理。
- 协议映射：保留功能码/显示索引/比例与对应文档。

已有注释随函数/变量迁移，并修正与实际代码不符的描述，例如 Timer2“仅时基”、状态回写“DPRAM”、UART 初始化“只保留入口”。不能继续使用所有函数都适用的泛化输出说明代替真实副作用。

## 11. 分阶段实施与每阶段出口

| 阶段 | 工作 | 阶段结束必须满足 |
|---|---|---|
| 0：锁定当前状态 | 保存当前源码/元数据/宏与硬件位图；生成函数、变量、调用、MMIO、初始化清单 | 当前用户修改被记录；本次审查清单作为起点，不用旧注释基线覆盖源码 |
| 1：机械迁移 | Main、完整 ISR、映射对象、各域数据定义、完整初始化函数迁到所属模块；同步构建路径 | 无重复定义；函数正文/数据类型/初值/分支保持；TAD/FAULT/Timer2 调用顺序等价 |
| 2：头文件与数据归属 | 拆 main.H，逐模块替换包含；分域初始化但保持启动轨迹 | 每个变量有唯一所有者；不新增全局汇总文件；活动模块不依赖万能头 |
| 3：后台职责分离 | 串口传输、协议、显示映射、参数保存、DPRAM Host和命令策略整理 | 完整后台顺序保持；协议与映射回归通过；存储不依赖所选调试协议 |
| 4：外围与历史整理 | PluginUnit 按硬件职责整理；活动端口状态分离；旧 UART/旧流程/多端口分支归档 | 活动引用完整；归档源码不参与构建；保护/输出边界清楚 |
| 5：交付验收 | Clean Build、全部配置、回归、map、注释、文档、manifest；板级前后对照 | 软件验证与板级记录分别明确，达成下面验收标准 |

每阶段保存可回退的完整代码快照和迁移清单。中途不批量格式化算法，不改变优化级别，不修改默认配置。若某项等价检查失败，先说明是源结构变化还是行为变化，不能靠覆盖基线或删断言放行。

当前模块掩码应明确保留：A1=`0x0001U`、A2=`0x0000U`，B/C 各为 `0x007FU/0x003FU`；即 A 相1个、B/C相各13个。保留当前磁盘配置，不从源码中的旧值注释或历史备份恢复另一组位图。

## 12. 验证体系需要同步调整

现有验证包含大量按旧文件定位的规则，结构重构前需先升级定位方式，保留断言内容。

| 当前工具 | 调整要求 |
|---|---|
| `tests/realtime_audit.py` | DPRAM pragma/配置宏改为全源或所属模块定位；后台根节点纳入 TaskSlice/CpuTask；保留完整 TAD token 等价、单一 FastStep 调用者、宏关闭不可达地址、中断映射及后台禁止全局关中断/CSR直接写/Cache操作等断言 |
| 启动顺序的源码位置检查 | 增加假硬件启动调用轨迹，核对完整初始化及中断开放门槛；不只在一个函数内查字符串位置 |
| `tests/control_regression.py` | 保留原核心场景；补真实 ISR、Timer2 与新模块接口调用轨迹，核对采样/保护/写出/应答顺序 |
| 协议/参数/维护/DPRAM/故障回归 | 新源文件和头纳入测试构建；尽量分别编译真实模块，避免仅拼接源码掩盖接口问题 |
| `tests/comment_coverage.py` | 将注释覆盖与逐文件历史 token 相等分开；新增旧函数/对象到新文件的搬运等价报告 |
| `build/comment_baseline` | 保留不变，用于历史证据；禁止重置为新实现后宣称等价 |
| `tools/annotate_source.py` | 不直接重跑旧基线生成器覆盖重构代码；新/迁移接口的中文注释人工核对 |
| `tests/dpram_map_audit.py` | 既有 19 对象/NOINIT/启动镜像审计继续；增加 FPGA/FT3 对象的地址、尺寸、定义唯一性 |
| `tools/build.py/.cproject` | CLI 自动发现源码，但 CCS Debug/Release include 路径须同步；Clean Build 避免旧 obj 混入 |
| `tools/ccs_validate.py` 与导入验证 | 工程名仍为 `AnyEyesVersion`，输出仍为 `SST_AnyEyes.out`，保留当前工作区直接导入兼容 |
| `tools/delivery_manifest.py` | 支持新的注释/函数搬运报告，重新计算源码、元数据与全部产物校验，不使用过期报告 |

活动代码识别需基于实际宏预处理、直接/间接调用和外部注册入口；不能只用 `rg` 结果或按函数名合并的正则调用图宣称“未调用”。旧 API 移出构建前核对链接 map 和所有宏组合。

计划执行的软件验证包括：CCS Debug/Release Clean Build、CLI 两配置、既有11宏组合、控制/参数精度/故障/通信/维护/DPRAM 回归、启动及ISR轨迹、所有交付 map、中文注释覆盖、交付校验清单。这里只列未来验收工作，本轮未重新运行这些测试。

对 TAD/FAULT/Timer2 的 TI 反汇编、代码尺寸、栈使用及关键调用序列做重构前后对照。函数/对象搬运后的二进制不要求整体字节相同，但任何新增调用、栈增长或快速路径指令变化须单独解释；这个对照仍不替代实板缓存/EMIFA等待造成的时序测量。

板级验证覆盖：仿真器模式、开环逆变、闭环兼容策略/Id斜坡策略、DAB独立启停、硬件故障、DPRAM启用/关闭、AnyEyes/VOFA、通信满负荷、参数存储与 Timer2/FAULT/DPRAM 并发。按已有 [BOARD_ACCEPTANCE.md](/D:/Code/SST/AnyEyesVersion/docs/BOARD_ACCEPTANCE.md) 比较重构前后 TAD 最大执行时间、响应延迟和输出。

## 13. 最终验收标准与交付物

### 结构验收

- `Main.c` 只包含入口组织，不再定义映射对象、算法数据、保护函数或 ISR。
- `TaskSlice` 能直接读出完整后台顺序、宏门控及触发规则；存储调度不藏在调试通信中。
- ISR 独立，中断来源和原快速链清楚；TAD 无新增后台工作。
- 每个全局/私有数据有明确模块归属和初始化接口；映射对象仅定义一次。
- 活动模块不引用旧 `main.H`；基础/布局/业务头独立可用，无新的万能头。
- 串口硬件层不包含参数/故障业务；DPRAM传输层不处理启停语义；EEPROM底层不定义SST参数图像。
- 旧状态流程与仍活动的联锁/状态回写已明确分开；不存在可读性更差的第二个综合文件。
- 所有自定义函数、变量按实际语义写清中文注释，函数与数据的执行上下文一致。

### 行为验收

- C6748+FPGA、10kHz触发与保护链、初始化安全门槛、20s等待保持。
- 仿真器/开环/闭环/DAB与当前硬件位图保持；故障复位不自动重新运行。
- PcDebug现有参数、波形、状态与故障逻辑地址、协议及映射规则保持。
- VOFA/AnyEyes二选一、DPRAM独立宏、维护兼容与存储互锁保持。
- FPGA/DPRAM/FT3硬件地址和尺寸、DPRAM `NOINIT`及关闭时不可达保持。
- 实际CCS和CLI构建、配置矩阵、回归、map/启动镜像审计全部通过。
- 板级时序结果单独记录；没有实测就不宣称“已证明100µs截止期完全不受影响”。

### 交付文件

1. 整理后的可直接在当前工作区导入、编译的工程及全部源码。
2. 原函数/变量/类型/宏 → 新模块的迁移清单与等价结果。
3. 最终模块职责、依赖图、启动/后台/TAD/FAULT调用关系文档。
4. 参数/波形/状态/故障映射文档，地址保持且链接更新。
5. 宏配置、仿真器Watch接口、板级位图与操作说明。
6. CCS/CLI/配置矩阵/回归/map/注释报告和新交付manifest。
7. 历史代码归档清单、每阶段回退点和板级对比验收记录。

建议按阶段0～3优先消除入口、万能头和后台责任混合，再完成外围/历史整理。不要在第一阶段同时改变算法参数、调度周期、中断嵌套或整组遥测快照策略。
