#ifndef SST_DPRAM_V2_TELEMETRY_H
#define SST_DPRAM_V2_TELEMETRY_H

#include <stdint.h>

/* 后台初始化版本戳，随后分片发布读区。普通扫描不会写控制或遥调请求。 */
void SstDpramV2Telemetry_Init(void);
void SstDpramV2Telemetry_Service(void);

/* 完整综合遥测发布后置位。主机按邮箱20优先的顺序发邮箱3。 */
uint16_t SstDpramV2Telemetry_TakeLiveNotification(void);

#endif
