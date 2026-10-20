#include "SstControlConfig.h"
/* 文件职责: ProtectionDefaults 所属数据域的默认初始化；保持原连续语句段。 */
#include "ProtectionDefaults.h"
#include "SstTypes.h"
#include "ProtectionData.h"
#include "ControlData.h"
#include "ProtectionConfig.h"

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
    Port_LV2 = 0;
    Port_Abnormal = 0;
    Flag_V_L = 0;
    Flag_V_L1_END = 0;
    Flag_V_L2_END = 0;
    Flag_V_L2_MID = 0;
    Flag_SlopeTime = 0;
    SlopeTime_N = 0;
    Cout_V_L = 0;
}

/*==================================================================
 * 函数功能: 按额定输入和保护系数计算并应用统一保护门限
 * 输入参数: 无
 * 输出参数: 旧RMS过流/过压、预充和高压保护门限。
 * 返回值: 无
 * 执行说明: 仅初始化阶段调用；DPRAM运行时写入仍经过原停机校验。
 *==================================================================*/
void ProtectionDefaults_ApplyDerivedThresholds(void)
{
#ifdef HVAC_online
    OC_level_SET = (ushort)(SST_RMS_OC_TRIP_A + 0.5F);
    OC_level_SET_L2 = (ushort)(SST_RMS_OC_L2_TRIP_A + 0.5F);
    OV_level_SET = (ushort)(SST_RMS_OV_TRIP_V + 0.5F);
    HA_L_Voltage_Precharge = SST_LEGACY_MODULE_PRECHARGE_V;
    HA_H_Voltage = SST_LEGACY_MODULE_OV_V;
#endif
}

#endif /* HVAC_online */
