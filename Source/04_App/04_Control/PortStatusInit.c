#include "SstControlConfig.h"
/* 文件职责: PortStatusInit 所属数据域的默认初始化；保持原连续语句段。 */
#include "PortStatusInit.h"
#include "SstTypes.h"
#include "ControlData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "DpramHostData.h"
#include "DpramConfig.h"
#include "X1226_RW.h"
#include "SstPortStatus.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 建立原端口主模式、停机状态、故障初值；EEPROM恢复由统一镜像执行
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void PortStatusInit_DefaultState(void)
{
    WriteBlock_Index = WriteBlock_PluginUnit_2;

    Port_MainStatus.MainStatus16 = 0x003f;
    Port_MainStatus.Bbits.P_M_Mode = 0x03;
    Port_MainStatus.Bbits.P_S1_Mode = 0x00;
    Port_MainStatus.Bbits.P_S2_Mode = 0x00;

    Port_Main_WorkMode = 3;
    Port_STATCOM_WorkMode = 0;
    Port_APF_WorkMode = 0;
    Port_Outer_Elec = 0;
    Port_Sys_Stop = 0;
    Port_State = PortStatusNum_Nonvaild;
    Port_RunStatus = PortStatusNum_Nonvaild;
    ControlFLAG.ControlFLAG16 = 0;

    PortRunFLAG.PortRunFLAG16 = 0x007c; //all port don't need shutdown
    Port_RealStatus.Bbits.S_Mode = 0; //Auto Run

    Port_RealFault.PORT_FAULT16 = 0;
    FaultTypeFLAG.FaultTypeFLAG16 = 0;
    FaultDisposeFLAG.FaultDisposeFLAG16 = 0;

    /* 不再读取无CRC的旧页0；合法持久化端口方式由统一镜像恢复。 */
    Port_StateUpdate_Write();
}

#endif /* HVAC_online */
