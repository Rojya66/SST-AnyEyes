#include "SstControlConfig.h"
/* 文件职责: MeasurementInit 所属数据域的默认初始化；保持原连续语句段。 */
#include "MeasurementInit.h"
#include "SstTypes.h"
#include "MeasurementData.h"
#include "ControlData.h"
#include "TransformData.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 复位RMS、P/Q和低通滤波历史样本
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void MeasurementInit_ResetFilters(void)
{
    //Measurement
    HA_PortUanRms_LF_1 = 0.0;
    HA_PortUanRms_LF_2 = 0.0;
    HA_PortUbnRms_LF_1 = 0.0;
    HA_PortUbnRms_LF_2 = 0.0;
    HA_PortUcnRms_LF_1 = 0.0;
    HA_PortUcnRms_LF_2 = 0.0;

    HA_PortIaRms_LF_1 = 0.0;
    HA_PortIaRms_LF_2 = 0.0;
    HA_PortIbRms_LF_1 = 0.0;
    HA_PortIbRms_LF_2 = 0.0;
    HA_PortIcRms_LF_1 = 0.0;
    HA_PortIcRms_LF_2 = 0.0;

    HA_GridIaRms_LF_1 = 0.0;
    HA_GridIaRms_LF_2 = 0.0;
    HA_GridIbRms_LF_1 = 0.0;
    HA_GridIbRms_LF_2 = 0.0;
    HA_GridIcRms_LF_1 = 0.0;
    HA_GridIcRms_LF_2 = 0.0;

    HA_PortP_LF_1 = 0.0;
    HA_PortP_LF_2 = 0.0;
    HA_PortQ_LF_1 = 0.0;
    HA_PortQ_LF_2 = 0.0;

    LPF_5KHZ_X_Volt_A_1 = 0.0;
    LPF_5KHZ_Y_Volt_A_1 = 0.0;
    LPF_5KHZ_X_Volt_B_1 = 0.0;
    LPF_5KHZ_Y_Volt_B_1 = 0.0;

    LPF_20HZ_X_Ud_1 = 0.0;
    LPF_20HZ_Y_Ud_1 = 0.0;
    LPF_20HZ_X_Uq_1 = 0.0;
    LPF_20HZ_Y_Uq_1 = 0.0;
}

/*==================================================================
 * 函数功能: 复位谐波平均滤波、三相累加以及谐波相位观察
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void MeasurementInit_ResetAverages(void)
{
    uchar i; /* i：本分域初始化的循环索引，每个循环均从0开始。 */
    for (i = 0; i < 4; i++) //Ave, P,Q
    {
        Va_Ave_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        Vb_Ave_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        Vc_Ave_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        Ave_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        Ave_H6_100_Filter.H_Filter_BUF[i] = 0.0;

        P_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        Q_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        Q_Load_H2_100_Filter.H_Filter_BUF[i] = 0.0;
    }

    for (i = 0; i < 4; i++) //SVG Voltage
    {
        U_d_Grid_H2_20_Filter.H_Filter_BUF[i] = 0.0;
        U_q_Grid_H2_20_Filter.H_Filter_BUF[i] = 0.0;
    }

    for (i = 0; i < 4; i++) //SVG Current
    {
        I_d_SVG_H2_20_Filter.H_Filter_BUF[i] = 0.0;
        I_d_SVG_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        I_d_SVG_H6_100_Filter.H_Filter_BUF[i] = 0.0;
        I_d_SVG_H12_100_Filter.H_Filter_BUF[i] = 0.0;

        I_q_SVG_H2_20_Filter.H_Filter_BUF[i] = 0.0;
        I_q_SVG_H2_100_Filter.H_Filter_BUF[i] = 0.0;
        I_q_SVG_H6_100_Filter.H_Filter_BUF[i] = 0.0;
        I_q_SVG_H12_100_Filter.H_Filter_BUF[i] = 0.0;
    }

    for (i = 0; i < 4; i++) //I_load
    {
        //      I_SVG_H2_100_Filter.H_Filter_BUF[i]=0.0;
        //      I_SVG_H6_100_Filter.H_Filter_BUF[i]=0.0;
        //      I_SVG_H12_100_Filter.H_Filter_BUF[i]=0.0;
        //      I_SVG_H2_20_Filter.H_Filter_BUF[i]=0.0;

        I_d_Load_H6_100_Filter.H_Filter_BUF[i] = 0.0;
        I_d_Load_H12_100_Filter.H_Filter_BUF[i] = 0.0;
        I_d_Load_H2_20_Filter.H_Filter_BUF[i] = 0.0;

        I_q_Load_H6_100_Filter.H_Filter_BUF[i] = 0.0;
        I_q_Load_H12_100_Filter.H_Filter_BUF[i] = 0.0;
        I_q_Load_H2_20_Filter.H_Filter_BUF[i] = 0.0;
    }

    for (i = 0; i < 3; i++)
    {
        HVAC_RealVoltage_Sum[i] = 0.0;
        HVAC_RealCurrent_Sum[i] = 0.0;
        HVAC_RealGridCurrent_Sum[i] = 0.0;
    }

    HVAC_RealParameter_N = 0;

    Har_5OR7_PH0 = TAD_PLL;
    Har_5OR7_PH0 = Har_5OR7_PH0 * 1884.95556;
    Har_11_PH0 = TAD_PLL;
    Har_11_PH0 = Har_11_PH0 * 3769.91112;
    Har_5_RMS = 0.0;
    Har_7_RMS = 0.0;
    Har_11_RMS = 0.0;
    Har_5_PH = 0.0;
    Har_7_PH = 0.0;
    Har_11_PH = 0.0;
}

#endif /* HVAC_online */
