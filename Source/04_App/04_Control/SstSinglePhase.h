// 文件职责：仅 A 相测试时，由 AN 电压与 A 相电流构建虚拟正交坐标。
#ifndef SST_SINGLE_PHASE_H_
#define SST_SINGLE_PHASE_H_

#include <stdint.h>
#include "ChbBuildConfig.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if SST_PHASE_A_ONLY_TEST
    typedef struct
    {
        float voltage_alpha_v; /* voltage_alpha_v：SOGI 同相电压分量，单位 V。 */
        float voltage_beta_v; /* voltage_beta_v：基频处滞后 alpha 90° 的虚拟电压分量，单位 V。 */
        float current_alpha_a; /* current_alpha_a：SOGI 同相电流分量，单位 A。 */
        float current_beta_a; /* current_beta_a：基频处滞后 alpha 90° 的虚拟电流分量，单位 A。 */
        uint16_t settling_count; /* settling_count：连续有效的 100 µs 拍数，在 1000 拍处饱和。 */
        uint16_t ready; /* ready：连续有效满 1000 拍后的滤波建立标志，不代替 PLL 锁定。 */
        uint16_t valid; /* valid：本拍输入、频率、周期及正交输出全部有效时为 1。 */
    } SST_SINGLE_PHASE_STATUS;

    extern volatile SST_SINGLE_PHASE_STATUS
        g_sst_single_phase_status; /* g_sst_single_phase_status：CCS 可观察的单相正交量及建立状态。 */

    /*==================================================================
     * 函数功能: 清零单相 SOGI 历史输入、正交输出及建立计数
     * 输入参数: 无
     * 输出参数: g_sst_single_phase_status 与内部滤波历史量。
     * 返回值: 无
     * 执行说明: 上电初始化及非法采样恢复使用；正常停机、再次启动时不重复复位。
     *================================================================*/
    void SstSinglePhase_Init(void);

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
    void SstSinglePhase_FastStep(float voltage_an_v, float current_a, float frequency_hz, float period_s);
#endif

#ifdef __cplusplus
}
#endif

#endif
