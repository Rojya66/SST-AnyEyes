
//###########################################################################
//
// FILE:   Stateflow.c
//
// FUNCTION:  1)Running state flow 
//            2)Status update of port controller
//            3)Switch operation
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

#include "main.h"
#include "Stateflow.h"
#include "PluginUnit.h"
#include "FaultCheck.h"
#include "MaintenanceProtocol.H"
#include "gpio.h"
#include "hw_types.h" 
/************************Variable definition********************************/

ushort PortPriority_Start[4]; /* PortPriority_Start：原四端口启动优先级表。 */    //Offset 0-3: Priority from high (1) to low (4)
ushort Port_StartPriorityPosition[4]; /* Port_StartPriorityPosition：当前端口在原启动优先级表中的位置。 */
ushort PortPriority_Stop[4]; /* PortPriority_Stop：原四端口停止优先级表。 */ 
ushort Port_StopPriorityPosition[4]; /* Port_StopPriorityPosition：当前端口在原停止优先级表中的位置。 */

/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 检查旧端口启动优先级及条件，判定启动请求是否有效
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void PortStartup_Effectiveness(void);
/*==================================================================
 * 函数功能: 读取并解码端口主状态和运行模式
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Port_StateUpdate_Read(void);
/*==================================================================
 * 函数功能: 将端口运行状态及工作模式编码回状态字
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Port_StateUpdate_Write(void);
/*==================================================================
 * 函数功能: 按旧全端口停机流程复位相关标志
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Port_AllStopFlag_RST(void);
/*==================================================================
 * 函数功能: 检查原四端口状态字是否存在主电源模式且外部有电的端口
 * 输入参数: 无
 * 输出参数: 无；只检查 MainStatus_HVAC/HVDC/LVDC/LVAC 的模式和外部电源位。
 * 返回值: 存在主电源且有电返回 1，否则返回 0
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
ushort Judge_MainPowerSupply(void);
/*==================================================================
 * 函数功能: 推进旧正常运行流程中的启动、停止及开关操作
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void NormalRun_flow(void);
/*==================================================================
 * 函数功能: 按编译端口推进旧手动启动与脉冲使能顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow_Manual(void);
/*==================================================================
 * 函数功能: 按编译端口推进旧自动预充及启动顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow(void);
/*==================================================================
 * 函数功能: 按旧停机类型撤销脉冲并处理开关断开及延时
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Stop_flow(void);
/*==================================================================
 * 函数功能: 建立所选端口的开关操作时序与初始状态
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void SwitchTable_Start(void);
/*==================================================================
 * 函数功能: 根据时序标志执行旧开关操作并判定完成
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void SwitchOperation_Dispose(void);

/****************************************************************************/
/*                                                                          */
/*               the effectiveness of port startup                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 检查旧端口启动优先级及条件，判定启动请求是否有效
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void PortStartup_Effectiveness(void)
{
	uchar PriorityNum_Start; /* PriorityNum_Start：当前查找的启动优先级编号。 */
	uchar PriorityTemp,PriorityPositionTemp; /* PriorityTemp：原端口启动优先级表的临时条目。 */
	/* PriorityPositionTemp：当前端口在启动优先级表中的临时位置。 */
	uchar i; /* i：本次数组、参数或模块遍历的索引。 */
	
	//which port working at the follow mode
	PriorityNum_Start=4;
	if(MainStatus_HVAC.Bbits.P_M_Mode==0x03)
	{	
		for(i=0;i<4;i++)
		{
			PortPriority_Start[i]=i+1;
			Port_StartPriorityPosition[i]=i;            //the position in the priority buffer for this port 
		}			
	}	
	else if(MainStatus_HVDC.Bbits.P_M_Mode==0x03)
	{	
		PortPriority_Start[0]=2;
		PortPriority_Start[1]=1;
		PortPriority_Start[2]=3;
		PortPriority_Start[3]=4;
		Port_StartPriorityPosition[0]=1;     
		Port_StartPriorityPosition[1]=0; 
		Port_StartPriorityPosition[2]=2; 
		Port_StartPriorityPosition[3]=3;        
	}	
	else if(MainStatus_LVDC.Bbits.P_M_Mode==0x03)
	{	
		PortPriority_Start[0]=3;
		PortPriority_Start[1]=1;
		PortPriority_Start[2]=2;
		PortPriority_Start[3]=4;
		Port_StartPriorityPosition[0]=1;     
		Port_StartPriorityPosition[1]=2; 
		Port_StartPriorityPosition[2]=0; 
		Port_StartPriorityPosition[3]=3;   		
	}	
	else if(MainStatus_LVAC.Bbits.P_M_Mode==0x03)
	{	
		PortPriority_Start[0]=4;
		PortPriority_Start[1]=1;
		PortPriority_Start[2]=2;
		PortPriority_Start[3]=3;
		Port_StartPriorityPosition[0]=1;     
		Port_StartPriorityPosition[1]=2; 
		Port_StartPriorityPosition[2]=3; 
		Port_StartPriorityPosition[3]=0;   		
	}			
	else
	{	
		for(i=0;i<4;i++)
		{
			PortPriority_Start[i]=0xff;
			Port_StartPriorityPosition[i]=i;            
		}			
		PriorityNum_Start=0;
	}	
		
	//which port is starting
	if(PriorityNum_Start)
	{	
		if(((MainStatus_HVAC.Bbits.P_M_Mode==0x03)&&(Port_COM_START.Bbits.HVAC_Start==0))|| \
			 ((MainStatus_HVDC.Bbits.P_M_Mode==0x03)&&(Port_COM_START.Bbits.HVDC_Start==0))|| \
			 ((MainStatus_LVDC.Bbits.P_M_Mode==0x03)&&(Port_COM_START.Bbits.LVDC_Start==0))|| \
			 ((MainStatus_LVAC.Bbits.P_M_Mode==0x03)&&(Port_COM_START.Bbits.LVAC_Start==0)))
		{
			for(i=0;i<4;i++)
			{
				PortPriority_Start[i]=0XFF;
				PriorityNum_Start=0;
			}			
		}
		else
		{
			if(Port_COM_START.Bbits.HVAC_Start==0)
			{
				PriorityPositionTemp=Port_StartPriorityPosition[0];
				for(i=PriorityPositionTemp;i<3;i++)
				{
					PortPriority_Start[i]=PortPriority_Start[i+1];
				}
				PortPriority_Start[3]=0xff;
					
				for(i=0;i<4;i++)
				{
					if(Port_StartPriorityPosition[i]>PriorityPositionTemp)
					{
						Port_StartPriorityPosition[i]--;
					}
				}
				Port_StartPriorityPosition[0]=3;
					
				PriorityNum_Start--;
			}
			if((Port_COM_START.Bbits.HVDC_Start==0)&&(PriorityNum_Start))
			{
				PriorityPositionTemp=Port_StartPriorityPosition[1];
				for(i=PriorityPositionTemp;i<3;i++)
				{
					PortPriority_Start[i]=PortPriority_Start[i+1];
				}
				PortPriority_Start[3]=0xff;
					
				for(i=0;i<4;i++)
				{
					if(Port_StartPriorityPosition[i]>PriorityPositionTemp)
					{
						Port_StartPriorityPosition[i]--;
					}
				}
				Port_StartPriorityPosition[1]=3;
									
				PriorityNum_Start--;
			}				
			if((Port_COM_START.Bbits.LVDC_Start==0)&&(PriorityNum_Start))
			{
				PriorityPositionTemp=Port_StartPriorityPosition[2];
				for(i=PriorityPositionTemp;i<3;i++)
				{
					PortPriority_Start[i]=PortPriority_Start[i+1];
				}
				PortPriority_Start[3]=0xff;
	
				for(i=0;i<4;i++)
				{
					if(Port_StartPriorityPosition[i]>PriorityPositionTemp)
					{
						Port_StartPriorityPosition[i]--;
					}
				}
				Port_StartPriorityPosition[2]=3;
					
				PriorityNum_Start--;
			}		
			if((Port_COM_START.Bbits.LVAC_Start==0)&&(PriorityNum_Start))
			{
				PriorityPositionTemp=Port_StartPriorityPosition[3];
				for(i=PriorityPositionTemp;i<3;i++)
				{
					PortPriority_Start[i]=PortPriority_Start[i+1];
				}
				PortPriority_Start[3]=0xff;

				for(i=0;i<4;i++)
				{
					if(Port_StartPriorityPosition[i]>PriorityPositionTemp)
					{
						Port_StartPriorityPosition[i]--;
					}
				}
				Port_StartPriorityPosition[3]=3;				
					
				PriorityNum_Start--;
			}						
		}	
	}
		
	if(PriorityNum_Start)
	{
		PriorityTemp=PORT_Index;
		PriorityTemp--;
		for(i=0;i<Port_StartPriorityPosition[PriorityTemp];i++)
		{
			if(PortPriority_Start[i]==1)//HVAC
			{
				if(MainStatus_HVAC.Bbits.P_Status!=PortStatusNum_Run)
				{
					ControlFLAG.Bbits.Port_StartValid=0;
					break;
				}
			}
			else if(PortPriority_Start[i]==2)//HVDC
			{
				if(MainStatus_HVDC.Bbits.P_Status!=PortStatusNum_Run)
				{
					ControlFLAG.Bbits.Port_StartValid=0;
					break;
				}					
			}
			else if(PortPriority_Start[i]==3)//LVDC
			{
				if(MainStatus_LVDC.Bbits.P_Status!=PortStatusNum_Run)
				{
					ControlFLAG.Bbits.Port_StartValid=0;
					break;
				}							
			}		
			else   //LVAC
			{
				if(MainStatus_LVAC.Bbits.P_Status!=PortStatusNum_Run)
				{
					ControlFLAG.Bbits.Port_StartValid=0;
					break;
				}	
			}		
		}
		ControlFLAG.Bbits.Port_StartValid=1;
	}
	else
	{
		ControlFLAG.Bbits.Port_StartValid=0;
	}	
}


/****************************************************************************/
/*                                                                          */
/*                           Port State Update                              */
/*                                                                          */
/****************************************************************************/
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

/****************************************************************************/
/*                                                                          */
/*                           Port State Update                              */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将端口运行状态及工作模式编码回状态字
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
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

/****************************************************************************/
/*                                                                          */
/*                           Port All-stop Flag Reset                       */
/*                                                                          */
/****************************************************************************/
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

/****************************************************************************/
/*                                                                          */
/*           Determine whether the port is the main power supply            */
/* return: 1: Extern power on 	                                            */
/****************************************************************************/
/*==================================================================
 * 函数功能: 检查原四端口状态字是否存在主电源模式且外部有电的端口
 * 输入参数: 无
 * 输出参数: 无；只检查 MainStatus_HVAC/HVDC/LVDC/LVAC 的模式和外部电源位。
 * 返回值: 存在主电源且有电返回 1，否则返回 0
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
ushort Judge_MainPowerSupply(void)
{
	if((MainStatus_HVAC.Bbits.P_M_Mode==0x03)&&(MainStatus_HVAC.Bbits.Outer_Elec))     //HVAC端口为主电源且有电
	{	
		return 1;
	}	
	else if((MainStatus_HVDC.Bbits.P_M_Mode==0x03)&&(MainStatus_HVDC.Bbits.Outer_Elec))
	{	
    return 1;
	}	
	else if((MainStatus_LVDC.Bbits.P_M_Mode==0x03)&&(MainStatus_LVDC.Bbits.Outer_Elec))
	{	
		return 1;
	}	
	else if((MainStatus_LVAC.Bbits.P_M_Mode==0x03)&&(MainStatus_LVAC.Bbits.Outer_Elec))
	{	
 		return 1;
	}			
	else
	{	
		return 0;
	}		
}

/****************************************************************************/
/*                                                                          */
/*              Normal flow of the system                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 推进旧正常运行流程中的启动、停止及开关操作
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void NormalRun_flow(void)
{
	switch(Port_RunStatus)
	{
		case PortStatusNum_Nonvaild:
		{

		}break;
		case PortStatusNum_Initial:
		{
			ControlFLAG.Bbits.Port_Stop=0x02;
			ControlFLAG.Bbits.Port_Start=0x00;

		}break;
		case PortStatusNum_Standby:
		{
			//Clear Variables
			
		}break;
		case PortStatusNum_Run:
		{
#ifdef HVAC_online					
			if((PORT_Index==1)&&(PortRunFLAG.Bbits.PWMState_Dab==0)&&(ControlFLAG.Bbits.DAB_StartMethod==0)&&(Port_RealStatus.Bbits.S_Mode>1))  // DAB H-bridge  pluse is on
			{
				ControlFLAG.Bbits.DAB_StartMethod=1;
				HA_DABDuty_A=350;    //5%
					
				ControlFLAG.Bbits.DAB_DutyStart=1;
				ControlFLAG.Bbits.DAB_DutyOver=0;			
			}	
#endif				
		}break;		
		default: break;
	}	
	
	if(PORT_Index==1)  //HVAC
	{
		if((PortRunFLAG.Bbits.PWMState_Dab==1)&&(PortRunFLAG.Bbits.PWMState_50Hz==1))
		{
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led1=0;
			Port_State=PortStatusNum_PluseClose;
		}
		else
		{
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led1=1;            //led of normal running
		}
	}
	else if(PORT_Index==2)  //HVDC
	{
		if(PortRunFLAG.Bbits.PWMState_Dab==1)
		{
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led1=0;
			Port_State=PortStatusNum_PluseClose;
		}
		else
		{
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led1=1;            //led of normal running
		}
	}	
	else if(PORT_Index==3)  //LVDC
	{
		if(PortRunFLAG.Bbits.PWMState_Dab==1)
		{
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led1=0;
			Port_State=PortStatusNum_PluseClose;
		}
		else
		{
			FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led1=1;            //led of normal running
		}
	}		
}

/****************************************************************************/
/*                                                                          */
/*              Start flow of the system                                    */
/*                                                                          */
/****************************************************************************/
#ifdef HVAC_online
/*==================================================================
 * 函数功能: 按编译端口推进旧自动预充及启动顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow(void)
{
	switch(Port_RunStatus)
	{
		case PortStatusNum_Start:                         // 收到启动请求，检查外部进线允许
		{
			ControlFLAG.Bbits.Port_Stop=0;
			ControlFLAG.Bbits.Port_Start=1;		
		FaultLatch_Clear(0,1);
		cout_Wait=2;
		Flag_Wait=0;
		while(Flag_Wait==0)
		{
			/* UART2 only queues maintenance bytes in its ISR.  Keep the
			 * protocol service alive while the legacy start flow waits. */
			MaintenanceProtocol_Process();
		}
			
			Port_DINRead(0);
			
			SW.Bbits.S_EXT_CB=1;          											//?????????????????????????????????????????
			SW.Bbits.S_EXT_CB_ON_EN=1;    											//?????????????????????????????????????????


			if(Port_Main_WorkMode==0x03)  //主端口电源
			{
//			if(Judge_MainPowerSupply()&&(SW.Bbits.S_EXT_CB||SW.Bbits.S_EXT_CB_ON_EN))   //主电源端口有电，且外部断路器已经合上或综自允许外部合上
				if((SW.Bbits.S_EXT_CB||SW.Bbits.S_EXT_CB_ON_EN))   //????????????????????
				{
					Flag_FaultOff=1;
					if(SW.Bbits.Flag_SwitchRun==0)
					{
						SwitchTable_Start();
					
						if(SW.Bbits.Flag_SwitchTable==0) 
						{
							Port_RunStatus=PortStatusNum_Stop;
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
							
							ControlFLAG.Bbits.Port_Start=0x03;			     //启动失败
      	
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.PWMPulseState=3;    
							FaultDisposeFLAG.Bbits.Port_StopType=2;    

							Flag_FaultOff=0;								
						}
						else if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))
						{
							SWTable_TotalTime=0;
													
							coutPrecharge500ms=0;
							Flag_Precharge500ms=1;						
							Port_RunStatus=PortStatusNum_Start_SWOver;   // 进入预充等待状态
						}
						else
						{
							cout_Wait=100;
							Flag_Wait=0;
							while(Flag_Wait==0)
							{
								MaintenanceProtocol_Process();
							}							
							Port_RunStatus=PortStatusNum_Start_SWTable;   //继续开关动作
							SWTable_TotalTime=SwitchTotalNum;
						}
					}				
				}
				else
				{
					ControlFLAG.Bbits.Port_Start=0x03;			    //启动失败
					
					Port_RunStatus=PortStatusNum_Stop;
					ControlFLAG.Bbits.Port_Stop=0x01;
					ControlFLAG.Bbits.Port_Start=0x00;
      	
					FaultDisposeFLAG.Bbits.FaultDisposeType=2;
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					FaultDisposeFLAG.Bbits.Port_StopType=2;     
				}			
			}	
/*			
			else //非主电源
			{
				if((SW.Bbits.S_EXT_CB||SW.Bbits.S_EXT_CB_ON_EN))
				{
					Flag_FaultOff=1;
					countWaitPowerStart10s=0;
					Flag_WaitPowerStart10s=0;
					Port_RunStatus=PortStatusNum_WaitPowerStart;
				}
				else
				{
					Port_RunStatus=PortStatusNum_Stop;
					ControlFLAG.Bbits.Port_Start=0x03;			    //启动失败
      	
					FaultDisposeFLAG.Bbits.FaultDisposeType=2;
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					FaultDisposeFLAG.Bbits.Port_StopType=2;     						
				}
			}
*/			
		}break;
/*		
		case PortStatusNum_WaitPowerStart:
		{
			if(Flag_WaitPowerStart10s==1)
			{
				Port_RunStatus=PortStatusNum_Stop;
				ControlFLAG.Bbits.Port_Start=0x03;			    //启动失败
      	
				FaultDisposeFLAG.Bbits.FaultDisposeType=2;
				FaultDisposeFLAG.Bbits.PWMPulseState=3;
				FaultDisposeFLAG.Bbits.Port_StopType=2;     	
				Flag_FaultOff=0;			
			}
			else
			{
				if(((MainStatus_HVAC.MainStatus16&0x30ff==0x30a0)||(MainStatus_HVDC.MainStatus16&0x30ff==0x30a0)||(MainStatus_LVDC.MainStatus16&0x30ff==0x30a0)||(MainStatus_LVAC.MainStatus16&0x30ff==0x30a0)))
				{//主电源工作
					if(SW.Bbits.Flag_SwitchRun==0)
					{
						SwitchTable_Start();
					
						if(SW.Bbits.Flag_SwitchTable==0) 
						{
							Port_RunStatus=PortStatusNum_Stop;
							ControlFLAG.Bbits.Port_Start=0x03;			     //启动失败
      	
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.PWMPulseState=3;    
							FaultDisposeFLAG.Bbits.Port_StopType=2;    

							Flag_FaultOff=0;								
						}
						else if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))
						{
							SWTable_TotalTime=0;
													
							coutPrecharge500ms=0;
							Flag_Precharge500ms=1;						
							Port_RunStatus=PortStatusNum_Start_SWOver;   //开始等待预充完成
						}
						else
						{
							Port_RunStatus=PortStatusNum_Start_SWTable;   //继续开关动作
							SWTable_TotalTime=SwitchTotalNum;
						}
					}				
				}						
			}	
		}break;
*/		
		case PortStatusNum_Start_SWTable:                  // 按开关表逐步执行接触器动作
		{
			if(SW.Bbits.Flag_SwitchRun==0)
			{
				if((SW.Bbits.Flag_SwitchSuccess==0)&&(SWTable_TotalTime==0))
				{
					Port_RunStatus=PortStatusNum_Stop;
					ControlFLAG.Bbits.Port_Stop=0x01;
					ControlFLAG.Bbits.Port_Start=0x00;
										
					ControlFLAG.Bbits.Port_Start=0x03;			   //启动失败

					FaultDisposeFLAG.Bbits.FaultDisposeType=2;
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					FaultDisposeFLAG.Bbits.Port_StopType=2;    

//				Port_RealFault.Bbits.PORT_FAULT_bit15=1;
					Flag_FaultOff=0;
				}
				else
				{
					if(SWTable_TotalTime==0)      //前一次成功
					{
						SwitchTable_Start();

						if(SW.Bbits.Flag_SwitchTable==0) 
						{
							Port_RunStatus=PortStatusNum_Stop;
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
					
							ControlFLAG.Bbits.Port_Start=0x03;			

							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							FaultDisposeFLAG.Bbits.Port_StopType=2;    
	
							Flag_FaultOff=0;					
						}
						else if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))
						{
							SWTable_TotalTime=0;
						
							coutPrecharge500ms=0;
							Flag_Precharge500ms=1;
							Port_RunStatus=PortStatusNum_Start_SWOver;
						}
						else
						{
							cout_Wait=100;
							Flag_Wait=0;
							while(Flag_Wait==0)
							{
								MaintenanceProtocol_Process();
							}								
							SWTable_TotalTime=SwitchTotalNum;
						}					
					}
				}
			}
		}break;

		case PortStatusNum_Start_SWOver:                   // 等待预充完成标志
		{
			if(Flag_Precharge500ms==0)
			{
					Port_RunStatus=PortStatusNum_Stop;
					ControlFLAG.Bbits.Port_Stop=0x01;
					ControlFLAG.Bbits.Port_Start=0x00;
					
					ControlFLAG.Bbits.Port_Start=0x03;			

					FaultDisposeFLAG.Bbits.FaultDisposeType=2;
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					FaultDisposeFLAG.Bbits.Port_StopType=2;    
	
					Flag_FaultOff=0;	
			}
			else
			{
				if(PortRunFLAG.Bbits.Port_SigUdc==1)            // 三相直流电压达到预充门槛
				{
					Flag_Precharge500ms=0;
					coutPrecharge500ms=0;

					FaultLatch_Clear(0,1);	
									
//					cout_Wait=1;
//					Flag_Wait=0;
//					while((Flag_Wait==0)||(Flag_Clear_0==1)||(Flag_Clear_1==1));	
					while((Flag_Clear_0==1)||(Flag_Clear_1==1))
					{
						MaintenanceProtocol_Process();
					}	
					
					Flag_FaultOff=0;
					
					if(SW.Bbits.Flag_SwitchTable==1)   //接触器闭合状态
					{						
						if(PORT_Index==1)  //HVAC启动SVG
						{
							Port_RunStatus=PortStatusNum_Start_PortStart; // 进入本端口PWM启动阶段
							//启动本端口
							flag_SendSVGRef_First=1;
							ControlFLAG.Bbits.SVG_Start=1;              // 请求FPGA侧开始执行HVAC控制
							ControlFLAG.Bbits.SVG_StartOver=0;
							Flag_SVG1s=1;
							coutSVG1s=0;	
						}
						else if(PORT_Index==2) //HVDC启动DAB
						{
							
						}
					}
					else
					{
						cout_Wait=1000;
						Flag_Wait=0;
						while(Flag_Wait==0)
						{
							MaintenanceProtocol_Process();
						}	
												
						SW.Bbits.Flag_Switch=1;
						SW.Bbits.Flag_SwitchZhi=1;
						SW.Bbits.Flag_SwitchRun=0;
						SW.Bbits.Flag_SwitchSuccess=0;

						SWTable_TotalTime=SwitchTotalNum;
						Port_RunStatus=PortStatusNum_Start_ContactorOn;

						ControlFLAG.Bbits.SVG_Start=1;
						ControlFLAG.Bbits.SVG_StartOver=0;
						flag_SendSVGRef_First=0;						
					}
				}
			}
		}break;			

		case PortStatusNum_Start_ContactorOn:              // 合上软起旁路接触器
		{
			if(SW.Bbits.Flag_SwitchRun==0)
			{
				if((SW.Bbits.Flag_SwitchSuccess==0)&&(SWTable_TotalTime==0))
				{
					Port_RunStatus=PortStatusNum_Stop;
					ControlFLAG.Bbits.Port_Stop=0x01;
					ControlFLAG.Bbits.Port_Start=0x00;
					
					ControlFLAG.Bbits.Port_Start=0x03;			

					FaultDisposeFLAG.Bbits.FaultDisposeType=2;
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					FaultDisposeFLAG.Bbits.Port_StopType=2;    
							
//				Port_RealFault.Bbits.PORT_FAULT_bit15=1;
				}
				else
				{
					if(SW.Bbits.Flag_SwitchSuccess==1)
					{						
						if(PORT_Index==1)  //HVAC启动SVG
						{
							Port_RunStatus=PortStatusNum_Start_PortStart; // 旁路完成，准备解锁PWM

							cout_Wait=500;
							Flag_Wait=0;
							while(Flag_Wait==0)
							{
								MaintenanceProtocol_Process();
							}	
															
							//启动本端口
							flag_SendSVGRef_First=1;
 							ControlFLAG.Bbits.SVG_StartOver=0;
							Flag_SVG1s=1;
							coutSVG1s=0;	
						}
					}				
				}
			}
		}break;			

		case PortStatusNum_Start_PortStart:                 // 等待AC PWM启动完成或超时
		{	
			if((PORT_Index==1)&&(flag_SendSVGRef_First==3))          //HVAC
			{
					if(PortRunFLAG.Bbits.PWMState_50Hz==1)
					{
						ControlFLAG.Bbits.SVG_Start=0;
						ControlFLAG.Bbits.SVG_StartOver=0;
						ControlFLAG.Bbits.Port_Start=0x03;	
						
						Flag_SVG1s=0;
						coutSVG1s=0;						
					}	
					else
					{	
						if(Flag_SVG1s==0)
						{
							ControlFLAG.Bbits.SVG_Start=0;
							ControlFLAG.Bbits.SVG_StartOver=0;
							ControlFLAG.Bbits.Port_Start=0x03;
					
#ifdef OV_Stop									
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.Port_StopType=2;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							Port_RunStatus=PortStatusNum_Stop;
							
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
#else
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.Port_StopType=0;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
			
							PWMPluse_Disable(0);
#endif
						}
						else
						{
							if(ControlFLAG.Bbits.SVG_StartOver)
							{
								ControlFLAG.Bbits.SVG_StartOver=0;
								Flag_SVG1s=0;
								coutSVG1s=0;
								ControlFLAG.Bbits.Port_Start=2;   //success
								
								Port_RunStatus=PortStatusNum_Start_SVGRun;
								
								cout_Wait=800;
								Flag_Wait=0;
								while(Flag_Wait==0)
								{
									MaintenanceProtocol_Process();
								}	
														
								ControlFLAG.Bbits.DAB_DutyStart=1;
								ControlFLAG.Bbits.DAB_DutyOver=0;	
								
								#ifdef HVAC_online
								HA_DABDuty_A=350;    //5%
								#endif
								
										PWMPluse_Enable(2);                     // AC启动完成后再解锁DAB

/*							
								ControlFLAG.Bbits.DAB_StartMethod=0;

								Port_State=PortStatusNum_Run;
								Port_RunStatus=PortStatusNum_Run;
								ControlFLAG.Bbits.DAB_DutyOver=0;								
*/								
							}
						}
					}
			}	
/*			
			else if(PORT_Index==2)                                //HVDC 非主电源模式
			{
				if(Flag_SVG1s==0)
				{
					Port_RunStatus=PortStatusNum_Stop;
					ControlFLAG.Bbits.Port_Start=0x03;			

					FaultDisposeFLAG.Bbits.FaultDisposeType=2;
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					FaultDisposeFLAG.Bbits.Port_StopType=2;    
									
					ControlFLAG.Bbits.DAB_DutyStart=0;
					ControlFLAG.Bbits.DAB_DutyOver=0;						
				}
				else
				{
					if(ControlFLAG.Bbits.DAB_DutyOver)
					{
						ControlFLAG.Bbits.DAB_DutyOver=0;
						Flag_SVG1s=0;
						coutSVG1s=0;
						ControlFLAG.Bbits.Port_Start=2;   //success
						Port_State=PortStatusNum_Run;
						Port_RunStatus=PortStatusNum_Run;
					}
				}				
			}	
*/					
		}break;

		case PortStatusNum_Start_SVGRun:
		{
			if((PORT_Index==1)&&(ControlFLAG.Bbits.DAB_DutyOver))
			{
				ControlFLAG.Bbits.DAB_StartMethod=1;
				ControlFLAG.Bbits.DAB_DutyOver=0;
				Port_State=PortStatusNum_Run;
				Port_RunStatus=PortStatusNum_Run;	
			}
		}break;
				
		default:break;
	}	
}
#endif

#ifdef HVDC_online
/*==================================================================
 * 函数功能: 按编译端口推进旧自动预充及启动顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow(void)
{
	switch(Port_RunStatus)
	{
		case PortStatusNum_Start:
		{
			ControlFLAG.Bbits.Port_Stop=0;
			ControlFLAG.Bbits.Port_Start=1;		
			FaultLatch_Clear(0,1);        		 //clear latch & fault flag

#ifdef FaultOutTest			
						Flag_DC_FAULT=0;
						Flag_DC_FAULTport=0;
						Flag_DC_FAULTDISPOSE=0;
#endif
						
			Port_DINRead(0);

			SW.Bbits.S_EXT_CB=1;         			 //?????????????????????????????????????????
			SW.Bbits.S_EXT_CB_ON_EN=1;    		 //?????????????????????????????????????????
				
//		if(SW.Bbits.Flag_SwitchRun==0)
//		{
//			SwitchTable_Start();
//		}
//		if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))   //switch on
			{				
				Port_RunStatus=PortStatusNum_ManualStart_SWOver;  // 手动启动：进入预充完成等待
			}				
		}break;

		case PortStatusNum_ManualStart_SWOver:              // 等待预充完成，准备启动本端口
		{
//		if(PortRunFLAG.Bbits.Port_SigUdc==1)&& 主电源已经工作           
			if((PortRunFLAG.Bbits.Port_SigUdc==1)||(flag_INV==1)) // 预充完成或逆变仿真直接放行
			{						
				if((PORT_Index==2)&&(PortRunFLAG.Bbits.PWMState_Dab==1)) // 本端口为HVDC且DAB仍闭锁
				{
					Port_RunStatus=PortStatusNum_ManualStart_SVGStart; // 等待DAB解锁反馈
				
					ControlFLAG.Bbits.SVG_Start=1;                   // 请求FPGA执行HVDC启动控制
					ControlFLAG.Bbits.SVG_StartOver=0;               // 清除启动完成标志
					flag_SendSVGRef_First=0;
					
					flag_DABOPEN=1;                                  // 触发DAB解锁，进入DAB闭环使能
				}
				
			}
		}break;				
				
		case PortStatusNum_ManualStart_SVGStart:               // 等待DAB PWM反馈已解锁
		{
			if((PORT_Index==2)&&(PortRunFLAG.Bbits.PWMState_Dab==0)) 
			{
				Port_RunStatus=PortStatusNum_ManualStart_PortStart;

				Flag_SVG1s=1;
				coutSVG1s=0;	
			}
		}break;		
				
		case PortStatusNum_ManualStart_PortStart:               // 启动完成判定和运行态切换
		{	
			if(PORT_Index==2)                                   //HVDC
			{
				if(flag_INV==1)
				{
					if(Flag_SVG1s==0)
					{
						ControlFLAG.Bbits.SVG_StartOver=0;
						Flag_SVG1s=0;
						coutSVG1s=0;
						ControlFLAG.Bbits.Port_Start=2;   //success
						
						ControlFLAG.Bbits.DAB_StartMethod=1;
						Port_State=PortStatusNum_Run;
						Port_RunStatus=PortStatusNum_Run;
						ControlFLAG.Bbits.DAB_DutyOver=0;
					}
				}	
				else
				{	
					if(PortRunFLAG.Bbits.PWMState_Dab==1)
					{
						ControlFLAG.Bbits.SVG_Start=0;
						ControlFLAG.Bbits.SVG_StartOver=0;
						ControlFLAG.Bbits.Port_Start=0x03;	
						
						Flag_SVG1s=0;
						coutSVG1s=0;						
					}	
					else
					{	
						if(Flag_SVG1s==0)
						{
							ControlFLAG.Bbits.SVG_Start=0;
							ControlFLAG.Bbits.SVG_StartOver=0;
							ControlFLAG.Bbits.Port_Start=0x03;
					
#ifdef OV_Stop									
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.Port_StopType=2;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							Port_RunStatus=PortStatusNum_Stop;
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
#else
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.Port_StopType=0;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
			
							PWMPluse_Disable(0);
#endif
						}
						else
						{
							if(ControlFLAG.Bbits.SVG_StartOver)
							{
								ControlFLAG.Bbits.SVG_StartOver=0;
								Flag_SVG1s=0;
								coutSVG1s=0;
								ControlFLAG.Bbits.Port_Start=2;   //success
						
								ControlFLAG.Bbits.DAB_StartMethod=1;

								Port_State=PortStatusNum_Run;
								Port_RunStatus=PortStatusNum_Run;
							}
						}
					}
				}
			}					
		}break;	

		
		default:break;
	}
}
#endif

#ifdef LVDC_online
/*==================================================================
 * 函数功能: 按编译端口推进旧自动预充及启动顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow(void)
{
	switch(Port_RunStatus)
	{
		case PortStatusNum_Start:
		{
			ControlFLAG.Bbits.Port_Stop=0;
			ControlFLAG.Bbits.Port_Start=1;		
			FaultLatch_Clear(0,1);        		 //clear latch & fault flag
			
#ifdef FaultOutTest			
						Flag_DC_FAULT=0;
						Flag_DC_FAULTport=0;
						Flag_DC_FAULTDISPOSE=0;
#endif
						
			Port_DINRead(0);
			
			//SW.Bbits.S_EXT_CB=1;         			 //?????????????????????????????????????????
			//SW.Bbits.S_EXT_CB_ON_EN=1;    		 //?????????????????????????????????????????
				
//		if(SW.Bbits.Flag_SwitchRun==0)
//		{
//			SwitchTable_Start();
//		}
//		if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))   //switch on

		//edited by shiqi, start, start flow including close contactor.
			//if(SW.Bbits.S_EXT_CB)
			//{
				//Port_RunStatus=PortStatusNum_ManualStart_SWOver;
				SW.Bbits.Flag_Switch=1;
				SW.Bbits.Flag_SwitchZhi=1;
				SW.Bbits.Flag_SwitchRun=0;
				SW.Bbits.Flag_SwitchSuccess=0;
				Port_RunStatus=PortStatusNum_Start_ContactorOn;
			//}
			//else
			//{
			//	ControlFLAG.Bbits.Port_Start=0x03;			    //启动失败

			//	Port_RunStatus=PortStatusNum_Stop;
			//	ControlFLAG.Bbits.Port_Stop=0x01;
//				ControlFLAG.Bbits.Port_Start=0x00;

			//	FaultDisposeFLAG.Bbits.FaultDisposeType=2;
			//	FaultDisposeFLAG.Bbits.PWMPulseState=3;
			//	FaultDisposeFLAG.Bbits.Port_StopType=2;
			//}
		}break;

		case PortStatusNum_Start_ContactorOn:
		{
			if(SW.Bbits.Flag_SwitchRun==0)
			{
				if((SW.Bbits.Flag_SwitchSuccess==0)&&(SWTable_TotalTime==0))
				{
					Port_RunStatus=PortStatusNum_Stop;
					ControlFLAG.Bbits.Port_Stop=0x01;
					ControlFLAG.Bbits.Port_Start=0x00;

					ControlFLAG.Bbits.Port_Start=0x03;

					FaultDisposeFLAG.Bbits.FaultDisposeType=2;
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					FaultDisposeFLAG.Bbits.Port_StopType=2;

//				Port_RealFault.Bbits.PORT_FAULT_bit15=1;
				}
				else
				{
					if(SW.Bbits.Flag_SwitchSuccess==1)
					{
						Port_RunStatus=PortStatusNum_ManualStart_SWOver;
						cout_Wait=50;
						Flag_Wait=0;
						while(Flag_Wait==0)
						{
							MaintenanceProtocol_Process();
						}
					}
				}
			}
		}break;
        //edited by shiqi, end

		case PortStatusNum_ManualStart_SWOver:
		{
//		if(PortRunFLAG.Bbits.Port_SigUdc==1)&& 主电源已经工作           
			if((PortRunFLAG.Bbits.Port_SigUdc==1)||(flag_INV==1))           //precharge over
			{						
				if((PORT_Index==3)&&(PortRunFLAG.Bbits.PWMState_Dab==1))
				{
					Port_RunStatus=PortStatusNum_ManualStart_SVGStart;
				
					ControlFLAG.Bbits.SVG_Start=1;
					ControlFLAG.Bbits.SVG_StartOver=0;
					flag_SendSVGRef_First=0;
					
					flag_DABOPEN=1;
				}			
			}
		}break;				
				
		case PortStatusNum_ManualStart_SVGStart:
		{
			if((PORT_Index==3)&&(PortRunFLAG.Bbits.PWMState_Dab==0)) 
			{
				Port_RunStatus=PortStatusNum_ManualStart_PortStart;

				Flag_SVG1s=1;
				coutSVG1s=0;	
			}
		}break;		
				
		case PortStatusNum_ManualStart_PortStart:
		{	
			if(PORT_Index==3)                                   //LVDC
			{
				if(flag_INV==1)
				{
					if(Flag_SVG1s==0)
					{
						ControlFLAG.Bbits.SVG_StartOver=0;
						Flag_SVG1s=0;
						coutSVG1s=0;
						ControlFLAG.Bbits.Port_Start=2;   //success
						
						ControlFLAG.Bbits.DAB_StartMethod=1;
						Port_State=PortStatusNum_Run;
						Port_RunStatus=PortStatusNum_Run;
						ControlFLAG.Bbits.DAB_DutyOver=0;
					}
				}	
				else
				{	
					if(PortRunFLAG.Bbits.PWMState_Dab==1)
					{
						ControlFLAG.Bbits.SVG_Start=0;
						ControlFLAG.Bbits.SVG_StartOver=0;
						ControlFLAG.Bbits.Port_Start=0x03;	
						
						Flag_SVG1s=0;
						coutSVG1s=0;						
					}	
					else
					{	
						if(Flag_SVG1s==0)
						{
							ControlFLAG.Bbits.SVG_Start=0;
							ControlFLAG.Bbits.SVG_StartOver=0;
							ControlFLAG.Bbits.Port_Start=0x03;
					
#ifdef OV_Stop									
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.Port_StopType=2;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							Port_RunStatus=PortStatusNum_Stop;
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
#else
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.Port_StopType=0;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
			
							PWMPluse_Disable(0);
#endif
						}
						else
						{
							if(ControlFLAG.Bbits.SVG_StartOver)
							{
								ControlFLAG.Bbits.SVG_StartOver=0;
								Flag_SVG1s=0;
								coutSVG1s=0;
								ControlFLAG.Bbits.Port_Start=2;   //success
						
								ControlFLAG.Bbits.DAB_StartMethod=1;

								Port_State=PortStatusNum_Run;
								Port_RunStatus=PortStatusNum_Run;
							}
						}
					}
				}
			}					
		}break;	

		
		default:break;
	}
}
#endif

/****************************************************************************/
/*                                                                          */
/*              Start flow of the system                                    */
/*                                                                          */
/****************************************************************************/
#ifdef HVAC_online
/*==================================================================
 * 函数功能: 按编译端口推进旧手动启动与脉冲使能顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow_Manual(void)
{
	switch(Port_RunStatus)
	{
		case PortStatusNum_Start:
		{
			ControlFLAG.Bbits.Port_Stop=0;
			ControlFLAG.Bbits.Port_Start=1;		
			FaultLatch_Clear(0,1);        //clear latch & fault flag
						
			Port_DINRead(0);

			SW.Bbits.S_EXT_CB=1;          //?????????????????????????????????????????
			SW.Bbits.S_EXT_CB_ON_EN=1;    //?????????????????????????????????????????
			
//		if(SW.Bbits.Flag_SwitchRun==0)
//		{
//			SwitchTable_Start();
//		}
//		if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))   //switch on
			{				
				Port_RunStatus=PortStatusNum_ManualStart_SWOver;
			}				
		}break;

		case PortStatusNum_ManualStart_SWOver:
		{
			if(PortRunFLAG.Bbits.Port_SigUdc==1)           //precharge over
			{						
				if((PORT_Index==1)&&(PortRunFLAG.Bbits.PWMState_Dab==1))
				{
					if(DAB_ONLY)
					{				
						if(PortRunFLAG.Bbits.PWMState_50Hz==1)
						{
							ControlFLAG.Bbits.DAB_StartMethod=0;
							Port_State=PortStatusNum_Run;
							Port_RunStatus=PortStatusNum_Run;
							ControlFLAG.Bbits.DAB_DutyOver=0;					
						}
					}
					else
					{					
						Port_RunStatus=PortStatusNum_ManualStart_SVGStart;
				
						ControlFLAG.Bbits.SVG_Start=1;
						ControlFLAG.Bbits.SVG_StartOver=0;
						flag_SendSVGRef_First=0;
					}
				}				
			}
		}break;			
				
		case PortStatusNum_ManualStart_SVGStart:
		{
			if((PORT_Index==1)&&(PortRunFLAG.Bbits.PWMState_50Hz==0))  //SVG pluse is on
			{
				Port_RunStatus=PortStatusNum_ManualStart_PortStart;

				Flag_SVG1s=1;
				coutSVG1s=0;	
			}
		}break;			

		case PortStatusNum_ManualStart_PortStart:
		{	
			if(PORT_Index==1)                                   //HVAC
			{	
				if(flag_INV==1)
				{
				//	if(ControlFLAG.Bbits.SVG_StartOver)
					{
						ControlFLAG.Bbits.SVG_StartOver=0;
						Flag_SVG1s=0;
						coutSVG1s=0;
						ControlFLAG.Bbits.Port_Start=2;   //success
						
						ControlFLAG.Bbits.DAB_StartMethod=1;
						Port_State=PortStatusNum_Run;
						Port_RunStatus=PortStatusNum_Run;
						ControlFLAG.Bbits.DAB_DutyOver=0;
					}
				}	
				else
				{
					if(PortRunFLAG.Bbits.PWMState_50Hz==1)
					{
						ControlFLAG.Bbits.SVG_Start=0;
						ControlFLAG.Bbits.SVG_StartOver=0;
						ControlFLAG.Bbits.Port_Start=0x03;	
						
						Flag_SVG1s=0;
						coutSVG1s=0;						
					}	
					else
					{	
						if(Flag_SVG1s==0)
						{
							ControlFLAG.Bbits.SVG_Start=0;
							ControlFLAG.Bbits.SVG_StartOver=0;
							ControlFLAG.Bbits.Port_Start=0x03;
					
#ifdef OV_Stop									
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.Port_StopType=2;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							Port_RunStatus=PortStatusNum_Stop;
							
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
#else
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.Port_StopType=0;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
			
							PWMPluse_Disable(0);
#endif
						}
						else
						{
							if(ControlFLAG.Bbits.SVG_StartOver)
							{
								ControlFLAG.Bbits.SVG_StartOver=0;
								Flag_SVG1s=0;
								coutSVG1s=0;
								ControlFLAG.Bbits.Port_Start=2;   //success
						
								ControlFLAG.Bbits.DAB_StartMethod=0;

								Port_State=PortStatusNum_Run;
								Port_RunStatus=PortStatusNum_Run;
								ControlFLAG.Bbits.DAB_DutyOver=0;
							}
						}
					}
				}
			}		
		}break;	

		default:break;
	}
}
#endif

#ifdef HVDC_online
/*==================================================================
 * 函数功能: 按编译端口推进旧手动启动与脉冲使能顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow_Manual(void)
{
	switch(Port_RunStatus)
	{
		case PortStatusNum_Start:
		{
			ControlFLAG.Bbits.Port_Stop=0;
			ControlFLAG.Bbits.Port_Start=1;		
			FaultLatch_Clear(0,1);        		 //clear latch & fault flag

#ifdef FaultOutTest			
						Flag_DC_FAULT=0;
						Flag_DC_FAULTport=0;
						Flag_DC_FAULTDISPOSE=0;
#endif
						
			Port_DINRead(0);

			SW.Bbits.S_EXT_CB=1;         			 //?????????????????????????????????????????
			SW.Bbits.S_EXT_CB_ON_EN=1;    		 //?????????????????????????????????????????
				
//		if(SW.Bbits.Flag_SwitchRun==0)
//		{
//			SwitchTable_Start();
//		}
//		if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))   //switch on
			{				
				Port_RunStatus=PortStatusNum_ManualStart_SWOver;
			}				
		}break;

		case PortStatusNum_ManualStart_SWOver:
		{
//		if(PortRunFLAG.Bbits.Port_SigUdc==1)&& 主电源已经工作           
			if((PortRunFLAG.Bbits.Port_SigUdc==1)||(flag_INV==1))           //precharge over
			{						
				if((PORT_Index==2)&&(PortRunFLAG.Bbits.PWMState_Dab==1))
				{
					Port_RunStatus=PortStatusNum_ManualStart_SVGStart;
				
					ControlFLAG.Bbits.SVG_Start=1;
					ControlFLAG.Bbits.SVG_StartOver=0;
					flag_SendSVGRef_First=0;
				}
				
			}
		}break;				
				
		case PortStatusNum_ManualStart_SVGStart:
		{
			if((PORT_Index==2)&&(PortRunFLAG.Bbits.PWMState_Dab==0)) 
			{
				Port_RunStatus=PortStatusNum_ManualStart_PortStart;

				Flag_SVG1s=1;
				coutSVG1s=0;	
			}
		}break;		
				
		case PortStatusNum_ManualStart_PortStart:
		{	
			if(PORT_Index==2)                                   //HVDC
			{
				if(flag_INV==1)
				{
					if(Flag_SVG1s==0)
					{
						ControlFLAG.Bbits.SVG_StartOver=0;
						Flag_SVG1s=0;
						coutSVG1s=0;
						ControlFLAG.Bbits.Port_Start=2;   //success
						
						ControlFLAG.Bbits.DAB_StartMethod=1;
						Port_State=PortStatusNum_Run;
						Port_RunStatus=PortStatusNum_Run;
						ControlFLAG.Bbits.DAB_DutyOver=0;
					}
				}	
				else
				{	
					if(PortRunFLAG.Bbits.PWMState_Dab==1)
					{
						ControlFLAG.Bbits.SVG_Start=0;
						ControlFLAG.Bbits.SVG_StartOver=0;
						ControlFLAG.Bbits.Port_Start=0x03;	
						
						Flag_SVG1s=0;
						coutSVG1s=0;						
					}	
					else
					{	
						if(Flag_SVG1s==0)
						{
							ControlFLAG.Bbits.SVG_Start=0;
							ControlFLAG.Bbits.SVG_StartOver=0;
							ControlFLAG.Bbits.Port_Start=0x03;
					
#ifdef OV_Stop									
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.Port_StopType=2;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							Port_RunStatus=PortStatusNum_Stop;
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
#else
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.Port_StopType=0;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
			
							PWMPluse_Disable(0);
#endif
						}
						else
						{
							if(ControlFLAG.Bbits.SVG_StartOver)
							{
								ControlFLAG.Bbits.SVG_StartOver=0;
								Flag_SVG1s=0;
								coutSVG1s=0;
								ControlFLAG.Bbits.Port_Start=2;   //success
						
								ControlFLAG.Bbits.DAB_StartMethod=1;

								Port_State=PortStatusNum_Run;
								Port_RunStatus=PortStatusNum_Run;
							}
						}
					}
				}
			}					
		}break;	

		
		default:break;
	}
}
#endif

#ifdef LVDC_online
/*==================================================================
 * 函数功能: 按编译端口推进旧手动启动与脉冲使能顺序
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Start_flow_Manual(void)
{
	switch(Port_RunStatus)
	{
		case PortStatusNum_Start:
		{
			ControlFLAG.Bbits.Port_Stop=0;
			ControlFLAG.Bbits.Port_Start=1;		
			FaultLatch_Clear(0,1);        		 //clear latch & fault flag
			
#ifdef FaultOutTest			
						Flag_DC_FAULT=0;
						Flag_DC_FAULTport=0;
						Flag_DC_FAULTDISPOSE=0;
#endif
						
			Port_DINRead(0);
//edited by shiqi, start, we will have external CB on site
//			SW.Bbits.S_EXT_CB=1;         			 //?????????????????????????????????????????
//			SW.Bbits.S_EXT_CB_ON_EN=1;    		 //?????????????????????????????????????????
//eidted by shiqi, end
//		if(SW.Bbits.Flag_SwitchRun==0)
//		{
//			SwitchTable_Start();
//		}
//		if((SW.Bbits.Flag_SwitchTable==1)||(SW.Bbits.Flag_SwitchTable==2))   //switch on
			{				
				Port_RunStatus=PortStatusNum_ManualStart_SWOver;
			}				
		}break;

		case PortStatusNum_ManualStart_SWOver:
		{
//		if(PortRunFLAG.Bbits.Port_SigUdc==1)&& 主电源已经工作           
			if((PortRunFLAG.Bbits.Port_SigUdc==1)||(flag_INV==1))           //precharge over
			{						
				if((PORT_Index==3)&&(PortRunFLAG.Bbits.PWMState_Dab==1))
				{
					Port_RunStatus=PortStatusNum_ManualStart_SVGStart;
				
					ControlFLAG.Bbits.SVG_Start=1;
					ControlFLAG.Bbits.SVG_StartOver=0;
					flag_SendSVGRef_First=0;
				}			
			}
		}break;				
				
		case PortStatusNum_ManualStart_SVGStart:
		{
			if((PORT_Index==3)&&(PortRunFLAG.Bbits.PWMState_Dab==0)) 
			{
				Port_RunStatus=PortStatusNum_ManualStart_PortStart;

				Flag_SVG1s=1;
				coutSVG1s=0;	
			}
		}break;		
				
		case PortStatusNum_ManualStart_PortStart:
		{	
			if(PORT_Index==3)                                   //LVDC
			{
				if(flag_INV==1)
				{
					if(Flag_SVG1s==0)
					{
						ControlFLAG.Bbits.SVG_StartOver=0;
						Flag_SVG1s=0;
						coutSVG1s=0;
						ControlFLAG.Bbits.Port_Start=2;   //success
						
						ControlFLAG.Bbits.DAB_StartMethod=1;
						Port_State=PortStatusNum_Run;
						Port_RunStatus=PortStatusNum_Run;
						ControlFLAG.Bbits.DAB_DutyOver=0;
					}
				}	
				else
				{	
					if(PortRunFLAG.Bbits.PWMState_Dab==1)
					{
						ControlFLAG.Bbits.SVG_Start=0;
						ControlFLAG.Bbits.SVG_StartOver=0;
						ControlFLAG.Bbits.Port_Start=0x03;	
						
						Flag_SVG1s=0;
						coutSVG1s=0;						
					}	
					else
					{	
						if(Flag_SVG1s==0)
						{
							ControlFLAG.Bbits.SVG_Start=0;
							ControlFLAG.Bbits.SVG_StartOver=0;
							ControlFLAG.Bbits.Port_Start=0x03;
					
#ifdef OV_Stop									
							FaultDisposeFLAG.Bbits.FaultDisposeType=2;
							FaultDisposeFLAG.Bbits.Port_StopType=2;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
							Port_RunStatus=PortStatusNum_Stop;
							ControlFLAG.Bbits.Port_Stop=0x01;
							ControlFLAG.Bbits.Port_Start=0x00;
#else
							FaultDisposeFLAG.Bbits.FaultDisposeType=1;
							FaultDisposeFLAG.Bbits.Port_StopType=0;
				
							FaultDisposeFLAG.Bbits.PWMPulseState=3;
			
							PWMPluse_Disable(0);
#endif
						}
						else
						{
							if(ControlFLAG.Bbits.SVG_StartOver)
							{
								ControlFLAG.Bbits.SVG_StartOver=0;
								Flag_SVG1s=0;
								coutSVG1s=0;
								ControlFLAG.Bbits.Port_Start=2;   //success
						
								ControlFLAG.Bbits.DAB_StartMethod=1;

								Port_State=PortStatusNum_Run;
								Port_RunStatus=PortStatusNum_Run;
							}
						}
					}
				}
			}					
		}break;	

		
		default:break;
	}
}
#endif

/****************************************************************************/
/*                                                                          */
/*              Stop flow of the system                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧停机类型撤销脉冲并处理开关断开及延时
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void Stop_flow(void)
{
	uchar tt_stop; /* tt_stop：原停止流程的临时状态字。 */
	ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */
	tt_stop=0;
		
	//if(FaultDisposeFLAG.Bbits.FaultDisposeType!=2)
	//{
	//	return;
	//}
		
//ControlFLAG.Bbits.DAB_StartMethod=0;
	
	switch(Port_RunStatus)
	{
		case PortStatusNum_Stop:
		{
			ControlFLAG.Bbits.Port_Stop=0x01;
			ControlFLAG.Bbits.Port_Start=0x00;
			if(Port_MainStatus.Bbits.P_M_Mode==0x03)
			{
				if(FaultDisposeFLAG.Bbits.Port_StopType==0x01)   //normal shutdown
				{					
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					PWMPluse_Disable(0);
					Port_StateUpdate_Write();	
					
					SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  SW.Bbits.Flag_SwitchZhi=0; 
			    SW.Bbits.Flag_SwitchRun=0;
			    SW.Bbits.Flag_SwitchSuccess=0;
			    SW.Bbits.Flag_SwitchTable=3;			
			    SWTable_TotalTime=SwitchTotalNum;
			    
			    Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;				
				}
				else if((FaultDisposeFLAG.Bbits.Port_StopType==0x02)||(ControlFLAG.Bbits.Port_Start==0x03))  //system shutdown
				{
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					PWMPluse_Disable(0);
										
					SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  SW.Bbits.Flag_SwitchZhi=0; 
			    SW.Bbits.Flag_SwitchRun=0;
			    SW.Bbits.Flag_SwitchSuccess=0;
			    SW.Bbits.Flag_SwitchTable=3;			
			    SWTable_TotalTime=SwitchTotalNum;
			    
			    Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;
				}
				else if(FaultDisposeFLAG.Bbits.Port_StopType==0x03)  //Remote-control shutdown
				{
					if(PORT_Index==1)
					{
						tt_stop=1;
						if((Port_OnState&0x0004)&&(!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}
								
						if((Port_OnState&0x0002)&&(!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}

						if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}						
					}
					else if(PORT_Index==2)
					{
						tt_stop=1;
						if((Port_OnState&0x0008)&&(!((MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}
								
						if((Port_OnState&0x0002)&&(!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}

						if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}		
					}
					else if(PORT_Index==3)
					{
						tt_stop=1;
						if((Port_OnState&0x0008)&&(!((MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}
								
						if((Port_OnState&0x0004)&&(!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}

						if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}		
					}
					else if(PORT_Index==4)
					{
						tt_stop=1;
						if((Port_OnState&0x0008)&&(!((MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVAC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}
								
						if((Port_OnState&0x0004)&&(!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}

						if((Port_OnState&0x0002)&&(!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose))))
						{
							tt_stop=0;
						}
					}	
					else{}		
						
					if(tt_stop)
					{
						FaultDisposeFLAG.Bbits.PWMPulseState=3;
						PWMPluse_Disable(0);
									
						SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  	SW.Bbits.Flag_SwitchZhi=0; 
			    	SW.Bbits.Flag_SwitchRun=0;
			    	SW.Bbits.Flag_SwitchSuccess=0;
			    	SW.Bbits.Flag_SwitchTable=3;			
			    	SWTable_TotalTime=SwitchTotalNum;
			    
			    	Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;	
			    }
			    else
			    {
			    	if((Flag_StopWait==0)&&(Flag_StopWaitEnd==0))
			    	{
			    		Flag_StopWait=1;
	
							InitialTemp=4-PORT_Index;
			    		coutStopWait=20+InitialTemp*10;
			    	}
			    	if((Flag_StopWait==0)&&(Flag_StopWaitEnd==1))
			    	{
			    		FaultDisposeFLAG.Bbits.PWMPulseState=3;
					    PWMPluse_Disable(0);
									
							SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  		SW.Bbits.Flag_SwitchZhi=0; 
			    		SW.Bbits.Flag_SwitchRun=0;
			    		SW.Bbits.Flag_SwitchSuccess=0;
			    		SW.Bbits.Flag_SwitchTable=3;			
			    		SWTable_TotalTime=SwitchTotalNum;
			    
			    		Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;			    		
			    	}			    	
			    }				
				}
				else
				{
					
				}
			}
			else
			{
				if(FaultDisposeFLAG.Bbits.Port_StopType==0x01)   //normal shutdown
				{					
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					PWMPluse_Disable(0);
					if(FaultDisposeFLAG.Bbits.Port_StopAll)
					{
						Port_StateUpdate_Write();
					}			
					SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  SW.Bbits.Flag_SwitchZhi=0; 
			    SW.Bbits.Flag_SwitchRun=0;
			    SW.Bbits.Flag_SwitchSuccess=0;
			    SW.Bbits.Flag_SwitchTable=3;			
			    SWTable_TotalTime=SwitchTotalNum;
			    
			    Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;				
				}
				else if((FaultDisposeFLAG.Bbits.Port_StopType==0x02)||(ControlFLAG.Bbits.Port_Start==0x03))  //system shutdown
				{
					FaultDisposeFLAG.Bbits.PWMPulseState=3;
					PWMPluse_Disable(0);
										
					SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  SW.Bbits.Flag_SwitchZhi=0; 
			    SW.Bbits.Flag_SwitchRun=0;
			    SW.Bbits.Flag_SwitchSuccess=0;
			    SW.Bbits.Flag_SwitchTable=3;			
			    SWTable_TotalTime=SwitchTotalNum;
			    
			    Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;
				}
				else if(FaultDisposeFLAG.Bbits.Port_StopType==0x03)  //Remote-control shutdown
				{				
					if(MainStatus_HVAC.Bbits.P_M_Mode==0x03)
					{
						if(PORT_Index==2)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0002)&&(!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVDC_STOP==1))
								{
									tt_stop=0;
								}

								if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVAC_STOP==1))
								{
									tt_stop=0;
								}
						}
						else if(PORT_Index==3)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVAC_STOP==1))
								{
									tt_stop=0;
								}						
						}
						else if(PORT_Index==4)
						{
								tt_stop=1;
						}	
						else{}								
					}
					else if(MainStatus_HVDC.Bbits.P_M_Mode==0x03)
					{	
						if(PORT_Index==1)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0002)&&(!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVDC_STOP==1))
								{
									tt_stop=0;
								}

								if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVAC_STOP==1))
								{
									tt_stop=0;
								}
						}
						else if(PORT_Index==3)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVAC_STOP==1))
								{
									tt_stop=0;
								}						
						}
						else if(PORT_Index==4)
						{
								tt_stop=1;
						}	
						else{}	
					}	
					else if(MainStatus_LVDC.Bbits.P_M_Mode==0x03)
					{	
						if(PORT_Index==1)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0004)&&(!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.HVDC_STOP==1))
								{
									tt_stop=0;
								}

								if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVAC_STOP==1))
								{
									tt_stop=0;
								}
						}
						else if(PORT_Index==2)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0001)&&(!((MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVAC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVAC_STOP==1))
								{
									tt_stop=0;
								}						
						}
						else if(PORT_Index==4)
						{
								tt_stop=1;
						}	
						else{}			
					}	
					else if(MainStatus_LVAC.Bbits.P_M_Mode==0x03)
					{	
						if(PORT_Index==1)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0004)&&(!((MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_HVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.HVDC_STOP==1))
								{
									tt_stop=0;
								}

								if((Port_OnState&0x0002)&&(!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVDC_STOP==1))
								{
									tt_stop=0;
								}
						}
						else if(PORT_Index==2)
						{
								tt_stop=1;
								
								if((Port_OnState&0x0002)&&(!((MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Standby)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_Initial)||(MainStatus_LVDC.Bbits.P_Status==PortStatusNum_PluseClose)))&&(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVDC_STOP==1))
								{
									tt_stop=0;
								}					
						}
						else if(PORT_Index==3)
						{
								tt_stop=1;
						}	
						else{}		
					}						
					if(tt_stop)
					{
						FaultDisposeFLAG.Bbits.PWMPulseState=3;
						PWMPluse_Disable(0);									
						SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  	SW.Bbits.Flag_SwitchZhi=0; 
			    	SW.Bbits.Flag_SwitchRun=0;
			    	SW.Bbits.Flag_SwitchSuccess=0;
			    	SW.Bbits.Flag_SwitchTable=3;			
			    	SWTable_TotalTime=SwitchTotalNum;			    
			    	Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;	
			    }
			    else
			    {
			    	if((Flag_StopWait==0)&&(Flag_StopWaitEnd==0))
			    	{
			    		Flag_StopWait=1;
	
							InitialTemp=4-PORT_Index;
			    		coutStopWait=20+InitialTemp*10;
			    	}
			    	if((Flag_StopWait==0)&&(Flag_StopWaitEnd==1))
			    	{
			    		FaultDisposeFLAG.Bbits.PWMPulseState=3;
							PWMPluse_Disable(0);
									
							SW.Bbits.Flag_Switch=3;                       //open EXT_CB
				  		SW.Bbits.Flag_SwitchZhi=0; 
			    		SW.Bbits.Flag_SwitchRun=0;
			    		SW.Bbits.Flag_SwitchSuccess=0;
			    		SW.Bbits.Flag_SwitchTable=3;			
			    		SWTable_TotalTime=SwitchTotalNum;
			    
			    		Port_RunStatus=PortStatusNum_Stop_EXTBreakerOff;		    		
			    	}			    	
			    }			    				
				}
				else
				{					
				}				
			}
		}break;
		
		case PortStatusNum_Stop_EXTBreakerOff:		
		{	
			Flag_StopWait=0;
			Flag_StopWaitEnd=0;
			FaultDisposeFLAG.Bbits.PWMPulseState=3;
			PWMPluse_Disable(0);
					
			if(SW.Bbits.Flag_SwitchRun==0)
			{
				if((SW.Bbits.Flag_SwitchSuccess==0)&&(SWTable_TotalTime==0))
				{
					Port_RunStatus=PortStatusNum_Standby;
					Port_State=PortStatusNum_Standby;
					
					ControlFLAG.Bbits.Port_Stop=0x03;	
					Port_RealFault.Bbits.PORT_FAULT_bit15=1;
					
					ControlFLAG.Bbits.Port_FaultDispose=0;
				}
				else
				{
					if(SW.Bbits.Flag_SwitchSuccess==1)
					{
						SW.Bbits.Flag_Switch=1;                       //open Contactor
				  	SW.Bbits.Flag_SwitchZhi=0; 
			    	SW.Bbits.Flag_SwitchRun=0;
			    	SW.Bbits.Flag_SwitchSuccess=0;
			    	SW.Bbits.Flag_SwitchTable=3;			
			    	SWTable_TotalTime=SwitchTotalNum;	
						Port_RunStatus=PortStatusNum_Stop_ContactorOff;
					}				
				}
			}			
		}break; 
		
		case PortStatusNum_Stop_ContactorOff:
		{
			if(SW.Bbits.Flag_SwitchRun==0)
			{
				if((SW.Bbits.Flag_SwitchSuccess==0)&&(SWTable_TotalTime==0))
				{
					Port_RunStatus=PortStatusNum_Standby;
					Port_State=PortStatusNum_Standby;
					
					ControlFLAG.Bbits.Port_Stop=0x03;	
					Port_RealFault.Bbits.PORT_FAULT_bit15=1;
					
					ControlFLAG.Bbits.Port_FaultDispose=0;
				}
				else
				{
					if(SW.Bbits.Flag_SwitchSuccess==1)
					{
						ControlFLAG.Bbits.Port_Stop=0x02;
						Port_RunStatus=PortStatusNum_Standby;
						Port_State=PortStatusNum_Standby;	
						
						ControlFLAG.Bbits.Port_FaultDispose=0;
					}				
				}		
			}
		}break;
	}
	
	if(ControlFLAG.Bbits.Port_Stop!=1)     //??????????????????若考虑故障复归，FaultDisposeFLAG.Bbits.FaultDisposeType如何清理
	{
		FaultDisposeFLAG.Bbits.FaultDisposeType=0;
		FaultDisposeFLAG.Bbits.PWMPulseState=0;
	}		
}


#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*              Switch operation table at startup                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 建立所选端口的开关操作时序与初始状态
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void SwitchTable_Start(void) 
{
	ushort PortAndSwitch_Status; /* PortAndSwitch_Status：建立开关动作表时的端口与开关综合状态。 */
	ushort Switch_StatusTemp; /* Switch_StatusTemp：原开关状态位图的临时解码值。 */
	
	PortAndSwitch_Status=PortRunFLAG.Bbits.Port_SigUdc;                 //预充完成
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	PortAndSwitch_Status=PortAndSwitch_Status|Port_Outer_Elec;          //端口有电
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB_ON_EN;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;        //外部断路器闭合使能
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB;                                //外部断路器状态
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;	
	
	Switch_StatusTemp=SW.Bbits.S_INT_CB;                                //内部断路器状态
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;	
		
	Switch_StatusTemp=SW.Bbits.S_CONTACTOR;                             //接触器状态
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
//	PortAndSwitch_Status=PortAndSwitch_Status<<1;	

		
	if((PortAndSwitch_Status&0x18)==0x08)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}
	}
	
	else if((PortAndSwitch_Status&0x38)==0x38)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}		
	}

	else if(PortAndSwitch_Status==0x18)
	{
		SW.Bbits.Flag_Switch=2;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}		
	
	else if(PortAndSwitch_Status==0x19)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1A)
	{
		SW.Bbits.Flag_Switch=3;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1B)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1C)
	{
		SW.Bbits.Flag_Switch=2;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1D)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1E)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//110
	}
	
	else if(PortAndSwitch_Status==0x1F)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//111     ???????????????????????????
	}

	else if((PortAndSwitch_Status&0x0C)==0x00)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=0;//Failed!
	}

	else if((PortAndSwitch_Status&0x1C)==0x04)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}
	}
	else if((PortAndSwitch_Status&0x3C)==0x34)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}
	}

	else if(PortAndSwitch_Status==0x14)
	{
		SW.Bbits.Flag_Switch=2;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}

	else if(PortAndSwitch_Status==0x15)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}

	else if(PortAndSwitch_Status==0x16)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//110
	}

	else if(PortAndSwitch_Status==0x17)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//111     ??????????????????????
	}

}
#endif

#ifdef HVDC_online
/****************************************************************************/
/*                                                                          */
/*              Switch operation table at startup                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 建立所选端口的开关操作时序与初始状态
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void SwitchTable_Start(void) 
{
	ushort PortAndSwitch_Status; /* PortAndSwitch_Status：建立开关动作表时的端口与开关综合状态。 */
	ushort Switch_StatusTemp; /* Switch_StatusTemp：原开关状态位图的临时解码值。 */
	
	PortAndSwitch_Status=PortRunFLAG.Bbits.Port_SigUdc;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	PortAndSwitch_Status=PortAndSwitch_Status|Port_Outer_Elec;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB_ON_EN;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;	
	
	Switch_StatusTemp=SW.Bbits.S_INT_CB;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;	
		
	Switch_StatusTemp=SW.Bbits.S_CONTACTOR;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
//	PortAndSwitch_Status=PortAndSwitch_Status<<1;	

		
	if((PortAndSwitch_Status&0x18)==0x08)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}
	}
	
	else if((PortAndSwitch_Status&0x38)==0x38)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}		
	}

	else if(PortAndSwitch_Status==0x18)
	{
		SW.Bbits.Flag_Switch=2;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}		
	
	else if(PortAndSwitch_Status==0x19)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1A)
	{
		SW.Bbits.Flag_Switch=3;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1B)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1C)
	{
		SW.Bbits.Flag_Switch=2;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1D)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x1E)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//110
	}
	
	else if(PortAndSwitch_Status==0x1F)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//111     ???????????????????????????
	}

	else if((PortAndSwitch_Status&0x0C)==0x00)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=0;//Failed!
	}

	else if((PortAndSwitch_Status&0x1C)==0x04)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}
	}

	else if((PortAndSwitch_Status&0x3C)==0x34)
	{
		if(SW.Bbits.S_INT_CB==0)
		{
			SW.Bbits.Flag_Switch=2;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//111
		}
	}

	else if(PortAndSwitch_Status==0x14)
	{
		SW.Bbits.Flag_Switch=2;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}

	else if(PortAndSwitch_Status==0x15)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}

	else if(PortAndSwitch_Status==0x16)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//110
	}

	else if(PortAndSwitch_Status==0x17)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//111     ??????????????????????
	}

}
#endif

#ifdef LVDC_online
/****************************************************************************/
/*                                                                          */
/*              Switch operation table at startup                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 建立所选端口的开关操作时序与初始状态
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void SwitchTable_Start(void) 
{
	ushort PortAndSwitch_Status; /* PortAndSwitch_Status：建立开关动作表时的端口与开关综合状态。 */
	ushort Switch_StatusTemp; /* Switch_StatusTemp：原开关状态位图的临时解码值。 */
	
	PortAndSwitch_Status=PortRunFLAG.Bbits.Port_SigUdc;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	PortAndSwitch_Status=PortAndSwitch_Status|Port_Outer_Elec;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB_ON_EN;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;	
	
	//Switch_StatusTemp=SW.Bbits.S_INT_CB;
	//PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	//PortAndSwitch_Status=PortAndSwitch_Status<<1;	
		
	Switch_StatusTemp=SW.Bbits.S_CONTACTOR;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
//	PortAndSwitch_Status=PortAndSwitch_Status<<1;	

		
	if((PortAndSwitch_Status&0x0C)==0x04)
	{
		if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}
	}
	
	else if((PortAndSwitch_Status&0x1C)==0x1C)
	{
		if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}		
	}

	else if(PortAndSwitch_Status==0x0C)
	{
		SW.Bbits.Flag_Switch=3;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}		
	
	else if(PortAndSwitch_Status==0x0D)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x0E)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//10
	}
	
	else if(PortAndSwitch_Status==0x0F)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//11   ?????????????????????????
	}

	else if((PortAndSwitch_Status&0x06)==0x00)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=0;//Failed!
	}

	else if((PortAndSwitch_Status&0x0E)==0x02)
	{
		if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}
	}

	else if((PortAndSwitch_Status&0x1E)==0x1A)
	{
		if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}
	}

	else if(PortAndSwitch_Status==0x0A)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//10
	}

	else if(PortAndSwitch_Status==0x0B)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//11     ??????????????????????????
	}
}
#endif

#ifdef LVAC_online
/****************************************************************************/
/*                                                                          */
/*              Switch operation table at startup                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 建立所选端口的开关操作时序与初始状态
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void SwitchTable_Start(void) 
{
	ushort PortAndSwitch_Status; /* PortAndSwitch_Status：建立开关动作表时的端口与开关综合状态。 */
	ushort Switch_StatusTemp; /* Switch_StatusTemp：原开关状态位图的临时解码值。 */
	
	PortAndSwitch_Status=PortRunFLAG.Bbits.Port_SigUdc;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	PortAndSwitch_Status=PortAndSwitch_Status|Port_Outer_Elec;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB_ON_EN;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;
	
	Switch_StatusTemp=SW.Bbits.S_EXT_CB;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	PortAndSwitch_Status=PortAndSwitch_Status<<1;	
	
	//Switch_StatusTemp=SW.Bbits.S_INT_CB;
	//PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
	//PortAndSwitch_Status=PortAndSwitch_Status<<1;	
		
	Switch_StatusTemp=SW.Bbits.S_CONTACTOR;
	PortAndSwitch_Status=PortAndSwitch_Status|Switch_StatusTemp;
//	PortAndSwitch_Status=PortAndSwitch_Status<<1;	

		
	if((PortAndSwitch_Status&0x0C)==0x04)
	{
		if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}
	}
	
	else if((PortAndSwitch_Status&0x1C)==0x1C)
	{
		if(SW.Bbits.S_EXT_CB==0)
		{
			SW.Bbits.Flag_Switch=3;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;			
		}
		else if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}		
	}

	else if(PortAndSwitch_Status==0x0C)
	{
		SW.Bbits.Flag_Switch=3;
		SW.Bbits.Flag_SwitchZhi=1;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}		
	
	else if(PortAndSwitch_Status==0x0D)
	{
		SW.Bbits.Flag_Switch=1;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=3;
	}
	
	else if(PortAndSwitch_Status==0x0E)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//10
	}
	
	else if(PortAndSwitch_Status==0x0F)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//11   ?????????????????????????
	}

	else if((PortAndSwitch_Status&0x06)==0x00)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=0;//Failed!
	}

	else if((PortAndSwitch_Status&0x0E)==0x02)
	{
		if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}
	}

	else if((PortAndSwitch_Status&0x1E)==0x1A)
	{
		if(SW.Bbits.S_CONTACTOR==0)
		{
			SW.Bbits.Flag_Switch=1;
			SW.Bbits.Flag_SwitchZhi=1;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=3;				
		}
		else
		{
			SW.Bbits.Flag_Switch=0;
			SW.Bbits.Flag_SwitchZhi=0;
			SW.Bbits.Flag_SwitchRun=0;
			SW.Bbits.Flag_SwitchSuccess=0;
			SW.Bbits.Flag_SwitchTable=1;//11
		}
	}

	else if(PortAndSwitch_Status==0x0A)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=2;//10
	}

	else if(PortAndSwitch_Status==0x0B)
	{
		SW.Bbits.Flag_Switch=0;
		SW.Bbits.Flag_SwitchZhi=0;
		SW.Bbits.Flag_SwitchRun=0;
		SW.Bbits.Flag_SwitchSuccess=0;
		SW.Bbits.Flag_SwitchTable=1;//11     ??????????????????????????
	}
}
#endif
/****************************************************************************/
/*                                                                          */
/*              Operation of the breaker and the contactor                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 根据时序标志执行旧开关操作并判定完成
 * 输入参数: 无
 * 输出参数: Port_State、ControlFLAG、SW 及旧流程定时标志。
 * 返回值: 无
 * 执行说明: 保留的原端口状态流程；迁移 CHB/DAB 主流程使用 ChbDabControl 状态机。
 *================================================================*/
void SwitchOperation_Dispose(void)
{
	if(SW.Bbits.Flag_Switch==1)        //contactor
	{
		if(SW.Bbits.Flag_SwitchRun==0)
		{
			if(Flag_Switch500ms==0)
			{						
				if(SW.Bbits.Flag_SwitchZhi)    //ON
				{
					Port_DOUT.Bbits.Contactor_Inner=1;
				}
				else                           //OFF
				{
					Port_DOUT.Bbits.Contactor_Inner=0;
				}
				Port_IO_Out();
			
				Flag_Switch500ms=1;
				coutSwitch500ms=200;
			
				SW.Bbits.Flag_SwitchRun=1;
			}
		}
		else
		{
			if(Flag_Switch500ms==0)
			{
				SW.Bbits.Flag_SwitchRun=0;
				
				SWTable_TotalTime--;
				if(SWTable_TotalTime==0)
				{
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					SW.Bbits.Flag_SwitchSuccess=0;
					
					Port_RealFault.Bbits.PORT_FAULT_bit0=1;				
					Port_DOUT.Bbits.Contactor_Inner=0;
					Port_IO_Out();
				}
				else
				{
					Port_DOUT.Bbits.Contactor_Inner=0;
					Port_IO_Out();
					Flag_Switch500ms=1;
					coutSwitch500ms=200;				
				}
			}
			else
			{
				Port_DINRead(0);
				if(((SW.Bbits.S_CONTACTOR==1)&&(SW.Bbits.Flag_SwitchZhi==1))||((SW.Bbits.S_CONTACTOR==0)&&(SW.Bbits.Flag_SwitchZhi==0)))
				{
					SWTable_TotalTime=0;
					SW.Bbits.Flag_SwitchSuccess=1;
					SW.Bbits.Flag_SwitchRun=0;
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					Flag_Switch500ms=0;
					coutSwitch500ms=0;
				}
			}			
		}
	}
	else if(SW.Bbits.Flag_Switch==2)   //inner breaker
	{
		if(SW.Bbits.Flag_SwitchRun==0)
		{			
			if(Flag_Switch1s==0)
			{
				if(SW.Bbits.Flag_SwitchZhi)    //ON
				{
					Port_DOUT.Bbits.Breaker_InnerOn=1;
					Port_DOUT.Bbits.Breaker_InnerOff=0;
				}
				else                           //OFF
				{
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=1;
				}
				Port_IO_Out();
				Flag_Switch1s=1;
				coutSwitch1s=300;
				
				SW.Bbits.Flag_SwitchRun=1;
			}
		}
		else
		{
			if(Flag_Switch1s==0)
			{
				SW.Bbits.Flag_SwitchRun=0;

				SWTable_TotalTime--;
				if(SWTable_TotalTime==0)
				{
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					SW.Bbits.Flag_SwitchSuccess=0;
					Port_RealFault.Bbits.PORT_FAULT_bit1=1;
					
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=0;	
					Port_IO_Out();
				}
				else
				{
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=0;	
					Port_IO_Out();	
					Flag_Switch1s=1;
					coutSwitch1s=300;				
				}
			}
			else
			{
				Port_DINRead(0);
				if(((SW.Bbits.S_INT_CB==1)&&(SW.Bbits.Flag_SwitchZhi==1))||((SW.Bbits.S_INT_CB==0)&&(SW.Bbits.Flag_SwitchZhi==0)))
				{
					SWTable_TotalTime=0;
					SW.Bbits.Flag_SwitchSuccess=1;
					SW.Bbits.Flag_SwitchRun=0;
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					Flag_Switch1s=0;
					coutSwitch1s=0;			
					
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=0;	
					Port_IO_Out();
				}
			}
		}
	}
//edited by shiqi, start, get the outer breaker back, instead of inner breaker
	else if(SW.Bbits.Flag_Switch==3)   //outer breaker
	{
#ifdef LVDC_online
		if(SW.Bbits.Flag_SwitchRun==0)
		{			
			if(Flag_Switch1s==0)
			{
				if(SW.Bbits.Flag_SwitchZhi)    //ON
				{
					Port_DOUT.Bbits.Breaker_OuterOn=1;
					Port_DOUT.Bbits.Breaker_OuterOff=0;
				}
				else                           //OFF
				{
					Port_DOUT.Bbits.Breaker_OuterOn=0;
					Port_DOUT.Bbits.Breaker_OuterOff=1;
				}
				Port_IO_Out();
				Flag_Switch1s=1;
				coutSwitch1s=300;			
					
				SW.Bbits.Flag_SwitchRun=1;
			}
		}
		else
		{
			if(Flag_Switch1s==0)
			{
				SW.Bbits.Flag_SwitchRun=0;
				
				SWTable_TotalTime--;
				if(SWTable_TotalTime==0)
				{
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					SW.Bbits.Flag_SwitchSuccess=0;
					Port_RealFault.Bbits.PORT_FAULT_bit1=1;
					
					Port_DOUT.Bbits.Breaker_OuterOn=0;
					Port_DOUT.Bbits.Breaker_OuterOff=0;
					Port_IO_Out();
				}
				else
				{
					Port_DOUT.Bbits.Breaker_OuterOn=0;
					Port_DOUT.Bbits.Breaker_OuterOff=0;
					Port_IO_Out();
					Flag_Switch1s=1;
					coutSwitch1s=300;						
				}
			}
			else
			{
				Port_DINRead(0);
				if(((SW.Bbits.S_EXT_CB==1)&&(SW.Bbits.Flag_SwitchZhi==1))||((SW.Bbits.S_EXT_CB==0)&&(SW.Bbits.Flag_SwitchZhi==0)))
				{
					SWTable_TotalTime=0;
					SW.Bbits.Flag_SwitchSuccess=1;
					SW.Bbits.Flag_SwitchRun=0;
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					Flag_Switch1s=0;
					coutSwitch1s=0;			
					
					Port_DOUT.Bbits.Breaker_OuterOn=0;
					Port_DOUT.Bbits.Breaker_OuterOff=0;
					Port_IO_Out();
				}
			}
		}
#endif

#ifdef HVAC_online
		if(SW.Bbits.Flag_SwitchRun==0)
		{			
			if(Flag_Switch1s==0)
			{
				if(SW.Bbits.Flag_SwitchZhi)    //ON
				{
					Port_DOUT.Bbits.Breaker_InnerOn=1;
					Port_DOUT.Bbits.Breaker_InnerOff=0;
				}
				else                           //OFF
				{
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=1;
				}
				Port_IO_Out();
				Flag_Switch1s=1;
				coutSwitch1s=300;
				
				SW.Bbits.Flag_SwitchRun=1;
			}
		}
		else
		{
			if(Flag_Switch1s==0)
			{
				SW.Bbits.Flag_SwitchRun=0;

				SWTable_TotalTime--;
				if(SWTable_TotalTime==0)
				{
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					SW.Bbits.Flag_SwitchSuccess=0;
					Port_RealFault.Bbits.PORT_FAULT_bit1=1;
					
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=0;	
					Port_IO_Out();
				}
				else
				{
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=0;	
					Port_IO_Out();	
					Flag_Switch1s=1;
					coutSwitch1s=300;				
				}
			}
			else
			{
				Port_DINRead(0);
				if(((SW.Bbits.S_INT_CB==1)&&(SW.Bbits.Flag_SwitchZhi==1))||((SW.Bbits.S_INT_CB==0)&&(SW.Bbits.Flag_SwitchZhi==0)))
				{
					SWTable_TotalTime=0;
					SW.Bbits.Flag_SwitchSuccess=1;
					SW.Bbits.Flag_SwitchRun=0;
					SW.Bbits.Flag_Switch=0;
					SW.Bbits.Flag_SwitchZhi=0;
					Flag_Switch1s=0;
					coutSwitch1s=0;			
					
					Port_DOUT.Bbits.Breaker_InnerOn=0;
					Port_DOUT.Bbits.Breaker_InnerOff=0;	
					Port_IO_Out();
				}
			}
		}
#endif
//edited by shiqi, end
	}
	else
	{}
}

		
