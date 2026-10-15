#include "PwmGate.h"
#include "SstControlConfig.h"
//###########################################################################
// FILE: FpgaModulation.c
// FUNCTION: 工频CHB和DAB调制/启动寄存器下发。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#include "ControlData.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "FpgaRegister.h"
#include "Ft3RecorderData.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstTypes.h"

#include "FpgaModulation.h"
#include "ChbBuildConfig.h"
#include "FpgaModulation.h"

float tr1, /* tr1：原 FPGA 通信/调制测试观察量 1。 */
    tr2, /* tr2：原 FPGA 通信/调制测试观察量 2。 */
    tr3; /* tr3：原 FPGA 通信/调制测试观察量 3。 */

/*==================================================================
 * 函数功能: 保留原调制测试入口，当前函数体内的测试代码被注释关闭
 * 输入参数: 无
 * 输出参数: 无；当前函数为空。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Modualation_Test(void)
{
    /*
    ushort ModualationTemp1,ModualationTemp2;
    ModualationTemp1=FPGA_Interface[Com_A2_PluginUnit_6-2].Debug_L;
    ModualationTemp2=FPGA_Interface[Com_A2_PluginUnit_6-2].Debug_R;
    ModualationTemp1=ModualationTemp1>>4;
    ModualationTemp2=ModualationTemp2>>4;

    DAC_BUF[0]=ModualationTemp2;
  DAC_BUF[1]=ModualationTemp1;

    FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA0=(unsigned short)DAC_BUF[0];
    FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA1=(unsigned short)DAC_BUF[1];
    FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA2=(unsigned short)DAC_BUF[2];
    FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA3=(unsigned short)DAC_BUF[3];
*/
}

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{
    float ControlParaTemp_1; /* ControlParaTemp_1：参考归一化使用的模块直流电压/基准缓存。 */
    float ControlParaTemp_2; /* ControlParaTemp_2：三相工频桥参考缩放与移位的中间值。 */

    //Phase A
    /*
if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(PortRunFLAG.Bbits.Port_UdcOK==0)&&(Udc_Ref!=HA_Module_RefUdc))
{
    ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
}
else
{
    ControlParaTemp_1=SVG_Udc_A_Ave;
}
*/

    ControlParaTemp_1 = Udc_Ref; //HA_Module_RefUdc;
    //ControlParaTemp_1=SVG_Udc_A_Ave;

    if (flag_INV == 1)
    {
        ControlParaTemp_1 = SVG_Udc_A_Ave;
    }

    ControlParaTemp_2 = HA_Module_RefVolt_A + ControlParaTemp_1;

    if (ControlParaTemp_1 == 0.0)
    {
        ControlParaTemp_2 = 0.0;
    }
    else
    {
        ControlParaTemp_2 = ControlParaTemp_2 / ControlParaTemp_1;
    }

    if (ControlParaTemp_2 > 1.98)
    {
        ControlParaTemp_2 = 1.98;
    }
    else if (ControlParaTemp_2 < 0.02)
    {
        ControlParaTemp_2 = 0.02;
    }

    ControlParaTemp_1 = HA_ReferenceDEF_A;
    ControlParaTemp_2 = ControlParaTemp_2 * ControlParaTemp_1;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].Reference = (ushort)ControlParaTemp_2;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].Reference = (ushort)ControlParaTemp_2;

#ifdef COM_TEST_PluginUnit

    Wrong_ControlReference_A = (ushort)ControlParaTemp_2;
    if (PortRunFLAG.Bbits.PWMState_50Hz == 0)
    {
        if ((Wrong_ControlReference_A - LastWrong_ControlReference_A) > 1300)
        {
            WrongTimes_ControlReference_A++;
            if (WrongTimes_ControlReference_A == 65535)
            {
                WrongTimes_ControlReference_A = 0;
            }
        }
        else if ((Wrong_ControlReference_A - LastWrong_ControlReference_A) < (-1300))
        {
            WrongTimes_ControlReference_A++;
            if (WrongTimes_ControlReference_A == 65535)
            {
                WrongTimes_ControlReference_A = 0;
            }
        }
    }
    LastWrong_ControlReference_A = Wrong_ControlReference_A;

#endif

    if (HA_IG_Log_A == 0)
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.IG_Orient = 0;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.IG_Orient = 0;
    }
    else
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.IG_Orient = 1;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.IG_Orient = 1;
    }

    if (HA_DC_Log_A == 0)
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.DC_Order = 0;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.DC_Order = 1;
    }
    else
    {
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.Bbits.DC_Order = 1;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.Bbits.DC_Order = 0;
    }

    //Phase B
    /*
if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(PortRunFLAG.Bbits.Port_UdcOK==0)&&(Udc_Ref!=HA_Module_RefUdc))
{
    ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
}
else
{
    ControlParaTemp_1=SVG_Udc_B_Ave;
}
*/

    ControlParaTemp_1 = Udc_Ref; //HA_Module_RefUdc;
    //	ControlParaTemp_1=SVG_Udc_B_Ave;

    if (flag_INV == 1)
    {
        ControlParaTemp_1 = SVG_Udc_B_Ave;
    }

    ControlParaTemp_2 = HA_Module_RefVolt_B + ControlParaTemp_1;

    if (ControlParaTemp_1 == 0.0)
    {
        ControlParaTemp_2 = 0.0;
    }
    else
    {
        ControlParaTemp_2 = ControlParaTemp_2 / ControlParaTemp_1;
    }

    if (ControlParaTemp_2 > 1.98)
    {
        ControlParaTemp_2 = 1.98;
    }
    else if (ControlParaTemp_2 < 0.02)
    {
        ControlParaTemp_2 = 0.02;
    }

    ControlParaTemp_1 = HA_ReferenceDEF_B;
    ControlParaTemp_2 = ControlParaTemp_2 * ControlParaTemp_1;

    FPGA_Interface[Com_B1_PluginUnit_9 - 2].Reference = (ushort)ControlParaTemp_2;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].Reference = (ushort)ControlParaTemp_2;

    if (HA_IG_Log_B == 0)
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.IG_Orient = 0;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.IG_Orient = 0;
    }
    else
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.IG_Orient = 1;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.IG_Orient = 1;
    }

    if (HA_DC_Log_B == 0)
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.DC_Order = 0;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.DC_Order = 1;
    }
    else
    {
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].DC_Log.Bbits.DC_Order = 1;
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].DC_Log.Bbits.DC_Order = 0;
    }

    //Phase C
    /*
if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(PortRunFLAG.Bbits.Port_UdcOK==0)&&(Udc_Ref!=HA_Module_RefUdc))
{
    ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
}
else
{
    ControlParaTemp_1=SVG_Udc_C_Ave;
}
*/

    ControlParaTemp_1 = Udc_Ref; //HA_Module_RefUdc;
    //ControlParaTemp_1=SVG_Udc_C_Ave;

    if (flag_INV == 1)
    {
        ControlParaTemp_1 = SVG_Udc_C_Ave;
    }

    ControlParaTemp_2 = HA_Module_RefVolt_C + ControlParaTemp_1;

    if (ControlParaTemp_1 == 0.0)
    {
        ControlParaTemp_2 = 0.0;
    }
    else
    {
        ControlParaTemp_2 = ControlParaTemp_2 / ControlParaTemp_1;
    }

    if (ControlParaTemp_2 > 1.98)
    {
        ControlParaTemp_2 = 1.98;
    }
    else if (ControlParaTemp_2 < 0.02)
    {
        ControlParaTemp_2 = 0.02;
    }

    ControlParaTemp_1 = HA_ReferenceDEF_C;
    ControlParaTemp_2 = ControlParaTemp_2 * ControlParaTemp_1;

    FPGA_Interface[Com_C1_PluginUnit_13 - 2].Reference = (ushort)ControlParaTemp_2;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].Reference = (ushort)ControlParaTemp_2;

//	DAC_BUF[0]=(float)ControlParaTemp_2/14000.0;
#ifdef COM_TEST_PluginUnit
/*
            Wrong_ControlReference_C=(ushort)ControlParaTemp_2;
            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_ControlReference_C-LastWrong_ControlReference_C)>1300)
                {
                    WrongTimes_ControlReference_C++;
                    if(WrongTimes_ControlReference_C==65535)
                    {
                        WrongTimes_ControlReference_C=0;
                    }
                }
                else if((Wrong_ControlReference_C-LastWrong_ControlReference_C)<(-1300))
                {
                    WrongTimes_ControlReference_C++;
                    if(WrongTimes_ControlReference_C==65535)
                    {
                        WrongTimes_ControlReference_C=0;
                    }
                }
            }
            LastWrong_ControlReference_C=Wrong_ControlReference_C;
*/
#endif

    if (HA_IG_Log_C == 0)
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.IG_Orient = 0;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.IG_Orient = 0;
    }
    else
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.IG_Orient = 1;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.IG_Orient = 1;
    }

    if (HA_DC_Log_C == 0)
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.DC_Order = 0;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.DC_Order = 1;
    }
    else
    {
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].DC_Log.Bbits.DC_Order = 1;
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].DC_Log.Bbits.DC_Order = 0;
    }

    /*
    if(flag_SendDABRef_First==2)
    {
        #ifdef HVAC_online
        PWMPluse_Enable(1);
        #endif

        #ifdef LVAC_online
        PWMPluse_Enable(1);
        #endif
    }
*/
}
#endif /* HVAC_online */
#ifdef HVAC_online
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{
    //		HA_DABDuty_A=2500;
    //		HA_DAB_T=5000;
    HA_DAB_PluseStart_A = 1250;

    //		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Duty=HA_DABDuty_A;
    //		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Duty=HA_DABDuty_A;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = HA_DAB_PluseStart_A;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = HA_DAB_PluseStart_A;

    //		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_T=HA_DAB_T;
    //		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_T=HA_DAB_T;

    //		HA_DABDuty_B=2500;
    //		HA_DAB_T=5000;
    HA_DAB_PluseStart_B = 1250;
    //		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Duty=HA_DABDuty_B;
    //		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Duty=HA_DABDuty_B;

    FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_Start = HA_DAB_PluseStart_B;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_Start = HA_DAB_PluseStart_B;

    //		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_T=HA_DAB_T;
    //		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_T=HA_DAB_T;

    //		HA_DABDuty_C=2500;
    //		HA_DAB_T=5000;
    HA_DAB_PluseStart_C = 1250;
    //		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Duty=HA_DABDuty_C;
    //		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Duty=HA_DABDuty_C;

    FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_Start = HA_DAB_PluseStart_C;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_Start = HA_DAB_PluseStart_C;

    //		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_T=HA_DAB_T;
    //		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_T=HA_DAB_T;
}
#endif /* HVAC_online */
#ifdef HVAC_online
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{
    if (HA_DABDuty_A < 2400)
    {
        HA_DABDuty_A = HA_DABDuty_A + 1;

        if (HA_DABDuty_A == 2400)
        {
            ControlFLAG.Bbits.DAB_DutyStart = 0;
            ControlFLAG.Bbits.DAB_DutyOver = 1;
        }
    }
    //	HA_DAB_T=5000;
    //	HA_DAB_PluseStart_A=1250;
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = HA_DABDuty_A;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = HA_DABDuty_A;

    //	FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=HA_DAB_PluseStart_A;
    //	FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=HA_DAB_PluseStart_A;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = HA_DABDuty_A;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = HA_DABDuty_A;

    //		HA_DABDuty_B=2500;
    //		HA_DAB_T=5000;
    //		HA_DAB_PluseStart_B=1250;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_LEFTDuty = HA_DABDuty_A;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_LEFTDuty = HA_DABDuty_A;

    //		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Start=HA_DAB_PluseStart_B;
    //		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Start=HA_DAB_PluseStart_B;

    FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_RIGHTDuty = HA_DABDuty_A;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_RIGHTDuty = HA_DABDuty_A;

    //		HA_DABDuty_C=2500;
    //		HA_DAB_T=5000;
    //		HA_DAB_PluseStart_C=1250; // 保留的 C 相 DAB 起始相位测试计数，当前未启用。
    FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_LEFTDuty = HA_DABDuty_A;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_LEFTDuty = HA_DABDuty_A;

    //		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Start=HA_DAB_PluseStart_C;
    //		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Start=HA_DAB_PluseStart_C;

    FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_RIGHTDuty = HA_DABDuty_A;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_RIGHTDuty = HA_DABDuty_A;

    /*
        if(flag_SendDABRef_First==2)
        {
                #ifdef HVAC_online
                PWMPluse_Enable(2);
                #endif

                #ifdef HVDC_online
                PWMPluse_Enable(3);
                #endif

                #ifdef LVDC_online
                PWMPluse_Enable(3);
                #endif

                #ifdef LVAC_online
                PWMPluse_Enable(2);
                #endif
        }
*/
}
#endif /* HVAC_online */
#ifdef HVDC_online
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{
}
#endif /* HVDC_online */
#ifdef HVDC_online
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{
    uchar i; /* i：本次数组、参数或模块遍历的索引。 */

    HD_DABDuty = 2500;
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = HD_DAB_PluseStart;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = HD_DAB_PluseStart;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = HD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = HD_DABDuty;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = HD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = HD_DABDuty;

    //#ifdef HD_10KV_JY
    //		#ifdef HD_10KV_JY_single

    if (Volt_EvenMethod == 1)
    {
        for (i = 0; i < 8; i++)
        {
            FPGA_Interface[Com_A1_PluginUnit_5 - 2].HD_S_PH[i] = MK_HD_DAB_PluseStart[i];
        }

        for (i = 8; i < 15; i++)
        {
            FPGA_Interface[Com_A2_PluginUnit_6 - 2].HD_S_PH[i - 8] = MK_HD_DAB_PluseStart[i];
        }
    }

    if (Volt_EvenMethod == 2)
    {
        //		#else

        for (i = 0; i < 5; i++)
        {
            FPGA_Interface[Com_A1_PluginUnit_5 - 2].HD_S_PH[i] = MK_HD_DAB_PluseStart[0];
        }

        for (i = 5; i < 8; i++)
        {
            FPGA_Interface[Com_A1_PluginUnit_5 - 2].HD_S_PH[i] = MK_HD_DAB_PluseStart[1];
        }
        for (i = 0; i < 2; i++)
        {
            FPGA_Interface[Com_A2_PluginUnit_6 - 2].HD_S_PH[i] = MK_HD_DAB_PluseStart[1];
        }

        for (i = 2; i < 7; i++)
        {
            FPGA_Interface[Com_A2_PluginUnit_6 - 2].HD_S_PH[i] = MK_HD_DAB_PluseStart[2];
        }
    }
    //		#endif
    //#endif
}
#endif /* HVDC_online */
#ifdef HVDC_online
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{
    if (HD_DABDuty < 2500)
    {
        HD_DABDuty = HD_DABDuty + 1;
        if (HD_DABDuty == 2500)
        {
            ControlFLAG.Bbits.DAB_DutyStart = 0;
            ControlFLAG.Bbits.DAB_DutyOver = 1;
        }
    }

    HD_DAB_PluseStart = 1250;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = HD_DAB_PluseStart;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = HD_DAB_PluseStart;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = HD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = HD_DABDuty;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = HD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = HD_DABDuty;
}
#endif /* HVDC_online */
#ifdef LVDC_online
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{
}
#endif /* LVDC_online */
#ifdef LVDC_online
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{
    LD_DABDuty = 2500;
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = LD_DAB_PluseStart;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = LD_DAB_PluseStart;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = LD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = LD_DABDuty;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = LD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = LD_DABDuty;
}
#endif /* LVDC_online */
#ifdef LVDC_online
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{
    if (LD_DABDuty < 2500)
    {
        LD_DABDuty = LD_DABDuty + 1;
        if (LD_DABDuty == 2500)
        {
            ControlFLAG.Bbits.DAB_DutyStart = 0;
            ControlFLAG.Bbits.DAB_DutyOver = 1;
        }
    }

    LD_DAB_PluseStart = 1250;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = LD_DAB_PluseStart;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = LD_DAB_PluseStart;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = LD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = LD_DABDuty;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = LD_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = LD_DABDuty;
}
#endif /* LVDC_online */
#ifdef LVAC_online
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{
    short ReferenceTemp; /* ReferenceTemp：原 FPGA 调制参考换算缓存。 */
    float ControlParaTemp; /* ControlParaTemp：原工频桥参考换算的浮点中间值。 */

    ControlParaTemp = LA_RefVolt_A;
    ControlParaTemp = ControlParaTemp * 10.0;
    ReferenceTemp = (short)ControlParaTemp;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].INV_REF = ReferenceTemp;

    ControlParaTemp = LA_RefVolt_B;
    ControlParaTemp = ControlParaTemp * 10.0;
    ReferenceTemp = (short)ControlParaTemp;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].INV_REF = ReferenceTemp;

    ControlParaTemp = LA_RefVolt_C;
    ControlParaTemp = ControlParaTemp * 10.0;
    ReferenceTemp = (short)ControlParaTemp;
    FPGA_Interface[Putout_B_PluginUnit_11 - 2].INV_REF = ReferenceTemp;

    FPGA_Interface[Input_B_PluginUnit_12 - 2].INV_REF = 0;
}
#endif /* LVAC_online */
#ifdef LVAC_online
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{
    //		LA_DABDuty=2500;
    //		LA_DAB_T=5000;
    LA_DAB_PluseStart = 1250;
    //		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Duty=LA_DABDuty;
    //		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Duty=LA_DABDuty;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = LA_DAB_PluseStart;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = LA_DAB_PluseStart;

    //		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_T=LA_DAB_T;
    //		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_T=LA_DAB_T;
}
#endif /* LVAC_online */
#ifdef LVAC_online
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{
    if (LA_DABDuty < 2500)
    {
        LA_DABDuty = LA_DABDuty + 1;
        if (LA_DABDuty == 2500)
        {
            ControlFLAG.Bbits.DAB_DutyStart = 0;
            ControlFLAG.Bbits.DAB_DutyOver = 1;
        }
    }

    //		LA_DAB_T=5000;
    //		LA_DAB_PluseStart=1250;
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Duty = LA_DABDuty;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Duty = LA_DABDuty;

    //		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=LA_DAB_PluseStart;
    //		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=LA_DAB_PluseStart;

    //		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_T=LA_DAB_T;
    //		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_T=LA_DAB_T;
}
#endif /* LVAC_online */
