/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef HVAC_STRATEGY_H_
#define HVAC_STRATEGY_H_

#include "SstTypes.h"
#ifdef __cplusplus
extern "C"
{
#endif
//###########################################################################
//
// FILE:   HVAC_Strategy.h
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

/**************************Macro definition*********************************/
//Switch frequency----20KHz
/*
#define H2_100_Filter_Num_1  -1.999013066
#define H2_100_Filter_Den_1  -1.986311316
#define H2_100_Filter_Den_0  0.9872918725
#define H2_100_Filter_Gain   0.9936459064

#define H6_100_Filter_Num_1  -1.991123915
#define H6_100_Filter_Den_1  -1.978472233
#define H6_100_Filter_Den_0  0.9872918725
#define H6_100_Filter_Gain   0.9936459064

#define H12_100_Filter_Num_1  -1.964574456
#define H12_100_Filter_Den_1  -1.952091455
#define H12_100_Filter_Den_0  0.9872918725
#define H12_100_Filter_Gain   0.9936459064

#define H2_20_Filter_Num_1  -1.999013066
#define H2_20_Filter_Den_1  -1.995822549
#define H2_20_Filter_Den_0  0.9968079329
#define H2_20_Filter_Gain   0.9984039664

#define Har_6_Controller_Num_2  -0.6
#define Har_6_Controller_Num_1  1.2
#define Har_6_Controller_Num_0  -0.5997
#define Har_6_Controller_Den_1  -1.991

#define Har_12_Controller_Num_2  -0.8
#define Har_12_Controller_Num_1  1.6
#define Har_12_Controller_Num_0  -0.7996
#define Har_12_Controller_Den_1  -1.965

#define Current_PID_Num_1  -60.02
#define Current_PID_Num_0  59.98

#define Volt_PID_Num_1  0.8013
#define Volt_PID_Num_0  -0.7987

#define PLL_PID_Num_1  -100.0
#define PLL_PID_Num_0  99.98
*/

//Switch frequency----10KHz
/* H2_100_Filter_Num_1：H2/100 原二阶滤波器分子第 1 项差分系数。 */
#define H2_100_Filter_Num_1 -1.996053457
/* H2_100_Filter_Den_1：H2/100 原二阶滤波器分母第 1 项差分系数。 */
#define H2_100_Filter_Den_1 -1.970843554
/* H2_100_Filter_Den_0：H2/100 原二阶滤波器分母第 0 项差分系数。 */
#define H2_100_Filter_Den_0 0.9747402668
/* H2_100_Filter_Gain：原 H2_100_Filter 滤波输出增益。 */
#define H2_100_Filter_Gain 0.9873701334

/* H6_100_Filter_Num_1：H6/100 原二阶滤波器分子第 1 项差分系数。 */
#define H6_100_Filter_Num_1 -1.964574456
/* H6_100_Filter_Den_1：H6/100 原二阶滤波器分母第 1 项差分系数。 */
#define H6_100_Filter_Den_1 -1.939762235
/* H6_100_Filter_Den_0：H6/100 原二阶滤波器分母第 0 项差分系数。 */
#define H6_100_Filter_Den_0 0.9747402668
/* H6_100_Filter_Gain：原 H6_100_Filter 滤波输出增益。 */
#define H6_100_Filter_Gain 0.9873701334

/* H12_100_Filter_Num_1：H12/100 原二阶滤波器分子第 1 项差分系数。 */
#define H12_100_Filter_Num_1 -1.859552979
/* H12_100_Filter_Den_1：H12/100 原二阶滤波器分母第 1 项差分系数。 */
#define H12_100_Filter_Den_1 -1.83606708
/* H12_100_Filter_Den_0：H12/100 原二阶滤波器分母第 0 项差分系数。 */
#define H12_100_Filter_Den_0 0.9747402668
/* H12_100_Filter_Gain：原 H12_100_Filter 滤波输出增益。 */
#define H12_100_Filter_Gain 0.9873701334

/* H2_20_Filter_Num_1：H2/20 原二阶滤波器分子第 1 项差分系数。 */
#define H2_20_Filter_Num_1 -1.996053457
/* H2_20_Filter_Den_1：H2/20 原二阶滤波器分母第 1 项差分系数。 */
#define H2_20_Filter_Den_1 -1.989691973
/* H2_20_Filter_Den_0：H2/20 原二阶滤波器分母第 0 项差分系数。 */
#define H2_20_Filter_Den_0 0.9936259389
/* H2_20_Filter_Gain：原 H2_20_Filter 滤波输出增益。 */
#define H2_20_Filter_Gain 0.9968129396

/* flux_k1：原磁通估算/滤波差分系数 k1。 */
#define flux_k1 0.3333 //*
/* flux_k2：原磁通估算/滤波差分系数 k2。 */
#define flux_k2 1.6667 //+
    /*
//380V
#define PLL_PID_Num_1  	-2.001 //-10.00  //-5.003  //-3.002
#define PLL_PID_Num_0  	1.999//9.996 //4.998   //2.998

//voltage loop
#define Volt_PID_Num_1  0.805    //0.8025/2.0         //2.0,1.0
#define Volt_PID_Num_0  -0.795   //-0.7975/2.0

#define PH_Volt_PID_Num_1  0.201
#define PH_Volt_PID_Num_0  -0.199

//Current loop
#define Har_2_Controller_Num_2  -0.05/10.0        //-0.4/4*0.5
#define Har_2_Controller_Num_1  0.0999/10.0       //0.7996/4*0.5
#define Har_2_Controller_Num_0  -0.05/10.0				//-0.3996/4*0.5
#define Har_2_Controller_Den_1  -1.996

#define Har_6_Controller_Num_2  -0.05          		//-0.6/6*0.5
#define Har_6_Controller_Num_1  0.0999        		//1.199/6*0.5
#define Har_6_Controller_Num_0  -0.05       			//-0.5994/6*0.5
#define Har_6_Controller_Den_1  -1.965

#define Har_12_Controller_Num_2  -0.05/10.0				//-0.8/8*0.5
#define Har_12_Controller_Num_1  0.0999/10.0     	//1.599/8*0.5
#define Har_12_Controller_Num_0  -0.05/10.0   	  //-0.7992/8*0.5
#define Har_12_Controller_Den_1  -1.86

#define Current_PID_Num_1  	-15.01/3.0 	          //-30.02*0.5
#define Current_PID_Num_0   14.99/3.0             //29.98*0.5      //3.0   6.0 success

#define Neg_Current_PID_Num_1  	-1.501 	      		//-3.002*0.5
#define Neg_Current_PID_Num_0   1.499         		//2.998*0.5
*/

    //Current loop
    //#define Har_2_Controller_Num_2  -0.1          //-0.4/4
    //#define Har_2_Controller_Num_1  0.1999        //0.7996/4
    //#define Har_2_Controller_Num_0  -0.0999				//-0.3996/4
    //#define Har_2_Controller_Den_1  -1.996

    //#define Har_6_Controller_Num_2  -0.1          //-0.6/6
    //#define Har_6_Controller_Num_1  0.1998        //1.199/6
    //#define Har_6_Controller_Num_0  -0.0999       //-0.5994/6
    //#define Har_6_Controller_Den_1  -1.965

    //#define Har_12_Controller_Num_2  -0.1					//-0.8/8
    //#define Har_12_Controller_Num_1  0.1999     	//1.599/8
    //#define Har_12_Controller_Num_0  -0.0999   	  //-0.7992/8
    //#define Har_12_Controller_Den_1  -1.86

    //#define Current_PID_Num_1  	-30.02 	          //-30.02
    //#define Current_PID_Num_0   29.98             //29.98

    //#define Neg_Current_PID_Num_1  	-3.002 	      //-3.002
    //#define Neg_Current_PID_Num_0   2.998         //2.998

    /***********************Function declaration*********************************/
    //#ifdef HVAC_online

    /*==================================================================
 * 函数功能: 执行原 H2/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
    extern float BandH2_100_Filter(float FilterIN);
    /*==================================================================
 * 函数功能: 执行原 H6/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
    extern float BandH6_100_Filter(float FilterIN);
    /*==================================================================
 * 函数功能: 执行原 H12/100 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
    extern float BandH12_100_Filter(float FilterIN);
    /*==================================================================
 * 函数功能: 执行原 H2/20 二阶滤波差分式，更新对应四个历史状态
 * 输入参数: FilterIN：本拍滤波输入，与输出同单位
 * 输出参数: 对应 H_Filter 全局缓冲区。
 * 返回值: 当前滤波输出
 * 执行说明: 原 HVAC 计算子函数，系数与运算顺序保持原代码。
 *================================================================*/
    extern float BandH2_20_Filter(float FilterIN);
    /*==================================================================
 * 函数功能: 执行原 2 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_2_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
    extern float SineHar_2_Controller(float ErrorIn);
    /*==================================================================
 * 函数功能: 执行原 6 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_6_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
    extern float SineHar_6_Controller(float ErrorIn);
    /*==================================================================
 * 函数功能: 执行原 12 次谐波谐振控制差分式并更新历史输入输出
 * 输入参数: ErrorIn：本拍控制误差，与被控制物理量同单位
 * 输出参数: Har_12_Controller 历史状态。
 * 返回值: 当前谐振控制输出
 * 执行说明: 原 HVAC 控制子函数，谐振系数保持原代码。
 *================================================================*/
    extern float SineHar_12_Controller(float ErrorIn);

    /*==================================================================
 * 函数功能: 对电网电压作坐标变换及序分量滤波，推进原锁相环相角
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void U_Decomposition_PLL(void);
    /*==================================================================
 * 函数功能: 对端口电流作 dq 变换并分离正序、负序及谐波分量
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void Iin_Decomposition(void);
    /*==================================================================
 * 函数功能: 对负载电流作 dq 变换并提取基波与谐波分量
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void Iload_Decomposition(void);
    /*==================================================================
 * 函数功能: 根据电网电压、电流 dq 分量计算并滤波有功、无功功率
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void P_Q_Calculator(void);
    /*==================================================================
 * 函数功能: 按旧 STATCOM 工作模式生成无功功率目标
 * 输入参数: StatcomIndex：旧 STATCOM 无功、功率因数或电压模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void SVG_Q_Reference(uchar StatcomIndex);
    /*==================================================================
 * 函数功能: 按各相在线模块数计算相平均及三相平均直流电压
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void SVG_Udc_Average(void);
    /*==================================================================
 * 函数功能: 按旧直流电压参考执行增量 PI，生成 d 轴电流参考
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void SVG_Total_Udc_Controller(void);
    /*==================================================================
 * 函数功能: 按旧 STATCOM/APF 模式合成正序、负序及谐波电流参考
 * 输入参数: ApfIndex：旧 APF 谐波补偿模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void SVG_I_Reference(uchar ApfIndex);
    /*==================================================================
 * 函数功能: 执行旧 dq 电流环、谐振补偿及三相 SPWM 给定合成
 * 输入参数: 无
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void SVG_Current_Controller(void);
    /*==================================================================
 * 函数功能: 按旧 SVG 控制链顺序执行分解、功率、参考和电流控制
 * 输入参数: StatcomIndex：旧 STATCOM 无功、功率因数或电压模式选择
 *           ApfIndex：旧 APF 谐波补偿模式选择
 * 输出参数: 旧控制器历史缓冲及 Ugrid/Iin/Iload/SVG/HA 相关全局计算量。
 * 返回值: 无
 * 执行说明: 旧 HVAC 控制兼容代码；保留原运算和历史状态，实际调用关系以现有控制入口为准。
 *================================================================*/
    extern void SVG_Controller(uchar StatcomIndex, uchar ApfIndex);

    //#endif

#ifdef __cplusplus
}
#endif
#endif
