#include "TaskSlice.h"
#include "CpuTask.h"
#include "ChbDabControl.h"
#include "SstFault.h"
#include "SstDpramHost.h"
#include "SstDpramMaintenance.h"
#include "ChbBuildConfig.h"
#include "SstDebug.h"
#include "SstSerial.h"
#include "SstParameterStore.h"
#include "MeasurementService.h"

/*==================================================================
 * 函数功能: 明确列出一次后台调度的原顺序和通信预算
 * 输入参数: 无
 * 输出参数: 所属模块状态；调用顺序和原实现保持。
 * 返回值: 无
 * 执行说明: 系统初始化或主循环；不增加 FPGA 中断调用。
 *================================================================*/
void TaskSlice_Service(void)
{
    uint32_t now; /* now：本轮首次发送前捕获的10ms节拍，供原VOFA周期判断。 */
    CpuTask_ServiceHeartbeat();
    ChbDabControl_Service();
    SstFault_Service();
#if SST_ENABLE_DPRAM
    SstDpramHost_ServiceRemoteControl();
    SstDpramHost_ServiceTelemetry();
    SstDpramMaintenance_Service();
#endif
    now = SstDebug_GetTick10ms();
    SstSerial_TxService();
    SstDebug_ReceiveService();
    SstDebug_ProtocolService(now);
    SstParameterStore_Service();
    SstSerial_TxService();
    Port_RealTimeParameter();
}
