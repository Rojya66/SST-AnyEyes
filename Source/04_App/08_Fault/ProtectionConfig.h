/* 文件职责: ProtectionConfig.h 模块公开接口。 */
#ifndef PROTECTIONCONFIG_H_
#define PROTECTIONCONFIG_H_ /* PROTECTIONCONFIG_H_：协议/保护配置常量。 */
#include "ChbBuildConfig.h"

/*
 * 保护门限只在这里定义一次。快速保护比较TAD瞬时采样，旧保护比较
 * 后台发布的电流有效值。直流过压只由 SST_FAST_OV_TRIP_V 派生的
 * dc_overvoltage_trip_v 执行；旧 OV 常量只保留 DPRAM/持久化兼容值。
 * 电流两种判据使用同一组额定输入和系数，避免分散维护魔数。
 */
#if SST_PHASE_A_ONLY_TEST
#define SST_GRID_PHASES 1U /* SST_GRID_PHASES：当前输入相数，单相为1。 */
#else
#define SST_GRID_PHASES 3U /* SST_GRID_PHASES：当前输入相数，三相为3。 */
#endif

#if SST_LOW_VOLTAGE_100V_TEST
#define SST_INPUT_VOLTAGE_RMS_V 100.0F /* SST_INPUT_VOLTAGE_RMS_V：低压额定输入电压，单位V RMS。 */
#define SST_INPUT_CURRENT_RMS_A 32.0F /* SST_INPUT_CURRENT_RMS_A：低压额定输入电流，单位A RMS。 */
#define SST_MODULE_REFERENCE_V 60.0F /* SST_MODULE_REFERENCE_V：低压模块额定均压，单位V。 */
#define SST_FAST_OC_FACTOR 1.325825F /* SST_FAST_OC_FACTOR：低压瞬时过流峰值系数。 */
#define SST_RMS_OC_FACTOR 1.25F /* SST_RMS_OC_FACTOR：低压一级有效值过流系数。 */
#define SST_RMS_OC_L2_FACTOR 1.40625F /* SST_RMS_OC_L2_FACTOR：低压二级有效值过流系数。 */
#define SST_FAST_OV_FACTOR 1.25F /* SST_FAST_OV_FACTOR：低压瞬时过压系数。 */
#define SST_RMS_OV_FACTOR 1.333333F /* SST_RMS_OV_FACTOR：低压有效值过压系数。 */
#else
#define SST_INPUT_VOLTAGE_RMS_V 380.0F /* SST_INPUT_VOLTAGE_RMS_V：高压额定输入电压，单位V RMS。 */
#define SST_INPUT_CURRENT_RMS_A 56.0F /* SST_INPUT_CURRENT_RMS_A：高压额定输入电流，单位A RMS。 */
#define SST_MODULE_REFERENCE_V 680.0F /* SST_MODULE_REFERENCE_V：高压模块额定均压，单位V。 */
#define SST_FAST_OC_FACTOR 1.515229F /* SST_FAST_OC_FACTOR：高压瞬时过流峰值系数。 */
#define SST_RMS_OC_FACTOR 1.25F /* SST_RMS_OC_FACTOR：高压一级有效值过流系数。 */
#define SST_RMS_OC_L2_FACTOR 1.339286F /* SST_RMS_OC_L2_FACTOR：高压二级有效值过流系数。 */
#define SST_FAST_OV_FACTOR 1.147059F /* SST_FAST_OV_FACTOR：高压瞬时过压系数。 */
#define SST_RMS_OV_FACTOR 1.220588235F /* SST_RMS_OV_FACTOR：高压有效值过压系数。 */
#endif

/* 额定输入峰值仅用于说明瞬时门限的计算，不是额外运行参数。 */
#define SST_INPUT_VOLTAGE_PEAK_V (SST_INPUT_VOLTAGE_RMS_V * 1.41421356F) /* SST_INPUT_VOLTAGE_PEAK_V：额定输入峰值，单位V。 */
#define SST_INPUT_CURRENT_PEAK_A (SST_INPUT_CURRENT_RMS_A * 1.41421356F) /* SST_INPUT_CURRENT_PEAK_A：额定输入峰值，单位A。 */
#define SST_FAST_OC_TRIP_A (SST_INPUT_CURRENT_PEAK_A * SST_FAST_OC_FACTOR) /* SST_FAST_OC_TRIP_A：瞬时过流门限，单位A。 */
#define SST_FAST_OV_TRIP_V (SST_MODULE_REFERENCE_V * SST_FAST_OV_FACTOR) /* SST_FAST_OV_TRIP_V：瞬时过压门限，单位V。 */
#define SST_RMS_OC_TRIP_A (SST_INPUT_CURRENT_RMS_A * SST_RMS_OC_FACTOR) /* SST_RMS_OC_TRIP_A：一级有效值过流门限，单位A。 */
#define SST_RMS_OC_L2_TRIP_A (SST_INPUT_CURRENT_RMS_A * SST_RMS_OC_L2_FACTOR) /* SST_RMS_OC_L2_TRIP_A：二级有效值过流门限，单位A。 */
#define SST_RMS_OV_TRIP_V (SST_MODULE_REFERENCE_V * SST_RMS_OV_FACTOR) /* SST_RMS_OV_TRIP_V：有效值过压门限，单位V。 */

/* 启动预充与旧巡检兼容参数集中在此处；当前 CHB 不调用旧逐模块巡检。 */
#if SST_LOW_VOLTAGE_100V_TEST
/* SST_CONTROL_PRECHARGE_V：现代闭环状态机的相模块均压预充门槛，单位 V。 */
#define SST_CONTROL_PRECHARGE_V SST_LV100_PRECHARGE_V /* SST_CONTROL_PRECHARGE_V：协议/保护配置常量。 */
/* SST_LEGACY_MODULE_PRECHARGE_V：旧 Udc_check 逐模块预充巡检门槛，单位 V。 */
#define SST_LEGACY_MODULE_PRECHARGE_V SST_LV100_PRECHARGE_V /* SST_LEGACY_MODULE_PRECHARGE_V：协议/保护配置常量。 */
/* SST_LEGACY_MODULE_OV_V：旧 Udc_check 逐模块高压巡检门槛，单位 V。 */
#define SST_LEGACY_MODULE_OV_V SST_LV100_MODULE_OV_V /* SST_LEGACY_MODULE_OV_V：协议/保护配置常量。 */
#else
/* SST_CONTROL_PRECHARGE_V：保留现场高压现代状态机预充门槛，单位 V。 */
#define SST_CONTROL_PRECHARGE_V 280.0F /* SST_CONTROL_PRECHARGE_V：协议/保护配置常量。 */
/* SST_LEGACY_MODULE_PRECHARGE_V：原高压逐模块预充巡检门槛，单位 V。 */
#define SST_LEGACY_MODULE_PRECHARGE_V 350.0F /* SST_LEGACY_MODULE_PRECHARGE_V：协议/保护配置常量。 */
/* SST_LEGACY_MODULE_OV_V：原高压逐模块高压巡检门槛，单位 V。 */
#define SST_LEGACY_MODULE_OV_V (SST_MODULE_REFERENCE_V * SST_FAST_OV_FACTOR) /* SST_LEGACY_MODULE_OV_V：协议/保护配置常量。 */
#endif

#ifdef __cplusplus
extern "C"
{
#endif
/* OC_level_SET_DEFAULT：由额定有效值和一级有效值系数自动计算，单位A。 */
#define OC_level_SET_DEFAULT ((unsigned short)(SST_RMS_OC_TRIP_A + 0.5F)) /* OC_level_SET_DEFAULT：协议/保护配置常量。 */
/* OC_time_SET_DEFAULT：原一级过流确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OC_time_SET_DEFAULT 100 /* OC_time_SET_DEFAULT：协议/保护配置常量。 */
/* OC_level_SET_L2_DEFAULT：原HVAC二级过流默认阈值，比较三相电流有效值，单位A。 */
/* OC_level_SET_L2_DEFAULT：由额定有效值和二级有效值系数自动计算，单位A。 */
#define OC_level_SET_L2_DEFAULT ((unsigned short)(SST_RMS_OC_L2_TRIP_A + 0.5F)) /* OC_level_SET_L2_DEFAULT：协议/保护配置常量。 */
/* OC_time_SET_L2_DEFAULT：原二级过流确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OC_time_SET_L2_DEFAULT 5 /* OC_time_SET_L2_DEFAULT：协议/保护配置常量。 */
/* OV_level_SET_DEFAULT：原HVAC过压默认阈值，比较三相模块均压合成滤波值，单位V。 */
/* OV_level_SET_DEFAULT：按模块额定均压和旧有效值保护系数计算，单位V。 */
#define OV_level_SET_DEFAULT ((unsigned short)(SST_RMS_OV_TRIP_V + 0.5F)) /* OV_level_SET_DEFAULT：协议/保护配置常量。 */
/* OV_time_SET_DEFAULT：原过压确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OV_time_SET_DEFAULT 5 /* OV_time_SET_DEFAULT：协议/保护配置常量。 */
/* OV_allport_shutdown：原全端口过压封波阈值，单位V；当前对应判断已注释，不是使能开关。 */
#if SST_LOW_VOLTAGE_100V_TEST
/* OV_allport_shutdown：低压试验原全端口过压封波阈值，单位 V。 */
#define OV_allport_shutdown 48 /* OV_allport_shutdown：低压试验原全端口过压封波阈值，单位 V。 */
#else
#define OV_allport_shutdown 900 /* OV_allport_shutdown：原全端口过压封波阈值，单位 V。 */
#endif
#ifdef __cplusplus
}
#endif
#endif
