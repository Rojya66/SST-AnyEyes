# SST PI 实现审查说明

## 1. 审查范围

本说明对应 `Source/01_ComLib/04_Pi/SstPi.h`、`SstPi.c`，以及
`Source/04_App/04_Control/ChbDabControl.c` 中的电压外环和 d/q 电流内环接入。
审查基准为：

* `CCSv12/F28377S_Plat/Source/01_ComLib/04_Pid/Positional_PID.*` 的位置式 PID；
* `CCSv6/BidirecConverter/pi_reg.c` 的离散增量式 PI。

## 2. 当前控制器结构

每个环路使用一个独立的 `SST_PI_CONTROLLER` 对象。对象保存给定、反馈、误差、
比例项、积分增量、积分状态、前馈、未限幅输出、限幅输出和饱和偏差，同时保存
输出限幅、积分限幅、采样周期、增益和运行标志。因此电压环、d 轴电流环、q 轴
电流环不会共享积分历史；原先只有 `integral` 的 `CHB_PI_STATE` 已不再用于当前
控制路径。

当前执行公式为：

```text
error(k)             = reference(k) - feedback(k)
proportional(k)      = Kp * error(k)
integral_increment   = Ki * Ts * error(k)
integral_candidate   = clamp(integral(k-1) + integral_increment, Imin, Imax)
output_unsaturated   = feedforward + proportional + integral
output               = clamp(output_unsaturated, Umin, Umax)
```

当输出饱和且积分增量仍把输出推向饱和方向时，`anti_windup_enable` 会保持上一拍
积分；当输出向内恢复时，积分继续更新。三相入口先执行 d/q 各自矩形限幅，再执行
动态圆限幅；动态电压上限为零时两轴输出为零并冻结积分。
单相只限最终物理 A 相电压，虚拟 beta 不再缩小 dq 幅值。最终饱和偏差按
`d: cos(theta)`、`q: -sin(theta)` 投影，撤销继续推向饱和的本拍积分；反向积分保留。

## 3. 与 F28377S_Plat 的关系

F28377S 的 `POSITONAL_PID::Run()` 也保存 `Err/Up/Ui/OutPreSat/SatErr`，并执行
位置式 PI/PID。它的积分更新形式为：

```text
Ui(k) = Ui(k-1) + Ki * Up(k) + Kc * SatErr(k)
```

其中 `Kc` 是饱和反馈系数，且 `Up = Kp * Err`。SST 当前实现选择条件积分保持，
没有引入 F28377S 的 `Kc` 回算项；这是抗积分饱和策略差异，不是遗漏字段。若以后
需要严格复现 F28377S 的回算动态，应新增显式 `tracking_gain`/`Kc` 配置和单独的
回算策略，不能只把 `Ki` 数值替换过去。

## 4. 与 BidirecConverter `pi_reg.c` 的关系

`pi_reg.c` 保存上一拍误差和输出，并按差分式计算：

```text
Err      = Ref - Fdb
OutPreSat = Out1 + Kp * (Err - Err1) + Ki * Err
Out      = clamp(OutPreSat, OutMin, OutMax)
Out1     = Out
Err1     = Err
```

该实现的 `Ki` 已经包含采样周期（文件注释为 `Ki=Ki'*T`），而 SST 的 `SstPi_Configure`
接收连续域 `ki`，运行时只乘一次 `sample_period_s`。因此不能直接把 `pi_reg.c` 的
`Ki`、或旧的 `Num_0/Num_1` 系数原样填入 `g_chb_dab_params.voltage_ki` 和
`current_ki`；必须先根据目标离散模型换算，并确认误差方向和采样周期。

## 5. 当前接入点核对

* `Chb_ConfigureControllers()` 在初始化及 RUNNING 入口配置固定增益和采样周期。
  停机修改 PI 后，下一次启动重新应用；运行周期不扫描整组配置。
* `Chb_VoltagePiLimited()` 只更新本拍动态电流边界，低压启动限流与 Iq 预算仍生效。
* `Chb_CurrentPiLimited()` 只更新动态边界，积分范围为 `[-2*limit, +2*limit]`。
  三相保留 dq 圆限幅；单相由最终 `phase_a` 限幅并反馈饱和方向。
  电网前馈与 P/I 项共同参与抗饱和，避免只限制内环修正量而漏掉实际输出饱和。
* 电压环接管前通过 `SstPi_Track()` 反算积分，使外环输出跟踪启动阶段的 Id 给定，
  避免固定 Id 斜坡切换到电压 PI 时产生突变。
* 停机或故障复位时清除三个 PI 的积分和运行量；故障路径保留数值异常标志供调试。

## 6. 调参约束

1. `voltage_ki`、`current_ki` 的单位是每秒的连续域积分增益，实际积分增量为
   `ki * Ts * error`；若参数来源已是离散系数，应除以 `Ts` 后再写入。
2. `output_min/max` 与 `integral_min/max` 必须成对设置且下限不大于上限；dq 控制器
   的输出范围必须跨过零。
3. 实际母线电压变化时，`Chb_CurrentPiLimited()` 会更新动态输出边界，但保留已有
   积分并按新的积分边界钳位。低压试验应使用实测母线对应的限幅，不能沿用 680 V
   工程的固定输出尺度。
4. 结构体目前没有独立的输入限幅字段；输入只做有限数检查。需要限制给定或反馈
   物理范围时，应在控制层做参数校验或增加明确的输入边界，不能误解为 PI 已完成
   输入限幅。
5. 单相电流环取消 dq 圆开方；三相 `sqrtf()` 仅在 dq 圆限幅真正触发时执行。现有 PLL/坐标变换本身已经使用浮点
   三角函数和开方。10 kHz 中断的最终周期、缓存/EMIF 等待和栈深仍须在目标 C6748
   + FPGA 硬件上测量，主机回归测试不能替代 WCET 结论。

## 7. 验证结果与边界

`tests/pi_regression.py` 覆盖正常运行、正负积分方向、条件积分、接管跟踪、dq
圆限幅、零电压边界和 NaN/Inf 失效保护，当前通过。该测试不覆盖实际 ADC 标定、
FPGA 调制极性、功率回路稳定性或硬件保护响应时间；这些项目仍需在目标 UNIT、
FPGA 位流和低压主回路上实测。
