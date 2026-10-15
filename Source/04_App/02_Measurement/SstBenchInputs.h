// 文件职责：由测试宏选择的单UNIT无主回路采样注入接口，测试量只由CCS临时修改。
#ifndef SST_BENCH_INPUTS_H_
#define SST_BENCH_INPUTS_H_
#include <stdint.h>
#include "ChbBuildConfig.h"

#ifdef __cplusplus
extern "C"
{
#endif
#if SST_BENCH_INPUT_SIM
    typedef struct
    {
        float dc_a_v; /* dc_a_v：A相虚拟单模块直流电压，单位V，范围0..10000。 */
        float dc_b_v; /* dc_b_v：B相虚拟单模块直流电压，单位V，范围0..10000。 */
        float dc_c_v; /* dc_c_v：C相虚拟单模块直流电压，单位V，范围0..10000。 */
        float grid_peak_v; /* grid_peak_v：虚拟相电压峰值，单相宏下为 AN 峰值，单位V，范围0..20000。 */
        float grid_frequency_hz; /* grid_frequency_hz：独立虚拟电网频率，单位Hz，范围0..100。 */
        float current_peak_a; /* current_peak_a：与虚拟相电压同相的相电流峰值，单相仅生成 A 相，单位A，范围0..10000。 */
    } SST_BENCH_INPUTS;

    typedef struct
    {
        uint32_t tad_count; /* tad_count：本次初始化后已注入TAD数量，32位自然回绕，不包含暂停墙钟时间。 */
        float angle_rad; /* angle_rad：本拍实际注入的独立电网相角，单位rad，范围0..2pi。 */
        uint16_t invalid_input; /* invalid_input：1表示测试参数非法且本拍注入全零，0表示参数有效。 */
    } SST_BENCH_STATUS;

    extern volatile SST_BENCH_INPUTS
        g_sst_bench_inputs; /* g_sst_bench_inputs：CCS可写的临时测试输入，不接入通信点表或EEPROM。 */
    extern volatile SST_BENCH_STATUS
        g_sst_bench_status; /* g_sst_bench_status：CCS观察的注入计数、独立相角及非法输入标志。 */
#endif

    /*==================================================================
 * 函数功能: 在外部中断开放前恢复临时测试输入、独立相角和计数
 * 输入参数: 无
 * 输出参数: 仅测试宏开启时重置测试输入及状态，不提交启动命令
 * 返回值: 无
 * 执行说明: SystemInit运行初始化调用；宏关闭时为空操作。
 *================================================================*/
    void SstBenchInputs_Init(void);
    /*==================================================================
 * 函数功能: 按独立电网相角生成本拍相/线电压、电流和模块直流电压
 * 输入参数: 无；读取CCS测试输入及控制采样周期
 * 输出参数: 原测量RAM、均压及测试状态；非法数值注入全零
 * 返回值: 无
 * 执行说明: 测试构建的Port_AD_Sample(0)调用；宏关闭时不访问测量或测试量。
 *================================================================*/
    void SstBenchInputs_FastStep(void);
#ifdef __cplusplus
}
#endif
#endif
