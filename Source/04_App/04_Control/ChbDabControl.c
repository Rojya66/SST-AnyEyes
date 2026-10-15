#include "FpgaIo.h"
#include "SstPortStatus.h"
#include "PwmGate.h"
#include "ControlData.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include <math.h>

#include "FaultCheck.h"
#include "ChbDabControl.h"
#include "SstSinglePhase.h"

/* CHB_PI：π 浮点常量。 */
#define CHB_PI 3.14159265358979323846F
/* CHB_TWO_PI：2π 浮点常量，用于角度周期归一化。 */
#define CHB_TWO_PI 6.28318530717958647692F
/* CHB_ONE_BY_SQRT3：1/√3 浮点常量，用于 Clarke 变换。 */
#define CHB_ONE_BY_SQRT3 0.57735026918962576451F
/* CHB_SQRT3_BY_TWO：√3/2 浮点常量，用于三相逆变换。 */
#define CHB_SQRT3_BY_TWO 0.86602540378443864676F
/* CHB_REFERENCE_MIN：原 FPGA 工频桥归一化参考下限 0.02。 */
#define CHB_REFERENCE_MIN 0.02F
/* CHB_REFERENCE_MAX：原 FPGA 工频桥归一化参考上限 1.98。 */
#define CHB_REFERENCE_MAX 1.98F

typedef struct
{
    float integral; /* integral：PI 积分状态，按 Ki*Ts*error 累加；输出及积分限幅由对应调用者执行。 */
} CHB_PI_STATE;

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
    CHB_AC_MODE_OPEN_LOOP, //CHB_AC_MODE_CLOSED_LOOP,    /* ac_mode */
    0U, /* fault_reset */
    0.20F, /* open_loop_modulation */
    50.0F, /* open_loop_frequency_hz */
    680.0F, /* dc_voltage_ref_v */
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

    350.0F, /* precharge_voltage_v */
    20U, /* precharge_stable_10ms: 200 ms */
    1000U, /* precharge_timeout_10ms: 10 s */
    20U, /* bypass_delay_10ms: 200 ms */
    50U, /* breaker_pulse_10ms: 500 ms */

    50.0F, /* pll_nominal_frequency_hz */
    80.0F, /* pll_kp, normalized q error */
    4000.0F, /* pll_ki, normalized q error */
    62.831853F, /* pll correction limit: +/-10 Hz */
    0.02F, /* pll_lock_error_pu */
    100.0F, /* pll_min_grid_peak_v */
    20U, /* pll_lock_10ms: 200 ms */
    500U, /* pll_timeout_10ms: 5 s after bypass */

    0.099375F, /* voltage_kp = -Volt_PID_Num_0_def */
    12.5F, /* voltage_ki = (Num_1 + Num_0) / 0.0001 */
    95.0F, /* voltage_current_limit_a */

    //CHB_STARTUP_ID_RAMP, /* startup_strategy: 先投Idref，再由电压环接管 */
    CHB_STARTUP_SOURCE_COMPATIBLE,
    20.0F, /* startup_udc_offset_v: 与C6748Test一致，当前电压+20V */
    2.0F, /* startup_udc_step_v: 电压给定每个TAD最多增加2V */
    10.0F, /* startup_id_reference_a: 启动阶段固定Id目标 */
    0.05F, /* startup_id_ramp_step_a: 每个TAD增加的Id参考 */
    2000U, /* startup_ramp_tad: 0.2s后强制进入电压环接管 */

    -1.249166667F, /* current_kp = -Current_PID_Num_0_def */
    -16.66666667F, /* current_ki = (Num_1 + Num_0) / 0.0001 */
    1.10F, /* source per-phase voltage limit factor */
    1.0F, /* current_feedback_sign */
    0.1085F, /* SoftwarePhaseComValue */
    0.16F, /* PhaseComValue */

    120.0F, /* ac_overcurrent_trip_a：每TAD相电流绝对值门槛，单位A。 */
    780.0F, /* dc_overvoltage_trip_v：三相取各相模块均压最大值，单相只检查 A 相，单位V。 */
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

static CHB_PI_STATE s_voltage_pi; /* s_voltage_pi：直流电压外环的历史积分状态。 */
static CHB_PI_STATE s_id_pi; /* s_id_pi：d 轴电流内环的历史积分状态。 */
static CHB_PI_STATE s_iq_pi; /* s_iq_pi：q 轴电流内环的历史积分状态。 */
static float s_pll_integral; /* s_pll_integral：PLL 角速度修正的积分状态，单位 rad/s。 */
static float s_open_loop_angle; /* s_open_loop_angle：开环输出当前相角，单位 rad。 */
static float s_id_reference; /* s_id_reference：控制核心当前 Id 给定，单位 A。 */
static unsigned long s_state_enter_tick; /* s_state_enter_tick：当前状态进入时的 10 ms 节拍。 */
static unsigned long s_breaker_pulse_end_tick; /* s_breaker_pulse_end_tick：断路器控制脉冲结束的 10 ms 节拍。 */
static unsigned long s_last_service_tick; /* s_last_service_tick：主循环上次推进状态机的 10 ms 节拍。 */
static unsigned short s_precharge_stable_count; /* s_precharge_stable_count：预充连续达标的 10 ms 拍数。 */
static unsigned short s_pll_lock_fast_count; /* s_pll_lock_fast_count：PLL 连续锁定的 TAD 样本计数。 */
static unsigned short s_pll_lock_10ms_count; /* s_pll_lock_10ms_count：PLL 连续锁定的 10 ms 确认计数。 */
static unsigned short s_ac_overcurrent_count; /* s_ac_overcurrent_count：新增交流过流连续超限样本数。 */
static unsigned short s_dc_overvoltage_count; /* s_dc_overvoltage_count：新增直流过压连续超限样本数。 */
static unsigned short s_startup_phase; /* s_startup_phase：核心内部闭环启动阶段编号。 */
static unsigned long s_startup_tad_count; /* s_startup_tad_count：核心内部启动斜坡 TAD 计数。 */
static float s_startup_voltage_reference; /* s_startup_voltage_reference：核心内部启动电压给定，单位 V。 */

/*==================================================================
 * 函数功能: 把输入值限制在指定上下限之间
 * 输入参数: value：当前限幅/控制计算输入，与对应反馈或目标同单位
 *           minimum：限幅下界，与 value 同单位
 *           maximum：限幅上界，与 value 同单位
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 限幅后的浮点值
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static float Chb_Clamp(float value, float minimum, float maximum)
{
    /* 所有 PI、调制比和输出电压均通过统一函数限幅，避免散落的边界判断。 */
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
 * 输入参数: value：当前限幅/控制计算输入，与对应反馈或目标同单位
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 非负绝对值
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static float Chb_Abs(float value)
{
    return (value < 0.0F) ? -value : value;
}

/*==================================================================
 * 函数功能: 比较三个浮点量并取最小值
 * 输入参数: a：三量比较的第一个输入，与其余输入同单位
 *           b：三量比较的第二个输入，与其余输入同单位
 *           c：三量比较的第三个输入，与其余输入同单位
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 三者最小值
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
#if !SST_PHASE_A_ONLY_TEST
static float Chb_Min3(float a, float b, float c)
{
    float result; /* result：已比较输入中的当前最小值。 */

    result = (a < b) ? a : b;
    return (result < c) ? result : c;
}
#endif

/*==================================================================
 * 函数功能: 比较三个浮点量并取最大值
 * 输入参数: a：三量比较的第一个输入，与其余输入同单位
 *           b：三量比较的第二个输入，与其余输入同单位
 *           c：三量比较的第三个输入，与其余输入同单位
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 三者最大值
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static float Chb_Max3(float a, float b, float c)
{
    float result; /* result：已比较输入中的当前最大值。 */

    result = (a > b) ? a : b;
    return (result > c) ? result : c;
}

/*==================================================================
 * 函数功能: 将角度归一化到 0 至 2π 的周期区间
 * 输入参数: angle：待归一化角度，单位 rad
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 归一化的弧度角
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static float Chb_WrapAngle(float angle)
{
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
 * 函数功能: 按原增量 PI 等效的位置式更新积分项并计算本拍输出；限幅由调用者执行
 * 输入参数: state：输入/输出 PI 状态指针，保存积分项 integral
 *           error：本拍给定减反馈的控制误差，单位取决于电压环或电流环
 *           kp：PI 比例系数
 *           ki：PI 积分系数，计算时乘参数 period
 *           period：控制采样周期，单位 s
 * 输出参数: 更新 state->integral；本函数不执行输出限幅。
 * 返回值: kp*error+integral 的本拍 PI 输出，本函数内不额外限幅
 * 执行说明: TAD 快速控制子函数；外环在调用处限幅 Id，内环通过相电压限幅，系数符号保持原方向。
 *================================================================*/
static float Chb_SourceEquivalentPiStep(CHB_PI_STATE *state, float error, float kp, float ki, float period)
{
    /*
     * 标准位置式 PI：
     *   integral(k) = integral(k-1) + Ki * Ts * error(k)
     *   output(k)   = Kp * error(k) + integral(k)
     *
     * 这里没有额外输出限幅；外环在调用处限幅 Idref，内环通过相电压限幅。
     * Kp/Ki 的符号已经按 C6748Test 原程序方向换算，不能随意改成正值。
     */
    state->integral += ki * error * period;
    return kp * error + state->integral;
}

/*==================================================================
 * 函数功能: 清空电压、电流 PI 及参考状态
 * 输入参数: 无
 * 输出参数: s_voltage_pi、s_id_pi、s_iq_pi 及电流给定历史状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_ResetControllers(void)
{
    /* 停机或重新投运时清空 PI 积分，避免上一次运行的积分造成第一拍跳变。 */
    s_voltage_pi.integral = 0.0F;
    s_id_pi.integral = 0.0F;
    s_iq_pi.integral = 0.0F;
    s_id_reference = 0.0F;
    g_chb_dab_status.id_reference_a = 0.0F;
}

/*
 * 启动阶段只在闭环AC运行入口重新建立一次。
 * 这样PWM解锁后的第一个TAD不会继承上一次运行的Id给定或电压PI积分。
 */
/*==================================================================
 * 函数功能: 复位 Id 斜坡与电压环接管阶段的历史状态
 * 输入参数: 无
 * 输出参数: 内部启动阶段、拍计数、给定及对应状态镜像。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_ResetStartupSequence(void)
{
    /* 闭环 PWM 解锁时重新建立启动阶段观察量和临时电压参考。 */
    s_startup_phase = CHB_STARTUP_IDLE;
    s_startup_tad_count = 0UL;
    s_startup_voltage_reference = g_chb_dab_status.dc_voltage_average_v + g_chb_dab_params.startup_udc_offset_v;

    g_chb_dab_status.startup_id_reference_a = 0.0F;
    g_chb_dab_status.startup_voltage_reference_v = s_startup_voltage_reference;
    g_chb_dab_status.startup_phase = CHB_STARTUP_IDLE;
    g_chb_dab_status.startup_tad_count = 0UL;
    g_chb_dab_status.voltage_loop_active = 0U;
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
static void Chb_AlignVoltagePiToIdReference(float voltage_error, float id_reference)
{
    /*
     * 电压环接管前预置积分项，使 PI 当前输出恰好等于现有 Idref：
     *   integral = Idref - Kp * error
     * 这样从固定 Idref 切换到电压外环时不会产生给定阶跃。
     */
    s_voltage_pi.integral = id_reference - g_chb_dab_params.voltage_kp * voltage_error;
    s_voltage_pi.integral = Chb_Clamp(
        s_voltage_pi.integral, -g_chb_dab_params.voltage_current_limit_a, g_chb_dab_params.voltage_current_limit_a);
}

/* 参照C6748Test：启动后先用当前电压+20V，再以2V步长靠近最终给定。 */
/*==================================================================
 * 函数功能: 按启动策略更新直流电压外环给定及状态镜像
 * 输入参数: 无
 * 输出参数: 内部启动电压给定及 startup_voltage_reference_v 状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_UpdateVoltageReference(void)
{
    float target_voltage; /* target_voltage：最终直流电压目标，单位 V。 */
    float measured_voltage; /* measured_voltage：当前单模块平均实测直流电压，单位 V。 */

    target_voltage = g_chb_dab_command.dc_voltage_ref_v;
    measured_voltage = g_chb_dab_status.dc_voltage_average_v;

    /*
     * 启动阶段的电压给定不是一步跳到最终值：
     * - 实测值比目标低 10 V 以上时，参考每个 TAD 增加 startup_udc_step_v；
     * - 实测值比目标高 10 V 以上时，参考逐步下降；
     * - 进入目标附近的 20 V 窗口后，直接对齐最终给定。
     */
    if (measured_voltage < (target_voltage - 10.0F))
    {
        if (s_startup_voltage_reference < (target_voltage - 2.0F))
        {
            s_startup_voltage_reference += g_chb_dab_params.startup_udc_step_v;
            if (s_startup_voltage_reference > (target_voltage - 2.0F))
            {
                s_startup_voltage_reference = target_voltage - 2.0F;
            }
        }
    }
    else if (measured_voltage > (target_voltage + 10.0F))
    {
        if (s_startup_voltage_reference > (target_voltage + 2.0F))
        {
            s_startup_voltage_reference -= g_chb_dab_params.startup_udc_step_v;
            if (s_startup_voltage_reference < (target_voltage + 2.0F))
            {
                s_startup_voltage_reference = target_voltage + 2.0F;
            }
        }
    }
    else
    {
        s_startup_voltage_reference = target_voltage;
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
static void Chb_WriteNormalizedReferences(float phase_a, float phase_b, float phase_c)
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
static void Chb_WriteNeutralReferences(void)
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
static void Chb_WriteDabDuty(unsigned short duty)
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
static void Chb_SetAcInput(unsigned short closed)
{
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
}

/*==================================================================
 * 函数功能: 撤销已到期的交流输入断路器控制脉冲
 * 输入参数: 无
 * 输出参数: Port_DOUT 断路器合/分闸脉冲及对应数字输出寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_ClearAcInputPulse(void)
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
static void Chb_SetPrechargeBypass(unsigned short closed)
{
    /* 软起电阻旁路是保持量，闭环预充达标后置 1，停机/故障时置 0。 */
    Port_DOUT.Bbits.Contactor_Inner = (closed != 0U) ? 1U : 0U;
    g_chb_dab_status.precharge_bypassed = (closed != 0U) ? 1U : 0U;
    Port_IO_Out();
}

/*==================================================================
 * 函数功能: 撤销工频桥和 DAB PWM 使能并清空状态镜像
 * 输入参数: 无
 * 输出参数: 原模块 PWM 控制寄存器、ac_pwm_enabled 和 dab_pwm_enabled。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_DisableAllPwm(void)
{
    /*
     * 统一封锁 CHB+DAB：先把 FaultCheck 的封波类型设为“端口全部封波”，
     * 再清零软件状态、调制参考、DAB 占空比和 PI 积分。
     */
    FaultDisposeFLAG.Bbits.PWMPulseState = 3U;
    PWMPluse_Disable(0U);
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
static void Chb_SetLegacyStatus(unsigned short port_status)
{
    /* 同步旧 HVAC 状态变量，保证原有 DPRAM 遥测和故障页面仍能看到状态。 */
    Port_RunStatus = port_status;
    Port_State = port_status;
    Port_MainStatus.Bbits.P_Status = port_status;
    Port_StateUpdate_Write();
}

/*==================================================================
 * 函数功能: 切换控制状态并记录状态进入的 10 ms 节拍
 * 输入参数: state：CHB_STATE_* 目标状态
 * 输出参数: g_chb_dab_status.state 及 s_state_enter_tick。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_EnterState(unsigned short state)
{
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

    switch (state)
    {
        case CHB_STATE_STOPPED: // 停止态：AC/DAB封波，交流进线和旁路断开
        {
            Chb_DisableAllPwm();
            /* 开环测试不接管外部开关量，避免单板调试时误动作交流接触器。 */
            if (g_chb_dab_command.ac_mode != CHB_AC_MODE_OPEN_LOOP)
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
            Chb_ResetControllers();
            Chb_ResetStartupSequence();
            if ((g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP) &&
                (g_chb_dab_params.startup_strategy == CHB_STARTUP_ID_RAMP))
            {
                /* 闭环先固定Idref斜坡，电压外环暂不更新积分器。 */
                s_startup_phase = CHB_STARTUP_ID_RAMP_PHASE;
            }
            else
            {
                /* 开环或兼容模式不经过Idref启动阶段。 */
                s_startup_phase = CHB_STARTUP_NORMAL;
                g_chb_dab_status.voltage_loop_active = (g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP) ? 1U : 0U;
            }
            g_chb_dab_status.startup_phase = s_startup_phase;
            if (g_chb_dab_command.run_enable != 0U)
            {
                Chb_WriteNeutralReferences(); // 首个载波先使用中性参考
                PWMPluse_Enable(1U); // 解锁AC侧模块PWM
                g_chb_dab_status.ac_pwm_enabled = 1U;
            }
            else
            {
                /* 允许仅用 dab_enable 验证DAB原边，不开启CHB交流PWM。 */
                g_chb_dab_status.ac_pwm_enabled = 0U;
            }
            if (g_chb_dab_command.dab_enable != 0U)
            {
                Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
                PWMPluse_Enable(2U);
                g_chb_dab_status.dab_pwm_enabled = 1U;
            }
            ControlFLAG.Bbits.Port_Start = 2U;
            ControlFLAG.Bbits.Port_Stop = 0U;
            Chb_SetLegacyStatus(PortStatusNum_Run);
        }
        break;

        case CHB_STATE_FAULT: // 故障态：封波、断进线、断旁路
        default:
        {
            Chb_DisableAllPwm();
            if (g_chb_dab_command.ac_mode != CHB_AC_MODE_OPEN_LOOP)
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
static unsigned short Chb_ConfigurationValid(void)
{
    /*
     * 这里只检查软件无法工作的基础配置，例如模块数为 0、采样周期非法、
     * DAB 目标占空比越界。预充时间、PLL 锁定时间和闭环 PI 限幅只在闭环
     * 模式检查，保证开环调试不被未使用的闭环参数阻塞。
     */
#if SST_PHASE_A_ONLY_TEST
    /* 单相开环/闭环均只允许 A 相；周期固定为原 FPGA 外部中断的 100 µs。 */
    if ((NHA_module_A == 0U) || (NHA_module_B != 0U) || (NHA_module_C != 0U) ||
        !(g_chb_dab_params.sample_period_s >= 0.00009999F && g_chb_dab_params.sample_period_s <= 0.00010001F) ||
        !(g_chb_dab_params.pll_nominal_frequency_hz >= 1.0F && g_chb_dab_params.pll_nominal_frequency_hz <= 200.0F))
    {
        return 0U;
    }
#else
    if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
    {
        /* 单板开环验证只要求至少存在一个有效 UNIT。 */
        if ((NHA_module_A == 0U) && (NHA_module_B == 0U) && (NHA_module_C == 0U))
        {
            return 0U;
        }
    }
    else if ((NHA_module_A == 0U) || (NHA_module_B == 0U) || (NHA_module_C == 0U))
    {
        /* 闭环三相控制仍要求 A/B/C 三相均有模块。 */
        return 0U;
    }
#endif
    if ((g_chb_dab_params.sample_period_s <= 0.0F) || (g_chb_dab_params.modulation_max <= 0.0F) ||
        (g_chb_dab_params.modulation_max > 0.98F) || (g_chb_dab_params.protection_confirm_samples == 0U) ||
        (g_chb_dab_params.dab_ramp_step_count == 0U) || (g_chb_dab_params.dab_target_duty_count > 2500U) ||
        (g_chb_dab_params.dab_target_duty_count < g_chb_dab_params.dab_start_duty_count))
    {
        return 0U;
    }

    /* 闭环才检查预充、锁相、外环限幅和启动斜坡参数。 */
    if (g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP)
    {
        if ((g_chb_dab_params.closed_loop_voltage_limit_factor <= 0.0F) ||
            (g_chb_dab_params.precharge_stable_10ms == 0U) || (g_chb_dab_params.pll_lock_10ms == 0U) ||
            (g_chb_dab_params.startup_strategy > CHB_STARTUP_ID_RAMP))
        {
            return 0U;
        }

        if ((g_chb_dab_params.startup_strategy == CHB_STARTUP_ID_RAMP) &&
            ((g_chb_dab_params.startup_udc_step_v <= 0.0F) || (g_chb_dab_params.startup_id_ramp_step_a <= 0.0F) ||
             (g_chb_dab_params.startup_ramp_tad == 0U)))
        {
            return 0U;
        }
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
    /* 软件或 FPGA 故障统一从这里进入 FAULT，禁止重复改写已锁存故障。 */
    if (fault_code == CHB_FAULT_NONE)
    {
        return;
    }

    if (g_chb_dab_status.state == CHB_STATE_FAULT)
    {
        return;
    }

    g_chb_dab_status.fault_code = fault_code;
    g_chb_dab_command.run_enable = 0U;
    g_chb_dab_command.dab_enable = 0U;
    Chb_EnterState(CHB_STATE_FAULT);
}

/*
 * 仿真器开环快速路径。
 *
 * 开环测试的目标是验证 FPGA/UNIT 是否能发出 CHB 或 DAB 波形，因此不经过
 * 交流进线、软起旁路、预充电电压和 PLL 条件。run_enable 与 dab_enable 可以
 * 独立使用：前者只解锁 CHB 交流 PWM，后者只解锁 DAB 原边 PWM。
 */
/*==================================================================
 * 函数功能: 处理开环模式下独立 CHB/DAB 启停及故障复位
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_ServiceOpenLoop(void)
{
    /*
     * 开环不经过 PRECHARGE/BYPASS/WAIT_PLL：
     * run_enable 只控制 CHB，dab_enable 只控制 DAB，两个命令可独立验证。
     * 仍保留基础模块配置检查和故障封波，避免写入明显非法的 FPGA 参数。
     */
    if ((g_chb_dab_command.run_enable == 0U) && (g_chb_dab_command.dab_enable == 0U))
    {
        if (g_chb_dab_status.state != CHB_STATE_STOPPED)
        {
            Chb_EnterState(CHB_STATE_STOPPED);
        }
        return;
    }

    if (g_chb_dab_status.state != CHB_STATE_RUNNING)
    {
        /* 只保留模块数量/调制参数检查，不读取预充电和锁相状态。 */
        if (Chb_ConfigurationValid() == 0U)
        {
            ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
            return;
        }
        Chb_EnterState(CHB_STATE_RUNNING);
        return;
    }

    /* run_enable 可在 DAB 已运行时单独打开/关闭 CHB。 */
    if ((g_chb_dab_command.run_enable != 0U) && (g_chb_dab_status.ac_pwm_enabled == 0U))
    {
        Chb_WriteNeutralReferences();
        PWMPluse_Enable(1U);
        g_chb_dab_status.ac_pwm_enabled = 1U;
    }
    else if ((g_chb_dab_command.run_enable == 0U) && (g_chb_dab_status.ac_pwm_enabled != 0U))
    {
        FaultDisposeFLAG.Bbits.PWMPulseState = 1U;
        PWMPluse_Disable(0U);
        g_chb_dab_status.ac_pwm_enabled = 0U;
        Chb_WriteNeutralReferences();
    }

    /* dab_enable 可独立启动/停止DAB，启动后仍按原逻辑逐步爬升到50%。 */
    if ((g_chb_dab_command.dab_enable != 0U) && (g_chb_dab_status.dab_pwm_enabled == 0U))
    {
        Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
        PWMPluse_Enable(2U);
        g_chb_dab_status.dab_pwm_enabled = 1U;
    }
    else if ((g_chb_dab_command.dab_enable == 0U) && (g_chb_dab_status.dab_pwm_enabled != 0U))
    {
        FaultDisposeFLAG.Bbits.PWMPulseState = 2U;
        PWMPluse_Disable(0U);
        g_chb_dab_status.dab_pwm_enabled = 0U;
        Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
    }
}

/*==================================================================
 * 函数功能: 每个 10 ms 节拍推进预充、旁路、PLL 等待、运行和停机状态
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_Service(void)
{
    unsigned long elapsed; /* elapsed：当前状态已经历的 10 ms 节拍数。 */
    unsigned short state; /* state：CHB_STATE_* 运行状态编号。 */

    /*
     * 开环命令不等待 10 ms 慢任务，仿真器写入后主循环下一次即可生效；
     * 闭环仍严格按 10 ms 节拍运行，避免重复执行接触器和超时计数。
     */
    if ((g_chb_dab_command.ac_mode != CHB_AC_MODE_OPEN_LOOP) && (s_last_service_tick == g_chb_dab_status.tick_10ms))
    {
        return;
    }
    s_last_service_tick = g_chb_dab_status.tick_10ms;

    if ((s_breaker_pulse_end_tick != 0UL) && (g_chb_dab_status.tick_10ms >= s_breaker_pulse_end_tick))
    {
        s_breaker_pulse_end_tick = 0UL;
        Chb_ClearAcInputPulse();
    }

    state = g_chb_dab_status.state;
    elapsed = g_chb_dab_status.tick_10ms - s_state_enter_tick;

    if ((Port_RealFault.Bbits.PORT_FAULT_bit15 != 0U) && (state != CHB_STATE_FAULT))
    {
        /* FPGA端口故障不等待软件计数，立即进入故障态。 */
        ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
        return;
    }

    if (state == CHB_STATE_FAULT)
    {
        /* 故障态只接受“停机状态下的故障复位”，不会边发波边清故障。 */
        if ((g_chb_dab_command.fault_reset != 0U) && (g_chb_dab_command.run_enable == 0U))
        {
            /* 只有PWM已停、启动命令已撤销时才允许清除故障锁存。 */
            g_chb_dab_command.fault_reset = 0U;
            g_chb_dab_status.fault_code = CHB_FAULT_NONE;
            Port_RealFault.PORT_FAULT16 = 0U;
            FaultLatch_Clear(0U, 1U);
            Chb_EnterState(CHB_STATE_STOPPED);
        }
        return;
    }

    if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
    {
        Chb_ServiceOpenLoop();
        return;
    }

    if ((g_chb_dab_command.run_enable == 0U) && (state != CHB_STATE_STOPPED))
    {
        Chb_EnterState(CHB_STATE_STOPPED);
        return;
    }

    switch (state)
    {
        case CHB_STATE_STOPPED:
        {
            if (g_chb_dab_command.run_enable != 0U)
            {
                /* 闭环启动先做基础检查，合格后才闭合交流进线进入预充。 */
                if (Chb_ConfigurationValid() != 0U)
                {
                    Chb_EnterState(CHB_STATE_PRECHARGE);
                }
                else
                {
                    ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
                }
            }
        }
        break;

        case CHB_STATE_PRECHARGE: // 检查三相最低单元电压是否连续达标
        {
            /*
             * 用三相最低单元电压做预充判据：必须连续达到门槛才旁路；
             * 超过超时时间仍未达标则进入故障，防止某一相未充电却强行旁路。
             */
            if (g_chb_dab_status.dc_voltage_min_v >= g_chb_dab_params.precharge_voltage_v)
            {
                if (s_precharge_stable_count < 0xFFFFU)
                {
                    s_precharge_stable_count++;
                }
            }
            else
            {
                s_precharge_stable_count = 0U;
            }

            if (s_precharge_stable_count >= g_chb_dab_params.precharge_stable_10ms)
            {
                Chb_EnterState(CHB_STATE_BYPASS_DELAY);
            }
            else if (elapsed >= g_chb_dab_params.precharge_timeout_10ms)
            {
                ChbDabControl_ForceFault(CHB_FAULT_PRECHARGE_TIMEOUT);
            }
        }
        break;

        case CHB_STATE_BYPASS_DELAY: // 旁路延时结束后，按AC模式选择下一状态
        {
            /* 旁路闭合后留出接触器机械稳定时间，再决定开环直达运行还是闭环等 PLL。 */
            if (elapsed >= g_chb_dab_params.bypass_delay_10ms)
            {
                if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
                {
                    /* 开环测试不依赖电网相位，旁路稳定后直接进入RUNNING。 */
                    Chb_EnterState(CHB_STATE_RUNNING);
                }
                else
                {
                    /* 闭环测试先等PLL锁定，再允许AC PWM。 */
                    Chb_EnterState(CHB_STATE_WAIT_PLL);
                }
            }
        }
        break;

        case CHB_STATE_WAIT_PLL: // PLL连续锁定后才允许进入运行态
        {
            /* PLL 必须连续保持锁定，短暂丢锁会清零计数而不是立即发波。 */
            /* PLL锁定必须连续保持指定的10 ms周期数。 */
            if (g_chb_dab_status.pll_locked != 0U)
            {
                if (s_pll_lock_10ms_count < 0xFFFFU)
                {
                    s_pll_lock_10ms_count++;
                }
            }
            else
            {
                s_pll_lock_10ms_count = 0U;
            }

            if (s_pll_lock_10ms_count >= g_chb_dab_params.pll_lock_10ms)
            {
                Chb_EnterState(CHB_STATE_RUNNING);
            }
            else if (elapsed >= g_chb_dab_params.pll_timeout_10ms)
            {
                ChbDabControl_ForceFault(CHB_FAULT_PLL_TIMEOUT);
            }
        }
        break;

        case CHB_STATE_RUNNING: // 运行态只处理DAB的独立启停
        {
            /* 闭环运行态不再重复判断预充，只处理 DAB 的独立启动和停止。 */
            if ((g_chb_dab_command.dab_enable != 0U) && (g_chb_dab_status.dab_pwm_enabled == 0U))
            {
                /* DAB只启动原边：先给350计数，再在TAD中逐步爬升到50%。 */
                Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
                PWMPluse_Enable(2U);
                g_chb_dab_status.dab_pwm_enabled = 1U;
            }
            else if ((g_chb_dab_command.dab_enable == 0U) && (g_chb_dab_status.dab_pwm_enabled != 0U))
            {
                /* 只撤销DAB PWM，AC侧仍由run_enable决定。 */
                Module_ENdab_PWMEN(0U);
                PortRunFLAG.Bbits.PWMState_Dab = 1U;
                g_chb_dab_status.dab_pwm_enabled = 0U;
                Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
            }
        }
        break;

        default:
        {
            ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
        }
        break;
    }
}

/*==================================================================
 * 函数功能: 依据 αβ 电压执行归一化 q 轴误差 PI，更新相角、频率及锁定计数
 * 输入参数: voltage_alpha：电网/端口电压 α 轴分量，单位 V
 *           voltage_beta：电网/端口电压 β 轴分量，单位 V
 * 输出参数: s_pll_integral、PLL 相角/频率以及锁定计数、状态镜像。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_UpdatePll(float voltage_alpha, float voltage_beta)
{
    float magnitude; /* magnitude：PLL 输入 αβ 电压矢量幅值，单位 V。 */
    float sine_angle; /* sine_angle：当前 PLL 相角的正弦值。 */
    float cosine_angle; /* cosine_angle：当前 PLL 相角的余弦值。 */
    float q_voltage; /* q_voltage：PLL 坐标变换的 q 轴电压，单位 V。 */
    float q_error; /* q_error：按电压矢量幅值归一化的 PLL q 轴误差。 */
    float correction; /* correction：PLL PI 输出的角速度修正，单位 rad/s。 */
    float omega; /* omega：电网额定角速度加 PLL 修正量，单位 rad/s。 */
    unsigned short lock_samples; /* lock_samples：由锁定时间及 sample_period_s 换算的所需 TAD 样本数。 */

    /*
     * 软件 PLL 输入为电网 alpha/beta 电压：
     * 1) 用当前角度计算 q 轴电压；
     * 2) q_voltage/magnitude 作为归一化相位误差；
     * 3) PI 输出频率修正量，并限制在 +/- correction_limit_rad_s；
     * 4) 积分得到下一采样周期的电网角度。
     * 电压幅值不足时不更新锁定计数，只按额定频率自由运行。
     */
    magnitude = sqrtf(voltage_alpha * voltage_alpha + voltage_beta * voltage_beta);
    sine_angle = sinf(g_chb_dab_status.pll_angle_rad);
    cosine_angle = cosf(g_chb_dab_status.pll_angle_rad);

    if (magnitude >= g_chb_dab_params.pll_min_grid_peak_v)
    {
        q_voltage = -voltage_alpha * sine_angle + voltage_beta * cosine_angle;
        q_error = q_voltage / magnitude; // q轴归一化误差，作为PLL校正输入
        s_pll_integral += g_chb_dab_params.pll_ki * q_error * g_chb_dab_params.sample_period_s;
        s_pll_integral = Chb_Clamp(
            s_pll_integral, -g_chb_dab_params.pll_correction_limit_rad_s, g_chb_dab_params.pll_correction_limit_rad_s);
        correction = g_chb_dab_params.pll_kp * q_error + s_pll_integral;
        correction = Chb_Clamp(
            correction, -g_chb_dab_params.pll_correction_limit_rad_s, g_chb_dab_params.pll_correction_limit_rad_s);
        omega = CHB_TWO_PI * g_chb_dab_params.pll_nominal_frequency_hz + correction;
        g_chb_dab_status.pll_angle_rad =
            Chb_WrapAngle(g_chb_dab_status.pll_angle_rad + omega * g_chb_dab_params.sample_period_s);
        g_chb_dab_status.pll_frequency_hz = omega / CHB_TWO_PI;

        lock_samples = (unsigned short)(0.010F / g_chb_dab_params.sample_period_s);
        if (lock_samples == 0U)
        {
            lock_samples = 1U;
        }
        if (Chb_Abs(q_error) <= g_chb_dab_params.pll_lock_error_pu)
        {
            if (s_pll_lock_fast_count < lock_samples)
            {
                s_pll_lock_fast_count++;
            }
        }
        else
        {
            s_pll_lock_fast_count = 0U;
        }
        g_chb_dab_status.pll_locked = (s_pll_lock_fast_count >= lock_samples) ? 1U : 0U;
    }
    else
    {
        s_pll_lock_fast_count = 0U;
        g_chb_dab_status.pll_locked = 0U;
        g_chb_dab_status.pll_frequency_hz = g_chb_dab_params.pll_nominal_frequency_hz;
        g_chb_dab_status.pll_angle_rad =
            Chb_WrapAngle(g_chb_dab_status.pll_angle_rad +
                          CHB_TWO_PI * g_chb_dab_params.pll_nominal_frequency_hz * g_chb_dab_params.sample_period_s);
    }
}

/*==================================================================
 * 函数功能: 推进开环正弦相角并写入三相限幅调制参考
 * 输入参数: 无
 * 输出参数: 开环相角、三相调制给定及运行调制幅值状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_RunOpenLoop(void)
{
    float modulation; /* modulation：限幅后的开环调制比。 */
    float phase_a; /* phase_a：A 相归一化参考 1+M*sin(theta)，无量纲。 */
    float phase_b; /* phase_b：B 相归一化参考 1+M*sin(theta)，无量纲。 */
    float phase_c; /* phase_c：C 相归一化参考 1+M*sin(theta)，无量纲。 */

    /*
     * 开环正弦发生器：angle(k+1)=angle(k)+2*pi*f*Ts，
     * 三相相差 +/-120 度，最后交给统一的 Reference 写入函数限幅。
     */
    modulation = Chb_Clamp(g_chb_dab_command.open_loop_modulation, 0.0F, g_chb_dab_params.modulation_max);
    /* 开环模式只使用设定频率和调制比，便于单板验证CHB发波。 */
    s_open_loop_angle = Chb_WrapAngle(s_open_loop_angle + CHB_TWO_PI * g_chb_dab_command.open_loop_frequency_hz *
                                                              g_chb_dab_params.sample_period_s);

    phase_a = 1.0F + modulation * sinf(s_open_loop_angle);
    phase_b = 1.0F + modulation * sinf(s_open_loop_angle - 2.0F * CHB_PI / 3.0F);
    phase_c = 1.0F + modulation * sinf(s_open_loop_angle + 2.0F * CHB_PI / 3.0F);
    Chb_WriteNormalizedReferences(phase_a, phase_b, phase_c);
}

/*
 * 计算闭环d轴电流给定。
 *
 * ID_RAMP阶段不调用电压PI，只让固定Id目标从0缓慢爬升；
 * HANDOVER阶段保持当前Id一拍；
 * NORMAL阶段才恢复C6748Test方向一致的直流电压外环。
 */
/*==================================================================
 * 函数功能: 执行兼容启动或 Id 斜坡，并在满足条件时移交电压外环
 * 输入参数: 无
 * 输出参数: Id 给定、启动阶段/拍数、外环接管标志及电压 PI 历史状态。
 * 返回值: 本拍闭环 d 轴电流给定，单位 A
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static float Chb_GetClosedLoopIdReference(void)
{
    float startup_target; /* startup_target：启动阶段最终 Id 目标，单位 A。 */
    float ramp_step; /* ramp_step：当前 Id 斜坡每拍增量，单位 A/TAD。 */
    float voltage_error; /* voltage_error：直流电压给定减反馈的误差，单位 V。 */
    float id_target; /* id_target：本拍计算出的 Id 目标，单位 A。 */
    unsigned short handover_now; /* handover_now：本拍需要从启动斜坡移交电压环的标志。 */

    /*
     * 闭环启动给定的三个阶段：
     * ID_RAMP：固定 Id 目标从 0 逐步爬升；
     * HANDOVER：保持一拍，观察电压 PI 接管点；
     * NORMAL：电压外环根据 Udc 误差产生 Idref，并限幅到电流上限。
     */
    handover_now = 0U;
    startup_target = Chb_Clamp(g_chb_dab_params.startup_id_reference_a,
                               -g_chb_dab_params.voltage_current_limit_a,
                               g_chb_dab_params.voltage_current_limit_a);
    ramp_step = Chb_Abs(g_chb_dab_params.startup_id_ramp_step_a);

    if (s_startup_phase == CHB_STARTUP_ID_RAMP_PHASE)
    {
        /* 固定Idref斜坡：只改变给定，不改变源程序PI方向。 */
        if (s_id_reference < startup_target)
        {
            s_id_reference += ramp_step;
            if (s_id_reference > startup_target)
            {
                s_id_reference = startup_target;
            }
        }
        else if (s_id_reference > startup_target)
        {
            s_id_reference -= ramp_step;
            if (s_id_reference < startup_target)
            {
                s_id_reference = startup_target;
            }
        }

        s_startup_tad_count++;
        if ((s_id_reference == startup_target) || (s_startup_tad_count >= g_chb_dab_params.startup_ramp_tad))
        {
            /* 让电压PI当前输出等于斜坡末值，避免接管时Idref跳变。 */
            voltage_error = s_startup_voltage_reference - g_chb_dab_status.dc_voltage_average_v;
            Chb_AlignVoltagePiToIdReference(voltage_error, s_id_reference);
            s_startup_phase = CHB_STARTUP_VOLTAGE_HANDOVER;
            g_chb_dab_status.voltage_loop_active = 0U;
            handover_now = 1U;
        }
    }

    if (s_startup_phase == CHB_STARTUP_VOLTAGE_HANDOVER)
    {
        /* 保持一拍固定Id，下一拍再执行电压PI，便于示波器分辨接管点。 */
        if (handover_now == 0U)
        {
            s_startup_phase = CHB_STARTUP_NORMAL;
            g_chb_dab_status.voltage_loop_active = 1U;
        }
    }

    if (s_startup_phase == CHB_STARTUP_NORMAL)
    {
        Chb_UpdateVoltageReference();
        voltage_error = s_startup_voltage_reference - g_chb_dab_status.dc_voltage_average_v;
        id_target = Chb_SourceEquivalentPiStep(&s_voltage_pi,
                                               voltage_error,
                                               g_chb_dab_params.voltage_kp,
                                               g_chb_dab_params.voltage_ki,
                                               g_chb_dab_params.sample_period_s);
        s_id_reference =
            Chb_Clamp(id_target, -g_chb_dab_params.voltage_current_limit_a, g_chb_dab_params.voltage_current_limit_a);
        g_chb_dab_status.voltage_loop_active = 1U;
    }

    g_chb_dab_status.startup_id_reference_a = s_id_reference;
    g_chb_dab_status.startup_phase = s_startup_phase;
    g_chb_dab_status.startup_tad_count = s_startup_tad_count;
    return s_id_reference;
}

/*==================================================================
 * 函数功能: 按原反馈方向执行 dq 电流闭环及电压前馈，输出三相调制参考
 * 输入参数: voltage_alpha：电网/端口电压 α 轴分量，单位 V
 *           voltage_beta：电网/端口电压 β 轴分量，单位 V
 * 输出参数: dq 电流反馈/PI 历史、三相调制参考以及对应控制状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_RunClosedLoop(float voltage_alpha, float voltage_beta)
{
    float current_sine_angle; /* current_sine_angle：加入电流相位补偿后的正弦值。 */
    float current_cosine_angle; /* current_cosine_angle：加入电流相位补偿后的余弦值。 */
    float voltage_sine_angle; /* voltage_sine_angle：电网电压 dq 变换的正弦值。 */
    float voltage_cosine_angle; /* voltage_cosine_angle：电网电压 dq 变换的余弦值。 */
    float output_sine_angle; /* output_sine_angle：加入输出相位补偿后的逆变换正弦值。 */
    float output_cosine_angle; /* output_cosine_angle：加入输出相位补偿后的逆变换余弦值。 */
    float current_alpha; /* current_alpha：端口电流 α 轴分量，单位 A。 */
    float current_beta; /* current_beta：端口电流 β 轴分量，单位 A。 */
    float current_d; /* current_d：端口电流 d 轴分量，单位 A。 */
    float current_q; /* current_q：端口电流 q 轴分量，单位 A。 */
    float voltage_d; /* voltage_d：电网电压 d 轴前馈分量，单位 V。 */
    float voltage_q; /* voltage_q：电网电压 q 轴前馈分量，单位 V。 */
    float vd_correction; /* vd_correction：d 轴电流 PI 电压修正，单位 V。 */
    float vq_correction; /* vq_correction：q 轴电流 PI 电压修正，单位 V。 */
    float command_d; /* command_d：限幅前/后的 d 轴输出电压指令，单位 V。 */
    float command_q; /* command_q：限幅前/后的 q 轴输出电压指令，单位 V。 */
    float command_alpha; /* command_alpha：输出电压 α 轴指令，单位 V。 */
#if !SST_PHASE_A_ONLY_TEST
    float command_beta; /* command_beta：三相输出电压 β 轴指令，单位 V。 */
#endif
    float phase_a; /* phase_a：A 相输出电压指令，单位 V；写入前换算为归一化参考。 */
#if !SST_PHASE_A_ONLY_TEST
    float phase_b; /* phase_b：B 相输出电压指令，单位 V；写入前换算为归一化参考。 */
    float phase_c; /* phase_c：C 相输出电压指令，单位 V；写入前换算为归一化参考。 */
#endif
    float phase_dc_a; /* phase_dc_a：A 相模块数乘单模块直流电压给定的总电压尺度，单位 V。 */
#if !SST_PHASE_A_ONLY_TEST
    float phase_dc_b; /* phase_dc_b：B 相模块数乘单模块直流电压给定的总电压尺度，单位 V。 */
    float phase_dc_c; /* phase_dc_c：C 相模块数乘单模块直流电压给定的总电压尺度，单位 V。 */
#endif

    /*
     * 闭环控制计算顺序：
     * 1) 采样三相电流/电压转换到 alpha-beta，再旋转到 dq；
     * 2) 直流电压外环给出 Idref，Iqref 由命令给出；
     * 3) d/q 电流 PI 输出电压修正量并叠加电网电压前馈；
     * 4) 逆 Park/Clarke 变换成三相电压命令；
     * 5) 按每相模块数和 Udc 限幅，归一化后写入 FPGA。
     */
    current_sine_angle = sinf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.current_phase_compensation_rad);
    current_cosine_angle = cosf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.current_phase_compensation_rad);
    voltage_sine_angle = sinf(g_chb_dab_status.pll_angle_rad);
    voltage_cosine_angle = cosf(g_chb_dab_status.pll_angle_rad);
    output_sine_angle = sinf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.output_phase_compensation_rad);
    output_cosine_angle = cosf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.output_phase_compensation_rad);
    /* 先把采样量变换到同步dq坐标系，电流极性由可调符号统一处理。 */
#if SST_PHASE_A_ONLY_TEST
    /* 两轴使用同一 SOGI 的滤波输出，并成对应用既有电流极性；虚拟 beta 不写入物理 B/C 采样。 */
    current_alpha = g_chb_dab_params.current_feedback_sign * g_sst_single_phase_status.current_alpha_a;
    current_beta = g_chb_dab_params.current_feedback_sign * g_sst_single_phase_status.current_beta_a;
#else
    current_alpha = g_chb_dab_params.current_feedback_sign * HA_Port_Current_A;
    current_beta =
        g_chb_dab_params.current_feedback_sign * (HA_Port_Current_A + 2.0F * HA_Port_Current_B) * CHB_ONE_BY_SQRT3;
#endif
    current_d = current_alpha * current_cosine_angle + current_beta * current_sine_angle;
    current_q = -current_alpha * current_sine_angle + current_beta * current_cosine_angle;
    voltage_d = voltage_alpha * voltage_cosine_angle + voltage_beta * voltage_sine_angle;
    voltage_q = -voltage_alpha * voltage_sine_angle + voltage_beta * voltage_cosine_angle;

    g_chb_dab_status.id_feedback_a = current_d;
    g_chb_dab_status.iq_feedback_a = current_q;

    /* 启动阶段先按固定Idref斜坡运行，接管后才投入直流电压外环。 */
    s_id_reference = Chb_GetClosedLoopIdReference();
    g_chb_dab_status.id_reference_a = s_id_reference;

    /* d/q电流内环输出交流电压修正量，再叠加电网电压前馈。 */
    vd_correction = Chb_SourceEquivalentPiStep(&s_id_pi,
                                               s_id_reference - current_d,
                                               g_chb_dab_params.current_kp,
                                               g_chb_dab_params.current_ki,
                                               g_chb_dab_params.sample_period_s);
    vq_correction = Chb_SourceEquivalentPiStep(&s_iq_pi,
                                               g_chb_dab_command.iq_reference_a - current_q,
                                               g_chb_dab_params.current_kp,
                                               g_chb_dab_params.current_ki,
                                               g_chb_dab_params.sample_period_s);

    /* Signed Kp/Ki preserve the source incremental PI direction, then the
     * source grid-voltage feed-forward is added. */
    command_d = voltage_d + vd_correction;
    command_q = voltage_q + vq_correction;

    command_alpha = command_d * output_cosine_angle - command_q * output_sine_angle;
#if SST_PHASE_A_ONLY_TEST
    /* 虚拟 dq 逆变换后只取真实 alpha=AN 输出，复用上面的电压外环、电流 PI 与电压前馈。 */
    phase_a = command_alpha;
    phase_dc_a = (float)NHA_module_A * g_chb_dab_command.dc_voltage_ref_v;
    if (phase_dc_a > 1.0F)
    {
        phase_a = Chb_Clamp(phase_a,
                            -g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_a,
                            g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_a);
        Chb_WriteNormalizedReferences(1.0F + phase_a / phase_dc_a, 1.0F, 1.0F);
    }
#else
    command_beta = command_d * output_sine_angle + command_q * output_cosine_angle;
    phase_a = command_alpha;
    phase_b = -0.5F * command_alpha + CHB_SQRT3_BY_TWO * command_beta;
    phase_c = -0.5F * command_alpha - CHB_SQRT3_BY_TWO * command_beta;

    phase_dc_a = (float)NHA_module_A * g_chb_dab_command.dc_voltage_ref_v;
    phase_dc_b = (float)NHA_module_B * g_chb_dab_command.dc_voltage_ref_v;
    phase_dc_c = (float)NHA_module_C * g_chb_dab_command.dc_voltage_ref_v;
    if ((phase_dc_a > 1.0F) && (phase_dc_b > 1.0F) && (phase_dc_c > 1.0F))
    {
        /* 按各相模块数进行电压限幅，再换算为FPGA Reference。 */
        phase_a = Chb_Clamp(phase_a,
                            -g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_a,
                            g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_a);
        phase_b = Chb_Clamp(phase_b,
                            -g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_b,
                            g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_b);
        phase_c = Chb_Clamp(phase_c,
                            -g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_c,
                            g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_c);
        Chb_WriteNormalizedReferences(
            1.0F + phase_a / phase_dc_a, 1.0F + phase_b / phase_dc_b, 1.0F + phase_c / phase_dc_c);
    }
#endif
    else
    {
        Chb_WriteNeutralReferences();
    }
}

/*==================================================================
 * 函数功能: 确认软件交流过流与直流过压连续样本并提交控制故障
 * 输入参数: 无
 * 输出参数: 连续超限计数；满足条件时经 ForceFault 进入故障并关闭 PWM。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
static void Chb_CheckSoftwareProtection(void)
{
    float maximum_current; /* maximum_current：三相端口电流绝对值中的最大值，单位 A。 */

    /*
     * 这是新增的软件快速保护；硬件故障回传和封波能力取决于现场 FPGA 位流。
     * NoInterlock 位流不能按完整硬件保护版本解释；本软件检查的判据为：
     * - 过流：每TAD标定相电流绝对值最大值达到门槛并连续确认，非200点RMS；
     * - 过压：三相各自模块平均Udc的最大值达到门槛并连续确认；
     * - 确认次数用于滤除单个 TAD 的采样毛刺。
     */
    if (g_chb_dab_protection_enable == 0U)
    {
        /* 仅关闭本函数的软件判断；不修改 FPGA 门控，硬件保护能力需按现场位流确认。 */
        s_ac_overcurrent_count = 0U;
        s_dc_overvoltage_count = 0U;
        return;
    }

#if SST_PHASE_A_ONLY_TEST
    /* 快速保护直接检查物理 A 相采样，不使用有滤波延迟的 SOGI 电流。 */
    maximum_current = Chb_Abs(HA_Port_Current_A);
#else
    maximum_current = Chb_Max3(Chb_Abs(HA_Port_Current_A), Chb_Abs(HA_Port_Current_B), Chb_Abs(HA_Port_Current_C));
#endif
    if (maximum_current >= g_chb_dab_params.ac_overcurrent_trip_a)
    {
        if (s_ac_overcurrent_count < 0xFFFFU)
        {
            s_ac_overcurrent_count++;
        }
    }
    else
    {
        s_ac_overcurrent_count = 0U;
    }

#if SST_PHASE_A_ONLY_TEST
    if (SVG_Udc_A_Ave >= g_chb_dab_params.dc_overvoltage_trip_v)
#else
    if (Chb_Max3(SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave) >= g_chb_dab_params.dc_overvoltage_trip_v)
#endif
    {
        if (s_dc_overvoltage_count < 0xFFFFU)
        {
            s_dc_overvoltage_count++;
        }
    }
    else
    {
        s_dc_overvoltage_count = 0U;
    }

    if (s_ac_overcurrent_count >= g_chb_dab_params.protection_confirm_samples)
    {
        /* 过流需连续确认若干个TAD，避免单点采样毛刺误停机。 */
        ChbDabControl_ForceFault(CHB_FAULT_AC_OVERCURRENT);
    }
    else if (s_dc_overvoltage_count >= g_chb_dab_params.protection_confirm_samples)
    {
        ChbDabControl_ForceFault(CHB_FAULT_DC_OVERVOLTAGE);
    }
}

/*==================================================================
 * 函数功能: 在原 TAD 路径执行 PLL、开环/闭环控制、软件保护及 DAB 占空比斜坡
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_FastStep(void)
{
    float voltage_alpha; /* voltage_alpha：电网/端口电压 α 轴分量，单位 V。 */
    float voltage_beta; /* voltage_beta：电网/端口电压 β 轴分量，单位 V。 */
    unsigned short duty; /* duty：本拍 DAB 占空比计数。 */
    unsigned short step; /* step：DAB 占空比每个 TAD 的递增计数。 */

    /*
     * TAD 快速任务入口。该函数在 FPGA 采样同步中断中调用，不能放入耗时的
     * 通讯或 EEPROM 操作；这里只做浮点控制计算和 FPGA 寄存器/内存映射写入。
     */

#if SST_PHASE_A_ONLY_TEST
    /* 单相预充、启动给定和电压外环均只读取 A 相模块均压。 */
    g_chb_dab_status.dc_voltage_average_v = SVG_Udc_A_Ave;
    g_chb_dab_status.dc_voltage_min_v = SVG_Udc_A_Ave;
    SstSinglePhase_FastStep(
        HA_Port_Volt_A, HA_Port_Current_A, g_chb_dab_status.pll_frequency_hz, g_chb_dab_params.sample_period_s);
    voltage_alpha = g_sst_single_phase_status.voltage_alpha_v;
    voltage_beta = g_sst_single_phase_status.voltage_beta_v;
    if ((g_sst_single_phase_status.valid == 0U) || !(SVG_Udc_A_Ave >= 0.0F && SVG_Udc_A_Ave <= 1000000.0F))
    {
        s_pll_integral = 0.0F;
        s_pll_lock_fast_count = 0U;
        g_chb_dab_status.pll_locked = 0U;
        g_chb_dab_status.pll_frequency_hz = g_chb_dab_params.pll_nominal_frequency_hz;
        if (g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP)
        {
            if (g_chb_dab_status.state == CHB_STATE_RUNNING)
            {
                ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
            }
            Chb_WriteNeutralReferences();
            return;
        }
        /* 开环脉冲验证仍独立于闭环采样有效性；非法输入不会送入 PLL。 */
        voltage_alpha = 0.0F;
        voltage_beta = 0.0F;
    }
#else
    g_chb_dab_status.dc_voltage_average_v = (SVG_Udc_A_Ave + SVG_Udc_B_Ave + SVG_Udc_C_Ave) / 3.0F;
    g_chb_dab_status.dc_voltage_min_v = Chb_Min3(SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave);

    voltage_alpha = HA_Port_Volt_A;
    voltage_beta = (HA_Port_Volt_A + 2.0F * HA_Port_Volt_B) * CHB_ONE_BY_SQRT3;
#endif
    /*****临时调试生成三相参考电压使用****/
    //    static float theta_openloop = 0.0f;
    //    if(theta_openloop < 6.283185307)
    //    {
    //        theta_openloop = theta_openloop + 6.283185307*0.005;
    //    }
    //    else
    //    {
    //        theta_openloop = 0;
    //    }
    //    voltage_alpha = 200*cosf(theta_openloop);
    //    voltage_beta = 200*sinf(theta_openloop);

    Chb_UpdatePll(voltage_alpha, voltage_beta);
#if SST_PHASE_A_ONLY_TEST
    if (g_sst_single_phase_status.ready == 0U)
    {
        /* 滤波建立前禁止累计锁定确认；建立后仍需通过原 PLL 连续锁定及 10 ms 状态机确认。 */
        s_pll_lock_fast_count = 0U;
        g_chb_dab_status.pll_locked = 0U;
    }
#endif

    if (g_chb_dab_status.state != CHB_STATE_RUNNING)
    {
        /* 非运行态写入指令中点；PWM 撤销使能由状态入口动作处理。 */
        Chb_WriteNeutralReferences();
        return;
    }

    if (g_chb_dab_status.ac_pwm_enabled == 0U)
    {
        /* DAB 独立开环测试时，已撤销使能的 CHB 保持指令中点。 */
        Chb_WriteNeutralReferences();
    }
    else if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
    {
        Chb_RunOpenLoop();
    }
    else
    {
        Chb_RunClosedLoop(voltage_alpha, voltage_beta);
    }

    if (g_chb_dab_status.dab_pwm_enabled != 0U)
    {
        /* DAB 每个 TAD 只增加固定步长，避免启动瞬间占空比跳到 50%。 */
        duty = g_chb_dab_status.dab_duty_count;
        step = g_chb_dab_params.dab_ramp_step_count;
        if (duty < g_chb_dab_params.dab_target_duty_count)
        {
            if ((unsigned short)(g_chb_dab_params.dab_target_duty_count - duty) < step)
            {
                duty = g_chb_dab_params.dab_target_duty_count;
            }
            else
            {
                duty = (unsigned short)(duty + step);
            }
            Chb_WriteDabDuty(duty);
        }
    }

    /* 开环不让闭环软件保护决定是否发波；闭环检查软件保护逻辑。 */
    if (g_chb_dab_command.ac_mode != CHB_AC_MODE_OPEN_LOOP)
    {
        Chb_CheckSoftwareProtection();
    }
}
