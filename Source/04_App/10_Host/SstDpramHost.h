#ifndef SST_DPRAM_HOST_H
#define SST_DPRAM_HOST_H

#ifdef __cplusplus
extern "C" {
#endif

/* EMIFA 建立后令签名无效，清除复位之前的协议就绪状态。 */
void SstDpramHost_Invalidate(void);
/* 控制对象初始化完成、DPRAM 中断开放前发布初值。 */
void SstDpramHost_Initialize(void);
/* 主循环先消费邮箱，再分片发布遥测。 */
void SstDpramHost_Service(void);

#ifdef __cplusplus
}
#endif
#endif
