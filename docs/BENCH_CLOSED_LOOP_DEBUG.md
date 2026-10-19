# 单UNIT、无主回路的闭环输入模拟与CCS操作指南

> 2026-10-19 更新：下文的 300→680 V 和默认模式描述保留为历史高压流程。当前源码默认是 100 Vac 单相试验，输入发生器仍默认 300 V/module、325 V peak，不能直接启动。低压注入需先设 `dc_a_v=19`、`grid_peak_v=141.42136`、`current_peak_a=0`，预充时手动升至 23～25 V，运行后逐步升至 35 V；具体按 [PI_REFACTOR_AND_LOW_VOLTAGE.md](PI_REFACTOR_AND_LOW_VOLTAGE.md) 第 5 节。复现本文原高压三相流程须显式 LOW=0、PHASE=0，并按实际硬件核对三相模块位图。

2026-10-14握手完善后的工作区保留用户手动设置`SST_BENCH_INPUT_SIM=1`，最新默认AnyEyes测试固件位于`build/Debug/anyeyes_identity/SST_AnyEyes.out`，电子标签见[ANYEYES_HANDSHAKE.md](ANYEYES_HANDSHAKE.md)。下述默认关闭说明及`bench_inputs`产物对应此前固定标签`bench-input-simulation-2026-10-14`；当前测试输入字段和启动操作仍相同。恢复真实采样应明确设置测试宏为0并重新构建。

核对日期：2026-10-14。适用工程：`D:/Code/SST/AnyEyesVersion`。本次已经实现独立的测试输入模块，不需要照旧建议向控制核心或ISR粘贴代码。本次实现前的用户工作区保存在提交`6675a72`及标签`bench-input-baseline-2026-10-14`；构建和软件回归的最终结果以当次交付验证记录为准，本文的操作预期不代表已经完成板级测试。

## 1. 测试范围与实际实现

输入发生器位于`Source/04_App/02_Measurement/SstBenchInputs.c/.h`。它生成独立相角的平衡三相相电压、线电压、相电流、网侧电流，以及CCS可独立修改的三相模块均压。输入相角不使用控制PLL输出，真实PLL仍需自行锁定。模块不包含电感、电容、负载、功率级或PWM到母线电压的响应模型。

测试宏开启时，`Port_AD_Sample(0)`在原HVAC采样入口调用`SstBenchInputs_FastStep()`后返回，注入发生在`HVAC_RealTimeParameter()`的RMS平方和累积之前。因此现代控制输入、旧200点RMS电流和旧软件过压检查使用同一份测试输入。`SVG_Udc_Average()`不再以硬件采样覆盖注入均压；`Udc_check()`不执行真实48位置模块电压巡检。上述替代仅在测试宏下生效，`Port_AD_Sample(1)`锁存采样仍保留原硬件读取语义。

`SystemInit_Runtime()`在控制初始化后、开放外部中断前调用`SstBenchInputs_Init()`，恢复测试默认量及相角。测试模块不提交启动、不改`ac_mode`或`startup_strategy`，不改变PM位图或逻辑模块数。`FPGAIsr`正文及GPIO69/TAD、Timer2注册配置保持原样；测试分支改变了采样被调用函数的计算量，不能据此沿用普通固件的ISR时序结论。

真实PLL、控制状态机、PI、软件过流/过压、FO/FB及GPIO142硬件故障链仍运行。当前默认`g_chb_dab_protection_enable=1`，下述零电流、300～680V流程不需要关闭软件保护。用本方法可以检查自然预充、旁路、锁相、Id斜坡、电压环接管、停止及故障流程；不能证明闭环动态稳定性、充电能力或实际保护延迟。

只在主回路、功率驱动及实际开关执行机构未接入的台架上进行。闭环状态入口仍会下发真实进线、旁路和PWM使能命令。DSP断点不会保证外部FPGA停止，FPGA可能继续输出最后下发的PWM；`ac_pwm_enabled=1`是软件状态镜像，不能替代实际输出检查。

## 2. 选择测试固件并确认加载正确

`SST_SIMULATOR_MODE=1`只决定仿真器命令权限，本身没有模拟电压、电流或FPGA。必须在测试构建中启用`SST_BENCH_INPUT_SIM=1`。正常头文件默认该宏为0，配置互锁禁止测试注入与DPRAM远控或EEPROM上电加载同时开启。

在CCS选择工程`AnyEyesVersion`的Debug配置，进入`Properties → Build → C6000 Compiler → Advanced Options → Predefined Symbols`，加入以下定义；各CCS安装的菜单层级可能略有不同，以编译器预定义符号页为准：

```text
SST_BENCH_INPUT_SIM=1
SST_SIMULATOR_MODE=1
SST_ENABLE_DPRAM=0
CHB_LOAD_EEPROM_PARAMETERS=0
SST_DEBUG_PROTOCOL=1
```

最后一项1为AnyEyes，改成2为VOFA。485仍只使用二者之一，没有A5 5A维护协议。修改预定义后执行Clean再Build；Debug便于查看局部量和使用源码断点。测试宏是编译选择，不能在Expressions中写入。

命令行可以生成同名独立变体，保持CCS工程自己的Debug/Release产物与CLI产物分开：

```powershell
python tools/build.py --configuration Debug --variant bench_inputs --define SST_BENCH_INPUT_SIM=1 --define SST_SIMULATOR_MODE=1 --define SST_ENABLE_DPRAM=0 --define CHB_LOAD_EEPROM_PARAMETERS=0
python tools/build.py --configuration Release --variant bench_inputs --define SST_BENCH_INPUT_SIM=1 --define SST_SIMULATOR_MODE=1 --define SST_ENABLE_DPRAM=0 --define CHB_LOAD_EEPROM_PARAMETERS=0
python tools/build.py --configuration Debug --variant bench_inputs_vofa --define SST_BENCH_INPUT_SIM=1 --define SST_SIMULATOR_MODE=1 --define SST_ENABLE_DPRAM=0 --define CHB_LOAD_EEPROM_PARAMETERS=0 --define SST_DEBUG_PROTOCOL=2
python tools/build.py --configuration Release --variant bench_inputs_vofa --define SST_BENCH_INPUT_SIM=1 --define SST_SIMULATOR_MODE=1 --define SST_ENABLE_DPRAM=0 --define CHB_LOAD_EEPROM_PARAMETERS=0 --define SST_DEBUG_PROTOCOL=2
```

| 构建 | 应加载的CLI固件 |
|---|---|
| Debug、AnyEyes | `build/Debug/bench_inputs/SST_AnyEyes.out` |
| Release、AnyEyes | `build/Release/bench_inputs/SST_AnyEyes.out` |
| Debug、VOFA | `build/Debug/bench_inputs_vofa/SST_AnyEyes.out` |
| Release、VOFA | `build/Release/bench_inputs_vofa/SST_AnyEyes.out` |

前两条命令使用头文件默认的AnyEyes协议1；后两条显式选择VOFA协议2，与本次构建报告及证据导出的配置一致。

在CCS的`Run → Load → Load Program`选择实际生成的测试`.out`，同时使用该文件的符号；不要继续加载普通`build/Debug/default`或先前的`Debug/SST_AnyEyes.out`。CCS直接构建时则加载其刚生成的`Debug/SST_AnyEyes.out`，并核对该配置确有上述预定义。

在Expressions添加`g_sst_bench_inputs`和`g_sst_bench_status`。若提示符号不存在，说明加载的固件/符号未启用测试宏，先核对文件路径及构建日志。Resume后`g_sst_bench_status.tad_count`应增加，`invalid_input`应为0；存在符号但计数不增加时，检查是否完成原20秒上电等待、外部TAD是否到达、是否一直停在断点或发生板卡复位。输入发生器依赖真实GPIO69/TAD，不会自行产生硬件中断。

本次恢复旧兼容枚举`CHB_CONTROL_MODE_SIMULATOR=0`、`CHB_CONTROL_MODE_DPRAM=1`，避免此前手改仿真器枚举为1造成重号。实际控制权限由`SST_SIMULATOR_MODE`、`SST_ENABLE_DPRAM`及其派生宏决定，不能通过修改旧枚举来启用输入发生器。

## 3. CCS输入字段与观察量

`g_sst_bench_inputs`在初始化后恢复下表默认值。CCS修改应在运行初始化完成后进行；初始化之前写入会被覆盖。暂停后双击Expressions的Value列、输入数值、按Enter并刷新读回。多相DC应在暂停时一起修改，避免运行中逐字段写入产生暂时不平衡。

| 表达式 | 默认值 | 单位与允许范围 | 作用 |
|---|---:|---|---|
| `g_sst_bench_inputs.dc_a_v` | 300.0 | V，0～10000 | A相虚拟模块均压 |
| `g_sst_bench_inputs.dc_b_v` | 300.0 | V，0～10000 | B相虚拟模块均压 |
| `g_sst_bench_inputs.dc_c_v` | 300.0 | V，0～10000 | C相虚拟模块均压 |
| `g_sst_bench_inputs.grid_peak_v` | 325.0 | V峰值，0～20000 | 平衡三相相电压峰值；不是线电压有效值 |
| `g_sst_bench_inputs.grid_frequency_hz` | 50.0 | Hz，0～100 | 独立输入电网频率 |
| `g_sst_bench_inputs.current_peak_a` | 0.0 | A峰值，0～10000 | 与虚拟相电压同相的平衡电流峰值 |

325V相电压峰值只是本次流程测试值，不代表实际高压系统额定量。理想正弦下相电压有效值约230V，线电压有效值约398V。注入同时更新线电压及网侧电流，因此旧遥测的电流有效值也是当前输入的200点统计，不能把电流峰值直接当作RMS。

| 观察量 | 含义 |
|---|---|
| `g_sst_bench_status.tad_count` | 初始化后实际执行的注入TAD数量，自然回绕；不包含断点暂停墙钟时间 |
| `g_sst_bench_status.angle_rad` | 本拍独立输入电网相角，0～2π |
| `g_sst_bench_status.invalid_input` | 1表示任一输入或采样周期非法，本拍全部输入置零；0表示合法 |
| `HA_Port_Volt_A/B/C`、`HA_Port_Current_A/B/C` | 当前注入的相电压、电流样本 |
| `SVG_Udc_A_Ave`、`SVG_Udc_B_Ave`、`SVG_Udc_C_Ave`、`SVG_Udc_Ave` | 三相均压及算术平均，与人工DC输入一致 |
| `PORT_RealTime_DATA.PORT_I_A/B/C` | 后台发布的200点RMS电流；不是当前TAD瞬时值 |

输入校验拒绝NaN、无穷及越界值；控制采样周期的允许范围为1μs～10ms，本次保持`g_chb_dab_params.sample_period_s=0.0001`。非法输入置零只是输入处理行为，不会自动请求停机或封波。若运行中出现`invalid_input=1`，应人工将`run_enable`、`dab_enable`均置0，Resume确认停机，再恢复合法输入；不能把此标志当作保护动作。该测试对象不接入AnyEyes/DPRAM点表，也不存入EEPROM；本次临时参数不要执行SAVE。

## 4. 按自然状态机完成闭环启动

在初始化结束后、`TaskSlice.c:24`调用`ChbDabControl_Service()`前设置一次断点。它每轮后台都会调用，命中后完成设置并禁用，避免反复暂停。默认`ac_mode=0`、`startup_strategy=0`、两路运行命令为0；测试固件没有自动切闭环或自动启动。

停机时设置以下量，保留原保护默认使能：

| 表达式 | 本次设置 |
|---|---:|
| `g_chb_dab_command.run_enable` | 0 |
| `g_chb_dab_command.dab_enable` | 0 |
| `g_chb_dab_command.ac_mode` | 1（闭环） |
| `g_chb_dab_command.dc_voltage_ref_v` | 680.0 |
| `g_chb_dab_command.iq_reference_a` | 0.0 |
| `g_chb_dab_params.startup_strategy` | 1（Id斜坡后移交） |
| `g_chb_dab_protection_enable` | 1 |
| `g_sst_bench_inputs.dc_a_v/dc_b_v/dc_c_v` | 各300.0 |
| `g_sst_bench_inputs.grid_peak_v` | 325.0 |
| `g_sst_bench_inputs.grid_frequency_hz` | 50.0 |
| `g_sst_bench_inputs.current_peak_a` | 0.0 |

正常操作顺序及预期如下。计数对应目标实际执行的TAD/Timer2/后台服务，不包含断点暂停时间。

1. 将`run_enable=1`，Resume，应从`STOPPED=0`进入`PRECHARGE=1`。此时进线命令`ac_input_closed=1`、旁路`precharge_bypassed=0`，CHB/DAB PWM均关闭。300V低于默认预充门槛350V，不会自动达标；及时执行下一步，默认预充等待超时为10s。
2. 暂停，把三个`dc_*_v`均改为400.0，Resume。控制读取的三相均压最小值达到350V，连续20个10ms服务拍后进入`BYPASS_DELAY=2`，旁路命令变为1；再经过默认20个10ms拍旁路延时进入`WAIT_PLL=3`。
3. 输入325V峰值、50Hz波形交给真实PLL计算，频率应接近50Hz。快速PLL在幅值≥100V且归一化q轴误差绝对值≤0.02时累计约100个TAD样本置锁定；等待态还要求连续20个10ms服务拍锁定后才进入`RUNNING=4`。PLL会在停机/预充阶段运行，可能在进入等待态前已经锁定。不要直接写`pll_locked=1`来替代这项测试。
4. 进入运行态的入口先执行PI/启动状态复位、中性参考及CHB PWM解锁。策略1下`startup_phase=1`、DAB仍关闭。Id给定从0按0.05A/TAD升到10A，约200拍，即10kHz时约20ms；`startup_ramp_tad=2000`是最长等待计数，不是固定200ms斜坡。
5. 到达Id目标或最长计数后，程序预置电压PI积分，使当前输出对齐斜坡末值，`startup_phase=2`保持移交拍；下一TAD进入3、`voltage_loop_active=1`。接管不要求人工DC先达到680V。进入运行时DC为400V，启动电压参考初值为400+20=420V。
6. 暂停后把三相DC依次共同改为500、600、680V，每次Resume观察`dc_voltage_average_v`、`startup_voltage_reference_v`及`id_reference_a`。电压参考默认每TAD最多步进2V；反馈进入680±10V范围时参考直接对齐680V。电流保持人工零值，PI可能限幅；680V反馈误差为0也不意味着Id输出必须为0，因为积分保留。
7. 可运行中设`dab_enable=1`，观察DAB初始占空计数350，每TAD增加1至2500，10kHz时名义约215ms。置0关闭DAB、CHB继续运行。最后将`run_enable`和`dab_enable`均置0，Resume，确认`state=0`、两路PWM关闭、闭环进线/旁路撤销。

这里400→680V全部由操作者指定，不是控制器已经给电容充电。要评估PI动态，需要由调制/PWM驱动的主回路模型或HIL，不能把输入自动跟随给定作为稳定性证据。`g_chb_dab_status`应以观察为主，**不要直接把`state`写成4**；那会绕过状态入口动作，产生运行镜像与实际PWM/PI初始化不一致。

## 5. 断点位置与电压环移交观察

以下行号对应本次`ChbDabControl.c`，该控制文件没有因输入模块而插入代码；后续修改仍应以函数及语句定位。在可执行行左侧双击加断点，`View → Breakpoints`可禁用；暂停后选当前C6748栈帧。`F6/F5/F8`通常是Step Over/Step Into/Resume，以当前CCS配置为准。

| 文件、行号与语句 | 用途 | 使用方式 |
|---|---|---|
| `TaskSlice.c:24`，`ChbDabControl_Service()` | 初始化后设置模式及启停 | 命中一次后禁用 |
| `ChbDabControl.c:620`，`g_chb_dab_status.state = state` | 捕获自然状态转换 | 暂停时局部`state`为目标状态，旧状态尚未更新 |
| `ChbDabControl.c:670`，建立Id斜坡阶段 | 策略1运行入口 | 单步到PWM使能后再观察，命中该行时不能认定PWM已经打开 |
| `ChbDabControl.c:1073/1100/1119`附近条件 | 预充、旁路、PLL慢速确认 | 临时逐段检查，不修改计数或状态跳过条件 |
| `ChbDabControl.c:1348`，`Chb_AlignVoltagePiToIdReference(...)` | 斜坡结束时积分对齐 | 比每TAD入口断点更容易捕获一次移交 |
| `ChbDabControl.c:1360`，进入正常电压环阶段 | 移交后的下一拍 | 捕获后禁用 |
| `ChbDabControl.c:1370`附近PI调用 | 查看真实电压PI计算 | 高频位置，短时单步后禁用 |
| `ChbDabControl.c:895`，ForceFault记录`fault_code` | 捕获控制故障原因 | 观察参数及Call Stack；函数定义在882行 |
| `Redef_Interrupt.c:575`，`FaultCheckAndDispose_StepOne()` | 查看GPIO142硬件FAULT路径 | 排查缺失板卡或UNIT反馈，不当作虚拟输入保护结果 |

关键状态观察量为`state`、`fault_code`、`dc_voltage_average_v`、`dc_voltage_min_v`、`pll_frequency_hz`、`pll_locked`、`startup_phase`、`startup_tad_count`、`startup_voltage_reference_v`、`id_reference_a`、`id_feedback_a`、`iq_feedback_a`、`voltage_loop_active`、`modulation_actual`和`dab_duty_count`。全局测试输入可以随时在停机/暂停时改；内部静态量和局部量只观察。局部量不可见时确认加载未优化Debug符号并选对栈帧。

频繁高频断点会改变整个执行过程。停在TAD中断时GPIO Bank4已暂禁，暂停期间丢失的外部边沿不会逐拍补回；Timer2 pending也不是墙钟时间的完整队列。一次流程断点与连续运行观察应分开，不以单步期间的数值评估动态性能或100μs截止期。

## 6. 停止、超时与快速保护测试

每项从完全停机开始，使用新的人工输入，不直接清故障码或改状态。在故障后先把两路运行命令置0、撤销测试故障输入，确认两路实际PWM镜像关闭，再设`fault_reset=1`，Resume让核心及Timer2锁存清除流程完成。确认故障消失后再启动；复位不会自动运行。硬件故障源仍存在时会重新跳闸。

| 测试 | 操作 | 应观察的结果 |
|---|---|---|
| 预充超时 | 三相DC保持300V，闭环`run_enable=1`，连续运行 | 保持预充，默认约10s后`state=5`、`fault_code=1`，PWM关闭 |
| PLL等待超时 | 停机时三相DC设400V、`grid_peak_v=0`；先Resume直至观察`pll_locked=0`，再提交`run_enable=1` | 预充/旁路达标后停在等待PLL，默认从进入等待态计约5s后故障码2 |
| 快速DC过压 | 先按正常步骤进入闭环运行，软件保护保持1，再将一相或三相DC改为800V | 超过快速阈值780V，默认连续5个运行TAD确认，进入故障码4并封波；10kHz时名义样本窗口0.5ms，响应未作板测 |
| 正常停止 | 运行时两路命令均置0 | 回停机、两路PWM关闭；不依赖故障复位 |

PLL超时测试不能在已有锁定后仅停机并立刻置`run_enable=1`：失压必须先经过TAD使真实`pll_locked`归零，否则遗留锁定状态可能让测试路径不同。运行中的PLL失锁没有在本方法中新增检测器，验证的是启动等待超时。

快速过压使用三相各自模块均压的最大值≥780V及连续TAD计数。800V低于原HVAC软件过压阈值830V，便于分别观察新快速保护。原过流比较200点RMS，原过压比较总均压，计时单位为10ms Timer2而非TAD；一级过流和过压保留95%回差，详情见[FAULT_CODES.md](FAULT_CODES.md)。注入保留这些软件链路，没有通过关闭保护来实现启动。

非法输入测试可在停机时把一个DC输入改成负值，Resume观察`invalid_input=1`及本拍输入归零；随后恢复合法值。该标志没有映射成新的故障码，不能将它当作FO/FB硬件保护测试。修改测试峰值时仍保留真实软件阈值：例如电流输入过大可能触发快速或RMS保护，这属于预期路径。

## 7. 单UNIT及实际硬件限制

当前PM保持A1=`0x0001`、A2=0，B/C两组为`0x007F/0x003F`，逻辑模块数A为1、B/C各13。它们是原硬件通道配置，不是软件模型的虚拟模块开关。本次输入注入没有自动把PM或`NHA_module_A/B/C`改成单UNIT；闭环配置仍要求三相逻辑模块数非零，输出归一化仍使用这些数。

只接一块UNIT时，虚拟三相反馈用于验证三相软件流程，不能说明三相物理通道存在。不要把B/C模块数清零并注释配置检查来通过启动。实际UNIT所处插件/光纤位须按接线确认；需要调整FPGA的AD/FO/FB使能时按实际硬件缺卡原因设置，保留已接通道保护，不能盲目把所有故障掩码清零。

测试宏不会屏蔽GPIO142、FO/FB、`Port_RealFault.bit15`或硬件故障ISR。缺失板卡/UNIT仍可能先于流程测试触发FAULT。用ForceFault及硬件FAULT入口断点查看原因和调用栈，再核对硬件使能；不靠每轮清故障或修改软件状态强行继续。GPIO142路径进入后先执行真实封波，只有软件FB掩码变化不能保证阻止它。

后台通过GPIO40翻转喂外部看门狗，DSP暂停后该翻转停止。长断点导致复位时，依据实际板卡电路确认调试旁路，不假定CCS能冻结外部看门狗。没有真实TAD、仍在20秒初始化等待或发生复位时，测试输入/PLL都不能按上述步骤推进。

## 8. 测试结束恢复真实采样

1. 将`run_enable`、`dab_enable`均置0，确认封波并结束测试。临时参数不执行SAVE。
2. 删除CCS中`SST_BENCH_INPUT_SIM=1`预定义，或明确设为0；按实际运行需求恢复仿真器/DPRAM/EEPROM选择。头文件的测试宏默认仍为0，不删除输入模块源码。
3. Clean并重新Build正常固件，加载其正确`.out`及符号。正常构建中`g_sst_bench_inputs/status`不应存在，`Port_AD_Sample(0)`、`SVG_Udc_Average()`及`Udc_check()`恢复原硬件路径。
4. 重新核对真实模块位图、采样比例、保护使能、相位补偿及硬件故障输入，再按[BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md)进行实际回路验收。输入测试固件不能作为真实主回路运行固件。

本次基线保存与交付不纳入工作区的本机`.launches/`及DPRAM临时zip文件；CCS调试启动配置、实板波形和测量记录需按实际使用环境保存。本文没有把源码实现、预期流程或软件回归当作已完成的板上10kHz时序与保护响应测量。

## 9. 本次编译与软件验证结果

本轮使用TI C6000 8.3.12完成7种目标构建：普通固件Debug/Release、AnyEyes测试固件Debug/Release、VOFA测试固件Debug/Release，以及关闭测试宏的DPRAM远控Debug。每种均为88个源文件，编译和链接无错误，保留17个既有告警。各产物、配置和报告的SHA256见[bench_input_validation.json](evidence/bench_input_validation.json)。

14项主机回归全部通过，新增用例直接编译真实输入发生器及控制函数体，在硬件边界替身上检查两种启动策略、自然PLL锁定/失锁、Id斜坡移交、DAB爬坡、停机、过流/过压及超时。独立TI隔离检查38项通过：测试宏关闭时四个接入函数和完整ISR/CHB单元的预处理词法与`6675a72`相同，四类ISR的归一化汇编、直接调用和栈量相同；开启时验证注入顺序、锁存采样保留及宏互锁。7份map的固定硬件地址、NOINIT及无启动加载镜像核对通过，130个自定义源码文件的注释审计通过。

这些软件结果不包含实板执行时间、实际PWM输出或电压环动态稳定性测量。测试开启后采样函数内部计算已经变化，需在板上连续运行时按[BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md)测量GPIO43/45脉宽和TAD丢拍情况，才能认定完整流程满足100μs截止期。

复验时先按第2节重建4个测试固件，再运行普通Debug/Release及DPRAM远控Debug；运行主机回归需要GCC及`tree_sitter`，可使用本机已配置的Python运行环境：

```powershell
python tools/build.py --configuration Debug
python tools/build.py --configuration Release
python tools/build.py --variant anyeyes_remote_dpram --define SST_SIMULATOR_MODE=0 --define SST_ENABLE_DPRAM=1 --define SST_BENCH_INPUT_SIM=0
& 'C:/Users/Roj/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' tools/run_checks.py
python tests/bench_isolation.py
python tests/dpram_map_audit.py build/Debug/default/SST_AnyEyes.map build/Release/default/SST_AnyEyes.map build/Debug/bench_inputs/SST_AnyEyes.map build/Release/bench_inputs/SST_AnyEyes.map build/Debug/bench_inputs_vofa/SST_AnyEyes.map build/Release/bench_inputs_vofa/SST_AnyEyes.map build/Debug/anyeyes_remote_dpram/SST_AnyEyes.map --report build/bench_isolation/dpram_maps.json
python tools/export_bench_validation.py
```

最终标签为`bench-input-simulation-2026-10-14`，与测试前基线分别保存。导出脚本要求全部产物、报告及当前源码指纹吻合才写入摘要；复验不能只沿用旧报告。
