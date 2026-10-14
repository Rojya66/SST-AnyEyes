#include "SstPortStatus.h"
#include <math.h>
#include "PwmGate.h"
#include "ControlData.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "SstTypes.h"
//###########################################################################
//
// FILE:   ADC_Sample.c
//
// FUNCTION:  1)Sample
//            2)calculation for display
//            3)Voltage ordering
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################
#include "ADC_Sample.h"
#include "hw_types.h" 
#include "FaultCheck.h"
#include "SstBenchInputs.h"
/************************Variable definition********************************/
short Port_AD_Buf[16]; /* Port_AD_Buf：原端口 FPGA AD 采样字缓存。 */  

/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 读取指定模块的 FPGA AD 数据并按所选量转换为工程值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           MK_index：插件内模块编号，从 1 开始，模块数组下标为编号减 1
 *           CS_index：ModuleIndex_* 采样类型编号
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 直流电压 V、电流 A、温度按原计数/10；非法插件、模块或类型返回 65536.0
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
float Module_AD_Sample(uchar CJ_index,uchar MK_index,uchar CS_index,uchar latch);
/*==================================================================
 * 函数功能: 采集当前端口 FPGA 的瞬时电压、电流及直流量，并推进原滤波/噪声处理
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_AD_Sample(uchar latch);

/*==================================================================
 * 函数功能: 读取模块直流采样汇总并更新各相总电压量
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Module_TotalUdc_Sample(void);

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 换算 HVAC 插件采样数据并更新端口实时测量缓存
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void HVAC_RealTimeParameter(void);
#endif
/****************************************************************************/
/*                                                                          */
/*              Module Sample                                               */
/*                                                                          */
/****************************************************************************/
//CJ_index；Plug-in unit index；MK_index：Module index；CS_index：Parameter index；latch--1：latch
/*==================================================================
 * 函数功能: 读取指定模块的 FPGA AD 数据并按所选量转换为工程值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           MK_index：插件内模块编号，从 1 开始，模块数组下标为编号减 1
 *           CS_index：ModuleIndex_* 采样类型编号
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 直流电压 V、电流 A、温度按原计数/10；非法插件、模块或类型返回 65536.0
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
float Module_AD_Sample(uchar CJ_index,uchar MK_index,uchar CS_index,uchar latch)
{	
	short SampleBuf_short; /* SampleBuf_short：从 FPGA 读取的有符号原始采样字。 */
	float ADSample_Result; /* ADSample_Result：模块原始 AD 值换算后的物理量。 */
	
	if((CJ_index<2)||(MK_index<1))
	{
		ADSample_Result=65536.0;
		return ADSample_Result;
	}
	
	switch(CS_index)
	{
		case ModuleIndex_DCbus_Volt:
		{
			if(latch==0)
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].DC[MK_index-1];  		
			}
			else
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].DC_LATCH[MK_index-1];				
			}
			
			ADSample_Result=(float)SampleBuf_short;					
			
		}break;
		case ModuleIndex_DCbus_Current:
		{
			if(latch==0)
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].IR[MK_index-1];
			}
			else
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].IR_LATCH[MK_index-1];
			}
			
//			AD_CalTemp=(float)SampleBuf_short;
//			AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
//			ADSample_Result=AD_CalTemp;
//			AD_CalTemp=Module_DCbusCurrent_Ratio;
//			ADSample_Result=ADSample_Result*AD_CalTemp;			

			ADSample_Result=(float)SampleBuf_short;		
			ADSample_Result=ADSample_Result/10.0;
			
		}break;		
		case ModuleIndex_Port_Current:
		{
			if(latch==0)
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].IG[MK_index-1];  		
			}
			else
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].IG_LATCH[MK_index-1];				
			}
			
			ADSample_Result=(float)SampleBuf_short;		
			ADSample_Result=ADSample_Result/10.0;	
		}break;		
		case ModuleIndex_Temp:
		{
			if(latch==0)
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].TEMP[MK_index-1];  		
			}
			else
			{
				SampleBuf_short=FPGA_Interface[CJ_index-2].TEMP_LATCH[MK_index-1];				
			}
			
			ADSample_Result=(float)SampleBuf_short;
			ADSample_Result=ADSample_Result/10.0;
	
		}break;					
		default:
		{
			ADSample_Result=65536.0;
		}break;
	}
	return ADSample_Result;
}


#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*              HVAC Realtime Parameter                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 换算 HVAC 插件采样数据并更新端口实时测量缓存
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void HVAC_RealTimeParameter(void)
{	
//unsigned char i;
	float AD_CalTemp,AD_CalTemp1; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
	/* AD_CalTemp1：采样/功率换算的第二个浮点中间值。 */
	float ADSample_buffer[3]; /* ADSample_buffer：本轮读取的 FPGA 原始采样字缓存。 */
	
	if(HVAC_RealParameter_N<200)
	{
		ADSample_buffer[0]=HA_Port_Volt_AB;
		ADSample_buffer[1]=HA_Port_Volt_BC;
		ADSample_buffer[2]=HA_Port_Volt_CA;		
		HVAC_RealVoltage_Sum[0]=HVAC_RealVoltage_Sum[0]+ADSample_buffer[0]*ADSample_buffer[0];
		HVAC_RealVoltage_Sum[1]=HVAC_RealVoltage_Sum[1]+ADSample_buffer[1]*ADSample_buffer[1];
		HVAC_RealVoltage_Sum[2]=HVAC_RealVoltage_Sum[2]+ADSample_buffer[2]*ADSample_buffer[2];
		
		ADSample_buffer[0]=HA_Port_Current_A;
		ADSample_buffer[1]=HA_Port_Current_B;
		ADSample_buffer[2]=HA_Port_Current_C;		
		HVAC_RealCurrent_Sum[0]=HVAC_RealCurrent_Sum[0]+ADSample_buffer[0]*ADSample_buffer[0];
		HVAC_RealCurrent_Sum[1]=HVAC_RealCurrent_Sum[1]+ADSample_buffer[1]*ADSample_buffer[1];
		HVAC_RealCurrent_Sum[2]=HVAC_RealCurrent_Sum[2]+ADSample_buffer[2]*ADSample_buffer[2];		
		
		ADSample_buffer[0]=HA_Grid_Current_A;
		ADSample_buffer[1]=HA_Grid_Current_B;
		ADSample_buffer[2]=HA_Grid_Current_C;
		HVAC_RealGridCurrent_Sum[0]=HVAC_RealGridCurrent_Sum[0]+ADSample_buffer[0]*ADSample_buffer[0];
		HVAC_RealGridCurrent_Sum[1]=HVAC_RealGridCurrent_Sum[1]+ADSample_buffer[1]*ADSample_buffer[1];
		HVAC_RealGridCurrent_Sum[2]=HVAC_RealGridCurrent_Sum[2]+ADSample_buffer[2]*ADSample_buffer[2];	
					
		HVAC_RealParameter_N++;
	}	

	//power and pf
		AD_CalTemp=HA_Port_Volt_A;
		AD_CalTemp=AD_CalTemp*HA_Port_Current_A;
		AD_CalTemp1=HA_Port_Volt_B;
		AD_CalTemp1=AD_CalTemp1*HA_Port_Current_B;
		AD_CalTemp+=AD_CalTemp1;
	
		AD_CalTemp1=HA_Port_Volt_C;
		AD_CalTemp1=AD_CalTemp1*HA_Port_Current_C;
		AD_CalTemp+=AD_CalTemp1;
		ADSample_buffer[0]=AD_CalTemp;
	
		AD_CalTemp=HA_Port_Volt_AB;
		AD_CalTemp=AD_CalTemp*HA_Port_Current_C;
		AD_CalTemp1=HA_Port_Volt_BC;
		AD_CalTemp1=AD_CalTemp1*HA_Port_Current_A;
		AD_CalTemp+=AD_CalTemp1;
	
		AD_CalTemp1=HA_Port_Volt_CA;
		AD_CalTemp1=AD_CalTemp1*HA_Port_Current_B;
		AD_CalTemp+=AD_CalTemp1;
		ADSample_buffer[1]=AD_CalTemp;
	
		AD_CalTemp=HA_PortP_LF_1*Sample_UP_20kHz+HA_PortP_LF_2*Sample_DOWN_20kHz;
		HA_PortP_LF_1=ADSample_buffer[0];
		HA_PortP_LF_2=AD_CalTemp;	
		PORT_RealTime_DATA.PORT_P=AD_CalTemp/1000.0;	
	
		AD_CalTemp=HA_PortQ_LF_1*Sample_UP_20kHz+HA_PortQ_LF_2*Sample_DOWN_20kHz;
		HA_PortQ_LF_1=ADSample_buffer[1];
		HA_PortQ_LF_2=AD_CalTemp;	
		PORT_RealTime_DATA.PORT_Q=AD_CalTemp/1732.0;		
}
/****************************************************************************/
/*                                                                          */
/*                Port Sample                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 采集当前端口 FPGA 的瞬时电压、电流及直流量，并推进原滤波/噪声处理
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_AD_Sample(uchar latch)
{	
	unsigned char i; /* i：本次数组、参数或模块遍历的索引。 */
	float AD_CalTemp,LPF_temp0,LPF_temp1; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
	/* LPF_temp0：原低通滤波器的前一项/输入项计算缓存。 */
	/* LPF_temp1：原低通滤波器的本拍输出项计算缓存。 */
	float ADSample_buffer[4]; /* ADSample_buffer：本轮读取的 FPGA 原始采样字缓存。 */

	float HA_Port_Volt_A_Temp; /* HA_Port_Volt_A_Temp：高压交流端口 A 相/相间原采样电压，单位 V。 */      
	float HA_Port_Volt_B_Temp; /* HA_Port_Volt_B_Temp：高压交流端口 B 相/相间原采样电压，单位 V。 */      
	float HA_Port_Volt_C_Temp; /* HA_Port_Volt_C_Temp：高压交流端口 C 相/相间原采样电压，单位 V。 */      

	float HA_Port_Current_A_Temp; /* HA_Port_Current_A_Temp：高压交流端口 A 相/相间端口电流，单位 A。 */
	float HA_Port_Current_B_Temp; /* HA_Port_Current_B_Temp：高压交流端口 B 相/相间端口电流，单位 A。 */
#if SST_BENCH_INPUT_SIM
    if(latch == 0U)
    {
        SstBenchInputs_FastStep(); /* 常规采样注入虚拟量；锁存采样接口仍保留原硬件语义。 */
        return;
    }
#endif
//	float HA_Port_Current_C_Temp;

//float HA_Grid_Current_A_Temp;
//float HA_Grid_Current_B_Temp;
//float HA_Grid_Current_C_Temp;

/*
//CH1,CH2 and CH3 for debug
  for(i=0;i<2;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[i];
  	}
  	else
  	{
  		Port_AD_Buf[i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[i];
  	}
  	
		AD_CalTemp=Port_AD_Buf[i];
		ADSample_buffer[i]=AD_CalTemp;	
	}
// 	HWREGH(0x60000624)=Port_AD_Buf[0];
// 	HWREGH(0x60000626)=Port_AD_Buf[1];
 	
	HA_Port_Volt_A_Temp=ADSample_buffer[0];
	ADSample_buffer[0]=0.0316196*HA_Port_Volt_A_Temp+1.43869;
	HA_Port_Volt_B_Temp=ADSample_buffer[1];
	ADSample_buffer[1]=0.0317048*HA_Port_Volt_B_Temp-0.0634;	
	
	ADSample_buffer[2]=-ADSample_buffer[0]-ADSample_buffer[1];

#ifdef SELECT_660V	
	ADSample_buffer[0]=ADSample_buffer[0]*1.75;
	ADSample_buffer[1]=ADSample_buffer[1]*1.75;
	ADSample_buffer[2]=ADSample_buffer[2]*1.75;
#else

#endif
	
	if(latch==0)
	{	
		HA_Port_Volt_AB=ADSample_buffer[0];      
		HA_Port_Volt_BC=ADSample_buffer[1];      
		HA_Port_Volt_CA=ADSample_buffer[2];		
	}
	else
	{
		HA_Port_Volt_AB_LATCH=ADSample_buffer[0];      
		HA_Port_Volt_BC_LATCH=ADSample_buffer[1];      
		HA_Port_Volt_CA_LATCH=ADSample_buffer[2];		
	}	
			
	AD_CalTemp=ADSample_buffer[0];
	HA_Port_Volt_A_Temp=(AD_CalTemp*2.0+ADSample_buffer[1])/3.0;
	
	HA_Port_Volt_B_Temp=ADSample_buffer[1];
	HA_Port_Volt_B_Temp=(HA_Port_Volt_B_Temp-ADSample_buffer[0])/3.0;
	
	HA_Port_Volt_C_Temp=ADSample_buffer[1];
	HA_Port_Volt_C_Temp=(HA_Port_Volt_C_Temp*2.0+ADSample_buffer[0])/(-3.0);

	HA_Port_Volt_A=HA_Port_Volt_A_Temp;      
	HA_Port_Volt_B=HA_Port_Volt_B_Temp;      
	HA_Port_Volt_C=HA_Port_Volt_C_Temp;	
*/

//system current
  for(i=0;i<2;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[3+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[3+i];
  	}
  	else
  	{
  		Port_AD_Buf[3+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[3+i];
  	}
  	
		AD_CalTemp=Port_AD_Buf[3+i];
		ADSample_buffer[i]=AD_CalTemp;	
	}

	HA_Port_Current_A_Temp=ADSample_buffer[0];
	ADSample_buffer[0]=0.0121963*HA_Port_Current_A_Temp+0.09757;
	HA_Port_Current_B_Temp=ADSample_buffer[1];
	ADSample_buffer[1]=0.0121963*HA_Port_Current_B_Temp-0.3293;
	
	ADSample_buffer[0]=ADSample_buffer[0]*HA_CT_Ratio;
	ADSample_buffer[1]=ADSample_buffer[1]*HA_CT_Ratio;
			
	if(latch==0)
	{
		HA_Grid_Current_A=ADSample_buffer[0];
		HA_Grid_Current_B=ADSample_buffer[1];			
		HA_Grid_Current_C=-ADSample_buffer[0]-ADSample_buffer[1];
	}
	else
	{
		HA_Grid_Current_A_LATCH=ADSample_buffer[0];	
		HA_Grid_Current_B_LATCH=ADSample_buffer[1];		
		HA_Grid_Current_C_LATCH=-ADSample_buffer[0]-ADSample_buffer[1];					
	}

//port voltage
  for(i=0;i<2;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[6+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[6+i];
  	}
  	else
  	{
  		Port_AD_Buf[6+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[6+i];
  	}
  	
		AD_CalTemp=Port_AD_Buf[6+i];
		ADSample_buffer[i]=AD_CalTemp;	
	}
// 	HWREGH(0x60000628)=Port_AD_Buf[0];
// 	HWREGH(0x6000062a)=Port_AD_Buf[1];
 	 
	HA_Port_Volt_A_Temp=ADSample_buffer[0];
	ADSample_buffer[0]=0.0079785*HA_Port_Volt_A_Temp+0.15957;
	HA_Port_Volt_B_Temp=ADSample_buffer[1];
	ADSample_buffer[1]=0.0079678*HA_Port_Volt_B_Temp+0.545795;	
	
	ADSample_buffer[0]=ADSample_buffer[0]*HA_PT_Ratio;
	ADSample_buffer[1]=ADSample_buffer[1]*HA_PT_Ratio;
	
//////////////////////

	LPF_temp0=LPF_5KHZ_X_Volt_A_1*0.9567;
	LPF_temp1=LPF_5KHZ_Y_Volt_A_1*0.04328;
	LPF_temp1=LPF_temp1+LPF_temp0;
	LPF_5KHZ_X_Volt_A_1=ADSample_buffer[0];
	LPF_5KHZ_Y_Volt_A_1=LPF_temp1;
	ADSample_buffer[0]=LPF_temp1;
	
	LPF_temp0=LPF_5KHZ_X_Volt_B_1*0.9567;
	LPF_temp1=LPF_5KHZ_Y_Volt_B_1*0.04328;
	LPF_temp1=LPF_temp1+LPF_temp0;
	LPF_5KHZ_X_Volt_B_1=ADSample_buffer[1];
	LPF_5KHZ_Y_Volt_B_1=LPF_temp1;
	ADSample_buffer[1]=LPF_temp1;	
	
///////////////////////
		
	ADSample_buffer[2]=-ADSample_buffer[0]-ADSample_buffer[1];
	
	if(latch==0)
	{	
		HA_Port_Volt_AB=ADSample_buffer[0];      
		HA_Port_Volt_BC=ADSample_buffer[1];      
		HA_Port_Volt_CA=ADSample_buffer[2];		
	}
	else
	{
		HA_Port_Volt_AB_LATCH=ADSample_buffer[0];      
		HA_Port_Volt_BC_LATCH=ADSample_buffer[1];      
		HA_Port_Volt_CA_LATCH=ADSample_buffer[2];		
	}	
			
	AD_CalTemp=ADSample_buffer[0];
	HA_Port_Volt_A_Temp=(AD_CalTemp*2.0+ADSample_buffer[1])/3.0;
	
	HA_Port_Volt_B_Temp=ADSample_buffer[1];
	HA_Port_Volt_B_Temp=(HA_Port_Volt_B_Temp-ADSample_buffer[0])/3.0;
	
	HA_Port_Volt_C_Temp=ADSample_buffer[1];
	HA_Port_Volt_C_Temp=(HA_Port_Volt_C_Temp*2.0+ADSample_buffer[0])/(-3.0);

	HA_Port_Volt_A=HA_Port_Volt_A_Temp;      
	HA_Port_Volt_B=HA_Port_Volt_B_Temp;    	  
	HA_Port_Volt_C=HA_Port_Volt_C_Temp;	

//port current from module

  	if(HA_PM_A1&0x007f)
  	{
  		Port_AD_Buf[12]=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_IG;
  	}
  	else if(HA_PM_A2&0x007f)
  	{
   		Port_AD_Buf[12]=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_IG;	
  	}
  	else
  	{
  		Port_AD_Buf[12]=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_IG;
  	}
  	  		
   	if(HA_PM_B1&0x007f)
  	{
  		Port_AD_Buf[13]=FPGA_Interface[Com_B1_PluginUnit_9-2].Avg_IG;
  	}
  	else if(HA_PM_B2&0x007f)
  	{
   		Port_AD_Buf[13]=FPGA_Interface[Com_B2_PluginUnit_10-2].Avg_IG;	
  	}
  	else
  	{
  		Port_AD_Buf[13]=FPGA_Interface[Com_B1_PluginUnit_9-2].Avg_IG;
  	}	


//for(i=0;i<3;i++)
  for(i=0;i<2;i++)
  {
			ADSample_buffer[i]=(float)Port_AD_Buf[12+i];		
			ADSample_buffer[i]=ADSample_buffer[i]/10.0;		
	}		

	HA_Port_Current_A=ADSample_buffer[0];		
	HA_Port_Current_B=ADSample_buffer[1];
	

	HA_Port_Current_C=-ADSample_buffer[0]-ADSample_buffer[1];	
	
//	DAC_BUF[1]=HA_Port_Current_A*100.0;
//	DAC_BUF[2]=HA_Port_Current_B*100.0;
//	DAC_BUF[3]=HA_Port_Current_C*100.0;


  if(HA_Port_Current_A<(-1.0))
  {
  	HA_IG_Log_A=0;
  }
  if(HA_Port_Current_A>1.0)
  {
  	HA_IG_Log_A=1;
  }	
  	
  if(HA_Port_Current_B<(-1.0))
  {
  	HA_IG_Log_B=0;
  }
  if(HA_Port_Current_B>1.0)
  {
  	HA_IG_Log_B=1;
  }  
  	
  if(HA_Port_Current_C<(-1.0))
  {
  	HA_IG_Log_C=0;
  }
  if(HA_Port_Current_C>1.0)
  {
  	HA_IG_Log_C=1;
  }  	

/*
  if(HA_Port_Current_A<0)
  {
  	HA_IG_Log_A=0;
  }
  else
  {
  	HA_IG_Log_A=1;
  }	
  	
  if(HA_Port_Current_B<0)
  {
  	HA_IG_Log_B=0;
  }
  else
  {
  	HA_IG_Log_B=1;
  }  
  	
  if(HA_Port_Current_C<0)
  {
  	HA_IG_Log_C=0;
  }
  else
  {
  	HA_IG_Log_C=1;
  }  	
*/
/*	
//port current from sensor
  Port_AD_Buf[12]=FPGA_Interface[Analog_PluginUnit_3-2].AD[12];
  Port_AD_Buf[14]=FPGA_Interface[Analog_PluginUnit_3-2].AD[14];
	ADSample_buffer[0]=Port_AD_Buf[12];	
	ADSample_buffer[1]=Port_AD_Buf[14];
	
	HA_Port_Current_A_Temp=ADSample_buffer[0];
	ADSample_buffer[0]=(HA_Port_Current_A_Temp/3276.8)*100.0;
	HA_Port_Current_B_Temp=ADSample_buffer[1];
	ADSample_buffer[1]=(HA_Port_Current_B_Temp/3276.8)*100.0;
	
	HA_Port_Current_A=ADSample_buffer[0];		
	HA_Port_Current_B=ADSample_buffer[1];
	HA_Port_Current_C=-ADSample_buffer[0]-ADSample_buffer[1];	

	
  if(HA_Port_Current_A<0)
  {
  	HA_IG_Log_A=0;
  }
  else
  {
  	HA_IG_Log_A=1;
  }	
  	
  if(HA_Port_Current_B<0)
  {
  	HA_IG_Log_B=0;
  }
  else
  {
  	HA_IG_Log_B=1;
  }  
  	
  if(HA_Port_Current_C<0)
  {
  	HA_IG_Log_C=0;
  }
  else
  {
  	HA_IG_Log_C=1;
  }  		
*/
}

/****************************************************************************/
/*                                                                          */
/*              Port calculation                                            */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*              Voltage ordering                                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取模块直流采样汇总并更新各相总电压量
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Module_TotalUdc_Sample(void)
{	
/*
//unsigned char i;
	short SampleBuf_short_1;
	short SampleBuf_short_2;

	SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Sum_DC;
	SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Sum_DC;
	HA_DC_Log_A=0;
	if(SampleBuf_short_1<SampleBuf_short_2)
	{
		HA_DC_Log_A=1;
	}
	SampleBuf_short_2=SampleBuf_short_2+SampleBuf_short_1;
	HA_TotalVDC_A=(float)SampleBuf_short_2;
	
	SampleBuf_short_1=FPGA_Interface[Com_B1_PluginUnit_9-2].Sum_DC;
	SampleBuf_short_2=FPGA_Interface[Com_B2_PluginUnit_10-2].Sum_DC;
	HA_DC_Log_B=0;
	if(SampleBuf_short_1<SampleBuf_short_2)
	{
		HA_DC_Log_B=1;
	}
	SampleBuf_short_2=SampleBuf_short_2+SampleBuf_short_1;
	HA_TotalVDC_B=(float)SampleBuf_short_2;	
	
	SampleBuf_short_1=FPGA_Interface[Com_C1_PluginUnit_13-2].Sum_DC;
	SampleBuf_short_2=FPGA_Interface[Com_C2_PluginUnit_14-2].Sum_DC;
	HA_DC_Log_C=0;
	if(SampleBuf_short_1<SampleBuf_short_2)
	{
		HA_DC_Log_C=1;
	}
	SampleBuf_short_2=SampleBuf_short_2+SampleBuf_short_1;
	HA_TotalVDC_C=(float)SampleBuf_short_2;	
	
	if((HA_TotalVDC_A>SigUst_HVAC_Port)&&(HA_TotalVDC_B>SigUst_HVAC_Port)&&(HA_TotalVDC_C>SigUst_HVAC_Port))     
	{
		PortRunFLAG.Bbits.Port_SigUdc=1;   //charge over
	}
	else
	{
		PortRunFLAG.Bbits.Port_SigUdc=0;
	}	

	if(((HA_TotalVDC_A>(UstOK_HVAC_Port-40.0))&&(HA_TotalVDC_A<(UstOK_HVAC_Port+40.0)))&& \
		 ((HA_TotalVDC_B>(UstOK_HVAC_Port-40.0))&&(HA_TotalVDC_B<(UstOK_HVAC_Port+40.0)))&& \
		 ((HA_TotalVDC_C>(UstOK_HVAC_Port-40.0))&&(HA_TotalVDC_C<(UstOK_HVAC_Port+40.0))))
	{
		PortRunFLAG.Bbits.Port_UdcOK=1;     //start over
	}
	else
	{
		PortRunFLAG.Bbits.Port_UdcOK=0;
	}	
*/	
}

#endif  

 
#ifdef HVDC_online  
/****************************************************************************/
/*                                                                          */
/*                Port Sample                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 采集当前端口 FPGA 的瞬时电压、电流及直流量，并推进原滤波/噪声处理
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_AD_Sample(uchar latch)
{	
//unsigned char i;
	float AD_CalTemp; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
	float ADSample_buffer[3]; /* ADSample_buffer：本轮读取的 FPGA 原始采样字缓存。 */

	short SampleBuf_short_1; /* SampleBuf_short_1：第一个 FPGA 原始采样/汇总字缓存。 */
	short SampleBuf_short_2; /* SampleBuf_short_2：第二个 FPGA 原始采样/汇总字缓存。 */
	float tempAve1,temp_0; /* tempAve1：第一相/第一组模块采样总和与平均值缓存。 */
	/* temp_0：采样累加与原滤波计算的浮点中间缓存 temp_0。 */
	uchar PluginUnit_ONNum; /* PluginUnit_ONNum：参与本次平均值计算的在线模块数量。 */
	
//Port Voltage
	PluginUnit_ONNum=0;
	if(HD_PM_A1)
	{
		SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_DC;
		PluginUnit_ONNum++;
			
		HD_DC_Log=0;
	}
	else
	{
		SampleBuf_short_1=0.0;
			
		HD_DC_Log=1;
	}
		
	if(HD_PM_A2)
	{
		SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_DC;	
		PluginUnit_ONNum++;
			
		if(HD_DC_Log==0)
		{
			if(SampleBuf_short_1<SampleBuf_short_2)
			{
				HD_DC_Log=1;
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
		
	ADSample_buffer[0]=LPF_1KHZ_X_Volt_A_1*0.2695;            
	ADSample_buffer[1]=LPF_1KHZ_Y_Volt_A_1*0.7305;
	ADSample_buffer[1]=ADSample_buffer[1]+ADSample_buffer[0];
	LPF_1KHZ_X_Volt_A_1=tempAve1;
	LPF_1KHZ_Y_Volt_A_1=ADSample_buffer[1];
	tempAve1=ADSample_buffer[1];	
		
	HD_Module_Volt=tempAve1;

//-------------------------------------------------------------------			
	if(HD_2_LD==1)
	{
		HD_Port_Volt=tempAve1;
		HD_TotalVDC=tempAve1;
	}
	else
	{
		temp_0=(float)NHD_module;
		HD_TotalVDC=tempAve1*temp_0;
		HD_Port_Volt=HD_TotalVDC;
	}
//-------------------------------------------------------------------					

//Port current
	PluginUnit_ONNum=0;
	if(HD_PM_A1)
	{
		SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_IG;    //Sum_IG
//		SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Sum_IG;
		PluginUnit_ONNum++;
	}
	else
	{
		SampleBuf_short_1=0.0;
	}
		
	if(HD_PM_A2)
	{
		SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_IG;	   //Sum_IG
//		SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Sum_IG;
		PluginUnit_ONNum++;
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

//-------------------------------------------------------------------	
	if(HD_2_LD==1)
	{
		AD_CalTemp=(float)NHD_module;
		tempAve1=tempAve1*AD_CalTemp;		
	}	
//-------------------------------------------------------------------			
	ADSample_buffer[0]=LPF_1KHZ_X_Current_1*0.2695;            
	ADSample_buffer[1]=LPF_1KHZ_Y_Current_1*0.7305;
	ADSample_buffer[1]=ADSample_buffer[1]+ADSample_buffer[0];
	LPF_1KHZ_X_Current_1=tempAve1;
	LPF_1KHZ_Y_Current_1=ADSample_buffer[1];
	HD_Port_Current=ADSample_buffer[1]/10.0;	     //正极流出
	
	DAC_BUF[2]=HD_Port_Current/50.0+2.5;
}

/****************************************************************************/
/*                                                                          */
/*              Port calculation                                            */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*              Voltage ordering                                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取模块直流采样汇总并更新各相总电压量
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Module_TotalUdc_Sample(void)
{	
/*	
//	unsigned char i;
	short SampleBuf_short_1;
	short SampleBuf_short_2;

	
	SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Sum_DC;
	SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Sum_DC;
	HD_DC_Log=0;
	if(SampleBuf_short_1<SampleBuf_short_2)
	{
		HD_DC_Log=1;
	}
	SampleBuf_short_2=SampleBuf_short_2+SampleBuf_short_1;
	HD_TotalVDC=(float)SampleBuf_short_2;
	
	
	if(HD_TotalVDC>SigUst_HVDC_Port)
	{
		PortRunFLAG.Bbits.Port_SigUdc=1;
	}
	else
	{
		PortRunFLAG.Bbits.Port_SigUdc=0;
	}	
	
	if((HD_TotalVDC>(UstOK_HVDC_Port-40.0))&&(HD_TotalVDC<(UstOK_HVDC_Port+40.0)))
	{
		PortRunFLAG.Bbits.Port_UdcOK=1;
	}
	else
	{
		PortRunFLAG.Bbits.Port_UdcOK=0;
	}	
*/		
}

#endif   	


#ifdef LVDC_online  
/****************************************************************************/
/*                                                                          */
/*                Port Sample                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 采集当前端口 FPGA 的瞬时电压、电流及直流量，并推进原滤波/噪声处理
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_AD_Sample(uchar latch)
{	
	unsigned char i; /* i：本次数组、参数或模块遍历的索引。 */
	float AD_CalTemp; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
	float ADSample_buffer[3]; /* ADSample_buffer：本轮读取的 FPGA 原始采样字缓存。 */

	short SampleBuf_short_1; /* SampleBuf_short_1：第一个 FPGA 原始采样/汇总字缓存。 */
	short SampleBuf_short_2; /* SampleBuf_short_2：第二个 FPGA 原始采样/汇总字缓存。 */
	float tempAve1,tempAve2; /* tempAve1：第一相/第一组模块采样总和与平均值缓存。 */
	/* tempAve2：第二相/第二组模块采样总和与平均值缓存。 */
	float tempAve_wonoise; /* tempAve_wonoise：按原噪声规则处理后的模块平均采样值。 */
	uchar PluginUnit_ONNum; /* PluginUnit_ONNum：参与本次平均值计算的在线模块数量。 */
	  
 //Port Voltage
	PluginUnit_ONNum=0;
	if(LD_PM_A1)
	{
		SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_DC;
		PluginUnit_ONNum++;
			
		LD_DC_Log=0;
	}
	else
	{
		SampleBuf_short_1=0.0;
			
		LD_DC_Log=1;
	}
		
	if(LD_PM_A2)
	{
		SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_DC;	
		PluginUnit_ONNum++;
			
		if(LD_DC_Log==0)
		{
			if(SampleBuf_short_1<SampleBuf_short_2)
			{
				LD_DC_Log=1;
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

	//edited by shiqi
	if ((tempAve1 > (LD_TotalVDC_last + 50))||(tempAve1 < (LD_TotalVDC_last - 50)))
	{
		Samplenoise_udc = 0;
	}
	else
	{
		Samplenoise_udc = 1;
	}
	LD_TotalVDC_last = tempAve1;

	if(Samplenoise_udc == 1)
	{
		tempAve_wonoise = tempAve1;
	}

	ADSample_buffer[0]=LPF_1KHZ_X_Volt_1*0.2695;
	ADSample_buffer[1]=LPF_1KHZ_Y_Volt_1*0.7305;
	ADSample_buffer[1]=ADSample_buffer[1]+ADSample_buffer[0];
	LPF_1KHZ_X_Volt_1=tempAve_wonoise;
	LPF_1KHZ_Y_Volt_1=ADSample_buffer[1];

	LD_TotalVDC=ADSample_buffer[1];

	HWREGH(0x60000622) = (ushort)LD_TotalVDC;
	HWREGH(0x60000624) = Samplenoise_udc;

	//end
	
 //正负极电压
 	for(i=0;i<2;i++)
	{ 
		if(latch==0)  
		{
			Port_AD_Buf[12+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[12+i];
		}
		else
		{
			Port_AD_Buf[12+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[12+i];
		}	
  	
		AD_CalTemp=(float)Port_AD_Buf[12+i];
		AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
		ADSample_buffer[i]=AD_CalTemp;
		AD_CalTemp=LD_Volt_Ratio;
		ADSample_buffer[i]=ADSample_buffer[i]*AD_CalTemp;	
	}
/******************/
	AD_CalTemp=1.001*ADSample_buffer[0]-0.2943;
	tempAve1=AD_CalTemp;

	AD_CalTemp=1.005*ADSample_buffer[1]+0.4078;
	tempAve2=AD_CalTemp;
/******************/
/* 
 	ADSample_buffer[0]=LPF_1KHZ_X_Volt_P_1*0.2695;            
	ADSample_buffer[1]=LPF_1KHZ_Y_Volt_P_1*0.7305;
	ADSample_buffer[1]=ADSample_buffer[1]+ADSample_buffer[0];
	LPF_1KHZ_X_Volt_P_1=tempAve1;
	LPF_1KHZ_Y_Volt_P_1=ADSample_buffer[1];
	LD_Port_PositiveVolt=ADSample_buffer[1];	
*/ 
 	ADSample_buffer[0]=LPF_1KHZ_X_Volt_N_1*0.2695;            
	ADSample_buffer[1]=LPF_1KHZ_Y_Volt_N_1*0.7305;
	ADSample_buffer[1]=ADSample_buffer[1]+ADSample_buffer[0];
	LPF_1KHZ_X_Volt_N_1=tempAve2;
	LPF_1KHZ_Y_Volt_N_1=ADSample_buffer[1];
 	LD_Port_NegativeVolt=ADSample_buffer[1];	
	
	LD_Port_PositiveVolt=LD_TotalVDC-LD_Port_NegativeVolt;	 


//Port Current
	PluginUnit_ONNum=0;
	if(LD_PM_A1)
	{
		SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_IG;   //Sum_IG;
		PluginUnit_ONNum++;
	}
	else
	{
		SampleBuf_short_1=0.0;
	}
		
	if(LD_PM_A2)
	{
		SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_IG;   //Sum_IG;	
		PluginUnit_ONNum++;
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
	AD_CalTemp=(float)NLD_module;
	tempAve1=tempAve1*AD_CalTemp;
				
	ADSample_buffer[0]=LPF_1KHZ_X_Current_1*0.2695;            
	ADSample_buffer[1]=LPF_1KHZ_Y_Current_1*0.7305;
	ADSample_buffer[1]=ADSample_buffer[1]+ADSample_buffer[0];
	LPF_1KHZ_X_Current_1=tempAve1;
	LPF_1KHZ_Y_Current_1=ADSample_buffer[1];
	LD_Port_PositiveCurrent=ADSample_buffer[1];	  //正极流出
	LD_Port_PositiveCurrent=LD_Port_PositiveCurrent/10.0;
//	LD_Port_NegativeCurrent
DAC_BUF[2]=LD_Port_PositiveCurrent/50.0+2.5;

//edited by shiqi, start, current sampling by port current sensor
    tempAve1 = FPGA_Interface[0].AD[14]/8.22;
    ADSample_buffer[0]=LPF_1KHZ_X_Current_port_1*0.2695;
    ADSample_buffer[1]=LPF_1KHZ_Y_Current_port_1*0.7305;
    ADSample_buffer[1]=ADSample_buffer[1]+ADSample_buffer[0];
    LPF_1KHZ_X_Current_port_1 = tempAve1;
    LPF_1KHZ_Y_Current_port_1 = ADSample_buffer[1];
    Port_Current = ADSample_buffer[1];	  //port current
//edited by shiqi, end
}

/****************************************************************************/
/*                                                                          */
/*              Port calculation                                            */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*              Voltage ordering                                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取模块直流采样汇总并更新各相总电压量
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Module_TotalUdc_Sample(void)
{	
/*	
//	unsigned char i;
	short SampleBuf_short_1;
	short SampleBuf_short_2;

	
	SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_DC;
	SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_DC;

//	SampleBuf_short_2=SampleBuf_short_2+SampleBuf_short_1;
//	LD_TotalVDC=(float)SampleBuf_short_2;
	
	
	if((SampleBuf_short_1>SigUst_LVDC_Port)&&(SampleBuf_short_2>SigUst_LVDC_Port))
	{
		PortRunFLAG.Bbits.Port_SigUdc=1;
	}
	else
	{
		PortRunFLAG.Bbits.Port_SigUdc=0;
	}	

	if((SampleBuf_short_1>(UstOK_LVDC_Port-3.0))&&(SampleBuf_short_1<(UstOK_LVDC_Port+3.0)))
	{
		PortRunFLAG.Bbits.Port_UdcOK=1;
	}
	else
	{
		PortRunFLAG.Bbits.Port_UdcOK=0;
	}			
*/	
}

#endif   


#ifdef LVAC_online
/****************************************************************************/
/*                                                                          */
/*                Port Sample                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 采集当前端口 FPGA 的瞬时电压、电流及直流量，并推进原滤波/噪声处理
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_AD_Sample(uchar latch)
{	
	unsigned char i; /* i：本次数组、参数或模块遍历的索引。 */
	float AD_CalTemp; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
	float ADSample_buffer[3]; /* ADSample_buffer：本轮读取的 FPGA 原始采样字缓存。 */
	float LA_Grid_Volt_A_Temp; /* LA_Grid_Volt_A_Temp：低压交流端口 A 相/相间原采样电压，单位 V。 */
	float LA_Grid_Volt_B_Temp; /* LA_Grid_Volt_B_Temp：低压交流端口 B 相/相间原采样电压，单位 V。 */
	float LA_Grid_Volt_C_Temp; /* LA_Grid_Volt_C_Temp：低压交流端口 C 相/相间原采样电压，单位 V。 */
  
  for(i=0;i<2;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[i];
  	}
  	else
  	{
  		Port_AD_Buf[i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[i];
  	}
  	
		AD_CalTemp=(float)Port_AD_Buf[i];
		AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
		ADSample_buffer[i]=AD_CalTemp;
		AD_CalTemp=LA_Volt_Ratio;
		ADSample_buffer[i]=ADSample_buffer[i]*AD_CalTemp;
	}
	
	AD_CalTemp=ADSample_buffer[0];
	LA_Grid_Volt_A_Temp=(AD_CalTemp*2.0+ADSample_buffer[1])/3.0;
	
	LA_Grid_Volt_B_Temp=ADSample_buffer[1];
	LA_Grid_Volt_B_Temp=(LA_Grid_Volt_B_Temp-ADSample_buffer[0])/3.0;
	
	LA_Grid_Volt_C_Temp=ADSample_buffer[1];
	LA_Grid_Volt_C_Temp=(LA_Grid_Volt_C_Temp*2.0+ADSample_buffer[0])/(-3.0);
	
	if(latch==0)
	{
		LA_Grid_Volt_A=LA_Grid_Volt_A_Temp;
		LA_Grid_Volt_B=LA_Grid_Volt_B_Temp;
		LA_Grid_Volt_C=LA_Grid_Volt_C_Temp;
	}
	else
	{
		LA_Grid_Volt_A_LATCH=LA_Grid_Volt_A_Temp;
		LA_Grid_Volt_B_LATCH=LA_Grid_Volt_B_Temp;
		LA_Grid_Volt_C_LATCH=LA_Grid_Volt_C_Temp;		
	}
	
	if(latch==0)
	{
		Port_AD_Buf[2]=FPGA_Interface[Analog_PluginUnit_3-2].AD[2];
	}
	else
	{
		Port_AD_Buf[2]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[2];
	}
	AD_CalTemp=(float)Port_AD_Buf[2];
	AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
	ADSample_buffer[0]=AD_CalTemp;
	AD_CalTemp=LA_DCVolt_Ratio;
	ADSample_buffer[0]=ADSample_buffer[0]*AD_CalTemp;

	if(latch==0)
	{
		LA_DCbus_Volt=ADSample_buffer[0];	
	}
	else
	{
		LA_DCbus_Volt_LATCH=ADSample_buffer[0];
	}

				  
  for(i=0;i<3;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[3+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[3+i];
  	}
  	else
  	{
  		Port_AD_Buf[3+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[3+i];
  	}  	
  	
		AD_CalTemp=(float)Port_AD_Buf[3+i];
		AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
		ADSample_buffer[i]=AD_CalTemp;
		AD_CalTemp=LA_Current_Ratio;
		ADSample_buffer[i]=ADSample_buffer[i]*AD_CalTemp;
	}

	if(latch==0)
	{
		LA_Grid_Current_A=ADSample_buffer[0];
		LA_Grid_Current_B=ADSample_buffer[1];
		LA_Grid_Current_C=ADSample_buffer[2];
	}
	else
	{
		LA_Grid_Current_A_LATCH=ADSample_buffer[0];
		LA_Grid_Current_B_LATCH=ADSample_buffer[1];
		LA_Grid_Current_C_LATCH=ADSample_buffer[2];		
	}

  for(i=0;i<3;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[6+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[6+i];
  	}
  	else
  	{
  		Port_AD_Buf[6+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[6+i];
  	}  
  	  	
		AD_CalTemp=(float)Port_AD_Buf[6+i];
		AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
		ADSample_buffer[i]=AD_CalTemp;
		AD_CalTemp=LA_InvCapVolt_Ratio;
		ADSample_buffer[i]=ADSample_buffer[i]*AD_CalTemp;
	}

	if(latch==0)
	{
		LA_Port_Volt_A=ADSample_buffer[0];
		LA_Port_Volt_B=ADSample_buffer[1];
		LA_Port_Volt_C=ADSample_buffer[2];
	}
	else
	{
		LA_Port_Volt_A_LATCH=ADSample_buffer[0];
		LA_Port_Volt_B_LATCH=ADSample_buffer[1];
		LA_Port_Volt_C_LATCH=ADSample_buffer[2];	
	}

  for(i=0;i<3;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[9+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[9+i];
  	}
  	else
  	{
  		Port_AD_Buf[9+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[9+i];
  	}  
  	
		AD_CalTemp=(float)Port_AD_Buf[9+i];
		AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
		ADSample_buffer[i]=AD_CalTemp;
		AD_CalTemp=LA_InvCapCurrent_Ratio;
		ADSample_buffer[i]=ADSample_buffer[i]*AD_CalTemp;
	}

	if(latch==0)
	{
		LA_Cap_Current_A=ADSample_buffer[0];
		LA_Cap_Current_B=ADSample_buffer[1];
		LA_Cap_Current_C=ADSample_buffer[2];
	}
	else
	{
		LA_Cap_Current_A_LATCH=ADSample_buffer[0];
		LA_Cap_Current_B_LATCH=ADSample_buffer[1];
		LA_Cap_Current_C_LATCH=ADSample_buffer[2];
	}

  for(i=0;i<3;i++)
  {
  	if(latch==0)
  	{
  		Port_AD_Buf[12+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD[12+i];
  	}
  	else
  	{
  		Port_AD_Buf[12+i]=FPGA_Interface[Analog_PluginUnit_3-2].AD_LATCH[12+i];
  	}  
  	
		AD_CalTemp=(float)Port_AD_Buf[12+i];
		AD_CalTemp=(AD_CalTemp*10.0)/32768.0;
		ADSample_buffer[i]=AD_CalTemp;
		AD_CalTemp=LA_InvLgCurrent_Ratio;
		ADSample_buffer[i]=ADSample_buffer[i]*AD_CalTemp;
	}
	
	if(latch==0)
	{
		LA_Lg_Current_A=ADSample_buffer[0];
		LA_Lg_Current_B=ADSample_buffer[1];
		LA_Lg_Current_C=ADSample_buffer[2];	
	}
	else
	{
		LA_Lg_Current_A_LATCH=ADSample_buffer[0];
		LA_Lg_Current_B_LATCH=ADSample_buffer[1];
		LA_Lg_Current_C_LATCH=ADSample_buffer[2];	
	}	
}

/****************************************************************************/
/*                                                                          */
/*              Port calculation                                            */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*              Voltage ordering                                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取模块直流采样汇总并更新各相总电压量
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Module_TotalUdc_Sample(void)
{	
//	unsigned char i;
	short SampleBuf_short_1; /* SampleBuf_short_1：第一个 FPGA 原始采样/汇总字缓存。 */
	short SampleBuf_short_2; /* SampleBuf_short_2：第二个 FPGA 原始采样/汇总字缓存。 */

	
	SampleBuf_short_1=FPGA_Interface[Com_A1_PluginUnit_5-2].Avg_DC;
	SampleBuf_short_2=FPGA_Interface[Com_A2_PluginUnit_6-2].Avg_DC;

//	SampleBuf_short_2=SampleBuf_short_2+SampleBuf_short_1;
//	LA_TotalVDC=(float)SampleBuf_short_2;
	
	
	if((SampleBuf_short_1>SigUst_LVAC_Port)&&(SampleBuf_short_2>SigUst_LVAC_Port))
	{
		PortRunFLAG.Bbits.Port_SigUdc=1;
	}
	else
	{
		PortRunFLAG.Bbits.Port_SigUdc=0;
	}	

	if((SampleBuf_short_1>(UstOK_LVAC_Port-3.0))&&(SampleBuf_short_1<(UstOK_LVAC_Port+3.0)))
	{
		PortRunFLAG.Bbits.Port_UdcOK=1;
	}
	else
	{
		PortRunFLAG.Bbits.Port_UdcOK=0;
	}
}

#endif  
















