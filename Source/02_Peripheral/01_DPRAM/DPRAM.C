#include "FpgaIo.h"
#include "FpgaInit.h"
#include "SstPortStatus.h"
#include "PwmGate.h"
#include "ControlData.h"
#include "DpramConfig.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "SstTypes.h"
#include "ChbBuildConfig.h"
//###########################################################################
//
// FILE:   DPRAM.c
//
// FUNCTION:  Initialize legacy control factors and defaults
// AUTHOR: Wen Wusong;  Modified by Z.W
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################
#include "DPRAM.h"
#include "X1226_RW.h"
#include "FaultCheck.h"
#include "hw_types.h"
#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp, /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
        InitialTempFloat; /* InitialTempFloat：原控制系数和给定初始化的浮点计算缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_2_EN = PORT_Regulate_DATA.FO_2_EN.FO_EN16;
    Port_FO_3_EN = PORT_Regulate_DATA.FO_3_EN.FO_EN16;
    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET; //1kw=10
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0; //w

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET;
    PORT_Regulate_DATA.PORT_Q_SET = ParameterTemp * 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET;
    PORT_Regulate_DATA.PORT_F_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_5TH_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_7TH_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_11TH_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_5TH_SET = ParameterTemp / 1000.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_7TH_SET = ParameterTemp / 1000.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_11TH_SET = ParameterTemp / 1000.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET;
    PORT_Regulate_DATA.STATCOM_V_SET = ParameterTemp;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET;
    PORT_Regulate_DATA.STATCOM_PF_SET = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET; //1kw=10
    PORT_Regulate_DATA.STATCOM_Q_SET = ParameterTemp * 100.0; //w

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp;

    /**********************下垂频率截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET;
    PORT_Regulate_DATA.PORT_F0_SET = ParameterTemp / 10.0;

    /**********************频率下垂斜率**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET;
    PORT_Regulate_DATA.PORT_KF_SET = ParameterTemp / 10.0;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    Har_5_RMS = PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
    Har_5_PH = PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
    Har_7_RMS = PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
    Har_7_PH = PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
    Har_11_RMS = PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
    Har_11_PH = PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET / 200;

    /**************************电压参考**********************/
    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 600.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else
    {
        HA_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    UstOK_HVAC_Port = HA_Module_RefUdc;
    Udc_Ref = HA_Module_RefUdc;

    if (NHA_module_A == 0)
    {
        Ud_st_Max = 10.0;
        Ud_st_Min = -10.0;
        Uq_st_Max = 10.0;
        Uq_st_Min = -10.0;
    }
    else
    {
        InitialTempFloat = (float)NHA_module_A;
        InitialTempFloat = InitialTempFloat * 1.1;
        InitialTempFloat = InitialTempFloat * HA_Module_RefUdc;

        Ud_st_Max = InitialTempFloat;
        Ud_st_Min = InitialTempFloat * (-1.0);
        Uq_st_Max = Ud_st_Max;
        Uq_st_Min = Ud_st_Min;
    }

    Uout_d_Positive_Max = Ud_st_Max;
    Uout_d_Positive_Min = Ud_st_Min;
    Uout_q_Positive_Max = Uq_st_Max;
    Uout_q_Positive_Min = Uq_st_Min;

    /**************************电流范围**********************/

    if (PORT_Regulate_DATA.Reserved_1[3] > 95.0)
    {
        PORT_Regulate_DATA.Reserved_1[3] = 95.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3] = 95;
        Idst_Max = 95.0;
    }
    else if (PORT_Regulate_DATA.Reserved_1[3] < 10.0)
    {
        PORT_Regulate_DATA.Reserved_1[3] = 10.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3] = 10;
        Idst_Max = 10.0;
    }
    else
    {
        Idst_Max = PORT_Regulate_DATA.Reserved_1[3];
    }

    Idst_Min = (-1.0) * Idst_Max;
    Iqst_Max = Idst_Max;
    Iqst_Min = Idst_Min;

    Id_Positive_Max = Idst_Max;
    Id_Positive_Min = Idst_Min;
    Iq_Positive_Max = Iqst_Max;
    Iq_Positive_Min = Iqst_Min;
    /* 上电位图在FpgaInit中已写入，FO/FB待EEPROM恢复完成再统一开启。 */
    if (Inital_Y == 0U)
    {
        const ushort module_masks[6] = {
            (ushort)PORT_Regulate_DATA.Reserved_1[0],
            (ushort)PORT_Regulate_DATA.Reserved_1[1],
            (ushort)PORT_Regulate_DATA.PORT_V0_SET,
            (ushort)PORT_Regulate_DATA.PORT_F0_SET,
            (ushort)PORT_Regulate_DATA.PORT_KF_SET,
            (ushort)PORT_Regulate_DATA.PORT_KV_SET
        };
        FpgaInit_ApplyModuleLayout(module_masks);
        Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
        Port_FB_EN_Set();
    }

    Port_OnState = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2];
    HWREGH(0x6000062e) = Port_OnState;

    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;


    PLL_Factor=PORT_Regulate_DATA.PORT_V_P_1;
    if(PLL_Factor<0.01)
    {
        PLL_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
    }

    Volt_Factor=PORT_Regulate_DATA.PORT_V_I_1;
    if(Volt_Factor<0.01)
    {
        Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    }

    Current_Factor=PORT_Regulate_DATA.PORT_I_P_1;
    if(Current_Factor<0.01)
    {
        Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    }

    Neg_Current_Factor=PORT_Regulate_DATA.PORT_I_I_1;
    if(Neg_Current_Factor<0.01)
    {
        Neg_Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
    }

    PH_Volt_Factor=PORT_Regulate_DATA.PORT_V_P_2;
    if(PH_Volt_Factor<0.01)
    {
        PH_Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
    }

    Har_2_Factor=PORT_Regulate_DATA.PORT_V_I_2;
    if(Har_2_Factor<0.01)
    {
        Har_2_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
    }

    Har_6_Factor=PORT_Regulate_DATA.PORT_I_P_2;
    if(Har_6_Factor<0.01)
    {
        Har_6_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
    }

    Har_12_Factor=PORT_Regulate_DATA.PORT_I_I_2;
    if(Har_12_Factor<0.01)
    {
        Har_12_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
    }

//controller
    PLL_PID_Num_1=PLL_PID_Num_1_def/PLL_Factor;
    PLL_PID_Num_0=PLL_PID_Num_0_def/PLL_Factor;

    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
    Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;

    PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def/PH_Volt_Factor;
    PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def/PH_Volt_Factor;

    //Current loop
    Har_2_Controller_Num_2=Har_2_Controller_Num_2_def/Har_2_Factor;
    Har_2_Controller_Num_1=Har_2_Controller_Num_1_def/Har_2_Factor;
    Har_2_Controller_Num_0=Har_2_Controller_Num_0_def/Har_2_Factor;


    Har_6_Controller_Num_2=Har_6_Controller_Num_2_def/Har_6_Factor;
    Har_6_Controller_Num_1=Har_6_Controller_Num_1_def/Har_6_Factor;
    Har_6_Controller_Num_0=Har_6_Controller_Num_0_def/Har_6_Factor;


    Har_12_Controller_Num_2=Har_12_Controller_Num_2_def/Har_12_Factor;
    Har_12_Controller_Num_1=Har_12_Controller_Num_1_def/Har_12_Factor;
    Har_12_Controller_Num_0=Har_12_Controller_Num_0_def/Har_12_Factor;


    Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
    Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;

    Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def/Neg_Current_Factor;
    Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def/Neg_Current_Factor;
*/

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}

/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
void RemoteRegulate_Dispose_2(uchar Inital_Y)
{
    float ParameterTemp, /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
        InitialTempFloat; /* InitialTempFloat：原控制系数和给定初始化的浮点计算缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    /*
    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_CtlMode;

    if((PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=0;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=1;
    }
    else if((PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=2;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=3;
    }
    else {}

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16  =PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;


    Port_AD_PORT_EN=PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN=PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_2_EN=PORT_Regulate_DATA.FO_2_EN.FO_EN16;
    Port_FO_3_EN=PORT_Regulate_DATA.FO_3_EN.FO_EN16;
    Port_FO_EN_Set(3,0,0,0,0);

    Port_FB_EN=PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();
*/
    /**********************端口参数设置**********************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;  //1kw=10
    PORT_Regulate_DATA.PORT_P_SET=ParameterTemp*100.0;                         //w

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET;
    PORT_Regulate_DATA.PORT_Q_SET=ParameterTemp*100.0;
*/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET;
    PORT_Regulate_DATA.PORT_F_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=ParameterTemp/1000.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=ParameterTemp/1000.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=ParameterTemp/1000.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET;
    PORT_Regulate_DATA.STATCOM_V_SET=ParameterTemp;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET;
    PORT_Regulate_DATA.STATCOM_PF_SET=ParameterTemp/100.0;
*/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET; //1kw=10
    PORT_Regulate_DATA.STATCOM_Q_SET = ParameterTemp * 100.0; //w

    /**********************下垂电压截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    //	PORT_Regulate_DATA.PORT_V0_SET=ParameterTemp;

    /**********************下垂频率截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET;
    //	PORT_Regulate_DATA.PORT_F0_SET=ParameterTemp/10.0;

    /**********************频率下垂斜率**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET;
    //	PORT_Regulate_DATA.PORT_KF_SET=ParameterTemp/10.0;

    /************************电压下垂斜率********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    //	PORT_Regulate_DATA.PORT_KV_SET=ParameterTemp/10.0;

    /**************************保留数据**********************/
    /*
    for (i=0;i<4;i++) {
        ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i]=ParameterTemp;
    }
*/
    /*
    Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
    Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
    Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
    Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
    Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
    Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
*/

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET / 200;

    /**************************电压参考**********************/

    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 600.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else
    {
        HA_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    UstOK_HVAC_Port = HA_Module_RefUdc;
    Udc_Ref = HA_Module_RefUdc;

    if (NHA_module_A == 0)
    {
        Ud_st_Max = 10.0;
        Ud_st_Min = -10.0;
        Uq_st_Max = 10.0;
        Uq_st_Min = -10.0;
    }
    else
    {
        InitialTempFloat = (float)NHA_module_A;
        InitialTempFloat = InitialTempFloat * 1.1;
        InitialTempFloat = InitialTempFloat * HA_Module_RefUdc;

        Ud_st_Max = InitialTempFloat;
        Ud_st_Min = InitialTempFloat * (-1.0);
        Uq_st_Max = Ud_st_Max;
        Uq_st_Min = Ud_st_Min;
    }

    Uout_d_Positive_Max = Ud_st_Max;
    Uout_d_Positive_Min = Ud_st_Min;
    Uout_q_Positive_Max = Uq_st_Max;
    Uout_q_Positive_Min = Uq_st_Min;

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    /**************************电流范围**********************/
    /*
    if(PORT_Regulate_DATA.Reserved_1[3]>95.0)
    {
        PORT_Regulate_DATA.Reserved_1[3]=95.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=95;
        Idst_Max=95.0;
    }
    else if(PORT_Regulate_DATA.Reserved_1[3]<10.0)
    {
        PORT_Regulate_DATA.Reserved_1[3]=10.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=10;
        Idst_Max=10.0;
    }
    else
    {
        Idst_Max=PORT_Regulate_DATA.Reserved_1[3];
    }

    Idst_Min=(-1.0)*Idst_Max;
    Iqst_Max=Idst_Max;
    Iqst_Min=Idst_Min;

    Id_Positive_Max=Idst_Max;
    Id_Positive_Min=Idst_Min;
    Iq_Positive_Max=Iqst_Max;
    Iq_Positive_Min=Iqst_Min;
*/
    /**************************光纤插件模块配置**********************/
    /*
    HA_PM_A1=(ushort)PORT_Regulate_DATA.Reserved_1[0];
    HA_PM_A2=(ushort)PORT_Regulate_DATA.Reserved_1[1];
    HA_PM_B1=(ushort)PORT_Regulate_DATA.PORT_V0_SET;
    HA_PM_B2=(ushort)PORT_Regulate_DATA.PORT_F0_SET;
    HA_PM_C1=(ushort)PORT_Regulate_DATA.PORT_KF_SET;
    HA_PM_C2=(ushort)PORT_Regulate_DATA.PORT_KV_SET;

    NHA_module_A=0;
    NHA_module_B=0;
    NHA_module_C=0;

    for(i=0;i<8;i++)
    {
        InitialTemp=HA_PM_A1;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_A++;
        }
        InitialTemp=HA_PM_A2;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_A++;
        }
    }

    for(i=0;i<8;i++)
    {
        InitialTemp=HA_PM_B1;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_B++;
        }
        InitialTemp=HA_PM_B2;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_B++;
        }
    }

    for(i=0;i<8;i++)
    {
        InitialTemp=HA_PM_C1;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_C++;
        }
        InitialTemp=HA_PM_C2;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_C++;
        }
    }

    FPGA_Interface[Com_A1_PluginUnit_5-2].PM=HA_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6-2].PM=HA_PM_A2;
    FPGA_Interface[Com_B1_PluginUnit_9-2].PM=HA_PM_B1;
    FPGA_Interface[Com_B2_PluginUnit_10-2].PM=HA_PM_B2;
    FPGA_Interface[Com_C1_PluginUnit_13-2].PM=HA_PM_C1;
    FPGA_Interface[Com_C2_PluginUnit_14-2].PM=HA_PM_C2;

    FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHA_module_A;
    FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHA_module_A;
    FPGA_Interface[Com_B1_PluginUnit_9-2].NUM=NHA_module_B;
    FPGA_Interface[Com_B2_PluginUnit_10-2].NUM=NHA_module_B;
    FPGA_Interface[Com_C1_PluginUnit_13-2].NUM=NHA_module_C;
    FPGA_Interface[Com_C2_PluginUnit_14-2].NUM=NHA_module_C;

    InitialTemp=NHA_module_A;
    HA_ReferenceDEF_A=(float)InitialTemp;
    InitialTempFloat=ReferenceDEF;
    HA_ReferenceDEF_A=HA_ReferenceDEF_A*InitialTempFloat;


    InitialTemp=NHA_module_B;
    HA_ReferenceDEF_B=(float)InitialTemp;
    InitialTempFloat=ReferenceDEF;
    HA_ReferenceDEF_B=HA_ReferenceDEF_B*InitialTempFloat;

    InitialTemp=NHA_module_C;
    HA_ReferenceDEF_C=(float)InitialTemp;
    InitialTempFloat=ReferenceDEF;
    HA_ReferenceDEF_C=HA_ReferenceDEF_C*InitialTempFloat;
*/
    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;


    PLL_Factor=PORT_Regulate_DATA.PORT_V_P_1;
    if(PLL_Factor<0.01)
    {
        PLL_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
    }

    Volt_Factor=PORT_Regulate_DATA.PORT_V_I_1;
    if(Volt_Factor<0.01)
    {
        Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    }

    Current_Factor=PORT_Regulate_DATA.PORT_I_P_1;
    if(Current_Factor<0.01)
    {
        Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    }

    Neg_Current_Factor=PORT_Regulate_DATA.PORT_I_I_1;
    if(Neg_Current_Factor<0.01)
    {
        Neg_Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
    }

    PH_Volt_Factor=PORT_Regulate_DATA.PORT_V_P_2;
    if(PH_Volt_Factor<0.01)
    {
        PH_Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
    }

    Har_2_Factor=PORT_Regulate_DATA.PORT_V_I_2;
    if(Har_2_Factor<0.01)
    {
        Har_2_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
    }

    Har_6_Factor=PORT_Regulate_DATA.PORT_I_P_2;
    if(Har_6_Factor<0.01)
    {
        Har_6_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
    }

    Har_12_Factor=PORT_Regulate_DATA.PORT_I_I_2;
    if(Har_12_Factor<0.01)
    {
        Har_12_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
    }

//controller
    PLL_PID_Num_1=PLL_PID_Num_1_def/PLL_Factor;
    PLL_PID_Num_0=PLL_PID_Num_0_def/PLL_Factor;

    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
    Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;

    PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def/PH_Volt_Factor;
    PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def/PH_Volt_Factor;

    //Current loop
    Har_2_Controller_Num_2=Har_2_Controller_Num_2_def/Har_2_Factor;
    Har_2_Controller_Num_1=Har_2_Controller_Num_1_def/Har_2_Factor;
    Har_2_Controller_Num_0=Har_2_Controller_Num_0_def/Har_2_Factor;


    Har_6_Controller_Num_2=Har_6_Controller_Num_2_def/Har_6_Factor;
    Har_6_Controller_Num_1=Har_6_Controller_Num_1_def/Har_6_Factor;
    Har_6_Controller_Num_0=Har_6_Controller_Num_0_def/Har_6_Factor;


    Har_12_Controller_Num_2=Har_12_Controller_Num_2_def/Har_12_Factor;
    Har_12_Controller_Num_1=Har_12_Controller_Num_1_def/Har_12_Factor;
    Har_12_Controller_Num_0=Har_12_Controller_Num_0_def/Har_12_Factor;


    Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
    Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;

    Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def/Neg_Current_Factor;
    Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def/Neg_Current_Factor;
*/

    /************************EER2的数据************************/
    /*
    for (i=0;i<4;i++) {
        PORT_Regulate_DATA.EER2_DATA[i]=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }
*/

    Flag_PC = 1;

    if (Inital_Y == 0)
    {
        /* 并口兼容遥调仅更新RAM/旧FPGA镜像，不进行阻塞EEPROM写。
         * 新参数需通过邮箱21显式SAVE，后台每10ms推进一字节。 */
        FLAG_RemRegulate_SetOver = 1;
    }
    Flag_PC = 0;
}
#endif

#ifdef HVDC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0, 0, 0, 0, 0);

    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET; //1kw=10
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0; //w

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;

    //-------------------------------------------------------------------
    //	#ifdef HD_2_LD
    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else
    {
        HD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }
    HD_RefUdc = HD_Module_RefUdc;
    /*
    #else
        ParameterTemp=(float)NHD_module;
        ParameterTemp=ParameterTemp*740.0;

        ParameterTemp2=(float)NHD_module;
        ParameterTemp2=ParameterTemp2*600.0;

        if(PORT_Regulate_DATA.PORT_V_SET>ParameterTemp)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp;
            HD_RefUdc=ParameterTemp;
        }
        else if(PORT_Regulate_DATA.PORT_V_SET<ParameterTemp2)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp2;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp2;
            HD_RefUdc=ParameterTemp2;
        }
        else
        {
            HD_RefUdc=PORT_Regulate_DATA.PORT_V_SET;
        }

        ParameterTemp=(float)NHD_module;
        HD_Module_RefUdc=HD_RefUdc/ParameterTemp;
    #endif
*/
    //-------------------------------------------------------------------

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    UstOK_HVDC_Port = HD_Module_RefUdc;
    Udc_Ref = HD_Module_RefUdc;

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    /**************************光纤插件模块配置**********************/
    HD_PM_A1 = (ushort)PORT_Regulate_DATA.Reserved_1[0];
    HD_PM_A2 = (ushort)PORT_Regulate_DATA.Reserved_1[1];

    NHD_module = 0;

    for (i = 0; i < 8; i++)
    {
        InitialTemp = HD_PM_A1;
        if (InitialTemp & (0x0001 << i))
        {
            NHD_module++;
        }
        InitialTemp = HD_PM_A2;
        if (InitialTemp & (0x0001 << i))
        {
            NHD_module++;
        }
    }

    if (HD_2_LD == 1)
    {
        DC_CurrentST_1 = (float)NHD_module;
        DC_CurrentST_1 = DC_CurrentST_1 * 135.0;
        DC_CurrentST_2 = (float)NHD_module;
        DC_CurrentST_2 = DC_CurrentST_2 * (-135.0);
    }
    //#else
    //-------------------------------------------------------------------
    //		InitialTempFloat=(float)NHD_module;
    //		HD_Module_RefUdc=HD_RefUdc/InitialTempFloat;
    //-------------------------------------------------------------------
    //#endif

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM = HD_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM = HD_PM_A2;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].NUM = NHD_module;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].NUM = NHD_module;

    Port_OnState = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2];
    HWREGH(0x6000062e) = Port_OnState;

    /************************PI参数************************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1 = ParameterTemp / 100.0;
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;
*/

    AVolt_Factor = PORT_Regulate_DATA.PORT_V_P_1;
    if (AVolt_Factor < 0.01)
    {
        AVolt_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1 = 100;
    }

    Volt_Factor = PORT_Regulate_DATA.PORT_V_I_1;
    if (Volt_Factor < 0.01)
    {
        Volt_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1 = 100;
    }

    Current_Factor = PORT_Regulate_DATA.PORT_I_P_1;
    if (Current_Factor < 0.01)
    {
        Current_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1 = 100;
    }

    //controller
    //voltage loop
    Volt_PID_Num_1 = Volt_PID_Num_1_def / Volt_Factor;
    Volt_PID_Num_0 = Volt_PID_Num_0_def / Volt_Factor;

    //Current loop
    Current_PID_Num_1 = Current_PID_Num_1_def / Current_Factor;
    Current_PID_Num_0 = Current_PID_Num_0_def / Current_Factor;

    ModuleVolt_PID_Num_1 = ModuleVolt_PID_Num_1_def / AVolt_Factor;
    ModuleVolt_PID_Num_0 = ModuleVolt_PID_Num_0_def / AVolt_Factor;

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}

/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
void RemoteRegulate_Dispose_2(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    /*
    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_CtlMode;

    if((PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=0;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=1;
    }
    else if((PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=2;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=3;
    }
    else {}


    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16  =PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN=PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN=PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0,0,0,0,0);

    Port_FB_EN=PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();
*/
    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET; //1kw=10
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0; //w

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;

    //-------------------------------------------------------------------
    //	#ifdef HD_2_LD
    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else
    {
        HD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }
    HD_RefUdc = HD_Module_RefUdc;
    /*
    #else
        ParameterTemp=(float)NHD_module;
        ParameterTemp=ParameterTemp*740.0;

        ParameterTemp2=(float)NHD_module;
        ParameterTemp2=ParameterTemp2*600.0;

        if(PORT_Regulate_DATA.PORT_V_SET>ParameterTemp)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp;
            HD_RefUdc=ParameterTemp;
        }
        else if(PORT_Regulate_DATA.PORT_V_SET<ParameterTemp2)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp2;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp2;
            HD_RefUdc=ParameterTemp2;
        }
        else
        {
            HD_RefUdc=PORT_Regulate_DATA.PORT_V_SET;
        }

        ParameterTemp=(float)NHD_module;
        HD_Module_RefUdc=HD_RefUdc/ParameterTemp;
    #endif
*/
    //-------------------------------------------------------------------

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    UstOK_HVDC_Port = HD_Module_RefUdc;
    Udc_Ref = HD_Module_RefUdc;

    /**********************下垂电压截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    //	PORT_Regulate_DATA.PORT_V0_SET=ParameterTemp;

    /************************电压下垂斜率********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    //	PORT_Regulate_DATA.PORT_KV_SET=ParameterTemp/10.0;

    /**************************保留数据**********************/
    /*
    for (i=0;i<4;i++) {
        ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i]=ParameterTemp;
    }
*/
    /**************************光纤插件模块配置**********************/
    /*
    HD_PM_A1=(ushort)PORT_Regulate_DATA.Reserved_1[0];
    HD_PM_A2=(ushort)PORT_Regulate_DATA.Reserved_1[1];

    NHD_module=0;

    for(i=0;i<8;i++)
    {
        InitialTemp=HD_PM_A1;
        if(InitialTemp&(0x0001<<i))
        {
            NHD_module++;
        }
        InitialTemp=HD_PM_A2;
        if(InitialTemp&(0x0001<<i))
        {
            NHD_module++;
        }
    }

    #ifdef HD_2_LD
    #else
        InitialTempFloat=(float)NHD_module;
        HD_Module_RefUdc=HD_RefUdc/InitialTempFloat;
    #endif

    FPGA_Interface[Com_A1_PluginUnit_5-2].PM=HD_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6-2].PM=HD_PM_A2;

    FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHD_module;
    FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHD_module;
*/

    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;

    Volt_Factor=PORT_Regulate_DATA.PORT_V_I_1;
    if(Volt_Factor<0.01)
    {
        Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    }

    Current_Factor=PORT_Regulate_DATA.PORT_I_P_1;
    if(Current_Factor<0.01)
    {
        Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    }

//controller
    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
    Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;

    //Current loop
    Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
    Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;
*/
    /************************EER2的数据************************/
    /*
    for (i=0;i<4;i++) {
        PORT_Regulate_DATA.EER2_DATA[i]=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }
*/

    Flag_PC = 1;
    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        //		DSP_2_EEPROM_Page2();
        //		DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
    Flag_PC = 0;
}
#endif

#ifdef LVDC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0, 0, 0, 0, 0);

    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0;

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp / 10.0;

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp / 10.0;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else
    {
        LD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    LD_RefUdc = LD_Module_RefUdc;
    UstOK_LVDC_Port = LD_RefUdc;

    Udc_Ref = LD_RefUdc;

    /**************************光纤插件模块配置**********************/
    LD_PM_A1 = (ushort)PORT_Regulate_DATA.Reserved_1[0];
    LD_PM_A2 = (ushort)PORT_Regulate_DATA.Reserved_1[1];

    NLD_module = 0;

    for (i = 0; i < 8; i++)
    {
        InitialTemp = LD_PM_A1;
        if (InitialTemp & (0x0001 << i))
        {
            NLD_module++;
        }
        InitialTemp = LD_PM_A2;
        if (InitialTemp & (0x0001 << i))
        {
            NLD_module++;
        }
    }

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM = LD_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM = LD_PM_A2;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].NUM = NLD_module;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].NUM = NLD_module;

    Port_OnState = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2];
    HWREGH(0x6000062e) = Port_OnState;
    /************************PI参数************************/

    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    //	PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1 = ParameterTemp / 100.0;

    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;

*/
    Volt_Factor = PORT_Regulate_DATA.PORT_V_I_1;
    if (Volt_Factor < 0.01)
    {
        Volt_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1 = 100;
    }

    Current_Factor = PORT_Regulate_DATA.PORT_I_P_1;
    if (Current_Factor < 0.01)
    {
        Current_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1 = 100;
    }

    //controller
    //voltage loop
    Volt_PID_Num_1 = Volt_PID_Num_1_def / Volt_Factor;
    Volt_PID_Num_0 = Volt_PID_Num_0_def / Volt_Factor;

    //Current loop
    Current_PID_Num_1 = Current_PID_Num_1_def / Current_Factor;
    Current_PID_Num_0 = Current_PID_Num_0_def / Current_Factor;

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}

/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
void RemoteRegulate_Dispose_2(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */
    /*
    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_CtlMode;

    if((PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=0;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=1;
    }
    else if((PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=2;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=3;
    }
    else {}

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16  =PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN=PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN=PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0,0,0,0,0);

    Port_FB_EN=PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();
*/
    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0;

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp / 10.0;

    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else
    {
        LD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    LD_RefUdc = LD_Module_RefUdc;
    UstOK_LVDC_Port = LD_RefUdc;

    Udc_Ref = LD_RefUdc;

    /**********************下垂电压截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    //	PORT_Regulate_DATA.PORT_V0_SET=ParameterTemp/10.0;

    /************************电压下垂斜率********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    //	PORT_Regulate_DATA.PORT_KV_SET=ParameterTemp/10.0;

    /**************************保留数据**********************/
    /*
    for (i=0;i<4;i++) {
        ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i]=ParameterTemp;
    }
*/
    /**************************光纤插件模块配置**********************/
    /*
    LD_PM_A1=(ushort)PORT_Regulate_DATA.Reserved_1[0];
    LD_PM_A2=(ushort)PORT_Regulate_DATA.Reserved_1[1];

    NLD_module=0;

    for(i=0;i<8;i++)
    {
        InitialTemp=LD_PM_A1;
        if(InitialTemp&(0x0001<<i))
        {
            NLD_module++;
        }
        InitialTemp=LD_PM_A2;
        if(InitialTemp&(0x0001<<i))
        {
            NLD_module++;
        }
    }
*/
    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;
*/

    /************************EER2的数据************************/
    /*
    for (i=0;i<4;i++) {
        PORT_Regulate_DATA.EER2_DATA[i]=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }
*/

    Flag_PC = 1;
    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        //		DSP_2_EEPROM_Page2();
        //		DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
    Flag_PC = 0;
}
#endif

#ifdef LVAC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LA_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LA_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(4,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16);

    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET;
    PORT_Regulate_DATA.PORT_Q_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET;
    PORT_Regulate_DATA.PORT_F_SET = ParameterTemp / 10.0;

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp / 10.0;

    /**********************下垂频率截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET;
    PORT_Regulate_DATA.PORT_F0_SET = ParameterTemp / 10.0;

    /**********************频率下垂斜率**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET;
    PORT_Regulate_DATA.PORT_KF_SET = ParameterTemp / 10.0;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    /**************************光纤插件模块配置**********************/
    LA_PM_A1 = (ushort)PORT_Regulate_DATA.Reserved_1[0];
    LA_PM_A2 = (ushort)PORT_Regulate_DATA.Reserved_1[1];

    NLA_module = 0;

    for (i = 0; i < 8; i++)
    {
        InitialTemp = LA_PM_A1;
        if (InitialTemp & (0x0001 << i))
        {
            NLA_module++;
        }
        InitialTemp = LA_PM_A2;
        if (InitialTemp & (0x0001 << i))
        {
            NLA_module++;
        }
    }
    /************************PI参数************************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3 = ParameterTemp / 100.0;

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}
#endif
