#include "SstControlConfig.h"
#include "FpgaIo.h"
#include "SstPortStatus.h"
#include "ControlData.h"
#include "ControlInit.h"
#include "DpramHostData.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SoftTimeData.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "SstTypes.h"

//###########################################################################
//
// FILE:   FaultCheck.c
//
// FUNCTION:  1)Fault Checking & disposing 
//            2)PWM disable & enable
//            3)Fault clear
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################
#include "FaultCheck.h"
#include "PwmGate.h"
#include "hw_types.h" 

/*
 * 故障处理文件的阅读提示：
 *
 * 1. OC_OV_LV_faultcheck() 只负责“采样并置故障标志”，不直接做封波；
 *    对应的 OC_OV_LV_FaultDispose() 再负责停机、封波和写入故障位。
 * 2. FaultCheckAndDispose_StepOne() 读取主 FPGA 的 FB、端口保护和 UNIT FO，
 *    把硬件故障翻译成 FaultTypeFLAG/FaultDisposeFLAG。
 * 3. FaultDispose() 根据这些标志决定封锁整端口、仅封锁 50 Hz、仅封锁 DAB，
 *    并维护旧 HVAC 状态变量和故障记录。
 * 4. PWMPluse_Enable()/PWMPluse_Disable() 是最终的 PWM 闸门；参数 1 表示
 *    CHB 50 Hz 桥，参数 2 表示 DAB 桥，参数 3/封波类型 3 表示整端口。
 *
 * 当前 Chb_DabPrim_Readable 工程定义了 HVAC_online，因此调试时优先阅读
 * HVAC_online 分支。LVDC_online/HVDC_online 是原工程保留的其他端口实现，
 * 只有切换编译宏后才会参与链接。
 */
/************************Variable definition********************************/

/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 读取端口 AD、FB、FO 锁存并执行原第一阶段快速故障判断
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void FaultCheckAndDispose_StepOne(void);
/*==================================================================
 * 函数功能: 逐模块分类故障并按原策略撤销 PWM、记录故障类型及处置状态
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void FaultDispose(void);
/*==================================================================
 * 函数功能: 发起或推进原两拍故障锁存清除流程
 * 输入参数: runindex：原故障锁存清除时序的执行/初始化选择值，沿用现有调用点
 *           RstFlagFault：旧故障复位阶段选择
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void FaultLatch_Clear(uchar runindex,uchar RstFlagFault);



/*==================================================================
 * 函数功能: 按原阈值和计数策略检查端口交流过流、直流过压及欠压条件
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void OC_OV_LV_faultcheck(void);
/*==================================================================
 * 函数功能: 将原软件电压、电流保护判据转换为端口故障及处置标志
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void OC_OV_LV_FaultDispose(void);

//over current and over voltage protection,edited by shiqi, start

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 按原阈值和计数策略检查端口交流过流、直流过压及欠压条件
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void OC_OV_LV_faultcheck(void)
{
	float temp_set1, temp_set2, temp_set3, temp_set4; /* temp_set1：原一级过流阈值的本轮计算缓存。 */
	/* temp_set2：原一级过流确认时间的本轮计算缓存。 */
	/* temp_set3：原二级过流阈值的本轮计算缓存。 */
	/* temp_set4：原二级过流确认时间的本轮计算缓存。 */
	float grid_current_A, grid_current_B, grid_current_C; /* grid_current_A：A 相软件过流保护读取的电流绝对值。 */
	/* grid_current_B：B 相软件过流保护读取的电流绝对值。 */
	/* grid_current_C：C 相软件过流保护读取的电流绝对值。 */
	float temp_set5, temp_set6; /* temp_set5：原过压阈值的本轮计算缓存。 */
	/* temp_set6：原过压确认时间的本轮计算缓存。 */
	float DC_vol; /* DC_vol：软件电压保护读取的直流采样/电压值。 */

	/*
	 * 软件 OC/OV 保护采用“越限 + 延时确认 + 回差复位”：
	 * - 一级过流超过 OC_level_SET，连续 OC_time_SET 后置 HVAC_OC_Fault；
	 * - 二级过流超过 OC_level_SET_L2，连续 OC_time_SET_L2 后置同一故障；
	 * - 直流过压超过 OV_level_SET，连续 OV_time_SET 后置 HVAC_OV_Fault；
	 * - 过流恢复到 95% 门槛以下才清计数，避免阈值附近抖动。
	 * 本函数由周期任务调用，counter_* 由定时中断递增。
	 */
	temp_set1 = OC_level_SET;
	temp_set2 = 0.95*temp_set1;

	temp_set5 = OV_level_SET;
	temp_set6 = 0.95*OV_level_SET;

	grid_current_A = PORT_RealTime_DATA.PORT_I_A;
	grid_current_B = PORT_RealTime_DATA.PORT_I_B;
	grid_current_C = PORT_RealTime_DATA.PORT_I_C;

	DC_vol = SVG_Udc_Ave;


  if((ControlFLAG.Bbits.SVG_Start)||(Port_RunStatus==PortStatusNum_Run))
  {
	if((grid_current_A > temp_set1)||(grid_current_B > temp_set1)||(grid_current_C > temp_set1))
	{
		if(counter_OC_flag != 1 && HVAC_OC_Fault != 1)
		{
			counter_OC_flag = 1;
			counter_OC_time = 0;
		}
		else if(counter_OC_time > OC_time_SET)
		{
			HVAC_OC_Fault = 1;
			counter_OC_flag = 0;
			counter_OC_time = 0;
		}
		else
		{
		}
	}
	else if ((grid_current_A < temp_set2)&&(grid_current_B < temp_set2)&&(grid_current_C < temp_set2))
	{
		counter_OC_flag = 0;
		counter_OC_time = 0;
	}

	if((grid_current_A > OC_level_SET_L2)||(grid_current_B > OC_level_SET_L2)||(grid_current_C > OC_level_SET_L2))
	{
		if(counter_OC_flag1 != 1 && HVAC_OC_Fault != 1)
		{
			counter_OC_flag1 = 1;
			counter_OC_time1 = 0;
		}
		else if(counter_OC_time1 > OC_time_SET_L2)
		{
			HVAC_OC_Fault = 1;
			counter_OC_flag1 = 0;
			counter_OC_time1 = 0;
		}
		else
		{
		}
	}
	else
	{
		counter_OC_flag1 = 0;
		counter_OC_time1 = 0;
	}


	if(DC_vol > temp_set5)
	{
		if(counter_OV_flag != 1 && HVAC_OV_Fault != 1)
		{
			counter_OV_flag = 1;
			counter_OV_time = 0;
		}
		else if(counter_OV_time > OV_time_SET)
		{
			HVAC_OV_Fault = 1;
			counter_OV_flag = 0;
			counter_OV_time = 0;
		}
		else
		{
		}
	}
	else if (DC_vol < temp_set6)
	{
		counter_OV_flag = 0;
		counter_OV_time = 0;
	}
  }
}

/*==================================================================
 * 函数功能: 将原软件电压、电流保护判据转换为端口故障及处置标志
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void OC_OV_LV_FaultDispose(void)
{
//	if(ControlFLAG.Bbits.Port_Stop == 1)
//	{
//	    OC_OV_LV_PortLatch_TAD = 1;
//	}
//	else if(ControlFLAG.Bbits.Port_Start != 0x02)
//	{
//		OC_OV_LV_PortLatch_TAD = 1;
//	}
//	else
//	{
		FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
		FaultDisposeFLAG.Bbits.Port_StopType = 1;
		Port_RunStatus = PortStatusNum_Stop;
		ControlFLAG.Bbits.Port_Stop = 0x01;
		ControlFLAG.Bbits.Port_Start = 0x00;
		if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
		{
			FaultDisposeFLAG.Bbits.Port_StopAll=1;
			PortRunFLAG.Bbits.Stop_AllPort=1;
			Port_StateUpdate_Write();
		}


		FaultDisposeFLAG.Bbits.PWMPulseState=3;
		PWMPluse_Disable(0);
		OC_OV_LV_PortLatch_TAD = 1;
		if (HVAC_OV_Fault == 1)
		{
		    Port_RealFault.Bbits.PORT_FAULT_bit13 = 1;
		}
		if (HVAC_OC_Fault == 1)
		{
		    Port_RealFault.Bbits.PORT_FAULT_bit14 = 1;
		}
//	}
}
#endif

#ifdef LVDC_online
/*==================================================================
 * 函数功能: 按原阈值和计数策略检查端口交流过流、直流过压及欠压条件
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void OC_OV_LV_faultcheck(void)
{
	ushort i; /* i：本次数组、参数或模块遍历的索引。 */
	float temp_set1, temp_set2, temp_set3, temp_set4; /* temp_set1：原一级过流阈值的本轮计算缓存。 */
	/* temp_set2：原一级过流确认时间的本轮计算缓存。 */
	/* temp_set3：原二级过流阈值的本轮计算缓存。 */
	/* temp_set4：原二级过流确认时间的本轮计算缓存。 */
	float grid_current; /* grid_current：软件过流保护读取的电流绝对值。 */
	float temp_set5, temp_set6, temp_set7, temp_set8; /* temp_set5：原过压阈值的本轮计算缓存。 */
	/* temp_set6：原过压确认时间的本轮计算缓存。 */
	/* temp_set7：原欠压阈值的本轮计算缓存。 */
	/* temp_set8：原欠压确认时间的本轮计算缓存。 */
	float DC_vol; /* DC_vol：软件电压保护读取的直流采样/电压值。 */

	temp_set1 = OC_level_SET;
	temp_set2 = 0.95*temp_set1;
	temp_set3 = 0 - OC_level_SET;
	temp_set4 = 0.95*temp_set3;

	temp_set5 = OV_level_SET;
	temp_set6 = 0.95*OV_level_SET;

	temp_set7 = OC_level_SET_L2;
	temp_set8 = 0 - OC_level_SET_L2;

	grid_current = Port_Current;
	DC_vol = LD_TotalVDC;


	if((grid_current > temp_set1)||(grid_current < temp_set3))
	{
		if(counter_OC_flag != 1 && LVDC_OC_Fault != 1)
		{
			counter_OC_flag = 1;
			counter_OC_time = 0;
		}
		else if(counter_OC_time > OC_time_SET)
		{
			LVDC_OC_Fault = 1;
			counter_OC_flag = 0;
			counter_OC_time = 0;
		}
		else
		{
		}
	}
	else if ((grid_current < temp_set2)&&(grid_current > temp_set4))
	{
		counter_OC_flag = 0;
		counter_OC_time = 0;
	}

	if((grid_current > temp_set7)||(grid_current < temp_set8))
	{
		if(counter_OC_flag1 != 1 && LVDC_OC_Fault != 1)
		{
			counter_OC_flag1 = 1;
			counter_OC_time1 = 0;
		}
		else if(counter_OC_time1 > OC_time_SET_L2)
		{
			LVDC_OC_Fault = 1;
			counter_OC_flag1 = 0;
			counter_OC_time1 = 0;
		}
		else
		{
		}
	}
	else
	{
		counter_OC_flag1 = 0;
		counter_OC_time1 = 0;
	}

	/* 只有 SVG 已启动或端口处于运行态时才检查运行中过流/过压。 */
	if((ControlFLAG.Bbits.SVG_Start)||(Port_RunStatus==PortStatusNum_Run))
	{
	if(DC_vol > temp_set5)
	{
		if(counter_OV_flag != 1 && LVDC_OV_Fault != 1)
		{
			counter_OV_flag = 1;
			counter_OV_time = 0;
		}
		else if(counter_OV_time > OV_time_SET)
		{
			LVDC_OV_Fault = 1;
			counter_OV_flag = 0;
			counter_OV_time = 0;
		}
		else
		{
		}
	}
	else if (DC_vol < temp_set6)
	{
		counter_OV_flag = 0;
		counter_OV_time = 0;
	}
	}

}

/*==================================================================
 * 函数功能: 将原软件电压、电流保护判据转换为端口故障及处置标志
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void OC_OV_LV_FaultDispose(void)
{
    /*
     * HVAC 软件 OC/OV 已经确认后，执行一次端口级停机：
     * 1) 设置正常/故障停机状态；
     * 2) 令 PWMPluse_Disable(0) 按 PWMPulseState=3 封锁 CHB+DAB；
     * 3) 置端口故障位 bit13/bit14，供 DPRAM/485 遥测读取。
     */
//	if(ControlFLAG.Bbits.Port_Stop == 1)
//	{
//	    OC_OV_LV_PortLatch_TAD = 1;
//	}
//	else if(ControlFLAG.Bbits.Port_Start != 0x02)
//	{
//		OC_OV_LV_PortLatch_TAD = 1;
//	}
//	else
//	{
		FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
		FaultDisposeFLAG.Bbits.Port_StopType = 1;
		Port_RunStatus = PortStatusNum_Stop;
		ControlFLAG.Bbits.Port_Stop = 0x01;
		ControlFLAG.Bbits.Port_Start = 0x00;
		if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
		{
			FaultDisposeFLAG.Bbits.Port_StopAll=1;
			PortRunFLAG.Bbits.Stop_AllPort=1;
			Port_StateUpdate_Write();
		}

		FaultDisposeFLAG.Bbits.PWMPulseState=3;
		PWMPluse_Disable(0);
		OC_OV_LV_PortLatch_TAD = 1;

		if (LVDC_OV_Fault == 1)
		{
		    Port_RealFault.Bbits.PORT_FAULT_bit13 = 1;
		    FaultDisposeFLAG.Bbits.Port_StopAll=1;
		    PortRunFLAG.Bbits.Stop_AllPort=1;
		    Port_StateUpdate_Write();
		}
		if (LVDC_OC_Fault == 1)
		{
		    Port_RealFault.Bbits.PORT_FAULT_bit14 = 1;
		}
//	}
}
#endif
//edited by shiqi, end

/****************************************************************************/
/*                                                                          */
/*                      Fault Check And Dispose   Step1                     */
/*                                                                          */
/****************************************************************************/	
/*==================================================================
 * 函数功能: 读取端口 AD、FB、FO 锁存并执行原第一阶段快速故障判断
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void FaultCheckAndDispose_StepOne(void)
{
	ushort FBLatch_Temp; /* FBLatch_Temp：本次读取的端口 FB 故障锁存位图。 */
			
//ushort PWMPulseState_Temp;
	ushort ADProtect_Temp; /* ADProtect_Temp：本次读取的端口 AD 保护锁存位图。 */

	/*
	 * 第一步是“读硬件并形成软件故障分类”，不是最终故障处理：
	 * - 先按端口级别封一次 PWM，保证读取故障寄存器时功率级不会继续加剧故障；
	 * - 读取主 FPGA FB，并按 FB_EN 屏蔽未使能的反馈通道；
	 * - 端口 AD 保护映射为端口过流/过压；
	 * - 再根据 PORT_Index 读取对应 UNIT 的 FO，形成模块故障标志。
	 */
	FaultDisposeFLAG.Bbits.PWMPulseState=0x03;
	PWMPluse_Disable(0);
	Port_BKU_Send(1);    //send BKU
	
	ControlFLAG.Bbits.Port_FaultDispose=1;
	//check
	FBLatch_Temp=Port_FBStatusRead(1);                               //读主FPGA的FB锁存
	FBLatch_Temp=(FBLatch_Temp&PORT_Regulate_DATA.FB_EN.FB_EN16);
	FBLatch_TAD=FBLatch_Temp;
		
	/* Analog 插件对应的 FB 位表示端口级保护触发。 */
	PortLatch_TAD=(FBLatch_Temp&(0x0001<<(Analog_PluginUnit_3-2)));    
	if(PortLatch_TAD)                                                //端口故障
	{
		ADProtect_Temp=Port_AD_ProtectStatusRead(1);
		ADProtect_Temp=ADProtect_Temp&PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
		
#ifdef FaultOutTest
		Flag_DC_FAULT=1;
		Flag_DC_FAULTport=1;
		Flag_DC_FAULTDISPOSE=ADProtect_Temp;
#endif
		
		if(ADProtect_Temp)
		{		
			/* 不同端口的 AD_PROTECT 位定义不同，这里按端口号解释。 */
			switch(PORT_Index)                          //需要依据实际接口而定？？？？？？？？？？？？？？？？？？？？？？？？
			{
				case 1:
				{
					FaultTypeFLAG.Bbits.Port_OverCurrent=1;       
				}break;
				case 2:
				{
					if(ADProtect_Temp==0x1000)
					{
						FaultTypeFLAG.Bbits.Port_OverVolt=1;
					}	
					else
					{
						FaultTypeFLAG.Bbits.Port_OverCurrent=1;
					}			
				}break;
				case 3:
				{
					if(ADProtect_Temp&0x3000)
					{
						FaultTypeFLAG.Bbits.Port_OverVolt=1;
					}	
					else
					{
						FaultTypeFLAG.Bbits.Port_OverCurrent=1;
					}					
				}break;
				case 4:
				{
					FaultTypeFLAG.Bbits.Port_OverCurrent=1;			
				}break;
			
				default:break;
			}
		}
	}


	/* 已经处于停机流程时不再重复扫描 UNIT FO，避免停机过程反复改标志。 */
	if ((ControlFLAG.Bbits.Port_Stop==1)||(ControlFLAG.Bbits.Port_Stop==2))
 {

 }
 else
 {
	ModuleFault_DisposeTimes=0;
	
	  if(PORT_Index==1)                                                //HVAC：A/B/C三相UNIT故障
  {
  	FOLatch_A_TAD=0;
  	FOLatch_B_TAD=0;
  	FOLatch_C_TAD=0;
  	
  	FBLatch_Temp=FBLatch_TAD;
  	if(FBLatch_Temp&(0x0001<<(Input_A_PluginUnit_8-2)))
  	{
  		FOLatch_A_TAD=Port_FOStatusRead(Input_A_PluginUnit_8,1);
  		FOLatch_A_TAD=FOLatch_A_TAD&PORT_Regulate_DATA.FO_1_EN.FO_EN16;
  	}
  	
  	FBLatch_Temp=FBLatch_TAD;
  	if(FBLatch_Temp&(0x0001<<(Input_B_PluginUnit_12-2)))
  	{
  		FOLatch_B_TAD=Port_FOStatusRead(Input_B_PluginUnit_12,1);
  		FOLatch_B_TAD=FOLatch_B_TAD&PORT_Regulate_DATA.FO_2_EN.FO_EN16;
  	}

  	FBLatch_Temp=FBLatch_TAD;
  	if(FBLatch_Temp&(0x0001<<(Input_C_PluginUnit_16-2)))  
  	{	
  		FOLatch_C_TAD=Port_FOStatusRead(Input_C_PluginUnit_16,1);
  		FOLatch_C_TAD=FOLatch_C_TAD&PORT_Regulate_DATA.FO_3_EN.FO_EN16;
		}
		
		if(FOLatch_A_TAD||FOLatch_B_TAD||FOLatch_C_TAD)
		{
			FaultTypeFLAG.Bbits.Module_Fault=1;
			ModuleFault_DisposeTimes=3;
		}
  }
	  else if((PORT_Index==2)||(PORT_Index==3))                        //其他直流端口：单路UNIT故障
  {
  	FOLatch_A_TAD=0;
  	FOLatch_B_TAD=0;
  	FOLatch_C_TAD=0;

  	FBLatch_Temp=FBLatch_TAD;
  	if(FBLatch_Temp&(0x0001<<(Input_A_PluginUnit_8-2)))
  	{  	
   		FOLatch_A_TAD=Port_FOStatusRead(Input_A_PluginUnit_8,1);
  		FOLatch_A_TAD=FOLatch_A_TAD&PORT_Regulate_DATA.FO_1_EN.FO_EN16;
		}
		
		if(FOLatch_A_TAD)
		{
			FaultTypeFLAG.Bbits.Module_Fault=1;
			ModuleFault_DisposeTimes=3;
		} 
		
#ifdef FaultOutTest
		Flag_DC_FAULT=1;
		Flag_DC_FAULTport=0;
		Flag_DC_FAULTDISPOSE=FOLatch_A_TAD;
#endif		
  }
	  else if(PORT_Index==4)                                          //LVAC：模块及逆变桥故障
  {
  	FOLatch_A_TAD=0;
  	FOLatch_B_TAD=0;
  	FOLatch_C_TAD=0;
		FOLatch_INV_TAD=0;
		
  	FBLatch_Temp=FBLatch_TAD;
  	if(FBLatch_Temp&(0x0001<<(Input_A_PluginUnit_8-2)))
  	{  	
   		FOLatch_A_TAD=Port_FOStatusRead(Input_A_PluginUnit_8,1);
  		FOLatch_A_TAD=FOLatch_A_TAD&PORT_Regulate_DATA.FO_1_EN.FO_EN16;
		}

  	FBLatch_Temp=FBLatch_TAD;
  	if(FBLatch_Temp&(0x0001<<(InvPWM_A_PluginUnit_9-2)))
  	{
  		FOLatch_INV_TAD=FOLatch_INV_TAD|0x0001;
  	}
  	FBLatch_Temp=FBLatch_TAD;	
  	if(FBLatch_Temp&(0x0001<<(InvPWM_B_PluginUnit_10-2)))
  	{
  		FOLatch_INV_TAD=FOLatch_INV_TAD|0x0002;
  	}  	
  	FBLatch_Temp=FBLatch_TAD;
  	if(FBLatch_Temp&(0x0001<<(InvPWM_C_PluginUnit_11-2)))
  	{
  		FOLatch_INV_TAD=FOLatch_INV_TAD|0x0004;
  	}
  	FBLatch_Temp=FBLatch_TAD;	
  	if(FBLatch_Temp&(0x0001<<(InvPWM_N_PluginUnit_12-2)))
  	{
  		FOLatch_INV_TAD=FOLatch_INV_TAD|0x0008;
  	}  	
  			
		if(FOLatch_A_TAD)                                        //模块故障
		{
			FaultTypeFLAG.Bbits.Module_Fault=1;
			ModuleFault_DisposeTimes=3;
		} 		
		
		if(FOLatch_INV_TAD)                                      //逆变故障
		{
			FaultTypeFLAG.Bbits.Module_50Hz_H=1;
			
			//Disable INV PWM Pluse
			FaultDisposeFLAG.Bbits.FaultDisposeType=1;
			FaultDisposeFLAG.Bbits.PWMPulseState=1;
			ModuleIndex_PWMPulseDisable=0;
			FaultDisposeFLAG.Bbits.Port_StopType=0;
    
			PWMPluse_Disable(0);								

			if((ModuleFault_DisposeTimes==0)&&(!FaultTypeFLAG.Bbits.Module_OverCurrent)&&(!FaultTypeFLAG.Bbits.Module_OverVolt)&&(!FaultTypeFLAG.Bbits.Module_OverTemp)&& \
				 (!FaultTypeFLAG.Bbits.Module_DAB_H))
			{
				//Clear Latch Register
				FaultLatch_Clear(0,0);
			}			
		}
  }
  else{}
 }
}

/****************************************************************************/
/*                                                                          */
/*                      Fault Dispose                                       */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 逐模块分类故障并按原策略撤销 PWM、记录故障类型及处置状态
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void FaultDispose(void)
{	
	/*
	 * 故障处理第二步：把 FaultCheckAndDispose_StepOne() 形成的锁存量真正作用到 PWM。
	 *
	 * 优先级从高到低可以理解为：端口故障 > 模块故障 > 单个桥故障。
	 * 端口故障直接封锁全部 PWM；HVAC 模块故障根据 FO/FB 位分别封锁 DAB、
	 * 50 Hz 或整端口，并在没有更严重故障时清除临时硬件锁存，允许后续恢复。
	 */
	ushort ModuleStatus_INTTemp,ModuleStatus_INTTemp_1,ModuleStatus_INTTemp_2,ModuleStatus_INTTemp_3; /* ModuleStatus_INTTemp：故障处置本次读取的模块状态字。 */
	/* ModuleStatus_INTTemp_1：模块状态字第一步掩码解码缓存。 */
	/* ModuleStatus_INTTemp_2：模块状态字第二步掩码解码缓存。 */
	/* ModuleStatus_INTTemp_3：模块状态字第三步掩码解码缓存。 */
	ushort DCVolt_INTTemp; /* DCVolt_INTTemp：故障处置期间读取的模块直流电压原始值。 */
	ushort FF_Module_DAB_H; /* FF_Module_DAB_H：本轮故障分类的模块DAB 桥位图。 */
	ushort FF_Module_50Hz_H; /* FF_Module_50Hz_H：本轮故障分类的模块工频桥位图。 */
//ushort FF_Module_Single;
	ushort FF_Module_OverVolt; /* FF_Module_OverVolt：本轮故障分类的模块过压位图。 */
	ushort FF_Module_OverCurrent; /* FF_Module_OverCurrent：本轮故障分类的模块过流位图。 */
	ushort FF_Module_OverTemp; /* FF_Module_OverTemp：本轮故障分类的模块过温位图。 */
	ushort FF_Module_OverVoltLevel2; /* FF_Module_OverVoltLevel2：本轮故障分类的模块二级过压位图。 */
	ushort tt_INT,i; /* tt_INT：原模块故障处置的临时索引/计数。 */
	/* i：本次数组、参数或模块遍历的索引。 */

	/* 正常停机或外部要求封波时，不再继续执行故障处理计数。 */
	if(ControlFLAG.Bbits.Port_Stop==1) //Stopping
	{
		ModuleFault_DisposeTimes=0;	
		ControlFLAG.Bbits.Port_FaultDispose=0;
	}
	else if(Flag_FaultOff==1)
	{
		ModuleFault_DisposeTimes=0;
		ControlFLAG.Bbits.Port_FaultDispose=0;
	}
	else
	{
		if(PortLatch_TAD)                          //端口故障
		{		
			/* 端口故障：整端口封波，并在主端口模式下通知系统停止其他端口。 */
  		    ModuleFault_DisposeTimes=0;
  		
			if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))             //主电源，整个系统停机
			{
				FaultDisposeFLAG.Bbits.Port_StopAll=1;
				PortRunFLAG.Bbits.Stop_AllPort=1; 		//??????????????????????????????????????????????????=0
				Port_StateUpdate_Write();
  		    }

			FaultDisposeFLAG.Bbits.FaultDisposeType=1;
			FaultDisposeFLAG.Bbits.Port_StopType=0;
			FaultDisposeFLAG.Bbits.PWMPulseState=3;
			
			PWMPluse_Disable(0);
	
			PortRunFLAG.Bbits.PWMState_Module=1; 	
			
			ControlFLAG.Bbits.Port_FaultDispose=0;	
			

#ifdef FaultOutTest
		Flag_DC_FAULT=2;
		Flag_DC_FAULTport=1;
		Flag_DC_FAULTDISPOSE=1;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;			
#endif			
		}
		else                                    	//模块故障
		{
			/* 模块故障：下面按 PORT_Index 和各 FO/FB 位决定封锁范围。 */
			switch(PORT_Index)
			{
				case 1:                             	//HVAC
				{
					FF_Module_DAB_H=0;
					FF_Module_50Hz_H=0;
					FF_Module_OverVolt=0;
					FF_Module_OverCurrent=0;
					FF_Module_OverTemp=0;
					FF_Module_OverVoltLevel2=0;	
					tt_INT=0;
					
					if(ModuleFault_DisposeTimes==3)                        //Disable SVG PWM Pluse
					{
						/*
						 * FO 只告诉我们“有模块故障”，先连续保留几次扫描窗口，
						 * 给 UNIT 状态寄存器稳定/读取留时间；计数结束后再进入详细分类。
						 */
						if(flag_FaultClose==1)
						{
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.PWMPulseState=1;
							ModuleIndex_PWMPulseDisable=0;
							FaultDisposeFLAG.Bbits.Port_StopType=0;
						}
						else
						{
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							ModuleIndex_PWMPulseDisable=0;
							FaultDisposeFLAG.Bbits.Port_StopType=0;

							if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))             //主电源，整个系统停机
							{
								FaultDisposeFLAG.Bbits.Port_StopAll=1;
								PortRunFLAG.Bbits.Stop_AllPort=1;       //??????????????????????????????????????????????????=0 		
								Port_StateUpdate_Write();
  						    }
						}
						PWMPluse_Disable(0);

						ModuleFault_DisposeTimes--;
					}
					else
					{
						ModuleFault_DisposeTimes--;
						if(FOLatch_A_TAD&0x00ff)
						{
							ModuleStatus_INTTemp=Module_TotalStatusRead(Com_A1_PluginUnit_5,1);     
						
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_Fault_Mask;
							if(ModuleStatus_INTTemp_2==Module_SVG_H_Mask)                      //只有SVG 的H桥故障
							{
								/* 仅 50 Hz 桥故障，DAB 不必随之封锁。 */
								FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
													
								FF_Module_50Hz_H=1;
							}
							else
							{
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;   //过流
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverCurrent=1;			
									FF_Module_OverCurrent=1;
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverVolt=1;	                        //过压									
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;      //过温
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverTemp=1;	
									FF_Module_OverTemp=1;							
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;   //SVG H桥故障
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
									FF_Module_50Hz_H=1;
								}
							
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;   //DAB H桥故障
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_DAB_H=1;	
									FF_Module_DAB_H=1;
								}		
								               
								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									/* 模块过压风险最高：按 OV_Stop 配置执行端口停机或封波。 */
									//Port shutdown
  								ModuleFault_DisposeTimes=0;
  								
  								ControlFLAG.Bbits.Port_FaultDispose=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;										
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
																		
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????????//=0
											Port_StateUpdate_Write();
  									    }
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????????//=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
								}
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										/* 过流/过温/DAB桥/50 Hz桥故障：至少封锁对应功率桥。 */
										ControlFLAG.Bbits.Port_FaultDispose=0;
										//DAB PWM Pluse Disable
  									    ModuleFault_DisposeTimes=0;
										
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; //???????????????????????????????????????//=0
											Port_StateUpdate_Write();
  									    }
  								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);								                         //闭锁DAB的H桥
										
										tt_INT=1;
									}		
								}
							}
            }
					
						if(FOLatch_A_TAD&0xff00)
						{
							ModuleStatus_INTTemp=Module_TotalStatusRead(Com_A2_PluginUnit_6,1);     
							
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_Fault_Mask;
							if(ModuleStatus_INTTemp_2==Module_SVG_H_Mask)
							{
								FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
								FF_Module_50Hz_H=1;
							}
							else
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}

								if(FF_Module_OverVolt==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
//										FF_Module_OverVolt=1;
									}
								}

								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}
								
								if(FF_Module_50Hz_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
										FF_Module_50Hz_H=1;
									}
								}
								
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	
							
								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									if(FF_Module_OverVolt==0)
									{
										ControlFLAG.Bbits.Port_FaultDispose=0;
										//Port shutdown
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
																		
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//?????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		  //?????????????????????????????????=0
										
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
									}
								}			
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										//DAB PWM Pluse Disable
										ControlFLAG.Bbits.Port_FaultDispose=0;
  									ModuleFault_DisposeTimes=0;			

										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								  								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);
									
										tt_INT=1;
									}		
								}
							}
						}
						
						if(FOLatch_B_TAD&0x00ff)
						{
							ModuleStatus_INTTemp=Module_TotalStatusRead(Com_B1_PluginUnit_9,1);     
							
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_Fault_Mask;
							if(ModuleStatus_INTTemp_2==Module_SVG_H_Mask)
							{
								FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
								FF_Module_50Hz_H=1;
							}
							else
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}

								if(FF_Module_OverVolt==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
//										FF_Module_OverVolt=1;
									}
								}

								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}
								
								if(FF_Module_50Hz_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
										FF_Module_50Hz_H=1;
									}
								}
								
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	

								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									if(FF_Module_OverVolt==0)
									{
										ControlFLAG.Bbits.Port_FaultDispose=0;
										//Port shutdown
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;									
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//?????????????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		//??????????????????????????????????????????=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
									}
								}																
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										//DAB PWM Pluse Disable
  									ControlFLAG.Bbits.Port_FaultDispose=0;
  									ModuleFault_DisposeTimes=0;			

										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//????????????????????????????????????????=0
											Port_StateUpdate_Write();
  									    }
  								  								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);
									
										tt_INT=1;
									}		
								}
							}
						}							
									
						if(FOLatch_B_TAD&0xff00)
						{
							ModuleStatus_INTTemp=Module_TotalStatusRead(Com_B2_PluginUnit_10,1);     
							
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_Fault_Mask;
							if(ModuleStatus_INTTemp_2==Module_SVG_H_Mask)
							{
								FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
								FF_Module_50Hz_H=1;
							}
							else
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}

								if(FF_Module_OverVolt==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
//										FF_Module_OverVolt=1;
									}
								}

								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}
								
								if(FF_Module_50Hz_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
										FF_Module_50Hz_H=1;
									}
								}
								
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	

								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									if(FF_Module_OverVolt==0)
									{
										ControlFLAG.Bbits.Port_FaultDispose=0;
										//Port shutdown
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;									
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//??????????????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		//??????????????????????????????????????????=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
									}
								}			
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										//DAB PWM Pluse Disable
										ControlFLAG.Bbits.Port_FaultDispose=0;
  									ModuleFault_DisposeTimes=0;			
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//????????????????????????????????????????/=0
											Port_StateUpdate_Write();
  									}
								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);
									
										tt_INT=1;
									}		
								}
							}
						}								
							
						if(FOLatch_C_TAD&0x00ff)
						{
							ModuleStatus_INTTemp=Module_TotalStatusRead(Com_C1_PluginUnit_13,1);     
							
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_Fault_Mask;
							if(ModuleStatus_INTTemp_2==Module_SVG_H_Mask)
							{
								FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
								FF_Module_50Hz_H=1;
							}
							else
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}

								if(FF_Module_OverVolt==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
//										FF_Module_OverVolt=1;
									}
								}
								
								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}
								
								if(FF_Module_50Hz_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
										FF_Module_50Hz_H=1;
									}
								}
								
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	

								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									if(FF_Module_OverVolt==0)
									{
										ControlFLAG.Bbits.Port_FaultDispose=0;
										//Port shutdown
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
								
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//?????????????????????????????????=0
											Port_StateUpdate_Write();
  									    }
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
									}
								}			
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										//DAB PWM Pluse Disable
										ControlFLAG.Bbits.Port_FaultDispose=0;
  									ModuleFault_DisposeTimes=0;			
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//?????????????????????????????????=0
											Port_StateUpdate_Write();
  									    }
 								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);
									
										tt_INT=1;
									}		
								}
							}
						}							
									
						if(FOLatch_C_TAD&0xff00)
						{
							ModuleStatus_INTTemp=Module_TotalStatusRead(Com_C2_PluginUnit_14,1);     
							
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_Fault_Mask;
							if(ModuleStatus_INTTemp_2==Module_SVG_H_Mask)
							{
								FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
								FF_Module_50Hz_H=1;
							}
							else
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}

								if(FF_Module_OverVolt==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
//										FF_Module_OverVolt=1;
									}
								}
								
								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}
								
								if(FF_Module_50Hz_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
										FF_Module_50Hz_H=1;
									}
								}
								
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	

								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									if(FF_Module_OverVolt==0)
									{
										ControlFLAG.Bbits.Port_FaultDispose=0;
										//Port shutdown
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
																		
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//??????????????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;  //??????????????????????????????????????????=0
										PortRunFLAG.Bbits.Stop_AllPort=1; 		
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
									}
								}			
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										ControlFLAG.Bbits.Port_FaultDispose=0;
										//DAB PWM Pluse Disable
  									ModuleFault_DisposeTimes=0;			
										if((Port_Main_WorkMode==0x03)&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(flag_FaultClose==1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????????????=0
											Port_StateUpdate_Write();
  									    }
 								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);
									
										tt_INT=1;
									}		
								}
							}
						}			
						
						if(tt_INT==0)
						{
							ControlFLAG.Bbits.Port_FaultDispose=0;	
						}					
						if((tt_INT==0)&&(FF_Module_50Hz_H==1)&&(flag_FaultClose==1))  //只有SVG H桥故障
						{
							ModuleFault_DisposeTimes=0;
							//Clear Latch Register
							ClearTimes=2;					
							FaultLatch_Clear(0,0);                  //清除锁存						
						}							
					}
				}break;
				case 2:                               //HVDC
				{
					FF_Module_DAB_H=0;
					FF_Module_OverVolt=0;
					FF_Module_OverCurrent=0;
					FF_Module_OverTemp=0;
					FF_Module_OverVoltLevel2=0;	
					tt_INT=0;
					
					if(ModuleFault_DisposeTimes==3)     
					{						
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							ModuleIndex_PWMPulseDisable=0;
							FaultDisposeFLAG.Bbits.Port_StopType=0;

							if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))             //主电源，整个系统停机
							{
								FaultDisposeFLAG.Bbits.Port_StopAll=1;	
								PortRunFLAG.Bbits.Stop_AllPort=1; 		//??????????????????????????????????????????=0
								Port_StateUpdate_Write();
  						}
						PWMPluse_Disable(0);
						ModuleFault_DisposeTimes--;
					}
					else
					{					
						ModuleFault_DisposeTimes--;
						
						ModuleStatus_INTTemp=Module_TotalStatusRead(Com_A1_PluginUnit_5,1);     
						ModuleStatus_INTTemp_1=Module_TotalStatusRead(Com_A2_PluginUnit_6,1);   //读模块状态
						ModuleStatus_INTTemp_2=0;
						for(i=0;i<16;i++)
						{
							if((FOLatch_A_TAD>>i)&0x0001)
							{
								ModuleStatus_INTTemp_2++;              //报故障的模块数
								ModuleIndex_PWMPulseDisable=i+1;       //记录模块编号
							}
						}
/*						
						if(ModuleStatus_INTTemp_2==1)              //单个模块
						{									
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp;
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_2&Module_HD_Fault_Mask;
							ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_1;
							ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_3&Module_HD_Fault_Mask;
						
							if(((ModuleStatus_INTTemp_2==Module_DAB_H_Mask)&&(ModuleStatus_INTTemp_3==0))|| \
							   ((ModuleStatus_INTTemp_3==Module_DAB_H_Mask)&&(ModuleStatus_INTTemp_2==0))&&(!FaultTypeFLAG.Bbits.Module_Single))//仅DAB H桥故障
							{

								FaultDisposeFLAG.Bbits.FaultDisposeType=1;
								FaultDisposeFLAG.Bbits.PWMPulseState=1;
								FaultDisposeFLAG.Bbits.Port_StopType=0;							
								PWMPluse_Disable(ModuleIndex_PWMPulseDisable);   //Disable Module PWM Pluse
								ModuleFault_DisposeTimes=0;
								//Clear Latch Register
								FaultLatch_Clear(0,0);
								FaultTypeFLAG.Bbits.Module_Single=1;		
							}
						}						
						else
*/
						{														
							if(FOLatch_A_TAD&0x00ff)
							{
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;   //过流
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverCurrent=1;			
									FF_Module_OverCurrent=1;
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverVolt=1;	                        //过压									
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;      //过温
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverTemp=1;	
									FF_Module_OverTemp=1;							
								}
  													
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;   //DAB H桥故障
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_DAB_H=1;	
									FF_Module_DAB_H=1;
								}		
								               
								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									//Port shutdown
									ControlFLAG.Bbits.Port_FaultDispose=0;
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
																		
										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&(PortRunFLAG.Bbits.PWMState_Dab==0)&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&(PortRunFLAG.Bbits.PWMState_Dab==0)&&(ControlFLAG.Bbits.Port_Start!=1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
#ifdef FaultOutTest
		Flag_DC_FAULT=3;
		Flag_DC_FAULTport=0;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;	
#endif
								}
								else
								{
									if(FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H)										
									{
										//DAB PWM Pluse Disable
  									ModuleFault_DisposeTimes=0;	
  									ControlFLAG.Bbits.Port_FaultDispose=0;		

										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&(PortRunFLAG.Bbits.PWMState_Dab==0)&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;

										PWMPluse_Disable(0);								                         //闭锁DAB的H桥
										
										tt_INT=1;
#ifdef FaultOutTest
		Flag_DC_FAULT=3;
		Flag_DC_FAULTport=1;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;	
#endif
									}		
								}
							}
					
							if(FOLatch_A_TAD&0xff00)
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}

								if(FF_Module_OverVolt==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
//										FF_Module_OverVolt=1;
									}
								}

								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}
								
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	
							
								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									if(FF_Module_OverVolt==0)
									{
										//Port shutdown
										ControlFLAG.Bbits.Port_FaultDispose=0;
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
																		
										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&(PortRunFLAG.Bbits.PWMState_Dab==0)&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&(PortRunFLAG.Bbits.PWMState_Dab==0)&&(ControlFLAG.Bbits.Port_Start!=1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		  //????????????????????????????????=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
#ifdef FaultOutTest
		Flag_DC_FAULT=4;
		Flag_DC_FAULTport=0;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;	
#endif
									}
								}			
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H)&&(tt_INT==0))
									{
										//DAB PWM Pluse Disable
  									ModuleFault_DisposeTimes=0;	
  									ControlFLAG.Bbits.Port_FaultDispose=0;		

										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&(PortRunFLAG.Bbits.PWMState_Dab==0)&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								  								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;

										PWMPluse_Disable(0);
									
										tt_INT=1;
#ifdef FaultOutTest
		Flag_DC_FAULT=4;
		Flag_DC_FAULTport=1;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;	
#endif
									}		
								}	
							}
							
							if(tt_INT==0)
							{
								ControlFLAG.Bbits.Port_FaultDispose=0;
							}
						}	
								
					}								
				}break;
				case 3:                               //LVDC
				{
					FF_Module_50Hz_H=0;
					FF_Module_DAB_H=0;
					FF_Module_OverVolt=0;
					FF_Module_OverCurrent=0;
					FF_Module_OverTemp=0;
					FF_Module_OverVoltLevel2=0;	
					tt_INT=0;


					
					if(ModuleFault_DisposeTimes==3)     
					{	
#ifdef FaultOutTest
		Flag_DC_FAULT=2;
		Flag_DC_FAULTport=0;
		Flag_DC_FAULTDISPOSE=2;		
#endif										
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							ModuleIndex_PWMPulseDisable=0;
							FaultDisposeFLAG.Bbits.Port_StopType=0;

							if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))             //主电源，整个系统停机
							{
								FaultDisposeFLAG.Bbits.Port_StopAll=1;	
								PortRunFLAG.Bbits.Stop_AllPort=1; 		     //??????????????????????????????????????????=0
								Port_StateUpdate_Write();
  						    }
						PWMPluse_Disable(0);

						ModuleFault_DisposeTimes--;

					}
					else
					{					
						ModuleFault_DisposeTimes--;
						
						ModuleStatus_INTTemp=Module_TotalStatusRead(Com_A1_PluginUnit_5,1);     
						ModuleStatus_INTTemp_1=Module_TotalStatusRead(Com_A2_PluginUnit_6,1);   //读模块状态
						ModuleStatus_INTTemp_2=0;
						for(i=0;i<16;i++)
						{
							if((FOLatch_A_TAD>>i)&0x0001)
							{
								ModuleStatus_INTTemp_2++;                //报故障的模块数
								ModuleIndex_PWMPulseDisable=i+1;         //记录模块编号
							}
						}
/*						
						if(ModuleStatus_INTTemp_2==1)              //单个模块
						{									
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp;
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_2&Module_HD_Fault_Mask;
							ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_1;
							ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_3&Module_HD_Fault_Mask;
						
							if(((ModuleStatus_INTTemp_2==Module_DAB_H_Mask)&&(ModuleStatus_INTTemp_3==0))|| \
							   ((ModuleStatus_INTTemp_3==Module_DAB_H_Mask)&&(ModuleStatus_INTTemp_2==0))&&(!FaultTypeFLAG.Bbits.Module_Single))//仅DAB H桥故障
							{

								FaultDisposeFLAG.Bbits.FaultDisposeType=1;
								FaultDisposeFLAG.Bbits.PWMPulseState=1;
								FaultDisposeFLAG.Bbits.Port_StopType=0;							
								PWMPluse_Disable(ModuleIndex_PWMPulseDisable);   //Disable Module PWM Pluse
								ModuleFault_DisposeTimes=0;
								//Clear Latch Register
								FaultLatch_Clear(0,0);
								FaultTypeFLAG.Bbits.Module_Single=1;		
							}
						}						
						else
*/
						{														
							if(FOLatch_A_TAD&0x00ff)
							{
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;   //过流
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverCurrent=1;			
									FF_Module_OverCurrent=1;
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverVolt=1;	                        //过压									
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;      //过温
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverTemp=1;	
									FF_Module_OverTemp=1;							
								}
  													
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;   //DAB H桥故障
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_DAB_H=1;	
									FF_Module_DAB_H=1;
								}		
								
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;   //SVG H桥故障
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
									FF_Module_50Hz_H=1;
								}

								               
								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{										
									//Port shutdown
									ControlFLAG.Bbits.Port_FaultDispose=0;
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
																		
										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
									
#ifdef FaultOutTest
		Flag_DC_FAULT=3;
		Flag_DC_FAULTport=0;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;		
#endif									
								}
								else
								{
									if(FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)										
									{										
										//DAB PWM Pluse Disable
  									ModuleFault_DisposeTimes=0;	
  									ControlFLAG.Bbits.Port_FaultDispose=0;		

										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//???????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);								                         //闭锁DAB的H桥
										
										tt_INT=1;
#ifdef FaultOutTest
		Flag_DC_FAULT=3;
		Flag_DC_FAULTport=1;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;				
#endif	
									}		
								}
							}
					
							if(FOLatch_A_TAD&0xff00)
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}

								if(FF_Module_OverVolt==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
//										FF_Module_OverVolt=1;
									}
								}

								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}

								if(FF_Module_50Hz_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
										FF_Module_50Hz_H=1;
									}
								}
																
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	
							
								if(FaultTypeFLAG.Bbits.Module_OverVolt)                          //过压，直接闭锁端口，停机
								{
									if(FF_Module_OverVolt==0)
									{
										//Port shutdown
										ControlFLAG.Bbits.Port_FaultDispose=0;
  								ModuleFault_DisposeTimes=0;			
#ifdef OV_Stop
									if(ControlFLAG.Bbits.Port_Start==1)  //启动过程中
									{
										ControlFLAG.Bbits.Port_Start=0x03;			
										ControlFLAG.Bbits.SVG_Start=0;
										ControlFLAG.Bbits.SVG_StartOver=0;
										Flag_SVG1s=0;
										coutSVG1s=0;
						
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										
										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
									}
									else
									{									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										Port_RunStatus=PortStatusNum_Stop;	

										ControlFLAG.Bbits.Port_Stop=0x01;
										ControlFLAG.Bbits.Port_Start=0x00;	
																		
										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								}												
#else								
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;
									
									if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;
										PortRunFLAG.Bbits.Stop_AllPort=1; 		  //????????????????????????????????=0
										Port_StateUpdate_Write();
  								}
#endif
									FaultDisposeFLAG.Bbits.PWMPulseState=2;
									ModuleIndex_PWMPulseDisable=0;
									PWMPluse_Disable(0);								                         
								
									FF_Module_OverVolt=1;
									tt_INT=1;
#ifdef FaultOutTest
		Flag_DC_FAULT=4;
		Flag_DC_FAULTport=0;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;		
#endif
									}
								}			
								else
								{
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										//DAB PWM Pluse Disable
  									ModuleFault_DisposeTimes=0;	
  									ControlFLAG.Bbits.Port_FaultDispose=0;		

										if(((Port_Main_WorkMode==0x03)||(Flag_DC_VoltSource==1))&&((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1))
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;
											PortRunFLAG.Bbits.Stop_AllPort=1; 		//????????????????????????????????=0
											Port_StateUpdate_Write();
  									}
  								  								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=2;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										//edited by shiqi
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=2;
										Port_RunStatus=PortStatusNum_Stop;
										PWMPluse_Disable(0);
									
										tt_INT=1;

#ifdef FaultOutTest
		Flag_DC_FAULT=4;
		Flag_DC_FAULTport=1;
		Flag_DC_FAULTDISPOSE=FaultTypeFLAG.FaultTypeFLAG16;
		HWREGH(0x6000082a)=Flag_DC_FAULT;
		HWREGH(0x6000082c)=Flag_DC_FAULTport;
		HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;	
#endif
									}		
								}	
							}
							
							if(tt_INT==0)
							{
								ControlFLAG.Bbits.Port_FaultDispose=0;
							}
						}	
								
					}								
				}break;
				
/*				
				case 3:
				{
					FF_Module_50Hz_H=0;
					FF_Module_DAB_H=0;
					FF_Module_OverVolt=0;
					FF_Module_OverCurrent=0;
					FF_Module_OverTemp=0;
					FF_Module_OverVoltLevel2=0;	
					tt_INT=0;
					
					if(ModuleFault_DisposeTimes==3)     
					{						
						ModuleFault_DisposeTimes--;
					}
					else
					{
						ModuleFault_DisposeTimes--;
						
						ModuleStatus_INTTemp=Module_TotalStatusRead(Com_A1_PluginUnit_5,1);     
						ModuleStatus_INTTemp_1=Module_TotalStatusRead(Com_A2_PluginUnit_6,1);   
						ModuleStatus_INTTemp_2=0;
						for(i=0;i<16;i++)
						{
							if((FOLatch_A_TAD>>i)&0x0001)
							{
								ModuleStatus_INTTemp_2++;
								ModuleIndex_PWMPulseDisable=i+1;
							}
						}
						
						if(ModuleStatus_INTTemp_2==1)
						{									
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp;
							ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_2&Module_LD_Fault_Mask;
							ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_1;
							ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_3&Module_LD_Fault_Mask;

						
							if((((ModuleStatus_INTTemp_2&Module_DABSVG_LD_Mask)==0)&&(ModuleStatus_INTTemp_3==0))|| \
							   (((ModuleStatus_INTTemp_3&Module_DABSVG_LD_Mask)==0)&&(ModuleStatus_INTTemp_2==0))&&(!FaultTypeFLAG.Bbits.Module_Single))
							{
								FaultDisposeFLAG.Bbits.FaultDisposeType=1;
								FaultDisposeFLAG.Bbits.PWMPulseState=1;
								FaultDisposeFLAG.Bbits.Port_StopType=0;										
								PWMPluse_Disable(ModuleIndex_PWMPulseDisable);   //Disable Module PWM Pluse
								ModuleFault_DisposeTimes=0;
								//Clear Latch Register
								FaultLatch_Clear(0,0);
								FaultTypeFLAG.Bbits.Module_Single=1;
							}
						}
						else
						{
							if(FOLatch_A_TAD&0x00ff)
							{
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC1_L_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverCurrent=1;			
									FF_Module_OverCurrent=1;
								}
								
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverCurrent=1;			
									FF_Module_OverCurrent=1;
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverVolt=1;	
									FF_Module_OverVolt=1;
								
									for(i=0;i<8;i++)
									{
										if((FOLatch_A_TAD>>i)&0x0001)
										{
											DCVolt_INTTemp=FPGA_Interface[Com_A1_PluginUnit_5-2].DC_LATCH[i];
										
											if(DCVolt_INTTemp>OverVolt_AlarmLevel2)
											{
												FaultTypeFLAG.Bbits.Module_OverVoltLevel2=1;
												FF_Module_OverVoltLevel2=1;
												break;
											}
										}
									}		
								}
  						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverTemp=1;	
									FF_Module_OverTemp=1;							
								}

								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
									FF_Module_50Hz_H=1;
								}
								  													
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_DAB_H=1;	
									FF_Module_DAB_H=1;
								}		
							
								if(FaultTypeFLAG.Bbits.Module_OverVoltLevel2)
								{
									//Port shutdown
  								ModuleFault_DisposeTimes=0;			
  								
									FaultDisposeFLAG.Bbits.FaultDisposeType=2;
									FaultDisposeFLAG.Bbits.Port_StopType=1;
									if(Port_Main_WorkMode==0x03)
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  								}
  								else
  								{
  									FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  								} 
  								
  								FaultDisposeFLAG.Bbits.PWMPulseState=3;
  								ModuleIndex_PWMPulseDisable=0;
  								
									PWMPluse_Disable(0);
									Port_RunStatus=PortStatusNum_Stop;	
									ControlFLAG.Bbits.Port_Stop=1;
									ControlFLAG.Bbits.Port_Start=0;
									
								}
								else
								{
									if(FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_OverVolt||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)
									{
										//DAB PWM Pluse Disable
  									ModuleFault_DisposeTimes=0;			
										if(Port_Main_WorkMode==0x03)
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  									}
  									else
  									{
  										FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  									}   								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=3;
										ModuleIndex_PWMPulseDisable=0;
										FaultDisposeFLAG.Bbits.Port_StopType=0;
										
										PWMPluse_Disable(0);								
										
										tt_INT=1;
									}		
								}
							}
					
							if(FOLatch_A_TAD&0xff00)
							{
								if(FF_Module_OverCurrent==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OC1_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
									
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
										FF_Module_OverCurrent=1;	
									}
								}
						
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OV_L_Mask;
								if(ModuleStatus_INTTemp_2)
								{
									FaultTypeFLAG.Bbits.Module_OverVolt=1;	
									FF_Module_OverVolt=1;
									
									if(FF_Module_OverVoltLevel2==0)
									{
										for(i=0;i<8;i++)
										{
											if((FOLatch_A_TAD>>(i+8))&0x0001)
											{
												DCVolt_INTTemp=FPGA_Interface[Com_A2_PluginUnit_6-2].DC_LATCH[i];
										
												if(DCVolt_INTTemp>OverVolt_AlarmLevel2)
												{
													FaultTypeFLAG.Bbits.Module_OverVoltLevel2=1;
													//FF_Module_OverVoltLevel2=1;	
													break;
												}
											}
										}		
									}
								}
            
								if(FF_Module_OverTemp==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;
									}
								}
								
								if(FF_Module_50Hz_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_SVG_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_50Hz_H=1;	
										FF_Module_50Hz_H=1;
									}
								}
																
								if(FF_Module_DAB_H==0)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}	
								}	
							
								if((FaultTypeFLAG.Bbits.Module_OverVoltLevel2)&&(FF_Module_OverVoltLevel2==0))
								{
									//Port shutdown
  								ModuleFault_DisposeTimes=0;			
  								
									FaultDisposeFLAG.Bbits.FaultDisposeType=2;
									FaultDisposeFLAG.Bbits.Port_StopType=1;
									if(Port_Main_WorkMode==0x03)
									{
										FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  								}
  								else
  								{
  									FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  								} 
  								
  								FaultDisposeFLAG.Bbits.PWMPulseState=3;
  								ModuleIndex_PWMPulseDisable=0;
  								
									PWMPluse_Disable(0);
									Port_RunStatus=PortStatusNum_Stop;	
									ControlFLAG.Bbits.Port_Stop=1;
									ControlFLAG.Bbits.Port_Start=0;
									
									FF_Module_OverVoltLevel2=1;
								}
								else
								{
									ModuleFault_DisposeTimes=0;
									if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_OverVolt||FaultTypeFLAG.Bbits.Module_DAB_H||FaultTypeFLAG.Bbits.Module_50Hz_H)&&(tt_INT==0))
									{
										//DAB PWM Pluse Disable
  									ModuleFault_DisposeTimes=0;			
										if(Port_Main_WorkMode==0x03)
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  									}
  									else
  									{
  										FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  									}   								
										FaultDisposeFLAG.Bbits.FaultDisposeType=1;
										FaultDisposeFLAG.Bbits.PWMPulseState=3;
										ModuleIndex_PWMPulseDisable=0;

										PWMPluse_Disable(0);
									
										tt_INT=1;
									}		
								}
							}
						}		
					}						
				}break;
				case 4:
				{											
					if(FOLatch_A_TAD)
					{
						FF_Module_DAB_H=0;
						FF_Module_OverVolt=0;
						FF_Module_OverCurrent=0;
						FF_Module_OverTemp=0;
						FF_Module_OverVoltLevel2=0;	
						tt_INT=0;
						
						if(ModuleFault_DisposeTimes==3)     
						{						
							ModuleFault_DisposeTimes--;
						}
						else
						{	
							ModuleFault_DisposeTimes--;
							ModuleStatus_INTTemp=Module_TotalStatusRead(Com_A1_PluginUnit_5,1);     
							ModuleStatus_INTTemp_1=Module_TotalStatusRead(Com_A2_PluginUnit_6,1);   
							ModuleStatus_INTTemp_2=0;
							for(i=0;i<16;i++)
							{
								if((FOLatch_A_TAD>>i)&0x0001)
								{
									ModuleStatus_INTTemp_2++;
									ModuleIndex_PWMPulseDisable=i+1;
								}
							}
						
							if(ModuleStatus_INTTemp_2==1)
							{									
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp;
								ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_2&Module_HD_Fault_Mask;
								ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_1;
								ModuleStatus_INTTemp_3=ModuleStatus_INTTemp_3&Module_HD_Fault_Mask;
						
								if(((ModuleStatus_INTTemp_2==Module_DAB_H_Mask)&&(ModuleStatus_INTTemp_3==0))|| \
							   	((ModuleStatus_INTTemp_3==Module_DAB_H_Mask)&&(ModuleStatus_INTTemp_2==0))&&(!FaultTypeFLAG.Bbits.Module_Single))
								{							
									FaultDisposeFLAG.Bbits.FaultDisposeType=1;
									FaultDisposeFLAG.Bbits.PWMPulseState=1;
									FaultDisposeFLAG.Bbits.Port_StopType=0;		
									PWMPluse_Disable(ModuleIndex_PWMPulseDisable);   //Disable Module PWM Pluse
									ModuleFault_DisposeTimes=0;
									//Clear Latch Register
									FaultLatch_Clear(0,0);
									FaultTypeFLAG.Bbits.Module_Single=1;
								}
							}
							else
							{
								if(FOLatch_A_TAD&0x00ff)
								{
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OC2_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverCurrent=1;			
										FF_Module_OverCurrent=1;
									}
  							
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
										FF_Module_OverVolt=1;
									
										for(i=0;i<8;i++)
										{
											if((FOLatch_A_TAD>>i)&0x0001)
											{
												DCVolt_INTTemp=FPGA_Interface[Com_A1_PluginUnit_5-2].DC_LATCH[i];
											
												if(DCVolt_INTTemp>OverVolt_AlarmLevel2)
												{
													FaultTypeFLAG.Bbits.Module_OverVoltLevel2=1;
													FF_Module_OverVoltLevel2=1;
													break;
												}
											}
										}		
									}
  							
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_OT_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverTemp=1;	
										FF_Module_OverTemp=1;							
									}
  														
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp&Module_DAB_H_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_DAB_H=1;	
										FF_Module_DAB_H=1;
									}		
								
									if(FaultTypeFLAG.Bbits.Module_OverVoltLevel2)
									{
										//Port shutdown
  									ModuleFault_DisposeTimes=0;			
  									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										if(Port_Main_WorkMode==0x03)
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  									}
  									else
  									{
  										FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  									}   
  									
  									FaultDisposeFLAG.Bbits.PWMPulseState=3;
  									ModuleIndex_PWMPulseDisable=0;
  									
										PWMPluse_Disable(0);
										Port_RunStatus=PortStatusNum_Stop;	
										ControlFLAG.Bbits.Port_Stop=1;
										ControlFLAG.Bbits.Port_Start=0;
										
									}
									else
									{
										if(FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_OverVolt||FaultTypeFLAG.Bbits.Module_DAB_H)
										{
											//DAB PWM Pluse Disable
  										ModuleFault_DisposeTimes=0;			
											if(Port_Main_WorkMode==0x03)
											{
												FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  										}
  										else
  										{
  											FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  										}   								
											FaultDisposeFLAG.Bbits.FaultDisposeType=1;
											FaultDisposeFLAG.Bbits.PWMPulseState=2;
											ModuleIndex_PWMPulseDisable=0;
											FaultDisposeFLAG.Bbits.Port_StopType=0;
        		
											PWMPluse_Disable(0);								
											
											if(!FaultTypeFLAG.Bbits.Module_50Hz_H)
											{
												//Clear Latch Register
												FaultLatch_Clear(0,0);
											}
											
											tt_INT=1;
										}		
									}
								}
						
								if(FOLatch_A_TAD&0xff00)
								{
									if(FF_Module_OverCurrent==0)
									{
										ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OC2_L_Mask;
										if(ModuleStatus_INTTemp_2)
										{
											FaultTypeFLAG.Bbits.Module_OverCurrent=1;		
											FF_Module_OverCurrent=1;	
										}
									}
							
									ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OV_L_Mask;
									if(ModuleStatus_INTTemp_2)
									{
										FaultTypeFLAG.Bbits.Module_OverVolt=1;	
										FF_Module_OverVolt=1;
										
										if(FF_Module_OverVoltLevel2==0)
										{
											for(i=0;i<8;i++)
											{
												if((FOLatch_A_TAD>>(i+8))&0x0001)
												{
													DCVolt_INTTemp=FPGA_Interface[Com_A2_PluginUnit_6-2].DC_LATCH[i];
											
													if(DCVolt_INTTemp>OverVolt_AlarmLevel2)
													{
														FaultTypeFLAG.Bbits.Module_OverVoltLevel2=1;
													//	FF_Module_OverVoltLevel2=1;	
														break;
													}
												}
											}		
										}
									}
        		  
									if(FF_Module_OverTemp==0)
									{
										ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_OT_Mask;
										if(ModuleStatus_INTTemp_2)
										{
											FaultTypeFLAG.Bbits.Module_OverTemp=1;	
											FF_Module_OverTemp=1;
										}
									}
									
									if(FF_Module_DAB_H==0)
									{
										ModuleStatus_INTTemp_2=ModuleStatus_INTTemp_1&Module_DAB_H_Mask;
										if(ModuleStatus_INTTemp_2)
										{
											FaultTypeFLAG.Bbits.Module_DAB_H=1;	
											FF_Module_DAB_H=1;
										}	
									}	
								
									if((FaultTypeFLAG.Bbits.Module_OverVoltLevel2)&&(FF_Module_OverVoltLevel2==0))
									{
										//Port shutdown
  									ModuleFault_DisposeTimes=0;			
  									
										FaultDisposeFLAG.Bbits.FaultDisposeType=2;
										FaultDisposeFLAG.Bbits.Port_StopType=1;
										if(Port_Main_WorkMode==0x03)
										{
											FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  									}
  									else
  									{
  										FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  									} 
  									
  									FaultDisposeFLAG.Bbits.PWMPulseState=3;
  									ModuleIndex_PWMPulseDisable=0;
  									
										PWMPluse_Disable(0);
										Port_RunStatus=PortStatusNum_Stop;	
										ControlFLAG.Bbits.Port_Stop=1;
										ControlFLAG.Bbits.Port_Start=0;
										
										FF_Module_OverVoltLevel2=1;
										
									}
									else
									{
										ModuleFault_DisposeTimes=0;	
										if((FaultTypeFLAG.Bbits.Module_OverCurrent||FaultTypeFLAG.Bbits.Module_OverTemp||FaultTypeFLAG.Bbits.Module_OverVolt||FaultTypeFLAG.Bbits.Module_DAB_H)&&(tt_INT==0))
										{
											//DAB PWM Pluse Disable
  										ModuleFault_DisposeTimes=0;			
											if(Port_Main_WorkMode==0x03)
											{
												FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1; 		
  										}
  										else
  										{
  											FaultDisposeFLAG.Bbits.Port_StopAll=0;  
  										}   								
											FaultDisposeFLAG.Bbits.FaultDisposeType=1;
											FaultDisposeFLAG.Bbits.PWMPulseState=2;
											ModuleIndex_PWMPulseDisable=0;
											FaultDisposeFLAG.Bbits.Port_StopType=0;
        		
											PWMPluse_Disable(0);
        	
											if(!FaultTypeFLAG.Bbits.Module_50Hz_H)
											{
												//Clear Latch Register
												FaultLatch_Clear(0,0);
											}										
											tt_INT=1;
										}		
									}
								}
							}		
						}	
					}				
				}break;
*/
				default:break;
			}
		}
	}
}

/****************************************************************************/
/*                                                                          */
/*                      Fault Latch Clear                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 发起或推进原两拍故障锁存清除流程
 * 输入参数: runindex：原故障锁存清除时序的执行/初始化选择值，沿用现有调用点
 *           RstFlagFault：旧故障复位阶段选择
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void FaultLatch_Clear(uchar runindex,uchar RstFlagFault)
{	
	/*
	 * 故障锁存复位分两步调用：
	 * runindex=0：拉起 UNIT 硬件复位和主 FPGA FB 复位，开始清故障；
	 * runindex=1：释放 UNIT 复位，清软件故障处理状态，并按 RstFlagFault
	 *              决定是否清除对外故障字。
	 * 末尾三个地址是原程序给 FPGA 的调试/故障通知寄存器。
	 */
	if(runindex==0)
	{
		Flag_Clear_zhi=RstFlagFault;
		Flag_Clear_0=1;
		Module_HW_RST(1);
		Port_DSPControl_FBRST(1);      //REST FB in main fpga
	}
	else if(runindex==1)
	{
		Module_HW_RST(0);
		
		Port_BKU_Send(0);
		Flag_Clear_0=0;
		Flag_Clear_1=0;

#ifdef COM_TEST_PluginUnit
		WrongTimes_LEFT=0;      
		WrongTimes_RIGHT=0;     

		WrongTimes_LEFT_A=0;      
		WrongTimes_RIGHT_A=0;  

		WrongTimes_Reference_A1=0;
		WrongTimes_Reference_A2=0;
	
		WrongTimes_Reference_C1=0;
		WrongTimes_Reference_C2=0;		

		WrongTimes_ControlReference_A=0;
		WrongTimes_ControlReference_C=0;		
#endif
				
//	Port_DSPControl_FBRST(1);      //REST FB in main fpga
		
		ControlFLAG.Bbits.Port_FaultDispose=0;
		FaultDisposeFLAG.Bbits.FaultDisposeType=0;
		FaultDisposeFLAG.Bbits.PWMPulseState=0;
		ModuleIndex_PWMPulseDisable=0;
		FaultDisposeFLAG.Bbits.PWMPulseDisableNum=0;
		FaultDisposeFLAG.Bbits.Port_StopType=0;
		FaultDisposeFLAG.Bbits.Port_StopAll=0;
		PortRunFLAG.Bbits.Stop_AllPort=0;
		Port_StateUpdate_Write();
		
		if(RstFlagFault)
		{
			Port_RealFault.PORT_FAULT16=0;
			FaultTypeFLAG.FaultTypeFLAG16=0;
			//edited by shiqi, start
#ifdef LVDC_online
			OC_OV_LV_PortLatch_TAD = 0;
			LVDC_OC_Fault = 0;
			LVDC_OV_Fault = 0;
#endif

#ifdef HVAC_online
			OC_OV_LV_PortLatch_TAD = 0;
			HVAC_OC_Fault = 0;
			HVAC_OV_Fault = 0;
#endif
			//edited by shiqi, end
		}
		Port_DSPControl_FBRST(0);   //active FB in main fpga
	}
	
	Flag_DC_FAULT=0;
	Flag_DC_FAULTport=0;
	Flag_DC_FAULTDISPOSE=0;	
	HWREGH(0x6000082a)=Flag_DC_FAULT;
	HWREGH(0x6000082c)=Flag_DC_FAULTport;
	HWREGH(0x6000082e)=Flag_DC_FAULTDISPOSE;	
}
