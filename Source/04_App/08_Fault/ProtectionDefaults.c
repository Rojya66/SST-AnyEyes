#include "SstControlConfig.h"
/* 文件职责: ProtectionDefaults 所属数据域的默认初始化；保持原连续语句段。 */
#include "ProtectionDefaults.h"
#include "SstTypes.h"
#include "ProtectionData.h"
#include "ControlData.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 复位欠压分段确认和斜坡保护标志
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void ProtectionDefaults_ResetIndicators(void)
{
    Port_LV2=0;
    Port_Abnormal=0;
    Flag_V_L=0;
    Flag_V_L1_END=0;
    Flag_V_L2_END=0;
    Flag_V_L2_MID=0;
    Flag_SlopeTime=0;
    SlopeTime_N=0;
    Cout_V_L=0;

}

#endif /* HVAC_online */
