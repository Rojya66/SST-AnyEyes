/* 文件职责: 后台测量服务公开接口；快速采样仍在ADC_Sample。 */
#ifndef MEASUREMENT_SERVICE_H_
#define MEASUREMENT_SERVICE_H_
#ifdef __cplusplus
extern "C" {
#endif
/*==================================================================
 * 函数功能: 换算当前端口锁存/滤波采样，更新有效值、功率及公共遥测
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: TaskSlice末尾主循环调用，消费RMS累计并更新外电源/欠压判据；保留原换算及顺序。
 *================================================================*/
extern void Port_RealTimeParameter(void);
#ifdef __cplusplus
}
#endif
#endif
