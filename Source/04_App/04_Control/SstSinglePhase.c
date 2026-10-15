//###########################################################################
// FILE: SstSinglePhase.c
// FUNCTION: 仅 A 相测试的电压、电流 SOGI 正交信号发生器。
//###########################################################################
#include "SstSinglePhase.h"

#if SST_PHASE_A_ONLY_TEST
#include <float.h>

/* SST_SINGLE_PHASE_PI：SOGI 梯形离散系数使用的圆周率。 */
#define SST_SINGLE_PHASE_PI 3.14159265358979323846f
/* SST_SINGLE_PHASE_SOGI_K：SOGI 阻尼增益 sqrt(2)，不属于 dq 闭环 PI 参数。 */
#define SST_SINGLE_PHASE_SOGI_K 1.41421356237309504880f
/* SST_SINGLE_PHASE_SETTLING_SAMPLES：正交信号最短建立时间 1000 拍，即 100 ms。 */
#define SST_SINGLE_PHASE_SETTLING_SAMPLES 1000U

typedef struct
{
    float voltage_alpha_v; /* voltage_alpha_v：上一拍 SOGI 同相电压，单位 V。 */
    float voltage_beta_v; /* voltage_beta_v：上一拍 SOGI 正交电压，单位 V。 */
    float voltage_previous_v; /* voltage_previous_v：上一拍 AN 电压输入，单位 V。 */
    float current_alpha_a; /* current_alpha_a：上一拍 SOGI 同相电流，单位 A。 */
    float current_beta_a; /* current_beta_a：上一拍 SOGI 正交电流，单位 A。 */
    float current_previous_a; /* current_previous_a：上一拍 A 相电流输入，单位 A。 */
} SST_SINGLE_PHASE_HISTORY;

static SST_SINGLE_PHASE_HISTORY
    single_phase_history; /* single_phase_history：滤波私有历史，正常停机时保留以连续跟踪输入。 */

volatile SST_SINGLE_PHASE_STATUS
    g_sst_single_phase_status; /* g_sst_single_phase_status：单相虚拟 alpha/beta 与有效、建立状态。 */

/*==================================================================
 * 函数功能: 清零单相 SOGI 历史输入、正交输出及建立计数
 * 输入参数: 无
 * 输出参数: g_sst_single_phase_status 与内部滤波历史量。
 * 返回值: 无
 * 执行说明: 上电初始化及非法采样恢复使用；正常停机、再次启动时不重复复位。
 *================================================================*/
void SstSinglePhase_Init(void)
{
    single_phase_history.voltage_alpha_v = 0.0f;
    single_phase_history.voltage_beta_v = 0.0f;
    single_phase_history.voltage_previous_v = 0.0f;
    single_phase_history.current_alpha_a = 0.0f;
    single_phase_history.current_beta_a = 0.0f;
    single_phase_history.current_previous_a = 0.0f;

    g_sst_single_phase_status.voltage_alpha_v = 0.0f;
    g_sst_single_phase_status.voltage_beta_v = 0.0f;
    g_sst_single_phase_status.current_alpha_a = 0.0f;
    g_sst_single_phase_status.current_beta_a = 0.0f;
    g_sst_single_phase_status.settling_count = 0U;
    g_sst_single_phase_status.ready = 0U;
    g_sst_single_phase_status.valid = 0U;
}

/*==================================================================
 * 函数功能: 用共享系数的梯形离散 SOGI 生成单相电压、电流正交量
 * 输入参数: voltage_an_v：AN 电压采样，单位 V，允许范围 -1e6..1e6
 *           current_a：A 相电流采样，单位 A，允许范围 -1e6..1e6
 *           frequency_hz：现有 PLL 估计频率，单位 Hz，允许范围 1..200
 *           period_s：本拍控制周期，单位 s，必须为 100 µs，容差 1e-8 s
 * 输出参数: g_sst_single_phase_status；非法数值清零历史及输出，并撤销 valid/ready。
 * 返回值: 无
 * 执行说明: 固定 10 kHz TAD 路径每拍调用，停机时持续滤波；无循环及三角函数。
 *================================================================*/
void SstSinglePhase_FastStep(float voltage_an_v, float current_a, float frequency_hz, float period_s)
{
    float half_omega_period; /* half_omega_period：半拍电角度 a=pi*f*Ts，无量纲。 */
    float damping_term; /* damping_term：阻尼离散项 k*a，无量纲。 */
    float angle_squared; /* angle_squared：离散角度平方 a*a，无量纲。 */
    float denominator_inverse; /* denominator_inverse：共享分母 1+k*a+a*a 的倒数，每拍仅计算一次除法。 */
    float alpha_history_gain; /* alpha_history_gain：上一拍 alpha 的离散系数 1-k*a-a*a。 */
    float voltage_alpha_v; /* voltage_alpha_v：待校验的本拍同相电压输出，单位 V。 */
    float voltage_beta_v; /* voltage_beta_v：待校验的本拍正交电压输出，单位 V。 */
    float current_alpha_a; /* current_alpha_a：待校验的本拍同相电流输出，单位 A。 */
    float current_beta_a; /* current_beta_a：待校验的本拍正交电流输出，单位 A。 */

    /* 有界比较同时拒绝 NaN 与正负无穷，避免非法数值污染滤波历史。 */
    if (!(voltage_an_v >= -1000000.0f && voltage_an_v <= 1000000.0f && current_a >= -1000000.0f &&
          current_a <= 1000000.0f && frequency_hz >= 1.0f && frequency_hz <= 200.0f && period_s >= 0.00009999f &&
          period_s <= 0.00010001f))
    {
        SstSinglePhase_Init();
        return;
    }

    half_omega_period = SST_SINGLE_PHASE_PI * frequency_hz * period_s;
    damping_term = SST_SINGLE_PHASE_SOGI_K * half_omega_period;
    angle_squared = half_omega_period * half_omega_period;
    denominator_inverse = 1.0f / (1.0f + damping_term + angle_squared);
    alpha_history_gain = 1.0f - damping_term - angle_squared;

    /* 同相通道为带通输出，积分正交通道在基频处滞后同相通道 90°。 */
    voltage_alpha_v = (alpha_history_gain * single_phase_history.voltage_alpha_v -
                       2.0f * half_omega_period * single_phase_history.voltage_beta_v +
                       damping_term * (voltage_an_v + single_phase_history.voltage_previous_v)) *
                      denominator_inverse;
    voltage_beta_v = single_phase_history.voltage_beta_v +
                     half_omega_period * (voltage_alpha_v + single_phase_history.voltage_alpha_v);
    current_alpha_a = (alpha_history_gain * single_phase_history.current_alpha_a -
                       2.0f * half_omega_period * single_phase_history.current_beta_a +
                       damping_term * (current_a + single_phase_history.current_previous_a)) *
                      denominator_inverse;
    current_beta_a = single_phase_history.current_beta_a +
                     half_omega_period * (current_alpha_a + single_phase_history.current_alpha_a);

    /* 输出必须仍为有限单精度数；溢出与输入非法使用同一清零恢复路径。 */
    if (!(voltage_alpha_v >= -FLT_MAX && voltage_alpha_v <= FLT_MAX && voltage_beta_v >= -FLT_MAX &&
          voltage_beta_v <= FLT_MAX && current_alpha_a >= -FLT_MAX && current_alpha_a <= FLT_MAX &&
          current_beta_a >= -FLT_MAX && current_beta_a <= FLT_MAX))
    {
        SstSinglePhase_Init();
        return;
    }

    single_phase_history.voltage_alpha_v = voltage_alpha_v;
    single_phase_history.voltage_beta_v = voltage_beta_v;
    single_phase_history.voltage_previous_v = voltage_an_v;
    single_phase_history.current_alpha_a = current_alpha_a;
    single_phase_history.current_beta_a = current_beta_a;
    single_phase_history.current_previous_a = current_a;

    g_sst_single_phase_status.voltage_alpha_v = voltage_alpha_v;
    g_sst_single_phase_status.voltage_beta_v = voltage_beta_v;
    g_sst_single_phase_status.current_alpha_a = current_alpha_a;
    g_sst_single_phase_status.current_beta_a = current_beta_a;
    g_sst_single_phase_status.valid = 1U;
    if (g_sst_single_phase_status.settling_count < SST_SINGLE_PHASE_SETTLING_SAMPLES)
    {
        g_sst_single_phase_status.settling_count++;
    }
    g_sst_single_phase_status.ready =
        (g_sst_single_phase_status.settling_count >= SST_SINGLE_PHASE_SETTLING_SAMPLES) ? 1U : 0U;
}
#endif
