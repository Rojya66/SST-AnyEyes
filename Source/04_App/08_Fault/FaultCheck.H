/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef FAULTCHECK_H_
#define FAULTCHECK_H_

#include "SstTypes.h"
#ifdef __cplusplus
extern "C"
{
#endif

//###########################################################################
//
// FILE:   FaultCheck.h
//
// AUTHOR: Wen Wusong;  Modified by Z.W
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

/**************************Macro definition*********************************/
/*
#define Module_OC1_L_Mask  0x0400
#define Module_OC2_L_Mask  0x0200
#define Module_OV_L_Mask   0x0100
#define Module_DAB_H_Mask  0x0080
#define Module_SVG_H_Mask  0x0040
#define Module_OT_Mask     0x0020

#define Module_Fault_Mask  		0x03e0
#define Module_HD_Fault_Mask  0x03A0
#define Module_LD_Fault_Mask  0x07E0
#define Module_DABSVG_Fault_Mask  0x0720
*/

/* Module_DAB_H_Mask：原模块故障状态字的 DAB_H 位掩码，保持 FPGA 状态布局。 */
#define Module_DAB_H_Mask 0x2000
/* Module_SVG_H_Mask：原模块故障状态字的 SVG_H 位掩码，保持 FPGA 状态布局。 */
#define Module_SVG_H_Mask 0x1000
/* Module_OT_Mask：原模块故障状态字的 OT 位掩码，保持 FPGA 状态布局。 */
#define Module_OT_Mask 0x0800
/* Module_OC1_L_Mask：原模块故障状态字的 OC1_L 位掩码，保持 FPGA 状态布局。 */
#define Module_OC1_L_Mask 0x0400
/* Module_OC2_L_Mask：原模块故障状态字的 OC2_L 位掩码，保持 FPGA 状态布局。 */
#define Module_OC2_L_Mask 0x0200
/* Module_OV_L_Mask：原模块故障状态字的 OV_L 位掩码，保持 FPGA 状态布局。 */
#define Module_OV_L_Mask 0x0100

/* Module_Fault_Mask：原模块故障状态字的 Fault 位掩码，保持 FPGA 状态布局。 */
#define Module_Fault_Mask 0x3b00
/* Module_HD_Fault_Mask：原模块故障状态字的 HD_Fault 位掩码，保持 FPGA 状态布局。 */
#define Module_HD_Fault_Mask 0x2b00
/* Module_DABSVG_HD_Mask：原模块故障状态字的 DABSVG_HD 位掩码，保持 FPGA 状态布局。 */
#define Module_DABSVG_HD_Mask 0x0b00
/* Module_LD_Fault_Mask：原模块故障状态字的 LD_Fault 位掩码，保持 FPGA 状态布局。 */
#define Module_LD_Fault_Mask 0x3f00
/* Module_DABSVG_LD_Mask：原模块故障状态字的 DABSVG_LD 位掩码，保持 FPGA 状态布局。 */
#define Module_DABSVG_LD_Mask 0x0f00

    /***********************Function declaration*********************************/
    /*==================================================================
 * 函数功能: 读取端口 AD、FB、FO 锁存并执行原第一阶段快速故障判断
 * 输入参数: 无
 * 输出参数: 先整端口封波，再更新 AD/FB/FO 锁存、故障类型及快速处置标志。
 * 返回值: 无
 * 执行说明: 在故障入口先封波；采集并筛选硬件锁存后，由 FaultDispose 执行后续分类及处置。
 *================================================================*/
    extern void FaultCheckAndDispose_StepOne(void);
    /*==================================================================
 * 函数功能: 逐模块分类故障并按原策略撤销 PWM、记录故障类型及处置状态
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 消费第一阶段锁存；按端口、模块和桥臂故障级别执行封波、状态回写及原恢复计数。
 *================================================================*/
    extern void FaultDispose(void);
    /*==================================================================
 * 函数功能: 发起或推进原两拍故障锁存清除流程
 * 输入参数: runindex：0 发起硬件复位，1 释放复位并清理处置状态
 *           RstFlagFault：阶段 1 是否清除对外故障字及软件故障标志，0 保留、非零清除
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 分两阶段调用；RstFlagFault 非零时在阶段 1 同时清除对外故障字和软件过流/过压标志。
 *================================================================*/
    extern void FaultLatch_Clear(uchar runindex, uchar RstFlagFault);

    /*==================================================================
 * 函数功能: 按原阈值和计时策略检查对应端口软件过流与直流过压
 * 输入参数: 无
 * 输出参数: 过流/过压计时使能、计数及 HVAC/LVDC 软件故障标志；不在本函数内封锁 PWM。
 * 返回值: 无
 * 执行说明: 保护检测调用点检查阈值；确认计数由 Timer2 的 10 ms 路径递增。
 *================================================================*/
    extern void OC_OV_LV_faultcheck(void);
    /*==================================================================
 * 函数功能: 将原软件电压、电流保护判据转换为端口故障及处置标志
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 在原保护调度路径中运行；仅更新原故障锁存、故障分类或 PWM 处置状态。
 *================================================================*/
    extern void OC_OV_LV_FaultDispose(void);

#ifdef __cplusplus
}
#endif
#endif
