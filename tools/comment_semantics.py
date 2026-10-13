"""Reviewed Chinese symbol descriptions used by the one-time source annotation."""
FUNCTIONS = {}
def functions(rows, context='', output=''):
    for line in rows.strip().splitlines():
        name, purpose, *rest = line.split('|')
        FUNCTIONS[name] = dict(purpose=purpose, result=rest[0] if rest else '无', context=context, output=output)

functions('''
Clarke_Transformation|将三相瞬时量变换为静止坐标系 α、β 分量，保持原幅值不变系数|无
Clarke_InverseTransformation|将 α、β 分量逆变换为三相瞬时量|无
Park_Transformation|按输入角度的正余弦将 α、β 分量变换为 d、q 分量|无
Park_InverseTransformation|按输入角度的正余弦将 d、q 分量逆变换为 α、β 分量|无
ConstantPID_Controller|执行原增量 PI 差分式并更新 PID_Control_buf 的上一拍输入、输出|本拍控制输出，与 ErrorIn 的量纲及控制器系数对应
H_Filter_Memcopy|复制滤波器的四个历史状态浮点数|无
''', '控制计算路径；沿用原运算顺序。', '坐标变换写入对应的 *_Trans 全局量；状态复制写入 DestAddr。')
functions('''
Modualation_Test|保留原调制测试入口，当前函数体内的测试代码被注释关闭
Write_ControlParameter_50Hz|将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
Write_ControlParameter_DAB|写入当前端口 DAB 左右桥占空比及脉冲起始计数
Write_ControlParameter_DAB_Startup|初始化 DAB 的周期和启动时序寄存器
PortController_Communication|触发当前端口主控与插件之间的 FPGA 读写通信
Module_Control_Write|将 Module_PM_CTL 控制字写入当前端口的模块插件
TX_EN_Control_Write|将各相在线模块位图写入 FPGA 发送使能寄存器
Module_PWMEN|按位图设置模块工频桥、DAB 桥的 PWM 使能
Module_50Hz_PWMEN|设置模块工频桥 PWM 使能并回写 FPGA 控制字
Module_ENdab_PWMEN|设置模块 DAB 桥 PWM 使能并回写 FPGA 控制字
MOSFET_50Hz_FrequencyDoubling|设置工频桥与 DAB 桥的驱动分频控制位
MOSFET_DriverReset|设置工频桥与 DAB 桥驱动复位控制位
Module_HW_RST|设置模块硬件复位控制位
Module_FAN_OFF|设置模块风扇关闭控制位
Module_Shutdown_valid|设置两类桥的硬件关断有效控制位
Module_TestMode|设置模块控制字中的测试模式字段
Port_IO_Out|将 Port_DOUT 开关控制字写入端口 FPGA 数字输出寄存器
Port_ADProtect_EN|写入端口 AD 硬件保护使能位图
Port_ModuleShutdown|按光纤插件编号写入各组模块关断位图
Port_FO_EN_Set|设置光纤故障输入的各相使能位图
INV_CONTROL_DEADZONE_Set|设置独立逆变桥的 FPGA 死区计数
INV_CONTROL_PERIOD_Set|设置独立逆变桥的 FPGA 周期计数
Port_BKU_Send|写入 FPGA BKU 故障锁存控制值
Port_FB_EN_Set|写入端口 FB 硬件保护使能位图
Port_DSPControl_FBRST|设置端口 FPGA 的硬件故障复位控制位
Port_DSPControl_AllShutdown|设置端口 FPGA 的全模块关断控制位
Module_StatusRead|读取指定插件、模块的实时或锁存状态字|模块的 16 位原始状态字
Module_TotalStatusRead|读取指定插件所有模块的汇总状态|插件的 16 位原始汇总状态
Port_DINRead|读取端口数字输入的实时或锁存值|16 位数字输入位图
Port_AD_ProtectStatusRead|读取端口 AD 硬件保护的实时或锁存值|16 位 AD 保护位图
Port_FOStatusRead|读取指定插件的光纤故障实时或锁存值|16 位光纤故障位图
Port_FBStatusRead|读取端口 FB 故障的实时或锁存值|16 位 FB 故障位图
Port_INVFOStatusRead|读取独立逆变桥光纤故障的实时或锁存值|16 位逆变桥光纤故障位图
FT3_PluginUint_Write|组装 FT3 采样、状态及故障数据并写入所选 FT3 插件
FT3_PluginUint_Read|读取所选 FT3 插件接收区到兼容接收缓存
DAC_Output|将 DAC_BUF 四个通道值写入主控插件的 DA 寄存器
''', '底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。', '写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。')
functions('''
MemDataCopy|按 16 位字复制指定长度的内存数据
MemDataCopy_1|将指定长度的普通内存数据复制到 volatile 外设区
MemDataCopy_2|从 volatile 外设区复制指定长度数据到普通内存
DPRAM_Write|把指定遥测、插件或遥调缓存回写到 DPRAM 并通知对端
DPRAM_Read|读取 DPRAM 新块标识以及对应遥控或遥调块
DPRAM_ReadDataDispose|分派 DPRAM 缓存中的遥控、遥调数据处理
RemoteControl_Dispose|按旧版遥控块处理启动、停止、脉冲、开关及复位控制
RemoteRegulate_Dispose|把遥调块定点值换算为浮点给定及控制器参数
RemoteRegulate_Dispose_2|按所选来源更新端口遥调参数、模块配置及控制系数
RemoteSig_Dispose|把端口浮点遥测及状态换算为 DPRAM 16 位上报缓存
''', '旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。', '目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。')
functions('''
r_eeprom_protect|写 EEPROM 控制寄存器以恢复写保护|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_eeprom_unprotect|写 EEPROM 控制寄存器以允许写入|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_eeprom_byte_write|通过软件 I²C 写入 EEPROM 单字节，保留最多三次重试|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_eeprom_section_write|通过软件 I²C 写入指定页内的一段字节数据|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_eeprom_byte_read|通过软件 I²C 读取 EEPROM 单字节|读取的字节；失败沿用原函数的返回约定
r_eeprom_section_read|通过软件 I²C 读取指定页内的一段数据|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_eeprom_page_read|读取 EEPROM 指定页的 64 个字节|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_iic_start|按 GPIO 时序产生软件 I²C 起始条件
r_iic_stop|按 GPIO 时序产生软件 I²C 停止条件
r_iic_send_byte|按高位优先在 SDA/SCL 上发送一个字节
r_iic_wait_ack|读取 SDA 以检查从器件的应答|VALIDFLAG 表示应答有效，INVALIDFLAG 表示无应答
r_iic_rec_byte|按 SCL 时序采集 SDA 的八位数据|接收的一个字节
r_iic_send_ack|向从器件发送低电平 ACK 应答
r_iic_send_nack|向从器件发送高电平 NACK 应答
r_eeprom_control_write|写入 EEPROM/RTC 的控制寄存器|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_time_read|通过软件 I²C 读取 RTC 时间寄存器到调用者缓存|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
r_time_write|通过软件 I²C 写入调用者提供的 RTC 时间字段|VALIDFLAG 表示成功，INVALIDFLAG 表示失败
DELAY_US|执行原 EEPROM 驱动的循环延时，实际时长依赖时钟及编译优化
BCD2HEX|把两位压缩 BCD 转为十进制数值|转换后的二进制整数数值
HEX2BCD|把十进制数值转为两位压缩 BCD|高半字节为十位、低半字节为个位
''', '板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。', '读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。')
for page in range(4):
    functions(f'EEPROM_Page{page}_2_DSP|读取旧版 EEPROM 第 {page} 页并恢复对应端口参数\nDSP_2_EEPROM_Page{page}|把对应端口旧版参数打包写入 EEPROM 第 {page} 页', '旧版存储兼容函数；页映射保持原项目定义。', '参数全局量或指定 EEPROM 页。')
functions('''
SstBoard_GetTick1ms|联合读取 Timer2 计数及 10 ms 节拍，生成单调的 1 ms 后台时间基准|32 位毫秒节拍，允许自然回绕
Delay|执行板级初始化使用的原循环延时
PSCInit|使能板级外设对应的 PSC 电源/时钟模块
GPIOBankPinMuxSet|配置板级 GPIO 的引脚复用
GPIOBankPinInit|初始化 LED、看门狗、保护和通信相关 GPIO 方向与初值
EMIFASetup|配置 FPGA/DPRAM 所用 EMIFA 时序及总线引脚
TimerInit|初始化 Timer2 周期计数，沿用原 10 ms 定时配置
TimerInterruptInit|将 Timer2 中断源与原 ISR 及 CPU 向量绑定
UARTInit|初始化 UART2 波特率、数据格式及 FIFO
UARTInterruptInit|保留 UART 初始化兼容入口，调试收发采用后台轮询
GPIOBankPinInterruptInit|绑定 FPGA TAD、FAULT 以及可选 DPRAM 通知 GPIO 中断
InterruptInit|初始化 DSP 中断控制器并使能所需 CPU 中断通道
EMIFAPinMuxSetup|设置 EMIFA 地址、数据及控制引脚的复用功能
UARTPinMuxSetup|按 UART 实例及流控选择设置串口引脚复用
''', '板级初始化或后台时基服务；不引入新的实时 ISR 工作。', '相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。')
functions('''
ChbApplication_InitHardware|按原顺序完成时钟、EMIFA、GPIO、Timer2 和指示灯初始化
ChbApplication_WaitPowerOnDelay|等待原 20 s 上电延时到达并恢复指示灯、看门狗初值
ChbApplication_InitRuntime|初始化运行对象及可选持久化参数，随后开放 TAD/FAULT 中断
ChbApplication_ServiceHeartbeat|消费 1 s 标志，翻转看门狗及运行指示灯
ChbApplication_ServiceRemoteControl|读取可选 DPRAM 遥控/遥调块并交给统一命令入口
ChbApplication_ServiceTelemetry|按轮询顺序回写可选 DPRAM 端口遥测及有效插件块
ChbApplication_ServiceNormal|执行主循环的正常状态、通信、故障上报及参数存储任务
ChbApplication_ServiceOpenLoopBridge|保留原开环桥接服务入口并交给迁移后的主循环服务
Initial_Variable|按原项目初始化控制系数、模块位图、保护阈值及运行变量
Variable_RST|复位滤波器、PI、遥控、故障以及原运行流程的历史状态
Udc_check|依次检查各相在线模块直流电压，更新预充及高压判断标志
main|执行板级初始化和上电等待，然后持续调度迁移后的后台服务
CRC16|计算旧 UART 兼容帧的 CRC16，保持原多项式及交换字节规则|原实现生成的 16 位 CRC 值
keyscan|保留原本地按键扫描入口及按键状态处理
Timer2Isr|应答 Timer2 中断并更新原软件定时、状态及保护服务
UART2Isr|保留原 UART2 ISR 符号；调试通信主流程不注册此中断
FPGAIsr|应答 FPGA TAD 外部中断并按原顺序完成 10 kHz 采样、保护及控制
FAULTIsr|应答 FPGA 硬件故障中断并执行原快速故障处置
DPRAMIsr|应答可选 DPRAM GPIO 通知并置后台读取标志
UARTReceiveDispose|保留旧 UART 接收处理声明；迁移工程中无对应实现或调用
Uart2_send|保留旧 UART 发送声明；迁移工程中无对应实现或调用
''', '入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。', '板级寄存器、控制/采样全局量及相应后台任务状态。')
functions('''
ReadEntry|读取映射参数并按比例、符号和 16 位范围饱和编码|16 位线上值，有符号参数使用二补码
WriteEntry|检查线上值范围后，反比例换算并写入映射成员|SST_PARAM_OK 或 SST_PARAM_RANGE
SstControl_GetTick10ms|读取控制状态中的 10 ms 节拍|32 位 10 ms 计数，允许自然回绕
SstParams_IsStorageBusy|查询 AnyEyes 与 DPRAM 共用参数存储服务的忙状态|1 表示存储正在进行，0 表示空闲
SstParams_IsStopped|同时检查状态、启动命令和两类 PWM 的停机条件|1 表示可以执行停机专属操作，0 表示不可
SstParams_RunCommand|检查串口控制权限及存储互锁后提交 CHB/DAB 启停命令|SST_PARAM_* 操作结果
SstParams_EmergencyStop|无条件撤销 CHB 和 DAB 启动命令，由控制核心执行停机
SstParams_Read|按固定组号及组内地址读取参数、显示量、历史或存储状态|SST_PARAM_OK 或 SST_PARAM_BAD_ADDRESS
SstParams_Write|校验地址、权限、范围及停机互锁后修改参数或提交存储请求|SST_PARAM_* 操作结果
Scale|把物理浮点值按指定比例饱和、四舍五入为 16 位线上值|16 位编码值，NaN 返回 0
SstTelemetry_ReadFloat|按固定索引读取 VOFA 的原始浮点遥测量|对应物理量；未知索引返回 0
SstDisplay_Refresh|刷新固定 200 字显示表中的状态、波形、参数及故障报告位置
SstDisplay_Read|读取固定显示表的一个 16 位字|表内值；越界返回 0
StoreCrc|计算参数存储镜像的 CRC16，初值 FFFF、多项式 A001|16 位 CRC 校验值
StorePut|按小端顺序把一个 16 位字写入 storageImage
StoreGet|按小端顺序从 storageImage 读取一个 16 位字|镜像中的 16 位字
StoreLoad|停机时读取页 6/7 的 92 字节镜像，整体校验后恢复可持久化参数|SST_PARAM_* 操作结果
StoreRequest|在停机且存储空闲时执行加载，或建立异步保存镜像|SST_PARAM_* 操作结果
SstParams_Service|每 10 ms 至多推进一次单字节写入或读回校验，最后提交镜像有效标记
SstParams_GetStorageResult|读取最近一次参数保存/加载的结果|SST_PARAM_* 结果码
SstParameters_LoadPersistent|在初始化阶段尝试恢复参数并记录存储结果
GetOneValueFromRealMem|把参考 FuncCode 读接口转接到 SST 固定参数地址表|成功返回 1，失败返回负的 SST_PARAM_* 结果
SetOneValue2BothMem|把参考 FuncCode 写接口转接到 SST 参数校验及写入服务|成功返回 1，失败返回负的 SST_PARAM_* 结果
''', '后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。', '读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。')
functions('''
SstAnyEyes_Crc8|计算参考 PcDebug 的 CRC8：初值 0、多项式 07、结果取反|帧尾 CRC8 字节
Init|清空 PcDebug 工作标志及接收状态，初始化后台上报时基
RxByte|收集单字节并检查帧头、长度、超时与 CRC，完整帧交给命令解析
ParseInstruct|按参考命令码校验载荷、设置参数读写/波形/面板标志并转接启停命令|有效命令返回 1，非法或未连接命令返回 -1
Pack|按参考帧格式打包字节载荷及 CRC，并将整帧提交非阻塞发送队列|true 表示入队成功，false 表示长度非法或队列满
ExecuteInstruct|执行待处理参数读写及应答，并按毫秒节拍发送波形、状态和心跳|未连接返回 0，已连接服务返回 1
MainFSM|检查 PC 心跳超时并推进参考 PcDebug 后台任务
SstAnyEyes_Init|初始化全局 PcDebug 对象
SstAnyEyes_RxByte|将后台 UART 收到的一个字节交给 PcDebug 解析器
SstAnyEyes_Service|在主循环执行 PcDebug 状态机及周期上报
SstDebug_GetTick10ms|读取控制核心的 10 ms 时间基准|32 位 10 ms 节拍
SstDebug_GetTick1ms|读取板级单调毫秒时间基准|32 位 1 ms 节拍
SstDebug_GetTxDropped|读取发送队列满或帧非法导致的丢弃计数|累计丢帧数
SstSerial_TxFreeFrames|查询共享发送队列剩余整帧容量|可用帧槽数量
SstSerial_SendFrame|验证帧长并复制到共享发送队列，不等待 UART 硬件|1 表示入队成功，0 表示失败
SstSerial_TxService|在字节预算内向可写的 UART FIFO 推进发送队列
SstSerial_RouteByte|将 UART 接收字节交给编译选中的 AnyEyes 解析器；VOFA只发送波形
SstDebug_Init|初始化共享收发状态、UART2 及编译选中的调试协议
SstDebug_Tick10ms|保留后台 10 ms 调试任务入口，推进故障状态报告
SstDebug_Service|以固定 RX/TX 字节预算轮询 UART 并调度所选协议、故障与参数服务
VOFA_Init|初始化 VOFA 波形调试入口
VOFA_Process|按发送周期采集 SST 九个浮点通道并提交 JustFloat 帧
VOFA_SendFloatArray|把最多 VOFA_MAX_CHANNELS 个浮点值按原字节顺序组成 JustFloat 帧|1 表示共享队列接受该帧，0 表示参数或队列无效
VOFA_Send4Float|按通道顺序发送四个浮点波形值
VOFA_Send6Float|按通道顺序发送六个浮点波形值
VOFA_Send8Float|按通道顺序发送八个浮点波形值
VOFA_Send12Float|按通道顺序发送十二个浮点波形值
''', '主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。', '协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。')
functions('''
Chb_Clamp|把输入值限制在指定上下限之间|限幅后的浮点值
Chb_Abs|计算浮点数绝对值|非负绝对值
Chb_Min3|比较三个浮点量并取最小值|三者最小值
Chb_Max3|比较三个浮点量并取最大值|三者最大值
Chb_WrapAngle|将角度归一化到 0 至 2π 的周期区间|归一化的弧度角
Chb_SourceEquivalentPiStep|按原增量 PI 等效式更新积分状态并限幅输出|限幅后的本拍 PI 输出
Chb_ResetControllers|清空电压、电流 PI 及参考状态
Chb_ResetStartupSequence|复位 Id 斜坡与电压环接管阶段的历史状态
Chb_AlignVoltagePiToIdReference|根据当前 Id 给定预置电压 PI 积分，减小接管突变
Chb_UpdateVoltageReference|按启动策略更新直流电压外环给定及状态镜像
Chb_WriteNormalizedReferences|将三相归一化调制指令映射到原 FPGA 工频桥参考格式
Chb_WriteNeutralReferences|写入原 FPGA 中性参考值，保持停机安全输出
Chb_WriteDabDuty|限幅 DAB 计数并更新各相占空比、起始计数及 FPGA 寄存器
Chb_SetAcInput|设置交流输入断路器的通断脉冲及状态
Chb_ClearAcInputPulse|撤销已到期的交流输入断路器控制脉冲
Chb_SetPrechargeBypass|设置预充旁路接触器及状态镜像
Chb_DisableAllPwm|撤销工频桥和 DAB PWM 使能并清空状态镜像
Chb_SetLegacyStatus|把简化控制状态映射到原工程端口状态字
Chb_EnterState|切换控制状态并记录状态进入的 10 ms 节拍
Chb_ConfigurationValid|检查模块数、周期、给定范围和控制参数是否有效|1 表示配置有效，0 表示非法
ChbDabControl_Init|清空控制状态并初始化安全 PWM、开关及原兼容状态
ChbDabControl_Tick10ms|递增控制状态的 10 ms 节拍
ChbDabControl_ApplyRemoteCommand|按停止优先原则处理遥控启停字，受宏及存储互锁约束
ChbDabControl_ForceFault|锁存控制故障并关闭 PWM，进入故障状态
Chb_ServiceOpenLoop|处理开环模式下独立 CHB/DAB 启停及故障复位
ChbDabControl_Service|每个 10 ms 节拍推进预充、旁路、PLL 等待、运行和停机状态
Chb_UpdatePll|依据 αβ 电压执行归一化 q 轴误差 PI，更新相角、频率及锁定计数
Chb_RunOpenLoop|推进开环正弦相角并写入三相限幅调制参考
Chb_GetClosedLoopIdReference|执行兼容启动或 Id 斜坡，并在满足条件时移交电压外环|本拍闭环 d 轴电流给定，单位 A
Chb_RunClosedLoop|按原反馈方向执行 dq 电流闭环及电压前馈，输出三相调制参考
Chb_CheckSoftwareProtection|确认软件交流过流与直流过压连续样本并提交控制故障
ChbDabControl_FastStep|在原 TAD 路径执行 PLL、开环/闭环控制、软件保护及 DAB 占空比斜坡
SstCommand_ApplyDpram|按停止和关脉冲优先规则将 DPRAM 字段接入统一控制命令
''', '控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。', 'CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。')
functions('''
U_Decomposition_PLL|对电网电压作坐标变换及序分量滤波，推进原锁相环相角
Iin_Decomposition|对端口电流作 dq 变换并分离正序、负序及谐波分量
Iload_Decomposition|对负载电流作 dq 变换并提取基波与谐波分量
P_Q_Calculator|根据电网电压、电流 dq 分量计算并滤波有功、无功功率
SVG_Q_Reference|按旧 STATCOM 工作模式生成无功功率目标
SVG_Udc_Average|按各相在线模块数计算相平均及三相平均直流电压
SVG_Total_Udc_Controller|按旧直流电压参考执行增量 PI，生成 d 轴电流参考
SVG_I_Reference|按旧 STATCOM/APF 模式合成正序、负序及谐波电流参考
SVG_Current_Controller|执行旧 dq 电流环、谐振补偿及三相 SPWM 给定合成
SVG_Controller|按旧 SVG 控制链顺序执行分解、功率、参考和电流控制
''', '旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。', '旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。')
for h, bw in ((2,100),(6,100),(12,100),(2,20)):
    functions(f'BandH{h}_{bw}_Filter|执行原 H{h}/{bw} 二阶滤波差分式，更新对应四个历史状态|当前滤波输出', '原 HVAC 计算子函数，系数与运算顺序保持原代码。', '对应 H_Filter 全局缓冲区。')
for h in (2,6,12):
    functions(f'SineHar_{h}_Controller|执行原 {h} 次谐波谐振控制差分式并更新历史输入输出|当前谐振控制输出', '原 HVAC 控制子函数，谐振系数保持原代码。', f'Har_{h}_Controller 历史状态。')
functions('''
PortStartup_Effectiveness|检查旧端口启动优先级及条件，判定启动请求是否有效
Port_StateUpdate_Read|读取并解码端口主状态和运行模式
Port_StateUpdate_Write|将端口运行状态及工作模式编码回状态字
Port_AllStopFlag_RST|按旧全端口停机流程复位相关标志
Judge_MainPowerSupply|根据输入开关及电压条件判断主电源投入状态
NormalRun_flow|推进旧正常运行流程中的启动、停止及开关操作
Start_flow|按编译端口推进旧自动预充及启动顺序
Start_flow_Manual|按编译端口推进旧手动启动与脉冲使能顺序
Stop_flow|按旧停机类型撤销脉冲并处理开关断开及延时
SwitchTable_Start|建立所选端口的开关操作时序与初始状态
SwitchOperation_Dispose|根据时序标志执行旧开关操作并判定完成
''', '保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。', 'Port_State、ControlFLAG、SW 及旧流程定时标志。')
functions('''
OC_OV_LV_faultcheck|按原阈值和计数策略检查端口交流过流、直流过压及欠压条件
OC_OV_LV_FaultDispose|将原软件电压、电流保护判据转换为端口故障及处置标志
FaultCheckAndDispose_StepOne|读取端口 AD、FB、FO 锁存并执行原第一阶段快速故障判断
FaultDispose|逐模块分类故障并按原策略撤销 PWM、记录故障类型及处置状态
FaultLatch_Clear|发起或推进原两拍故障锁存清除流程
PWMPluseEnable_Initial|初始化原 PWM 使能及故障处置控制字
PWMPluse_Enable|按旧故障处置策略使能指定桥类型的 PWM
PWMPluse_Disable|按旧故障处置策略禁用指定模块或指定桥类型的 PWM
''', '原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。', '原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。')
functions('''
SstFault_ReadInput|在后台收集原故障锁存与控制状态，检查采集前后关键状态一致性|1 表示本轮可用，0 表示下次主循环再采集
SstFault_Add|对当前故障去重，记录等级并写入详细历史和参考组 19 历史环
SstFault_Init|清空当前故障与 RAM 历史，初始化参考历史环写位置
SstFault_Service|把原控制/硬件故障映射为参考五位故障码和等级并更新报告
SstFault_GetCode|按当前故障索引读取报告码|故障码；越界返回 0
SstFault_GetFirstCode|读取当前首个故障报告码|首故障码；无故障返回 0
SstFault_GetLevel|读取当前故障中的最高严重等级|参考等级 0..4，4 表示无故障
SstFault_GetActiveCount|读取当前故障去重数量|当前故障数量，最多 32
SstFault_GetHistoryCount|读取 RAM 详细历史的有效记录数|有效历史数量，最多 32
SstFault_GetLegacyHistoryWord|读取参考 FuncCode 组 19 的原布局历史环字|字 0 为下次写位置，字 1..33 为故障码；越界返回 0
SstFault_HistoryIndex|将按新到旧的历史序号换算为环形缓存物理下标|环形数组下标
SstFault_GetHistoryCode|按新到旧读取详细历史故障码|故障码；越界返回 0
SstFault_GetHistoryLevel|读取 RAM 历史中最高严重等级|参考等级 0..4
SstFault_GetHistory|按新到旧复制一条详细历史记录给调用者|1 表示复制成功，0 表示指针为空或越界
''', '纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。', '当前故障表、RAM 历史环及调用者的记录输出缓存。')

VARIABLES = {}
def variables(rows):
    for line in rows.strip().splitlines():
        if not line.strip(): continue
        name, meaning = line.split('|', 1)
        VARIABLES[name] = meaning

variables('''
address|被映射控制成员的地址，只允许表内已绑定的对象
scale|物理值到 16 位线上值的乘数，写入时使用其倒数
minimum|允许的最小值；映射表中按线上整数范围定义
maximum|允许的最大值；映射表中按线上整数范围定义
isSigned|1 表示线上值按有符号 16 位二补码解释
isWord|1 表示目标是 unsigned short，0 表示 float
commands|组 0 的八个控制命令/给定映射，顺序即固定组内地址
parameters|37 个控制参数的固定地址、比例及范围映射
display|200 字显示表，索引与参考面板及波形地址保持一致
storageResult|最近一次参数保存/加载的 SST_PARAM_* 结果
storagePending|异步参数保存正在执行的标志
storagePosition|参数镜像写入/提交/读回校验所处的步序
storageTick|上次推进参数存储任务的 10 ms 节拍
storageImage|页 6/7 的 92 字节参数镜像，包含魔数、版本及 CRC
g_chb_dab_command|调试器/通信提交的控制命令与给定，控制核心读取执行
g_chb_dab_params|CHB/DAB 控制参数；调试器可观察，通信写入需停机校验
g_chb_dab_status|控制核心维护的运行状态镜像，后台仅用于读取上报
g_chb_dab_protection_enable|新增软件过流/过压保护使能，0 关闭、1 开启；硬件保护不受其影响
run_enable|CHB 启动命令，0 停止、1 启动；开环直接解锁，闭环走预充流程
dab_enable|DAB 原边使能命令，0 停止、1 启动；开环可独立控制
ac_mode|工频桥模式，0 开环逆变、1 闭环运行
fault_reset|停机时置 1 请求复位控制故障，控制核心消费后清零
open_loop_modulation|开环调制比，范围 0..modulation_max
open_loop_frequency_hz|开环正弦输出频率，单位 Hz
dc_voltage_ref_v|单模块直流电压目标，单位 V
iq_reference_a|闭环 q 轴电流目标，单位 A
sample_period_s|TAD 计算周期，单位 s，默认 0.0001；通信只读，不改变硬件中断频率
modulation_max|三相调制比绝对值上限，默认 0.98
precharge_voltage_v|三相单模块最低直流电压的预充达标阈值，单位 V
precharge_stable_10ms|预充电压须持续达标的 10 ms 拍数
precharge_timeout_10ms|预充允许的最大等待时间，单位 10 ms
bypass_delay_10ms|投入预充旁路后的等待时间，单位 10 ms
breaker_pulse_10ms|交流输入断路器合/分闸脉冲宽度，单位 10 ms
pll_nominal_frequency_hz|PLL 额定电网频率，单位 Hz
pll_kp|PLL 归一化 q 轴误差比例系数
pll_ki|PLL 归一化 q 轴误差积分系数，积分乘 sample_period_s
pll_correction_limit_rad_s|PLL 角速度修正绝对值上限，单位 rad/s
pll_lock_error_pu|PLL 锁定允许的归一化 q 轴误差幅值
pll_min_grid_peak_v|PLL 有效电网电压矢量幅值下限，单位 V
pll_lock_10ms|PLL 连续锁定确认时间，单位 10 ms
pll_timeout_10ms|PLL 等待锁定超时，单位 10 ms
voltage_kp|直流电压外环比例系数，沿用原增量 PI 等效值
voltage_ki|直流电压外环积分系数，积分乘 sample_period_s
voltage_current_limit_a|电压外环输出 Id 的绝对值上限，单位 A
startup_strategy|闭环启动策略，0 原版兼容、1 固定 Id 斜坡后移交电压环
startup_udc_offset_v|PWM 解锁时叠加在实测电压上的给定偏移，单位 V
startup_udc_step_v|电压环接管后每个 TAD 的电压给定步长，单位 V
startup_id_reference_a|启动阶段固定 Id 目标/状态镜像，单位 A
startup_id_ramp_step_a|启动阶段每个 TAD 的 Id 斜坡步长，单位 A
startup_ramp_tad|固定 Id 启动斜坡最长持续拍数，单位 TAD
current_kp|dq 电流内环比例系数，保留原反馈方向对应的符号
current_ki|dq 电流内环积分系数，积分乘 sample_period_s
closed_loop_voltage_limit_factor|dq 电压指令上限相对电网幅值的比例
current_feedback_sign|电流采样方向系数，只允许 +1 或 -1
current_phase_compensation_rad|电流坐标变换相位补偿，单位 rad
output_phase_compensation_rad|输出电压坐标逆变换相位补偿，单位 rad
ac_overcurrent_trip_a|新增软件交流过流阈值，单位 A
dc_overvoltage_trip_v|新增软件单模块直流过压阈值，单位 V
protection_confirm_samples|新增软件保护所需的连续超限 TAD 样本数
dab_start_duty_count|DAB 使能时初始占空比计数，默认 350
dab_target_duty_count|DAB 斜坡目标占空比计数，上限 2500
dab_ramp_step_count|每个 TAD 的 DAB 占空比递增计数，默认 1
dab_start_count|DAB PWM 起始相位计数，范围 0..4999
state|CHB_STATE_* 运行状态编号
fault_code|CHB_FAULT_* 控制故障编号，区别于五位上报故障码
ac_input_closed|交流输入闭合状态镜像，非零表示闭合
precharge_bypassed|预充旁路状态镜像，非零表示已旁路
ac_pwm_enabled|CHB 工频桥 PWM 使能状态镜像
dab_pwm_enabled|DAB PWM 使能状态镜像
pll_locked|PLL 连续锁定已确认标志
dab_duty_count|当前实际下发 DAB 占空比计数
tick_10ms|10 ms 软件节拍；故障记录中为后台观察时刻，不是精确跳闸时间
dc_voltage_average_v|三相单模块平均直流电压，单位 V
dc_voltage_min_v|三相单模块平均电压中的最小值，单位 V
pll_angle_rad|PLL 当前电网相角，单位 rad
pll_frequency_hz|PLL 当前估计电网频率，单位 Hz
id_feedback_a|闭环 d 轴电流反馈，单位 A
iq_feedback_a|闭环 q 轴电流反馈，单位 A
id_reference_a|闭环当前 d 轴电流给定，单位 A
startup_voltage_reference_v|启动过程中的电压外环给定，单位 V
startup_phase|CHB_STARTUP_* 启动阶段编号
startup_tad_count|启动斜坡已执行的 TAD 拍数
voltage_loop_active|电压外环已接管 Id 给定的标志
modulation_actual|当前三相调制指令的最大绝对值
integral|PI 积分/等效历史状态，限幅后参与下一拍输出
s_voltage_pi|直流电压外环的历史积分状态
s_id_pi|d 轴电流内环的历史积分状态
s_iq_pi|q 轴电流内环的历史积分状态
s_pll_integral|PLL 角速度修正的积分状态，单位 rad/s
s_open_loop_angle|开环输出当前相角，单位 rad
s_id_reference|控制核心当前 Id 给定，单位 A
s_state_enter_tick|当前状态进入时的 10 ms 节拍
s_breaker_pulse_end_tick|断路器控制脉冲结束的 10 ms 节拍
s_last_service_tick|主循环上次推进状态机的 10 ms 节拍
s_precharge_stable_count|预充连续达标的 10 ms 拍数
s_pll_lock_fast_count|PLL 连续锁定的 TAD 样本计数
s_pll_lock_10ms_count|PLL 连续锁定的 10 ms 确认计数
s_ac_overcurrent_count|新增交流过流连续超限样本数
s_dc_overvoltage_count|新增直流过压连续超限样本数
s_startup_phase|核心内部闭环启动阶段编号
s_startup_tad_count|核心内部启动斜坡 TAD 计数
s_startup_voltage_reference|核心内部启动电压给定，单位 V
target_voltage|最终直流电压目标，单位 V
measured_voltage|当前单模块平均实测直流电压，单位 V
reference|由归一化调制指令换算的原 FPGA 参考值
elapsed|当前状态已经历的 10 ms 节拍数
magnitude|PLL 输入 αβ 电压矢量幅值，单位 V
sine_angle|当前 PLL 相角的正弦值
cosine_angle|当前 PLL 相角的余弦值
q_voltage|PLL 坐标变换的 q 轴电压，单位 V
q_error|按电压矢量幅值归一化的 PLL q 轴误差
correction|PLL PI 输出的角速度修正，单位 rad/s
omega|电网额定角速度加 PLL 修正量，单位 rad/s
lock_samples|由锁定时间及 sample_period_s 换算的所需 TAD 样本数
modulation|限幅后的开环调制比
phase_a|A 相指令；开环为调制比、闭环为电压量，在写入前归一化
phase_b|B 相指令；开环为调制比、闭环为电压量，在写入前归一化
phase_c|C 相指令；开环为调制比、闭环为电压量，在写入前归一化
startup_target|启动阶段最终 Id 目标，单位 A
ramp_step|当前 Id 斜坡每拍增量，单位 A/TAD
voltage_error|直流电压给定减反馈的误差，单位 V
id_target|本拍计算出的 Id 目标，单位 A
handover_now|本拍需要从启动斜坡移交电压环的标志
current_sine_angle|加入电流相位补偿后的正弦值
current_cosine_angle|加入电流相位补偿后的余弦值
voltage_sine_angle|电网电压 dq 变换的正弦值
voltage_cosine_angle|电网电压 dq 变换的余弦值
output_sine_angle|加入输出相位补偿后的逆变换正弦值
output_cosine_angle|加入输出相位补偿后的逆变换余弦值
current_alpha|端口电流 α 轴分量，单位 A
current_beta|端口电流 β 轴分量，单位 A
current_d|端口电流 d 轴分量，单位 A
current_q|端口电流 q 轴分量，单位 A
voltage_d|电网电压 d 轴前馈分量，单位 V
voltage_q|电网电压 q 轴前馈分量，单位 V
vd_correction|d 轴电流 PI 电压修正，单位 V
vq_correction|q 轴电流 PI 电压修正，单位 V
command_d|限幅前/后的 d 轴输出电压指令，单位 V
command_q|限幅前/后的 q 轴输出电压指令，单位 V
command_alpha|输出电压 α 轴指令，单位 V
command_beta|输出电压 β 轴指令，单位 V
phase_dc_a|A 相用于调制归一化的单模块直流电压，单位 V
phase_dc_b|B 相用于调制归一化的单模块直流电压，单位 V
phase_dc_c|C 相用于调制归一化的单模块直流电压，单位 V
maximum_current|三相端口电流绝对值中的最大值，单位 A
voltage_alpha|电网/端口电压 α 轴分量，单位 V
voltage_beta|电网/端口电压 β 轴分量，单位 V
duty|本拍 DAB 占空比计数
step|DAB 占空比每个 TAD 的递增计数
''')
variables('''
sPc|后台独占的参考 PcDebug 协议对象
ReadFuncCodeFlag|待执行单参数读取标志，入队响应后清零
ReadAdsH|读参数组号，对应参考地址高字节
ReadAdsL|读参数组内索引，对应参考地址低字节
ReadData|保留参考读参数结果字段
WriteFuncCodeFalg|待执行单参数写入标志，沿用参考字段拼写
WriteAdsH|写参数组号，对应参考地址高字节
WriteAdsL|写参数组内索引，对应参考地址低字节
WriteData|待写入的 16 位参数编码
ScopeRunFalg|参考波形连续上报使能，沿用参考字段拼写
WorkChNum|实际波形通道数，范围 1..16
ScopeRunSendInvertTick|保留参考波形发送间隔计数
ScopeRunSendInvertTickUp|波形发送间隔，单位 ms，并按 UART 带宽设下限
ChannelSel|16 个波形通道的显示表索引，索引范围 0..199
ChannelData|参考波形通道值缓存
StatePanelRunFlag|状态面板连续上报使能
StatePanelPos|当前状态面板发送位置
StateValueSendInvertTick|保留参考状态面板间隔计数
NeedAckPcLinkResulFlag|连接握手结果等待发送的标志
PcLinkFlag|PC 连接握手成功标志
LostPcLifeTick|最近有效 PC 指令的毫秒节拍，用于连接超时判断
LifeTick|保留参考 MCU 心跳计数字段
stReadFunc|参考单参数读取工作状态
stWriteFunc|参考单参数写入工作状态
stScope|参考波形选择及发送工作状态
stStatePanel|参考状态面板工作状态
stPcLink|参考握手及心跳工作状态
ReadAllFuncCodeFlag|保留参考 F6 请求标志，参考实现未执行批量读取
WriteAllFuncCodeFlag|保留参考 F7 请求标志，参考实现未执行批量写入
QueryErrInfoFalg|保留参考 F9 查询标志，故障通过显示表/组 19 上报
QuerySoftInfoFalg|保留参考 FA 查询标志，参考实现未构造版本响应
stWorkFlag|PcDebug 参数、波形、状态和连接任务的聚合状态
rx|32 字节 AnyEyes 接收帧缓存
rxCount|当前 AnyEyes 已收字节数
rxExpected|当前 AnyEyes 帧的预期总字节数
lastRxTick|最近接收字节时刻，单位 ms
lastServiceTick|最近周期服务时刻，单位 ms，用于抑制同拍重复上报
lastScopeTick|最近波形上报时刻，单位 ms
lastPanelTick|最近状态面板上报时刻，单位 ms
lastLifeTick|最近 MCU 心跳上报时刻，单位 ms
FuncCode|参考参数访问接口，目标绑定到 SST 固定参数表
Instct|接收帧第 2 字节的参考命令码
raw|参数物理值缩放后的有符号整数中间值
frame|完整发送帧的本地字节缓存，包含帧头及校验
now|本轮后台服务的时间基准；通信使用 ms，参数保存使用 10 ms
crc|逐字节、逐位累加的 CRC 校验状态
bit|CRC 或软件 I²C 的当前位序号，范围 0..7
b|CRC 位循环序号，或三量比较的第二个输入
record|详细故障历史记录的指针；输出接口由调用者提供目标缓存
input|后台采集的原故障输入及控制状态
has_detail|本轮是否已获得可分类的故障明细
code|参考五位 SST_FAULT_* 上报故障码，0 表示无故障
level|参考故障等级，0 最严重、4 无故障
control_fault|原 CHB_FAULT_* 控制故障编号
port_fault|旧 Port_RealFault 的原始 16 位故障字
fault_type|旧 FaultTypeFLAG 原始 16 位故障分类位图
port_latch|端口 AD 保护锁存的后台副本
fb_latch|端口 FB 硬件故障锁存的后台副本
fo_a|A 相光纤故障锁存的后台副本
fo_b|B 相光纤故障锁存的后台副本
fo_c|C 相光纤故障锁存的后台副本
module_dispose|原模块故障处置计数的后台副本
sequence|维护协议请求序号，或详细故障记录的递增事件序号
software_oc|原 HVAC 软件过流故障标志的后台副本
software_ov|原 HVAC 软件过压故障标志的后台副本
reset_pending|原两拍故障清除尚未结束的标志
mask|故障原始字中与本映射条目对应的位掩码
s_type_map|旧 FaultTypeFLAG 各位到五位故障码及等级的映射
s_port_map|旧端口故障字的已核实位映射；重用位另由软件故障判定
s_current|本轮故障事件去重后的当前五位故障码表
s_current_count|当前故障码有效数量，上限 32
s_current_level|当前故障中最严重的参考等级
s_history|32 条详细故障历史的 RAM 环形缓存
s_history_count|详细历史的有效记录数量，上限 32
s_history_next|详细历史下一次写入的环形物理下标
s_sequence|后台报告事件的累计递增序号
s_legacy_history|参考组 19 的 34 字 RAM 历史环，字 0 保存下一写位置
s_legacy_first|本次会话首次故障前需要插入零分隔符的标志
''')
variables('''
FPGA_Interface|15 个插件的 EMIFA 映射寄存器窗口，索引为原插件编号减 2
Port_MainStatus|当前端口主状态、主/子工作模式的位域缓存
Port_Main_WorkMode|当前端口主工作模式编号
Port_STATCOM_WorkMode|旧 STATCOM 无功/功率因数/电压工作模式编号
Port_APF_WorkMode|旧 APF 谐波补偿工作模式编号
Port_Outer_Elec|端口外部电源接入状态
Port_Sys_Stop|旧系统停机状态
Port_State|旧端口状态编号，迁移核心维护兼容镜像
Port_RunStatus|旧端口运行状态编号
Port_DSP_CTL|端口 FPGA 全关断及硬件复位控制字缓存
Port_DOUT|端口断路器、接触器数字输出控制字缓存
Module_PM_CTL|模块工频桥/DAB 使能、驱动复位及测试控制字缓存
DPRAM_Block3_remMeaSig|原地址映射的端口遥测区；访问受 SST_ENABLE_DPRAM 门控
PORT_RealTime_DATA_buf|端口遥测的 16 位定点打包缓存
PORT_RealTime_DATA|端口遥测的浮点物理量缓存及兼容状态
Port_RealStatus|端口开关及桥状态的原位域缓存
Port_RealFault|端口故障的原位域缓存，某些位在 HVAC 中复用
DPRAM_Block1_remControl|原地址映射的遥控区；访问受 SST_ENABLE_DPRAM 门控
PORT_Control_DATA_Buf|旧遥控协议的本地字缓存
Port_COM_START|旧遥控启动字的位域解码缓存
Port_COM_STOP|旧遥控停止字的位域解码缓存
DPRAM_Block2_remRegulate|原地址映射的遥调区；访问受 SST_ENABLE_DPRAM 门控
PORT_Regulate_DATA_Buf|旧遥调协议的本地定点字缓存
PORT_Regulate_DATA|旧遥调定点值换算后的浮点给定缓存
Port_AD_PORT_EN|端口各通道 AD 保护使能位图
Port_FB_EN|端口 FB 硬件故障输入使能位图
SW|旧开关时序、动作状态及完成标志的位域缓存
FaultTypeFLAG|原故障分类位图，后台故障报告据此映射五位码
PortRunFLAG|旧端口运行、预充、PWM 及相序状态的位域缓存
ControlFLAG|旧启动、停止、PWM、DAB 斜坡流程控制标志
FaultDisposeFLAG|原故障处理策略及 PWM 禁用范围的位域缓存
ModuleIndex_PWMPulseDisable|原故障策略中待禁用 PWM 的模块索引
FaultRecordStatus|FT3 故障录波主运行模式状态字
FaultRecordStatus_1|FT3 故障录波谐波/STATCOM 模式状态字
FT3_PTCT_Register|PT/CT FT3 插件的原映射寄存器窗口
FT3_FaultRecorder_Register|故障录波 FT3 插件的原映射寄存器窗口
FT3_EER2_Register|EER2 FT3 插件的原映射寄存器窗口
FT3_Backup_Register|备用 FT3 插件的原映射寄存器窗口
faultsignal|原 FT3 故障触发信号
faultRecordStart|原 FT3 录波启动标志
fault_count|原故障录波触发计数
fault_count_start|原故障录波开始计数/状态
Flag_PC|旧 PC/本地控制兼容标志
Flag_DC_FAULT|原直流故障总标志
Flag_DC_FAULTDISPOSE|原直流故障处置状态
Flag_DC_FAULTport|原直流故障端口标识
Flag_DC_VoltSource|原直流电压源控制模式标志
Flag_DC_VoltSource_Set|原直流电压源模式设置完成标志
flag_INV|原工频桥开环逆变模式标志
flag_SVGOPEN|原工频桥开环投入标志
flag_DABOPEN|原 DAB 开环投入标志
flag_FaultClose|原故障后脉冲关闭标志
Flag_FaultOff|原故障停机标志
FLAG_RemRegulate_SetOver|原遥调设置完成标志
flag_SendDABRef_First|原 DAB 参考首次下发标志
flag_SendSVGRef_First|原工频桥参考首次下发标志
Flag_testT|原测试定时观察标志
Flag_Key|原本地按键扫描状态
Flag_temp|后台运行指示灯翻转状态
Flag_WDI|后台外部看门狗 GPIO 翻转状态
Flag_VofaSend|保留原 VOFA 发送通知变量；迁移发送由后台时基调度
Flag_Uart|旧 UART 维护帧兼容状态，保留符号供原路径回读
CommNum|旧 UART 兼容通信序号
CommFunc|旧 UART 兼容功能码
StartAddr|旧 UART 兼容请求的起始地址
ReadLength|旧 UART 兼容读取长度
CommLength|旧 UART 兼容帧有效长度
CommInNum|旧 UART 兼容帧当前接收位置
CommTotalNum|旧 UART 兼容帧预期总长度
WriteOK|旧 UART 兼容写入成功标志
PCuartBUFF|旧 PC UART 帧兼容缓存，容量 140 字节
ReceiveuartBUFF|旧 UART 接收帧兼容缓存，容量 11 字节
Flag_ComTest|旧端口通信测试标志
FourPortComZhi|旧四端口通信测试状态字
Flag_Clear_0|原故障锁存清除第一阶段请求标志
Flag_Clear_1|原故障锁存清除第二阶段请求标志
Flag_Clear_zhi|原故障清除期间的兼容状态
Flag_Wait|原控制流程等待标志
Flag_StopWait|原停机延时等待标志
Flag_StopWaitEnd|原停机等待结束标志
Flag_RemoteControlRegulate|DPRAM 通知 ISR 置位、后台消费的遥控/遥调待处理标志
flag_RemoteControlRegulate|原头文件小写兼容声明，实际使用符号为 Flag_RemoteControlRegulate
FLAG_NEW_R|DPRAM 向对端发布的新数据块标识
FLAG_NEW_L|DPRAM 从对端读取的新数据块标识
SWTable_TotalTime|旧开关动作表的总定时时间
WriteBlock_Index|后台 DPRAM 插件遥测轮询的当前块号
Module_DCbus_Volt|当前检查模块的直流母线电压，单位 V
Udc_Ref|旧直流电压控制参考，单位 V
Current_Ref|旧直流/交流电流控制参考，单位 A
Ust_add|旧直流电压给定爬升增量，单位 V
Udc_Check_Index|逐模块直流电压巡检的当前索引
Udc_Checked_Num|已检查模块数量
PLL_Factor|旧 PLL 系数调整因子
Volt_Factor|旧总直流电压环系数调整因子
PH_Volt_Factor|旧相间电压平衡环系数调整因子
Current_Factor|旧正序电流环系数调整因子
Neg_Current_Factor|旧负序电流环系数调整因子
AVolt_Factor|旧交流电压相关调整因子
Port_OnState|原端口已投入状态
TrigeSignal|原调试/录波触发信号
TestINT|原中断调试观察标志
Ref_ph_test|原开环测试相角，单位 rad
Ref_js_test|原调制测试幅值/计算缓存
rtt|DPRAM 收到通知后的插件 LED 翻转状态
DAC_BUF|四路 DA 调试输出缓存，写出时转为 FPGA 原始计数
TEATabc|原三相调试观察缓存，沿用原符号拼写
FT3_Samp_Cnt|FT3 上报的采样序号
dataPackNum|FT3 数据包数量/控制字字段
PID_Control_buf|通用增量 PI 本拍使用的控制器状态缓存
H2_100_Filter|通用 H2/100 滤波计算历史缓存
H6_100_Filter|通用 H6/100 滤波计算历史缓存
H12_100_Filter|通用 H12/100 滤波计算历史缓存
H2_20_Filter|通用 H2/20 滤波计算历史缓存
Phase_PLL|旧 HVAC 锁相环电网相角，单位 rad
Grid_P_Power|旧电网有功功率计算值
Grid_Q_Power|旧电网无功功率计算值
Load_Q_Power|旧负载无功功率计算值
SVG_Q_Power_Ref|旧 SVG 无功功率目标
SVG_Udc_Ave|三相单模块直流电压的总平均值，单位 V
SVG_Udc_Ave_temp|旧直流平均电压噪声处理的本拍缓存，单位 V
SVG_Udc_Ave_temp_last|旧直流平均电压噪声处理的上一拍缓存，单位 V
HA_L_Voltage_Precharge|原 HVAC 预充欠压判断阈值，单位 V
HA_H_Voltage|原 HVAC 直流高压判断阈值，单位 V
Ust_HVAC_Port|旧 HVAC 单模块直流电压运行给定，单位 V
UstOK_HVAC_Port|旧 HVAC 启动电压达标值，单位 V
OverVolt_AlarmLevel2|旧模块二级过压报警阈值，单位 V
VoltLevel_1|旧电压判断第一级阈值
VoltLevel_2|旧电压判断第二级阈值
Port_Abnormal|旧端口异常状态标志
Port_LV2|旧端口第二级欠压状态
Flag_V_L|旧欠压阶段标志
Flag_V_L1_END|旧一级欠压处理完成标志
Flag_V_L2_END|旧二级欠压处理完成标志
Flag_V_L2_MID|旧二级欠压处理中间阶段标志
Flag_SlopeTime|旧电压给定斜坡计时使能
SlopeTime_N|旧电压斜坡的计时长度/计数
Cout_V_L|旧欠压处理计时计数
HA_Module_RefUdc|旧 HVAC 单模块直流电压目标，单位 V
Phase_PLL_First|旧 PLL 首拍相角缓存，单位 rad
PLL_Phase_Com|旧电流变换相位补偿，单位 rad
PLL_Phase_Com_NEW|旧输出变换相位补偿，单位 rad
PhaseCom|旧附加相位补偿量，单位 rad
Ugrid_Max|旧控制使用的电网电压幅值上限/估计值，单位 V
Add_3th|旧三次谐波注入使能标志
Method_3th|旧三次谐波注入算法选择
flag_Standard|旧调制标准化方式选择标志
StandardValue|旧调制参考标准化基值
DAB_ONLY|旧仅运行 DAB 的测试选择标志
HVAC_OC_Fault|原 HVAC 软件过流故障确认标志
HVAC_OV_Fault|原 HVAC 软件过压故障确认标志
OC_OV_LV_PortLatch_TAD|原软件过流/过压/欠压的 TAD 故障锁存值
OC_level_SET|原一级过流阈值，沿用采样计数/系数约定
OC_time_SET|原一级过流确认计数阈值，单位 TAD
OC_level_SET_L2|原二级过流阈值，沿用采样计数/系数约定
OC_time_SET_L2|原二级过流确认计数阈值，单位 TAD
OV_level_SET|原直流过压阈值，沿用采样计数/系数约定
OV_time_SET|原过压确认计数阈值，单位 TAD
Samplenoise|原交流电压采样噪声判断状态
Samplenoise_Idq|原 dq 电流采样噪声判断状态
Samplenoise_udc|原直流电压采样噪声判断状态
GNDSwitch|原接地开关状态
ISOSwitch|原隔离开关状态
Contactor_err|原接触器动作异常标志
Breaker_err|原断路器动作异常标志
ISO_counter_on|原隔离开关闭合确认计数
ISO_counter_off|原隔离开关断开确认计数
Alpha_Trans|通用坐标变换 α 轴输出，与输入电压/电流同单位
Beta_Trans|通用坐标变换 β 轴输出，与输入电压/电流同单位
A_Trans|通用逆变换 A 相输出，与输入量同单位
B_Trans|通用逆变换 B 相输出，与输入量同单位
C_Trans|通用逆变换 C 相输出，与输入量同单位
D_Trans|通用 Park 变换 d 轴输出，与输入量同单位
Q_Trans|通用 Park 变换 q 轴输出，与输入量同单位
''')
variables('''
bytes|发送帧槽的完整字节缓存
budget|本次服务允许处理的最大字节数，避免后台单轮无界阻塞
status|维护协议固定状态字响应缓存
source|读取对象的 16 位源地址
target|写对象的 16 位目标地址
payload|帧载荷字节缓存/指针，不含帧头和 CRC
response|维护请求处理生成的响应载荷缓存
command|维护请求/响应命令码
object|维护协议对象类型编号
outLength|维护响应有效载荷长度，单位字节
payloadLength|维护帧头声明的载荷长度，单位字节
next|环形接收队列下一写入下标
pos|持久化镜像参数打包/解包的位置
combined|页 4/5 合并后的维护参数镜像
data5|EEPROM 第 5 页读取缓存
byteValue|本次持久化单字节写入值
clearIndex|旧故障清除命令的兼容索引
''')

# Meanings specific to a function, for short temporary names that carry no units.
LOCALS = {}
def locals_for(owner, rows):
    for row in rows.strip().splitlines():
        name, meaning = row.split('|',1)
        LOCALS[(owner,name)] = meaning
locals_for('Clarke_Transformation', 'TempA|α 轴差值、缩放计算缓存\nTempB|β 轴相间差值及除 √3 的计算缓存')
locals_for('Clarke_InverseTransformation', 'Tempa|A 相逆变换计算缓存\nTempb|B 相逆变换计算缓存\nTempc|C 相逆变换计算缓存')
for owner in ('Park_Transformation','Park_InverseTransformation'):
    for j, meaning in enumerate(('第一轴分量乘 cos 的乘积','第二轴分量乘 sin 的乘积','第二轴分量乘 cos 的乘积','第一轴分量乘 sin 的乘积'),1):
        LOCALS[(owner,'Temp'+str(j))]=meaning+'，用于本拍坐标变换'
locals_for('ConstantPID_Controller', 'tempjs_0|当前参与乘法的 PI 差分系数\ntempjs_1|本拍误差与 Num_1 的乘积\ntempjs_2|上一拍误差与 Num_0 的乘积\ntempjs_3|上一拍输出加两个误差项得到的本拍 PI 输出')
for owner in ('BandH2_100_Filter','BandH6_100_Filter','BandH12_100_Filter','BandH2_20_Filter','SineHar_2_Controller','SineHar_6_Controller','SineHar_12_Controller'):
    for j in range(5):
        LOCALS[(owner,'tempjs_'+str(j))]=('差分式当前系数/累计计算项','本拍输入项的乘法缓存','前一拍输入/输出项的乘法缓存','历史输入/输出项与当前输出的累计缓存','第二阶历史项的乘法缓存')[j]
locals_for('SstBoard_GetTick1ms', 'previous|上次输出的单调毫秒值，跨主循环调用保留\ntick|采集的控制核心 10 ms 节拍\ncheck|二次读取的 10 ms 节拍，用于检查 Timer2 回绕附近的一致性\nfraction|Timer2 当前周期内的毫秒分量\nresult|本次组合得到的单调毫秒结果')
locals_for('Chb_Min3', 'result|已比较输入中的当前最小值')
locals_for('Chb_Max3', 'result|已比较输入中的当前最大值')
locals_for('Chb_WriteDabDuty', 'start|限幅后的 DAB 脉冲起始计数')
locals_for('ReadEntry', 'value|物理成员读取值及缩放后的浮点线上值')
locals_for('StoreLoad', 'n|持久化的五个命令给定所对应的 commands 下标\nword|镜像解码出的参数线上字\nresult|已整体校验参数的写入结果\np|本次校验的参数映射条目')
locals_for('StoreRequest', 'n|需要保存的命令给定所对应的 commands 下标')
locals_for('SstParams_Write', 'n|组号与索引换算后的 parameters 表下标')
locals_for('SstParams_Service', 'value|本拍写入 EEPROM 的单字节值\nok|EEPROM 单字节写入的 VALIDFLAG/INVALIDFLAG 结果\noffset|本拍写入或读回的镜像字节偏移')
locals_for('Scale', 'v|物理量乘比例后的浮点线上值')
locals_for('C_COMM2PC::RxByte', 'data|把收到的字节帧扩展为参考解析接口的 16 位字数组\nlength|校验通过帧的完整字节数')
locals_for('C_COMM2PC::ParseInstruct', 'n|去掉帧头、长度、命令和 CRC 后的载荷字节数\nminimum|按通道数和 UART 波特率计算的最小波形发送间隔，单位 ms')
locals_for('C_COMM2PC::ExecuteInstruct', 'data|参考应答、波形、面板及心跳的临时字节载荷\nword|读取参数或显示表得到的 16 位字\nresult|FuncCode 参数访问结果，成功为 1、失败为负结果码')
locals_for('VOFA_SendFloatArray', 'word|单通道浮点值与四字节表示的联合缓存')
locals_for('CRC16', 'l|CRC 当前位循环序号\nch|当前参与 CRC 的输入字节\nswap|原 CRC 高低字节交换的中间值')
locals_for('UART2Isr', 'rxData|原 UART ISR 读取的接收字符\nint_id|原 UART 中断源标识\nrxtemp|原 UART 接收处理临时字符')
locals_for('CRC16', 'ba|CRC 输入缓冲区首地址，调用者提供有效帧字节')
variables('''
OC_level_SET_DEFAULT|原一级过流保护默认阈值
OC_time_SET_DEFAULT|原一级过流确认默认 TAD 计数
OC_level_SET_L2_DEFAULT|原二级过流保护默认阈值
OC_time_SET_L2_DEFAULT|原二级过流确认默认 TAD 计数
OV_level_SET_DEFAULT|原过压保护默认阈值
OV_time_SET_DEFAULT|原过压确认默认 TAD 计数
''')
functions('''
Module_AD_Sample|读取指定模块的 FPGA AD 数据并按所选量转换为工程值|所选模块采样物理值
HVAC_RealTimeParameter|换算 HVAC 插件采样数据并更新端口实时测量缓存
Port_AD_Sample|采集当前端口 FPGA 的瞬时电压、电流及直流量，并推进原滤波/噪声处理
Port_RealTimeParameter|换算当前端口锁存/滤波采样，更新有效值、功率及公共遥测
Module_TotalUdc_Sample|读取模块直流采样汇总并更新各相总电压量
''', '原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。', 'HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。')
variables('''
by|CRC 计算缓存的字节视图，供高低字节访问
crc16|CRC 计算缓存的 16 位整字视图
Bbits|与整字重叠的位域视图，字段位置对应原硬件/协议布局
Bits|与整字重叠的位域视图，字段位置对应原录波协议布局
FT3_Rec_Data|故障录波 FT3 接收的八字原始数据缓存
temp_rec|FT3 接收通信测试的原临时观察值
tr1|原 FPGA 通信/调制测试观察量 1
tr2|原 FPGA 通信/调制测试观察量 2
tr3|原 FPGA 通信/调制测试观察量 3
UartDebugBuf|原 UART 调试发送缓存
UartDebugIndex|原 UART 调试缓存当前位置
Port_AD_Buf|原端口 FPGA AD 采样字缓存
PortPriority_Start|原四端口启动优先级表
PortPriority_Stop|原四端口停止优先级表
Port_StartPriorityPosition|当前端口在原启动优先级表中的位置
Port_StopPriorityPosition|当前端口在原停止优先级表中的位置
FOLatch_A_TAD|A 相插件光纤故障的 TAD 锁存值
FOLatch_B_TAD|B 相插件光纤故障的 TAD 锁存值
FOLatch_C_TAD|C 相插件光纤故障的 TAD 锁存值
FOLatch_INV_TAD|独立逆变桥光纤故障的 TAD 锁存值
PortLatch_TAD|端口 AD 保护故障的 TAD 锁存值
FBLatch_TAD|端口 FB 硬件故障的 TAD 锁存值
ModuleFault_DisposeTimes|原模块故障处置已执行次数
ClearTimes|原故障清除过程的执行次数
tx|共享整帧发送队列，后台独占维护
txHead|发送队列下一写入帧槽索引
txTail|发送队列下一发送帧槽索引
txCount|发送队列已占用的整帧槽数量
txOffset|当前队尾帧已经发送的字节数
txDropped|因帧非法或队列满累计丢弃的帧数
txWaiting|RS485 最后一个字节发送完成前的方向保持标志
rxTick|最近一次后台 UART 接收的毫秒节拍
muxState|AnyEyes接收状态，0等帧头、2等长度、3接收正文
muxIndex|当前分流帧已接收字节数
muxExpected|AnyEyes当前帧剩余正文及校验字节数
lastVofaTick|最近提交 VOFA 波形帧的 10 ms 节拍
savePinmux|引脚复用寄存器的读改写缓存，保留非目标引脚字段
savePinMux|EMIFA 引脚复用寄存器的读改写缓存
svPinMuxRtsCts|UART 流控引脚复用寄存器的读改写缓存
svPinMuxTxdRxd|UART 收发引脚复用寄存器的读改写缓存
u16ByteAdd|EEPROM 绝对字节地址，由页号乘 64 加页内偏移得到
u8HighAdd|EEPROM 绝对字节地址的高 8 位
u8LowAdd|EEPROM 绝对字节地址的低 8 位
u8Retry|软件 I²C 访问重试序号，最多三次
u8Result|软件 I²C 控制写入的累计成功/失败标志
u8RecByte|EEPROM 读取的一个原始字节
u8RcvByte|软件 I²C 逐位组装的接收字节
Temp_error|EEPROM 段写入的应答失败累计状态
DataTemp|旧 EEPROM 页的原始字节打包/解包联合缓存
EE_TEMP|旧 EEPROM 浮点参数的两字联合换算缓存
InitialTemp|原模块配置/控制字初始化的整字计算缓存
InitialTemp_2|原模块配置初始化的附加整字计算缓存
InitialTempFloat|原控制系数和给定初始化的浮点计算缓存
CRC_Temp_1|旧维护帧 CRC 校验的第一缓存字
CRC_Temp_2|旧维护帧 CRC 校验的第二缓存字
Addr_write_temp|旧维护帧写操作的临时地址
SM_write_count|旧维护存储写入的累计计数
SM_write_flag|旧维护存储写入状态标志
WriteD|旧维护写入数据/状态的兼容变量
i_temp|旧初始化/维护流程临时整字计算值
ADProtect_Temp|本次读取的端口 AD 保护锁存位图
FBLatch_Temp|本次读取的端口 FB 故障锁存位图
AD_ProtectStatus_Temp|读取并选择后的端口 AD 保护原始状态字
ModuleStatus_Temp|读取的模块/插件原始状态字
DIN_Temp|读取的端口数字输入原始位图
FBStatus_Temp|读取的端口 FB 原始故障位图
FOStatus_Temp|读取的插件 FO 原始故障位图
INVFOStatus_Temp|读取的独立逆变桥 FO 原始故障位图
DCVolt_INTTemp|故障处置期间读取的模块直流电压原始值
ModuleStatus_INTTemp|故障处置本次读取的模块状态字
ModuleStatus_INTTemp_1|模块状态字第一步掩码解码缓存
ModuleStatus_INTTemp_2|模块状态字第二步掩码解码缓存
ModuleStatus_INTTemp_3|模块状态字第三步掩码解码缓存
tt_INT|原模块故障处置的临时索引/计数
indextemp|PWM 禁用策略选出的模块/相索引
ParameterTemp|原遥调/遥测的定点与浮点转换缓存
tt|DPRAM 写入通知的原临时控制值
tt_flag|原遥控处理的临时状态字
tt_allstop|原全端口停机流程的临时状态字
tt_stop|原停止流程的临时状态字
PriorityNum_Start|当前查找的启动优先级编号
PriorityTemp|原端口启动优先级表的临时条目
PriorityPositionTemp|当前端口在启动优先级表中的临时位置
PortAndSwitch_Status|建立开关动作表时的端口与开关综合状态
Switch_StatusTemp|原开关状态位图的临时解码值
index_temp|本次模块电压检查使用的模块/相索引
Udc_Temp|本次巡检读取的模块直流电压，单位 V
SampleBuf_short|从 FPGA 读取的有符号原始采样字
SampleBuf_short_1|第一个 FPGA 原始采样/汇总字缓存
SampleBuf_short_2|第二个 FPGA 原始采样/汇总字缓存
ADSample_Result|模块原始 AD 值换算后的物理量
ADSample_buffer|本轮读取的 FPGA 原始采样字缓存
AD_CalTemp|采样比例换算、滤波或有效值计算的浮点中间值
AD_CalTemp1|采样/功率换算的第二个浮点中间值
AD_CalTemp2|采样/功率换算的第三个浮点中间值
LPF_temp0|原低通滤波器的前一项/输入项计算缓存
LPF_temp1|原低通滤波器的本拍输出项计算缓存
PluginUnit_ONNum|参与本次平均值计算的在线模块数量
tempAve1|第一相/第一组模块采样总和与平均值缓存
tempAve2|第二相/第二组模块采样总和与平均值缓存
tempAve3|第三相模块采样总和与平均值缓存
tempAve_wonoise|按原噪声规则处理后的模块平均采样值
ControlParaTemp|原工频桥参考换算的浮点中间值
ControlParaTemp_1|参考归一化使用的模块直流电压/基准缓存
ControlParaTemp_2|三相工频桥参考缩放与移位的中间值
ReferenceTemp|原 FPGA 调制参考换算缓存
tempDAC|单通道 DA 输出缩放、限幅后的计数缓存
DC_vol|软件电压保护读取的直流采样/电压值
grid_current|软件过流保护读取的电流绝对值
grid_current_A|A 相软件过流保护读取的电流绝对值
grid_current_B|B 相软件过流保护读取的电流绝对值
grid_current_C|C 相软件过流保护读取的电流绝对值
temp_set1|原一级过流阈值的本轮计算缓存
temp_set2|原一级过流确认时间的本轮计算缓存
temp_set3|原二级过流阈值的本轮计算缓存
temp_set4|原二级过流确认时间的本轮计算缓存
temp_set5|原过压阈值的本轮计算缓存
temp_set6|原过压确认时间的本轮计算缓存
temp_set7|原欠压阈值的本轮计算缓存
temp_set8|原欠压确认时间的本轮计算缓存
hartemp|旧负载电流谐波分离后的分量缓存
VoltErrorTemp|旧直流电压外环给定减反馈的误差，单位 V
CurrentErrorTemp|旧 dq 电流环给定减反馈的误差，单位 A
CurrentOut1|旧电流控制输出/第一相计算缓存
CurrentOut2|旧电流控制输出/第二相计算缓存
CurrentOut3|旧电流控制输出/第三相计算缓存
tempspwm|旧三相 SPWM 给定计算的中间值
tempspwm_min|旧 SPWM 三相给定中的最小值，用于零序/三次谐波处理
x|按通道顺序排列的本次 VOFA 浮点发送数组
''')
variables('''
Breaker2_OuterOn|原第二外部断路器/故障电流控制器合闸控制位
BUCK_EN|BUCK PWM 使能位，沿用原 FPGA 有效电平
Label|原状态/驱动字中的标识字段，保持 FPGA 返回格式
PORT_STATUS|端口开关及桥状态的原 16 位位域字
PORT_FAULT|端口故障的原 16 位位域字，位定义随旧端口分支解释
PORT_P|端口有功功率；float 为物理值，16 位版本为旧遥测缩放编码
PORT_Q|端口无功功率；float 为物理值，16 位版本为旧遥测缩放编码
PORT_PF|端口功率因数；16 位版使用旧协议缩放格式
DC_V|端口直流电压；float 为 V，16 位版使用旧协议缩放格式
PORT_V0|旧下垂控制的电压基准量，编码比例见 RemoteSig_Dispose
PORT_F0|旧下垂控制的频率基准量，编码比例见 RemoteSig_Dispose
PORT_KF|旧频率下垂系数，编码比例见 RemoteSig_Dispose
PORT_KV|旧电压下垂系数，编码比例见 RemoteSig_Dispose
PORT_P_SET|旧遥调有功功率给定；定点与浮点转换见 RemoteRegulate_Dispose
PORT_Q_SET|旧遥调无功功率给定；定点与浮点转换见 RemoteRegulate_Dispose
PORT_V_SET|旧遥调电压给定；float 为 V，定点编码见 RemoteRegulate_Dispose
PORT_F_SET|旧遥调频率给定；float 为 Hz，定点编码见 RemoteRegulate_Dispose
STATCOM_V_SET|旧 STATCOM 电压控制给定；编码见 RemoteRegulate_Dispose
STATCOM_PF_SET|旧 STATCOM 功率因数给定；编码见 RemoteRegulate_Dispose
STATCOM_Q_SET|旧 STATCOM 无功功率给定；编码见 RemoteRegulate_Dispose
PORT_V0_SET|旧下垂控制电压基准给定；编码见 RemoteRegulate_Dispose
PORT_F0_SET|旧下垂控制频率基准给定；编码见 RemoteRegulate_Dispose
PORT_KF_SET|旧频率下垂系数给定；编码见 RemoteRegulate_Dispose
PORT_KV_SET|旧电压下垂系数给定；编码见 RemoteRegulate_Dispose
EER2_DATA|旧 EER2 兼容数据字段，按原 16 位字数组布局解释
S_EXT_CB_ON_EN|旧开关流程的外部断路器合闸允许条件
S_EXT_CB|旧开关流程的外部断路器目标/判定状态
S_INT_CB|旧开关流程的内部断路器目标/判定状态
S_CONTACTOR|旧开关流程的接触器目标/判定状态
EXT_CB_ON_EN|外部断路器合闸允许标志
Flag_SwitchZhi|旧开关动作时序状态值
Flag_SwitchSuccess|旧开关动作成功标志
Flag_SwitchRun|旧开关动作正在执行标志
Port_OverVolt|原端口过压故障分类位
Port_OverCurrent|原端口过流故障分类位
Module_DAB_H|原模块 DAB 桥故障分类位
Module_50Hz_H|原模块工频桥故障分类位
Module_Single|原单模块故障分类位
Module_OverVolt|原模块过压故障分类位
Module_OverCurrent|原模块过流故障分类位
Module_OverTemp|原模块过温故障分类位
Contactor|原接触器动作故障分类位
Breaker|原断路器动作故障分类位
Controller|原控制器故障分类位
Sync|原同步故障分类位
Communication|原通信故障分类位
Module_Fault|原模块汇总故障分类位
Module_LowerVolt|原模块欠压故障分类位
Module_OverVoltLevel2|原模块二级过压故障分类位
Port_SigUdc|旧端口直流电压单独检查/信号标志
Port_UdcOK|旧端口模块直流电压预充达标标志
Port_AutoOrManual|旧端口自动/手动运行选择字段
Port_RunOrTest|旧端口正常运行/测试选择字段
PWMState_50Hz|旧工频桥实际 PWM 状态镜像
PWMState_Dab|旧 DAB 实际 PWM 状态镜像
PWMState_Module|旧模块桥 PWM 汇总状态镜像
Stop_AllPort|旧全端口停机请求标志
PhaseSquence_Error|旧相序错误标志，保留原字段拼写
Port_HighUdc|旧端口模块直流电压过高标志
SVG_PWMEN|旧工频桥 PWM 使能命令标志
DAB_PWMEN|旧 DAB PWM 使能命令标志
Port_Start|旧端口启动流程请求
Port_Stop|旧端口停止流程请求
Port_FaultDispose|旧端口故障处置待执行标志
DAB_DutyStart|旧 DAB 占空比启动斜坡进行标志
SVG_Start|旧工频桥启动流程进行标志
Port_StartValid|旧端口启动条件检查通过标志
DAB_DutyOver|旧 DAB 占空比启动斜坡完成标志
SVG_StartOver|旧工频桥启动流程完成标志
DAB_StartMethod|旧 DAB 启动策略字段
FaultDisposeType|原故障处置策略类型字段
PWMPulseState|原故障处置中的 PWM 状态字段
PWMPulseDisableNum|原策略需要禁用 PWM 的模块数/编号字段
PWMEN_FibreOptical|原光纤 PWM 使能控制字段
Port_StopType|旧端口停机类型字段
Port_StopAll|旧故障策略要求全端口停机的标志
FT3_SendCTL|FT3 发送启动/控制寄存器
FT3_ReadCTL|FT3 接收读取/控制寄存器
StatusNum_ready|FT3 录波的准备状态位
StatusNum_Run|FT3 录波的运行状态位
StatusNum_Stop|FT3 录波的停止状态位
STATCOM_Q|FT3 录波的无功控制模式标志
STATCOM_PF|FT3 录波的功率因数控制模式标志
STATCOM_V|FT3 录波的电压控制模式标志
Sys_Stop|FT3 录波/端口主状态的系统停机位
Outer_Elec|FT3 录波/端口主状态的外部电源接入位
voltage_mode|FT3 录波的电压源模式标志
power_mode|FT3 录波的功率控制模式标志
droop_mode|FT3 录波的下垂控制模式标志
gridfollow_mode|FT3 录波的跟网控制模式标志
no_harmonic|FT3 录波的无谐波补偿模式标志
harmonic_inject|FT3 录波的谐波注入模式标志
harmonic_limit|FT3 录波的谐波限制模式标志
no_STATCOM|FT3 录波的无 STATCOM 控制模式标志
''')

# Corrections verified against the actual retained compatibility bodies.
FUNCTIONS['VOFA_Init']['purpose']='保留旧 VOFA 初始化入口；串口与发送时基由 SstDebug_Init 统一初始化，本函数为空'
FUNCTIONS['VOFA_Process']['purpose']='保留旧 VOFA 周期入口；实际四通道发送由 SstDebug_Service 调度，本函数为空'
FUNCTIONS['VOFA_SendFloatArray']['purpose']='按小端 IEEE754 组成最多十二通道的 JustFloat 帧，追加 00 00 80 7F 帧尾并提交队列'
FUNCTIONS['VOFA_SendFloatArray']['result']='无；队列入队结果沿用原接口直接忽略'
FUNCTIONS['SstDebug_Tick10ms']['purpose']='保留 10 ms 调试服务兼容入口，本函数为空；时间基准直接读取控制状态'
FUNCTIONS['SstDebug_Service']['purpose']='以固定 RX/TX 字节预算轮询 UART 并调度所选协议与参数保存服务'
VARIABLES['rxTick']='最近接收字节的 10 ms 节拍，用于后台分流超时判断'
LOCALS[('VOFA_SendFloatArray','word')]='通过 memcpy 得到的单通道 IEEE754 位表示，用于按小端逐字节打包'
for owner in ('BandH2_100_Filter','BandH6_100_Filter','BandH12_100_Filter','BandH2_20_Filter'):
    locals_for(owner,'tempjs_0|差分式当前系数，最后用于输出增益\ntempjs_1|本拍及前两拍输入项的累计值\ntempjs_2|输入缓存及本拍滤波输出计算值\ntempjs_3|前一拍输出乘第一反馈系数的缓存\ntempjs_4|前两拍输出乘第二反馈系数的缓存')

# Output contracts: getters/calculation helpers do not mutate all module state.
for key in ('Chb_Clamp','Chb_Abs','Chb_Min3','Chb_Max3','Chb_WrapAngle','Chb_ConfigurationValid',
            'SstAnyEyes_Crc8','ReadEntry','Scale','SstControl_GetTick10ms',
            'SstParams_IsStorageBusy','SstParams_IsStopped','SstTelemetry_ReadFloat',
            'SstDisplay_Read','StoreCrc','StoreGet','SstParams_GetStorageResult',
            'SstDebug_GetTick10ms','SstDebug_GetTick1ms','SstDebug_GetTxDropped',
            'SstSerial_TxFreeFrames','CRC16','BCD2HEX','HEX2BCD',
            'SstFault_GetCode','SstFault_GetFirstCode','SstFault_GetLevel','SstFault_GetActiveCount',
            'SstFault_GetHistoryCount','SstFault_GetLegacyHistoryWord','SstFault_HistoryIndex',
            'SstFault_GetHistoryCode','SstFault_GetHistoryLevel'):
    FUNCTIONS[key]['output']='无指针输出；通过返回值提供计算结果或状态。'
for key, contract in dict(
    Clarke_Transformation='Alpha_Trans、Beta_Trans，单位与三相输入相同。',
    Clarke_InverseTransformation='A_Trans、B_Trans、C_Trans，单位与 αβ 输入相同。',
    Park_Transformation='D_Trans、Q_Trans，单位与 αβ 输入相同。',
    Park_InverseTransformation='Alpha_Trans、Beta_Trans，单位与 dq 输入相同。',
    ConstantPID_Controller='更新 PID_Control_buf 中的 IN_n_1、OUT_n_1。',
    H_Filter_Memcopy='DestAddr 指向的连续四个 float，调用者保证源/目标容量。',
    SstParams_Read='value 指向的 16 位线上编码；空指针返回地址错误。',
    GetOneValueFromRealMem='dataList 指向的一个 16 位参数值。',
    WriteEntry='仅更新 p->address 指定的受控成员。',
    SstFault_ReadInput='input 指向的后台故障输入；不保证各标量为同一硬件瞬间。',
    SstFault_GetHistory='record 指向的一条历史记录，newest_index=0 读取最新。',
    StorePut='storageImage 中 offset、offset+1 两字节。',
    Chb_SourceEquivalentPiStep='更新指定 PI 状态的 integral，返回本拍限幅输出。',
    VOFA_Init='无。', VOFA_Process='无。', SstDebug_Tick10ms='无。'
).items(): FUNCTIONS[key]['output']=contract
for key in ('MemDataCopy','MemDataCopy_1','MemDataCopy_2'):
    FUNCTIONS[key]['output']='DestAddr 指向的连续 length 个 16 位字，调用者保证容量及有效地址。'
for key in ('Module_StatusRead','Module_TotalStatusRead','Port_DINRead','Port_AD_ProtectStatusRead',
            'Port_FOStatusRead','Port_FBStatusRead','Port_INVFOStatusRead','Module_AD_Sample'):
    FUNCTIONS[key]['output']='无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。'
FUNCTIONS['Module_AD_Sample']['result']='直流电压 V、电流 A、温度按原计数/10；非法插件、模块或类型返回 65536.0'
FUNCTIONS['r_eeprom_section_read']['output']='u8Data 指向的 length 个接收字节。'
FUNCTIONS['r_eeprom_page_read']['output']='pu8RecByte 指向的 64 字节页缓存。'
FUNCTIONS['r_time_read']['output']='time 指向的原 RTC 时间寄存器字段缓存。'
FUNCTIONS['SstBoard_GetTick1ms']['output']='无指针输出；保存上一输出值以维持单调性。'
FUNCTIONS['Modualation_Test']['output']='无；当前函数为空。'
FUNCTIONS['SstFault_GetHistory']['context']='后台读取 RAM 历史，不访问硬件、不关中断；输出为后台观察记录。'
FUNCTIONS['Chb_SourceEquivalentPiStep']['purpose']='按原增量 PI 等效的位置式更新积分项并计算本拍输出；限幅由调用者执行'
FUNCTIONS['Chb_SourceEquivalentPiStep']['result']='kp*error+integral 的本拍 PI 输出，本函数内不额外限幅'
FUNCTIONS['Chb_SourceEquivalentPiStep']['context']='TAD 快速控制子函数；外环在调用处限幅 Id，内环通过相电压限幅，系数符号保持原方向。'
FUNCTIONS['main']['result']='声明为 int；初始化完成后进入永久主循环，正常运行不返回'
FUNCTIONS['Judge_MainPowerSupply']['purpose']='检查原四端口状态字是否存在主电源模式且外部有电的端口'
FUNCTIONS['Judge_MainPowerSupply']['result']='存在主电源且有电返回 1，否则返回 0'
FUNCTIONS['Judge_MainPowerSupply']['output']='无；只检查 MainStatus_HVAC/HVDC/LVDC/LVAC 的模式和外部电源位。'
VARIABLES['integral']='PI 积分状态，按 Ki*Ts*error 累加；输出及积分限幅由对应调用者执行'
FUNCTIONS['GetOneValueFromRealMem']['output']='value 指向的一个 16 位参数值。'
FUNCTIONS['SetOneValue2BothMem']['output']='更新固定映射地址对应的受控成员；保存须单独发出请求。'
for key,contract in dict(
    Chb_UpdatePll='s_pll_integral、PLL 相角/频率以及锁定计数、状态镜像。',
    Chb_ResetControllers='s_voltage_pi、s_id_pi、s_iq_pi 及电流给定历史状态。',
    Chb_ResetStartupSequence='内部启动阶段、拍计数、给定及对应状态镜像。',
    Chb_AlignVoltagePiToIdReference='s_voltage_pi.integral，按当前 Id 给定及比例项预置。',
    Chb_UpdateVoltageReference='内部启动电压给定及 startup_voltage_reference_v 状态。',
    Chb_WriteNormalizedReferences='HA_Module_RefVolt_A/B/C、原调制给定寄存器及 modulation_actual。',
    Chb_WriteNeutralReferences='原调制给定寄存器的中性参考及 modulation_actual。',
    Chb_WriteDabDuty='HA_DABDuty_*、HA_DAB_PluseStart_*、FPGA DAB 计数及 dab_duty_count。',
    Chb_SetAcInput='Port_DOUT 断路器控制脉冲、结束时刻以及 ac_input_closed。',
    Chb_ClearAcInputPulse='Port_DOUT 断路器合/分闸脉冲及对应数字输出寄存器。',
    Chb_SetPrechargeBypass='Port_DOUT 接触器控制位及 precharge_bypassed。',
    Chb_DisableAllPwm='原模块 PWM 控制寄存器、ac_pwm_enabled 和 dab_pwm_enabled。',
    Chb_SetLegacyStatus='Port_State、Port_RunStatus 及旧端口主状态/运行位域。',
    Chb_EnterState='g_chb_dab_status.state 及 s_state_enter_tick。',
    ChbDabControl_Tick10ms='g_chb_dab_status.tick_10ms。',
    ChbDabControl_ApplyRemoteCommand='g_chb_dab_command.run_enable、dab_enable；只提交命令。',
    SstCommand_ApplyDpram='g_chb_dab_command 的启停、DAB 使能及故障复位字段；只提交命令。',
    SstParams_RunCommand='g_chb_dab_command.run_enable、dab_enable；PWM 由控制核心执行。',
    SstParams_EmergencyStop='run_enable、dab_enable 清零；原保护/控制路径完成关断。',
    Chb_RunOpenLoop='开环相角、三相调制给定及运行调制幅值状态。',
    Chb_GetClosedLoopIdReference='Id 给定、启动阶段/拍数、外环接管标志及电压 PI 历史状态。',
    Chb_RunClosedLoop='dq 电流反馈/PI 历史、三相调制参考以及对应控制状态。',
    Chb_CheckSoftwareProtection='连续超限计数；满足条件时经 ForceFault 进入故障并关闭 PWM。',
    SstSerial_SendFrame='共享 tx 队列的一个帧槽及队列头/占用数，失败更新 txDropped。',
    Pack='共享整帧 TX 队列；函数不等待 UART FIFO。',
    RxByte='接收帧状态；完整有效帧更新参考工作标志并执行命令。',
    Init='PcDebug 工作标志、接收计数及各个毫秒时基。',
    MainFSM='连接及接收超时状态，并调用 ExecuteInstruct 推进响应/上报。'
).items(): FUNCTIONS[key]['output']=contract
for owner in ('C_COMM2PC::RxByte','C_COMM2PC::ExecuteInstruct','C_COMM2PC::MainFSM'):
    LOCALS[(owner,'now')]='本轮读取的单调毫秒节拍，用于接收/连接超时或周期上报'
for owner in ('SstSerial_RouteByte','SstDebug_Service','SstParams_Service'):
    LOCALS[(owner,'now')]='本轮读取的 10 ms 控制节拍，用于后台超时或逐拍服务'
