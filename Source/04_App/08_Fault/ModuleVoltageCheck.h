/* 文件职责: ModuleVoltageCheck.h 模块公开接口。 */
#ifndef MODULEVOLTAGECHECK_H_
#define MODULEVOLTAGECHECK_H_
#ifdef __cplusplus
extern "C" {
#endif
/*==================================================================
 * 函数功能: 依次检查各相在线模块直流电压，更新预充及高压判断标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Udc_check(void);
#ifdef __cplusplus
}
#endif
#endif
