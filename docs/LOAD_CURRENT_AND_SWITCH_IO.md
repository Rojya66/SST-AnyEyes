# 50 A 峰值带载限流与开关量板控制修改记录

日期：2026-10-19。工程：`D:/Code/SST/AnyEyesVersion`。本轮基于提交 `248cff0`（标签 `low-voltage-100vac-20261019`）。本轮标签为 `load-current-switch-io-20261019`；构建和主机检查证据另存 `evidence/load_current_switch_validation.json`，不覆盖历史证据。

## 1. 运行配置与试验边界

用户确定最大输入电流暂定 **50 A 峰值**。在正弦、功率因数为 1 时，约为 35.36 A RMS，100 Vac 输入约 3.54 kW。该数值是电流给定上限，不是开机后立即给定 50 A，也不代表硬件额定值或已验证的连续功率。

工作期间保留了现场已有选择：`SST_PHASE_A_ONLY_TEST=1`、`SST_LOW_VOLTAGE_100V_TEST=0`、`SST_BENCH_INPUT_SIM=1`、VOFA、460800 bit/s、仿真器模式、DPRAM 关闭。因此**当前直接编译的默认产物仍是高压参数加临时输入注入，不能用于 100 Vac 实物带载**。高压参数的预充门槛保留现场的 280 V/模块；本轮的低压组仍为 20 V/模块。此前 README 的低压默认配置已同步更正。

100 Vac 实物试验必须明确设置：

```c
#define SST_PHASE_A_ONLY_TEST       1U
#define SST_LOW_VOLTAGE_100V_TEST    1U
#define SST_BENCH_INPUT_SIM          0U
#define SST_SIMULATOR_MODE          1U
#define SST_ENABLE_DPRAM            0U
```

先用仿真器控制，核对 A 相真实采样和 6 个实际模块的位图；EEPROM 上电加载保持关闭，避免旧参数覆盖。仅改上述宏不会自动替现场核验模块数量或接线。低压限流逻辑只在 LOW=1 下生效，高压单相及三相保持原电流限幅算法。

## 2. 为什么将启动限流与运行限流分开

原 1 A 用于空载启动试验，不能满足带载需求。直接从启动就放宽至 50 A，会在预充后母线误差较大时允许电压环要求大电流。因此保留小电流启动，待母线实际建立后再增加允许带载能力。

配置集中在 `Source/04_App/00_Config/ChbBuildConfig.h`：

| 配置 | 默认值 | 含义 |
|---|---:|---|
| `SST_LV100_START_CURRENT_LIMIT_A` | 1 A | 母线建立前 dq 给定矢量幅值上限 |
| `SST_LV100_RUN_CURRENT_LIMIT_A` | 50 A | 低压运行给定幅值最大值 |
| `SST_LV100_CURRENT_LIMIT_SLEW_A_S` | 10 A/s | 启动完成后放宽限幅的速率 |
| `SST_LV100_READY_ERROR_V` | 1 V/模块 | 实测均压与最终给定的允许差值 |
| `SST_LV100_READY_TAD` | 2000 | 连续确认 200 ms，10 kHz |
| `SST_LV100_FAST_OC_A` | 60 A | 低压快速瞬时过流门槛 |

实际运行上限为 `g_chb_dab_params.voltage_current_limit_a`，默认 50 A；可先调低该参数进行逐级试验。启动仍按原策略建立母线。进入 `CHB_STARTUP_NORMAL`，电压参考已经到达最终给定，且实测均压连续 200 ms 位于目标 ±1 V 内后，`load_ready=1`。随后 `current_limit_active_a` 从 1 A 以 10 A/s 增加，约 4.9 s 达到 50 A。

这只是放宽限幅，实际 Idref 仍由电压 PI 决定。没有负载需求时不会自动输出 50 A。Id、Iq 共用预算：`Id_limit=sqrt(I_limit²-Iq_ref²)`；例如 Iq=30 A、总预算 50 A 时，Id 最大 40 A。电压 PI 的输出、积分和交接对齐均使用该动态预算；内环实际 Iq 给定也受限。运行中降低上限立即收紧，提高上限仍按斜坡。达标状态在本次运行锁存，加载后的暂降不会突然将限幅收回 1 A；停机、故障后的控制器复位会清除达标标志并恢复启动限幅。

## 3. 保护配套与电容纹波

低压配置下：快速过流 60 A，连续 5 TAD 确认，名义窗口 0.5 ms；旧软件 RMS 一级/二级过流分别为 40/45 A，原确认计数 100/5 不变。旧保护采用严格大于计数判断，不能把它们描述为精确 1 s/50 ms。低压参数检查要求运行上限不超过 50 A，快速过流门槛大于运行上限且不超过 60 A；NaN/非法配置拒绝启动或运行。

42 V/模块的均压过压门槛保持不变，**没有为支持 50 A 而提高过压阈值**。它检查相模块均压或原合成滤波均压，不是所有子模块最大电压保护。

用户给定每模块 3.2 mF、6 模块、目标 35 V/模块，平均储能约 11.76 J。单相 50 Hz、3.54 kW 时，忽略损耗和电感能量，100 Hz 功率脉动所需储能幅值约 `P/(2ω)=5.63 J`。按理想均分估算，每模块电压约在 25.3～42.6 V 之间，已经可能触及 42 V 门槛。因此**软件允许 50 A 不等于此电容与电压设定允许连续满载**。6 mH 电感、实际功率分配、控制器带宽和滤波还会改变结果；该估算不代替电路仿真或现场整定。先空载建立母线，再逐级加载，观察各模块最高/最低电压、均压纹波、调制裕量、电流及故障；不要通过提高保护值掩盖纹波。

## 4. 开关量地址、位定义与宏

参考 `D:/Code/SST/C6748Test` 的 HVAC 接触器操作，保留非阻塞状态机，不迁入阻塞等待或旧 485 维护通信。IO 插件是 `IO_PluginUnit_2`，`FPGA_Interface[0]`。以下为 **DSP 字节地址**，用 TI C6000 编译器 `offsetof` 静态断言核对，不用主机编译器位域布局推断。

| 地址 | 权限 | 定义 |
|---|---|---|
| `0x60000040` | W | DOUT，16 位完整输出字；bit4 进线分闸脉冲，bit5 进线合闸脉冲，bit7 预充旁路保持命令 |
| `0x600001C0` | R | 实时 DIN；默认 bit3 为进线已合，低有效；bit6 为旁路已合，高有效 |
| `0x600001C2` | R | 锁存 DIN；本轮闭环联锁读取实时 DIN，不使用此锁存字 |

其它 DOUT 位保持现有 `Port_DOUT` 镜像，未被三路线圈极性转换修改。以上 DI 默认来自旧 HVAC 判据，**现场有效电平尚未确定**，必须验证后配置。

| 宏 | 默认 | 用途 |
|---|---:|---|
| `SST_ENABLE_SWITCH_OUTPUT` | 0 | 1 才允许 `Port_IO_Out()` 写 DOUT；0 时包括初始化在内均不经此入口写 DOUT |
| `SST_ENABLE_SWITCH_FEEDBACK` | 0 | 1 启用实时反馈确认及超时联锁，必须同时启用输出 |
| `SST_SWITCH_CLOSE_ACTIVE_HIGH` | 1 | 合闸脉冲物理高有效；0 反相 |
| `SST_SWITCH_OPEN_ACTIVE_HIGH` | 1 | 分闸脉冲物理高有效；0 反相 |
| `SST_SWITCH_BYPASS_ACTIVE_HIGH` | 1 | 旁路保持输出物理高有效；0 反相 |
| `SST_SWITCH_AC_CLOSED_MASK` | `0x0008` | 进线已合反馈位 |
| `SST_SWITCH_AC_CLOSED_HIGH` | 0 | 进线已合的原始输入电平 |
| `SST_SWITCH_BYPASS_CLOSED_MASK` | `0x0040` | 旁路已合反馈位 |
| `SST_SWITCH_BYPASS_CLOSED_HIGH` | 1 | 旁路已合的原始输入电平 |
| `SST_SWITCH_FEEDBACK_STABLE_10MS` | 3 | 连续反馈确认 3 拍 |
| `SST_SWITCH_ACTION_TIMEOUT_10MS` | 100 | 非运行态动作最长等待 1 s |

输出镜像始终按 1=动作理解，物理极性只在写板时转换，采用显式 if/else。使能输出但关闭反馈时，仅按命令和电压/延时推进；`ac_input_closed`、`precharge_bypassed` 仍是命令镜像，不能证明接触器实际到位。输出关闭意味着不写寄存器，不能据此判断外部接触器已断开。

## 5. 启停顺序与故障

1. STOPPED：PWM 关闭，旁路撤销，分闸脉冲；启用反馈时，实际进线和旁路须已断并连续确认后才接受启动。
2. PRECHARGE：旁路保持断开，发进线合闸脉冲，原预充电压连续达标条件保留。启用反馈后先确认进线实际已合、旁路实际断开，再推进预充判断。
3. BYPASS_DELAY：旁路保持吸合；先确认旁路反馈，再满足原机械延时。
4. WAIT_PLL：满足原连续锁相、母线条件后进入 RUNNING 发波。
5. STOP/FAULT：先执行既有封 PWM，再撤销旁路、发进线分闸脉冲。合/分闸脉冲默认 500 ms，到期两路均清零；旁路为保持输出。
6. 开环模式保持原独立脉冲调试方式，不自动接管外部接触器。

反馈打开时，每 10 ms 在后台读取一次 DIN。运行中连续 3 拍与命令不符即故障；机械动作/停止阶段最长等待 100 拍；反馈抖动未连续到位也不能无限重启等待期限。故障复位前要撤销 AC/DAB 命令，两桥停波，且实际进线、旁路已断并稳定。新增内部故障 `CHB_FAULT_SWITCH_FEEDBACK=7`，沿既有故障报告与历史机制上报 **29108，二级**；不占用既有相序 29106 或频率 29107 编码，AnyEyes/DPRAM 原地址不变。

## 6. CCS 和 VOFA 验证步骤

先在外部线圈断开的条件下核对原始输入与三路输出电平，再接入执行器。建议分三个阶段编译，每次核对下载产物：

| 阶段 | OUTPUT | FEEDBACK | 目的 |
|---|---:|---:|---|
| 软件流程/输入注入 | 0 | 0 | 观察命令镜像、预充和限流流程，不输出接触器命令 |
| 输出极性核对 | 1 | 0 | 核对 bit4/5/7、脉冲宽度和旁路保持，反馈尚不参与流程 |
| 实际闭环联锁 | 1 | 1 | 使用正确 DI 位和极性确认真实开关到位 |

CCS Expressions 加入 `g_chb_dab_status` 的这些字段：`state`、`startup_phase`、`dc_voltage_average_v`、`startup_voltage_reference_v`、`id_reference_a`、`iq_feedback_a`、`load_ready_count`、`load_ready`、`current_limit_active_a`、`switch_din_raw`、`ac_input_feedback`、`bypass_feedback`、`switch_stable_count`、`switch_mismatch_count`、`fault_code`；同时观察 `Port_DOUT`、`g_chb_dab_params.voltage_current_limit_a`、`g_chb_dab_command`。

无功率回路时可打开临时注入验证软件路径，按 `BENCH_CLOSED_LOOP_DEBUG.md` 操作注入对象；不要在真实采样模式中反复改写会被下一 TAD 覆盖的反馈。断点仅用于不带功率的逻辑验证：在 `Chb_EnterState()`、`ChbDabControl_ForceFault()` 及设置 `load_ready=1` 的位置暂停，核对状态、命令与限幅。DSP 暂停时 FPGA 不一定同步停机，实物带电试验应使用不停机观测和示波器。

验证母线未达最终目标时限流保持 1 A；连续达标后约 4.9 s 放宽至 50 A；Iq=30 A 时 Id 不超过 40 A；降低运行限值立即生效；停止再启动恢复 1 A。先从小负载逐级增加，只有 `load_ready=1` 且 `current_limit_active_a` 已达到本级试验需求才增加负载，不能一开机就接入满载。

VOFA 保留 12 通道，关键状态/Udc/Idref 仍可见；CH9 保留现场修改的 PLL 相角（rad），锁相标志在 CCS 观察。当前为 JustFloat、460800、8N1、周期 10 ms，100 帧/s。单帧 52 字节约 1.13 ms，平均占用 52 kbit/s。修复 `TaskSlice` 把 1 ms 计数传给 10 ms 接口的问题，初始化与服务统一 10 ms 时基；没有新增 ISR Tick。新增限流/开关反馈字段暂通过 CCS 观察，未重排 AnyEyes 参数、波形、状态地址。

## 7. 验证范围

`tests/switch_io.py` 编译执行真实输出函数和控制状态机，覆盖高压三相/低压单相、输出关闭/反馈关闭/反馈打开、三路线圈全部 8 种极性；每配置穷举 65536 个输出字检查其它位保持，测试预充/旁路/发波/停机、反馈丢失、拒动、抖动超时和故障复位。TI 静态断言核对上述寄存器偏移。`common_control_safety` 覆盖动态限流、矢量预算、在线降限、重启复位以及高压兼容。

最终证据应包含 17 个 CLI 编译链接组合、CCS 隔离导入与 Debug/Release Clean 构建、21 项主机检查及统一源码指纹。原 FPGA ISR 路由和中断入口保持，开关量读取/联锁在后台，不新增忙等待；低压电流预算计算在原控制路径内增加，**未以主机测试代替板上 WCET 验收**。仍需实测 10 kHz 最坏执行时间、栈余量、板级电平、门极输出和真实闭环稳定性，方法见 `BOARD_ACCEPTANCE.md`。

本轮实际结果：上述 17 组 CLI、CCS 导入及两配置 Clean 构建、21 项检查全部通过，源码在验证期间保持不变。统一固件输入 SHA256 为 `13a8941b5bccd0aa9e3b30784231e063c98c79a70a6241fbc0358b732ca30638`。CLI 每组 91 个源码单元，编译/链接零错误，各有 17 条警告，主要来自 TI 设备头文件；不将有警告的构建称为零警告。固定证据包含产物、日志及输入哈希，可用 `python tools/export_load_switch_validation.py` 核对当前构建报告后重新导出。
