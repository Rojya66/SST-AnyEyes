/*======================================== 函数实现 =====================================*/

/*======================================================================================*
 * 文件名  : SstDebug.c
 * 功能说明: 协调 AnyEyes/VOFA 后台协议，UART 收发和整帧队列由 SstSerial 维护。
 *=====================================================================================*/

/*======================================== 文件包含 =====================================*/
#include <stdint.h>
#include "SstDebug.h"
#include "ChbDabControl.h"
#include "PcDebugDispData.h"
#include "SstParameterStore.h"
#include "CpuTask.h"
#include "ChbBuildConfig.h"
#include "vofa.h"

/*======================================== 私有变量 =====================================*/
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_ANYEYES
static uint32_t rxTick; /* 最近接收字节的 10 ms 节拍，供外层接收超时判断。 */
static uint16_t muxState, /* AnyEyes 外层状态：0 等帧头、2 等长度、3 收正文。 */
    muxExpected; /* 当前帧尚待接收的正文及校验字节数。 */

#endif
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_VOFA
static uint32_t lastVofaTick; /* 最近提交 VOFA 帧时的 10 ms 节拍。 */
#endif

/*==================================================================
 * 函数名称: SstDebug_GetTick10ms
 * 函数功能: 读取控制核心维护的 10 ms 节拍。
 * 输入参数: 无
 * 输出参数: 无
 * 返回值  : g_chb_dab_status.tick_10ms 当前值，单位为 10 ms。
 *================================================================*/
uint32_t SstDebug_GetTick10ms(void)
{
    return (uint32_t)g_chb_dab_status.tick_10ms;
}

/*==================================================================
 * 函数名称: SstDebug_GetTick1ms
 * 函数功能: 读取板级单调毫秒计数。
 * 输入参数: 无
 * 输出参数: 无
 * 返回值  : SstBoard_GetTick1ms 返回的 32 位计数，单位为 1 ms。
 *================================================================*/
uint32_t SstDebug_GetTick1ms(void)
{
    return SstBoard_GetTick1ms();
}

/*==================================================================
 * 函数名称: SstDebug_GetTxDropped
 * 函数功能: 读取共享串口发送服务的累计丢帧数。
 * 输入参数: 无
 * 输出参数: 无
 * 返回值  : 发送队列满或帧非法导致的累计丢帧计数。
 *================================================================*/
uint16_t SstDebug_GetTxDropped(void)
{
    return SstSerial_GetTxDropped();
}

/*==================================================================
 * 函数名称: SstDebug_RouteByte
 * 函数功能: 按外层帧头、长度和正文接收状态转交 AnyEyes 字节。
 * 输入参数: value：本轮 UART 接收到的单字节。
 * 输出参数: AnyEyes 外层接收状态及 PcDebug 解析器接收缓存。
 * 返回值  : 无
 * 调用说明: 接收间隔超过 10 个 10 ms 节拍时清空外层状态；VOFA 配置下忽略接收字节。
 *================================================================*/
static void SstDebug_RouteByte(uint8_t value)
{
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_ANYEYES
    uint32_t now = SstDebug_GetTick10ms(); /* 本次服务入口采集的 10 ms 节拍。 */
    if (muxState && (uint32_t)(now - rxTick) > 10U)
    {
        muxState = muxExpected = 0;
    }
    rxTick = now;
    /* 接收长度字节并转交解析器，再记录正文和校验的剩余字节数。 */
    if (muxState == 2)
    {
        SstAnyEyes_RxByte(value);
        if (value < 1 || value > 29)
            muxState = 0;
        else
        {
            muxState = 3;
            muxExpected = (uint16_t)value + 1;
        }
        return;
    }
    /* 正文与校验字节逐个转交，收满后重新等待帧头。 */
    if (muxState == 3)
    {
        SstAnyEyes_RxByte(value);
        if (--muxExpected == 0)
            muxState = 0;
        return;
    }
    /* AnyEyes 固定请求帧头为 0x6B。 */
    if (value == 0x6B)
    {
        SstAnyEyes_RxByte(value);
        muxState = 2;
        return;
    }
#else
    (void)value;
#endif
}

/*==================================================================
 * 函数名称: SstDebug_Init
 * 函数功能: 初始化调试协议时基、外层接收状态及共享 UART2 服务。
 * 输入参数: 无
 * 输出参数: 编译选中的协议状态和 SstSerial 串口状态。
 * 返回值  : 无
 * 调用说明: 系统启动时调用；AnyEyes 与 VOFA 的有效实现由 SST_DEBUG_PROTOCOL 选择。
 *================================================================*/
void SstDebug_Init(void)
{
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_ANYEYES
    muxState = muxExpected = 0;
    rxTick = SstDebug_GetTick10ms();
#endif
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_VOFA
    lastVofaTick = SstDebug_GetTick10ms();
#endif
    SstSerial_Init();
    SstAnyEyes_Init();
    VOFA_Init();
}

/*==================================================================
 * 函数名称: SstDebug_Tick10ms
 * 函数功能: 保留原 10 ms 调度接口，函数体不更新任何状态。
 * 输入参数: 无
 * 输出参数: 无
 * 返回值  : 无
 * 调用说明: 通信时基直接读取控制状态中的 tick_10ms。
 *================================================================*/
void SstDebug_Tick10ms(void)
{ /* 兼容空入口：节拍由控制核心维护。 */
}

/*==================================================================
 * 函数名称: SstDebug_ReceiveService
 * 函数功能: 以固定接收预算轮询 UART，并转交所选调试协议。
 * 输入参数: 无
 * 输出参数: UART 接收 FIFO 消耗及协议解析状态更新。
 * 返回值  : 无
 * 调用说明: 后台调用；单次字节预算由 SstSerial_ReceiveService 内部限制。
 *================================================================*/
void SstDebug_ReceiveService(void)
{
    SstSerial_ReceiveService(SstDebug_RouteByte);
}

/*==================================================================
 * 函数名称: SstDebug_ProtocolService
 * 函数功能: 执行 AnyEyes 状态机或按周期提交 VOFA 四通道波形。
 * 输入参数: now：本轮 UART 发送前采集的 10 ms 节拍。
 * 输出参数: 协议状态更新；应答帧或 VOFA 波形帧加入 SstSerial 发送队列。
 * 返回值  : 无
 * 调用说明: VOFA 四通道依次为 A 相电压(V)、直流平均电压(V)、端口状态码和 PLL 角度(rad)。
 *================================================================*/
void SstDebug_ProtocolService(uint32_t now)
{
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_ANYEYES
    SstAnyEyes_Service();
#elif SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_VOFA
    if ((uint32_t)(now - lastVofaTick) >= SST_TELEMETRY_PERIOD_10MS && SstSerial_TxFreeFrames() > 1)
    {
        VOFA_Send4Float(
            SstTelemetry_ReadFloat(0), SstTelemetry_ReadFloat(6), SstTelemetry_ReadFloat(7), SstTelemetry_ReadFloat(8));
        lastVofaTick = now;
    }
#endif
    (void)now;
}

/*==================================================================
 * 函数名称: SstDebug_Service
 * 函数功能: 保留原后台包装接口，依次执行 TX、RX、协议、参数存储和 TX。
 * 输入参数: 无
 * 输出参数: UART 收发、协议处理及参数存储状态更新。
 * 返回值  : 无
 * 调用说明: 活动 TaskSlice 已显式调度相同顺序，不调用此兼容包装函数。
 *================================================================*/
void SstDebug_Service(void)
{
    uint32_t now = SstDebug_GetTick10ms(); /* 本次服务入口采集的 10 ms 节拍。 */
    SstSerial_TxService();
    SstDebug_ReceiveService();
    SstDebug_ProtocolService(now);
    SstParameterStore_Service();
    SstSerial_TxService();
}
