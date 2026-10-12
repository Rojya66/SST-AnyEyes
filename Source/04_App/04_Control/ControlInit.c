/* 文件职责: 协调原启动数据初始化及运行缓存复位。算法和硬件赋值由各分域Init模块实现。 */
#include "ControlInit.h"
#include "SstControlConfig.h"
#include "RuntimeStateInit.h"
#include "ProtectionDefaults.h"
#include "FpgaInit.h"
#include "PortStatusInit.h"
#include "DpramDefaults.h"
#include "ControlDefaults.h"
#include "MeasurementInit.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 按原项目初始化控制系数、模块位图、保护阈值及运行变量
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Initial_Variable(void)
{
    RuntimeStateInit_ResetObservationAndModes();
    RuntimeStateInit_ResetTimeFlags();
    ProtectionDefaults_ResetIndicators();
    RuntimeStateInit_ResetLegacySerial();
    FpgaInit_SetSafeOutputs();
    PortStatusInit_DefaultState();
    DpramDefaults_InitRegulation();
    FpgaInit_ConfigureModuleLayout();
    ControlDefaults_InitLimits();
    ControlDefaults_InitCoefficients();
    FpgaInit_InitTimingDuty();
    FpgaInit_InitOutputControls();
    MeasurementInit_ResetFilters();
    ControlDefaults_InitPllState();
    MeasurementInit_ResetAverages();
    FpgaInit_InitRecorderFanStatus();
}

#endif /* HVAC_online */

/*==================================================================
 * 函数功能: 复位滤波器、PI、遥控、故障以及原运行流程的历史状态
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Variable_RST(void)
{
    ControlDefaults_ResetRegulators();
}

