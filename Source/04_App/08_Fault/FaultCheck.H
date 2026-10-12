/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef FAULTCHECK_H_
#define FAULTCHECK_H_

#include "SstTypes.h"
#ifdef __cplusplus
extern "C" {
#endif

//###########################################################################
//
// FILE:   FaultCheck.h
//
// AUTHOR: Wen Wusong
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
#define Module_DAB_H_Mask  0x2000
/* Module_SVG_H_Mask：原模块故障状态字的 SVG_H 位掩码，保持 FPGA 状态布局。 */
#define Module_SVG_H_Mask  0x1000
/* Module_OT_Mask：原模块故障状态字的 OT 位掩码，保持 FPGA 状态布局。 */
#define Module_OT_Mask     0x0800
/* Module_OC1_L_Mask：原模块故障状态字的 OC1_L 位掩码，保持 FPGA 状态布局。 */
#define Module_OC1_L_Mask  0x0400
/* Module_OC2_L_Mask：原模块故障状态字的 OC2_L 位掩码，保持 FPGA 状态布局。 */
#define Module_OC2_L_Mask  0x0200
/* Module_OV_L_Mask：原模块故障状态字的 OV_L 位掩码，保持 FPGA 状态布局。 */
#define Module_OV_L_Mask   0x0100

/* Module_Fault_Mask：原模块故障状态字的 Fault 位掩码，保持 FPGA 状态布局。 */
#define Module_Fault_Mask  		0x3b00
/* Module_HD_Fault_Mask：原模块故障状态字的 HD_Fault 位掩码，保持 FPGA 状态布局。 */
#define Module_HD_Fault_Mask  0x2b00
/* Module_DABSVG_HD_Mask：原模块故障状态字的 DABSVG_HD 位掩码，保持 FPGA 状态布局。 */
#define Module_DABSVG_HD_Mask  0x0b00
/* Module_LD_Fault_Mask：原模块故障状态字的 LD_Fault 位掩码，保持 FPGA 状态布局。 */
#define Module_LD_Fault_Mask  0x3f00
/* Module_DABSVG_LD_Mask：原模块故障状态字的 DABSVG_LD 位掩码，保持 FPGA 状态布局。 */
#define Module_DABSVG_LD_Mask  0x0f00

/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 读取端口 AD、FB、FO 锁存并执行原第一阶段快速故障判断
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
extern void FaultCheckAndDispose_StepOne(void);
/*==================================================================
 * 函数功能: 逐模块分类故障并按原策略撤销 PWM、记录故障类型及处置状态
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
extern void FaultDispose(void);
/*==================================================================
 * 函数功能: 发起或推进原两拍故障锁存清除流程
 * 输入参数: runindex：原故障锁存清除时序的执行/初始化选择值，沿用现有调用点
 *           RstFlagFault：旧故障复位阶段选择
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
extern void FaultLatch_Clear(uchar runindex,uchar RstFlagFault);



/*==================================================================
 * 函数功能: 按原阈值和计数策略检查端口交流过流、直流过压及欠压条件
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
extern void OC_OV_LV_faultcheck(void);
/*==================================================================
 * 函数功能: 将原软件电压、电流保护判据转换为端口故障及处置标志
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
extern void OC_OV_LV_FaultDispose(void);




#ifdef __cplusplus
}
#endif
#endif
