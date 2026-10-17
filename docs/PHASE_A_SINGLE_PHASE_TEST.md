# A相 AN 采样与虚拟 dq 闭环测试说明

本说明对应 `D:/Code/SST/AnyEyesVersion` 当前版本，目标硬件为 C6748 + FPGA/UNIT。单相测试只改变控制算法选择和采样解释，不改变 FPGA 10 kHz 外部中断入口、应答顺序、`ControlTAD`、`TAD_PLL` 或原有故障上报接口。

## 构建开关

`Source/04_App/00_Config/ChbBuildConfig.h` 中的宏可以由 CCS 的 Compiler Predefined Symbols 覆盖。Debug 和 Release 必须分别设置并 Clean 后重新构建。

| 宏 | 值 | 作用 |
|---|---:|---|
| `SST_PHASE_A_ONLY_TEST` | 1 | 使用 AN 作为真实 alpha，SOGI 生成虚拟 beta，封锁 B/C |
| `SST_PHASE_A_ONLY_TEST` | 0 | 保留三相采样、Clarke/Park 和原三相控制 |
| `SST_BENCH_INPUT_SIM` | 1 | 无主回路时注入测试 AN、IA、A相母线；仅允许仿真器模式 |
| `SST_BENCH_INPUT_SIM` | 0 | 使用 FPGA 模拟插件和光纤模块的真实采样 |
| `SST_DEBUG_PROTOCOL` | 1/2 | AnyEyes / VOFA 二选一 |
| `SST_ENABLE_DPRAM` | 0/1 | 关闭 / 启用并口 DPRAM 维护 |
| `SST_SIMULATOR_MODE` | 1/0 | 仿真器直接控制、普通远程启动受限 / 允许所选通信源远控 |

当前默认配置为单相、真实采样、VOFA、仿真器模式、DPRAM关闭。现场改用 AnyEyes 或 DPRAM 时，必须在对应配置中明确覆盖宏，不能凭工程名称判断产物。

当前模块位图为 A1=`0x0007`、A2=`0x0007`，即 A1/A2 两张 501 卡各3个有效逻辑模块，共6个 A 相模块；B/C 为0。`350 V` 是单模块预充门槛，`680 V` 是单模块目标，`780 V` 是快速软件过压门槛。只有一套 UNIT 时，应把对应位图改成现场实际通道后重新编译。

## 采样到调制的路径

```text
GPIO69 TAD -> FPGAIsr -> Port_AD_Sample(0)
  -> HVAC_RealTimeParameter / SVG_Udc_Average
  -> ChbDabControl_FastStep
       -> 电压、电流 SOGI(alpha,beta)
       -> 原 PLL（q误差 + d>0 正向锁定）
       -> Udc 外环 -> Id 给定 -> dq 电流 PI
       -> dq 逆变换的 alpha -> A相 Reference -> FPGA501/UNIT
```

单相采样使用模拟插件 `AD[6]` 的 AN 通道，标定为 `0.0305185 * raw`，不叠加旧三相 PT 比和偏置。之后仍经过原 `LPF_5KHZ` 命名的滤波路径，当前递推为 `y[n]=0.9567*x[n-1]+0.04328*y[n-1]`；名称不代表已经实测了截止频率。`HA_Port_Volt_A` 与兼容缓存 `HA_Port_Volt_AB` 来自同一个低通结果；实时采样完成后两者应相等，二者不是一个瞬时值、一个有效值。有效值由 `HVAC_RealTimeParameter` 的窗口计算发布到 `PORT_RealTime_DATA.PORT_V_A`。

AN 的 beta 轴只存在于 `g_sst_single_phase_status`，不写入 B/C 物理采样。电压 SOGI 与电流 SOGI 共用估计频率，beta 相对 alpha 滞后90度；锁定后同相正弦进入正 d 轴。`ready` 只表示SOGI已建立100 ms，不代表PLL已经锁定。

闭环输出按实际母线归一化：

`Udc_total = NHA_module_A * SVG_Udc_A_Ave`

`M_limit = min(modulation_max, closed_loop_voltage_limit_factor)`

`normalized_reference_a = clamp(1 + alpha_command / Udc_total, 0.02, 1.98)`

实际写入501的是 `Reference = (unsigned short)(normalized_reference_a * HA_ReferenceDEF_A)`；归一化参考和寄存器计数不能直接比较。

母线低于预充门槛、采样无效、参数非有限或PLL连续失锁20 ms时，控制核封波并复用 `CHB_FAULT_PLL_TIMEOUT` 或 `CHB_FAULT_BAD_CONFIGURATION`。运行中修改开环/闭环模式会进入配置故障，必须停机复位后重新走启动链。

## 闭环启动顺序

1. 停机时写 `ac_mode`、目标电压和保护参数；确认 `run_enable=dab_enable=0`。
2. 置 `run_enable=1`。闭环依次经过 `PRECHARGE -> BYPASS_DELAY -> WAIT_PLL -> RUNNING`，预充按 A 相单模块均压检查。
3. `pll_locked=1` 且达到连续锁定时间后，CHB PWM 才在短临界区内提交使能。DAB 由 `dab_enable` 独立提交，初始占空比从350逐拍爬升至目标。
4. `startup_strategy=1` 时，`startup_phase` 依次为 Id 斜坡、移交、电压环；外环为Id标量限幅，电流环为dq矢量限幅，两环均有条件积分。
5. 停机或故障时先封锁 FPGA PWM，再清参考和PI；故障注入到使能事务期间不会被慢任务重新写成 RUNNING。

## 无主回路 CCS 验证

使用 `SST_PHASE_A_ONLY_TEST=1`、`SST_BENCH_INPUT_SIM=1`、`SST_SIMULATOR_MODE=1`、`SST_ENABLE_DPRAM=0` 构建。观察 `g_sst_bench_inputs`、`g_sst_single_phase_status`、`g_chb_dab_command`、`g_chb_dab_params`、`g_chb_dab_status` 和 `Module_PM_CTL`。

`Module_PM_CTL` 是共享控制缓存，A相运行时使能位可为1。核验B/C最终封波须分别观察 `FPGA_Interface[7].PM_CTL`、`[8].PM_CTL`、`[11].PM_CTL`、`[12].PM_CTL`（B1/B2/C1/C2）；其 `Bbits.SVG_EN` 和 `Bbits.DAB_EN` 都应为0，整字可用 `PM_CTL16` 查看，不能要求其他配置位一并为0。

下载后等待原上电初始化结束。在 CCS Expressions 中按下表操作，每一步修改后 Resume，让实际 TAD 和 10 ms 状态机推进，再观察结果：

| 步骤 | 写入变量 | 预期 |
|---|---|---|
| 初始设置 | `run_enable=0`、`dab_enable=0`、`ac_mode=1` | STOPPED，两桥封波 |
| 设测试电网 | `g_sst_bench_inputs.grid_peak_v=311`、`grid_frequency_hz=50`、`current_peak_a=0`、`dc_a_v=300` | 至少200 ms后正交量建立，PLL能够锁定 |
| 请求启动 | `g_chb_dab_command.run_enable=1` | PRECHARGE，均压300 V低于门槛，不能发波 |
| 模拟预充完成 | `g_sst_bench_inputs.dc_a_v=400` | 200 ms预充确认、200 ms旁路延时、200 ms慢速锁相确认后RUNNING；实际还受PLL建立影响 |
| 观察外环 | 分步调 `dc_a_v` 至600、680 V | Id给定及PI输出随误差变化；输入注入的母线不会被PWM自动充电 |
| 正常停机 | `g_chb_dab_command.run_enable=0`、`dab_enable=0` | STOPPED，参考回中点，两桥封波 |

表中未写全路径的命令成员属于 `g_chb_dab_command`，注入成员属于 `g_sst_bench_inputs`。除非专门验证DAB，始终保持 `dab_enable=0`；无需修改状态编号或PLL锁定标志来推进流程。

先保持停机运行至少200 ms，确认 SOGI `valid=1`、`ready=1`，alpha/beta 幅值和相位正确，PLL频率接近50 Hz，B/C参考为中点且控制字封锁。再设置 `ac_mode=1`、`run_enable=1`：母线低于350 V时应停在预充；提高 A 相母线到400 V以上后，才可进入旁路、锁相和运行。默认目标是680 V，输入注入不会替代真实 L/C 功率回路，不能据此证明功率级稳定。

运行中可将 IA 注入到小幅值，观察 `id_feedback_a`、`iq_feedback_a`、A相参考和功率遥测；B/C物理采样及PWM使能保持0，归一化参考保持中点1.0，FPGA参考寄存器保持对应 `HA_ReferenceDEF_B/C`。将母线设为800 V验证快速过压，将电流设为超过 `ac_overcurrent_trip_a` 验证快速过流；故障后清除启动命令，置 `fault_reset=1`，确认不会自动重启。

断点应放在 `ChbDabControl_Service` 状态切换或 `Chb_EnterState` 的一次性位置，避免在10 kHz `FPGAIsr` 每拍断点。仿真器暂停期间 FPGA 可能保持最后PWM，断点不能代替封波；使用停机命令或硬件急停确保安全。

## 验证命令和边界

```powershell
python -X utf8 tests/single_phase_control.py
python -X utf8 tests/control_regression.py
python -X utf8 tests/single_phase_io.py
python -X utf8 tools/run_checks.py
python tools/build.py --configuration Debug --variant phase_a_real --define SST_PHASE_A_ONLY_TEST=1 --define SST_BENCH_INPUT_SIM=0
python tools/build.py --configuration Release --variant phase_a_real --define SST_PHASE_A_ONLY_TEST=1 --define SST_BENCH_INPUT_SIM=0
python tools/build.py --configuration Debug --variant phase_a_bench --define SST_PHASE_A_ONLY_TEST=1 --define SST_BENCH_INPUT_SIM=1 --define SST_SIMULATOR_MODE=1 --define SST_ENABLE_DPRAM=0
```

主机回归验证算法边界、模式互锁、非有限参数、实测母线归一化、PI抗饱和、反相PLL和PWM使能竞态。TI构建验证源文件和链接布局；仍需在板上用GPIO69及ISR尾部测量最坏执行时间，确认满负载下每次完成时间小于100 µs并留有余量。真实采样极性、L/C参数、501输出方向及硬件保护时序必须用低压功率级继续验收。
