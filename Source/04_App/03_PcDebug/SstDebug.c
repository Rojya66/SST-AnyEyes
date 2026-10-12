//###########################################################################
// FILE: SstDebug.c
// FUNCTION: AnyEyes/VOFA二选一的后台协议协调与可选维护协议字节分流。
//           UART寄存器及整帧队列归SstSerial，参数存储由TaskSlice显式调度。
//###########################################################################
#include <stdint.h>
#include "SstDebug.h"
#include "ChbDabControl.h"
#include "PcDebugDispData.h"
#include "SstParameterStore.h"
#include "CpuTask.h"
#include "ChbBuildConfig.h"
#include "vofa.h"
#if SST_ENABLE_MAINTENANCE
#include "MaintenanceProtocol.H"
#endif
/* SST_FRAME_MAX：协议分流整帧缓存最大字节数，与串口帧槽一致。 */
#define SST_FRAME_MAX 260U
static uint32_t rxTick; /* rxTick：最近接收字节的 10 ms 节拍，用于后台分流超时判断。 */
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_VOFA
static uint32_t lastVofaTick; /* lastVofaTick：最近提交 VOFA 波形帧的 10 ms 节拍。 */
#endif
static uint16_t muxState,muxExpected; /* muxState：主协议与可选维护协议的接收分流状态。 */
/* muxExpected：当前分流帧预期总字节数。 */
#if SST_ENABLE_MAINTENANCE
static uint16_t muxIndex; /* muxIndex：当前分流帧已接收字节数。 */
static uint8_t maintHeader[SST_FRAME_MAX]; /* maintHeader：可选维护分流的整帧字节缓存，容量 SST_FRAME_MAX。 */
#endif
/*==================================================================
 * 函数功能: 读取控制核心的 10 ms 时间基准
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 32 位 10 ms 节拍
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
uint32_t SstDebug_GetTick10ms(void) { return (uint32_t)g_chb_dab_status.tick_10ms; }

/*==================================================================
 * 函数功能: 读取板级单调毫秒时间基准
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 32 位 1 ms 节拍
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
uint32_t SstDebug_GetTick1ms(void) { return SstBoard_GetTick1ms(); }

/*==================================================================
 * 函数功能: 读取发送队列满或帧非法导致的丢弃计数
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 累计丢帧数
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
uint16_t SstDebug_GetTxDropped(void) { return SstSerial_GetTxDropped(); }

/*==================================================================
 * 函数功能: 按编译协议及可选维护帧头将 UART 接收字节分流到后台解析器
 * 输入参数: value：本轮 UART 收到的单字节，按当前协议分流状态解释
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
static void SstDebug_RouteByte(uint8_t value)
{
 uint32_t now=SstDebug_GetTick10ms(); /* now：本轮读取的 10 ms 控制节拍，用于后台超时或逐拍服务。 */
 if(muxState && (uint32_t)(now-rxTick)>10U) {
  muxState=muxExpected=0;
#if SST_ENABLE_MAINTENANCE
  muxIndex=0;
#endif
 }
 rxTick=now;
#if SST_ENABLE_MAINTENANCE
 if(muxState==1) {
  if(value==MAINT_SOF1) {
   maintHeader[0]=MAINT_SOF0;maintHeader[1]=value;
   muxState=4;muxIndex=2;return;
  }
  muxState=0;
 }
 if(muxState==4) {
  maintHeader[muxIndex++]=value;
  if(muxIndex==8) {
   muxExpected=(uint16_t)maintHeader[6]|((uint16_t)maintHeader[7]<<8);
   if(muxExpected>MAINT_MAX_PAYLOAD) { muxState=0;return; }
   muxExpected+=10; muxState=5;
  }
  return;
 }
 if(muxState==5) {
  maintHeader[muxIndex++]=value;
  if(muxIndex>=muxExpected) {
   uint16_t i; /* i：本次数组、参数或模块遍历的索引。 */
   for(i=0;i<muxExpected;++i) MaintenanceProtocol_RxByte(maintHeader[i]);
   muxState=0;
  }
  return;
 }
#endif
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_ANYEYES
 if(muxState==2) {
  SstAnyEyes_RxByte(value);
  if(value<1 || value>29) muxState=0;
  else { muxState=3;muxExpected=(uint16_t)value+1; }
  return;
 }
 if(muxState==3) {
  SstAnyEyes_RxByte(value);
  if(--muxExpected==0) muxState=0;
  return;
 }
 if(value==0x6B) { SstAnyEyes_RxByte(value);muxState=2;return; }
#endif
#if SST_ENABLE_MAINTENANCE
 if(value==MAINT_SOF0) muxState=1;
#endif
}

/*==================================================================
 * 函数功能: 初始化共享收发状态、UART2 及编译选中的调试协议
 * 输入参数: 无
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
void SstDebug_Init(void)
{
 muxState=muxExpected=0;
#if SST_ENABLE_MAINTENANCE
 muxIndex=0;
#endif
 rxTick=SstDebug_GetTick10ms();
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_VOFA
 lastVofaTick=rxTick;
#endif
 SstSerial_Init();
 SstAnyEyes_Init();
 VOFA_Init();
#if SST_ENABLE_MAINTENANCE
 MaintenanceProtocol_Init();
#endif
}

/*==================================================================
 * 函数功能: 保留 10 ms 调试服务兼容入口，本函数为空；时间基准直接读取控制状态
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
void SstDebug_Tick10ms(void) { /* Timebase is g_chb_dab_status.tick_10ms. */ }

/*==================================================================
 * 函数功能: 接收一个有界UART批次并按原状态机分流给调试或维护协议
 * 输入参数: 无
 * 输出参数: 最多处理64个接收字节，协议解析状态更新。
 * 返回值: 无
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
void SstDebug_ReceiveService(void)
{
 SstSerial_ReceiveService(SstDebug_RouteByte);
}

/*==================================================================
 * 函数功能: 推进所选调试协议及可选维护协议，不执行UART或参数存储任务
 * 输入参数: now：本轮UART发送前采集的10ms节拍，保持VOFA原读取顺序
 * 输出参数: 协议状态更新；完整应答或波形帧加入SstSerial队列。
 * 返回值: 无
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
void SstDebug_ProtocolService(uint32_t now)
{
#if SST_ENABLE_MAINTENANCE
 MaintenanceProtocol_Process();
#endif
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_ANYEYES
 SstAnyEyes_Service();
#elif SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_VOFA
 if((uint32_t)(now-lastVofaTick)>=SST_TELEMETRY_PERIOD_10MS && SstSerial_TxFreeFrames()>1) {
  VOFA_Send4Float(SstTelemetry_ReadFloat(0),SstTelemetry_ReadFloat(6),
                 SstTelemetry_ReadFloat(7),SstTelemetry_ReadFloat(8));
  lastVofaTick=now;
 }
#endif
 (void)now;
}

/*==================================================================
 * 函数功能: 保留旧后台调用入口；按原顺序协调两次TX、一次RX、协议和存储
 * 输入参数: 无
 * 输出参数: UART、协议和存储状态推进。
 * 返回值: 无
 * 执行说明: 兼容入口；活动TaskSlice显式列出同一顺序，不调用本包装。
 *================================================================*/
void SstDebug_Service(void)
{
 uint32_t now=SstDebug_GetTick10ms(); /* now：本轮发送前读取的控制10ms节拍。 */
 SstSerial_TxService();
 SstDebug_ReceiveService();
 SstDebug_ProtocolService(now);
 SstParameterStore_Service();
 SstSerial_TxService();
}
