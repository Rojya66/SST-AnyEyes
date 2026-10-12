#include "SstControlConfig.h"
/* 文件职责: ControlDefaults 所属数据域的默认初始化；保持原连续语句段。 */
#include "ControlDefaults.h"
#include "SstTypes.h"
#include "ControlData.h"
#include "ProtectionConfig.h"
#include "ProtectionData.h"
#include "PortStatusData.h"
#include "DpramHostData.h"
#include "MeasurementData.h"
#include "TransformData.h"
#include "X1226_RW.h"
#include "DPRAM.h"
#include "ChbBuildConfig.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 设置电压量程、欠压/过压门限和控制限幅
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void ControlDefaults_InitLimits(void)
{
    float InitialTempFloat; /* InitialTempFloat：参考比例或控制限幅计算的原浮点临时值。 */
//range
#ifdef SVG_10KV
    Ust_HVAC_Port=900.0;                     //端口母线有电
    HA_L_Voltage_Precharge=350.0;    //预充直流母线平均电压
    HA_H_Voltage=780.0;

    OverVolt_AlarmLevel2=900.0;
    VoltLevel_1=3000.0;    //0.3Urated
    VoltLevel_2=9000.0;    //0.9Urated

    StandardValue=26.0;
    //HVAC 控制算法里的电压标幺/缩放系数
    //HVAC_Strategy.C 里会把 Alpha_Trans、Beta_Trans、Ugrid_d/q 等除以或乘以
    //StandardValue，用于 10kV 和 380V 两种工程配置共用同一套控制算法
#endif

#ifdef SVG_380V
    Ust_HVAC_Port=200.0;                     //端口母线有电
    HA_L_Voltage_Precharge=80.0;    //预充直流母线平均电压
    HA_H_Voltage=780.0

    OverVolt_AlarmLevel2=900.0;
    VoltLevel_1=114.0;    //0.3Urated
    VoltLevel_2=342.0;    //0.9Urated

    StandardValue=1.0;
#endif
    Udc_Check_Index=0;
    Udc_Checked_Num=0;

    HA_A_LV_Precharge=0;
    HA_B_LV_Precharge=0;
    HA_C_LV_Precharge=0;

    HA_A_HV_Protect=0;
    HA_B_HV_Protect=0;
    HA_C_HV_Protect=0;

    Ust_add=10.0;
    HA_Module_RefUdc=680.0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=680;
    UstOK_HVAC_Port=HA_Module_RefUdc;



    if(NHA_module_A==0)
    {
        Ud_st_Max=10.0;
        Ud_st_Min=-10.0;
        Uq_st_Max=10.0;
        Uq_st_Min=-10.0;
    }
    else
    {
        InitialTempFloat=(float)NHA_module_A;
        InitialTempFloat=InitialTempFloat*1.1;
        InitialTempFloat=InitialTempFloat*HA_Module_RefUdc;

        Ud_st_Max=InitialTempFloat;
        Ud_st_Min=InitialTempFloat*(-1.0);
        Uq_st_Max=Ud_st_Max;
        Uq_st_Min=Ud_st_Min;
    }

    Idst_Max=95.0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=95;
    Idst_Min=(-1.0)*Idst_Max;
    Iqst_Max=Idst_Max;
    Iqst_Min=Idst_Min;


    Udc_Ref=HA_Module_RefUdc;

    Id_Positive_Max=Idst_Max;
    Id_Positive_Min=Idst_Min;
    Iq_Positive_Max=Iqst_Max;
    Iq_Positive_Min=Iqst_Min;
    Uout_d_Positive_Max=Ud_st_Max;
    Uout_d_Positive_Min=Ud_st_Min;
    Uout_q_Positive_Max=Uq_st_Max;
    Uout_q_Positive_Min=Uq_st_Min;

}

/*==================================================================
 * 函数功能: 初始化PLL/电压/电流/谐波系数并保留页1至3恢复顺序
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void ControlDefaults_InitCoefficients(void)
{
//controller
    PLL_PID_Num_1=PLL_PID_Num_1_def;
    PLL_PID_Num_0=PLL_PID_Num_0_def;

    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def;
    Volt_PID_Num_0=Volt_PID_Num_0_def;

    PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def;
    PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def;

    //Current loop
    Har_2_Controller_Num_2=Har_2_Controller_Num_2_def;
    Har_2_Controller_Num_1=Har_2_Controller_Num_1_def;
    Har_2_Controller_Num_0=Har_2_Controller_Num_0_def;
    Har_2_Controller_Den_1=Har_2_Controller_Den_1_def;

    Har_6_Controller_Num_2=Har_6_Controller_Num_2_def;
    Har_6_Controller_Num_1=Har_6_Controller_Num_1_def;
    Har_6_Controller_Num_0=Har_6_Controller_Num_0_def;
    Har_6_Controller_Den_1=Har_6_Controller_Den_1_def;

    Har_12_Controller_Num_2=Har_12_Controller_Num_2_def;
    Har_12_Controller_Num_1=Har_12_Controller_Num_1_def;
    Har_12_Controller_Num_0=Har_12_Controller_Num_0_def;
    Har_12_Controller_Den_1=Har_12_Controller_Den_1_def;

    Current_PID_Num_1=Current_PID_Num_1_def;
    Current_PID_Num_0=Current_PID_Num_0_def;

    Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def;
    Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def;


    //regulate
    PLL_Factor=1.0;
    Volt_Factor=1.0;
    PH_Volt_Factor=1.0;
    Har_2_Factor=1.0;
    Har_6_Factor=1.0;
    Har_12_Factor=1.0;
    Current_Factor=1.0;
    Neg_Current_Factor=1.0;


    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3=0;

#ifdef EEPROM_FirstW
    DSP_2_EEPROM_Page1();
    DSP_2_EEPROM_Page2();
    DSP_2_EEPROM_Page3();
#endif


#ifdef EEPROM_ON
    EEPROM_Page1_2_DSP();
    EEPROM_Page2_2_DSP();
    EEPROM_Page3_2_DSP();
#endif

    RemoteRegulate_Dispose(1);
    FLAG_RemRegulate_SetOver=1;


}

/*==================================================================
 * 函数功能: 初始化PLL相位、变换正余弦和PLL滤波缓存
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void ControlDefaults_InitPllState(void)
{
    uchar i; /* i：本分域初始化的循环索引，每个循环均从0开始。 */
    //PLL
    PLL_Phase_Com=0.0;
    Phase_PLL=0.0;
    PhaseCom=PhaseComValue;

    Cos_ph_Trans=1.0;
    Sin_ph_Trans=0.0;
    Cos_ph_Trans_Com=1.0;
    Sin_ph_Trans_Com=0.0;
    Cos_ph_Trans_next=1.0;
    Sin_ph_Trans_next=0.0;

    for(i=0;i<4;i++)     //PLL
    {
        PLL_PID_Controller.PID_Controller_BUF[i]=0.0;

        U_d_Grid_H2_100_Filter.H_Filter_BUF[i]=0.0;
        U_d_Grid_H6_100_Filter.H_Filter_BUF[i]=0.0;
        U_d_Grid_H12_100_Filter.H_Filter_BUF[i]=0.0;

        U_q_Grid_H2_100_Filter.H_Filter_BUF[i]=0.0;
        U_q_Grid_H6_100_Filter.H_Filter_BUF[i]=0.0;
        U_q_Grid_H12_100_Filter.H_Filter_BUF[i]=0.0;
    }

}

#endif /* HVAC_online */

/*==================================================================
 * 函数功能: 复位原谐波、电压、电流调节器及电流给定滤波缓存
 * 输入参数: 无
 * 输出参数: 原调节器/滤波器缓存；按原单个for循环的逐项写入順序清零。
 * 返回值: 无
 * 执行说明: 开机及原PWM开锁路径通过Variable_RST调用；循环固定4项，无额外等待或中断屏蔽。
 *================================================================*/
void ControlDefaults_ResetRegulators(void)
{
    ushort i; /* i：本次数组、参数或模块遍历的索引。 */

#ifdef HVAC_online

    for(i=0;i<4;i++)    //controller
    {
        I_d_6_Har_Controller.Har_Controller_BUF[i]=0.0;
        I_d_12_Har_Controller.Har_Controller_BUF[i]=0.0;

        I_q_6_Har_Controller.Har_Controller_BUF[i]=0.0;
        I_q_12_Har_Controller.Har_Controller_BUF[i]=0.0;

        Volt_PID_Controller.PID_Controller_BUF[i]=0.0;
        ClusterVolt_A_PID_Controller.PID_Controller_BUF[i]=0.0;
        ClusterVolt_B_PID_Controller.PID_Controller_BUF[i]=0.0;
        ClusterVolt_C_PID_Controller.PID_Controller_BUF[i]=0.0;

        Current_d_PID_Controller.PID_Controller_BUF[i]=0.0;
        Current_q_PID_Controller.PID_Controller_BUF[i]=0.0;
        Neg_Current_d_PID_Controller.PID_Controller_BUF[i]=0.0;
        Neg_Current_q_PID_Controller.PID_Controller_BUF[i]=0.0;

        I_d_Ref_H2_100_Filter.H_Filter_BUF[i]=0.0;
        I_d_Ref_H6_100_Filter.H_Filter_BUF[i]=0.0;
    }
#endif



}

