#include "ControlData.h"
#include <math.h>
#include "ControlInit.h"
#include "DpramHostData.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "SstTypes.h"
#include "TransformData.h"
//###########################################################################
//
// FILE:   HVAC_Strategy.c
//
// FUNCTION:   
//            
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

//###########################################################################
#include "CommonProgram.h"
#include "HVAC_Strategy.h"
#include "hw_types.h" 
//#include <fastrts67x.h>
//#include <recip.h>
/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 执行原 H2/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH2_100_Filter(float FilterIN);
/*==================================================================
 * 函数功能: 执行原 H6/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH6_100_Filter(float FilterIN);
/*==================================================================
 * 函数功能: 执行原 H12/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH12_100_Filter(float FilterIN);
/*==================================================================
 * 函数功能: 执行原 H2/20 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH2_20_Filter(float FilterIN);
/*==================================================================
 * 函数功能: 执行原 2 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_2_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
float SineHar_2_Controller(float ErrorIn);
/*==================================================================
 * 函数功能: 执行原 6 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_6_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
float SineHar_6_Controller(float ErrorIn);
/*==================================================================
 * 函数功能: 执行原 12 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_12_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
float SineHar_12_Controller(float ErrorIn);

/*==================================================================
 * 函数功能: 对电网电压作坐标变换及序分量滤波，推进原锁相环相角
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void U_Decomposition_PLL(void);
/*==================================================================
 * 函数功能: 对端口电流作 dq 变换并分离正序、负序及谐波分量
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void Iin_Decomposition(void);
/*==================================================================
 * 函数功能: 对负载电流作 dq 变换并提取基波与谐波分量
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void Iload_Decomposition(void);
/*==================================================================
 * 函数功能: 根据电网电压、电流 dq 分量计算并滤波有功、无功功率
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void P_Q_Calculator(void);
/*==================================================================
 * 函数功能: 按旧 STATCOM 工作模式生成无功功率目标
 * 输入参数: StatcomIndex：旧 STATCOM 无功、功率因数或电压模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Q_Reference(uchar StatcomIndex);
/*==================================================================
 * 函数功能: 按各相在线模块数计算相平均及三相平均直流电压
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Udc_Average(void);
/*==================================================================
 * 函数功能: 按旧直流电压参考执行增量 PI，生成 d 轴电流参考
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Total_Udc_Controller(void);
/*==================================================================
 * 函数功能: 按旧 STATCOM/APF 模式合成正序、负序及谐波电流参考
 * 输入参数: ApfIndex：旧 APF 谐波补偿模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_I_Reference(uchar ApfIndex);
/*==================================================================
 * 函数功能: 执行旧 dq 电流环、谐振补偿及三相 SPWM 给定合成
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Current_Controller(void);
/*==================================================================
 * 函数功能: 按旧 SVG 控制链顺序执行分解、功率、参考和电流控制
 * 输入参数: StatcomIndex：旧 STATCOM 无功、功率因数或电压模式选择
 *           ApfIndex：旧 APF 谐波补偿模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Controller(uchar StatcomIndex,uchar ApfIndex);


#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*              H2_100 Filter                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 H2/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH2_100_Filter(float FilterIN)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3,tempjs_4; /* tempjs_0：差分式当前系数，最后用于输出增益。 */
	/* tempjs_1：本拍及前两拍输入项的累计值。 */
	/* tempjs_2：输入缓存及本拍滤波输出计算值。 */
	/* tempjs_3：前一拍输出乘第一反馈系数的缓存。 */
	/* tempjs_4：前两拍输出乘第二反馈系数的缓存。 */
	
	tempjs_0=H2_100_Filter_Num_1;
	tempjs_1=H2_100_Filter.H_Filter.IN_n_1;
	tempjs_1=tempjs_1*tempjs_0;
	
	tempjs_2=FilterIN;
	tempjs_1=tempjs_1+tempjs_2;
	tempjs_2=H2_100_Filter.H_Filter.IN_n_2;
	tempjs_1=tempjs_1+tempjs_2;
	
	tempjs_0=H2_100_Filter_Den_1;
	tempjs_3=H2_100_Filter.H_Filter.OUT_n_1;
	tempjs_3=tempjs_3*tempjs_0;

	tempjs_0=H2_100_Filter_Den_0;
	tempjs_4=H2_100_Filter.H_Filter.OUT_n_2;
	tempjs_4=tempjs_4*tempjs_0;	

	H2_100_Filter.H_Filter.IN_n_2=H2_100_Filter.H_Filter.IN_n_1;
	H2_100_Filter.H_Filter.IN_n_1=FilterIN;
	H2_100_Filter.H_Filter.OUT_n_2=H2_100_Filter.H_Filter.OUT_n_1;
	
	tempjs_2=tempjs_1-tempjs_3-tempjs_4;
	H2_100_Filter.H_Filter.OUT_n_1=tempjs_2;
	tempjs_0=H2_100_Filter_Gain;
	tempjs_2=tempjs_2*tempjs_0;
	
	return(tempjs_2);		
}

/****************************************************************************/
/*                                                                          */
/*              H6_100 Filter                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 H6/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH6_100_Filter(float FilterIN)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3,tempjs_4; /* tempjs_0：差分式当前系数，最后用于输出增益。 */
	/* tempjs_1：本拍及前两拍输入项的累计值。 */
	/* tempjs_2：输入缓存及本拍滤波输出计算值。 */
	/* tempjs_3：前一拍输出乘第一反馈系数的缓存。 */
	/* tempjs_4：前两拍输出乘第二反馈系数的缓存。 */
	
	tempjs_0=H6_100_Filter_Num_1;
	tempjs_1=H6_100_Filter.H_Filter.IN_n_1;
	tempjs_1=tempjs_1*tempjs_0;
	
	tempjs_2=FilterIN;
	tempjs_1=tempjs_1+tempjs_2;
	tempjs_2=H6_100_Filter.H_Filter.IN_n_2;
	tempjs_1=tempjs_1+tempjs_2;
	
	tempjs_0=H6_100_Filter_Den_1;
	tempjs_3=H6_100_Filter.H_Filter.OUT_n_1;
	tempjs_3=tempjs_3*tempjs_0;

	tempjs_0=H6_100_Filter_Den_0;
	tempjs_4=H6_100_Filter.H_Filter.OUT_n_2;
	tempjs_4=tempjs_4*tempjs_0;	

	H6_100_Filter.H_Filter.IN_n_2=H6_100_Filter.H_Filter.IN_n_1;
	H6_100_Filter.H_Filter.IN_n_1=FilterIN;
	H6_100_Filter.H_Filter.OUT_n_2=H6_100_Filter.H_Filter.OUT_n_1;
	
	tempjs_2=tempjs_1-tempjs_3-tempjs_4;
	H6_100_Filter.H_Filter.OUT_n_1=tempjs_2;
	tempjs_0=H6_100_Filter_Gain;
	tempjs_2=tempjs_2*tempjs_0;
	
	return(tempjs_2);		
}

/****************************************************************************/
/*                                                                          */
/*              H12_100 Filter                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 H12/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH12_100_Filter(float FilterIN)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3,tempjs_4; /* tempjs_0：差分式当前系数，最后用于输出增益。 */
	/* tempjs_1：本拍及前两拍输入项的累计值。 */
	/* tempjs_2：输入缓存及本拍滤波输出计算值。 */
	/* tempjs_3：前一拍输出乘第一反馈系数的缓存。 */
	/* tempjs_4：前两拍输出乘第二反馈系数的缓存。 */
	
	tempjs_0=H12_100_Filter_Num_1;
	tempjs_1=H12_100_Filter.H_Filter.IN_n_1;
	tempjs_1=tempjs_1*tempjs_0;
	
	tempjs_2=FilterIN;
	tempjs_1=tempjs_1+tempjs_2;
	tempjs_2=H12_100_Filter.H_Filter.IN_n_2;
	tempjs_1=tempjs_1+tempjs_2;
	
	tempjs_0=H12_100_Filter_Den_1;
	tempjs_3=H12_100_Filter.H_Filter.OUT_n_1;
	tempjs_3=tempjs_3*tempjs_0;

	tempjs_0=H12_100_Filter_Den_0;
	tempjs_4=H12_100_Filter.H_Filter.OUT_n_2;
	tempjs_4=tempjs_4*tempjs_0;	

	H12_100_Filter.H_Filter.IN_n_2=H12_100_Filter.H_Filter.IN_n_1;
	H12_100_Filter.H_Filter.IN_n_1=FilterIN;
	H12_100_Filter.H_Filter.OUT_n_2=H12_100_Filter.H_Filter.OUT_n_1;
	
	tempjs_2=tempjs_1-tempjs_3-tempjs_4;
	H12_100_Filter.H_Filter.OUT_n_1=tempjs_2;
	tempjs_0=H12_100_Filter_Gain;
	tempjs_2=tempjs_2*tempjs_0;
	
	return(tempjs_2);		
}

/****************************************************************************/
/*                                                                          */
/*              H2_20 Filter                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 H2/20 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
float BandH2_20_Filter(float FilterIN)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3,tempjs_4; /* tempjs_0：差分式当前系数，最后用于输出增益。 */
	/* tempjs_1：本拍及前两拍输入项的累计值。 */
	/* tempjs_2：输入缓存及本拍滤波输出计算值。 */
	/* tempjs_3：前一拍输出乘第一反馈系数的缓存。 */
	/* tempjs_4：前两拍输出乘第二反馈系数的缓存。 */
	
	tempjs_0=H2_20_Filter_Num_1;
	tempjs_1=H2_20_Filter.H_Filter.IN_n_1;
	tempjs_1=tempjs_1*tempjs_0;
	
	tempjs_2=FilterIN;
	tempjs_1=tempjs_1+tempjs_2;
	tempjs_2=H2_20_Filter.H_Filter.IN_n_2;
	tempjs_1=tempjs_1+tempjs_2;
	
	tempjs_0=H2_20_Filter_Den_1;
	tempjs_3=H2_20_Filter.H_Filter.OUT_n_1;
	tempjs_3=tempjs_3*tempjs_0;

	tempjs_0=H2_20_Filter_Den_0;
	tempjs_4=H2_20_Filter.H_Filter.OUT_n_2;
	tempjs_4=tempjs_4*tempjs_0;	

	H2_20_Filter.H_Filter.IN_n_2=H2_20_Filter.H_Filter.IN_n_1;
	H2_20_Filter.H_Filter.IN_n_1=FilterIN;
	H2_20_Filter.H_Filter.OUT_n_2=H2_20_Filter.H_Filter.OUT_n_1;
	
	tempjs_2=tempjs_1-tempjs_3-tempjs_4;
	H2_20_Filter.H_Filter.OUT_n_1=tempjs_2;
	tempjs_0=H2_20_Filter_Gain;
	tempjs_2=tempjs_2*tempjs_0;
	
	return(tempjs_2);		
}

/****************************************************************************/
/*                                                                          */
/*              Sine Current Controller                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 2 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_2_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
float SineHar_2_Controller(float ErrorIn)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3,tempjs_4; /* tempjs_0：差分式当前系数/累计计算项。 */
	/* tempjs_1：本拍输入项的乘法缓存。 */
	/* tempjs_2：前一拍输入/输出项的乘法缓存。 */
	/* tempjs_3：历史输入/输出项与当前输出的累计缓存。 */
	/* tempjs_4：第二阶历史项的乘法缓存。 */

	tempjs_0=Har_2_Controller_Num_2;
	tempjs_1=ErrorIn;
	tempjs_1=tempjs_1*tempjs_0;

	tempjs_0=Har_2_Controller_Num_1;
	tempjs_2=Har_2_Controller.Har_Controller.IN_n_1;
	tempjs_2=tempjs_2*tempjs_0;	

	tempjs_0=Har_2_Controller_Num_0;
	tempjs_3=Har_2_Controller.Har_Controller.IN_n_2;
	tempjs_3=tempjs_3*tempjs_0;	
	
	tempjs_1=tempjs_1+tempjs_2+tempjs_3;
		
	
	tempjs_0=Har_2_Controller_Den_1;
	tempjs_3=Har_2_Controller.Har_Controller.OUT_n_1;
	tempjs_3=tempjs_3*tempjs_0;

	tempjs_4=Har_2_Controller.Har_Controller.OUT_n_2;
	
	
	Har_2_Controller.Har_Controller.IN_n_2=Har_2_Controller.Har_Controller.IN_n_1;
	Har_2_Controller.Har_Controller.IN_n_1=ErrorIn;
	Har_2_Controller.Har_Controller.OUT_n_2=Har_2_Controller.Har_Controller.OUT_n_1;
	
	tempjs_2=tempjs_1-tempjs_3-tempjs_4;
	Har_2_Controller.Har_Controller.OUT_n_1=tempjs_2;
	
	return(tempjs_2);		
}

/****************************************************************************/
/*                                                                          */
/*              Sine Current Controller                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 6 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_6_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
float SineHar_6_Controller(float ErrorIn)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3,tempjs_4; /* tempjs_0：差分式当前系数/累计计算项。 */
	/* tempjs_1：本拍输入项的乘法缓存。 */
	/* tempjs_2：前一拍输入/输出项的乘法缓存。 */
	/* tempjs_3：历史输入/输出项与当前输出的累计缓存。 */
	/* tempjs_4：第二阶历史项的乘法缓存。 */

	tempjs_0=Har_6_Controller_Num_2;
	tempjs_1=ErrorIn;
	tempjs_1=tempjs_1*tempjs_0;

	tempjs_0=Har_6_Controller_Num_1;
	tempjs_2=Har_6_Controller.Har_Controller.IN_n_1;
	tempjs_2=tempjs_2*tempjs_0;	

	tempjs_0=Har_6_Controller_Num_0;
	tempjs_3=Har_6_Controller.Har_Controller.IN_n_2;
	tempjs_3=tempjs_3*tempjs_0;	
	
	tempjs_1=tempjs_1+tempjs_2+tempjs_3;
		
	
	tempjs_0=Har_6_Controller_Den_1;
	tempjs_3=Har_6_Controller.Har_Controller.OUT_n_1;
	tempjs_3=tempjs_3*tempjs_0;

	tempjs_4=Har_6_Controller.Har_Controller.OUT_n_2;
	
	
	Har_6_Controller.Har_Controller.IN_n_2=Har_6_Controller.Har_Controller.IN_n_1;
	Har_6_Controller.Har_Controller.IN_n_1=ErrorIn;
	Har_6_Controller.Har_Controller.OUT_n_2=Har_6_Controller.Har_Controller.OUT_n_1;
	
	tempjs_2=tempjs_1-tempjs_3-tempjs_4;
	Har_6_Controller.Har_Controller.OUT_n_1=tempjs_2;
	
	return(tempjs_2);		
}

/****************************************************************************/
/*                                                                          */
/*              Sine Current Controller                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 12 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_12_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
float SineHar_12_Controller(float ErrorIn)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3,tempjs_4; /* tempjs_0：差分式当前系数/累计计算项。 */
	/* tempjs_1：本拍输入项的乘法缓存。 */
	/* tempjs_2：前一拍输入/输出项的乘法缓存。 */
	/* tempjs_3：历史输入/输出项与当前输出的累计缓存。 */
	/* tempjs_4：第二阶历史项的乘法缓存。 */

	tempjs_0=Har_12_Controller_Num_2;
	tempjs_1=ErrorIn;
	tempjs_1=tempjs_1*tempjs_0;

	tempjs_0=Har_12_Controller_Num_1;
	tempjs_2=Har_12_Controller.Har_Controller.IN_n_1;
	tempjs_2=tempjs_2*tempjs_0;	

	tempjs_0=Har_12_Controller_Num_0;
	tempjs_3=Har_12_Controller.Har_Controller.IN_n_2;
	tempjs_3=tempjs_3*tempjs_0;	
	
	tempjs_1=tempjs_1+tempjs_2+tempjs_3;
		
	
	tempjs_0=Har_12_Controller_Den_1;
	tempjs_3=Har_12_Controller.Har_Controller.OUT_n_1;
	tempjs_3=tempjs_3*tempjs_0;

	tempjs_4=Har_12_Controller.Har_Controller.OUT_n_2;
	
	
	Har_12_Controller.Har_Controller.IN_n_2=Har_12_Controller.Har_Controller.IN_n_1;
	Har_12_Controller.Har_Controller.IN_n_1=ErrorIn;
	Har_12_Controller.Har_Controller.OUT_n_2=Har_12_Controller.Har_Controller.OUT_n_1;
	
	tempjs_2=tempjs_1-tempjs_3-tempjs_4;
	Har_12_Controller.Har_Controller.OUT_n_1=tempjs_2;
	
	return(tempjs_2);		
}


#ifdef SimHVAC
/****************************************************************************/
/*                                                                          */
/*              Voltage Decomposition & PLL                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 对电网电压作坐标变换及序分量滤波，推进原锁相环相角
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void U_Decomposition_PLL(void)
{
//	ushort att;
	float tempTrans,temp_0,temp_1; /* tempTrans：电压坐标变换/PLL 相角计算的浮点中间缓存 tempTrans。 */
	/* temp_0：电压坐标变换/PLL 相角计算的浮点中间缓存 temp_0。 */
	/* temp_1：电压坐标变换/PLL 相角计算的浮点中间缓存 temp_1。 */
//	float temp_2,temp_3,temp_freq;
//	float D_Trans_temp,Q_Trans_temp;

	//edited by shiqi
	if ((HA_Port_Volt_A > (HA_Port_Volt_A_last+1000))||(HA_Port_Volt_A < (HA_Port_Volt_A_last-1000))||(HA_Port_Volt_B > (HA_Port_Volt_B_last+1000))||(HA_Port_Volt_B < (HA_Port_Volt_B_last-1000))||(HA_Port_Volt_C > (HA_Port_Volt_C_last+1000))||(HA_Port_Volt_C < (HA_Port_Volt_C_last-1000)))
	{
		Samplenoise = 0;
	}
	else
	{
		Samplenoise = 1;
	}
	HA_Port_Volt_A_last = HA_Port_Volt_A;
	HA_Port_Volt_B_last = HA_Port_Volt_B;
	HA_Port_Volt_C_last = HA_Port_Volt_C;

	if (Samplenoise == 1)
	{
		VA_Control = HA_Port_Volt_A;
		VB_Control = HA_Port_Volt_B;
		VC_Control = HA_Port_Volt_C;
	}
	Clarke_Transformation(VA_Control, VB_Control, VC_Control);
	//Clarke_Transformation(HA_Port_Volt_A, HA_Port_Volt_B, HA_Port_Volt_C);
	//end

	Sin_ph_Trans_NEW=sinf(Phase_PLL_First);
	Cos_ph_Trans_NEW=cosf(Phase_PLL_First);
		
	Park_Transformation(Alpha_Trans, Beta_Trans, Cos_ph_Trans_NEW, Sin_ph_Trans_NEW);

	temp_0=LPF_20HZ_X_Ud_1*0.00626;
	temp_1=LPF_20HZ_Y_Ud_1*0.9937;
	temp_1=temp_1+temp_0;
	LPF_20HZ_X_Ud_1=D_Trans;
	LPF_20HZ_Y_Ud_1=temp_1;
	Ugrid_d_Positive=temp_1;
/*
	H_Filter_Memcopy(&U_d_Grid_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	Ugrid_d_Positive=BandH6_100_Filter(temp_1);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&U_d_Grid_H6_100_Filter.H_Filter_BUF[0]);
*/	
	temp_0=LPF_20HZ_X_Uq_1*0.00626;
	temp_1=LPF_20HZ_Y_Uq_1*0.9937;
	temp_1=temp_1+temp_0;
	LPF_20HZ_X_Uq_1=Q_Trans;
	LPF_20HZ_Y_Uq_1=temp_1;
	Ugrid_q_Positive=temp_1;		

/*
	H_Filter_Memcopy(&U_q_Grid_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	Ugrid_q_Positive=BandH6_100_Filter(temp_1);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&U_q_Grid_H6_100_Filter.H_Filter_BUF[0]);
*/
	Park_InverseTransformation(Ugrid_d_Positive,Ugrid_q_Positive,Cos_ph_Trans_NEW,Sin_ph_Trans_NEW);
	
/*		
	Clarke_Transformation(HA_Port_Volt_A, HA_Port_Volt_B, HA_Port_Volt_C);
	if(flag_Standard==1)
	{
		Alpha_Trans=Alpha_Trans/StandardValue;
		Beta_Trans=Beta_Trans/StandardValue;
	}
	Sin_ph_Trans_NEW=sinf(Phase_PLL_First);
	Cos_ph_Trans_NEW=cosf(Phase_PLL_First);
		
	Park_Transformation(Alpha_Trans, Beta_Trans, Cos_ph_Trans_NEW, Sin_ph_Trans_NEW);
	
	H_Filter_Memcopy(&U_d_Grid_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH2_100_Filter(D_Trans);
	H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&U_d_Grid_H2_100_Filter.H_Filter_BUF[0]);

	H_Filter_Memcopy(&U_d_Grid_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH6_100_Filter(tempTrans);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&U_d_Grid_H6_100_Filter.H_Filter_BUF[0]);
		
	H_Filter_Memcopy(&U_d_Grid_H12_100_Filter.H_Filter_BUF[0], &H12_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH12_100_Filter(tempTrans);
	H_Filter_Memcopy(&H12_100_Filter.H_Filter_BUF[0],&U_d_Grid_H12_100_Filter.H_Filter_BUF[0]);
	Ugrid_d_Positive=tempTrans;	

	H_Filter_Memcopy(&U_q_Grid_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH2_100_Filter(Q_Trans);
	H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&U_q_Grid_H2_100_Filter.H_Filter_BUF[0]);

	H_Filter_Memcopy(&U_q_Grid_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH6_100_Filter(tempTrans);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&U_q_Grid_H6_100_Filter.H_Filter_BUF[0]);
		
	H_Filter_Memcopy(&U_q_Grid_H12_100_Filter.H_Filter_BUF[0], &H12_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH12_100_Filter(tempTrans);
	H_Filter_Memcopy(&H12_100_Filter.H_Filter_BUF[0],&U_q_Grid_H12_100_Filter.H_Filter_BUF[0]);
	Ugrid_q_Positive=tempTrans;

	Park_InverseTransformation(Ugrid_d_Positive,Ugrid_q_Positive,Cos_ph_Trans_NEW,Sin_ph_Trans_NEW);

	if(flag_Standard==1)
	{
		Alpha_Trans=Alpha_Trans*StandardValue;
		Beta_Trans=Beta_Trans*StandardValue;
	}	
*/
			
	temp_1=TAD_PLL;
	tempTrans=314.15926*temp_1;

	Phase_PLL_First=Phase_PLL_First+tempTrans;
	
	while(Phase_PLL_First>6.2831852)
	{
		Phase_PLL_First=Phase_PLL_First-6.2831852;
	}	
	
	temp_0=Alpha_Trans;
	temp_0=temp_0*temp_0;
	temp_1=Beta_Trans;
	temp_1=temp_1*temp_1;
	temp_1=temp_1+temp_0;
	
	temp_1=1.0/rsqrtf(temp_1);
	
	Sin_ph_Trans_NEW=Beta_Trans/temp_1;
	Cos_ph_Trans_NEW=Alpha_Trans/temp_1;
	
//Clarke_Transformation(HA_Port_Volt_A, HA_Port_Volt_B, HA_Port_Volt_C);
	Park_Transformation(Alpha_Trans, Beta_Trans, Cos_ph_Trans_NEW, Sin_ph_Trans_NEW);
	Ugrid_d_Positive=D_Trans;
	Ugrid_q_Positive=Q_Trans;
	Ugrid_Max=Ugrid_d_Positive;
	
/*	
	Ugrid_Max=Ugrid_d_Positive;
	Ugrid_Max=Ugrid_Max*Ugrid_Max;
	temp_0=Ugrid_q_Positive;
	temp_0=temp_0*temp_0;
	temp_0=temp_0+Ugrid_Max;
	Ugrid_Max=1.0/rsqrtf(temp_0);
*/	
	
	Phase_PLL=asinf(Sin_ph_Trans_NEW);
	
	if(Cos_ph_Trans_NEW<0.0)
	{
		Phase_PLL=3.14159-Phase_PLL;
	}
	PLL_Phase_Com_NEW=Phase_PLL+PhaseCom;
	Sin_ph_Trans_Com_NEW=sinf(PLL_Phase_Com_NEW);
	Cos_ph_Trans_Com_NEW=cosf(PLL_Phase_Com_NEW);	

	Sin_ph_Trans_NEW=sinf(Phase_PLL+SoftwarePhaseComValue);
	Cos_ph_Trans_NEW=cosf(Phase_PLL+SoftwarePhaseComValue);

////////////////////////////////////////////////////////////
//	temp_1=cosf(Phase_PLL+SoftwarePhaseComValue);
//	temp_2=cosf(Phase_PLL+SoftwarePhaseComValue-2.0*3.1415926/3.0);	
//	
//	DAC_BUF[1]=Ugrid_Max*(temp_1-temp_2)/10000.0+2.5;
//	DAC_BUF[2]=Ugrid_d_Positive/2500.0+1.0;
//	DAC_BUF[3]=Ugrid_q_Positive/100.0+2.5;		
////////////////////////////////////////////////////////////

/*
	if(flag_Standard==1)
	{
		Alpha_Trans=Alpha_Trans/StandardValue;
		Beta_Trans=Beta_Trans/StandardValue;
	}

	Park_Transformation(Alpha_Trans, Beta_Trans, Cos_ph_Trans, Sin_ph_Trans);
	
	Ugrid_hp_d=D_Trans;
	Ugrid_hp_q=Q_Trans;
		
	if(flag_Standard==1)
	{
		Ugrid_hp_d=Ugrid_hp_d*StandardValue;
		Ugrid_hp_q=Ugrid_hp_q*StandardValue;
	}	
	
	//d axis
	H_Filter_Memcopy(&U_d_Grid_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH2_100_Filter(D_Trans);
	H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&U_d_Grid_H2_100_Filter.H_Filter_BUF[0]);

	H_Filter_Memcopy(&U_d_Grid_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH6_100_Filter(tempTrans);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&U_d_Grid_H6_100_Filter.H_Filter_BUF[0]);
		
	H_Filter_Memcopy(&U_d_Grid_H12_100_Filter.H_Filter_BUF[0], &H12_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH12_100_Filter(tempTrans);
	H_Filter_Memcopy(&H12_100_Filter.H_Filter_BUF[0],&U_d_Grid_H12_100_Filter.H_Filter_BUF[0]);
	Ugrid_d_Positive=tempTrans;

	//q axis
	H_Filter_Memcopy(&U_q_Grid_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH2_100_Filter(Q_Trans);
	H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&U_q_Grid_H2_100_Filter.H_Filter_BUF[0]);

	H_Filter_Memcopy(&U_q_Grid_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH6_100_Filter(tempTrans);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&U_q_Grid_H6_100_Filter.H_Filter_BUF[0]);
		
	H_Filter_Memcopy(&U_q_Grid_H12_100_Filter.H_Filter_BUF[0], &H12_100_Filter.H_Filter_BUF[0]);
	tempTrans=BandH12_100_Filter(tempTrans);
	H_Filter_Memcopy(&H12_100_Filter.H_Filter_BUF[0],&U_q_Grid_H12_100_Filter.H_Filter_BUF[0]);
	Ugrid_q_Positive=tempTrans;

	if(flag_Standard==1)
	{
		Ugrid_d_Positive=Ugrid_d_Positive*StandardValue;
		Ugrid_q_Positive=Ugrid_q_Positive*StandardValue;
	}

	Ugrid_Max=Ugrid_d_Positive;
	Ugrid_Max=Ugrid_Max*Ugrid_Max;
	temp_0=Ugrid_q_Positive;
	temp_0=temp_0*temp_0;
	temp_0=temp_0+Ugrid_Max;
	Ugrid_Max=1.0/rsqrtf(temp_0);

	//PLL
	temp_1=0.0-tempTrans;

	PLL_PID_Controller.PID_Controller.Num_1=PLL_PID_Num_1;
	PLL_PID_Controller.PID_Controller.Num_0=PLL_PID_Num_0;	
	H_Filter_Memcopy(&PLL_PID_Controller.PID_Controller_BUF[0],&PID_Control_buf.PID_Controller_BUF[0]);
	temp_1=ConstantPID_Controller(temp_1);
	H_Filter_Memcopy(&PID_Control_buf.PID_Controller_BUF[0],&PLL_PID_Controller.PID_Controller_BUF[0]);
	
	tempTrans=temp_1+314.15926;
	
	temp_freq=tempTrans/6.2831852;
//	DAC_BUF[0]=temp_freq-47.0;
	
	temp_1=TAD_PLL;
	tempTrans=tempTrans*temp_1;
	
	temp_1=Phase_PLL;
	Phase_PLL=tempTrans+temp_1;
	
	while(Phase_PLL>6.2831852)
	{
		Phase_PLL=Phase_PLL-6.2831852;
	}
	

///////////////////////////////////////////////////////////////////
//	temp_0=Phase_PLL+0.5235987+SoftwarePhaseComValue;
//	while(temp_0>6.2831852)
//	{
//		temp_0=temp_0-6.2831852;
//	}	
//  DAC_BUF[1]=temp_0/2.0;
//
//	temp_1=cosf(Phase_PLL+SoftwarePhaseComValue);
//	temp_2=cosf(Phase_PLL+SoftwarePhaseComValue-2.0*3.1415926/3.0);	
	
//  DAC_BUF[0]=Ugrid_Max*(temp_1-temp_2)/10000.0+2.5;
	
//  DAC_BUF[2]=Ugrid_d_Positive/2500.0+1.0;
//  DAC_BUF[3]=Ugrid_q_Positive/100.0+2.5;
//  DAC_BUF[1]=HA_Port_Volt_A/5000.0+2.5;
//  DAC_BUF[2]=HA_Port_Volt_B/5000.0+2.5;
//  DAC_BUF[3]=HA_Port_Volt_C/5000.0+2.5;
///////////////////////////////////////////////////////////////////		
	
	Sin_ph_Trans_next=sinf(Phase_PLL);
	Cos_ph_Trans_next=cosf(Phase_PLL);
	
//	Sin_ph_Trans_NEW=sinf(Phase_PLL+SoftwarePhaseComValue);
//	Cos_ph_Trans_NEW=cosf(Phase_PLL+SoftwarePhaseComValue);	

	Sin_ph_Trans_NEW=sinf(Phase_PLL);
	Cos_ph_Trans_NEW=cosf(Phase_PLL);	
	
	PLL_Phase_Com_NEW=Phase_PLL+PhaseCom;
	
	Sin_ph_Trans_Com_NEW=sinf(PLL_Phase_Com_NEW);
	Cos_ph_Trans_Com_NEW=cosf(PLL_Phase_Com_NEW);

	if((Port_Outer_Elec==1)&&(((Port_RunStatus==PortStatusNum_Standby)||(Port_RunStatus==PortStatusNum_Initial)))&&(temp_freq<0.0))
	{
		Port_RealFault.Bbits.PORT_FAULT_bit8=1;
		PortRunFLAG.Bbits.PhaseSquence_Error=1;
	}
	else
	{
		Port_RealFault.Bbits.PORT_FAULT_bit8=0;
		PortRunFLAG.Bbits.PhaseSquence_Error=0;
	}
	
//	temp_1=cosf(PLL_Phase_Com_NEW);
//	temp_2=cosf(PLL_Phase_Com_NEW-2.0*3.1415926/3.0);
//	temp_3=cosf(PLL_Phase_Com_NEW+2.0*3.1415926/3.0);
//	DAC_BUF[1]=2.0*temp_1+2.5;
*/

	if(Add_3th==1)
	{
		temp_1=PLL_Phase_Com_NEW;
		TEATabc[3]=cosf(temp_1*3.0);
		TEATabc[3]=TEATabc[3]*(-0.167);
	}
}

/****************************************************************************/
/*                                                                          */
/*             Input Current Decomposition                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 对端口电流作 dq 变换并分离正序、负序及谐波分量
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void Iin_Decomposition(void)
{
//DAC_BUF[0]=HA_Port_Current_A/50.0+2.5;
//DAC_BUF[1]=HA_Port_Current_B/50.0+2.5;
//DAC_BUF[2]=HA_Port_Current_C/50.0+2.5;	

	Clarke_Transformation(HA_Port_Current_A, HA_Port_Current_B, HA_Port_Current_C);

	Park_Transformation(Alpha_Trans, Beta_Trans, Cos_ph_Trans_NEW, Sin_ph_Trans_NEW);	

	//edited by shiqi
	//Iin_d_Positive=D_Trans;
	//Iin_q_Positive=Q_Trans;
    if((D_Trans > (Iin_d_Positive_last+20))||(D_Trans < (Iin_d_Positive_last-20))||(Q_Trans > (Iin_q_Positive_last+20))||(D_Trans < (Iin_q_Positive_last-20)))
    {
    	Samplenoise_Idq = 0;
    }
    else
    {
    	Samplenoise_Idq = 1;
    }

    Iin_d_Positive_last = D_Trans;
    Iin_q_Positive_last = Q_Trans;

    if(Samplenoise_Idq == 1)
    {
        Iin_d_Positive = D_Trans;
        Iin_q_Positive = Q_Trans;
    }

	if(Iin_d_Positive > 200)
	{
		Iin_d_Positive = 200;
	}
	if(Iin_d_Positive < -200)
	{
		Iin_d_Positive = -200;
	}
	if(Iin_q_Positive > 200)
	{
		Iin_q_Positive = 200;
	}
	if(Iin_q_Positive < -200)
	{
		Iin_q_Positive = -200;
	}
	//end
}

/****************************************************************************/
/*                                                                          */
/*              Load Current Decomposition                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 对负载电流作 dq 变换并提取基波与谐波分量
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void Iload_Decomposition(void)
{
	float tempTrans1,tempTrans2,tempTrans3,hartemp; /* tempTrans1：负载电流正负序及谐波分离计算的浮点中间缓存 tempTrans1。 */
	/* tempTrans2：负载电流正负序及谐波分离计算的浮点中间缓存 tempTrans2。 */
	/* tempTrans3：负载电流正负序及谐波分离计算的浮点中间缓存 tempTrans3。 */
	/* hartemp：旧负载电流谐波分离后的分量缓存。 */
	float Iload_d_Positive,Iload_q_Positive; /* Iload_d_Positive：负载电流 d 轴/分量的正序基波值，单位 A。 */
	/* Iload_q_Positive：负载电流 q 轴/分量的正序基波值，单位 A。 */
	
	tempTrans1=HA_Port_Current_A;
	tempTrans1=tempTrans1-HA_Grid_Current_A;

	tempTrans2=HA_Port_Current_B;
	tempTrans2=tempTrans2-HA_Grid_Current_B;

	tempTrans3=HA_Port_Current_C;
	tempTrans3=tempTrans3-HA_Grid_Current_C;		
	
	Clarke_Transformation(tempTrans1, tempTrans2, tempTrans3);
	

	Park_Transformation(Alpha_Trans, Beta_Trans, Cos_ph_Trans_NEW, Sin_ph_Trans_NEW);	

	//d axis
	H_Filter_Memcopy(&I_d_Load_H2_20_Filter.H_Filter_BUF[0], &H2_20_Filter.H_Filter_BUF[0]);
	tempTrans1=BandH2_20_Filter(D_Trans);
	H_Filter_Memcopy(&H2_20_Filter.H_Filter_BUF[0],&I_d_Load_H2_20_Filter.H_Filter_BUF[0]);
	hartemp=tempTrans1;
	
	H_Filter_Memcopy(&I_d_Load_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	tempTrans2=BandH6_100_Filter(tempTrans1);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&I_d_Load_H6_100_Filter.H_Filter_BUF[0]);
		
	H_Filter_Memcopy(&I_d_Load_H12_100_Filter.H_Filter_BUF[0], &H12_100_Filter.H_Filter_BUF[0]);
	tempTrans3=BandH12_100_Filter(tempTrans2);
	H_Filter_Memcopy(&H12_100_Filter.H_Filter_BUF[0],&I_d_Load_H12_100_Filter.H_Filter_BUF[0]);
	Iload_d_Positive=tempTrans3;
	Iload_har_d=hartemp-Iload_d_Positive;
	
	//q axis
	H_Filter_Memcopy(&I_q_Load_H2_20_Filter.H_Filter_BUF[0], &H2_20_Filter.H_Filter_BUF[0]);
	tempTrans1=BandH2_20_Filter(Q_Trans);
	H_Filter_Memcopy(&H2_20_Filter.H_Filter_BUF[0],&I_q_Load_H2_20_Filter.H_Filter_BUF[0]);
	hartemp=tempTrans1;
	
	H_Filter_Memcopy(&I_q_Load_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
	tempTrans2=BandH6_100_Filter(tempTrans1);
	H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&I_q_Load_H6_100_Filter.H_Filter_BUF[0]);
		
	H_Filter_Memcopy(&I_q_Load_H12_100_Filter.H_Filter_BUF[0], &H12_100_Filter.H_Filter_BUF[0]);
	tempTrans3=BandH12_100_Filter(tempTrans2);
	H_Filter_Memcopy(&H12_100_Filter.H_Filter_BUF[0],&I_q_Load_H12_100_Filter.H_Filter_BUF[0]);
	Iload_q_Positive=tempTrans3;
	Iload_har_q=hartemp-Iload_q_Positive;
}

#else

#endif

/****************************************************************************/
/*                                                                          */
/*              P,Q Calculator                                              */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 根据电网电压、电流 dq 分量计算并滤波有功、无功功率
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void P_Q_Calculator(void)
{
	float AD_CalTemp,AD_CalTemp1,tempTrans1,tempTrans2,tempTrans3; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
	/* AD_CalTemp1：采样/功率换算的第二个浮点中间值。 */
	/* tempTrans1：dq 电压电流乘积与功率滤波计算的浮点中间缓存 tempTrans1。 */
	/* tempTrans2：dq 电压电流乘积与功率滤波计算的浮点中间缓存 tempTrans2。 */
	/* tempTrans3：dq 电压电流乘积与功率滤波计算的浮点中间缓存 tempTrans3。 */
		
//P	
	AD_CalTemp=HA_Port_Volt_A;
	AD_CalTemp=AD_CalTemp*HA_Grid_Current_A;

	AD_CalTemp1=HA_Port_Volt_B;
	AD_CalTemp1=AD_CalTemp1*HA_Grid_Current_B;
	
	AD_CalTemp+=AD_CalTemp1;
	
	AD_CalTemp1=HA_Port_Volt_C;
	AD_CalTemp1=AD_CalTemp1*HA_Grid_Current_C;
		
	AD_CalTemp+=AD_CalTemp1;

	H_Filter_Memcopy(&P_H2_100_Filter.H_Filter_BUF[0],&H2_100_Filter.H_Filter_BUF[0]);
	Grid_P_Power=BandH2_100_Filter(AD_CalTemp);
	H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&P_H2_100_Filter.H_Filter_BUF[0]);	

	//Q
	tempTrans1=HA_Port_Current_A;
	tempTrans1=tempTrans1-HA_Grid_Current_A;
	tempTrans2=HA_Port_Current_B;
	tempTrans2=tempTrans2-HA_Grid_Current_B;
	tempTrans3=HA_Port_Current_C;
	tempTrans3=tempTrans3-HA_Grid_Current_C;	

	AD_CalTemp=HA_Port_Volt_AB;
	AD_CalTemp=AD_CalTemp*tempTrans3;
	AD_CalTemp1=HA_Port_Volt_BC;
	AD_CalTemp1=AD_CalTemp1*tempTrans1;
	AD_CalTemp+=AD_CalTemp1;
	
	AD_CalTemp1=HA_Port_Volt_CA;
	AD_CalTemp1=AD_CalTemp1*tempTrans2;
	AD_CalTemp+=AD_CalTemp1;
	AD_CalTemp=AD_CalTemp/1.732;
			
	H_Filter_Memcopy(&Q_Load_H2_100_Filter.H_Filter_BUF[0],&H2_100_Filter.H_Filter_BUF[0]);
	Load_Q_Power=BandH2_100_Filter(AD_CalTemp);
	H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&Q_Load_H2_100_Filter.H_Filter_BUF[0]);	
}

/****************************************************************************/
/*                                                                          */
/*              SVG Q reference Calculator                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧 STATCOM 工作模式生成无功功率目标
 * 输入参数: StatcomIndex：旧 STATCOM 无功、功率因数或电压模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Q_Reference(uchar StatcomIndex)
{
	float tempTrans1,tempTrans2,tempTrans3; /* tempTrans1：无功目标及功率因数换算计算的浮点中间缓存 tempTrans1。 */
	/* tempTrans2：无功目标及功率因数换算计算的浮点中间缓存 tempTrans2。 */
	/* tempTrans3：无功目标及功率因数换算计算的浮点中间缓存 tempTrans3。 */
	switch(StatcomIndex)
	{
		case 0:
		{
			SVG_Q_Power_Ref=0.0;	
			Iq_Positive_Ref=0.0;		
		}break;
		case 1:
		{
			SVG_Q_Power_Ref=PORT_Regulate_DATA.STATCOM_Q_SET;
			
			tempTrans1=Ugrid_Max;
			tempTrans1=tempTrans1*(-1.5);
			Iq_Positive_Ref=SVG_Q_Power_Ref/tempTrans1;		
		
		}break;
		case 2:
		{
			if(PORT_Regulate_DATA.STATCOM_PF_SET==0.0)
			{
				Iq_Positive_Ref=0.0;
			}
			else
			{
				tempTrans1=PORT_Regulate_DATA.STATCOM_PF_SET;
				tempTrans2=tempTrans1;
								
				tempTrans3=tempTrans2*tempTrans2;
				tempTrans3=1.0-tempTrans3;
											
				tempTrans2=rsqrtf(tempTrans3);
								
				tempTrans3=Grid_P_Power;
				tempTrans3=tempTrans3/tempTrans2;
				tempTrans3=tempTrans3/tempTrans1;
				
				SVG_Q_Power_Ref=tempTrans3+Load_Q_Power;
										
				//iq reference
				tempTrans1=Ugrid_Max;
				tempTrans1=tempTrans1*(-1.5);
				Iq_Positive_Ref=SVG_Q_Power_Ref/tempTrans1;		
	
			}		
		}break;
		case 3:
		{
			//PORT_Regulate_DATA.STATCOM_V_SET
			//SVG_Q_Power_Ref=
		}break;
		default:break;
	}
		
	if(Iq_Positive_Ref>Iq_Positive_Max)
	{
		Iq_Positive_Ref=Iq_Positive_Max;
	}
	if(Iq_Positive_Ref<Iq_Positive_Min)
	{
		Iq_Positive_Ref=Iq_Positive_Min;
	}
}
/****************************************************************************/
/*                                                                          */
/*              SVG Udc Average Calculator                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按各相在线模块数计算相平均及三相平均直流电压
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Udc_Average(void)
{
#if SST_BENCH_INPUT_SIM
    return; /* Port_AD_Sample已注入三相及总均压，测试时不以真实FPGA采样覆盖。 */
#else
	short SampleBuf_short_1; /* SampleBuf_short_1：第一个 FPGA 原始采样/汇总字缓存。 */
	short SampleBuf_short_2; /* SampleBuf_short_2：第二个 FPGA 原始采样/汇总字缓存。 */
	float tempAve1,tempAve2,tempAve3; /* tempAve1：第一相/第一组模块采样总和与平均值缓存。 */
	/* tempAve2：第二相/第二组模块采样总和与平均值缓存。 */
	/* tempAve3：第三相模块采样总和与平均值缓存。 */
//	float temp_0,temp_1;
	uchar PluginUnit_ONNum; /* PluginUnit_ONNum：参与本次平均值计算的在线模块数量。 */
	
//Phase A		

		PluginUnit_ONNum=0;
		if(HA_PM_A1)
		{
			SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_DC;
			//edited by shiqi
			//if(SampleBuf_short_1 > 1000)
			//{
			//	SampleBuf_short_1 = 1000;
			//}
			//if(SampleBuf_short_1 < 500)
			//{
			//	SampleBuf_short_1 = 500;
			//}
			//end
			PluginUnit_ONNum++;
			
			HA_DC_Log_A=0;
		}
		else
		{
			SampleBuf_short_1=0.0;
			
			HA_DC_Log_A=1;
		}
		
		if(HA_PM_A2)
		{
			SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_DC;	
			//edited by shiqi
			//if(SampleBuf_short_2 > 1000)
			//{
			//	SampleBuf_short_2 = 1000;
			//}
			//if(SampleBuf_short_2 < 500)
			//{
			//	SampleBuf_short_2 = 500;
			//}
			//end
			PluginUnit_ONNum++;
			
			if(HA_DC_Log_A==0)
			{
				if(SampleBuf_short_1<SampleBuf_short_2)
				{
					HA_DC_Log_A=1;
				}				
			}
		}
		else
		{
			SampleBuf_short_2=0.0;
		}	
		
		tempAve1=(float)SampleBuf_short_1+(float)SampleBuf_short_2;
		
		if(PluginUnit_ONNum==0)
		{
			tempAve1=0.0;
		}
		else
		{
			tempAve1=tempAve1/(float)PluginUnit_ONNum;
		}
//		DAC_BUF[1]=tempAve1/150.0;


		H_Filter_Memcopy(&Va_Ave_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
		SVG_Udc_A_Ave=BandH2_100_Filter(tempAve1);
		H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&Va_Ave_H2_100_Filter.H_Filter_BUF[0]);				
		
//		SVG_Udc_A_Ave=tempAve1;
		
//Phase B		
		PluginUnit_ONNum=0;
		if(HA_PM_B1)
		{
			SampleBuf_short_1=FPGA_Interface[Com_B1_PluginUnit_9-2].Avg_DC;
			//edited by shiqi
			//if(SampleBuf_short_1 > 1000)
			//{
			//	SampleBuf_short_1 = 1000;
			//}
			//if(SampleBuf_short_1 < 500)
			//{
			//	SampleBuf_short_1 = 500;
			//}
			//end
			PluginUnit_ONNum++;
			
			HA_DC_Log_B=0;
		}
		else
		{
			SampleBuf_short_1=0.0;
			
			HA_DC_Log_B=1;
		}
		
		if(HA_PM_B2)
		{
			SampleBuf_short_2=FPGA_Interface[Com_B2_PluginUnit_10-2].Avg_DC;
			//edited by shiqi
			//if(SampleBuf_short_2 > 1000)
			//{
			//	SampleBuf_short_2 = 1000;
			//}
			//if(SampleBuf_short_2 < 500)
			//{
			//	SampleBuf_short_2 = 500;
			//}
			//end
			PluginUnit_ONNum++;
			
			if(HA_DC_Log_B==0)
			{
				if(SampleBuf_short_1<SampleBuf_short_2)
				{
					HA_DC_Log_B=1;
				}	
			}		
		}
		else
		{
			SampleBuf_short_2=0.0;
		}	
		tempAve2=(float)SampleBuf_short_1+(float)SampleBuf_short_2;
		if(PluginUnit_ONNum==0)
		{
			tempAve2=0.0;
		}
		else
		{
			tempAve2=tempAve2/(float)PluginUnit_ONNum;
		}
		
//		DAC_BUF[2]=tempAve2/150.0;		

		H_Filter_Memcopy(&Vb_Ave_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
		SVG_Udc_B_Ave=BandH2_100_Filter(tempAve2);
		H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&Vb_Ave_H2_100_Filter.H_Filter_BUF[0]);
	
//		SVG_Udc_B_Ave=tempAve2;
		
//Phase C		
		PluginUnit_ONNum=0;
		if(HA_PM_C1)
		{
			SampleBuf_short_1=FPGA_Interface[Com_C1_PluginUnit_13-2].Avg_DC;
			//edited by shiqi
			//if(SampleBuf_short_1 > 1000)
			//{
			//	SampleBuf_short_1 = 1000;
			//}
			//if(SampleBuf_short_1 < 500)
			//{
			//	SampleBuf_short_1 = 500;
			//}
			//end
			PluginUnit_ONNum++;
			
			HA_DC_Log_C=0;
		}
		else
		{
			SampleBuf_short_1=0.0;
			
			HA_DC_Log_C=1;
		}
		
		if(HA_PM_C2)
		{
			SampleBuf_short_2=FPGA_Interface[Com_C2_PluginUnit_14-2].Avg_DC;
			//edited by shiqi
			//if(SampleBuf_short_2 > 1000)
			//{
			//	SampleBuf_short_2 = 1000;
			//}
			//if(SampleBuf_short_2 < 500)
			//{
			//	SampleBuf_short_2 = 500;
			//}
			//end
			PluginUnit_ONNum++;
			
			if(HA_DC_Log_C==0)
			{
				if(SampleBuf_short_1<SampleBuf_short_2)
				{
					HA_DC_Log_C=1;
				}				
			}
		}
		else
		{
			SampleBuf_short_2=0.0;
		}			
		tempAve3=(float)SampleBuf_short_1+(float)SampleBuf_short_2;

		if(PluginUnit_ONNum==0)
		{
			tempAve3=0.0;
		}
		else
		{
			tempAve3=tempAve3/(float)PluginUnit_ONNum;	
		}
			
//		DAC_BUF[3]=tempAve3/150.0;

		H_Filter_Memcopy(&Vc_Ave_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
		SVG_Udc_C_Ave=BandH2_100_Filter(tempAve3);
		H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&Vc_Ave_H2_100_Filter.H_Filter_BUF[0]);

//		SVG_Udc_C_Ave=tempAve3;

        //edited by shiqi
		//SVG_Udc_Ave=SVG_Udc_A_Ave;
		//SVG_Udc_Ave=SVG_Udc_Ave+SVG_Udc_B_Ave;
		//SVG_Udc_Ave=SVG_Udc_Ave+SVG_Udc_C_Ave;
		//SVG_Udc_Ave=SVG_Udc_Ave/3.0;
		SVG_Udc_Ave_temp=SVG_Udc_A_Ave;
		SVG_Udc_Ave_temp=SVG_Udc_Ave_temp+SVG_Udc_B_Ave;
		SVG_Udc_Ave_temp=SVG_Udc_Ave_temp+SVG_Udc_C_Ave;
		SVG_Udc_Ave_temp=SVG_Udc_Ave_temp/3.0;

		if((SVG_Udc_Ave_temp > (SVG_Udc_Ave_temp_last+50))||(SVG_Udc_Ave_temp < (SVG_Udc_Ave_temp_last-50)))
		{
			Samplenoise_udc = 0;
		}
		else
		{
			Samplenoise_udc = 1;
		}
		SVG_Udc_Ave_temp_last = SVG_Udc_Ave_temp;

		if(Samplenoise_udc == 1)
		{
			SVG_Udc_Ave = SVG_Udc_Ave_temp;
		}


		if(SVG_Udc_Ave > 1000)
		{
			SVG_Udc_Ave = 1000;
		}
		if(SVG_Udc_Ave < 500)
		{
			SVG_Udc_Ave = 500;
		}
        //end
		H_Filter_Memcopy(&Ave_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
		SVG_Udc_Ave=BandH6_100_Filter(SVG_Udc_Ave);
		H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&Ave_H6_100_Filter.H_Filter_BUF[0]);

		
//		DAC_BUF[0]=SVG_Udc_Ave/150.0;
		
		if(((SVG_Udc_A_Ave>(UstOK_HVAC_Port-Ust_add))&&(SVG_Udc_A_Ave<(UstOK_HVAC_Port+Ust_add)))&& \
			 ((SVG_Udc_B_Ave>(UstOK_HVAC_Port-Ust_add))&&(SVG_Udc_B_Ave<(UstOK_HVAC_Port+Ust_add)))&& \
			 ((SVG_Udc_C_Ave>(UstOK_HVAC_Port-Ust_add))&&(SVG_Udc_C_Ave<(UstOK_HVAC_Port+Ust_add))))
		{
			PortRunFLAG.Bbits.Port_UdcOK=1;     //start over
		}
		else
		{
			PortRunFLAG.Bbits.Port_UdcOK=0;
		}				
		
///////////////////////////////////////////////////////////
//		DAC_BUF[0]=SVG_Udc_A_Ave/50.0;
//		DAC_BUF[1]=SVG_Udc_B_Ave/50.0;
//		DAC_BUF[2]=SVG_Udc_C_Ave/50.0;
//		DAC_BUF[3]=SVG_Udc_Ave/50.0;
//		DAC_Output();				
#endif
}

/****************************************************************************/
/*                                                                          */
/*              Total Udc Controller                                        */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧直流电压参考执行增量 PI，生成 d 轴电流参考
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Total_Udc_Controller(void)
{
	float VoltErrorTemp; /* VoltErrorTemp：旧直流电压外环给定减反馈的误差，单位 V。 */
	if(PortRunFLAG.Bbits.PWMState_50Hz==1)     
	{
		Udc_Ref=SVG_Udc_Ave;
		Variable_RST();
	}
//	else if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(PortRunFLAG.Bbits.Port_UdcOK==0)&&(Udc_Ref!=HA_Module_RefUdc))
	else if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(Udc_Ref!=HA_Module_RefUdc))		
	{	
		VoltErrorTemp=HA_Module_RefUdc;
		
		if(SVG_Udc_Ave<(VoltErrorTemp-Ust_add-10.0))
		{
			if(Udc_Ref<(HA_Module_RefUdc-2.0))
			{
				Udc_Ref=Udc_Ref+2.0;
			}
		}
		else if(SVG_Udc_Ave>(VoltErrorTemp+Ust_add+10.0))
		{
			if(Udc_Ref>(HA_Module_RefUdc+2.0))
			{
				Udc_Ref=Udc_Ref-2.0;
			}
		}		
		else
		{
			Udc_Ref=HA_Module_RefUdc;
		}
	}

//	Udc_Ref=HA_Module_RefUdc;	
	
	
	VoltErrorTemp=Udc_Ref;
	VoltErrorTemp=VoltErrorTemp-SVG_Udc_Ave;
		
	Volt_PID_Controller.PID_Controller.Num_1=Volt_PID_Num_1;
	Volt_PID_Controller.PID_Controller.Num_0=Volt_PID_Num_0;	
	H_Filter_Memcopy(&Volt_PID_Controller.PID_Controller_BUF[0], &PID_Control_buf.PID_Controller_BUF[0]);
	Id_Positive_Ref=ConstantPID_Controller(VoltErrorTemp);
	H_Filter_Memcopy(&PID_Control_buf.PID_Controller_BUF[0],&Volt_PID_Controller.PID_Controller_BUF[0]);

/*
		H_Filter_Memcopy(&I_d_Ref_H2_100_Filter.H_Filter_BUF[0], &H2_100_Filter.H_Filter_BUF[0]);
		Id_Positive_Ref=BandH2_100_Filter(Id_Positive_Ref);
		H_Filter_Memcopy(&H2_100_Filter.H_Filter_BUF[0],&I_d_Ref_H2_100_Filter.H_Filter_BUF[0]);

		H_Filter_Memcopy(&I_d_Ref_H6_100_Filter.H_Filter_BUF[0], &H6_100_Filter.H_Filter_BUF[0]);
		Id_Positive_Ref=BandH6_100_Filter(Id_Positive_Ref);
		H_Filter_Memcopy(&H6_100_Filter.H_Filter_BUF[0],&I_d_Ref_H6_100_Filter.H_Filter_BUF[0]);
*/		
		
	if(Id_Positive_Ref>Id_Positive_Max)
	{
		Id_Positive_Ref=Id_Positive_Max;
	}
	if(Id_Positive_Ref<Id_Positive_Min)
	{
		Id_Positive_Ref=Id_Positive_Min;
	}	
}

/****************************************************************************/
/*                                                                          */
/*              SVG I reference Calculator                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧 STATCOM/APF 模式合成正序、负序及谐波电流参考
 * 输入参数: ApfIndex：旧 APF 谐波补偿模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_I_Reference(uchar ApfIndex)
{
	float tempTrans1,tempTrans2; /* tempTrans1：正负序电流参考合成计算的浮点中间缓存 tempTrans1。 */
	/* tempTrans2：正负序电流参考合成计算的浮点中间缓存 tempTrans2。 */
	switch(ApfIndex)
	{
		case 0:
		{					
		}break;
		case 1:  //Inject
		{					
			Har_5_PH=Har_5_PH+Har_5OR7_PH0;
			Har_7_PH=Har_7_PH+Har_5OR7_PH0;
			Har_11_PH=Har_11_PH+Har_11_PH0;
			
			while(Har_5_PH>6.2831852)
			{
				Har_5_PH=Har_5_PH-6.2831852;
			}
	
			while(Har_7_PH>6.2831852)
			{
				Har_7_PH=Har_7_PH-6.2831852;
			}	

			while(Har_11_PH>6.2831852)
			{
				Har_11_PH=Har_11_PH-6.2831852;
			}				
			
			tempTrans1=-cosf(Har_5_PH);	
			tempTrans2=sinf(Har_5_PH);						
			tempTrans1=tempTrans1*Har_5_RMS;
			tempTrans2=tempTrans2*Har_5_RMS;
			Id_Positive_Ref=Id_Positive_Ref+tempTrans1;
			Iq_Positive_Ref=Iq_Positive_Ref+tempTrans2;

			tempTrans1=cosf(Har_7_PH);	
			tempTrans2=sinf(Har_7_PH);						
			tempTrans1=tempTrans1*Har_7_RMS;
			tempTrans2=tempTrans2*Har_7_RMS;
			Id_Positive_Ref=Id_Positive_Ref+tempTrans1;
			Iq_Positive_Ref=Iq_Positive_Ref+tempTrans2;
/*
			tempTrans1=-cosf(Har_11_PH);	
			tempTrans2=sinf(Har_11_PH);						
			tempTrans1=tempTrans1*Har_11_RMS;
			tempTrans2=tempTrans2*Har_11_RMS;
			Id_Positive_Ref=Id_Positive_Ref+tempTrans1;
			Iq_Positive_Ref=Iq_Positive_Ref+tempTrans2;
*/									
		}break;
		case 2:  //APF Filter
		{	
			Id_Positive_Ref=Id_Positive_Ref+Iload_har_d;
			Iq_Positive_Ref=Iq_Positive_Ref+Iload_har_q;			
		}break;
		default:break;
	}
}

#ifdef SimHVAC
/****************************************************************************/
/*                                                                          */
/*                SVG Current Controller                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行旧 dq 电流环、谐振补偿及三相 SPWM 给定合成
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Current_Controller(void)
{
	float CurrentErrorTemp,CurrentOut1,CurrentOut2,CurrentOut3; /* CurrentErrorTemp：旧 dq 电流环给定减反馈的误差，单位 A。 */
	/* CurrentOut1：旧电流控制输出/第一相计算缓存。 */
	/* CurrentOut2：旧电流控制输出/第二相计算缓存。 */
	/* CurrentOut3：旧电流控制输出/第三相计算缓存。 */
	float tempspwm,tempspwm_min; /* tempspwm：旧三相 SPWM 给定计算的中间值。 */
	/* tempspwm_min：旧 SPWM 三相给定中的最小值，用于零序/三次谐波处理。 */
	
	if(PortRunFLAG.Bbits.PWMState_50Hz==1)
	{
		Id_Positive_Ref=Iin_d_Positive;		
		Iq_Positive_Ref=Iin_q_Positive;
		Variable_RST();
	}	
	//d
	CurrentErrorTemp=Id_Positive_Ref;
	
///////////////////////////////////////////////////////////
	CurrentErrorTemp=CurrentErrorTemp-Iin_d_Positive;
///////////////////////////////////////////////////////////

	Current_d_PID_Controller.PID_Controller.Num_1=Current_PID_Num_1;
	Current_d_PID_Controller.PID_Controller.Num_0=Current_PID_Num_0;	
	H_Filter_Memcopy(&Current_d_PID_Controller.PID_Controller_BUF[0], &PID_Control_buf.PID_Controller_BUF[0]);
	CurrentOut1=ConstantPID_Controller(CurrentErrorTemp);
	H_Filter_Memcopy(&PID_Control_buf.PID_Controller_BUF[0],&Current_d_PID_Controller.PID_Controller_BUF[0]);

	H_Filter_Memcopy(&I_d_6_Har_Controller.Har_Controller_BUF[0], &Har_6_Controller.Har_Controller_BUF[0]);
	CurrentOut2=SineHar_6_Controller(CurrentErrorTemp);
	H_Filter_Memcopy(&Har_6_Controller.Har_Controller_BUF[0],&I_d_6_Har_Controller.Har_Controller_BUF[0]);

	H_Filter_Memcopy(&I_d_12_Har_Controller.Har_Controller_BUF[0], &Har_12_Controller.Har_Controller_BUF[0]);
	CurrentOut3=SineHar_12_Controller(CurrentErrorTemp);
	H_Filter_Memcopy(&Har_12_Controller.Har_Controller_BUF[0],&I_d_12_Har_Controller.Har_Controller_BUF[0]);	
	
//Uout_d_Positive=CurrentOut1+CurrentOut2+CurrentOut3;
	Uout_d_Positive=CurrentOut1;
//	Uout_d_Positive=CurrentOut1+CurrentOut2;
/*
	CurrentErrorTemp=WL_decouple;
	CurrentErrorTemp=CurrentErrorTemp*Iin_q_Positive;
	Uout_d_Positive=Uout_d_Positive+CurrentErrorTemp;
*/	
	if(flag_Standard==1)
	{
		Uout_d_Positive=Uout_d_Positive*StandardValue;
	}


	//q
	CurrentErrorTemp=Iq_Positive_Ref;

///////////////////////////////////////////////////////////
	CurrentErrorTemp=CurrentErrorTemp-Iin_q_Positive;
///////////////////////////////////////////////////////////

	Current_q_PID_Controller.PID_Controller.Num_1=Current_PID_Num_1;
	Current_q_PID_Controller.PID_Controller.Num_0=Current_PID_Num_0;	
	H_Filter_Memcopy(&Current_q_PID_Controller.PID_Controller_BUF[0],&PID_Control_buf.PID_Controller_BUF[0]);
	CurrentOut1=ConstantPID_Controller(CurrentErrorTemp);
	H_Filter_Memcopy(&PID_Control_buf.PID_Controller_BUF[0],&Current_q_PID_Controller.PID_Controller_BUF[0]);

	H_Filter_Memcopy(&I_q_6_Har_Controller.Har_Controller_BUF[0], &Har_6_Controller.Har_Controller_BUF[0]);
	CurrentOut2=SineHar_6_Controller(CurrentErrorTemp);
	H_Filter_Memcopy(&Har_6_Controller.Har_Controller_BUF[0],&I_q_6_Har_Controller.Har_Controller_BUF[0]);

	H_Filter_Memcopy(&I_q_12_Har_Controller.Har_Controller_BUF[0], &Har_12_Controller.Har_Controller_BUF[0]);
	CurrentOut3=SineHar_12_Controller(CurrentErrorTemp);
	H_Filter_Memcopy(&Har_12_Controller.Har_Controller_BUF[0],&I_q_12_Har_Controller.Har_Controller_BUF[0]);	
	
//	Uout_q_Positive=CurrentOut1+CurrentOut2+CurrentOut3;
//	Uout_q_Positive=CurrentOut1+CurrentOut2;
	Uout_q_Positive=CurrentOut1;
/*
	CurrentErrorTemp=WL_decouple;
	CurrentErrorTemp=CurrentErrorTemp*Iin_d_Positive;
	Uout_q_Positive=Uout_q_Positive-CurrentErrorTemp;
*/	
	if(flag_Standard==1)
	{
		Uout_q_Positive=Uout_q_Positive*StandardValue;
	}

	Park_InverseTransformation(Uout_d_Positive,Uout_q_Positive,Cos_ph_Trans_Com_NEW,Sin_ph_Trans_Com_NEW);
	Clarke_InverseTransformation(Alpha_Trans,Beta_Trans); 
	CurrentOut1=A_Trans;
	CurrentOut2=B_Trans;
	CurrentOut3=C_Trans;

	if(flag_INV==1)
	{
		CurrentOut1=0.0;
		CurrentOut2=0.0;
		CurrentOut3=0.0;		
	}
	
	Park_InverseTransformation(Ugrid_d_Positive,Ugrid_q_Positive,Cos_ph_Trans_Com_NEW,Sin_ph_Trans_Com_NEW);
	Clarke_InverseTransformation(Alpha_Trans,Beta_Trans);
	A_Trans=A_Trans+CurrentOut1;
	B_Trans=B_Trans+CurrentOut2;
	C_Trans=C_Trans+CurrentOut3;

	//A_Trans=CurrentOut1+VA_Control;
	//B_Trans=CurrentOut2+VB_Control;
	//C_Trans=CurrentOut3+VC_Control;

	if(A_Trans>Uout_q_Positive_Max)
	{
		A_Trans=Uout_q_Positive_Max;
	}
	if(A_Trans<Uout_q_Positive_Min)
	{
		A_Trans=Uout_q_Positive_Min;
	}		

	if(B_Trans>Uout_q_Positive_Max)
	{
		B_Trans=Uout_q_Positive_Max;
	}
	if(B_Trans<Uout_q_Positive_Min)
	{
		B_Trans=Uout_q_Positive_Min;
	}	

	if(C_Trans>Uout_q_Positive_Max)
	{
		C_Trans=Uout_q_Positive_Max;
	}
	if(C_Trans<Uout_q_Positive_Min)
	{
		C_Trans=Uout_q_Positive_Min;
	}	

	if((Add_3th==1)&&(Method_3th==2))
	{
		TEATabc[3]=TEATabc[3]*Ugrid_Max;

		A_Trans=A_Trans+TEATabc[3];
		B_Trans=B_Trans+TEATabc[3];
		C_Trans=C_Trans+TEATabc[3]; 
	} 


	if((Add_3th==1)&&(Method_3th==1))
	{
		CurrentOut1=A_Trans;
		CurrentOut2=B_Trans;
		CurrentOut3=C_Trans;
		Clarke_Transformation(CurrentOut1, CurrentOut2, CurrentOut3);
	
		CurrentOut1=Alpha_Trans;
		CurrentOut1=CurrentOut1*Alpha_Trans;

		CurrentOut2=Beta_Trans;
		CurrentOut2=CurrentOut2*Beta_Trans;
	
		CurrentOut3=CurrentOut1+CurrentOut2;
		CurrentErrorTemp=CurrentOut2*3.0-CurrentOut1;
	
		CurrentErrorTemp=CurrentErrorTemp*Alpha_Trans;
		CurrentErrorTemp=CurrentErrorTemp/CurrentOut3;
		CurrentErrorTemp=CurrentErrorTemp/6.0;
		
		A_Trans=A_Trans+CurrentErrorTemp;
		B_Trans=B_Trans+CurrentErrorTemp;
		C_Trans=C_Trans+CurrentErrorTemp; 
	}

	if((Add_3th==1)&&(Method_3th==3))
	{
		if(A_Trans>B_Trans)
		{
			tempspwm=A_Trans;
			tempspwm_min=B_Trans;
		}
		else
		{
			tempspwm=B_Trans;
			tempspwm_min=A_Trans;		
		}
	
		if(tempspwm<C_Trans)
		{
			tempspwm=C_Trans;
		}
		if(tempspwm_min>C_Trans)
		{
			tempspwm_min=C_Trans;
		}
		tempspwm=(tempspwm+tempspwm_min)*0.5;

		A_Trans=A_Trans-tempspwm;
		B_Trans=B_Trans-tempspwm;
		C_Trans=C_Trans-tempspwm;
	}

	CurrentOut1=NHA_module_A;
	Uout_A_Positive=A_Trans/CurrentOut1;
	CurrentOut1=NHA_module_B;
	Uout_B_Positive=B_Trans/CurrentOut1;
	CurrentOut1=NHA_module_C;
	Uout_C_Positive=C_Trans/CurrentOut1;

}

#else

#endif

/****************************************************************************/
/*                                                                          */
/*                    SVG Controller                                        */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧 SVG 控制链顺序执行分解、功率、参考和电流控制
 * 输入参数: StatcomIndex：旧 STATCOM 无功、功率因数或电压模式选择
 *           ApfIndex：旧 APF 谐波补偿模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
void SVG_Controller(uchar StatcomIndex,uchar ApfIndex)
{
//	float INVtemp;
	P_Q_Calculator();   			
	
	SVG_Total_Udc_Controller();
	
	SVG_Q_Reference(StatcomIndex);
	SVG_I_Reference(ApfIndex);
	
	SVG_Current_Controller();

	HA_Module_RefVolt_A=Uout_A_Positive;
	HA_Module_RefVolt_B=Uout_B_Positive;
	HA_Module_RefVolt_C=Uout_C_Positive;	
}

#endif

