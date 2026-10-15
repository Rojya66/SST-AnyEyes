/* 文件职责: ModuleVoltageCheck.h 模块公开接口。 */
#ifndef MODULEVOLTAGECHECK_H_
#define MODULEVOLTAGECHECK_H_
#ifdef __cplusplus
extern "C"
{
#endif
    /*==================================================================
 * 函数功能: 依次检查各相在线模块直流电压，更新预充及高压判断标志
 * 输入参数: 无
 * 输出参数: Udc_Check_Index、各相预充/高压判断位图及 Port_SigUdc / Port_HighUdc 汇总标志。
 * 返回值: 无
 * 执行说明: HVAC 闭环的 FPGA 中断路径每次巡检一个位置；SST_BENCH_INPUT_SIM=1 时跳过真实电压巡检。
 *================================================================*/
    void Udc_check(void);
#ifdef __cplusplus
}
#endif
#endif
