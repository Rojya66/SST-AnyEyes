//###########################################################################
// FILE: Ft3Recorder.h
// FUNCTION: FT3故障录波通信及原八字接收缓存。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#ifndef FT3RECORDER_H
#define FT3RECORDER_H
#include "SstTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================
 * 函数功能: 组装 FT3 采样、状态及故障数据并写入所选 FT3 插件
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 *           dataPackNum：FT3 本次发送的数据包数/控制字字段
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
extern void FT3_PluginUint_Write(unsigned short FT3_index,unsigned short dataPackNum);

/*==================================================================
 * 函数功能: 读取所选 FT3 插件接收区到兼容接收缓存
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
extern void FT3_PluginUint_Read(ushort FT3_index);

/* FT3_PTCT：PT/CT FT3 插件原 EMIFA 地址。 */
#define FT3_PTCT           1   //合并单元
/* FT3_FaultRecorder：故障录波 FT3 插件原 EMIFA 地址。 */
#define FT3_FaultRecorder  2   //故障录波器
/* FT3_EER2：EER2 FT3 插件原 EMIFA 地址。 */
#define FT3_EER2           3   //特变设备
/* FT3_Backup：备用 FT3 插件原 EMIFA 地址。 */
#define FT3_Backup         4   //备用设备


#ifdef __cplusplus
}
#endif
#endif
