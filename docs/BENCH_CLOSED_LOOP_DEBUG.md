# 单UNIT、无主回路的闭环启动模拟与CCS断点指南

核对日期：2026-10-14。适用工程：`D:/Code/SST/AnyEyesVersion`。本文提供临时修改方法；本轮没有修改固件源码或重新编译。行号对应修改前源码，插入代码后按函数及语句定位。

## 1. 先明确验证目标

可以在现有C6748控制系统上验证预充、旁路、PLL等待、PWM使能命令、Id斜坡、电压环接管、停止和故障复位的程序流程。没有主回路或被控对象模型时，模拟采样不会证明PI稳定性、母线充电能力或实际保护响应。

`SST_SIMULATOR_MODE=1`只改变命令权限，没有模拟电压、电流或FPGA。当前工作区已临时设为该模式，并将`SST_ENABLE_DPRAM`设为0。保留这两个配置，并确认CCS编译器预定义符号没有用相反值覆盖头文件。建议上电EEPROM加载保持`CHB_LOAD_EEPROM_PARAMETERS=0`，避免加载旧参数覆盖调试值。

当前控制默认`ac_mode=0`（开环）、`startup_strategy=0`（原版兼容）。进入闭环要设`ac_mode=1`；观察Id斜坡及电压环移交则再设`startup_strategy=1`。保留策略0时，运行入口直接启用电压环，这也是合法启动方式。

调试期间保持主回路、功率驱动和实际开关执行机构未接入。DSP断点不会保证外部FPGA停止：FPGA可能继续输出最后下发的PWM。状态中的`ac_pwm_enabled=1`只是软件使能镜像。

## 2. 最快方法：先不改算法，用断点逐段走通

使用Debug构建和对应`.out`。在初始化结束后、主循环调用`ChbDabControl_Service()`之前暂停，例如在`TaskSlice_Service()`中调用该函数的行上加断点。不要在`main()`最开始就修改参数，因为后续初始化可能覆盖它们。

打开`View -> Expressions`，加入下列量。在暂停状态双击Value列输入数值、按Enter确认，刷新读回。

| 表达式 | 初始值 | 用途 |
|---|---:|---|
| `g_chb_dab_command.run_enable` | 0 | 设置模式和参数时保持停机 |
| `g_chb_dab_command.dab_enable` | 0 | 第一轮只检查交流启动；之后再验证DAB |
| `g_chb_dab_command.ac_mode` | 1 | 闭环 |
| `g_chb_dab_command.dc_voltage_ref_v` | 680.0 | 最终模块电压目标 |
| `g_chb_dab_command.iq_reference_a` | 0.0 | q轴给定 |
| `g_chb_dab_params.startup_strategy` | 1 | 固定Id斜坡后移交电压环 |
| `g_chb_dab_protection_enable` | 0 | 第一轮暂不测试软件OC/OV；不屏蔽硬件故障 |
| `g_chb_dab_status` | 观察 | 状态、故障、PLL、使能、给定和反馈 |
| `NHA_module_A/B/C` | 各非0 | 闭环基础配置要求 |

在完成初始化后的停机状态将`run_enable=1`，继续执行。状态应按`0 -> 1 -> 2 -> 3 -> 4`变化。为了立即检查分支，可使用下面的临时赋值；这些赋值用于单步，不用于连续闭环运行。

| 修改前源码断点 | 暂停时改什么 | 单步后的预期 |
|---|---|---|
| `ChbDabControl.c:1073`，预充电压判断前 | `g_chb_dab_status.dc_voltage_min_v=400.0`；`s_precharge_stable_count=g_chb_dab_params.precharge_stable_10ms-1`（默认19） | 越过条件及计数，进入`BYPASS_DELAY=2` |
| `ChbDabControl.c:1100`，旁路延时判断前 | 当前栈帧局部量`elapsed=g_chb_dab_params.bypass_delay_10ms`（默认20） | 进入`WAIT_PLL=3` |
| `ChbDabControl.c:1119`，PLL慢速确认前 | `g_chb_dab_status.pll_locked=1`；`s_pll_lock_10ms_count=g_chb_dab_params.pll_lock_10ms-1`（默认19）；`g_chb_dab_status.dc_voltage_average_v=400.0` | 进入`RUNNING=4`，执行PI复位、初始电压参考420V及PWM使能入口 |

旁路断点处的`elapsed`已在1010行计算；此时只改`tick_10ms`不能改变当前判断。若局部量不可修改，可在1010行执行前改`tick_10ms=s_state_enter_tick+bypass_delay_10ms`，再单步计算。跳过服务节拍限制也必须让`tick_10ms != s_last_service_tick`；不要单纯把这两个计数都清零。

在断点处修改状态镜像只会影响当前单步：恢复运行后TAD会重算均压、PLL和启动量。如果暂时遇到硬件故障，先按第6节排查，不能持续清故障后硬往下跑。

**不要直接将`g_chb_dab_status.state`改成4。** 这会跳过`Chb_EnterState()`中的PI清零、启动阶段建立、中性参考及实际PWM使能，出现“状态显示运行但程序没有真正执行启动”的假象。

## 3. 连续运行方法：增加临时输入注入

需要真实验证PLL计算及自然状态跳转时，增加一个独立测试宏。它只用于当前无主回路测试，正常构建默认关闭。

在`ChbBuildConfig.h`加入：

```c
#ifndef SST_BENCH_INPUT_SIM
/* SST_BENCH_INPUT_SIM：无主回路输入注入开关，0正常采样，1软件模拟。 */
#define SST_BENCH_INPUT_SIM 0U
#endif
```

测试构建在CCS的编译器预定义符号中加入`SST_BENCH_INPUT_SIM=1`，并确认`SST_SIMULATOR_MODE=1`、`SST_ENABLE_DPRAM=0`。该宏需要重新编译，不能在Expressions里修改。

最小方法是在`ChbDabControl.c`中加入下面代码（该文件已有数学库及测量量声明）。放在`ChbDabControl_FastStep()`定义之前：

```c
#if SST_BENCH_INPUT_SIM
/* 各相虚拟模块直流电压，单位V；CCS修改后每TAD持续注入。 */
volatile float g_sst_bench_udc_a_v = 300.0F;
volatile float g_sst_bench_udc_b_v = 300.0F;
volatile float g_sst_bench_udc_c_v = 300.0F;
/* 虚拟相电压峰值，单位V；325V是测试值，不代表10kV系统额定值。 */
volatile float g_sst_bench_grid_peak_v = 325.0F;
/* 虚拟相电流峰值，单位A；默认零，仅检查计算及启动流程。 */
volatile float g_sst_bench_current_peak_a = 0.0F;
/* 独立虚拟电网相角，单位rad；不使用PLL输出伪造PLL输入。 */
static float s_sst_bench_angle_rad = 0.0F;

/*==================================================================
 * 函数功能: 在真实采样之后注入独立50Hz电网和可调模块直流电压
 * 输入参数: 无；读取上述CCS可写测试量
 * 输出参数: HA_Port_Volt_*、HA_Port_Current_*、SVG_Udc_*_Ave
 * 返回值: 无
 * 执行说明: 每TAD调用，仅用于无主回路流程检查，没有功率回路模型。
 *================================================================*/
static void Chb_BenchInjectInputs(void)
{
    float phase_a; /* A相虚拟单位幅值余弦。 */
    float phase_b; /* B相虚拟单位幅值余弦，滞后A相120度。 */

    phase_a = cosf(s_sst_bench_angle_rad);
    phase_b = cosf(s_sst_bench_angle_rad - CHB_TWO_PI / 3.0F);
    HA_Port_Volt_A = g_sst_bench_grid_peak_v * phase_a;
    HA_Port_Volt_B = g_sst_bench_grid_peak_v * phase_b;
    HA_Port_Volt_C = -HA_Port_Volt_A - HA_Port_Volt_B;
    HA_Port_Current_A = g_sst_bench_current_peak_a * phase_a;
    HA_Port_Current_B = g_sst_bench_current_peak_a * phase_b;
    HA_Port_Current_C = -HA_Port_Current_A - HA_Port_Current_B;
    SVG_Udc_A_Ave = g_sst_bench_udc_a_v;
    SVG_Udc_B_Ave = g_sst_bench_udc_b_v;
    SVG_Udc_C_Ave = g_sst_bench_udc_c_v;

    s_sst_bench_angle_rad += CHB_TWO_PI * 50.0F *
                              g_chb_dab_params.sample_period_s;
    if(s_sst_bench_angle_rad >= CHB_TWO_PI)
        s_sst_bench_angle_rad -= CHB_TWO_PI;
}
#endif
```

在`ChbDabControl_FastStep()`局部变量声明后，且在原1594行计算`dc_voltage_average_v`之前，加入：

```c
#if SST_BENCH_INPUT_SIM
    Chb_BenchInjectInputs(); /* 本拍真实采样已结束，控制消费前覆盖输入。 */
#endif
```

注入必须在`Port_AD_Sample()`和`SVG_Udc_Average()`之后。放在二者之前会被覆盖；写`g_chb_dab_status.dc_voltage_*`也会被FastStep重算。若需要模拟旧RMS遥测和原软件保护，还应改成在ISR中以测试分支替代真实采样/均压、并在`HVAC_RealTimeParameter()`之前注入，以保持RMS与控制输入一致；上述最小方案只覆盖现代控制核心输入。

`Udc_check()`读取的是逐模块FPGA采样，不读取这三个模拟均压。单独模拟均压不能同时证明旧48位置巡检。在`Redef_Interrupt.c`原277行调用处，纯流程测试可暂以宏排除该巡检：

```c
            if(g_chb_dab_command.ac_mode != CHB_AC_MODE_OPEN_LOOP)
            {
#if !SST_BENCH_INPUT_SIM
                Udc_check(); /* 正常构建完整保留原模块电压巡检。 */
#endif
            }
```

这只是让虚拟均压测试与真实逐模块巡检分开，不屏蔽GPIO142、FO/FB或端口故障。测试后关闭宏即可恢复真实输入和巡检。新增数学计算会改变测试构建ISR用时，不能沿用原固件的100us时序验证结论。保持GPIO69/TAD及Timer2配置原样；没有实际TAD脉冲时，该注入和PLL都不会运行。

## 4. 连续模拟的操作顺序和预期

1. 在初始化后暂停，设置第2节的模式、策略及命令；`run_enable=0`、`dab_enable=0`。保持`sample_period_s=0.0001`。初始三个虚拟直流电压均300V，电网峰值325V。
2. 将`run_enable=1`，继续运行。应进入`PRECHARGE=1`，且`ac_input_closed=1`、`precharge_bypassed=0`、AC/DAB使能均0。默认预充超时10s，暂停等待不能当作正常运行时间。
3. 暂停，将三相虚拟电压均改为400V，继续运行。连续20个10ms服务拍达标后进入旁路，再等20拍进入PLL等待；这些时长指实际执行节拍，不是包含断点暂停的墙钟时间。
4. 让虚拟50Hz波形自然锁定PLL，观察频率接近50Hz、`pll_locked=1`。默认幅值下限100V、误差门槛0.02，快速层约100个达标TAD样本后置锁定，慢层还需连续20个10ms服务拍才进入运行。PLL可能在预充时已经锁定。
5. 在进入RUNNING的入口断点检查：`ac_pwm_enabled=1`、`dab_pwm_enabled=0`、`startup_phase=1`。随后Id参考按0.05A/TAD升到10A，约200拍，即20ms；2000TAD参数是最长斜坡上限，不是固定等待0.2s。
6. 观察`startup_phase`从1到2，下一TAD到3；`voltage_loop_active`从0到1。接管条件为Id到目标或计数到上限，**不要求母线先达到680V**。电压PI以当前Id预置积分，初始电压参考为进入RUNNING时均压加20V。
7. 分阶段把三相虚拟电压改为500、600、680V，观察电压参考与Id输出。电压参考默认每TAD最多步进2V；反馈进入680±10V附近后对齐680V。反馈、电流都由人工指定，PI输出可能限幅；电压误差为0也不代表Id必然为0，因为积分保留。
8. 独立验证DAB：运行中把`dab_enable=1`，初始占空计数350，每TAD+1至2500，约215ms；置0应关闭DAB，交流侧仍运行。最后将两路运行命令均置0，应回STOPPED并封波。

第一轮使用零电流仅检查程序链。真实闭环动态验证需要另加由调制/PWM驱动的L/R/C及负载模型，或接入HIL，不能用“电压自动跟随给定”或“电流直接等于Id给定”来证明现有PI稳定。

## 5. CCS断点位置及使用方式

在源码可执行行左侧双击添加断点；`View -> Breakpoints`可勾选/取消。暂停后选择当前C6748栈帧，在Expressions修改全局量，在Variables修改当前函数局部量。`F6`通常为Step Over，`F5`为Step Into，`F8`为Resume；以当前CCS菜单/快捷键配置为准。先禁用已经检查完的高频断点，再Resume，避免每100us目标调用都停一次。

| 函数/修改前行号 | 用途 | 推荐方式 |
|---|---|---|
| `TaskSlice.c:24`，调用控制Service前 | 初始化后设模式/参数、提交启停 | 命中一次后禁用 |
| `ChbDabControl.c:620`，写入新状态前 | 捕获每一次状态转换 | 常驻；局部`state`是目标状态，原状态尚未更新 |
| `Redef_Interrupt.c:268`，FastStep前 | 不改源码时注入本拍输入 | 单步使用；新增注入代码后应在注入之后查看输入 |
| `ChbDabControl.c:1073/1100/1119` | 预充/旁路/PLL条件检查 | 逐段启用，检查完禁用 |
| `ChbDabControl.c:670` | 已执行运行入口初始化，准备进入Id斜坡 | 只检查策略1入口 |
| `ChbDabControl.c:1348` | Id斜坡结束、电压PI积分对齐 | 此处只在实际接管时执行，优于每拍断1342 |
| `ChbDabControl.c:1360` | 下一拍进入电压环正常阶段 | 本轮只命中一次 |
| `ChbDabControl.c:1370` | 查看真实电压PI计算 | 临时启用；高频点 |
| `ChbDabControl_ForceFault()`正文 | 捕获进入故障的原因及调用栈 | 查看`fault_code`参数与Call Stack |
| `Redef_Interrupt.c:575` | 捕获硬件FAULT中断路径 | 看是否来自缺失板卡/UNIT反馈 |

若CCS条件断点支持，可在1342行设置条件`s_id_reference >= g_chb_dab_params.startup_id_reference_a`。普通源码条件断点可能通过频繁停机检查实现，不保证实时性；更稳妥的是直接断在只执行一次的1348行。局部量显示不可用时使用未优化Debug构建，并选对栈帧。不要依赖未选中栈帧中的同名局部量。

关键观察量：`state`、`fault_code`、`dc_voltage_average_v`、`dc_voltage_min_v`、`pll_frequency_hz`、`pll_locked`、`startup_phase`、`startup_tad_count`、`startup_voltage_reference_v`、`id_reference_a`、`id_feedback_a`、`iq_feedback_a`、`voltage_loop_active`、`modulation_actual`、`dab_duty_count`。

这些状态镜像原则上观察即可。人工修改只用于第2节的单步条件注入；连续模拟应修改`g_sst_bench_*`及命令/参数。静态内部量若Expressions无法解析，可尝试从当前函数Variables查看；值修改失败不能假定已经写入。

## 6. 单UNIT配置、故障和暂停的实际限制

目前PM配置是A相1模块，B/C各13模块。PM是实际光纤通道位图，不是软件模型模块数，也不是左右桥臂选择。闭环校验要求`NHA_module_A/B/C`全部非零；输出归一化也使用各相模块数。**不能将B/C清零后简单注释掉配置检查**。

纯流程模拟可以沿用当前三相逻辑模块数并注入虚拟三相反馈，但这不代表已有三相实物。若要把真实PM设成仅A相一块UNIT、B/C物理通道关闭，同时运行三相软件模型，需要在测试宏内另设虚拟三相模块数及参考尺度，并隔离虚拟模型与硬件PM/NUM配置；不能把虚拟B/C通道当作实际在线通道。实际UNIT是否位于A1的bit0也应按接线确认。

缺失板卡、UNIT或采样可能触发硬件FAULT。优先按实际接线配置FPGA的AD/FO/FB使能，保留已接UNIT的真实故障通道。旧软件默认FO掩码为`0x3F7F`、FB为`0x4441`，并非单UNIT测试配置，不能机械沿用或全部关闭后宣称保护有效。

`g_chb_dab_protection_enable=0`在当前实现中门控新增软件OC/OV和ISR中的原HVAC软件OC/OV调用，不屏蔽逐模块巡检、GPIO142/FO/FB以及`Port_RealFault.bit15`。硬件故障处置函数进入时先封PWM，再按软件FB掩码分类，因此只在调试器清`Port_RealFault`或修改软件FB掩码不能阻止FAULT中断封波。

遇到故障时用故障入口断点及调用栈找原因。若只做纯软件单步、物理PWM已封锁而缺失输入一直产生FAULT，可在专用测试分支临时屏蔽FAULT IRQ；这会放弃真实硬件保护验证，不应进入实际发波/主回路测试。不要每轮把故障状态和锁存清零。

正常复位顺序为：`run_enable=0`、`dab_enable=0`，确认封波，再`fault_reset=1`并继续运行，让复位及Timer2两步锁存清除完成，确认故障消失后重新启动。如果硬件故障源仍存在，会再次跳闸。

保留GPIO69产生的10kHzTAD和Timer2的10ms时基。停在TAD中断时GPIO bank4已暂禁，暂停期间丢失的采样边沿不会在Resume后逐拍补回；Timer2 pending也不是所有暂停时间的节拍队列。不要以断点恢复后的第一拍判断连续动态性能。

后台通过GPIO40喂外部看门狗，DSP暂停后该翻转也会停。如果长时间断点导致复位，需要根据板卡电路确认外部看门狗调试旁路；不要仅猜测CCS暂停会冻结外部看门狗。

## 7. 测试结束恢复

关闭`SST_BENCH_INPUT_SIM`并清理编译器预定义，恢复实际模块配置、保护使能及实际相位补偿。临时参数不执行SAVE。重新构建、加载固件后确认模拟分支关闭，再按实际主回路验收步骤测试。原生产固件的软件验证不自动覆盖加入测试分支后的构建或板上时序。
