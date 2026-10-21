# PI 重构与 100 Vac 单相低压试验修改记录

2026-10-21 的快速环、单相饱和与保护收敛变更见 [FAST_LOOP_AND_PROTECTION_FIX.md](FAST_LOOP_AND_PROTECTION_FIX.md)。本页后段固件尺寸/构建结果为此前版本记录，不代表本次已完成 TI 构建。

## 1. 版本与适用范围

工程路径：`D:\Code\SST\AnyEyesVersion`。

高压调试检查点已保存为 commit `64aab66`，提交说明为 `refactor: replace CHB PI with documented common controller`，附注 tag 为 `pi-refactor-high-20261019`。该检查点保存公共 PI 重构后的高压参数组，tag 说明记录了当时 Debug/Release 构建及 17 项主机检查通过。后续低压修改不移动此 tag；恢复检查点应使用独立 checkout，避免覆盖未提交的试验修改。

该历史高压检查点仍含本次已发现的“三相 DC_Log 更新缺失及公共安全约束仅限单相”问题；它用于追溯，不能作为这些问题已修复的版本。使用当前源码的 LOW=0 分支可保留高压参数并同时取得本次修复。

当前 `ChbBuildConfig.h` 显式默认 PHASE_A=1、LOW=1，得到单相低压配置。切换三相必须同时设 PHASE_A=0、LOW=0；单相保持 PHASE_A=1、显式 LOW=0 可选历史高压参数。恢复高压硬件使用前还需核对实际模块位图及已保存的现场参数。显式组合 `SST_LOW_VOLTAGE_100V_TEST=1` 与 `SST_PHASE_A_ONLY_TEST=0` 时编译直接报错。

本记录说明代码、参数和验证方法。100 Vac 试验仍须核实主回路额定值、采样标定、UNIT 接线和 FPGA 位流；软件编译及主机测试不等于功率闭环已经完成硬件验收。

## 2. 公共 PI 结构与参考来源

公共模块为 `Source/01_ComLib/04_Pi/SstPi.h/.c`。参考代码为：

- `D:\Code\CCSv12\F28377S_Plat\Source\01_ComLib\04_Pid\Positional_PID.*`；
- `D:\Code\CCSv6\BidirecConverter\pi_reg.c`。

原 `CHB_PI_STATE` 只保存积分，调用者难以直接观察给定、误差、P/I 项和限幅结果。现使用 `SST_PI_CONTROLLER`，电压、d 轴电流、q 轴电流各有独立对象，字段包括给定、反馈、误差、增益、采样周期、前馈、比例项、积分增量、积分状态、未限幅输出、输出、饱和偏差、输出/积分上下限及运行标志。

单轴控制执行关系为：

```text
error(k)              = reference(k) - feedback(k)
proportional(k)       = Kp * error(k)
integral_increment    = Ki * Ts * error(k)
integral_candidate    = clamp(integral(k-1) + integral_increment, Imin, Imax)
output_unsaturated    = feedforward + proportional + integral
output                = clamp(output_unsaturated, Umin, Umax)
```

输出饱和且积分继续推向饱和方向时保持上一拍积分；能够使输出退出饱和时允许积分更新。d/q 两轴共用同一 PI 实现；三相保留动态圆形矢量限幅，单相只限最终 A 相输出并将饱和偏差反馈给两轴。电压外环接管前执行 `SstPi_Track()`，按当前 Idref 反算积分，实现连续接管。

`ki` 是连续域积分增益，执行时乘一次 `sample_period_s`。`pi_reg.c` 已乘过采样周期的离散 Ki 不能原样填入本工程。F28377S 位置式 PID 的 `Err/Up/Ui/OutPreSat/SatErr` 与本结构对应；本工程抗饱和使用条件积分，参考工程的 `Kc` 饱和回算未作为隐含功能引入。详细公式与差异见 [PI_IMPLEMENTATION_REVIEW.md](PI_IMPLEMENTATION_REVIEW.md)。

电压外环输出与积分使用本拍动态电流边界。电流 PI 积分限于实测母线电压边界的两倍；三相最终 dq 受圆限幅，单相最终 `phase_a` 受物理母线限幅。固定 PI 参数仅在初始化和运行入口配置，必要的运行输入/数值异常仍会封波。

## 3. 低压试验条件和参数口径

| 项目 | 低压配置 | 说明 |
|---|---:|---|
| 输入电压 | 100 V RMS AN | 正弦峰值约 141.4 V；不执行线电压到相电压转换 |
| 活动模块 | A 相 6 个 | A1/A2 位图分别为 `0x0007`，各 3 个；B/C 不投入 |
| 电感 | 6 mH | 等效串联电阻与总延迟尚需实测 |
| 每模块电容 | 3.2 mF | 用于估算电压外环及充电能量 |
| 目标模块均压 | 60 V/module | A 相串联电压约 360 V |
| 预充门槛 | 25 V/module | A 相模块均压连续达标 200 ms 后进入旁路延时 |
| 预充超时 | 10 s | 超时报告 `20103` |
| 旁路延时 | 200 ms | 保留原状态机动作顺序 |
| 输入已建立 | 50 V RMS | `SST_LV100_INPUT_PRESENT_V`，消费者为 AN 有效值 |
| PLL 最低电网幅值 | 100 V peak | 与输入已建立判据不同，使用电压矢量峰值 |
| PLL 连续锁定/超时 | 200 ms / 5 s | 运行中失锁继续按现有保护处理 |
| 调制绝对值上限 | 0.98 | 保持 FPGA Reference 的 0.02～1.98 范围 |
| 电压给定允许范围 | 20～150 V/module | DPRAM 扩展点校验；默认 60 V，快速过压门槛为 75 V |
| 电压环 Id 限幅 | 启动 5 A，运行上限 50 A | 低压配置按当前动态限流状态限制 dq 矢量，不直接等同输入有效值 |
| 快速过流 | 60 A，连续 5 TAD | 比较 A 相每拍标定瞬时电流绝对值 |
| 电流方向滞环 | ±0.1 A | 仅 LOW=1 的 A 相，严格越界更新 `HA_IG_Log_A`，不是保护阈值 |
| 快速过压 | 75 V，连续 5 TAD | 比较 A 相模块均压，非逐模块最大电压 |
| 原一级/二级过流 | 40 A / 45 A | 比较后台发布的 200 点电流有效值 |
| 原 HVAC 过压 | 已移除 | 旧参数字保留兼容，不再执行第二套直流过压判据 |

当前唯一直流过压参数为 `g_chb_dab_params.dc_overvoltage_trip_v`，默认 75 V，单相比较 A 相模块均压、三相比较各相模块均压最大值，不能代表相内单个模块最大值。`SST_LV100_MODULE_OV_V`、旧 OV 参数和 `HA_H_Voltage` 保留兼容，但 CHB 不再调用旧巡检及旧均压滤波过压判据。逐模块硬件保护取决于 UNIT 和现场位流。

## 4. 低压 PI 与启动给定

| 参数 | 低压起始值 | 说明 |
|---|---:|---|
| `voltage_kp` | 0.05 | 电压外环比例项，A/V |
| `voltage_ki` | 0.10 | 连续域积分项，A/(V·s) |
| `voltage_current_limit_a` | 50.0 | 运行 Id/Iq 合成矢量上限，A 峰值；启动阶段由动态限流状态收紧 |
| `startup_strategy` | `CHB_STARTUP_ID_RAMP` | 先投小 Id 斜坡，再移交电压环 |
| `startup_udc_offset_v` | 0.0 | 从解锁时实测均压起步，不套用高压 +20 V 偏置 |
| `startup_udc_step_v` | 0.001 | V/TAD，10 kHz 时约 10 V/s |
| `startup_id_reference_a` | 0.25 | 启动 Id 目标，A 峰值坐标量 |
| `startup_id_ramp_step_a` | 0.00025 | A/TAD，约 100 ms 升至 0.25 A |
| `startup_ramp_tad` | 2000 | 最长 0.2 s，达到目标可提前接管 |
| `current_kp` | -1.249166667 | 保留原反馈方向对应的符号和起始值 |
| `current_ki` | -16.66666667 | 连续域增益，不随输入电压按比例缩放 |
| `current_feedback_sign` | +1 | 仍须通过实测验证实际电流方向 |

这里的关键调整是去掉原高压启动时 `实测均压 +20 V`、每 TAD 增加 2 V 的快速给定。对于 60 V/module 的低压目标，20 V 偏置会直接触及最终目标，失去缓启动作用。新给定先从实测电压建立，固定小 Id 平滑上升，电压环接管后再按 10 V/s 更新参考。

参数估算采用六个电容共同充电的简化模型：

```text
E = 6 * C * Vmodule^2 / 2
Pavg ≈ Vgrid_peak * Id_peak / 2
dVmodule/dt ≈ Vgrid_peak * Id_peak / (2 * 6 * C * Vmodule)
```

在 60 V/module、C=3.2 mF、Vgrid_peak=141.4 V 时，`dVmodule/dt≈105.2×Id_peak V/s`。Id 为 0.25 A 时理想平均输入功率约 17.7 W，25 V 到 60 V 所需电容能量约 7.92 J，忽略损耗时约 0.45 s；实际过程还受预充、调制能力、限流和负载影响。

在 60 V 工作点按电流理想跟踪线性化，外环对象增益约 105.2 V/(A·s)，上述外环增益对应自然角频率约 3.24 rad/s（约 0.52 Hz）、阻尼比约 0.81。这只是保守起始估算。内环在忽略 R 和其他环节时比例交越量级约为 `|Kp|/(2πL)≈33 Hz`，等效电阻、dq 耦合、采样滤波、SOGI 和 PWM 延迟未知，不能据此认定已经整定稳定。

## 5. 启动流程和 CCS 验证方法

状态枚举为 `0 STOPPED`、`1 PRECHARGE`、`2 BYPASS_DELAY`、`3 WAIT_PLL`、`4 RUNNING`、`5 FAULT`。启动子阶段为 `0 IDLE`、`1 ID_RAMP_PHASE`、`2 VOLTAGE_HANDOVER`、`3 NORMAL`。

1. 首次下载保持 `run_enable=0`、`dab_enable=0`，检查 A 相 6 个模块在线配置、AN 电压/电流方向、每模块电压和硬件故障字。
2. 确认当前宏为 LOW=1、PHASE_A=1、BENCH_INPUT_SIM=0；真实主回路不得带着临时输入注入运行。无主回路控制板验证则按 [BENCH_CLOSED_LOOP_DEBUG.md](BENCH_CLOSED_LOOP_DEBUG.md) 显式选择注入模式。
3. 输入 100 V RMS 后确认 `PORT_RealTime_DATA.PORT_V_A` 为有效值，`HA_Port_Volt_A` 为瞬时量；`SVG_Udc_A_Ave` 为 A 相模块均压。
4. 在停机状态确认 `ac_mode=CHB_AC_MODE_CLOSED_LOOP`、目标 60 V/module、Iqref=0，置 `run_enable=1`。
5. 观察预充连续达标、旁路延时、PLL 连续锁定，之后才进入 RUNNING。进入 RUNNING 时由当前 TAD 的 FastStep 直接计算 PI、调制和参考值；启动状态在 Service 中准备完成，不再额外写一拍中性参考。
6. Idref 应从 0 缓慢爬升至 0.25 A，外环通过积分跟踪接管，之后电压参考从实测均压逐步升至 60 V。观察 Idref 不出现高压版本 10 A 级启动阶跃。
7. 对无功率注入模式，可在停机、预充、锁相等待点使用断点并修改注入变量验证状态边界。带真实主回路运行时不使用会冻结 DSP 而 FPGA 继续发波的普通断点；动态过程使用 CCS 实时观察、缓冲录波和示波器。
8. 清 `run_enable`、`dab_enable` 后验证 PWM 关闭；故障复位要求启动命令为 0。检查控制故障和五位故障记录一致，故障未排除时不得反复强行复位启动。

临时输入发生器仍保留历史高压默认 `300 V/module`、电网 `325 V peak`。若 LOW=1 同时开启 BENCH，必须在启动前将 `g_sst_bench_inputs.dc_a_v` 设为 19 V、`grid_peak_v` 设为 141.42136 V、`current_peak_a` 设为 0；预充等待期间人工升至至少 25 V 通过预充，进入运行后逐步升至 60 V 模拟充电。B/C 在单相模式被隔离。不得直接套用旧说明中 300→680 V 的流程，否则会触发唯一的 75 V 快速均压过压保护。输入发生器不含主回路模型，母线不会因 PI 自动升高。

启动 Id 步长在线编码为 `0.0001 A/TAD` 一格。源码 `0.00025` 经现有 AnyEyes/DPRAM/EEPROM 编码保存再加载会量化为 `0.0003`，约 83.3 ms 到 0.25 A；地址和缩放保持原协议，未为此修改。默认 `CHB_LOAD_EEPROM_PARAMETERS=0` 时启动使用源码值；显式 LOAD 后须按实际读回值确认，量化测试覆盖此半 LSB 误差。

模块电压不足、PLL 未锁定、超限或非法输入时，程序应停在对应等待/故障状态，而非绕过条件进入闭环。电压环起始值不保证预充电阻、旁路继电器和硬件保护的额定能力匹配，应依据实际硬件逐级加电。

## 6. DC_Log 与三相公共路径修复

`Chb_WriteNormalizedReferences()` 中电流方向和电压排序属于每拍调制数据。之前使用整字拼位并套在 `SST_PHASE_A_ONLY_TEST` 内，会使关闭单相宏的新开环/闭环路径跳过该处排序更新，同时整字赋值覆盖其他位。这不是单相算法必须的差异。

修正后 A/B/C 均使用原 `FpgaModulation.c` 的显式 `if/else` 位域逻辑：同相两卡 `IG_Orient` 一致，`DC_Order` 互补；逐位修改保留其他位。单相特有的 B/C 中性参考、AN/SOGI 运算和 B/C 封波仍保留在明确的单相条件内。三相运行还必须配置真实存在的 B/C 模块位图；仅关闭单相宏而保持 B/C mask 全零会被配置校验拒绝，不能作为三相可运行的配置。

实际调用链为 `FPGAIsr → ChbDabControl_FastStep → Chb_WriteNormalizedReferences`；原 `Write_ControlParameter_50Hz()` 没有活动调用点，不能以旧函数仍有三相代码来证明实时路径完整。本次回归直接从 FastStep 检查六张卡、四种方向/排序组合、非零布尔值、停止/运行、保留位及无关槽位，并验证重新加单相宏、整字清零及 B 相误读 A 相三种错误都能使测试失败。

同类审查还将此前仅限单相的基础约束推广到两种模式：运行及启动期间模式锁存、PWM/接触器提交前最终故障复核、首故障保留、有效相逐相母线门槛、正向 PLL 锁定及 200 TAD 连续失锁、参数/采样有限性检查，以及先快速保护再 PI/调制。进入 RUNNING 后，FastStep 按当前有效状态直接选择控制路径并计算参考值，不再插入额外的中性参考拍。公共助手使用 `Chb_GateBlocked`、`Chb_EnablePwm`、`Chb_DisablePwm` 命名，避免名称误导其适用范围。长调节器复位仍放在门控关中断区外。DAB 单独停机保留原 `DAB_StartMethod=0` 副作用。

低压复核还发现原 `HA_IG_Log_A` 仅在瞬时电流严格超过 ±1 A 才翻转，0.25 A 启动给定无法跨越该门槛，虽写入 DC_Log 仍可能保持旧排序方向。LOW=1 下 A 相改用 `SST_LV100_CURRENT_DIRECTION_HYST_A=0.1F`，大于 +0.1 A 置 1，小于 -0.1 A 置 0，边界及中间区保持；LOW=0 以及 B/C 的原 ±1 A 逻辑不变。501 `Avg_IG/10` 当前分辨率为 0.1 A，因此实际正负 2 count（±0.2 A）切换，正负 1 count 和 0 保持。这是小电流试验的排序方向判据，不改变 60 A 快速过流门槛；零点漂移/噪声及实际小电流测量能力仍需示波器和原始计数核验。

## 7. 保护、封波和故障码

快速软件保护继续经过 `ChbDabControl_ForceFault()`，原 HVAC 软件保护继续使用 `FaultCheck.C` 结果，FPGA/UNIT 硬件 FO/FB 保护路径保持原定义。`g_chb_dab_protection_enable` 只控制新增快速软件 OC/OV，不屏蔽原硬件故障。

| 控制故障枚举 | 五位上报码 | 含义 |
|---:|---:|---|
| 1 | 20103 | 预充超时 |
| 2 | 29102 | PLL 等待超时或单相/三相闭环运行连续失锁 |
| 3 | 29103 | 快速过流；原 HVAC OC 也映射到此报告码 |
| 4 | 29104 | 快速过压；原 HVAC OV 也映射到此报告码 |
| 5 | 19101 或原硬件细分 | 硬件故障汇总，具体依据已有 FO/FB/类型标志 |
| 6 | 29105 | 非法配置、启动配置和模式切换约束 |
| 8 | 29109 | 运行中采样、PI 数值或母线状态失效 |

原保护时限继续按 Timer2 的 10 ms 节拍和严格大于判据执行：设置 100/5 需要 101/6 次递增。电流有效值发布还包含 200 点积累及后台延迟，不能把原 RMS 保护与 5 TAD 瞬时保护按同一时间和电流口径比较。完整等级、细分码、首故障及历史语义见 [FAULT_CODES.md](FAULT_CODES.md)。

## 8. VOFA 关键观察量

UART2/RS485 选择 `SST_DEBUG_PROTOCOL_VOFA` 时使用 115200、8N1、JustFloat，每 20 ms 一帧，12 float 加 `00 00 80 7F` 帧尾，共 52 字节。

| 通道 | 变量 | 含义/单位 |
|---:|---|---|
| 0 | `state` | 控制状态枚举 |
| 1 | `fault_code` | 控制故障枚举，非五位上报码 |
| 2 | `dc_voltage_average_v` | A 相模块均压，V/module |
| 3 | `id_reference_a` | Idref，A |
| 4 | `id_feedback_a` | d 轴电流反馈，A |
| 5 | `iq_feedback_a` | q 轴电流反馈，A |
| 6 | `startup_phase` | 启动子阶段 |
| 7 | `startup_voltage_reference_v` | 正在执行的模块电压参考，V |
| 8 | `modulation_actual` | 调制绝对值 |
| 9 | `pll_locked` | 0/1 |
| 10 | `HA_Port_Volt_A` | 原标定/低通后的 AN 瞬时电压，V |
| 11 | `HA_Port_Current_A` | 501 标定瞬时 A 相电流，A |

数据在后台顺序采集，可能跨相邻 TAD，不视为同拍快照。50 Hz 上报用于状态和慢变量趋势，不能重建 50 Hz 工频波形或判断电流内环瞬态。VOFA 不接受启停/调参；仿真器模式通过 CCS 改命令。AnyEyes 与 VOFA 二选一，DPRAM 独立使能，既有 AnyEyes 地址保持不变。详见 [VOFA_LOW_VOLTAGE.md](VOFA_LOW_VOLTAGE.md)。

## 9. 编译、回归与硬件验收范围

低压单相、高压单相及高压三相构建示例（Debug/Release 须分别设置）：

```text
python tools/build.py --configuration Debug --variant low_voltage_100v --define SST_PHASE_A_ONLY_TEST=1 --define SST_LOW_VOLTAGE_100V_TEST=1
python tools/build.py --configuration Release --variant low_voltage_100v --define SST_PHASE_A_ONLY_TEST=1 --define SST_LOW_VOLTAGE_100V_TEST=1
python tools/build.py --configuration Debug --variant high_voltage_single --define SST_PHASE_A_ONLY_TEST=1 --define SST_LOW_VOLTAGE_100V_TEST=0
python tools/build.py --configuration Release --variant high_voltage_single --define SST_PHASE_A_ONLY_TEST=1 --define SST_LOW_VOLTAGE_100V_TEST=0
python tools/build.py --configuration Debug --variant high_voltage_three --define SST_PHASE_A_ONLY_TEST=0
python tools/build.py --configuration Release --variant high_voltage_three --define SST_PHASE_A_ONLY_TEST=0
python tools/control_compatibility_matrix.py
python tools/ccs_validate.py
python tools/run_checks.py
```

2026-10-19 最终源码冻结后验证如下。所有构建与主机总检查的固件输入 SHA256 均为 `180339b8c7523e665238e1a837f0906d8f06af8fc58d379ee75b04871501c649`，检查期间未变化。可随仓库查看的汇总证据为 [low_voltage_100vac_validation.json](evidence/low_voltage_100vac_validation.json)，包含 13 份 CLI 和 2 份 CCS 固件的实际 SHA256、宏组合、日志摘要、检查脚本及报告哈希。固件哈希在构建后采集，并在导出时重新核对。

| 验证项 | 最终结果 | 证据 |
|---|---|---|
| TI C6000 8.3.12 编译矩阵 | 13/13 通过；每组 91 源文件、0 错误、17 条既有警告 | Debug/Release × 三相高压/单相高压/单相低压 × AnyEyes+DPRAM远控/VOFA+仿真器；另加只关闭单相宏的默认三相 Debug；`build/control_compatibility_matrix.json` |
| CCS 实际工程验证 | 隔离工作区导入、Clean Debug、Clean Release 全部通过 | `build/ccs_validation/result.json`、`summary.json`；默认低压输出 `Debug/SST_AnyEyes.out` 和 `Release/SST_AnyEyes.out` |
| 三相配置/排序及公共保护 | 通过；六卡每拍排序及保留位正确 | 合法三相位图下验证闭环、各相母线、正向PLL、200 TAD失锁、模式锁存、FastStep 当前拍控制选择、故障竞态、NaN/Inf、先保护后PI；真实默认B/C位图仍需按现场修改 |
| PI/控制/VOFA/故障/DPRAM 等主机检查 | 20/20 通过 | `build/host_validation/result.json`；低压默认预充阻挡、Id斜坡、接管及电压参考斜率通过；方向滞环覆盖LOW1、LOW0单相、LOW0三相及边界 |
| 注释覆盖与结构/等价审计 | 通过，零未知源码变更 | 136 个自定义文件；2419 原实体→2386 当前实体，179 项精确登记变更；`build/refactor_equivalence.json`、`build/comment_inventory/verification.json` |
| 实时静态及 TI 隔离检查 | 静态 67/67；四个 ISR 宏0/1的指令、直接调用和栈一致 | `build/host_validation/realtime_audit.log`、`build/single_phase_isolation/result.json`；宏0采样/IO保持严格token比较；ISR调用闭包真实耗时未实测 |
| 目标板 10 kHz 时限和闭环 | 未进行硬件验收 | 按 `BOARD_ACCEPTANCE.md` 实测 |

每组的 17 条既有警告为：TI 库头 `1557-D` 9 条、原变量未使用 `179-D` 7 条、原变量赋值未使用 `552-D` 1 条；没有新增 PI/控制编译警告。CCS 默认 Debug 固件为 745428 字节，SHA256 `86f8bbe5388ab9ee31d4f901f2d180fb9d053ca4f0dabe6bf18962f2e6710e62`；Release 为 745776 字节，SHA256 `930ca75460bf25e2590c3f5a824175ad195746522f35d8937421d9dc0eb6070c`。这些产物均对应当前低压单相、VOFA、真实采样配置。

版本区分：高压 PI 检查点为 `pi-refactor-high-20261019`（`64aab66`，保留历史问题）；本次低压参数及单相/三相公共路径修复使用独立附注标签 `low-voltage-100vac-20261019`，后者同时保留 LOW=0 的高压参数分支。重新导出验证摘要使用 `python tools/export_low_voltage_validation.py`，它会拒绝过期源码指纹、失败检查或改变后的固件文件，不替代构建及测试。

TAD 中不加入串口打包、DPRAM 维护、EEPROM、动态内存或长时间关中断。每拍 PI/数学运算仍增加真实 CPU 工作，必须在 C6748 上测量 GPIO69 触发、ISR 入口延迟、完整执行时间、最大抖动、栈深和丢拍情况；不能将编译或主机测试当作 100 μs 最坏时间证明。硬件验收详见 [BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md)。
