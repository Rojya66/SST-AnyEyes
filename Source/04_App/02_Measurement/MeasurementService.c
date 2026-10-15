/* 文件职责: 主循环测量整理、遥测和运行判据；完整保留原Port_RealTimeParameter正文及条件。 */
#include "MeasurementService.h"
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
#include "ADC_Sample.h"
#include "hw_types.h"
#include "FaultCheck.h"
#ifdef HVAC_online
/*==================================================================
 * 函数功能: 换算当前端口锁存/滤波采样，更新有效值、功率及公共遥测
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: TaskSlice末尾主循环调用，消费RMS累计并更新外电源/欠压判据；保留原换算及顺序。
 *================================================================*/
void Port_RealTimeParameter(void)
{
    float AD_CalTemp; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
    float ADSample_buffer[3]; /* ADSample_buffer：本轮读取的 FPGA 原始采样字缓存。 */

    if (HVAC_RealParameter_N == 200)
    {
        AD_CalTemp = HVAC_RealVoltage_Sum[0] / 200.0;
        PORT_RealTime_DATA.PORT_V_A = sqrtf(AD_CalTemp);
        AD_CalTemp = HVAC_RealVoltage_Sum[1] / 200.0;
        PORT_RealTime_DATA.PORT_V_B = sqrtf(AD_CalTemp);
        AD_CalTemp = HVAC_RealVoltage_Sum[2] / 200.0;
        PORT_RealTime_DATA.PORT_V_C = sqrtf(AD_CalTemp);

        /*
        ADSample_buffer[0]=HA_Port_Volt_AB;
        ADSample_buffer[1]=HA_Port_Volt_BC;
        ADSample_buffer[2]=HA_Port_Volt_CA;
        HVAC_RealVoltage_Sum[0]=ADSample_buffer[0]*ADSample_buffer[0];
        HVAC_RealVoltage_Sum[1]=ADSample_buffer[1]*ADSample_buffer[1];
        HVAC_RealVoltage_Sum[2]=ADSample_buffer[2]*ADSample_buffer[2];
*/
        HVAC_RealVoltage_Sum[0] = 0.0;
        HVAC_RealVoltage_Sum[1] = 0.0;
        HVAC_RealVoltage_Sum[2] = 0.0;

        AD_CalTemp = HVAC_RealCurrent_Sum[0] / 200.0;
        PORT_RealTime_DATA.PORT_I_A = sqrtf(AD_CalTemp);
        AD_CalTemp = HVAC_RealCurrent_Sum[1] / 200.0;
        PORT_RealTime_DATA.PORT_I_B = sqrtf(AD_CalTemp);
        AD_CalTemp = HVAC_RealCurrent_Sum[2] / 200.0;
        PORT_RealTime_DATA.PORT_I_C = sqrtf(AD_CalTemp);

        /*
        ADSample_buffer[0]=HA_Port_Current_A;
        ADSample_buffer[1]=HA_Port_Current_B;
        ADSample_buffer[2]=HA_Port_Current_C;
        HVAC_RealCurrent_Sum[0]=ADSample_buffer[0]*ADSample_buffer[0];
        HVAC_RealCurrent_Sum[1]=ADSample_buffer[1]*ADSample_buffer[1];
        HVAC_RealCurrent_Sum[2]=ADSample_buffer[2]*ADSample_buffer[2];
*/
        HVAC_RealCurrent_Sum[0] = 0.0;
        HVAC_RealCurrent_Sum[1] = 0.0;
        HVAC_RealCurrent_Sum[2] = 0.0;

        AD_CalTemp = HVAC_RealGridCurrent_Sum[0] / 200.0;
        PORT_RealTime_DATA.I_A_2 = sqrtf(AD_CalTemp);
        AD_CalTemp = HVAC_RealGridCurrent_Sum[1] / 200.0;
        PORT_RealTime_DATA.I_B_2 = sqrtf(AD_CalTemp);
        AD_CalTemp = HVAC_RealGridCurrent_Sum[2] / 200.0;
        PORT_RealTime_DATA.I_C_2 = sqrtf(AD_CalTemp);

        /*
        ADSample_buffer[0]=HA_Grid_Current_A;
        ADSample_buffer[1]=HA_Grid_Current_B;
        ADSample_buffer[2]=HA_Grid_Current_C;
        HVAC_RealGridCurrent_Sum[0]=ADSample_buffer[0]*ADSample_buffer[0];
        HVAC_RealGridCurrent_Sum[1]=ADSample_buffer[1]*ADSample_buffer[1];
        HVAC_RealGridCurrent_Sum[2]=ADSample_buffer[2]*ADSample_buffer[2];
*/

        HVAC_RealGridCurrent_Sum[0] = 0.0;
        HVAC_RealGridCurrent_Sum[1] = 0.0;
        HVAC_RealGridCurrent_Sum[2] = 0.0;

        HVAC_RealParameter_N = 0;
    }

    if ((PORT_RealTime_DATA.PORT_V_A > Ust_HVAC_Port) && (PORT_RealTime_DATA.PORT_V_B > Ust_HVAC_Port) &&
        (PORT_RealTime_DATA.PORT_V_C > Ust_HVAC_Port))
    {
        Port_Outer_Elec = 1;
    }
    else
    {
        Port_Outer_Elec = 0;
    }

    if ((PORT_RealTime_DATA.PORT_V_A < VoltLevel_2) || (PORT_RealTime_DATA.PORT_V_B < VoltLevel_2) ||
        (PORT_RealTime_DATA.PORT_V_C < VoltLevel_2))
    {
        Port_Abnormal = 1;
    }
    else
    {
        Port_Abnormal = 0;
    }

    if ((PORT_RealTime_DATA.PORT_V_A < VoltLevel_1) || (PORT_RealTime_DATA.PORT_V_B < VoltLevel_1) ||
        (PORT_RealTime_DATA.PORT_V_C < VoltLevel_1))
    {
        Port_LV2 = 1;
    }
    else
    {
        Port_LV2 = 0;
    }
}
#endif

#ifdef HVDC_online
/*==================================================================
 * 函数功能: 换算当前端口锁存/滤波采样，更新有效值、功率及公共遥测
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_RealTimeParameter(void)
{
    //	unsigned char i;
    float AD_CalTemp, /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
        AD_CalTemp1, /* AD_CalTemp1：采样/功率换算的第二个浮点中间值。 */
        AD_CalTemp2; /* AD_CalTemp2：采样/功率换算的第三个浮点中间值。 */
    //	float ADSample_buffer[3];

    //	AD_CalTemp=HD_Port_Volt;
    PORT_RealTime_DATA.PORT_V_A = HD_Port_Volt;

    //	AD_CalTemp=HD_Port_Current;
    PORT_RealTime_DATA.PORT_I_A = HD_Port_Current;

    if (PORT_RealTime_DATA.PORT_V_A > Ust_HVDC_Port)
    {
        Port_Outer_Elec = 1;
    }
    else
    {
        Port_Outer_Elec = 0;
    }

    //power
    AD_CalTemp = PORT_RealTime_DATA.PORT_V_A;
    AD_CalTemp = AD_CalTemp * PORT_RealTime_DATA.PORT_I_A;
    PORT_RealTime_DATA.PORT_P = AD_CalTemp / 1000.0;

    //Protect
    if (HD_2_LD == 1)
    {
        AD_CalTemp = HD_RefUdc;
    }
    else
    {
        AD_CalTemp = HD_RefUdc;
        AD_CalTemp1 = (float)NHD_module;
        AD_CalTemp = AD_CalTemp * AD_CalTemp1;

        AD_CalTemp2 = DC_AddUst;
        DC_AddUst = AD_CalTemp2 * AD_CalTemp1;
    }

    //	if((PORT_RealTime_DATA.PORT_V_A>(HD_RefUdc+DC_AddUst))||(PORT_RealTime_DATA.PORT_V_A<(HD_RefUdc-DC_AddUst)))
    if ((PORT_RealTime_DATA.PORT_V_A > (AD_CalTemp + DC_AddUst)) ||
        (PORT_RealTime_DATA.PORT_V_A < (AD_CalTemp - DC_AddUst)))
    {
        if ((PortRunFLAG.Bbits.PWMState_Dab == 0) && (ControlFLAG.Bbits.Port_Start != 1) && (flag_INV != 1))
        {
            FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
            FaultDisposeFLAG.Bbits.Port_StopType = 1;
            Port_RunStatus = PortStatusNum_Stop;
            ControlFLAG.Bbits.Port_Stop = 0x01;
            ControlFLAG.Bbits.Port_Start = 0x00;

#ifdef FaultOutTest
            Flag_DC_FAULT = 5;
            Flag_DC_FAULTport = 0;
            Flag_DC_FAULTDISPOSE = (ushort)PORT_RealTime_DATA.PORT_V_A;

            HWREGH(0x6000082a) = Flag_DC_FAULT;
            HWREGH(0x6000082c) = Flag_DC_FAULTport;
            HWREGH(0x6000082e) = Flag_DC_FAULTDISPOSE;
#endif

            if (Flag_DC_VoltSource == 1)
            {
                FaultDisposeFLAG.Bbits.Port_StopAll = 1;
                PortRunFLAG.Bbits.Stop_AllPort = 1; // 置全端口停机请求，随后写回端口状态。
                Port_StateUpdate_Write();
            }

            FaultDisposeFLAG.Bbits.PWMPulseState = 2;
            ModuleIndex_PWMPulseDisable = 0;
            PWMPluse_Disable(0);
        }
    }

    if (HD_2_LD == 1)
    {
        AD_CalTemp = 860.0;
    }
    else
    {
        AD_CalTemp = (float)NHD_module;
        AD_CalTemp = AD_CalTemp * 766.0;
    }

    //	if(PORT_RealTime_DATA.PORT_V_A>AD_CalTemp)
    if (PortRunFLAG.Bbits.Port_HighUdc)
    {
        FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
        FaultDisposeFLAG.Bbits.Port_StopType = 1;
        Port_RunStatus = PortStatusNum_Stop;
        ControlFLAG.Bbits.Port_Stop = 0x01;
        ControlFLAG.Bbits.Port_Start = 0x00;

        if (Flag_DC_VoltSource == 1)
        {
            FaultDisposeFLAG.Bbits.Port_StopAll = 1;
            PortRunFLAG.Bbits.Stop_AllPort = 1; // 置全端口停机请求，随后写回端口状态。
            Port_StateUpdate_Write();
        }

        FaultDisposeFLAG.Bbits.PWMPulseState = 2; // 记录 DAB 封波选择值 2，随后调用 PWMPluse_Disable(0)。
        ModuleIndex_PWMPulseDisable = 0;
        PWMPluse_Disable(0);

#ifdef FaultOutTest
        Flag_DC_FAULT = 5;
        Flag_DC_FAULTport = 1;
        Flag_DC_FAULTDISPOSE = (ushort)PORT_RealTime_DATA.PORT_V_A;

        HWREGH(0x6000082a) = Flag_DC_FAULT;
        HWREGH(0x6000082c) = Flag_DC_FAULTport;
        HWREGH(0x6000082e) = Flag_DC_FAULTDISPOSE;
#endif
    }

    if ((PORT_RealTime_DATA.PORT_I_A > DC_CurrentST_1) || (PORT_RealTime_DATA.PORT_I_A < DC_CurrentST_2))
    {
        FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
        FaultDisposeFLAG.Bbits.Port_StopType = 1;
        Port_RunStatus = PortStatusNum_Stop;
        ControlFLAG.Bbits.Port_Stop = 0x01;
        ControlFLAG.Bbits.Port_Start = 0x00;

        if (Flag_DC_VoltSource == 1)
        {
            FaultDisposeFLAG.Bbits.Port_StopAll = 1;
            PortRunFLAG.Bbits.Stop_AllPort = 1; // 置全端口停机请求，随后写回端口状态。
            Port_StateUpdate_Write();
        }

        FaultDisposeFLAG.Bbits.PWMPulseState = 2; // 记录 DAB 封波选择值 2，随后调用 PWMPluse_Disable(0)。
        ModuleIndex_PWMPulseDisable = 0;
        PWMPluse_Disable(0);

#ifdef FaultOutTest
        Flag_DC_FAULT = 5;
        Flag_DC_FAULTport = 2;
        Flag_DC_FAULTDISPOSE = (ushort)PORT_RealTime_DATA.PORT_I_A;

        HWREGH(0x6000082a) = Flag_DC_FAULT;
        HWREGH(0x6000082c) = Flag_DC_FAULTport;
        HWREGH(0x6000082e) = Flag_DC_FAULTDISPOSE;
#endif
    }
}
#endif

#ifdef LVDC_online
/*==================================================================
 * 函数功能: 换算当前端口锁存/滤波采样，更新有效值、功率及公共遥测
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_RealTimeParameter(void)
{
    //	unsigned char i;
    float AD_CalTemp; /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
    //	float ADSample_buffer[3];

    //	AD_CalTemp=HD_Port_Volt;
    PORT_RealTime_DATA.PORT_V_A = LD_TotalVDC;

    //	AD_CalTemp=HD_Port_Current;
    PORT_RealTime_DATA.PORT_I_A = LD_Port_PositiveCurrent;

    if (PORT_RealTime_DATA.PORT_V_A > Ust_LVDC_Port)
    {
        Port_Outer_Elec = 1;
    }
    else
    {
        Port_Outer_Elec = 0;
    }

    //power
    AD_CalTemp = PORT_RealTime_DATA.PORT_V_A;
    AD_CalTemp = AD_CalTemp * PORT_RealTime_DATA.PORT_I_A;
    PORT_RealTime_DATA.PORT_P = AD_CalTemp / 1000.0;

    //Protect
    if ((PORT_RealTime_DATA.PORT_V_A > (LD_RefUdc + DC_AddUst)) ||
        (PORT_RealTime_DATA.PORT_V_A < (LD_RefUdc - DC_AddUst)))
    {
        if ((PortRunFLAG.Bbits.PWMState_Dab == 0) && (ControlFLAG.Bbits.Port_Start != 1) &&
            (flag_INV != 1)) // DAB 未封波、无启动命令且不处于逆变测试模式时，处理电压偏差。
        {
            FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
            FaultDisposeFLAG.Bbits.Port_StopType = 1;
            Port_RunStatus = PortStatusNum_Stop;
            ControlFLAG.Bbits.Port_Stop = 0x01;
            ControlFLAG.Bbits.Port_Start = 0x00;

            if (Flag_DC_VoltSource == 1)
            {
                FaultDisposeFLAG.Bbits.Port_StopAll = 1;
                PortRunFLAG.Bbits.Stop_AllPort = 1; // 置全端口停机请求，随后写回端口状态。
                Port_StateUpdate_Write();
            }

            FaultDisposeFLAG.Bbits.PWMPulseState = 2; // 记录 DAB 封波选择值 2，随后调用 PWMPluse_Disable(0)。
            ModuleIndex_PWMPulseDisable = 0;
            PWMPluse_Disable(0);

#ifdef FaultOutTest
            Flag_DC_FAULT = 5;
            Flag_DC_FAULTport = 1;
            Flag_DC_FAULTDISPOSE = (ushort)PORT_RealTime_DATA.PORT_V_A;

            HWREGH(0x6000082a) = Flag_DC_FAULT;
            HWREGH(0x6000082c) = Flag_DC_FAULTport;
            HWREGH(0x6000082e) = Flag_DC_FAULTDISPOSE;
#endif
        }
    }

    if (PORT_RealTime_DATA.PORT_V_A > 860.0)
    {
        FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
        FaultDisposeFLAG.Bbits.Port_StopType = 1;
        Port_RunStatus = PortStatusNum_Stop;
        ControlFLAG.Bbits.Port_Stop = 0x01;
        ControlFLAG.Bbits.Port_Start = 0x00;

#ifdef FaultOutTest
        Flag_DC_FAULT = 5;
        Flag_DC_FAULTport = 1;
        Flag_DC_FAULTDISPOSE = (ushort)PORT_RealTime_DATA.PORT_V_A;

        HWREGH(0x6000082a) = Flag_DC_FAULT;
        HWREGH(0x6000082c) = Flag_DC_FAULTport;
        HWREGH(0x6000082e) = Flag_DC_FAULTDISPOSE;
#endif

        if (Flag_DC_VoltSource == 1)
        {
            FaultDisposeFLAG.Bbits.Port_StopAll = 1;
            PortRunFLAG.Bbits.Stop_AllPort = 1; // 置全端口停机请求，随后写回端口状态。
            Port_StateUpdate_Write();
        }

        FaultDisposeFLAG.Bbits.PWMPulseState = 2; // 记录 DAB 封波选择值 2，随后调用 PWMPluse_Disable(0)。
        ModuleIndex_PWMPulseDisable = 0;
        PWMPluse_Disable(0);
    }

    if ((PORT_RealTime_DATA.PORT_I_A > DC_CurrentST_1) || (PORT_RealTime_DATA.PORT_I_A < DC_CurrentST_2))
    {
        FaultDisposeFLAG.Bbits.FaultDisposeType = 2;
        FaultDisposeFLAG.Bbits.Port_StopType = 1;
        Port_RunStatus = PortStatusNum_Stop;
        ControlFLAG.Bbits.Port_Stop = 0x01;
        ControlFLAG.Bbits.Port_Start = 0x00;

        if (Flag_DC_VoltSource == 1)
        {
            FaultDisposeFLAG.Bbits.Port_StopAll = 1;
            PortRunFLAG.Bbits.Stop_AllPort = 1; // 置全端口停机请求，随后写回端口状态。
            Port_StateUpdate_Write();
        }

        FaultDisposeFLAG.Bbits.PWMPulseState = 2; // 记录 DAB 封波选择值 2，随后调用 PWMPluse_Disable(0)。
        ModuleIndex_PWMPulseDisable = 0;
        PWMPluse_Disable(0);

#ifdef FaultOutTest
        Flag_DC_FAULT = 5;
        Flag_DC_FAULTport = 2;
        Flag_DC_FAULTDISPOSE = (ushort)PORT_RealTime_DATA.PORT_I_A;

        HWREGH(0x6000082a) = Flag_DC_FAULT;
        HWREGH(0x6000082c) = Flag_DC_FAULTport;
        HWREGH(0x6000082e) = Flag_DC_FAULTDISPOSE;
#endif
    }
}
#endif

#ifdef LVAC_online
/*==================================================================
 * 函数功能: 换算当前端口锁存/滤波采样，更新有效值、功率及公共遥测
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
void Port_RealTimeParameter(void)
{
    //	unsigned char i;
    float AD_CalTemp, /* AD_CalTemp：采样比例换算、滤波或有效值计算的浮点中间值。 */
        AD_CalTemp1; /* AD_CalTemp1：采样/功率换算的第二个浮点中间值。 */
    float ADSample_buffer[3]; /* ADSample_buffer：本轮读取的 FPGA 原始采样字缓存。 */

    ADSample_buffer[0] = LA_Port_Volt_A;
    ADSample_buffer[1] = LA_Port_Volt_B;
    ADSample_buffer[2] = LA_Port_Volt_C;

    ADSample_buffer[0] = ADSample_buffer[0] * ADSample_buffer[0];
    AD_CalTemp = LA_PortUanRms_LF_1 * Sample_UP_20kHz + LA_PortUanRms_LF_2 * Sample_DOWN_20kHz;
    LA_PortUanRms_LF_1 = ADSample_buffer[0];
    LA_PortUanRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_V_A = sqrtf(AD_CalTemp);

    ADSample_buffer[1] = ADSample_buffer[1] * ADSample_buffer[1];
    AD_CalTemp = LA_PortUbnRms_LF_1 * Sample_UP_20kHz + LA_PortUbnRms_LF_2 * Sample_DOWN_20kHz;
    LA_PortUbnRms_LF_1 = ADSample_buffer[1];
    LA_PortUbnRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_V_B = sqrtf(AD_CalTemp);

    ADSample_buffer[2] = ADSample_buffer[2] * ADSample_buffer[2];
    AD_CalTemp = LA_PortUcnRms_LF_1 * Sample_UP_20kHz + LA_PortUcnRms_LF_2 * Sample_DOWN_20kHz;
    LA_PortUcnRms_LF_1 = ADSample_buffer[2];
    LA_PortUcnRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_V_C = sqrtf(AD_CalTemp);

    ADSample_buffer[0] = LA_Grid_Current_A; // 读取 A 相电网瞬时电流，供后续平方、滤波和开方求有效值。
    ADSample_buffer[1] = LA_Grid_Current_B; // 读取 B 相电网瞬时电流，供后续平方、滤波和开方求有效值。
    ADSample_buffer[2] = LA_Grid_Current_C; // 读取 C 相电网瞬时电流，供后续平方、滤波和开方求有效值。

    ADSample_buffer[0] = ADSample_buffer[0] * ADSample_buffer[0];
    AD_CalTemp = LA_GridIaRms_LF_1 * Sample_UP_20kHz + LA_GridIaRms_LF_2 * Sample_DOWN_20kHz;
    LA_GridIaRms_LF_1 = ADSample_buffer[0];
    LA_GridIaRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_I_A = sqrtf(AD_CalTemp);

    ADSample_buffer[1] = ADSample_buffer[1] * ADSample_buffer[1];
    AD_CalTemp = LA_GridIbRms_LF_1 * Sample_UP_20kHz + LA_GridIbRms_LF_2 * Sample_DOWN_20kHz;
    LA_GridIbRms_LF_1 = ADSample_buffer[1];
    LA_GridIbRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_I_B = sqrtf(AD_CalTemp);

    ADSample_buffer[2] = ADSample_buffer[2] * ADSample_buffer[2];
    AD_CalTemp = LA_GridIcRms_LF_1 * Sample_UP_20kHz + LA_GridIcRms_LF_2 * Sample_DOWN_20kHz;
    LA_GridIcRms_LF_1 = ADSample_buffer[2];
    LA_GridIcRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_I_C = sqrtf(AD_CalTemp);

    ADSample_buffer[0] = LA_Grid_Volt_A;
    ADSample_buffer[1] = LA_Grid_Volt_B;
    ADSample_buffer[2] = LA_Grid_Volt_C;

    ADSample_buffer[0] = ADSample_buffer[0] * ADSample_buffer[0];
    AD_CalTemp = LA_GridUanRms_LF_1 * Sample_UP_20kHz + LA_GridUanRms_LF_2 * Sample_DOWN_20kHz;
    LA_GridUanRms_LF_1 = ADSample_buffer[0];
    LA_GridUanRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.V_A_2 = sqrtf(AD_CalTemp);

    ADSample_buffer[1] = ADSample_buffer[1] * ADSample_buffer[1];
    AD_CalTemp = LA_GridUbnRms_LF_1 * Sample_UP_20kHz + LA_GridUbnRms_LF_2 * Sample_DOWN_20kHz;
    LA_GridUbnRms_LF_1 = ADSample_buffer[1];
    LA_GridUbnRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.V_B_2 = sqrtf(AD_CalTemp);

    ADSample_buffer[2] = ADSample_buffer[2] * ADSample_buffer[2];
    AD_CalTemp = LA_GridUcnRms_LF_1 * Sample_UP_20kHz + LA_GridUcnRms_LF_2 * Sample_DOWN_20kHz;
    LA_GridUcnRms_LF_1 = ADSample_buffer[2];
    LA_GridUcnRms_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.V_C_2 = sqrtf(AD_CalTemp);

    AD_CalTemp = LA_DCbus_Volt;
    PORT_RealTime_DATA.DC_V = sqrtf(AD_CalTemp);

    if ((PORT_RealTime_DATA.PORT_V_A > Ust_LVAC_Port) && (PORT_RealTime_DATA.PORT_V_B > Ust_LVAC_Port) &&
        (PORT_RealTime_DATA.PORT_V_C > Ust_LVAC_Port))
    {
        Port_Outer_Elec = 1;
    }
    else
    {
        Port_Outer_Elec = 0;
    }

    //power and pf
    AD_CalTemp = LA_Port_Volt_A;
    AD_CalTemp = AD_CalTemp * LA_Grid_Current_A;
    AD_CalTemp1 = LA_Port_Volt_B;
    AD_CalTemp1 = AD_CalTemp1 * LA_Grid_Current_B;
    AD_CalTemp += AD_CalTemp1;

    AD_CalTemp1 = LA_Port_Volt_C;
    AD_CalTemp1 = AD_CalTemp1 * LA_Grid_Current_C;
    AD_CalTemp += AD_CalTemp1;
    ADSample_buffer[0] = AD_CalTemp;

    AD_CalTemp = LA_Port_Volt_A - LA_Port_Volt_B;
    AD_CalTemp = AD_CalTemp * LA_Grid_Current_C;
    AD_CalTemp1 = LA_Port_Volt_B - LA_Port_Volt_C;
    AD_CalTemp1 = AD_CalTemp1 * LA_Grid_Current_A;
    AD_CalTemp += AD_CalTemp1;

    AD_CalTemp1 = LA_Port_Volt_C - LA_Port_Volt_A;
    AD_CalTemp1 = AD_CalTemp1 * LA_Grid_Current_B;
    AD_CalTemp += AD_CalTemp1;
    ADSample_buffer[1] = AD_CalTemp;

    AD_CalTemp = LA_PortP_LF_1 * Sample_UP_20kHz + LA_PortP_LF_2 * Sample_DOWN_20kHz;
    LA_PortP_LF_1 = ADSample_buffer[0];
    LA_PortP_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_P = AD_CalTemp;

    AD_CalTemp = LA_PortQ_LF_1 * Sample_UP_20kHz + LA_PortQ_LF_2 * Sample_DOWN_20kHz;
    LA_PortQ_LF_1 = ADSample_buffer[1];
    LA_PortQ_LF_2 = AD_CalTemp;
    PORT_RealTime_DATA.PORT_Q = AD_CalTemp;

    AD_CalTemp = PORT_RealTime_DATA.PORT_P;
    AD_CalTemp = AD_CalTemp * AD_CalTemp;
    AD_CalTemp1 = PORT_RealTime_DATA.PORT_Q;
    AD_CalTemp1 = AD_CalTemp1 * AD_CalTemp1;
    AD_CalTemp += AD_CalTemp1;
    AD_CalTemp = sqrtf(AD_CalTemp);
    PORT_RealTime_DATA.PORT_PF = PORT_RealTime_DATA.PORT_P / AD_CalTemp;
}
#endif
