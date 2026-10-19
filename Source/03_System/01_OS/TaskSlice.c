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
 * 函数功能: 按固定顺序执行一次后台控制、维护、通信和测量服务
 * 输入参数: 无
 * 输出参数: 心跳、慢速控制状态、故障报告、串口队列和实时测量镜像。
 *           启用 DPRAM 时还更新并口遥控、遥测和参数维护状态。
 * 返回值: 无
 * 执行说明: 由主循环反复调用，不在 FPGA 10 kHz ISR 内运行。
 *           每轮 TX 服务两次、每次最多 16 字节，RX 服务一次、最多 64 字节。
 *================================================================*/
void TaskSlice_Service(void)
{
    uint32_t now; /* 本轮首次发送前捕获的 10 ms 节拍，供 VOFA 周期判断。 */
    CpuTask_ServiceHeartbeat();
    ChbDabControl_Service();
    SstFault_Service();
#if SST_ENABLE_DPRAM
    SstDpramHost_ServiceRemoteControl();
    SstDpramHost_ServiceTelemetry();
    SstDpramMaintenance_Service();
#endif
    now = SstDebug_GetTick10ms(); /* 与协议初始化及周期参数保持同一时基。 */
    SstSerial_TxService();
    SstDebug_ReceiveService();
    SstDebug_ProtocolService(now);
    SstParameterStore_Service();
    SstSerial_TxService();
    Port_RealTimeParameter();
}
