#include "DpramHostData.h"
#include "SstControlConfig.h"
/* 文件职责: RuntimeStateInit 所属数据域的默认初始化；保持原连续语句段。 */
#include "RuntimeStateInit.h"
#include "SstTypes.h"
#include "ControlData.h"
#include "Ft3RecorderData.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "SoftTimeData.h"
#include "LegacySerialData.h"
#include "ProtectionData.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 复位录波序号、采样历史、运行观察和调试模式
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void RuntimeStateInit_ResetObservationAndModes(void)
{
    //edited by shiqi
    FT3_Samp_Cnt = 0;
    dataPackNum = 1;
    HA_Port_Volt_A_last = 0;
    HA_Port_Volt_B_last = 0;
    HA_Port_Volt_C_last = 0;
    Iin_d_Positive_last = 0;
    Iin_q_Positive_last = 0;
    SVG_Udc_Ave_temp_last = 0;
    Samplenoise = 1;
    Samplenoise_Idq = 1;
    Samplenoise_udc = 1;
    Contactor_err = 0;
    Breaker_err = 0;
    SM_write_flag = 0;
    SM_write_count = 0;
    ISO_counter_on = 0;
    ISO_counter_off = 0;

#ifdef COM_TEST_PluginUnit
    WrongTimes_LEFT = 0;
    WrongTimes_RIGHT = 0;
    LastWrong_LEFT = 0.0;
    LastWrong_RIGHT = 0.0;

    WrongTimes_LEFT_A = 0;
    WrongTimes_RIGHT_A = 0;
    LastWrong_LEFT_A = 0.0;
    LastWrong_RIGHT_A = 0.0;

    WrongTimes_Reference_A1 = 0;
    WrongTimes_Reference_A2 = 0;
    LastWrong_Reference_A1 = 0.0;
    LastWrong_Reference_A2 = 0.0;

    WrongTimes_Reference_C1 = 0;
    WrongTimes_Reference_C2 = 0;
    LastWrong_Reference_C1 = 0.0;
    LastWrong_Reference_C2 = 0.0;

    WrongTimes_ControlReference_A = 0;
    WrongTimes_ControlReference_C = 0;
    LastWrong_ControlReference_A = 0.0;
    LastWrong_ControlReference_C = 0.0;

#endif

    HA_IG_Log_A = 0;
    HA_IG_Log_B = 0;
    HA_IG_Log_C = 0;

    DAB_ONLY = 0;
    Phase_PLL_First = 0.0;
    Add_3th = 0;
    Method_3th = 1;
    Ref_ph_test = 0.0;
    Ref_js_test = 0.0;

    Flag_PC = 0;
    Flag_FaultOff = 0;
    Flag_DC_VoltSource = 0;
    flag_Standard = 1;
    flag_INV = 0;
    flag_FaultClose = 0;
    flag_SVGOPEN = 0;
    flag_DABOPEN = 0;

    flag_SendDABRef_First = 0;
    flag_SendSVGRef_First = 0;
}

/*==================================================================
 * 函数功能: 复位原软时标、远程服务标志和计数器
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void RuntimeStateInit_ResetTimeFlags(void)
{
    Flag_Key = 0;
    Flag_temp = 0;
    Flag_RemoteControlRegulate = 0;

    Flag_1s = 0;
    Flag_50ms = 0;
    Flag_100ms = 0;
    Flag_RemoteMeasure = 0;
    Flag_Clear_0 = 0;
    Flag_Clear_1 = 0;
    Flag_StopWait = 0;
    Flag_StopWaitEnd = 0;

    cout200us = 0;
    cout1s = 0;
    cout20s = 0;
    cout50ms = 0;
    cout100ms = 0;
    coutRemoteMeasure = 0;
    coutStopWait = 0;

    Flag_20s = 0;
}

/*==================================================================
 * 函数功能: 复位保留维护通信的长度、地址和清除阶段
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void RuntimeStateInit_ResetLegacySerial(void)
{
    Flag_ComTest = 0;
    FourPortComZhi = 0;

    /* 旧版上位机通讯变量初始化已废弃 */
    /*
    Flag_Uart=0;
    CommNum=0;
    CommFunc=0;
    */
    StartAddr = 0;
    ReadLength = 0;
    CommLength = 0;
    CommInNum = 0;
    CommTotalNum = 0;
    WriteOK = 0;
    ClearTimes = 1;
}

#endif /* HVAC_online */
