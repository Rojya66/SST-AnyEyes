#include "SstControlConfig.h"
//###########################################################################
// FILE: PwmGate.c
// FUNCTION: 正常启停与快速保护公用的最终PWM闸门执行层。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#include "ControlData.h"
#include "ControlInit.h"
#include "DpramHostData.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SoftTimeData.h"
#include "SstBoardConfig.h"
#include "SstTypes.h"

#include "PwmGate.h"
#include "FpgaIo.h"
#include "hw_types.h"

#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Initial                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化原 PWM 使能及故障处置控制字
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluseEnable_Initial(void)
{
    /* 上电安全态：DSP shutdown、光纤 shutdown、CHB/DAB PWM 状态均置为封锁。 */
    Port_DSPControl_AllShutdown(1); //dsp_ctl shutdown灭
    Port_ModuleShutdown(3, 0, 0, 0, 0, 0); //光纤输出shutdown亮
    Module_PWMEN(0);

    FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0; //shutdown光纤灭
    PortRunFLAG.Bbits.PWMState_50Hz = 1;
    PortRunFLAG.Bbits.PWMState_Dab = 1;
    PortRunFLAG.Bbits.PWMState_Module = 1;
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Enable                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略使能指定桥类型的 PWM
 * 输入参数: HbridgeIndexEn：旧 PWM 桥类型/使能选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Enable(uchar HbridgeIndexEn)
{
    /*
     * PWM 开锁入口：
     * 1 = CHB 50 Hz H 桥；2 = DAB H 桥；3 = 整端口。
     * 第一次开锁时还要释放 DSP 总 shutdown，之后只打开对应桥，
     * 因而 CHB 和 DAB 可以在开环模式下独立验证。
     */
    if (HbridgeIndexEn == 1) //50Hz H-bridge PWM Enable
    {
        if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical) //shutdown光纤亮，DSP_CTL和光纤输出shutdown无效
        {
            Module_50Hz_PWMEN(1);
        }
        else
        {
            Module_PWMEN(1);
            Port_DSPControl_AllShutdown(0);
            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        }
        PortRunFLAG.Bbits.PWMState_50Hz = 0;
        Variable_RST();
    }
    else if (HbridgeIndexEn == 2) //Dab H-bridge PWM Enable
    {
        if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical)
        {
            Module_ENdab_PWMEN(1);
        }
        else
        {
            Module_PWMEN(2);
            Port_DSPControl_AllShutdown(0);
            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        }
        PortRunFLAG.Bbits.PWMState_Dab = 0;
    }
    else if (HbridgeIndexEn == 3) //Port PWM Enable
    {
        Module_PWMEN(3);
        Port_DSPControl_AllShutdown(0);
        FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;

        PortRunFLAG.Bbits.PWMState_50Hz = 0;
        PortRunFLAG.Bbits.PWMState_Dab = 0;
        Variable_RST();
    }
    else
    {
    }
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Disable                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略禁用指定模块或指定桥类型的 PWM
 * 输入参数: IndexDis：旧 PWM 禁用类型/范围选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Disable(uchar IndexDis)
{
    /*
     * PWM 封锁由 FaultDisposeFLAG.Bbits.PWMPulseState 指定范围：
     * 3 = CHB+DAB 全部封锁；1 = 只封锁 CHB 50 Hz；2 = 只封锁 DAB。
     * IndexDis=0 表示按上述状态执行端口/桥封锁；非零值是历史的单模块封锁接口。
     */
    if (IndexDis == 0) //no module PWM disable
    {
        if (FaultDisposeFLAG.Bbits.PWMPulseState == 0x03) //Port PWM disable
        {
            /* 端口级封锁：撤销模块 PWM、拉起总 shutdown，并将 DAB 占空比恢复初值。 */
            Module_PWMEN(0); //避免shutdown光纤通信失败
            Port_DSPControl_AllShutdown(1);

            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0; //shutdown光纤灭
            PortRunFLAG.Bbits.PWMState_50Hz = 1;
            PortRunFLAG.Bbits.PWMState_Dab = 1;

            ControlFLAG.Bbits.DAB_StartMethod = 0; //变脉宽

            FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = 350;
            FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = 350;

            FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_RIGHTDuty = 350;
            FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_RIGHTDuty = 350;

            FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_RIGHTDuty = 350;
            FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_RIGHTDuty = 350;
        }
        else if (FaultDisposeFLAG.Bbits.PWMPulseState == 0x01) //50Hz H-bridge PWM disable
        {
            /* 仅封锁 CHB 50 Hz 桥，DAB 状态保持不变。 */
            Module_50Hz_PWMEN(0);
            PortRunFLAG.Bbits.PWMState_50Hz = 1;

            //		ControlFLAG.Bbits.DAB_StartMethod=0;
        }
        else if (FaultDisposeFLAG.Bbits.PWMPulseState == 0x02) //Dab H-bridge PWM disable
        {
            /* 仅封锁 DAB 桥，同时把左右桥占空比恢复到 350 初始值。 */
            Module_ENdab_PWMEN(0);
            PortRunFLAG.Bbits.PWMState_Dab = 1;
            ControlFLAG.Bbits.DAB_StartMethod = 0;

            FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = 350;
            FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = 350;

            FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_B1_PluginUnit_9 - 2].DAB_RIGHTDuty = 350;
            FPGA_Interface[Com_B2_PluginUnit_10 - 2].DAB_RIGHTDuty = 350;

            FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_LEFTDuty = 350;
            FPGA_Interface[Com_C1_PluginUnit_13 - 2].DAB_RIGHTDuty = 350;
            FPGA_Interface[Com_C2_PluginUnit_14 - 2].DAB_RIGHTDuty = 350;
        }
        else
        {
        }
    }
}

#endif

#ifdef HVDC_online
/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Initial                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化原 PWM 使能及故障处置控制字
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluseEnable_Initial(void)
{
    Port_DSPControl_AllShutdown(1);
    Module_PWMEN(0);

    FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0;
    PortRunFLAG.Bbits.PWMState_50Hz = 1;
    PortRunFLAG.Bbits.PWMState_Dab = 1;
    PortRunFLAG.Bbits.PWMState_Module = 1;
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Enable                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略使能指定桥类型的 PWM
 * 输入参数: HbridgeIndexEn：旧 PWM 桥类型/使能选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Enable(uchar HbridgeIndexEn)
{
    if (HbridgeIndexEn == 3) //Port PWM Enable
    {
        Port_DSPControl_AllShutdown(0);
        Port_ModuleShutdown(0, 0, 0, 0, 0, 0);
        Module_PWMEN(3);
        FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        PortRunFLAG.Bbits.PWMState_Dab = 0;
        PortRunFLAG.Bbits.PWMState_Module = 0;

        Variable_RST();
    }
    else
    {
    }
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Disable                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略禁用指定模块或指定桥类型的 PWM
 * 输入参数: IndexDis：旧 PWM 禁用类型/范围选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Disable(uchar IndexDis)
{
    //	uchar indextemp;
    if (IndexDis == 0) //Dab H-Bridge PWM disable
    {
        Module_PWMEN(0);
        Port_DSPControl_AllShutdown(1);
        FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0;
        PortRunFLAG.Bbits.PWMState_Dab = 1;

        HD_DAB_PluseStart = 1250;
        if ((Port_Main_WorkMode == 3) || (flag_INV == 1))
        {
            HD_DABDuty = 350;
        }
        else
        {
            HD_DABDuty = 2500;
        }
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_Start = HD_DAB_PluseStart;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_Start = HD_DAB_PluseStart;

        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_LEFTDuty = HD_DABDuty;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_LEFTDuty = HD_DABDuty;

        FPGA_Interface[Com_A1_PluginUnit_5 - 2].DAB_RIGHTDuty = HD_DABDuty;
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].DAB_RIGHTDuty = HD_DABDuty;
    }
    /*
    else if(IndexDis<16)  //Module PWM disable
    {
        indextemp=0x0001;
        indextemp=indextemp<<(IndexDis-1);

        Port_ModuleShutdown(0,indextemp,0,0,0,0);
        PortRunFLAG.Bbits.PWMState_Module=1;
    }
    else{}
*/
}
#endif

#ifdef LVDC_online
/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Initial                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化原 PWM 使能及故障处置控制字
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluseEnable_Initial(void)
{
    Port_DSPControl_AllShutdown(1);
    Module_PWMEN(0);

    FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0;
    PortRunFLAG.Bbits.PWMState_50Hz = 1;
    PortRunFLAG.Bbits.PWMState_Dab = 1;
    PortRunFLAG.Bbits.PWMState_Module = 1;
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Enable                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略使能指定桥类型的 PWM
 * 输入参数: HbridgeIndexEn：旧 PWM 桥类型/使能选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Enable(uchar HbridgeIndexEn)
{
    if (HbridgeIndexEn == 1) //BUCK PWM Enable
    {
        if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical)
        {
            Module_50Hz_PWMEN(1);
        }
        else
        {
            Module_PWMEN(1);
            Port_ModuleShutdown(0, 0, 0, 0, 0, 0);
            Port_DSPControl_AllShutdown(0);
            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        }
        PortRunFLAG.Bbits.PWMState_50Hz = 0;
    }
    else if (HbridgeIndexEn == 2) //DAB PWM Enable
    {
        if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical)
        {
            Module_ENdab_PWMEN(1);
        }
        else
        {
            Module_PWMEN(2);
            Port_ModuleShutdown(0, 0, 0, 0, 0, 0);
            Port_DSPControl_AllShutdown(0);
            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        }
        PortRunFLAG.Bbits.PWMState_Dab = 0;

        Variable_RST();
    }
    else if (HbridgeIndexEn == 3) //Port PWM Enable
    {
        Port_DSPControl_AllShutdown(0);
        Port_ModuleShutdown(0, 0, 0, 0, 0, 0);
        Module_PWMEN(3);
        FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        PortRunFLAG.Bbits.PWMState_Dab = 0;
        PortRunFLAG.Bbits.PWMState_Module = 0;
        PortRunFLAG.Bbits.PWMState_50Hz = 0;
    }
    else
    {
    }
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Disable                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略禁用指定模块或指定桥类型的 PWM
 * 输入参数: IndexDis：旧 PWM 禁用类型/范围选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Disable(uchar IndexDis)
{
    //	ushort indextemp;
    if (IndexDis == 0) //Dab H-Bridge PWM disable
    {
        Port_DSPControl_AllShutdown(1);
        Module_PWMEN(0);
        FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0;
        PortRunFLAG.Bbits.PWMState_Dab = 1;
        PortRunFLAG.Bbits.PWMState_50Hz = 1;
    }
    /*
    else if(IndexDis<16)  //Module PWM disable
    {
        indextemp=0x0001;
        indextemp=indextemp<<(IndexDis-1);

        Port_ModuleShutdown(0,indextemp,0,0,0,0);
        PortRunFLAG.Bbits.PWMState_Module=1;
    }
    else{}
*/
}
#endif

#ifdef LVAC_online
/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Initial                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化原 PWM 使能及故障处置控制字
 * 输入参数: 无
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluseEnable_Initial(void)
{
    Port_DSPControl_AllShutdown(1);

    Module_PWMEN(3);

    FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0;
    PortRunFLAG.Bbits.PWMState_50Hz = 1;
    PortRunFLAG.Bbits.PWMState_Dab = 1;
    PortRunFLAG.Bbits.PWMState_Module = 1;
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Enable                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略使能指定桥类型的 PWM
 * 输入参数: HbridgeIndexEn：旧 PWM 桥类型/使能选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Enable(uchar HbridgeIndexEn)
{
    if (HbridgeIndexEn == 1) //50Hz H-bridge PWM Enable
    {
        if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical)
        {
            Port_ModuleShutdown(4, 0, 0, 0, 0, 0);
        }
        else
        {
            Port_ModuleShutdown(0, 0x7fff, 0, 0, 0, 0);
            Port_ModuleShutdown(4, 0, 0, 0, 0, 0);
            Port_DSPControl_AllShutdown(0);
            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        }
        PortRunFLAG.Bbits.PWMState_50Hz = 0;
    }
    else if (HbridgeIndexEn == 2) //Dab H-bridge or Module PWM Enable
    {
        if (FaultDisposeFLAG.Bbits.PWMEN_FibreOptical)
        {
            Port_ModuleShutdown(0, 0, 0, 0, 0, 0);
        }
        else
        {
            Port_ModuleShutdown(4, 0, 0x0f, 0x0f, 0x0f, 0x0f);
            Port_ModuleShutdown(0, 0, 0, 0, 0, 0);
            Port_DSPControl_AllShutdown(0);
            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        }
        PortRunFLAG.Bbits.PWMState_Dab = 0;
        PortRunFLAG.Bbits.PWMState_Module = 0;
    }
    else if (HbridgeIndexEn == 3) //Port PWM Enable
    {
        Port_DSPControl_AllShutdown(0);
        Port_ModuleShutdown(4, 0, 0, 0, 0, 0);
        Port_ModuleShutdown(0, 0, 0, 0, 0, 0);
        FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 1;
        PortRunFLAG.Bbits.PWMState_50Hz = 0;
        PortRunFLAG.Bbits.PWMState_Dab = 0;
        PortRunFLAG.Bbits.PWMState_Module = 0;
    }
    else
    {
    }
}

/****************************************************************************/
/*                                                                          */
/*                      PWM Pluse Disable                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧故障处置策略禁用指定模块或指定桥类型的 PWM
 * 输入参数: IndexDis：旧 PWM 禁用类型/范围选择值
 * 输出参数: 原故障类型/锁存/处置全局量及 FPGA PWM 控制寄存器。
 * 返回值: 无
 * 执行说明: 原保护路径；保护阈值、确认次数、锁存及快速关断运算不因本次注释改变。
 *================================================================*/
void PWMPluse_Disable(uchar IndexDis)
{
    ushort indextemp; /* indextemp：PWM 禁用策略选出的模块/相索引。 */
    if (IndexDis == 0) //no module PWM disable
    {
        if (FaultDisposeFLAG.Bbits.PWMPulseState == 0x03) //Port PWM disable
        {
            Port_DSPControl_AllShutdown(1);
            FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0;
            PortRunFLAG.Bbits.PWMState_50Hz = 1;
            PortRunFLAG.Bbits.PWMState_Dab = 1;
        }
        else if (FaultDisposeFLAG.Bbits.PWMPulseState == 0x01) //50Hz H-bridge PWM disable
        {
            Port_ModuleShutdown(4, 0, 0x0f, 0x0f, 0x0f, 0x0f);
            PortRunFLAG.Bbits.PWMState_50Hz = 1;
        }
        else if (FaultDisposeFLAG.Bbits.PWMPulseState == 0x02) //Dab H-bridge PWM disable
        {
            Port_ModuleShutdown(0, 0x7fff, 0, 0, 0, 0);
            PortRunFLAG.Bbits.PWMState_Dab = 1;
        }
        else
        {
        }
    }
    else if (IndexDis < 16) //module PWM disable
    {
        indextemp = 0x0001;
        indextemp = indextemp << (IndexDis - 1);

        Port_ModuleShutdown(0, indextemp, 0, 0, 0, 0);

        PortRunFLAG.Bbits.PWMState_Module = 1;
    }
}

#endif
