
//###########################################################################
//
// FILE:   CommonProgram.c
//
// FUNCTION:  1)Transform 
//            2)
//            3)
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

#include "main.h"

/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 将三相瞬时量变换为静止坐标系 α、β 分量，保持原幅值不变系数
 * 输入参数: VIa：A 相瞬时输入，与调用者电压/电流量同单位
 *           VIb：B 相瞬时输入，与调用者电压/电流量同单位
 *           VIc：C 相瞬时输入，与调用者电压/电流量同单位
 * 输出参数: Alpha_Trans、Beta_Trans，单位与三相输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Clarke_Transformation(float VIa, float VIb, float VIc);
/*==================================================================
 * 函数功能: 将 α、β 分量逆变换为三相瞬时量
 * 输入参数: VIalpha：α 轴输入，与调用者电压/电流量同单位
 *           VIbeta：β 轴输入，与调用者电压/电流量同单位
 * 输出参数: A_Trans、B_Trans、C_Trans，单位与 αβ 输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Clarke_InverseTransformation(float VIalpha, float VIbeta);
/*==================================================================
 * 函数功能: 按输入角度的正余弦将 α、β 分量变换为 d、q 分量
 * 输入参数: VIalpha：α 轴输入，与调用者电压/电流量同单位
 *           VIbeta：β 轴输入，与调用者电压/电流量同单位
 *           Cos_ph：变换相角余弦值
 *           Sin_ph：变换相角正弦值
 * 输出参数: D_Trans、Q_Trans，单位与 αβ 输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Park_Transformation(float VIalpha, float VIbeta, float Cos_ph, float Sin_ph);
/*==================================================================
 * 函数功能: 按输入角度的正余弦将 d、q 分量逆变换为 α、β 分量
 * 输入参数: VId：d 轴输入，与调用者电压/电流量同单位
 *           VIq：q 轴输入，与调用者电压/电流量同单位
 *           Cos_ph：变换相角余弦值
 *           Sin_ph：变换相角正弦值
 * 输出参数: Alpha_Trans、Beta_Trans，单位与 dq 输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Park_InverseTransformation(float VId, float VIq, float Cos_ph, float Sin_ph);

/*==================================================================
 * 函数功能: 复制滤波器的四个历史状态浮点数
 * 输入参数: SourceAddr：源地址，连续数据至少覆盖要求的复制长度
 *           DestAddr：输出目标地址，调用者须提供足够容量
 * 输出参数: DestAddr 指向的连续四个 float，调用者保证源/目标容量。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void H_Filter_Memcopy(float *SourceAddr, float *DestAddr);
/*==================================================================
 * 函数功能: 执行原增量 PI 差分式并更新 PID_Control_buf 的上一拍输入、输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: 更新 PID_Control_buf 中的 IN_n_1、OUT_n_1。
 * 返回值: 本拍控制输出，与 ErrorIn 的量纲及控制器系数对应
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
float ConstantPID_Controller(float ErrorIn);

/****************************************************************************/
/*                                                                          */
/*              Clarke transformation                                       */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将三相瞬时量变换为静止坐标系 α、β 分量，保持原幅值不变系数
 * 输入参数: VIa：A 相瞬时输入，与调用者电压/电流量同单位
 *           VIb：B 相瞬时输入，与调用者电压/电流量同单位
 *           VIc：C 相瞬时输入，与调用者电压/电流量同单位
 * 输出参数: Alpha_Trans、Beta_Trans，单位与三相输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Clarke_Transformation(float VIa, float VIb, float VIc) 
{
	float TempA,TempB; /* TempA：α 轴差值、缩放计算缓存。 */
	/* TempB：β 轴相间差值及除 √3 的计算缓存。 */

	TempA=VIa;
	TempA=TempA*2.0;
	TempA=TempA-VIb;
	TempA=TempA-VIc;
	TempA=TempA/3.0;

	TempB=VIb;
	TempB=TempB-VIc;
	TempB=TempB/1.732;
	
	Alpha_Trans=TempA;
	Beta_Trans=TempB;
}

/****************************************************************************/
/*                                                                          */
/*              Clarke inverse transformation                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 α、β 分量逆变换为三相瞬时量
 * 输入参数: VIalpha：α 轴输入，与调用者电压/电流量同单位
 *           VIbeta：β 轴输入，与调用者电压/电流量同单位
 * 输出参数: A_Trans、B_Trans、C_Trans，单位与 αβ 输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Clarke_InverseTransformation(float VIalpha, float VIbeta) 
{
	float Tempa,Tempb,Tempc; /* Tempa：A 相逆变换计算缓存。 */
	/* Tempb：B 相逆变换计算缓存。 */
	/* Tempc：C 相逆变换计算缓存。 */

	Tempa=VIalpha;

	Tempb=VIbeta;
	Tempb=Tempb*1.732;
	Tempb=Tempb-VIalpha;
	Tempb=Tempb/2.0;

	Tempc=VIbeta;
	Tempc=-1.732*Tempc;
	Tempc=Tempc-VIalpha;
	Tempc=Tempc/2.0;

	A_Trans=Tempa;
	B_Trans=Tempb;
	C_Trans=Tempc;
}

/****************************************************************************/
/*                                                                          */
/*              Park transformation                                         */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按输入角度的正余弦将 α、β 分量变换为 d、q 分量
 * 输入参数: VIalpha：α 轴输入，与调用者电压/电流量同单位
 *           VIbeta：β 轴输入，与调用者电压/电流量同单位
 *           Cos_ph：变换相角余弦值
 *           Sin_ph：变换相角正弦值
 * 输出参数: D_Trans、Q_Trans，单位与 αβ 输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Park_Transformation(float VIalpha, float VIbeta, float Cos_ph, float Sin_ph) 
{
	float Temp1,Temp2,Temp3,Temp4; /* Temp1：第一轴分量乘 cos 的乘积，用于本拍坐标变换。 */
	/* Temp2：第二轴分量乘 sin 的乘积，用于本拍坐标变换。 */
	/* Temp3：第二轴分量乘 cos 的乘积，用于本拍坐标变换。 */
	/* Temp4：第一轴分量乘 sin 的乘积，用于本拍坐标变换。 */

	Temp1=VIalpha;
	Temp1=Temp1*Cos_ph;
	
	Temp2=VIbeta;
	Temp2=Temp2*Sin_ph;
	
	Temp3=VIbeta;
	Temp3=Temp3*Cos_ph;
	
	Temp4=VIalpha;
	Temp4=Temp4*Sin_ph;

	D_Trans=Temp1+Temp2;
	Q_Trans=Temp3-Temp4;
}

/****************************************************************************/
/*                                                                          */
/*              Park inverse transformation                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按输入角度的正余弦将 d、q 分量逆变换为 α、β 分量
 * 输入参数: VId：d 轴输入，与调用者电压/电流量同单位
 *           VIq：q 轴输入，与调用者电压/电流量同单位
 *           Cos_ph：变换相角余弦值
 *           Sin_ph：变换相角正弦值
 * 输出参数: Alpha_Trans、Beta_Trans，单位与 dq 输入相同。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void Park_InverseTransformation(float VId, float VIq, float Cos_ph, float Sin_ph) 
{
	float Temp1,Temp2,Temp3,Temp4; /* Temp1：第一轴分量乘 cos 的乘积，用于本拍坐标变换。 */
	/* Temp2：第二轴分量乘 sin 的乘积，用于本拍坐标变换。 */
	/* Temp3：第二轴分量乘 cos 的乘积，用于本拍坐标变换。 */
	/* Temp4：第一轴分量乘 sin 的乘积，用于本拍坐标变换。 */

	Temp1=VId;
	Temp1=Temp1*Cos_ph;
	
	Temp2=VIq;
	Temp2=Temp2*Sin_ph;
	
	Temp3=VIq;
	Temp3=Temp3*Cos_ph;
	
	Temp4=VId;
	Temp4=Temp4*Sin_ph;

	Alpha_Trans=Temp1-Temp2;
	Beta_Trans=Temp3+Temp4;
}

/****************************************************************************/
/*                                                                          */
/*                       PID Controller                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原增量 PI 差分式并更新 PID_Control_buf 的上一拍输入、输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: 更新 PID_Control_buf 中的 IN_n_1、OUT_n_1。
 * 返回值: 本拍控制输出，与 ErrorIn 的量纲及控制器系数对应
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
float ConstantPID_Controller(float ErrorIn)
{
	float tempjs_0,tempjs_1,tempjs_2,tempjs_3; /* tempjs_0：当前参与乘法的 PI 差分系数。 */
	/* tempjs_1：本拍误差与 Num_1 的乘积。 */
	/* tempjs_2：上一拍误差与 Num_0 的乘积。 */
	/* tempjs_3：上一拍输出加两个误差项得到的本拍 PI 输出。 */

	tempjs_0=PID_Control_buf.PID_Controller.Num_1;
	tempjs_1=ErrorIn;
	tempjs_1=tempjs_1*tempjs_0;

	tempjs_0=PID_Control_buf.PID_Controller.Num_0;
	tempjs_2=PID_Control_buf.PID_Controller.IN_n_1;
	tempjs_2=tempjs_2*tempjs_0;	

	tempjs_3=PID_Control_buf.PID_Controller.OUT_n_1;
	tempjs_3=tempjs_3+tempjs_1+tempjs_2;	
	
	PID_Control_buf.PID_Controller.OUT_n_1=tempjs_3;
	PID_Control_buf.PID_Controller.IN_n_1=ErrorIn;
	
	return(tempjs_3);	
}

/****************************************************************************/
/*                                                                          */
/*              H Filter memcopy                                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 复制滤波器的四个历史状态浮点数
 * 输入参数: SourceAddr：源地址，连续数据至少覆盖要求的复制长度
 *           DestAddr：输出目标地址，调用者须提供足够容量
 * 输出参数: DestAddr 指向的连续四个 float，调用者保证源/目标容量。
 * 返回值: 无
 * 执行说明: 控制计算路径；沿用原运算顺序。
 *================================================================*/
void H_Filter_Memcopy(float *SourceAddr, float *DestAddr)
{
	ushort i; /* i：本次数组、参数或模块遍历的索引。 */
   for(i=0; i <4; i++)
   { 
      *DestAddr++ = *SourceAddr++;
   }	
}

