/* 文件职责: 保护阈值、计数、锁存与故障策略。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef PROTECTIONDATA_H_
#define PROTECTIONDATA_H_
#include "SstControlConfig.h"
#include "SstTypes.h"
#include "SstControlConfig.h"
#include "ProtectionTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

extern XXG_PORT_FAULT Port_RealFault; /* Port_RealFault：端口故障的原位域缓存，某些位在 HVAC 中复用。 */

extern XXG_FaultTypeFLAG FaultTypeFLAG; /* FaultTypeFLAG：原故障分类位图，后台故障报告据此映射五位码。 */

extern XXG_FaultDisposeFLAG FaultDisposeFLAG; /* FaultDisposeFLAG：原故障处理策略及 PWM 禁用范围的位域缓存。 */

extern ushort ModuleIndex_PWMPulseDisable; /* ModuleIndex_PWMPulseDisable：原故障策略中待禁用 PWM 的模块索引。 */

extern unsigned char flag_FaultClose; /* flag_FaultClose：原故障后脉冲关闭标志。 */

extern unsigned char Flag_FaultOff; /* Flag_FaultOff：原故障停机标志。 */

extern ushort FOLatch_A_TAD; /* FOLatch_A_TAD：A 相插件光纤故障的 TAD 锁存值。 */

extern ushort FOLatch_B_TAD; /* FOLatch_B_TAD：B 相插件光纤故障的 TAD 锁存值。 */

extern ushort FOLatch_C_TAD; /* FOLatch_C_TAD：C 相插件光纤故障的 TAD 锁存值。 */

extern ushort FOLatch_INV_TAD; /* FOLatch_INV_TAD：独立逆变桥光纤故障的 TAD 锁存值。 */

extern ushort PortLatch_TAD; /* PortLatch_TAD：端口 AD 保护故障的 TAD 锁存值。 */

extern ushort FBLatch_TAD; /* FBLatch_TAD：端口 FB 硬件故障的 TAD 锁存值。 */

extern ushort ModuleFault_DisposeTimes; /* ModuleFault_DisposeTimes：原模块故障处置已执行次数。 */

extern unsigned char ClearTimes; /* ClearTimes：原故障清除过程的执行次数。 */

extern float Module_DCbus_Volt; /* Module_DCbus_Volt：当前检查模块的直流母线电压，单位 V。 */   //Module DCbus voltage

extern uchar Udc_Check_Index; /* Udc_Check_Index：逐模块直流电压巡检的当前索引。 */

extern uchar Udc_Checked_Num; /* Udc_Checked_Num：已检查模块数量。 */

#ifdef HVAC_online
extern float OverVolt_AlarmLevel2; /* OverVolt_AlarmLevel2：旧模块二级过压报警阈值，单位 V。 */

#endif
#ifdef HVAC_online
extern float VoltLevel_1; /* VoltLevel_1：旧电压判断第一级阈值。 */

#endif
#ifdef HVAC_online
extern float VoltLevel_2; /* VoltLevel_2：旧电压判断第二级阈值。 */

#endif
#ifdef HVAC_online
extern unsigned char Port_LV2; /* Port_LV2：旧端口第二级欠压状态。 */

#endif
#ifdef HVAC_online
extern unsigned char Flag_V_L; /* Flag_V_L：旧欠压阶段标志。 */

#endif
#ifdef HVAC_online
extern unsigned char Flag_V_L1_END; /* Flag_V_L1_END：旧一级欠压处理完成标志。 */

#endif
#ifdef HVAC_online
extern unsigned char Flag_V_L2_END; /* Flag_V_L2_END：旧二级欠压处理完成标志。 */

#endif
#ifdef HVAC_online
extern unsigned char Flag_V_L2_MID; /* Flag_V_L2_MID：旧二级欠压处理中间阶段标志。 */

#endif
#ifdef HVAC_online
extern ushort Cout_V_L; /* Cout_V_L：旧欠压处理计时计数。 */

#endif
extern uchar Udc_Check_Index; /* Udc_Check_Index：逐模块直流电压巡检的当前索引。 */

extern uchar Udc_Checked_Num; /* Udc_Checked_Num：已检查模块数量。 */

#ifdef HVAC_online
extern ushort counter_OC_flag; /* counter_OC_flag：原一级过流连续超限样本计数。 */

#endif
#ifdef HVAC_online
extern ushort counter_OC_time; /* counter_OC_time：原一级过流保护确认计时计数。 */

#endif
#ifdef HVAC_online
extern ushort counter_OC_flag1; /* counter_OC_flag1：原二级过流连续超限样本计数。 */

#endif
#ifdef HVAC_online
extern ushort counter_OC_time1; /* counter_OC_time1：原二级过流保护确认计时计数。 */

#endif
#ifdef HVAC_online
extern ushort OC_OV_LV_PortLatch_TAD; /* OC_OV_LV_PortLatch_TAD：原软件过流/过压/欠压的 TAD 故障锁存值。 */

#endif
#ifdef HVAC_online
extern ushort HVAC_OC_Fault; /* HVAC_OC_Fault：原 HVAC 软件过流故障确认标志。 */

#endif
#ifdef HVAC_online
extern ushort OC_level_SET; /* OC_level_SET：原一级过流阈值，沿用采样计数/系数约定。 */

#endif
#ifdef HVAC_online
extern ushort OC_time_SET; /* OC_time_SET：原一级过流确认计数阈值，单位 TAD。 */

#endif
#ifdef HVAC_online
extern ushort OC_level_SET_L2; /* OC_level_SET_L2：原二级过流阈值，沿用采样计数/系数约定。 */

#endif
#ifdef HVAC_online
extern ushort OC_time_SET_L2; /* OC_time_SET_L2：原二级过流确认计数阈值，单位 TAD。 */

#endif
#ifdef HVAC_online
extern ushort counter_OV_flag; /* counter_OV_flag：原一级过压连续超限样本计数。 */

#endif
#ifdef HVAC_online
extern ushort counter_OV_time; /* counter_OV_time：原一级过压保护确认计时计数。 */

#endif
#ifdef HVAC_online
extern ushort HVAC_OV_Fault; /* HVAC_OV_Fault：原 HVAC 软件过压故障确认标志。 */

#endif
#ifdef HVAC_online
extern ushort OV_level_SET; /* OV_level_SET：原直流过压阈值，沿用采样计数/系数约定。 */

#endif
#ifdef HVAC_online
extern ushort OV_time_SET; /* OV_time_SET：原过压确认计数阈值，单位 TAD。 */

#endif
#ifdef HVAC_online
extern ushort Contactor_err; /* Contactor_err：原接触器动作异常标志。 */

#endif
#ifdef HVAC_online
extern ushort Breaker_err; /* Breaker_err：原断路器动作异常标志。 */


/* 本模块拥有的保护量。 */
#endif
extern ushort Port_AD_PORT_EN; /* Port_AD_PORT_EN：端口各通道 AD 保护使能位图。 */



extern ushort Port_FO_1_EN; /* Port_FO_1_EN：第 1 组光纤故障输入使能位图。 */



extern ushort Port_FO_2_EN; /* Port_FO_2_EN：第 2 组光纤故障输入使能位图。 */



extern ushort Port_FO_3_EN; /* Port_FO_3_EN：第 3 组光纤故障输入使能位图。 */



extern ushort Port_FB_EN; /* Port_FB_EN：端口 FB 硬件故障输入使能位图。 */



extern unsigned char Flag_DC_FAULT; /* Flag_DC_FAULT：原直流故障总标志。 */



extern unsigned short Flag_DC_FAULTDISPOSE; /* Flag_DC_FAULTDISPOSE：原直流故障处置状态。 */



extern unsigned char Flag_DC_FAULTport; /* Flag_DC_FAULTport：原直流故障端口标识。 */



extern unsigned char Flag_Clear_0,Flag_Clear_1; /* Flag_Clear_0：原故障锁存清除第一阶段请求标志。 */



/* Flag_Clear_1：原故障锁存清除第二阶段请求标志。 */



extern unsigned char Flag_Clear_zhi; /* Flag_Clear_zhi：原故障清除期间的兼容状态。 */



#ifdef HVAC_online
extern ushort HA_A_LV_Precharge; /* HA_A_LV_Precharge：高压交流端口 预充欠压判断标志。 */



#endif
#ifdef HVAC_online
extern ushort HA_B_LV_Precharge; /* HA_B_LV_Precharge：高压交流端口 预充欠压判断标志。 */



#endif
#ifdef HVAC_online
extern ushort HA_C_LV_Precharge; /* HA_C_LV_Precharge：高压交流端口 预充欠压判断标志。 */



#endif
#ifdef HVAC_online
extern ushort HA_A_HV_Protect; /* HA_A_HV_Protect：高压交流端口 原直流高压判断标志。 */



#endif
#ifdef HVAC_online
extern ushort HA_B_HV_Protect; /* HA_B_HV_Protect：高压交流端口 原直流高压判断标志。 */



#endif
#ifdef HVAC_online
extern ushort HA_C_HV_Protect; /* HA_C_HV_Protect：高压交流端口 原直流高压判断标志。 */



#endif
#ifdef HVAC_online
extern float HA_L_Voltage_Precharge; /* HA_L_Voltage_Precharge：原 HVAC 预充欠压判断阈值，单位 V。 */



#endif
#ifdef HVAC_online
extern float HA_H_Voltage; /* HA_H_Voltage：原 HVAC 直流高压判断阈值，单位 V。 */



#endif
#ifdef HVAC_online
extern unsigned char Port_Abnormal; /* Port_Abnormal：旧端口异常状态标志。 */




#endif
#ifdef __cplusplus
}
#endif
#endif
