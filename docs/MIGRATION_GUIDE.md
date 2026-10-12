# 迁移与架构说明

## 来源与目标

功能来源：`D:/Code/SST/Chb_DabPrim_vofa`。框架与 PcDebug 来源：`D:/Code/CCSv12/F28377S_Plat`。新工程：`D:/Code/SST/AnyEyesVersion`。原两个工程未修改。

项目保留 TMS320C6748、C6000 8.3.12、FPGA 的采样/PWM/保护接口、EMIFA 16 位总线、GPIO69 的 TAD 中断和原寄存器链接映射。参考框架的 C2000 启动、CLA、ePWM、PIE、库和链接文件不属于这个工程。

## 模块分层及迁移对应

| 新目录 | 源文件或内容 | 职责 |
|---|---|---|
| `00_Device` | include、system_config | 器件定义、DSP 中断分发与汇编向量 |
| `01_Drive` | drivers | TI 原平台驱动 |
| `01_ComLib` | CommonProgram | 源工程公共算法与工具 |
| `02_Peripheral/00_FPGA` | PluginUnit | 插件、PWM/参考值、DAC、FPGA 数据交换 |
| `02_Peripheral/01_DPRAM` | DPRAM | 既有地址布局的遥控遥测 |
| `02_Peripheral/02_Storage` | EEProm、X1226_RW | 原 EEPROM/RTC 硬件接口 |
| `03_System/00_Board` | ChbPlatform、Hardware、EMIFAPinmuxSetup | 平台初始化、引脚与中断映射 |
| `03_System/01_OS` | main_485test、ChbApplication | 入口、原实时链路、后台任务组织 |
| `04_App/00_Config` | main.H、ChbBuildConfig | 兼容共享数据声明和独立构建选项 |
| `04_App/01_FuncCode` | 新参数适配 | 保留功能码组/组内地址，接入 SST 参数 |
| `04_App/02_Measurement` | ADC_Sample | 标定、采样与数据处理 |
| `04_App/03_PcDebug` | PcDebug 适配、VOFA、MaintenanceProtocol | AnyEyes/VOFA、后台传输、兼容协议 |
| `04_App/04_Control` | ChbDabControl、HVAC_Strategy、Stateflow | 原 CHB/DAB 算法与状态机 |
| `04_App/08_Fault` | FaultCheck、SstFault | 原快速保护、后台故障编码/历史/观测 |

首版保持控制核心、共享 FPGA/插件结构和原控制数值语义，重新组织目录并增加明确模块接口。`main.H` 与原主文件中仍有兼容全局变量，以维持源算法、录波及硬件位域布局；它们并非全量重写后的纯对象模型。

## 初始化和执行顺序

`main` → PSC/引脚/EMIFA/Timer2 初始化 → 20 秒上电等待 → 原变量初始化与复位 → 可选旧维护参数加载 → CHB/DAB 初始化 → 故障、通信初始化 → 可选SST参数加载 → GPIO TAD/FAULT 中断开放 → 无限后台循环。

运行对象与安全输出在 TAD/FAULT 注册前初始化。Timer2 在上电等待期间保持原 10 ms 节拍。通信不在等待期间执行启停。旧维护参数加载仅在两个相关宏同时开启时执行，它不等同于SST控制参数；新的SST参数图像在核心初始化后加载，避免被默认值初始化覆盖。

后台循环执行：看门狗/LED、CHB/DAB 慢状态机、故障汇总、可选 DPRAM、调试通信和原实时参数整理。通信接收、CRC、参数访问、波形编码、故障上报和持久化均在后台。

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

`FPGAIsr` 保持原采样 → 计算 → 控制 → 保护 → 状态/录波 → DAC 链路。中断函数仅删除原 VOFA 降采样计数和通知标志；通信依赖后台时基。INT5 映射 GPIO Bank4，GPIO69 上升沿触发保持一致。DPRAM关闭时，既有FT3录波中的故障字改从本拍本地`Port_RealFault`读取，避免原代码仍读取已关闭的CS5；此项是编译期选择，仍为一次16位赋值，不增加ISR处理步骤。DPRAM启用时该录波来源保持原代码。

新通信不注册 UART RX/TX 中断，主循环限预算轮询 UART2；不会新增低优先级 UART ISR 阻挡 TAD。新故障编码、历史与报文处理不加入 TAD。新后台运行路径不全局禁止 DSP 中断。

TI 原中断入口不自动允许低优先级 ISR 内被高优先级抢占，因此原 Timer2/FAULT ISR 的最大执行时间仍需板级核验。软件回归及编译确认结构与功能等价，不能证明物理外部触发的最大延迟。测量要求见 BOARD_ACCEPTANCE.md。

## 地址与兼容约束

- `C6748.cmd` 原 FPGA/DPRAM section 地址与位域结构保持。
- PcDebug 帧头 0x6B，F0～FF 请求、70～7F 应答、CRC8、功能码组和组内序号、显示索引维持参考格式。
- 将参考 C28x 的依赖类型替换为明确的 16 位类型，传输显式编码高低字节，禁止通过结构体原始内存迁移通信。
- 波形与状态使用 16 位传输的观测适配，浮点控制参数以映射表中的量纲/比例接入。
- VOFA 使用原小端 float32 和 JustFloat 帧尾，默认后台限频，发送结束等待改为非阻塞检查。
- DPRAM 的启用和控制权限由独立宏明确；仿真器模式不接受 DPRAM 或串口远程启动。
- 原CS5地址对象保持链接地址，其19个DPRAM section在链接文件标为`NOINIT`，避免C运行库在EMIFA初始化前清零外部通信区。DPRAM关闭时后台不读写、GPIO中断不注册；启用后由原后台收发使用实际内容。普通RAM中的`PORT_*_Buf`仍初始化，串口不依赖映射DPRAM对象。

DPRAM控制块仍使用原地址及字位图：START/STOP的bit0，PLUSE_ON/OFF的bit0为DAB、bit1为CHB，FAULT_CLEAR的bit0请求停机复位；其余位保留。STOP优先于START，OFF优先于同一路ON。新`SstCommand`适配只修改核心命令，PWM由原控制状态机驱动。旧直接开关器件/门极命令不再从DPRAM控制块执行；REGULATE仍更新原兼容对象，不能当作新的CHB参数表。新的CHB调参使用AnyEyes表或调试器。

参数保存与旧维护保存共享忙标志，存储期间远程启动和修改拒绝/过滤，停止和封波请求仍有效。仿真器直接写RAM属于调试操作，工程无法拦截调试器写入；保存时若检测到运行，则中止并报告忙，保持即时保护链。

## 验证层次和交付边界

已提供 TI 编译脚本、协议测试、故障测试、原/新算法对照测试及实时链路静态检查。当前构建结果与软件测试记录见 VALIDATION.md。板级验收包含外部中断延迟、输出引脚、EEPROM、AnyEyes 客户端和故障注入，需按 BOARD_ACCEPTANCE.md 实施后记录实测值。
