#include "PwmGate.h"
/* 文件职责: 原完整初始化/复位或巡检；保留全部可执行语句。 */
#include "ControlData.h"
#include "ModuleVoltageCheck.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "SstTypes.h"
#include "ADC_Sample.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 依次检查各相在线模块直流电压，更新预充及高压判断标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Udc_check(void)
{
    ushort index_temp; /* index_temp：本次模块电压检查使用的模块/相索引。 */
    ushort Check_temp_A,Check_temp_B,Check_temp_C; /* Check_temp_A：A 相模块电压巡检的位图/判定缓存。 */
    /* Check_temp_B：B 相模块电压巡检的位图/判定缓存。 */
    /* Check_temp_C：C 相模块电压巡检的位图/判定缓存。 */
    ushort temp_A,temp_B,temp_C; /* temp_A：A 相模块电压巡检的位图/判定缓存。 */
    /* temp_B：B 相模块电压巡检的位图/判定缓存。 */
    /* temp_C：C 相模块电压巡检的位图/判定缓存。 */
    float Udc_Temp; /* Udc_Temp：本次巡检读取的模块直流电压，单位 V。 */

//  if(Udc_Checked_Num<48)
//  {
//      Udc_Checked_Num++;
//  }

    if(Udc_Check_Index<48)
    {
        Udc_Check_Index++;
    }
    else
    {
        HA_A_LV_Precharge=0;
        HA_B_LV_Precharge=0;
        HA_C_LV_Precharge=0;

        HA_A_HV_Protect=0;
        HA_B_HV_Protect=0;
        HA_C_HV_Protect=0;

        Udc_Check_Index=1;
    }

    if(Udc_Check_Index<9)
    {
        Udc_Temp=Module_AD_Sample(Com_A1_PluginUnit_5,Udc_Check_Index,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<17)
    {
        Udc_Temp=Module_AD_Sample(Com_A2_PluginUnit_6,Udc_Check_Index-8,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<25)
    {
        Udc_Temp=Module_AD_Sample(Com_B1_PluginUnit_9,Udc_Check_Index-16,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<33)
    {
        Udc_Temp=Module_AD_Sample(Com_B2_PluginUnit_10,Udc_Check_Index-24,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<41)
    {
        Udc_Temp=Module_AD_Sample(Com_C1_PluginUnit_13,Udc_Check_Index-32,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<49)
    {
        Udc_Temp=Module_AD_Sample(Com_C2_PluginUnit_14,Udc_Check_Index-40,ModuleIndex_DCbus_Volt,0);
    }


    if(Udc_Check_Index<17)
    {
        index_temp=Udc_Check_Index-1;
        index_temp=0x0001<<index_temp;

        if(Udc_Temp>HA_H_Voltage)  //过压
        {
            HA_A_HV_Protect=HA_A_HV_Protect|index_temp;
        }

        if(Udc_Temp<HA_L_Voltage_Precharge)//欠压
        {
            HA_A_LV_Precharge=HA_A_LV_Precharge|index_temp;
        }
    }
    else if(Udc_Check_Index<33)
    {
        index_temp=Udc_Check_Index-17;
        index_temp=0x0001<<index_temp;

        if(Udc_Temp>HA_H_Voltage)  //过压
        {
            HA_B_HV_Protect=HA_B_HV_Protect|index_temp;
        }

        if(Udc_Temp<HA_L_Voltage_Precharge)//欠压
        {
            HA_B_LV_Precharge=HA_B_LV_Precharge|index_temp;
        }
    }
    else if(Udc_Check_Index<49)
    {
        index_temp=Udc_Check_Index-33;
        index_temp=0x0001<<index_temp;

        if(Udc_Temp>HA_H_Voltage)  //过压
        {
            HA_C_HV_Protect=HA_C_HV_Protect|index_temp;
        }

        if(Udc_Temp<HA_L_Voltage_Precharge)//欠压
        {
            HA_C_LV_Precharge=HA_C_LV_Precharge|index_temp;
        }
    }

    if(Udc_Check_Index==48)
    {
        Check_temp_A=HA_PM_A2;
        Check_temp_A=Check_temp_A<<8;
        Check_temp_A=Check_temp_A+HA_PM_A1;

        Check_temp_B=HA_PM_B2;
        Check_temp_B=Check_temp_B<<8;
        Check_temp_B=Check_temp_B+HA_PM_B1;

        Check_temp_C=HA_PM_C2;
        Check_temp_C=Check_temp_C<<8;
        Check_temp_C=Check_temp_C+HA_PM_C1;


        temp_A=HA_A_LV_Precharge;
        index_temp=Check_temp_A;
        temp_A=temp_A&index_temp;

        temp_B=HA_B_LV_Precharge;
        index_temp=Check_temp_B;
        temp_B=temp_B&index_temp;

        temp_C=HA_C_LV_Precharge;
        index_temp=Check_temp_C;
        temp_C=temp_C&index_temp;

        if(temp_A||temp_B||temp_C)
        {
            PortRunFLAG.Bbits.Port_SigUdc=0;   //欠压
        }
        else
        {
            PortRunFLAG.Bbits.Port_SigUdc=1;   //charge over
        }


        temp_A=HA_A_HV_Protect;
        index_temp=Check_temp_A;
        temp_A=temp_A&index_temp;

        temp_B=HA_B_HV_Protect;
        index_temp=Check_temp_B;
        temp_B=temp_B&index_temp;

        temp_C=HA_C_HV_Protect;
        index_temp=Check_temp_C;
        temp_C=temp_C&index_temp;

        if(temp_A||temp_B||temp_C)
        {
            PortRunFLAG.Bbits.Port_HighUdc=1;   //过压

            if((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))
            {
//              if(Port_Main_WorkMode==0x03)             //主电源，整个系统停机
//              {
//                  FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1;
//              }
//              else
//              {
//                  FaultDisposeFLAG.Bbits.Port_StopAll=0;
//              }

/*
                FaultDisposeFLAG.Bbits.Port_StopType=0;
                FaultDisposeFLAG.Bbits.FaultDisposeType=1;
                FaultDisposeFLAG.Bbits.PWMPulseState=3;

                PWMPluse_Disable(0);
*/
                //Port_RealFault.Bbits.PORT_FAULT_bit5=1;                           //过压
            }
        }
        else
        {
            PortRunFLAG.Bbits.Port_HighUdc=0;
        }
    }
}
#endif
