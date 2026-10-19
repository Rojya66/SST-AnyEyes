/* 文件职责: ProtectionConfig.h 模块公开接口。 */
#ifndef PROTECTIONCONFIG_H_
#define PROTECTIONCONFIG_H_
#include "ChbBuildConfig.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* OC_level_SET_DEFAULT：原HVAC一级过流默认阈值，比较三相电流有效值，单位A。 */
#if SST_LOW_VOLTAGE_100V_TEST
/* OC_level_SET_DEFAULT：低压试验原 HVAC 一级过流默认阈值，单位 A。 */
#define OC_level_SET_DEFAULT 3 /* OC_level_SET_DEFAULT：低压试验原 HVAC 一级过流默认阈值，单位 A。 */
#else
#define OC_level_SET_DEFAULT 70 /* OC_level_SET_DEFAULT：原 HVAC 一级过流默认阈值，单位 A。 */
#endif
/* OC_time_SET_DEFAULT：原一级过流确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OC_time_SET_DEFAULT 100
/* OC_level_SET_L2_DEFAULT：原HVAC二级过流默认阈值，比较三相电流有效值，单位A。 */
#if SST_LOW_VOLTAGE_100V_TEST
/* OC_level_SET_L2_DEFAULT：低压试验原 HVAC 二级过流默认阈值，单位 A。 */
#define OC_level_SET_L2_DEFAULT 4 /* OC_level_SET_L2_DEFAULT：低压试验原 HVAC 二级过流默认阈值，单位 A。 */
#else
#define OC_level_SET_L2_DEFAULT 75 /* OC_level_SET_L2_DEFAULT：原 HVAC 二级过流默认阈值，单位 A。 */
#endif
/* OC_time_SET_L2_DEFAULT：原二级过流确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OC_time_SET_L2_DEFAULT 5
/* OV_level_SET_DEFAULT：原HVAC过压默认阈值，比较三相模块均压合成滤波值，单位V。 */
#if SST_LOW_VOLTAGE_100V_TEST
/* OV_level_SET_DEFAULT：低压试验原 HVAC 过压阈值，检查模块均压合成滤波值，单位 V/模块。 */
#define OV_level_SET_DEFAULT 42 /* OV_level_SET_DEFAULT：模块均压合成滤波值阈值，非逐模块最大值，单位 V/模块。 */
#else
#define OV_level_SET_DEFAULT 830 /* OV_level_SET_DEFAULT：原 HVAC 过压默认阈值，单位 V。 */
#endif
/* OV_time_SET_DEFAULT：原过压确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OV_time_SET_DEFAULT 5
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
