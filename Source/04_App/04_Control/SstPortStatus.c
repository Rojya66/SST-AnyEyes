#include "SstControlConfig.h"
//###########################################################################
// FILE: SstPortStatus.c
// FUNCTION: 活动端口状态解码、状态回写和全端口停机标志复位。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#include "ControlData.h"
#include "DpramHostData.h"
#include "FpgaRegister.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SoftTimeData.h"
#include "SstBoardConfig.h"
#include "SstTypes.h"

#include "SstPortStatus.h"
#include "FpgaIo.h"
#include "gpio.h"
#include "hw_types.h"

/*==================================================================
 * 函数功能: 读取并解码端口主状态和运行模式
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Port_StateUpdate_Read(void)
{		
	PortController_Communication(1);     //读其它端口信息
	
//	if(Flag_ComTest)
	{
		HWREGH(0x60000820)=0XEBEB;
		HWREGH(0x60000822)=MainStatus_HVAC.MainStatus16;
		HWREGH(0x60000824)=MainStatus_HVDC.MainStatus16;
		HWREGH(0x60000826)=MainStatus_LVDC.MainStatus16;
		HWREGH(0x60000828)=MainStatus_LVAC.MainStatus16;
	}
	
	if((PORT_Index==2)||(PORT_Index==3))
	{
		if((MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_PluseClose))
		{
			Flag_DC_VoltSource=0;
			HWREGH(0x60000626)=0; 
		}
	}
	
			
	//if(((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))&&(ControlFLAG.Bbits.Port_Start!=1)&&(!PortRunFLAG.Bbits.Stop_AllPort))
	if((ControlFLAG.Bbits.Port_Start!=1)&&(!PortRunFLAG.Bbits.Stop_AllPort))
	{
		if(PORT_Index==1)
		{
			if(((Port_OnState&0x0004)&&MainStatus_HVDC.Bbits.Sys_Stop)||((Port_OnState&0x0002)&&MainStatus_LVDC.Bbits.Sys_Stop)||((Port_OnState&0x0001)&&MainStatus_LVAC.Bbits.Sys_Stop))//要求整机停机
			{
				ControlFLAG.Bbits.Port_Stop=0x01;
				ControlFLAG.Bbits.Port_Start=0x00;
				Port_RunStatus=PortStatusNum_Stop;

				FaultDisposeFLAG.Bbits.FaultDisposeType=2;
				FaultDisposeFLAG.Bbits.PWMPulseState=3;
				FaultDisposeFLAG.Bbits.Port_StopType=2;    //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown		
				
				PortRunFLAG.Bbits.Stop_AllPort=1;		
			}
		}
		else if(PORT_Index==2)
		{
			if(((Port_OnState&0x0008)&&MainStatus_HVAC.Bbits.Sys_Stop)||((Port_OnState&0x0002)&&MainStatus_LVDC.Bbits.Sys_Stop)||((Port_OnState&0x0001)&&MainStatus_LVAC.Bbits.Sys_Stop))
			{
				ControlFLAG.Bbits.Port_Stop=0x01;
				ControlFLAG.Bbits.Port_Start=0x00;
				Port_RunStatus=PortStatusNum_Stop;

				FaultDisposeFLAG.Bbits.FaultDisposeType=2;
				FaultDisposeFLAG.Bbits.PWMPulseState=3;
				FaultDisposeFLAG.Bbits.Port_StopType=2;    //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown
				
				PortRunFLAG.Bbits.Stop_AllPort=1;
			}
		}
		else if(PORT_Index==3)
		{
			if(((Port_OnState&0x0008)&&MainStatus_HVAC.Bbits.Sys_Stop)||((Port_OnState&0x0004)&&MainStatus_HVDC.Bbits.Sys_Stop)||((Port_OnState&0x0001)&&MainStatus_LVAC.Bbits.Sys_Stop))
			{
				ControlFLAG.Bbits.Port_Stop=0x01;
				ControlFLAG.Bbits.Port_Start=0x00;
				Port_RunStatus=PortStatusNum_Stop;

				FaultDisposeFLAG.Bbits.FaultDisposeType=2;
				FaultDisposeFLAG.Bbits.PWMPulseState=3;
				FaultDisposeFLAG.Bbits.Port_StopType=2;    //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown	
				
				PortRunFLAG.Bbits.Stop_AllPort=1;
			}
		}
		else if(PORT_Index==4)
		{
			if(((Port_OnState&0x0008)&&MainStatus_HVAC.Bbits.Sys_Stop)||((Port_OnState&0x0004)&&MainStatus_HVDC.Bbits.Sys_Stop)||((Port_OnState&0x0002)&&MainStatus_LVDC.Bbits.Sys_Stop))
			{
				ControlFLAG.Bbits.Port_Stop=0x01;
				ControlFLAG.Bbits.Port_Start=0x00;
				Port_RunStatus=PortStatusNum_Stop;

				FaultDisposeFLAG.Bbits.FaultDisposeType=2;
				FaultDisposeFLAG.Bbits.PWMPulseState=3;
				FaultDisposeFLAG.Bbits.Port_StopType=2;    //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown	
				
				PortRunFLAG.Bbits.Stop_AllPort=1;
			}
		}		
		else{}
	}			
}

/*==================================================================
 * 函数功能: 将端口运行、故障及模式编码写回FPGA主控HVAC_STATUS
 * 输入参数: 无
 * 输出参数: Port_RealFault、Port_MainStatus及端口状态镜像；GPIO144、FPGA状态字与LED。
 * 返回值: 无
 * 执行说明: FPGA TAD中断原位置调用，末尾经PortController_Communication(0)提交；不可移至后台。
 *================================================================*/
void Port_StateUpdate_Write(void)
{	
	GPIOPinWrite(SOC_GPIO_0_REGS, 144, 0);

	if(Flag_ComTest)
	{
		Port_MainStatus.MainStatus16=FourPortComZhi;
	}
	else
	{
/*
		//Fault Check??????????????
		//Port_RealFault.Bbits.PORT_FAULT_bit14=1;
		//Port_RealFault.Bbits.PORT_FAULT_bit13=1;
		//Port_RealFault.Bbits.PORT_FAULT_bit3=1;
		//FaultDisposeFLAG.Bbits.PWMPulseState=3;
		//FaultDisposeFLAG.Bbits.PWMEN_FibreOptical=0;
		//PortRunFLAG.Bbits.PWMState_50Hz=1;
		//PortRunFLAG.Bbits.PWMState_Dab=1;
		//PortRunFLAG.Bbits.PWMState_Module=1;
*/	
		Port_RealFault.Bbits.PORT_FAULT_bit3=FaultTypeFLAG.Bbits.Controller;
		Port_RealFault.Bbits.PORT_FAULT_bit6=FaultTypeFLAG.Bbits.Module_Fault;
		Port_RealFault.Bbits.PORT_FAULT_bit7=FaultTypeFLAG.Bbits.Module_50Hz_H;
		Port_RealFault.Bbits.PORT_FAULT_bit10=FaultTypeFLAG.Bbits.Port_OverCurrent;
		Port_RealFault.Bbits.PORT_FAULT_bit11=FaultTypeFLAG.Bbits.Module_LowerVolt;
		Port_RealFault.Bbits.PORT_FAULT_bit12=FaultTypeFLAG.Bbits.Port_OverVolt;
	
		if((Port_RealFault.PORT_FAULT16&0x75FF)==0)    //Fault Flag
		{
			Port_RealFault.Bbits.PORT_FAULT_bit15=0;
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led2=0;
		}
		else
		{
			Port_RealFault.Bbits.PORT_FAULT_bit15=1;
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led2=1;
		}
		//HWREGH(0x60000622)=Port_RealFault.PORT_FAULT16;
		//update port main_status
		Port_MainStatus.Bbits.P_Status=Port_State;
		Port_MainStatus.Bbits.Sys_Stop=FaultDisposeFLAG.Bbits.Port_StopAll;
		Port_MainStatus.Bbits.Outer_Elec=Port_Outer_Elec;
		Port_MainStatus.Bbits.P_S2_Mode=Port_APF_WorkMode;
		Port_MainStatus.Bbits.P_S1_Mode=Port_STATCOM_WorkMode;
		Port_MainStatus.Bbits.P_M_Mode=Port_Main_WorkMode;
	}
	
	if(PORT_Index==1)
	{
		MainStatus_HVAC.MainStatus16=Port_MainStatus.MainStatus16;
	}
	else if(PORT_Index==2)
	{
		MainStatus_HVDC.MainStatus16=Port_MainStatus.MainStatus16;
	}
	else if(PORT_Index==3)
	{
		MainStatus_LVDC.MainStatus16=Port_MainStatus.MainStatus16;
	}
	else if(PORT_Index==4)
	{
		MainStatus_LVAC.MainStatus16=Port_MainStatus.MainStatus16;
	}
	else{}	
	
	PortController_Communication(0); 
	
	GPIOPinWrite(SOC_GPIO_0_REGS, 144, 1);			
}

/*==================================================================
 * 函数功能: 按旧全端口停机流程复位相关标志
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Port_AllStopFlag_RST(void)
{	
	uchar tt_allstop; /* tt_allstop：原全端口停机流程的临时状态字。 */
	tt_allstop=1;
	if(Port_Main_WorkMode==0x03)
	{
		if(PORT_Index==1)  //其它端口已经停机
		{
			if((!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0004))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0002))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0001))
			{
				tt_allstop=0;
			}
		}
		else if(PORT_Index==2)
		{
			if((!((MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0008))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0002))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0001))
			{
				tt_allstop=0;
			}
		}
		else if(PORT_Index==3)
		{
			if((!((MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0008))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0004))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0001))
			{
				tt_allstop=0;
			}
		}
		else if(PORT_Index==4)
		{
			if((!((MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0008))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0004))
			{
				tt_allstop=0;
			}
			if((!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(Port_OnState&0x0002))
			{
				tt_allstop=0;
			}
		}	
		else{}		
			
		if(tt_allstop)   //清除端口整机停机标志，并告知其它端口
		{
			PortRunFLAG.Bbits.Stop_AllPort=0;
			FaultDisposeFLAG.Bbits.Port_StopAll=0;
						
			MainStatus_HVAC.Bbits.Sys_Stop=0;
			MainStatus_HVDC.Bbits.Sys_Stop=0;
			MainStatus_LVDC.Bbits.Sys_Stop=0;
			MainStatus_LVAC.Bbits.Sys_Stop=0;
 
		}
	}
/*	
	else
	{
		if(PORT_Index==1)
		{
			if((MainStatus_HVDC.Bbits.Sys_Stop==0)&&(MainStatus_LVDC.Bbits.Sys_Stop==0)&&(MainStatus_LVAC.Bbits.Sys_Stop==0))
			{
				tt_allstop=1;
			}
		}
		else if(PORT_Index==2)
		{
			if((MainStatus_HVAC.Bbits.Sys_Stop==0)&&(MainStatus_LVDC.Bbits.Sys_Stop==0)&&(MainStatus_LVAC.Bbits.Sys_Stop==0))
			{
				tt_allstop=1;
			}
		}
		else if(PORT_Index==3)
		{
			if((MainStatus_HVAC.Bbits.Sys_Stop==0)&&(MainStatus_HVDC.Bbits.Sys_Stop==0)&&(MainStatus_LVAC.Bbits.Sys_Stop==0))
			{
				tt_allstop=1;
			}
		}
		else if(PORT_Index==4)
		{
			if((MainStatus_HVAC.Bbits.Sys_Stop==0)&&(MainStatus_HVDC.Bbits.Sys_Stop==0)&&(MainStatus_LVDC.Bbits.Sys_Stop==0))
			{
				tt_allstop=1;
			}
		}	
		else{}		
			
		if(tt_allstop)
		{
			PortRunFLAG.Bbits.Stop_AllPort=0;
		}		
	}	
*/	
}

