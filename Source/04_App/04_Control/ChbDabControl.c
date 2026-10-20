#include "FpgaIo.h"
#include "SstPortStatus.h"
#include "PwmGate.h"
#include "ControlData.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "ProtectionConfig.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include <math.h>

#include "FaultCheck.h"
#include "ModuleVoltageCheck.h"
#include "ChbDabControl.h"
#include "ChbDabControlInternal.h"
#include "SstSinglePhase.h"
#include "SstPi.h"
#include "ControlInit.h"
#include "interrupt.h"
#include "vofa.h"
#include <float.h>

/*
 * 本文件是简化后的 CHB/DAB 控制核心，执行上下文分为两部分：
 *
 * 1. ChbDabControl_Service()：主循环中的慢速状态机，负责启动请求、
 *    闭环预充/旁路/锁相流程、故障复位以及 DAB 启停命令。
 * 2. ChbDabControl_FastStep()：TAD 采样中断中的快速控制，负责采样量更新、
 *    PLL、开环正弦参考、闭环 dq 电流控制和 DAB 占空比爬升。
 *
 * g_chb_dab_command 是调试器/上位机写入的“命令”，
 * g_chb_dab_params 是可整定的参数，g_chb_dab_status 是运行观察量。
 * 三者分开后，调试时可以直接看出“命令是什么、参数是多少、状态机实际到哪一步”。
 */

/*
 * 调试命令默认值：工程上电后保持停机，调试器只需修改 run_enable 或
 * dab_enable 即可验证对应功率桥；开环默认 50 Hz、0.20 调制比。
 */
volatile CHB_DAB_COMMAND g_chb_dab_command = {
    0U, /* run_enable */
    0U, /* dab_enable */
#if SST_LOW_VOLTAGE_100V_TEST
    CHB_AC_MODE_CLOSED_LOOP, /* ac_mode: 100 V 单相低压试验默认闭环。 */
#else
    CHB_AC_MODE_OPEN_LOOP, //CHB_AC_MODE_CLOSED_LOOP,    /* ac_mode */
#endif
    0U, /* fault_reset */
    0.20F, /* open_loop_modulation */
    50.0F, /* open_loop_frequency_hz */
#if SST_LOW_VOLTAGE_100V_TEST
    60.0F, /* dc_voltage_ref_v: 6 个模块各 60 V。 */
#else
    680.0F, /* dc_voltage_ref_v */
#endif
    0.0F /* iq_reference_a */
}; /* g_chb_dab_command：调试器/通信提交的控制命令与给定，控制核心读取执行。 */

/*
 * 控制参数默认值。结构体顺序必须与 ChbDabControl.h 完全一致：
 * 采样/调制 -> 预充/旁路 -> PLL -> 电压外环 -> 启动策略 -> 电流内环
 * -> 保护 -> DAB 占空比。在线协议和调试器均按这个结构访问。
 */
volatile CHB_DAB_PARAMETERS g_chb_dab_params = {
    0.0001F, /* sample_period_s: 10 kHz */
    0.98F, /* modulation_max: source Reference range is 0.02 ... 1.98 */

    SST_CONTROL_PRECHARGE_V, /* precharge_voltage_v：按保护配置选择现代预充门槛，单位 V/模块。 */
    20U, /* precharge_stable_10ms: 200 ms */
    1000U, /* precharge_timeout_10ms: 10 s */
    20U, /* bypass_delay_10ms: 200 ms */
    50U, /* breaker_pulse_10ms: 500 ms */

    50.0F, /* pll_nominal_frequency_hz */
    80.0F, /* pll_kp, normalized q error */
    4000.0F, /* pll_ki, normalized q error */
    62.831853F, /* pll correction limit: +/-10 Hz */
    0.06F, /* pll_lock_error_pu */ //0.02   20261020修改至0.06
    100.0F, /* pll_min_grid_peak_v */
    20U, /* pll_lock_10ms: 200 ms */
    500U, /* pll_timeout_10ms: 5 s after bypass */

#if SST_LOW_VOLTAGE_100V_TEST
    0.05F, /* voltage_kp: 100 V 低压试验保守外环比例系数。 */
    0.10F, /* voltage_ki: 100 V 低压试验外环积分系数。 */
    SST_LV100_RUN_CURRENT_LIMIT_A, /* voltage_current_limit_a：带载给定上限，单位A峰值；启动时动态收紧。 */
#else
    0.099375F, /* voltage_kp = -Volt_PID_Num_0_def */
    12.5F, /* voltage_ki = (Num_1 + Num_0) / 0.0001 */
    95.0F, /* voltage_current_limit_a */
#endif

#if SST_LOW_VOLTAGE_100V_TEST
    CHB_STARTUP_SOURCE_COMPATIBLE,//CHB_STARTUP_SOURCE_COMPATIBLE, /* startup_strategy: 低压试验先投 Idref，再由电压环平滑接管。 */
    0.0F, /* startup_udc_offset_v: 从解锁时实测均压起步，单位 V/模块。 */
    0.001F, /* startup_udc_step_v: 每 TAD 增加 0.001 V，即 10 V/s。 */
#else
    CHB_STARTUP_SOURCE_COMPATIBLE,
    20.0F, /* startup_udc_offset_v: 与C6748Test一致，当前电压+20V */
    2.0F, /* startup_udc_step_v: 电压给定每个TAD最多增加2V */
#endif
#if SST_LOW_VOLTAGE_100V_TEST
    0.25F, /* startup_id_reference_a: 低压试验启动 Id 目标，单位 A。 */
    0.00025F, /* startup_id_ramp_step_a: 每 TAD 增加 0.00025 A，约 100 ms 升至 0.25 A。 */
#else
    10.0F, /* startup_id_reference_a: 启动阶段固定Id目标 */
    0.05F, /* startup_id_ramp_step_a: 每个TAD增加的Id参考 */
#endif
    2000U, /* startup_ramp_tad: 0.2s后强制进入电压环接管 */

    -1.249166667F, /* current_kp = -Current_PID_Num_0_def */
    -16.66666667F, /* current_ki = (Num_1 + Num_0) / 0.0001 */
    1.10F, /* source per-phase voltage limit factor */
    1.0F, /* current_feedback_sign */
    0.1685F, /* SoftwarePhaseComValue */
    0.1085F,//0.16F, /* PhaseComValue */

#if SST_LOW_VOLTAGE_100V_TEST
    SST_FAST_OC_TRIP_A, /* ac_overcurrent_trip_a：由额定输入有效值和峰值系数计算的快速过流门槛，单位A。 */
    SST_FAST_OV_TRIP_V, /* dc_overvoltage_trip_v：由模块额定均压和快速过压系数计算，单位V/模块。 */
#else
    SST_FAST_OC_TRIP_A, /* ac_overcurrent_trip_a：由额定输入有效值和峰值系数计算的快速过流门槛，单位A。 */
    SST_FAST_OV_TRIP_V, /* dc_overvoltage_trip_v：由模块额定均压和快速过压系数计算，单位V/模块。 */
#endif
    5U, /* protection_confirm_samples：连续超限TAD样本数。 */

    350U, /* DAB initial duty (7% of 5000) */
    2500U, /* DAB target duty (50% of 5000) */
    1U, /* DAB ramp count per TAD */
    1250U /* DAB phase start count */
}; /* g_chb_dab_params：CHB/DAB 控制参数；调试器可观察，通信写入需停机校验。 */

/* 运行状态只由控制程序更新，上位机/调试器只读观察。 */
volatile CHB_DAB_STATUS g_chb_dab_status; /* g_chb_dab_status：控制核心维护的运行状态镜像，后台仅用于读取上报。 */
/*
 * 仿真器开环路径不调用 DSP 软件 OC/OV 快速停机。
 *
 * 开环测试的目的首先是确认 UNIT 能否稳定发出 PWM；此时交流采样、直流
 * 电压和电流可能尚未接入或仍为无效值，使用闭环保护门限会把第一拍误判
 * 为故障。闭环按下方使能变量执行本软件检查；调试闭环时可在 CCS 表达式窗口中把
 * 该变量改为 0 临时关闭本软件判断。
 * FPGA/UNIT 的硬件故障回传和封波能力取决于现场位流，
 * 不能将 NoInterlock 位流按完整硬件保护版本解释。
 */
volatile unsigned short g_chb_dab_protection_enable =
    1U; /* g_chb_dab_protection_enable：新增软件过流/过压检查使能，0 关闭、1 开启；硬件保护能力取决于现场 FPGA 位流。 */

SST_PI_CONTROLLER s_voltage_pi; /* s_voltage_pi：直流电压外环对象，保存给定、反馈、P/I项和限幅状态。 */
SST_PI_CONTROLLER s_id_pi; /* s_id_pi：d 轴电流内环对象，保存给定、反馈、P/I项和限幅状态。 */
SST_PI_CONTROLLER s_iq_pi; /* s_iq_pi：q 轴电流内环对象，保存给定、反馈、P/I项和限幅状态。 */
float s_pll_integral; /* s_pll_integral：PLL 角速度修正的积分状态，单位 rad/s。 */
float s_open_loop_angle; /* s_open_loop_angle：开环输出当前相角，单位 rad。 */
float s_id_reference; /* s_id_reference：控制核心当前 Id 给定，单位 A。 */
unsigned long s_state_enter_tick; /* s_state_enter_tick：当前状态进入时的 10 ms 节拍。 */
unsigned long s_breaker_pulse_end_tick; /* s_breaker_pulse_end_tick：断路器控制脉冲结束的 10 ms 节拍。 */
unsigned long s_last_service_tick; /* s_last_service_tick：主循环上次推进状态机的 10 ms 节拍。 */
unsigned short s_precharge_stable_count; /* s_precharge_stable_count：预充连续达标的 10 ms 拍数。 */
unsigned short s_pll_lock_fast_count; /* s_pll_lock_fast_count：PLL 连续锁定的 TAD 样本计数。 */
unsigned short s_pll_lock_10ms_count; /* s_pll_lock_10ms_count：PLL 连续锁定的 10 ms 确认计数。 */
unsigned short s_ac_overcurrent_count; /* s_ac_overcurrent_count：新增交流过流连续超限样本数。 */
unsigned short s_dc_overvoltage_count; /* s_dc_overvoltage_count：新增直流过压连续超限样本数。 */
unsigned short s_startup_phase; /* s_startup_phase：核心内部闭环启动阶段编号。 */
unsigned long s_startup_tad_count; /* s_startup_tad_count：核心内部启动斜坡 TAD 计数。 */
float s_startup_voltage_reference; /* s_startup_voltage_reference：核心内部启动电压给定，单位 V。 */
volatile unsigned short
    s_active_ac_mode; /* s_active_ac_mode：停机启动时锁存的控制模式，启动及运行中禁止切换。 */
unsigned short s_pll_loss_count; /* s_pll_loss_count：闭环运行连续失锁的 TAD 计数。 */

/*==================================================================
 * 函数功能: 把输入值限制在指定上下限之间
 * 输入参数: value：当前控制计算输入；minimum/maximum：允许边界
 * 输出参数: 无。
 * 返回值: 限幅后的浮点值
 * 执行说明: 控制核心；FastStep及Service共用。
 *================================================================*/
float Chb_Clamp(float value, float minimum, float maximum)
{
    if (value > maximum)
    {
        return maximum;
    }
    if (value < minimum)
    {
        return minimum;
    }
    return value;
}

/*==================================================================
 * 函数功能: 计算浮点数绝对值
 * 输入参数: value：待处理浮点量
 * 输出参数: 无。
 * 返回值: 非负绝对值
 * 执行说明: 控制核心；FastStep及Service共用。
 *================================================================*/
float Chb_Abs(float value)
{
    if (value < 0.0F)
    {
        return -value;
    }
    return value;
}

/*==================================================================
 * 函数功能: 检查浮点量是否为有限值且处于指定范围
 * 输入参数: value：待检查浮点量；minimum/maximum：允许边界
 * 输出参数: 无。
 * 返回值: 1表示有效，0表示NaN、无穷或越界
 * 执行说明: 控制核心；启动配置和TAD采样共用。
 *================================================================*/
unsigned short Chb_FiniteInRange(float value, float minimum, float maximum)
{
    if (value != value)
    {
        return 0U;
    }
    if (value < minimum)
    {
        return 0U;
    }
    if (value > maximum)
    {
        return 0U;
    }
    return 1U;
}

/*==================================================================
 * 函数功能: 比较三个浮点量并取最小值
 * 输入参数: a、b、c：待比较的三个浮点量
 * 输出参数: 无。
 * 返回值: 三者最小值
 * 执行说明: 控制核心；三相路径使用。
 *================================================================*/
float Chb_Min3(float a, float b, float c)
{
    float result; /* result：已比较输入中的当前最小值。 */

    result = a;
    if (b < result)
    {
        result = b;
    }
    if (c < result)
    {
        result = c;
    }
    return result;
}

/*==================================================================
 * 函数功能: 比较三个浮点量并取最大值
 * 输入参数: a、b、c：待比较的三个浮点量
 * 输出参数: 无。
 * 返回值: 三者最大值
 * 执行说明: 控制核心；三相路径及保护路径使用。
 *================================================================*/
float Chb_Max3(float a, float b, float c)
{
    float result; /* result：已比较输入中的当前最大值。 */

    result = a;
    if (b > result)
    {
        result = b;
    }
    if (c > result)
    {
        result = c;
    }
    return result;
}

/*==================================================================
 * 函数功能: 将角度归一化到0至2π的周期区间
 * 输入参数: angle：待归一化角度，单位rad
 * 输出参数: 无。
 * 返回值: 归一化的弧度角
 * 执行说明: 控制核心；FastStep及其计算子函数属于100us TAD路径。
 *================================================================*/
float Chb_WrapAngle(float angle)
{
    /* 参数及历史值的第二道边界，避免 CCS 写入 Inf 后陷入无界减法循环。
     * 正常每拍相角只增加不到 0.13 rad，有限异常值也在固定时间内归一化。 */
    if (Chb_FiniteInRange(angle, -FLT_MAX, FLT_MAX) == 0U)
    {
        return 0.0F;
    }
    if (angle >= 2.0F * CHB_TWO_PI || angle < -CHB_TWO_PI)
    {
        angle = fmodf(angle, CHB_TWO_PI);
    }
    /* 将相角限制在 [0, 2*pi)，防止长期运行时浮点数不断增大。 */
    while (angle >= CHB_TWO_PI)
    {
        angle -= CHB_TWO_PI;
    }
    while (angle < 0.0F)
    {
        angle += CHB_TWO_PI;
    }
    return angle;
}

/*==================================================================
 * 函数功能: 配置并执行直流电压外环 PI
 * 输入参数: reference：单模块直流电压给定，单位 V
 *           feedback：单模块直流电压反馈，单位 V
 * 输出参数: s_voltage_pi：本拍完整 PI 状态及限幅结果。
 * 返回值: 限幅后的 Id 给定，单位 A；计算失败返回 0 并锁存运行故障。
 * 执行说明: 仅由TAD闭环路径调用；电流矢量限幅在调用者中统一处理。
 *==================================================================*/
float Chb_VoltagePiLimited(float reference, float feedback)
{
    float limit; /* limit：外环输出电流的绝对值边界，单位 A。 */
    float iq; /* iq：本拍 q 轴给定，单位 A。 */

    if (Chb_FiniteInRange(reference, -FLT_MAX, FLT_MAX) == 0U ||
        Chb_FiniteInRange(feedback, -FLT_MAX, FLT_MAX) == 0U)
    {
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
        return 0.0F;
    }

    limit = g_chb_dab_params.voltage_current_limit_a;
#if SST_LOW_VOLTAGE_100V_TEST
    limit = g_chb_dab_status.current_limit_active_a;
    iq = Chb_Clamp(g_chb_dab_command.iq_reference_a, -limit, limit);
    if (limit > Chb_Abs(iq))
    {
        limit = sqrtf(limit * limit - iq * iq);
    }
    else
    {
        limit = 0.0F;
    }
#else
    (void)iq;
#endif

    if (limit < 0.0F ||
        SstPi_Configure(&s_voltage_pi, g_chb_dab_params.voltage_kp, g_chb_dab_params.voltage_ki,
                        g_chb_dab_params.sample_period_s, -limit, limit, -limit, limit, 1U) == 0U ||
        SstPi_Run(&s_voltage_pi, reference, feedback, 0.0F) == SST_PI_FAILURE)
    {
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
        return 0.0F;
    }
    return s_voltage_pi.output;
}



/*==================================================================
 * 函数功能: 配置并执行 dq 电流 PI，电网前馈一同参与矢量限幅及抗饱和
 * 输入参数: voltage_d/voltage_q：d、q 轴电网前馈，单位 V
 *           error_d/error_q：d、q 轴给定减反馈误差，单位 A
 *           limit：实际母线允许的电压矢量幅值，单位 V
 *           command_d/command_q：输出 d、q 轴电压指令的存储指针，单位 V
 * 输出参数: command_d/command_q：限幅后的 d、q 轴电压指令，单位 V；更新完整两轴 PI 对象。
 * 返回值: 1 成功，0 失败且已封波；单相和三相复用相同调节器。
 *================================================================*/
unsigned short Chb_CurrentPiLimited(
    float voltage_d, float voltage_q, float error_d, float error_q, float limit, float *command_d, float *command_q)
{
    float d_inner; /* d_inner：d轴电流PI输出，不含电网电压前馈，单位V。 */
    float q_inner; /* q_inner：q轴电流PI输出，不含电网电压前馈，单位V。 */
    float magnitude; /* magnitude：叠加前馈后的dq电压矢量幅值，单位V。 */
    float scale; /* scale：超过圆限幅时的统一缩放系数。 */
    unsigned short valid_d; /* valid_d：d轴参数及动态边界校验结果。 */
    unsigned short valid_q; /* valid_q：q轴参数及动态边界校验结果。 */
    unsigned short input_valid; /* input_valid：前馈、误差和电压边界是否为有限值。 */
    unsigned short pi_valid; /* pi_valid：两轴 PI 配置及本拍计算是否成功。 */

    if (command_d == (float *)0 || command_q == (float *)0)
    {
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
        return 0U;
    }
    input_valid = 1U;
    if (Chb_FiniteInRange(voltage_d, -FLT_MAX, FLT_MAX) == 0U)
    {
        input_valid = 0U;
    }
    if (Chb_FiniteInRange(voltage_q, -FLT_MAX, FLT_MAX) == 0U)
    {
        input_valid = 0U;
    }
    if (Chb_FiniteInRange(error_d, -FLT_MAX, FLT_MAX) == 0U)
    {
        input_valid = 0U;
    }
    if (Chb_FiniteInRange(error_q, -FLT_MAX, FLT_MAX) == 0U)
    {
        input_valid = 0U;
    }
    if (Chb_FiniteInRange(limit, 0.0F, FLT_MAX) == 0U)
    {
        input_valid = 0U;
    }
    if (input_valid == 0U)
    {
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
        return 0U;
    }

    /* d、q轴分别执行电流PI；PI本身只负责内环修正量和轴向限幅。 */
    valid_d = SstPi_Configure(&s_id_pi, g_chb_dab_params.current_kp, g_chb_dab_params.current_ki,
                              g_chb_dab_params.sample_period_s, -limit, limit, -2.0F * limit, 2.0F * limit, 1U);
    valid_q = SstPi_Configure(&s_iq_pi, g_chb_dab_params.current_kp, g_chb_dab_params.current_ki,
                              g_chb_dab_params.sample_period_s, -limit, limit, -2.0F * limit, 2.0F * limit, 1U);
    pi_valid = 1U;
    if (valid_d == 0U)
    {
        pi_valid = 0U;
    }
    if (valid_q == 0U)
    {
        pi_valid = 0U;
    }
    if (pi_valid != 0U)
    {
        if (SstPi_Run(&s_id_pi, g_chb_dab_status.id_feedback_a + error_d,
                      g_chb_dab_status.id_feedback_a, 0.0F) == SST_PI_FAILURE)
        {
            pi_valid = 0U;
        }
    }
    if (pi_valid != 0U)
    {
        if (SstPi_Run(&s_iq_pi, g_chb_dab_status.iq_feedback_a + error_q,
                      g_chb_dab_status.iq_feedback_a, 0.0F) == SST_PI_FAILURE)
        {
            pi_valid = 0U;
        }
    }
    if (pi_valid == 0U)
    {
        *command_d = 0.0F;
        *command_q = 0.0F;
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
        return 0U;
    }

    d_inner = s_id_pi.output;
    q_inner = s_iq_pi.output;
    /* 调制波由“内环输出 + 电网电压前馈”组成，再统一执行dq圆限幅。 */
    *command_d = d_inner + voltage_d;
    *command_q = q_inner + voltage_q;
    magnitude = sqrtf((*command_d * *command_d) + (*command_q * *command_q));
    if (Chb_FiniteInRange(magnitude, 0.0F, FLT_MAX) == 0U)
    {
        *command_d = 0.0F;
        *command_q = 0.0F;
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
        return 0U;
    }
    if (magnitude > limit)
    {
        if (magnitude <= 0.0F)
        {
            *command_d = 0.0F;
            *command_q = 0.0F;
        }
        else
        {
            scale = limit / magnitude;
            *command_d *= scale;
            *command_q *= scale;
        }
    }
    return 1U;
}

/*==================================================================
 * 函数功能: 清空电压、电流 PI 及参考状态
 * 输入参数: 无
 * 输出参数: s_voltage_pi、s_id_pi、s_iq_pi 及电流给定历史状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_ResetControllers(void)
{
    unsigned short voltage_error_flags; /* voltage_error_flags：故障前外环的数值异常证据。 */
    unsigned short id_error_flags; /* id_error_flags：故障前 d 轴的数值异常证据。 */
    unsigned short iq_error_flags; /* iq_error_flags：故障前 q 轴的数值异常证据。 */

    voltage_error_flags = s_voltage_pi.flags & 0x0700U;
    id_error_flags = s_id_pi.flags & 0x0700U;
    iq_error_flags = s_iq_pi.flags & 0x0700U;
    SstPi_Reset(&s_voltage_pi);
    SstPi_Reset(&s_id_pi);
    SstPi_Reset(&s_iq_pi);
    /* 故障时清积分和输出，保留异常标志供 CCS 排查；重新配置后自动清除。 */
    if (g_chb_dab_status.state == CHB_STATE_FAULT)
    {
        if (voltage_error_flags != 0U)
        {
            s_voltage_pi.flags = voltage_error_flags;
        }
        if (id_error_flags != 0U)
        {
            s_id_pi.flags = id_error_flags;
        }
        if (iq_error_flags != 0U)
        {
            s_iq_pi.flags = iq_error_flags;
        }
    }
    s_id_reference = 0.0F;
    g_chb_dab_status.id_reference_a = 0.0F;
    g_chb_dab_status.load_ready = 0U;
    g_chb_dab_status.load_ready_count = 0U;
#if SST_LOW_VOLTAGE_100V_TEST
    if (g_chb_dab_params.voltage_current_limit_a < SST_LV100_START_CURRENT_LIMIT_A)
    {
        g_chb_dab_status.current_limit_active_a = g_chb_dab_params.voltage_current_limit_a;
    }
    else
    {
        g_chb_dab_status.current_limit_active_a = SST_LV100_START_CURRENT_LIMIT_A;
    }
#else
    g_chb_dab_status.current_limit_active_a = g_chb_dab_params.voltage_current_limit_a;
#endif
}

/*
 * 启动阶段只在闭环AC运行入口重新建立一次，PWM解锁后直接执行下一次控制计算。
 */
/*==================================================================
 * 函数功能: 复位 Id 斜坡与电压环接管阶段的历史状态
 * 输入参数: 无
 * 输出参数: 内部启动阶段、拍计数、给定及对应状态镜像。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_ResetStartupSequence(void)
{
    /* 闭环 PWM 解锁时重新建立启动阶段观察量和临时电压参考。 */
    s_startup_phase = CHB_STARTUP_IDLE;
    s_startup_tad_count = 0UL;
    /* 初值不跨过最终目标；已高于目标时从实测值向下平滑收敛。 */
    if (g_chb_dab_status.dc_voltage_average_v < g_chb_dab_command.dc_voltage_ref_v)
    {
        s_startup_voltage_reference = Chb_Clamp(
            g_chb_dab_status.dc_voltage_average_v + g_chb_dab_params.startup_udc_offset_v,
            g_chb_dab_status.dc_voltage_average_v, g_chb_dab_command.dc_voltage_ref_v);
    }
    else
    {
        s_startup_voltage_reference = g_chb_dab_status.dc_voltage_average_v;
    }

    g_chb_dab_status.startup_id_reference_a = 0.0F;
    g_chb_dab_status.startup_voltage_reference_v = s_startup_voltage_reference;
    g_chb_dab_status.startup_phase = CHB_STARTUP_IDLE;
    g_chb_dab_status.startup_tad_count = 0UL;
    g_chb_dab_status.voltage_loop_active = 0U;
    g_chb_dab_status.load_ready = 0U;
    g_chb_dab_status.load_ready_count = 0U;
#if SST_LOW_VOLTAGE_100V_TEST
    if (g_chb_dab_params.voltage_current_limit_a < SST_LV100_START_CURRENT_LIMIT_A)
    {
        g_chb_dab_status.current_limit_active_a = g_chb_dab_params.voltage_current_limit_a;
    }
    else
    {
        g_chb_dab_status.current_limit_active_a = SST_LV100_START_CURRENT_LIMIT_A;
    }
#else
    g_chb_dab_status.current_limit_active_a = g_chb_dab_params.voltage_current_limit_a;
#endif
}

/* 将电压PI积分器预置到当前Idref，避免电压环接管时发生Idref跳变。 */
/*==================================================================
 * 函数功能: 根据当前 Id 给定预置电压 PI 积分，减小接管突变
 * 输入参数: voltage_error：直流电压给定减反馈的误差，单位 V
 *           id_reference：用于 PI 接管对齐的 d 轴电流给定，单位 A
 * 输出参数: s_voltage_pi.integral，按当前 Id 给定及比例项预置。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_AlignVoltagePiToIdReference(float voltage_error, float id_reference)
{
    float limit; /* limit：电压外环允许的电流输出边界，单位 A。 */

    limit = g_chb_dab_params.voltage_current_limit_a;
#if SST_LOW_VOLTAGE_100V_TEST
    limit = g_chb_dab_status.current_limit_active_a;
    {
        float iq; /* iq：与电流内环一致的限幅后 q 轴给定，单位 A。 */
        iq = Chb_Clamp(g_chb_dab_command.iq_reference_a, -limit, limit);
        if (limit > Chb_Abs(iq))
        {
            limit = sqrtf(limit * limit - iq * iq);
        }
        else
        {
            limit = 0.0F;
        }
    }
#endif
    if (!SstPi_Configure(&s_voltage_pi, g_chb_dab_params.voltage_kp, g_chb_dab_params.voltage_ki,
                         g_chb_dab_params.sample_period_s, -limit, limit, -limit, limit, 1U))
    {
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
        return;
    }
    /* 显式建立本拍输入，再反算积分，使电压环输出跟踪现有 Idref。 */
    s_voltage_pi.reference = s_startup_voltage_reference;
    s_voltage_pi.feedback = g_chb_dab_status.dc_voltage_average_v;
    s_voltage_pi.error = voltage_error;
    s_voltage_pi.proportional = s_voltage_pi.kp * voltage_error;
    s_voltage_pi.feedforward = 0.0F;
    if (!SstPi_Track(&s_voltage_pi, id_reference))
    {
        ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
    }
}

/* 启动给定按可配置步长收敛，避免固定电压窗口在低压试验中产生阶跃。 */
/*==================================================================
 * 函数功能: 按启动策略更新直流电压外环给定及状态镜像
 * 输入参数: 无
 * 输出参数: 内部启动电压给定及 startup_voltage_reference_v 状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_UpdateVoltageReference(void)
{
    float target_voltage; /* target_voltage：最终单模块直流目标，单位 V。 */
    float step; /* step：每个 TAD 的给定最大变化量，单位 V。 */

    target_voltage = g_chb_dab_command.dc_voltage_ref_v;
    step = g_chb_dab_params.startup_udc_step_v;
    /* 不再使用固定的 ±10 V/±2 V 窗口，低压启动及在线改目标同样连续。 */
    if (s_startup_voltage_reference < target_voltage)
    {
        s_startup_voltage_reference += step;
        if (s_startup_voltage_reference > target_voltage)
        {
            s_startup_voltage_reference = target_voltage;
        }
    }
    else if (s_startup_voltage_reference > target_voltage)
    {
        s_startup_voltage_reference -= step;
        if (s_startup_voltage_reference < target_voltage)
        {
            s_startup_voltage_reference = target_voltage;
        }
    }
    g_chb_dab_status.startup_voltage_reference_v = s_startup_voltage_reference;
}

/*==================================================================
 * 函数功能: 将三相归一化调制指令映射到原 FPGA 工频桥参考格式
 * 输入参数: phase_a：A 相归一化参考 q，无量纲；调用者已完成电压归一化及加 1 偏置
 *           phase_b：B 相归一化参考 q，与 phase_a 同一格式
 *           phase_c：C 相归一化参考 q，与 phase_a 同一格式
 * 输出参数: 各相 FPGA501 Reference 寄存器、DAC_BUF[0] 及 modulation_actual。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_WriteNormalizedReferences(float phase_a, float phase_b, float phase_c)
{
    float reference; /* reference：由归一化调制指令换算的原 FPGA 参考值。 */

    /*
     * phase_x 使用 FPGA 参考的归一化格式：1.0 是指令中点，
     * 0.02/1.98 是原程序允许的最小/最大边界。
     * 归一化值乘以各相 ReferenceDEF 后写入 FPGA501 的 Reference 寄存器。
     * 指令中点不单独保证实际零输出，仍取决于 FPGA 调制实现及模块配置。
     * 同时把 A 相换算到 DAC_BUF[0]，便于示波器观察软件实际输出参考。
     */
    phase_a = Chb_Clamp(phase_a, CHB_REFERENCE_MIN, CHB_REFERENCE_MAX);
#if SST_PHASE_A_ONLY_TEST
    /* B/C 只写指令中点；其 CHB/DAB 实际封波由 Module_Control_Write 最终门控保证。 */
    phase_b = 1.0F;
    phase_c = 1.0F;
#endif
    phase_b = Chb_Clamp(phase_b, CHB_REFERENCE_MIN, CHB_REFERENCE_MAX);
    phase_c = Chb_Clamp(phase_c, CHB_REFERENCE_MIN, CHB_REFERENCE_MAX);

    /* DA0 输出 A 相归一化调制波，供示波器观察。
     * 调制中点 1.0 对应 2.5 V，0.0/2.0 分别对应 0/5 V。
     * DAC_Output() 在本次 TAD 中断末尾把电压值换算为 12 位 DAC 码。
     */
    DAC_BUF[0] = phase_a * 2.5F;

    reference = phase_a * HA_ReferenceDEF_A;
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].Reference = (unsigned short)reference;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].Reference = (unsigned short)reference;

    reference = phase_b * HA_ReferenceDEF_B;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].Reference = (unsigned short)reference;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].Reference = (unsigned short)reference;

    reference = phase_c * HA_ReferenceDEF_C;
    FPGA_Interface[Com_C1_PluginUnit_13 - 2].Reference = (unsigned short)reference;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].Reference = (unsigned short)reference;

    /* 501 的电流方向和电压排序属于每拍调制数据，单相、三相均须更新。
     * 沿用 Write_ControlParameter_50Hz 的定义：同相两卡电流方向一致，
     * 电压排序互补；逐个位域赋值，保留 DC_Log 中的其他位。 */
    if (HA_IG_Log_A == 0)
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.IG_Orient = 0;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.IG_Orient = 0;
    }
    else
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.IG_Orient = 1;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.IG_Orient = 1;
    }

    if (HA_DC_Log_A == 0)
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.DC_Order = 0;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.DC_Order = 1;
    }
    else
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.DC_Order = 1;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.DC_Order = 0;
    }

    if (HA_IG_Log_B == 0)
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.IG_Orient = 0;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.IG_Orient = 0;
    }
    else
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.IG_Orient = 1;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.IG_Orient = 1;
    }

    if (HA_DC_Log_B == 0)
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.DC_Order = 0;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.DC_Order = 1;
    }
    else
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.DC_Order = 1;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.DC_Order = 0;
    }

    if (HA_IG_Log_C == 0)
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.IG_Orient = 0;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.IG_Orient = 0;
    }
    else
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.IG_Orient = 1;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.IG_Orient = 1;
    }

    if (HA_DC_Log_C == 0)
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.DC_Order = 0;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.DC_Order = 1;
    }
    else
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.DC_Order = 1;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.DC_Order = 0;
    }

    g_chb_dab_status.modulation_actual =
        Chb_Max3(Chb_Abs(phase_a - 1.0F), Chb_Abs(phase_b - 1.0F), Chb_Abs(phase_c - 1.0F));
}

/*==================================================================
 * 函数功能: 向三相 FPGA 写入指令中点，不单独改变 PWM 使能
 * 输入参数: 无
 * 输出参数: 原调制给定寄存器的中性参考及 modulation_actual。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_WriteNeutralReferences(void)
{
    /* 三相均写归一化参考 1.0；实际封波由 PWM 使能控制，不能仅靠此参考值判断。 */
    Chb_WriteNormalizedReferences(1.0F, 1.0F, 1.0F);
}

/*==================================================================
 * 函数功能: 限幅 DAB 计数并更新各相占空比、起始计数及 FPGA 寄存器
 * 输入参数: duty：本拍 DAB 占空比计数
 * 输出参数: HA_DABDuty_*、HA_DAB_PluseStart_*、FPGA DAB 计数及 dab_duty_count。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_WriteDabDuty(unsigned short duty)
{
    unsigned short start; /* start：限幅后的 DAB 脉冲起始计数。 */

    /*
     * 左右桥的占空比和启动相位必须成组写入所有已配置 UNIT，
     * 否则不同相/不同桥会出现占空比不一致。duty 从初值逐步爬升到 50%。
     */
    start = g_chb_dab_params.dab_start_count;

    /* PM mask 为 0 的槽位没有物理 UNIT，单板开环时不能向其发启动帧。 */
    if (HA_PM_A1 != 0U)
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = duty;
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = duty;
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = start;
    }
    if (HA_PM_A2 != 0U)
    {
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = duty;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = duty;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = start;
    }
    if (HA_PM_B1 != 0U)
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_LEFTDuty = duty;
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_RIGHTDuty = duty;
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_Start = start;
    }
    if (HA_PM_B2 != 0U)
    {
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_LEFTDuty = duty;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_RIGHTDuty = duty;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_Start = start;
    }
    if (HA_PM_C1 != 0U)
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_LEFTDuty = duty;
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_RIGHTDuty = duty;
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_Start = start;
    }
    if (HA_PM_C2 != 0U)
    {
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_LEFTDuty = duty;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_RIGHTDuty = duty;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_Start = start;
    }

    g_chb_dab_status.dab_duty_count = duty;
}

/*==================================================================
 * 函数功能: 设置交流输入断路器的通断脉冲及状态
 * 输入参数: closed：交流输入或预充旁路闭合选择，0 断开、非零闭合
 * 输出参数: Port_DOUT 断路器控制脉冲、结束时刻以及 ac_input_closed。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_SetAcInput(unsigned short closed)
{
    unsigned int interrupt_state; /* interrupt_state：只保护最终开关量提交的原中断状态。 */

    interrupt_state = IntGlobalDisable();
    if (closed != 0U)
    {
        if (g_chb_dab_status.state == CHB_STATE_FAULT ||
            g_chb_dab_status.fault_code != CHB_FAULT_NONE)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (Port_RealFault.Bbits.PORT_FAULT_bit15 != 0U)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (g_chb_dab_command.ac_mode != s_active_ac_mode)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (s_active_ac_mode != CHB_AC_MODE_CLOSED_LOOP)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (g_chb_dab_command.run_enable == 0U)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
    }
    /*
     * 闭环启动使用两个脉冲量控制交流进线接触器：
     * On/Off 只保持 breaker_pulse_10ms 个 10 ms 节拍，随后由
     * Chb_ClearAcInputPulse() 清零，避免线圈长时间通电。
     */
    if (closed != 0U)
    {
        Port_DOUT.Bbits.Breaker_InnerOn = 1U;
        Port_DOUT.Bbits.Breaker_InnerOff = 0U;
        g_chb_dab_status.ac_input_closed = 1U;
    }
    else
    {
        Port_DOUT.Bbits.Breaker_InnerOn = 0U;
        Port_DOUT.Bbits.Breaker_InnerOff = 1U;
        g_chb_dab_status.ac_input_closed = 0U;
    }
    s_breaker_pulse_end_tick = g_chb_dab_status.tick_10ms + g_chb_dab_params.breaker_pulse_10ms;
    Port_IO_Out();
    IntGlobalRestore(interrupt_state);
}

/*==================================================================
 * 函数功能: 撤销已到期的交流输入断路器控制脉冲
 * 输入参数: 无
 * 输出参数: Port_DOUT 断路器合/分闸脉冲及对应数字输出寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_ClearAcInputPulse(void)
{
    /* 清除交流接触器的 On/Off 脉冲，不改变软件中的闭合状态记录。 */
    Port_DOUT.Bbits.Breaker_InnerOn = 0U;
    Port_DOUT.Bbits.Breaker_InnerOff = 0U;
    Port_IO_Out();
}

/*==================================================================
 * 函数功能: 设置预充旁路接触器及状态镜像
 * 输入参数: closed：交流输入或预充旁路闭合选择，0 断开、非零闭合
 * 输出参数: Port_DOUT 接触器控制位及 precharge_bypassed。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_SetPrechargeBypass(unsigned short closed)
{
    unsigned int interrupt_state; /* interrupt_state：只保护最终开关量提交的原中断状态。 */

    interrupt_state = IntGlobalDisable();
    if (closed != 0U)
    {
        if (g_chb_dab_status.state == CHB_STATE_FAULT)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (g_chb_dab_status.fault_code != CHB_FAULT_NONE)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (Port_RealFault.Bbits.PORT_FAULT_bit15 != 0U)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (g_chb_dab_command.ac_mode != s_active_ac_mode)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (s_active_ac_mode != CHB_AC_MODE_CLOSED_LOOP)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
        if (g_chb_dab_command.run_enable == 0U)
        {
            IntGlobalRestore(interrupt_state);
            return;
        }
    }
    /* 软起电阻旁路是保持量，闭环预充达标后置 1，停机/故障时置 0。 */
    if (closed != 0U)
    {
        Port_DOUT.Bbits.Contactor_Inner = 1U;
        g_chb_dab_status.precharge_bypassed = 1U;
    }
    else
    {
        Port_DOUT.Bbits.Contactor_Inner = 0U;
        g_chb_dab_status.precharge_bypassed = 0U;
    }
    Port_IO_Out();
    IntGlobalRestore(interrupt_state);
}

/*==================================================================
 * 函数功能: 检查所有已配置相的模块均压是否达到闭环发波门槛
 * 输入参数: 无；读取实际模块数量和本拍各相模块均压
 * 输出参数: 无
 * 返回值: 1 表示至少一相已配置且各有效相母线均达标，0 表示未达标
 * 执行说明: 单相只检查 A；三相按 NHA_module_A/B/C 的实际配置逐相检查。
 *           有界比较同时拒绝 NaN/Inf，不能仅检查三相平均值。
 *================================================================*/
unsigned short Chb_DcBusReady(void)
{
    unsigned short active_phases; /* active_phases：本次检查中已配置的相数。 */

    active_phases = 0U;
    if (NHA_module_A != 0U)
    {
        active_phases++;
        if (Chb_FiniteInRange(SVG_Udc_A_Ave, g_chb_dab_params.precharge_voltage_v, 1000000.0F) == 0U)
        {
            return 0U;
        }
    }
#if !SST_PHASE_A_ONLY_TEST
    if (NHA_module_B != 0U)
    {
        active_phases++;
        if (Chb_FiniteInRange(SVG_Udc_B_Ave, g_chb_dab_params.precharge_voltage_v, 1000000.0F) == 0U)
        {
            return 0U;
        }
    }
    if (NHA_module_C != 0U)
    {
        active_phases++;
        if (Chb_FiniteInRange(SVG_Udc_C_Ave, g_chb_dab_params.precharge_voltage_v, 1000000.0F) == 0U)
        {
            return 0U;
        }
    }
#endif
    if (active_phases == 0U)
    {
        return 0U;
    }
    return 1U;
}

/*==================================================================
 * 函数功能: 判断快速故障和控制状态是否禁止新的硬件动作
 * 输入参数: 无
 * 输出参数: 无
 * 返回值: 1 表示故障已存在或状态已锁存，0 表示允许继续提交动作
 * 执行说明: 只读取状态；供 PWM 门控及兼容状态镜像在提交前做最后一次检查。
 *================================================================*/
unsigned short Chb_GateBlocked(void)
{
    if (g_chb_dab_status.state == CHB_STATE_FAULT)
    {
        return 1U;
    }
    if (g_chb_dab_status.fault_code != CHB_FAULT_NONE)
    {
        return 1U;
    }
    if (Port_RealFault.Bbits.PORT_FAULT_bit15 != 0U)
    {
        return 1U;
    }
    return 0U;
}

/*==================================================================
 * 函数功能: 在极短临界区内提交 CHB/DAB PWM 使能
 * 输入参数: bridge：1 为 CHB 工频桥，2 为 DAB 桥
 * 输出参数: FPGA PWM 门控寄存器及原有运行标志
 * 返回值: 1 表示已提交，0 表示故障/请求已撤销而未提交
 * 执行说明: 只保护 FPGA 门控寄存器写入；Variable_RST 在关中断之前执行，
 *           防止 10 kHz FPGA 外部中断被较长的调节器清零过程阻塞。
 *================================================================*/
unsigned short Chb_EnablePwm(unsigned short bridge)
{
    unsigned int interrupt_state; /* interrupt_state：进入临界区前的 CPU 全局中断状态。 */
    unsigned short committed; /* committed：门控已成功提交的返回标志。 */

    committed = 0U;
    if (bridge == 1U)
    {
        Variable_RST(); /* 原 HVAC 的 AC 开锁复位；允许 10 kHz 中断抢占。 */
    }
    interrupt_state = IntGlobalDisable();
    if ((Chb_GateBlocked() == 0U) && (g_chb_dab_status.state == CHB_STATE_RUNNING) &&
        (g_chb_dab_command.ac_mode == s_active_ac_mode) &&
        ((s_active_ac_mode == CHB_AC_MODE_OPEN_LOOP) ||
         ((g_chb_dab_command.run_enable != 0U) && (g_chb_dab_status.pll_locked != 0U) &&
          (Chb_DcBusReady() != 0U))) &&
        (((bridge == 1U) && (g_chb_dab_command.run_enable != 0U)) ||
         ((bridge == 2U) && (g_chb_dab_command.dab_enable != 0U))))
    {
        if (bridge == 1U)
        {
            if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical != 0U)
            {
                Module_50Hz_PWMEN(1U);
            }
            else
            {
                Module_PWMEN(1U);
                Port_DSPControl_AllShutdown(0U);
                FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1U;
            }
            PortRunFLAG.Bbits.PWMState_50Hz = 0U;
            g_chb_dab_status.ac_pwm_enabled = 1U;
            committed = 1U;
        }
        else if (bridge == 2U)
        {
            if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical != 0U)
            {
                Module_ENdab_PWMEN(1U);
            }
            else
            {
                Module_PWMEN(2U);
                Port_DSPControl_AllShutdown(0U);
                FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1U;
            }
            PortRunFLAG.Bbits.PWMState_Dab = 0U;
            g_chb_dab_status.dab_pwm_enabled = 1U;
            committed = 1U;
        }
    }
    IntGlobalRestore(interrupt_state);

    return committed;
}

/*==================================================================
 * 函数功能: 在极短临界区内提交 CHB/DAB PWM 封锁
 * 输入参数: bridge：0 为两桥全部封锁，1 为 CHB，2 为 DAB
 * 输出参数: FPGA PWM 门控寄存器及原有运行标志
 * 返回值: 无
 * 执行说明: 先提交硬件封锁再恢复中断；占空比、控制器和状态镜像由调用者随后清理。
 *================================================================*/
void Chb_DisablePwm(unsigned short bridge)
{
    unsigned int interrupt_state; /* interrupt_state：进入临界区前的 CPU 全局中断状态。 */

    interrupt_state = IntGlobalDisable();
    if (bridge == 1U)
    {
        Module_50Hz_PWMEN(0U);
        PortRunFLAG.Bbits.PWMState_50Hz = 1U;
    }
    else if (bridge == 2U)
    {
        Module_ENdab_PWMEN(0U);
        PortRunFLAG.Bbits.PWMState_Dab = 1U;
        ControlFLAG.Bbits.DAB_StartMethod = 0U; /* 保留原 PWMPluse_Disable 的 DAB 停机复位。 */
    }
    else
    {
        Module_PWMEN(0U);
        Port_DSPControl_AllShutdown(1U);
        FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0U;
        PortRunFLAG.Bbits.PWMState_50Hz = 1U;
        PortRunFLAG.Bbits.PWMState_Dab = 1U;
    }
    IntGlobalRestore(interrupt_state);
}

/*==================================================================
 * 函数功能: 撤销工频桥和 DAB PWM 使能并清空状态镜像
 * 输入参数: 无
 * 输出参数: 原模块 PWM 控制寄存器、ac_pwm_enabled 和 dab_pwm_enabled。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_DisableAllPwm(void)
{
    /*
     * 统一封锁 CHB+DAB：先把 FaultCheck 的封波类型设为“端口全部封波”，
     * 再清零软件状态、调制参考、DAB 占空比和 PI 积分。
     */
    FaultDisposeFLAG.Bbits.PWMPulseState = 3U;
    Chb_DisablePwm(0U);
    ControlFLAG.Bbits.DAB_StartMethod = 0U;
    g_chb_dab_status.ac_pwm_enabled = 0U;
    g_chb_dab_status.dab_pwm_enabled = 0U;
    Chb_WriteNeutralReferences();
    Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
    Chb_ResetControllers();
    Chb_ResetStartupSequence();
}

/*==================================================================
 * 函数功能: 把简化控制状态映射到原工程端口状态字
 * 输入参数: port_status：旧端口 PortStatusNum_* 兼容状态值
 * 输出参数: Port_State、Port_RunStatus 及旧端口主状态/运行位域。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_SetLegacyStatus(unsigned short port_status)
{
    unsigned int interrupt_state; /* interrupt_state：兼容状态镜像最后提交前的原中断状态。 */

    interrupt_state = IntGlobalDisable();
    if (Chb_GateBlocked() != 0U)
    {
        /* 慢任务被 FAULT ISR 抢占后，不允许把故障兼容状态重新写成运行/启动。 */
        port_status = PortStatusNum_Stop;
        ControlFLAG.Bbits.Port_Start = 0U;
        ControlFLAG.Bbits.Port_Stop = 1U;
    }
    /* 同步旧 HVAC 状态变量，保证原有 DPRAM 遥测和故障页面仍能看到状态。 */
    Port_RunStatus = port_status;
    Port_State = port_status;
    Port_MainStatus.Bbits.P_Status = port_status;
    IntGlobalRestore(interrupt_state);
    Port_StateUpdate_Write();
}

/*==================================================================
 * 函数功能: 切换控制状态并记录状态进入的 10 ms 节拍
 * 输入参数: state：CHB_STATE_* 目标状态
 * 输出参数: g_chb_dab_status.state 及 s_state_enter_tick。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_EnterState(unsigned short state)
{
    unsigned short mode; /* mode：本次入口使用的控制模式，启动后始终使用锁存值。 */
    unsigned int interrupt_state; /* interrupt_state：状态发布的原 CPU 中断状态。 */

    mode = s_active_ac_mode;
    if ((state == CHB_STATE_RUNNING) && (Chb_GateBlocked() == 0U))
    {
        /* 先完成控制器准备，再发布 RUNNING；此过程允许 TAD/FAULT 中断抢占。 */
        Chb_ResetControllers();
        Chb_ResetStartupSequence();
        s_pll_loss_count = 0U;
        if ((mode == CHB_AC_MODE_CLOSED_LOOP) && (g_chb_dab_params.startup_strategy == CHB_STARTUP_ID_RAMP))
        {
            s_startup_phase = CHB_STARTUP_ID_RAMP_PHASE;
        }
        else
        {
            s_startup_phase = CHB_STARTUP_NORMAL;
            if (mode == CHB_AC_MODE_CLOSED_LOOP)
            {
                g_chb_dab_status.voltage_loop_active = 1U;
            }
            else
            {
                g_chb_dab_status.voltage_loop_active = 0U;
            }
        }
        g_chb_dab_status.startup_phase = s_startup_phase;
    }
    interrupt_state = IntGlobalDisable();
    if ((state != CHB_STATE_FAULT) &&
        ((g_chb_dab_status.fault_code != CHB_FAULT_NONE) || (Port_RealFault.Bbits.PORT_FAULT_bit15 != 0U) ||
         ((g_chb_dab_status.state == CHB_STATE_FAULT) && (state != CHB_STATE_STOPPED)) ||
         ((state != CHB_STATE_STOPPED) &&
          ((g_chb_dab_command.ac_mode != mode) ||
           ((g_chb_dab_command.run_enable == 0U) &&
            ((mode != CHB_AC_MODE_OPEN_LOOP) || (g_chb_dab_command.dab_enable == 0U)))))))
    {
        /* 故障 ISR 已抢占或请求已撤销，后台不能继续覆盖故障状态。 */
        IntGlobalRestore(interrupt_state);
        return;
    }
    /*
     * 状态入口动作集中在这里，避免状态切换时遗漏硬件动作：
     * STOPPED       -> 全部封波；闭环断开进线/旁路，开环不碰外部开关；
     * PRECHARGE     -> 闭环闭合交流进线，旁路保持断开；
     * BYPASS_DELAY  -> 闭合软起旁路，等待接触器稳定；
     * WAIT_PLL      -> 闭环等待 PLL 连续锁定；
     * RUNNING       -> 按命令分别解锁 CHB 和 DAB；
     * FAULT         -> 全部封波并锁存故障。
     */
    g_chb_dab_status.state = state;
    s_state_enter_tick = g_chb_dab_status.tick_10ms;
    g_chb_dab_status.switch_stable_count = 0U;
    g_chb_dab_status.switch_mismatch_count = 0U;
    IntGlobalRestore(interrupt_state);

    switch (state)
    {
        case CHB_STATE_STOPPED: // 停止态：AC/DAB封波，交流进线和旁路断开
        {
            Chb_DisableAllPwm();
            /* 开环测试不接管外部开关量，避免单板调试时误动作交流接触器。 */
            if (mode != CHB_AC_MODE_OPEN_LOOP)
            {
                Chb_SetPrechargeBypass(0U);
                Chb_SetAcInput(0U);
            }
            ControlFLAG.Bbits.Port_Start = 0U;
            ControlFLAG.Bbits.Port_Stop = 2U;
            Chb_SetLegacyStatus(PortStatusNum_Standby);
        }
        break;

        case CHB_STATE_PRECHARGE: // 预充态：只闭合交流进线，不发PWM
        {
            s_precharge_stable_count = 0U;
            Chb_DisableAllPwm();
            Chb_SetPrechargeBypass(0U);
            Chb_SetAcInput(1U);
            ControlFLAG.Bbits.Port_Start = 1U;
            ControlFLAG.Bbits.Port_Stop = 0U;
            Chb_SetLegacyStatus(PortStatusNum_Start);
        }
        break;

        case CHB_STATE_BYPASS_DELAY: // 旁路态：软起电阻已旁路，等待接触器稳定
        {
            Chb_SetPrechargeBypass(1U);
            Chb_SetLegacyStatus(PortStatusNum_Start_ContactorOnOver);
        }
        break;

        case CHB_STATE_WAIT_PLL: // 锁相等待态：闭环模式发波前的最后一道条件
        {
            s_pll_lock_10ms_count = 0U;
            Chb_SetLegacyStatus(PortStatusNum_Start_PortStart);
        }
        break;

        case CHB_STATE_RUNNING: // 运行态：AC PWM已解锁，DAB按命令单独使能
        {
            if (g_chb_dab_command.run_enable != 0U)
            {
                if (Chb_EnablePwm(1U) == 0U)
                {
                    /* PLL/母线在最终提交前失效时重新等待；FAULT 不允许被覆盖。 */
                    if ((mode == CHB_AC_MODE_CLOSED_LOOP) && (Chb_GateBlocked() == 0U))
                    {
                        Chb_EnterState(CHB_STATE_WAIT_PLL);
                    }
                    return;
                }
            }
            else
            {
                /* 允许仅用 dab_enable 验证DAB原边，不开启CHB交流PWM。 */
                g_chb_dab_status.ac_pwm_enabled = 0U;
            }
            if (g_chb_dab_command.dab_enable != 0U)
            {
                Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
                (void)Chb_EnablePwm(2U);
            }
            ControlFLAG.Bbits.Port_Start = 2U;
            ControlFLAG.Bbits.Port_Stop = 0U;
            Chb_SetLegacyStatus(PortStatusNum_Run);
            /* PWM 门控、DAB初值和兼容状态均已提交；下一次TAD直接执行控制计算。 */
        }
        break;

        case CHB_STATE_FAULT: // 故障态：封波、断进线、断旁路
        default:
        {
            Chb_DisableAllPwm();
            if (mode != CHB_AC_MODE_OPEN_LOOP)
            {
                Chb_SetPrechargeBypass(0U);
                Chb_SetAcInput(0U);
            }
            ControlFLAG.Bbits.Port_Start = 0U;
            ControlFLAG.Bbits.Port_Stop = 1U;
            Chb_SetLegacyStatus(PortStatusNum_Stop);
        }
        break;
    }
}

/*==================================================================
 * 函数功能: 检查模块数、周期、给定范围和控制参数是否有效
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 1 表示配置有效，0 表示非法
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
unsigned short Chb_ConfigurationValid(void)
{
    float correction_limit; /* correction_limit：PLL角速度修正允许范围，单位rad/s。 */

#if SST_PHASE_A_ONLY_TEST
    if (NHA_module_A == 0U || NHA_module_A > 14U)
    {
        return 0U;
    }
    if (NHA_module_B != 0U || NHA_module_C != 0U)
    {
        return 0U;
    }
#else
    if (NHA_module_A > 14U || NHA_module_B > 14U || NHA_module_C > 14U)
    {
        return 0U;
    }
    if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
    {
        if (NHA_module_A == 0U && NHA_module_B == 0U && NHA_module_C == 0U)
        {
            return 0U;
        }
    }
    else if (NHA_module_A == 0U || NHA_module_B == 0U || NHA_module_C == 0U)
    {
        return 0U;
    }
#endif

    if (g_chb_dab_command.ac_mode > CHB_AC_MODE_CLOSED_LOOP)
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_params.sample_period_s, 0.00009999F, 0.00010001F))
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_params.modulation_max, 0.000001F, 0.98F))
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_command.open_loop_modulation, -1.0F, 1.0F))
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_command.open_loop_frequency_hz, 0.0F, 200.0F))
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_params.pll_nominal_frequency_hz, 1.0F, 200.0F))
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_params.pll_kp, 0.0F, 1000000.0F))
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_params.pll_ki, 0.0F, 1000000.0F))
    {
        return 0U;
    }
    correction_limit = CHB_TWO_PI * (g_chb_dab_params.pll_nominal_frequency_hz - 1.0F);
    if (!Chb_FiniteInRange(g_chb_dab_params.pll_correction_limit_rad_s, 0.0F, correction_limit))
    {
        return 0U;
    }
    correction_limit = CHB_TWO_PI * (200.0F - g_chb_dab_params.pll_nominal_frequency_hz);
    if (g_chb_dab_params.pll_correction_limit_rad_s > correction_limit)
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_params.pll_lock_error_pu, 0.000001F, 1.0F))
    {
        return 0U;
    }
    if (!Chb_FiniteInRange(g_chb_dab_params.pll_min_grid_peak_v, 0.000001F, 1000000.0F))
    {
        return 0U;
    }
    if (g_chb_dab_params.dab_ramp_step_count == 0U || g_chb_dab_params.dab_start_count >= 5000U)
    {
        return 0U;
    }
    if (g_chb_dab_params.dab_target_duty_count > 2500U ||
        g_chb_dab_params.dab_start_duty_count > g_chb_dab_params.dab_target_duty_count)
    {
        return 0U;
    }

    if (g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP)
    {
        if (!Chb_FiniteInRange(g_chb_dab_params.precharge_voltage_v, 1.000001F, 1000000.0F))
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_command.dc_voltage_ref_v,
                               g_chb_dab_params.precharge_voltage_v, 1000000.0F))
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.dc_overvoltage_trip_v,
                               g_chb_dab_command.dc_voltage_ref_v + 0.000001F, 1000000.0F))
        {
            return 0U;
        }
        if (g_chb_dab_params.precharge_stable_10ms == 0U ||
            g_chb_dab_params.precharge_timeout_10ms == 0U ||
            g_chb_dab_params.pll_lock_10ms == 0U ||
            g_chb_dab_params.pll_timeout_10ms == 0U)
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.voltage_kp, 0.0F, 1000000.0F) ||
            !Chb_FiniteInRange(g_chb_dab_params.voltage_ki, 0.0F, 1000000.0F))
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.voltage_current_limit_a, 0.000001F, 1000000.0F))
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_command.iq_reference_a,
                               -g_chb_dab_params.voltage_current_limit_a,
                               g_chb_dab_params.voltage_current_limit_a))
        {
            return 0U;
        }
        if (g_chb_dab_params.startup_strategy > CHB_STARTUP_ID_RAMP)
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.startup_udc_offset_v, 0.0F, 1000000.0F) ||
            !Chb_FiniteInRange(g_chb_dab_params.startup_udc_step_v, 0.000001F, 1000000.0F))
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.startup_id_reference_a,
                               -g_chb_dab_params.voltage_current_limit_a,
                               g_chb_dab_params.voltage_current_limit_a) ||
            !Chb_FiniteInRange(g_chb_dab_params.startup_id_ramp_step_a, 0.000001F, 1000000.0F) ||
            g_chb_dab_params.startup_ramp_tad == 0U)
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.current_kp, -1000000.0F, 1000000.0F) ||
            !Chb_FiniteInRange(g_chb_dab_params.current_ki, -1000000.0F, 1000000.0F))
        {
            return 0U;
        }
        if (g_chb_dab_params.current_feedback_sign != 1.0F &&
            g_chb_dab_params.current_feedback_sign != -1.0F)
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.current_phase_compensation_rad,
                               -CHB_TWO_PI, CHB_TWO_PI) ||
            !Chb_FiniteInRange(g_chb_dab_params.output_phase_compensation_rad,
                               -CHB_TWO_PI, CHB_TWO_PI))
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.closed_loop_voltage_limit_factor,
                               0.000001F, 2.0F))
        {
            return 0U;
        }
        if (!Chb_FiniteInRange(g_chb_dab_params.ac_overcurrent_trip_a, 0.000001F, 1000000.0F) ||
            g_chb_dab_params.protection_confirm_samples == 0U)
        {
            return 0U;
        }
#if SST_LOW_VOLTAGE_100V_TEST
        if (g_chb_dab_params.voltage_current_limit_a > SST_LV100_RUN_CURRENT_LIMIT_A ||
            g_chb_dab_params.ac_overcurrent_trip_a <= g_chb_dab_params.voltage_current_limit_a ||
            g_chb_dab_params.ac_overcurrent_trip_a > SST_LV100_FAST_OC_A)
        {
            return 0U;
        }
#endif
    }
    return 1U;
}
/*==================================================================
 * 函数功能: 清空控制状态，撤销 PWM 使能并初始化开关及兼容状态
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_Init(void)
{
#if SST_PHASE_A_ONLY_TEST
    SstSinglePhase_Init(); /* 仅上电清零；预充、锁相及停机期间持续构造正交量。 */
#endif
    s_active_ac_mode = g_chb_dab_command.ac_mode;
    s_pll_loss_count = 0U;
    /* 初始化运行变量；随后经 Chb_EnterState() 撤销 PWM 使能并设置停止状态。 */
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
    g_chb_dab_status.ac_input_closed = 0U;
    g_chb_dab_status.precharge_bypassed = 0U;
    g_chb_dab_status.ac_pwm_enabled = 0U;
    g_chb_dab_status.dab_pwm_enabled = 0U;
    g_chb_dab_status.pll_locked = 0U;
    g_chb_dab_status.dab_duty_count = g_chb_dab_params.dab_start_duty_count;
    g_chb_dab_status.tick_10ms = 0UL;
    g_chb_dab_status.dc_voltage_average_v = 0.0F;
    g_chb_dab_status.dc_voltage_min_v = 0.0F;
    g_chb_dab_status.pll_angle_rad = 0.0F;
    g_chb_dab_status.pll_frequency_hz = g_chb_dab_params.pll_nominal_frequency_hz;
    g_chb_dab_status.id_feedback_a = 0.0F;
    g_chb_dab_status.iq_feedback_a = 0.0F;
    g_chb_dab_status.id_reference_a = 0.0F;
    g_chb_dab_status.startup_id_reference_a = 0.0F;
    g_chb_dab_status.startup_voltage_reference_v = 0.0F;
    g_chb_dab_status.startup_phase = CHB_STARTUP_IDLE;
    g_chb_dab_status.startup_tad_count = 0UL;
    g_chb_dab_status.voltage_loop_active = 0U;
    g_chb_dab_status.modulation_actual = 0.0F;
    g_chb_dab_status.switch_din_raw = 0U;
    g_chb_dab_status.ac_input_feedback = 0U;
    g_chb_dab_status.bypass_feedback = 0U;
    g_chb_dab_status.switch_stable_count = 0U;
    g_chb_dab_status.switch_mismatch_count = 0U;

    s_pll_integral = 0.0F;
    s_open_loop_angle = 0.0F;
    s_state_enter_tick = 0UL;
    s_breaker_pulse_end_tick = 0UL;
    s_last_service_tick = 0UL;
    s_precharge_stable_count = 0U;
    s_pll_lock_fast_count = 0U;
    s_pll_lock_10ms_count = 0U;
    s_ac_overcurrent_count = 0U;
    s_dc_overvoltage_count = 0U;
    SstPi_Init(&s_voltage_pi);
    SstPi_Init(&s_id_pi);
    SstPi_Init(&s_iq_pi);
    Chb_ResetControllers();

    g_chb_dab_command.run_enable = 0U;
    g_chb_dab_command.dab_enable = 0U;
    g_chb_dab_command.fault_reset = 0U;
    Chb_EnterState(CHB_STATE_STOPPED);
}

/*==================================================================
 * 函数功能: 递增控制状态的 10 ms 节拍
 * 输入参数: 无
 * 输出参数: g_chb_dab_status.tick_10ms。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_Tick10ms(void)
{
    /* Timer2 中断每 10 ms 调用一次，状态机用该计数做超时和延时判断。 */
    g_chb_dab_status.tick_10ms++;
}

/*==================================================================
 * 函数功能: 按停止优先原则处理遥控启停字，受宏及存储互锁约束
 * 输入参数: start_word：原遥控启动字，按控制核心的端口位解释
 *           stop_word：原遥控停止字，停止优先于启动
 * 输出参数: g_chb_dab_command.run_enable、dab_enable；只提交命令。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_ApplyRemoteCommand(unsigned short start_word, unsigned short stop_word)
{
    /*
     * DPRAM/485 的旧启停位统一转换为简化命令。停止优先于启动，
     * 这样同一帧同时带有 start/stop 时不会误启动。
     */
    if ((stop_word & 0x0001U) != 0U)
    {
        /* 停机命令优先于启动命令，同时关闭AC和DAB启动请求。 */
        g_chb_dab_command.run_enable = 0U;
        g_chb_dab_command.dab_enable = 0U;
    }
    else if ((start_word & 0x0001U) != 0U)
    {
        /* 原有485启动位映射为：预充/AC运行和DAB原边同时请求。 */
        g_chb_dab_command.run_enable = 1U;
        g_chb_dab_command.dab_enable = 1U;
    }
}

/*==================================================================
 * 函数功能: 锁存控制故障并关闭 PWM，进入故障状态
 * 输入参数: fault_code：控制核心 CHB_FAULT_* 故障编号
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_ForceFault(unsigned short fault_code)
{
    unsigned int interrupt_state; /* interrupt_state：首故障锁存及状态发布的原 CPU 中断状态。 */
    /* 软件或 FPGA 故障统一从这里进入 FAULT，禁止重复改写已锁存故障。 */
    if (fault_code == CHB_FAULT_NONE)
    {
        return;
    }

    interrupt_state = IntGlobalDisable();
    if ((g_chb_dab_status.state == CHB_STATE_FAULT) || (g_chb_dab_status.fault_code != CHB_FAULT_NONE))
    {
        IntGlobalRestore(interrupt_state);
        return;
    }

    g_chb_dab_status.fault_code = fault_code;
    g_chb_dab_command.run_enable = 0U;
    g_chb_dab_command.dab_enable = 0U;
    /* 先发布 FAULT，防止恢复中断后下一拍 TAD 继续执行旧运行态闭环。 */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    IntGlobalRestore(interrupt_state);
    Chb_EnterState(CHB_STATE_FAULT);
}
