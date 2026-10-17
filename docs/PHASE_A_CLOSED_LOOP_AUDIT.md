# A 相闭环启动与持续运行审计

审计日期：2026-10-17；工程：`D:/Code/SST/AnyEyesVersion`。本文回答“采样正确并开启闭环后，软件是否按预期推进、哪些异常会阻止发波”。软件审计不能承诺程序在任意硬件与参数下无bug，也不能代替真实功率环稳定性验证。

## 1. 当前范围和参数

当前目标为 C6748 + 202采样板 + `501_compatible_build` + UNIT。宏默认单相=1、实际采样（注入=0）、仿真器=1、DPRAM=0、VOFA。A1/A2各 `0x0007`，A相共6逻辑模块；现场板数/编号必须匹配，B/C不参与控制。

| 参数 | 当前值 | 解释 |
|---|---:|---|
| `sample_period_s` | 0.0001 s | 10 kHz外部TAD；不能通过改变量改变中断周期 |
| `precharge_voltage_v` | 350 V | A相模块均压连续达标，非单模块最小值 |
| `precharge_stable_10ms` | 20 | 达标确认200 ms |
| `precharge_timeout_10ms` | 1000 | 预充超时10 s |
| `bypass_delay_10ms` | 20 | 旁路稳定200 ms |
| `pll_min_grid_peak_v` | 100 V | SOGI alpha/beta矢量幅值下限，非RMS |
| `pll_lock_error_pu` | 0.02 | 归一化q误差门限，同时要求d正向 |
| `pll_lock_10ms` | 20 | 慢状态机连续锁定确认200 ms |
| `pll_timeout_10ms` | 500 | WAIT_PLL超时5 s |
| `dc_voltage_ref_v` | 680 V/模块 | 电压外环目标；共6模块时总尺度4080 V |
| `dc_overvoltage_trip_v` | 780 V/模块 | A相均压快速过压阈值，不能排除个别模块过压 |
| `ac_overcurrent_trip_a` | 120 A | A相物理电流快速过流绝对值阈值 |
| `protection_confirm_samples` | 5 | 快速OC/OV连续确认500 us |
| 运行PLL异常确认 | 200 TAD | 20 ms，从算法检测失锁/失压后计数 |

默认开机仍为开环模式、run/dab=0。输入220 Vrms不保证达到350 V/模块预充门槛，尤其当前6模块配置不能当作单模块演示。不得为了跳过预充而直接写状态机或伪造实际母线。

## 2. 调用链和状态路径

```text
FPGA TAD / GPIO69
  -> FPGAIsr
  -> Port_AD_Sample(0)
       signed AD[6] -> 0.0305185*raw -> 原电压LPF -> AN
       501 Avg_IG/10 -> IA
  -> HVAC_RealTimeParameter()：200点RMS及功率滤波
  -> SVG_Udc_Average()：仅A相均压及排序方向
  -> ChbDabControl_FastStep()
       数值/配置检查 -> 电压/电流SOGI -> PLL
       非RUNNING：中点参考，保持封波
       RUNNING：母线/模式/失锁检查 -> 快速软件OC/OV -> Udc/Id -> dqPI -> 单相alpha
       -> 实测母线限幅/归一化 -> Reference和互补DC_Log
       -> DAB斜坡
```

主循环 `ChbDabControl_Service()` 以10 ms节拍推进：

```text
STOPPED
  配置有效且run=1 -> PRECHARGE（进线合，旁路断，PWM封锁）
  均压连续>=350 V -> BYPASS_DELAY（旁路合）
  延时完成 -> WAIT_PLL
  正向PLL连续确认 -> RUNNING
  最终PWM使能前临界区复查 -> CHB发波；DAB按独立命令
任何锁存故障 -> FAULT -> 恢复合法条件、run/dab撤销、fault_reset -> STOPPED
```

SOGI在停机期间持续跟踪，`ready=1`要求1000拍有效输入；它不代表PLL已锁定。默认兼容启动策略0在进入RUNNING后建立临时Udc参考再靠近目标；策略1先走Id斜坡、HANDOVER、NORMAL。原PI增益和相位补偿不改，只修正限幅、积分及运行约束。

## 3. 本轮发现及修复规则

| 检查点 | 原风险 | 单相修复规则 |
|---|---|---|
| PLL方向 | 相差180度也可能q=0 | q误差合格且d>0才计锁定 |
| 运行失网/失锁 | PLL自由转角，控制继续发波 | 连续200TAD异常报控制故障2并封波 |
| 在线切模式 | 开环RUNNING直接改闭环可绕过预充/PLL | 活动态模式不一致报故障6，要求停机重新启动 |
| 非法浮点/参数 | NaN绕过比较，Inf可能卡住角度循环 | 单相快速路径范围/有限性检查，故障6 |
| 母线不足 | PWM使能后缺乏可用电压 | 检查运行下限，低于预充阈值报故障6 |
| 标幺分母 | 用目标母线压低尚未充满时的前馈 | 单相用实测A相均压乘模块数 |
| 外环积分 | Id已限幅但积分仍增长 | 条件积分和积分边界 |
| 内环积分 | 最终调制饱和但dq积分继续增长 | 按实际母线、min(modulation_max,factor)矢量限幅及条件积分 |
| 501排序 | 快速控制路径遗漏/覆盖DC_Log | 依据当前IG/DC方向更新组1，组2与其排序位互补 |
| 故障竞态 | 主循环使能前被ISR判故障，恢复后仍可能解锁 | 在短临界区最终复查故障、bit15、模式与run/dab |

故障2同时用于启动PLL超时和运行连续失锁；根据当时 `state`、PLL波形和历史记录区分触发原因。故障6表示非法配置/运行条件，不代表所有这些异常都是传感器损坏。现有五位故障上报码和 `FaultCheck.C` 继续保留，参阅 [FAULT_CODES.md](FAULT_CODES.md)。

## 4. 采样核对

当前 `ADC_Sample.C` 在单相分支只使用 `0.0305185*raw`，无偏置和 `HA_PT_Ratio`，然后执行 `y[n]=0.9567*x[n-1]+0.04328*y[n-1]`。`HA_Port_Volt_AB`和`HA_Port_Volt_A`取同一个低通输出；`HVAC_RealTimeParameter()`不再次修改瞬时电压，只累加200点平方和滤波P/Q。

CCS应同时观察202原始AD[6]、两种瞬时电压和RMS。瞬时量与RMS本来不同；实时与锁存量的时刻也不同。输入注入宏1绕过实际202链路，只能验证软件状态流程，不能验证传感器/AD标定。

## 5. 可复现CCS验证

所有操作先在无主回路且隔离驱动/开关执行机构的台架完成。Expressions包括：`g_sst_bench_inputs`（注入构建）、`g_sst_single_phase_status`、`g_chb_dab_command`、`g_chb_dab_params`、`g_chb_dab_status`、`SVG_Udc_A_Ave`、A1/A2 `DC_Log`、B/C `PM_CTL`、旧故障字及首故障。

| 操作 | 预期 |
|---|---|
| 上电、run/dab=0 | STOPPED，CHB/DAB封锁，不因PLL建立而自动启动 |
| 正常AN，保持至少100 ms以上 | SOGI valid/ready，PLL最终正向锁定；反相错误不得假锁 |
| 闭环run=1，均压300 V | 保持PRECHARGE，10 s未达标故障1 |
| 均压400 V、正常AN | 预充确认、旁路延时、WAIT_PLL确认后RUNNING |
| WAIT_PLL中无有效AN | 不发PWM，5 s后故障2 |
| RUNNING将AN置零 | 经过检测延迟及连续20 ms确认后故障2，全桥封波 |
| RUNNING切ac_mode | 故障6，不能直接用新模式继续发波 |
| RUNNING使母线低于350 V | 故障6，禁止继续归一化/最终使能 |
| 运行中参数NaN/Inf | 故障6，无无限循环，无非法Reference转换 |
| 均压800 V或IA峰值200 A | 快速OV/OC或既有保护首先触发，核对首故障和上报 |
| 长时电压/调制饱和后恢复输入 | PI积分有界，误差回落后能够退出饱和 |
| 状态切换/保护触发同时启停DAB | 故障优先，最终控制字不会被后续使能覆盖 |
| 任意正常/异常阶段 | B/C参考中点、SVG_EN和DAB_EN始终0；A1/A2排序位互补 |

临时改变量需保持运行一段时间让状态机处理，暂停不会推进TAD计数。每次故障先撤销run/dab、恢复输入和参数，再设fault_reset=1，确认不会自动重启。建议断点放在状态入口、WAIT_PLL转RUNNING、ForceFault；DSP断点不冻结外部FPGA，不得把断点当作功率封波。

## 6. 主机测试、构建与证据

```powershell
python tests/single_phase_control.py
python tests/single_phase_io.py
python tests/control_regression.py
python tools/run_checks.py
python tools/build.py --variant phase_a_real --define SST_PHASE_A_ONLY_TEST=1 --define SST_BENCH_INPUT_SIM=0 --define SST_SIMULATOR_MODE=1 --define SST_ENABLE_DPRAM=0 --define SST_DEBUG_PROTOCOL=2
python tools/build.py --configuration Release --variant phase_a_real --define SST_PHASE_A_ONLY_TEST=1 --define SST_BENCH_INPUT_SIM=0 --define SST_SIMULATOR_MODE=1 --define SST_ENABLE_DPRAM=0 --define SST_DEBUG_PROTOCOL=2
```

2026-10-17最终源码构建结果如下，均使用 TI C6000 8.3.12。源码及工程输入指纹为 `f739601cd750cd891222d6a4373d3349614d01337be5d0740ca5c6a2b3aa8202`，构建期间未变化；默认宏采用本文第1节的真实AN配置。

| 验证 | 结果 | 本地产物/记录 |
|---|---|---|
| 命令行 Debug 全量构建 | 90编译单元，0编译/链接错误，17警告 | `build/Debug/phase_a_flow_audit_20261017/` |
| 命令行 Release 全量构建 | 90编译单元，0编译/链接错误，17警告 | `build/Release/phase_a_flow_audit_20261017/` |
| CCS临时工作区导入、Clean Debug/Release | 三步全部exit 0 | `build/ccs_validation/result.json`及对应日志；产物为`Debug/SST_AnyEyes.out`、`Release/SST_AnyEyes.out` |

两份命令行目录均含 `SST_AnyEyes.out`、`.map`、`build.log` 和 `build_result.json`。17条警告涉及SDK头文件宏续行空白、DPRAM/采样/故障旧分支未用变量，当前控制核无编译警告。链接未溢出，`.stack`仍为 `0x800` 字节；这只是分配容量，不是栈峰值实测。

最终测试结果与上述固件指纹一致，完整记录及产物SHA256见 [可携带验证证据](evidence/phase_a_closed_loop_validation.json)。源码及测试提交为 `4bb170a`，修改前版本为 `b265bf7`，均位于 `refactor/f28377s-framework` 分支。

| 检查 | 本轮结果 |
|---|---|
| `tools/run_checks.py` | 16/16通过，覆盖控制、保护、通信、DPRAM、参数存储、框架及注释 |
| 单相控制 | 14组通过；含45/50/55/60 Hz SOGI、启动/移交、反相拒锁、199/200拍失锁边界、28个浮点字段的NaN/Inf、PI抗饱和及最终使能竞态 |
| 三相兼容 | 8个场景与原程序逐拍一致，轨迹SHA256为 `c34cd06dda3d78a2ae07e3fc333a5bd67e614b0613b6ccae31967b2eb2c50a40` |
| AN及B/C隔离 | 单相/三相宏两分支采样、遥测和最终门控通过 |
| 静态实时路径 | 38/38通过；单相临界区仅允许既定9段有界状态/寄存器操作，控制器复位及通信/EEPROM不进入临界区 |
| TI宏隔离及ISR汇编 | 宏0控制按两处精确模式别名展开后与基线一致；宏0/1四个ISR的入口指令、直接调用和自身栈信息一致；不包含被调控制函数总耗时证明 |
| 源码格式及注释 | 134文件通过，拥挤多语句行和代码超120列均为0；条件编译尾部函数及中文UTF-8偏移识别亦已回归验证 |

主机测试使用生产函数及桩模拟寄存器，可以覆盖软件分支和异常边界；没有实际功率回路模型的输入注入只能证明控制流程可推进。即使简化L-R-C数值模型通过，也不意味着现场稳定。

## 7. 必须现场补充的验收

- 按实际模块位图、传感器比例、CT方向、AN极性和调制正负方向核对硬件。UNIT的左右桥臂约定和501参考方向必须与现有接线分析一致。
- 记录真实电感L、母线电容C、串联电阻、负载、采样/执行延迟；使用限流低压源验证闭环电流方向，之后逐步验证目标电压、100Hz脉动和负载阶跃。
- 原电流/电压PI和0.1085/0.16 rad补偿是继承值，单相SOGI改变动态，需依据实际L/C验证带宽和裕量，不能据静态审计承诺功率稳定。
- 350/680/780 V均为每模块均压尺度。原始各模块DC极值、通信缺失、NoInterlock位流的封波边界仍需单独核验。
- 按 [BOARD_ACCEPTANCE.md](BOARD_ACCEPTANCE.md) 同时测GPIO69 TAD、GPIO43/45、PWM和故障封波。完整ISR包含入口延迟、序言、控制运算、写寄存器和尾部，最坏完成时间必须小于100 us且有余量；覆盖锁相、闭环、DAB、通信满载及故障。

入口和ACK保持、编译通过、主机测试通过均不等于10kHz最坏执行时间已实测，也不代表真实主回路已经完成闭环验收。
