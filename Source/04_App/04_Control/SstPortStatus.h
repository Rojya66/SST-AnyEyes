//###########################################################################
// FILE: SstPortStatus.h
// FUNCTION: 活动端口状态与联锁接口。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#ifndef SSTPORTSTATUS_H
#define SSTPORTSTATUS_H
#include "SstTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*==================================================================
 * 函数功能: 读取并解码端口主状态和运行模式
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
    extern void Port_StateUpdate_Read(void);

    /*==================================================================
 * 函数功能: 将端口运行、故障及模式编码写回FPGA主控HVAC_STATUS
 * 输入参数: 无
 * 输出参数: Port_RealFault、Port_MainStatus及端口状态镜像；GPIO144、FPGA状态字与LED。
 * 返回值: 无
 * 执行说明: FPGA TAD中断原位置调用，末尾经PortController_Communication(0)提交；不可移至后台。
 *================================================================*/
    extern void Port_StateUpdate_Write(void);

    /*==================================================================
 * 函数功能: 按旧全端口停机流程复位相关标志
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
    extern void Port_AllStopFlag_RST(void);

#ifdef __cplusplus
}
#endif
#endif
