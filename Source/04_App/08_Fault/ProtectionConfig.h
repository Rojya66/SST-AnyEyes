/* 文件职责: ProtectionConfig.h 模块公开接口。 */
#ifndef PROTECTIONCONFIG_H_
#define PROTECTIONCONFIG_H_
#ifdef __cplusplus
extern "C" {
#endif
/* OC_level_SET_DEFAULT：原HVAC一级过流默认阈值，比较三相电流有效值，单位A。 */
#define OC_level_SET_DEFAULT 70
/* OC_time_SET_DEFAULT：原一级过流确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OC_time_SET_DEFAULT 100
/* OC_level_SET_L2_DEFAULT：原HVAC二级过流默认阈值，比较三相电流有效值，单位A。 */
#define OC_level_SET_L2_DEFAULT 75
/* OC_time_SET_L2_DEFAULT：原二级过流确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OC_time_SET_L2_DEFAULT 5
/* OV_level_SET_DEFAULT：原HVAC过压默认阈值，比较三相模块均压合成滤波值，单位V。 */
#define OV_level_SET_DEFAULT 830
/* OV_time_SET_DEFAULT：原过压确认默认Timer2计数，每计数10 ms；使用严格大于判据。 */
#define OV_time_SET_DEFAULT 5
/* OV_allport_shutdown：原全端口过压封波阈值，单位V；当前对应判断已注释，不是使能开关。 */
#define OV_allport_shutdown 900
#ifdef __cplusplus
}
#endif
#endif
