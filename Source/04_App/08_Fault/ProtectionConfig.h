/* 文件职责: ProtectionConfig.h 模块公开接口。 */
#ifndef PROTECTIONCONFIG_H_
#define PROTECTIONCONFIG_H_
#ifdef __cplusplus
extern "C" {
#endif
/* OC_level_SET_DEFAULT：原一级过流保护默认阈值。 */
#define OC_level_SET_DEFAULT 70
/* OC_time_SET_DEFAULT：原一级过流确认默认 TAD 计数。 */
#define OC_time_SET_DEFAULT 100
/* OC_level_SET_L2_DEFAULT：原二级过流保护默认阈值。 */
#define OC_level_SET_L2_DEFAULT 75
/* OC_time_SET_L2_DEFAULT：原二级过流确认默认 TAD 计数。 */
#define OC_time_SET_L2_DEFAULT 5
/* OV_level_SET_DEFAULT：原过压保护默认阈值。 */
#define OV_level_SET_DEFAULT 830
/* OV_time_SET_DEFAULT：原过压确认默认 TAD 计数。 */
#define OV_time_SET_DEFAULT 5
/* OV_allport_shutdown：原过压时全端口关断策略开关。 */
#define OV_allport_shutdown 900
#ifdef __cplusplus
}
#endif
#endif
