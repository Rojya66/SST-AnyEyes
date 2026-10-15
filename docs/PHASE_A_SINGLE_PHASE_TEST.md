# A 相 AN 采样与 SOGI 虚拟 dq 双闭环测试

日期：2026-10-15；工程：`D:/Code/SST/AnyEyesVersion`；目标：C6748 + FPGA。修改前基线为 `511583a`，现场模块配置和已有注释归属保存在 `bcd8d1c`（标签 `baseline-before-phase-a-sogi-2026-10-15`）；实现版本使用标签 `phase-a-sogi-2026-10-15` 区分。本次增加独立 `SstSinglePhase.c/.h`，不替换原三相闭环 PI、参数或状态机。

## 1. 宏和模块配置

`Source/04_App/00_Config/ChbBuildConfig.h` 新增 `SST_PHASE_A_ONLY_TEST`，默认 **0**。设为1并重新编译才启用 AN 单相测试。CCS 可在 C6000 Compiler → Predefined Symbols 添加 `SST_PHASE_A_ONLY_TEST=1`，注意 Debug/Release 分别配置，切换后 Clean + Build；同名宏使用 `#ifndef`，命令行定义优先。

| 场景 | 单相宏 | 输入注入宏 | 仿真器宏 | DPRAM宏 |
|---|---:|---:|---:|---:|
| 保留当前三相算法及台架输入 | 0 | 1 | 1 | 0 |
| 无主回路，验证单相启动流程 | 1 | 1 | 1 | 0 |
| 接真实 AN、IA 反馈的单相控制 | 1 | 0 | 按控制来源选择 | 按需求选择 |
| 恢复三相真实采样 | 0 | 0 | 按控制来源选择 | 按需求选择 |

`SST_DEBUG_PROTOCOL` 仍是1=AnyEyes、2=VOFA二选一；输入注入开启时仍要求仿真器=1、DPRAM=0、EEPROM加载=0。单相宏与串口协议互相独立。

当前 `ChbDabControl.h` 保留用户已有位图 A1=`0x0007`、A2/B1/B2/C1/C2=`0`，即 **A 相3个逻辑模块**，不是1个。只有一套 UNIT 时应把 A1 改成 `0x0001` 后重编译；串联3个模块则保留 `0x0007`。单相模式要求 `NHA_module_A>0` 且 `NHA_module_B=NHA_module_C=0`，否则启动报配置故障。宏不擅自改现场模块位图；模块数仍用于总相电压尺度 `N_A * dc_voltage_ref_v`。

## 2. 采样与输出的完整路径

```text
GPIO69 FPGA TAD → FPGAIsr（入口、应答、周期及调用顺序保持）
  → Port_AD_Sample(0)
      真实输入：模拟插件 AD[6] → 原零偏/增益/PT比例/低通 → HA_Port_Volt_A=AN
                A1/A2光通信插件 Avg_IG / 10 → HA_Port_Current_A=IA
      注入输入：SstBenchInputs_FastStep → AN、IA、A相母线
  → HVAC_RealTimeParameter（AN/IA有效值、P和单相Q）
  → SVG_Udc_Average（仅A相模块均压）
  → ChbDabControl_FastStep
      → 电压/电流 SOGI → 虚拟 αβ → 原 PLL
      → 原 Udc 外环 → Id给定；原 dq 电流PI和电压前馈
      → dq逆变换的真实 α 分量 → A相归一化参考 → 原501/UNIT链路
```

真实电压来自 `FPGA_Interface[Analog_PluginUnit_3 - 2].AD[6]`，锁存读取同一插件的 `AD_LATCH[6]`。本次把原第一路线电压输入的**物理接线改为 AN**作为前提，不在软件中除以 `sqrt(3)`，也不执行 AB/BC→ABC 变换。AD[7] 即使有输入，也不参与控制电压。保留标定：`x[n]=(0.0079785*raw[n]+0.15957)*HA_PT_Ratio`，原低通为 `y[n]=0.9567*x[n-1]+0.04328*y[n-1]`；不能仅因接线改为AN就任意沿用不匹配的传感器/PT比例。先用已知低压校验幅值、零点及极性。

电流仍优先读取存在的 A1（否则 A2）501插件 `Avg_IG/10`，不改为模拟插件网侧电流；`current_feedback_sign` 同时作用于电流 α、β。网侧电流遥测另保留原 AD[3] 读取路径。B/C相实际电压、电流明确清零，虚拟β只放在 `g_sst_single_phase_status`，不会伪装成B相测量。

旧 `HA_Port_Volt_AB` / `HA_Port_Volt_AB_LATCH` 首槽在单相模式兼容存放 AN；其余旧线电压槽清零。通信地址、电子标签和波形索引未改。原首电压RMS点 `PORT_V_A` 在单相模式表示 **AN有效值**，B/C电压电流为0；P是实际 `AN*IA` 的滤波平均值（kW），Q为基波 `0.5*(vβ*iα-vα*iβ)`（kvar，电流滞后为正）。单相外电源/欠压判据只检查AN，`Ust_HVAC_Port`、`VoltLevel_1/2`须按AN电压等级设置。

## 3. SOGI方向与既有调节器复用

只参考 `D:/Code/ClaudeWork/Solid_State_Transformer_V2_vofa/SST_PLL.txt` 中“电压、电流分别构造正交量”的思路。本工程采用梯形/Tustin离散，阻尼 `k=sqrt(2)`，没有复制参考项目的PLL、闭环算法、PI参数或补偿角。SOGI频率取本工程现有PLL上一拍估计频率。

连续模型为 `dα/dt=kω(u-α)-ωβ`、`dβ/dt=ωα`。稳态输入 `U*cos(θ)` 时，`α≈U*cos(θ)`、`β≈U*sin(θ)`，即β滞后α90°。匹配现有Park约定 `d=α*cosθ+β*sinθ`、`q=-α*sinθ+β*cosθ`，同相正弦在锁定后落到正d轴。

仅上电或非法输入时清历史，正常停机继续滤波。`valid=1`表示本拍数值有效；连续1000个有效TAD（100ms）后 `ready=1`，随后还需原PLL锁定误差及连续锁定时间判据。`ready`本身不代表电网有效或PLL已锁定，零电压仍不能通过PLL最低幅值检查。

电压外环继续读取模块平均Udc，输出Id；原d/q电流PI、给定限幅、启动Id斜坡/电压环移交、电压前馈和补偿角继续使用 `g_chb_dab_params`。逆变换仅取真实α，归一化为 `1+uα/(N_A*Udc_ref)`。**参数数值保持已有版本**，不是已针对单相主回路重新整定的结果；单相母线存在100Hz能量脉动，SOGI也引入滤波动态，实物电感、电容和电流环带宽应另行验证。

## 4. B/C封波和保护

单相下 B/C 归一化参考始终为中点1.0。`Module_Control_Write()`另外向B1/B2/C1/C2全部槽位显式写入 `SVG_EN=0`、`DAB_EN=0`，即使模块掩码为0或寄存器遗留旧值也撤销两桥使能。A相仍走原使能链。开环、独立DAB和闭环都遵循此门控；仅写参考中点不等价于硬件封波，现场需读回控制字并观察输出。

快速软件过流检查物理 `HA_Port_Current_A`，不等待SOGI滤波；快速过压只检查 `SVG_Udc_A_Ave`，仍使用原 `ac_overcurrent_trip_a`、`dc_overvoltage_trip_v` 和连续确认拍数。原 `FaultCheck.C`、FO/FB及故障上报编码机制保留。`SVG_Udc_Ave`直接采用A相原100Hz滤波后的均压，去除不适合单相的三相除三、总均压500V下限和B/C就绪条件。`UstOK_HVAC_Port±Ust_add`仍为原母线就绪判据，和现代预充阈值是不同用途。

非法采样、非法频率或周期会清SOGI并撤销锁相；运行中的闭环进入 `CHB_FAULT_BAD_CONFIGURATION=6` 并封波。闭环周期必须为100µs（浮点容差1e-8s），不得用调试器改变它来调控制速度。开环仍可独立于未接采样做脉冲测试。

运行中PLL一般失锁的处理仍沿用原控制逻辑，本次没有增加“只要pll_locked变0便立即跳闸”的新保护；既有欠压、硬件故障和软件保护继续负责各自条件。不要把SOGI的数值非法封波与完整的失网/失锁保护混为一谈。实际电流反馈方向、501归一化指令与桥臂接线极性仍须低压核对。

## 5. 无主回路的CCS测试步骤

1. 使用单相+输入注入构建。没有主回路的台架仍需隔离功率驱动和实际开关执行机构，因为状态机会下发真实开关/PWM命令。加载正确 `.out` 与符号，运行到20秒初始化结束。
2. Expressions 添加 `g_sst_bench_inputs`、`g_sst_bench_status`、`g_sst_single_phase_status`、`g_chb_dab_command`、`g_chb_dab_params`、`g_chb_dab_status`、`Module_PM_CTL`，以及四个B/C插件槽位的 `PM_CTL.PM_CTL16`。确认 `run_enable=dab_enable=0`。
3. 运行至少200ms后短暂停机观察：注入默认 `grid_peak_v=325`、`grid_frequency_hz=50`、`current_peak_a=0`、`dc_a_v=300`；`settling_count=1000`、`ready=valid=1`，PLL应最终锁定且频率约50Hz。观察α/β的峰值和相位；余弦输入时β滞后90°。B/C采样及母线为0。
4. 停机修改 `ac_mode=1`、`dab_enable=0`，确认 `sample_period_s=0.0001`，保留保护使能。设置 `run_enable=1` 后连续运行：默认300V低于350V预充门槛，应停留PRECHARGE。短暂停机把 `g_sst_bench_inputs.dc_a_v=400`，继续后应依次经过 BYPASS_DELAY、WAIT_PLL、RUNNING。此过程只改A相注入，B/C值不影响预充。
5. 默认启动策略为0；停机后可把 `startup_strategy=1` 再运行上述流程，观察 `startup_phase` 从Id斜坡到电压移交及正常阶段、`voltage_loop_active=1`。Udc目标默认680V，输入不会响应PWM；可逐步人工改 `dc_a_v` 到680以检查给定和误差，不能由此证明真实闭环收敛。
6. 在RUNNING且SOGI稳定时把 `current_peak_a`短暂设为10，观察Id/Iq反馈有限且随相位补偿变化，A相参考有界，B/C参考中点、使能位始终为0。默认补偿角非零，因此不要求Iq恰好为0。检查P约为 `325*10/2/1000=1.625kW`，同相输入Q约为0。
7. 将 `current_peak_a`恢复0，再把 `dc_a_v=800`，运行后应触发快速过压故障（默认780V门槛、连续5拍）；测试过流可恢复正常母线并复位后把 `current_peak_a=200`。旧软件保护也在运行，实际首故障可能受旧阈值与后台时序影响，应同时查看现代故障、原故障字和上报清单。停止、恢复测试输入后设 `fault_reset=1`，确认不会自动重启。
8. 把 `run_enable=dab_enable=0`，确认封波。临时CCS参数不要SAVE；真实运行前关闭输入注入，按实际位图与采样比例重编译。

建议在 `Chb_EnterState()` 中以新状态为条件加断点，或在 `ChbDabControl_Service()` 的预充达标、WAIT_PLL转RUNNING位置加一次性断点。检查正交量时用连续采集/波形更可靠；避免在10kHz函数每拍停机。**DSP断点不会冻结外部FPGA**，其可能继续保持最后PWM，暂停也会停止外部看门狗喂狗；不得靠断点作为功率封波手段。断点暂停期间不累计TAD算法时间，恢复后再等滤波/PLL建立。

## 6. 构建与软件验证

```powershell
# 单相、台架输入（AnyEyes）
python tools/build.py --variant phase_a_bench --define SST_PHASE_A_ONLY_TEST=1
# 单相、真实采样（AnyEyes）
python tools/build.py --variant phase_a_real --define SST_PHASE_A_ONLY_TEST=1 --define SST_BENCH_INPUT_SIM=0
# 单相、真实采样、VOFA并口远控兼容
python tools/build.py --variant phase_a_vofa_dpram --define SST_PHASE_A_ONLY_TEST=1 --define SST_BENCH_INPUT_SIM=0 --define SST_DEBUG_PROTOCOL=2 --define SST_SIMULATOR_MODE=0 --define SST_ENABLE_DPRAM=1
# 恢复三相真实采样
python tools/build.py --variant phase_abc_real --define SST_PHASE_A_ONLY_TEST=0 --define SST_BENCH_INPUT_SIM=0
python tools/run_checks.py
```

产物为 `build/Debug/<变体>/SST_AnyEyes.out`；Release加 `--configuration Release`。当前默认单相宏0，因此普通Build仍产生三相算法，必须确认加载的变体。专项主机测试直接编译生产SOGI/控制函数，验证45/50/55/60Hz正交幅相、1000拍ready、非法输入恢复、PLL、预充/电压环移交、物理A过流/过压、B/C参考及最终硬件控制字封锁。构建与测试报告随本轮验证保存。

本轮6种构建均为90个编译单元，编译和链接0错误；单相保留16个原有告警，三相17个，没有新增告警。`tools/run_checks.py` 的16项全部通过，原三相8个控制场景逐拍结果与原版相同；134个自定义源码的clang-format 20.1.8与注释覆盖检查通过，多语句拥挤行及超过120列的代码行为0。6份map的硬件地址及NOINIT检查通过。可携带证据见 [single_phase_validation.json](evidence/single_phase_validation.json)，用 `python tools/export_single_phase_validation.py` 在构建和回归完成后重新导出；导出器拒绝源码指纹不一致的旧构建报告。

## 7. 10kHz外部中断的验证边界

本次没有更改 `FPGAIsr`正文、GPIO69中断注册/应答、`ControlTAD`、`TAD_PLL`、Timer2或任务调度。单相SOGI在原FastStep内部每TAD执行一次；固定算式无循环、无新增三角函数、无通信/EEPROM/等待，每拍共享一次浮点除法。宏0下恢复既有三相分支，宏1减少未使用相的部分运算，但同时增加双SOGI。

软件审计和编译只能确认入口与调度不变、代码可构建，**不能代替完整ISR最坏执行时间实测**。请按 [BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md) 同时测GPIO69与GPIO43/45，覆盖预充、锁相、闭环、DAB、通信满负载和故障；验证无漏拍且包含入口/尾部的总完成时间小于100µs并有余量。测试固件开启输入注入还会执行原台架波形生成，需要分别测真实采样和注入配置。
