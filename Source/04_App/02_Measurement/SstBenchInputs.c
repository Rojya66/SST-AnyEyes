// 文件职责：单UNIT无主回路测试的独立输入发生器，不包含主回路、负载或PI反馈模型。
#include "SstBenchInputs.h"
#if SST_BENCH_INPUT_SIM
#include <math.h>
#include "MeasurementData.h"
#include "ChbDabControl.h"

/* SST_BENCH_TWO_PI：电网相角归一化及频率积分常数2pi。 */
#define SST_BENCH_TWO_PI 6.28318530717958647692F
volatile SST_BENCH_INPUTS g_sst_bench_inputs = {
    300.0F, 300.0F, 300.0F, 325.0F, 50.0F, 0.0F}; /* g_sst_bench_inputs：无预充达标的默认测试输入，需CCS人工升压。 */
volatile SST_BENCH_STATUS g_sst_bench_status; /* g_sst_bench_status：本拍注入观察量，不用作控制反馈。 */
static float s_grid_angle_rad; /* s_grid_angle_rad：下一TAD输入相角，独立于控制PLL输出。 */

/*==================================================================
 * 函数功能: 验证测试浮点量有限且在允许区间
 * 输入参数: value：待检查量；minimum：闭区间下界；maximum：闭区间上界
 * 输出参数: 无
 * 返回值: 1有效，0表示NaN、无穷或越界
 * 执行说明: 两次有界比较拒绝非法浮点值，不调用硬件或通信。
 *================================================================*/
static uint16_t BenchRangeValid(float value, float minimum, float maximum)
{
    return (uint16_t)((value >= minimum) && (value <= maximum));
}
#endif

/*==================================================================
 * 函数功能: 初始化临时输入、注入计数及独立电网相角
 * 输入参数: 无
 * 输出参数: 测试输入恢复为300V直流、325V峰值、50Hz、零电流
 * 返回值: 无
 * 执行说明: 控制对象初始化后、中断开放前调用；不修改控制模式或启动命令。
 *================================================================*/
void SstBenchInputs_Init(void)
{
#if SST_BENCH_INPUT_SIM
    g_sst_bench_inputs.dc_a_v = 300.0F;
    g_sst_bench_inputs.dc_b_v = 300.0F;
    g_sst_bench_inputs.dc_c_v = 300.0F;
    g_sst_bench_inputs.grid_peak_v = 325.0F;
    g_sst_bench_inputs.grid_frequency_hz = 50.0F;
    g_sst_bench_inputs.current_peak_a = 0.0F;
    g_sst_bench_status.tad_count = 0U;
    g_sst_bench_status.angle_rad = 0.0F;
    g_sst_bench_status.invalid_input = 0U;
    s_grid_angle_rad = 0.0F;
#endif
}

/*==================================================================
 * 函数功能: 注入测试电网和直流电压；三相生成平衡输入，单相仅生成 AN、IA 及 A 相母线
 * 输入参数: 无；读取g_sst_bench_inputs和sample_period_s
 * 输出参数: HA相/线电压、端口及网侧电流、三相/总均压和注入状态
 * 返回值: 无
 * 执行说明: 只在测试宏开启的常规采样中替代真实输入，锁存采样仍读取硬件。
 *================================================================*/
void SstBenchInputs_FastStep(void)
{
#if SST_BENCH_INPUT_SIM
    float dc_a = g_sst_bench_inputs.dc_a_v; /* dc_a：本拍A相虚拟模块均压快照。 */
    float dc_b = g_sst_bench_inputs.dc_b_v; /* dc_b：本拍B相虚拟模块均压快照。 */
    float dc_c = g_sst_bench_inputs.dc_c_v; /* dc_c：本拍C相虚拟模块均压快照。 */
    float peak_v = g_sst_bench_inputs.grid_peak_v; /* peak_v：本拍虚拟相电压峰值快照。 */
    float frequency = g_sst_bench_inputs.grid_frequency_hz; /* frequency：本拍独立电网频率快照。 */
    float peak_a = g_sst_bench_inputs.current_peak_a; /* peak_a：本拍虚拟相电流峰值快照。 */
    float period = g_chb_dab_params.sample_period_s; /* period：每TAD积分周期，测试保持0.0001s。 */
    float phase_a, /* phase_a：本拍A相单位幅值余弦。 */
        phase_b; /* phase_b：本拍B相单位幅值余弦，滞后A相120度。 */
    uint16_t valid; /* valid：所有测试参数有限且在规定范围内的整体结果。 */

    valid = (uint16_t)(BenchRangeValid(dc_a, 0.0F, 10000.0F) &&
#if !SST_PHASE_A_ONLY_TEST
                       BenchRangeValid(dc_b, 0.0F, 10000.0F) && BenchRangeValid(dc_c, 0.0F, 10000.0F) &&
#endif
                       BenchRangeValid(peak_v, 0.0F, 20000.0F) && BenchRangeValid(frequency, 0.0F, 100.0F) &&
                       BenchRangeValid(peak_a, 0.0F, 10000.0F) && BenchRangeValid(period, 0.000001F, 0.01F));
    g_sst_bench_status.invalid_input = (uint16_t)!valid;
    ++g_sst_bench_status.tad_count;
    g_sst_bench_status.angle_rad = s_grid_angle_rad;
    if (valid)
    {
        phase_a = cosf(s_grid_angle_rad);
        phase_b = cosf(s_grid_angle_rad - SST_BENCH_TWO_PI / 3.0F);
    }
    else
    {
        phase_a = phase_b = 0.0F;
        dc_a = dc_b = dc_c = peak_v = peak_a = 0.0F;
    }
    HA_Port_Volt_A = peak_v * phase_a;
    HA_Port_Volt_B = peak_v * phase_b;
    HA_Port_Volt_C = -HA_Port_Volt_A - HA_Port_Volt_B;
    HA_Port_Volt_AB = HA_Port_Volt_A - HA_Port_Volt_B;
    HA_Port_Volt_BC = HA_Port_Volt_B - HA_Port_Volt_C;
    HA_Port_Volt_CA = HA_Port_Volt_C - HA_Port_Volt_A;
    HA_Port_Current_A = peak_a * phase_a;
    HA_Port_Current_B = peak_a * phase_b;
    HA_Port_Current_C = -HA_Port_Current_A - HA_Port_Current_B;
#if SST_PHASE_A_ONLY_TEST
    /* 单相流程注入同一 AN/IA 输入；首旧线电压槽兼容 AN，B/C 采样及母线均不虚构。 */
    HA_Port_Volt_B = 0.0F;
    HA_Port_Volt_C = 0.0F;
    HA_Port_Volt_AB = HA_Port_Volt_A;
    HA_Port_Volt_BC = 0.0F;
    HA_Port_Volt_CA = 0.0F;
    HA_Port_Current_B = 0.0F;
    HA_Port_Current_C = 0.0F;
    dc_b = 0.0F;
    dc_c = 0.0F;
#endif
    HA_Grid_Current_A = HA_Port_Current_A;
    HA_Grid_Current_B = HA_Port_Current_B;
    HA_Grid_Current_C = HA_Port_Current_C;
    SVG_Udc_A_Ave = dc_a;
    SVG_Udc_B_Ave = dc_b;
    SVG_Udc_C_Ave = dc_c;
#if SST_PHASE_A_ONLY_TEST
    SVG_Udc_Ave = dc_a;
#else
    SVG_Udc_Ave = (dc_a + dc_b + dc_c) / 3.0F;
#endif
    if (valid)
    {
        s_grid_angle_rad += SST_BENCH_TWO_PI * frequency * period;
        if (s_grid_angle_rad >= SST_BENCH_TWO_PI)
            s_grid_angle_rad -= SST_BENCH_TWO_PI;
    }
#endif
}
