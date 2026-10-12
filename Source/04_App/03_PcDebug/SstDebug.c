#include <stdint.h>
#include "SstDebug.h"
#include "SstParams.h"
#include "SstBoard.h"
#include "ChbBuildConfig.h"
#include "soc_C6748.h"
#include "hw_types.h"
#include "hw_uart.h"
#include "uart.h"
#include "gpio.h"
#include "vofa.h"
#if SST_ENABLE_MAINTENANCE
#include "MaintenanceProtocol.H"
#endif
/* SST_TX_SLOTS：共享 UART 发送队列容量，8 个整帧槽。 */
#define SST_TX_SLOTS 8U
/* SST_FRAME_MAX：共享 UART 单帧最大字节数，260 字节。 */
#define SST_FRAME_MAX 260U
/* SST_RX_BUDGET：每次后台调用最多处理的 UART 接收字节数，64 字节。 */
#define SST_RX_BUDGET 64U
/* SST_TX_BUDGET：每次 TX 服务最多送入 FIFO 的字节数，16 字节。 */
#define SST_TX_BUDGET 16U
typedef struct
{
    uint16_t length; /* length：有效数据数量；字节缓存按字节、16 位对象复制按字计数。 */
    uint8_t bytes[SST_FRAME_MAX]; /* bytes：发送帧槽的完整字节缓存。 */
} SST_TX_FRAME;
static SST_TX_FRAME tx[SST_TX_SLOTS]; /* tx：共享整帧发送队列，后台独占维护。 */
static uint16_t txHead,txTail,txCount,txOffset,txWaiting,txDropped; /* txHead：发送队列下一写入帧槽索引。 */
/* txTail：发送队列下一发送帧槽索引。 */
/* txCount：发送队列已占用的整帧槽数量。 */
/* txOffset：当前队尾帧已经发送的字节数。 */
/* txWaiting：RS485 最后一个字节发送完成前的方向保持标志。 */
/* txDropped：因帧非法或队列满累计丢弃的帧数。 */
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
uint32_t SstDebug_GetTick10ms(void) { return SstControl_GetTick10ms(); }
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
uint16_t SstDebug_GetTxDropped(void) { return txDropped; }
/*==================================================================
 * 函数功能: 查询共享发送队列剩余整帧容量
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 可用帧槽数量
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
uint16_t SstSerial_TxFreeFrames(void) { return (uint16_t)(SST_TX_SLOTS-txCount); }
/*==================================================================
 * 函数功能: 验证帧长并复制到共享发送队列，不等待 UART 硬件
 * 输入参数: bytes：输入完整帧字节缓存，容量至少 length
 *           length：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 共享 tx 队列的一个帧槽及队列头/占用数，失败更新 txDropped。
 * 返回值: 1 表示入队成功，0 表示失败
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
int SstSerial_SendFrame(const uint8_t *bytes,uint16_t length)
{
 uint16_t i; /* i：本次数组、参数或模块遍历的索引。 */
 if(!bytes || !length || length>SST_FRAME_MAX || txCount==SST_TX_SLOTS) {
  ++txDropped; return 0;
 }
 tx[txHead].length=length;
 for(i=0;i<length;++i) tx[txHead].bytes[i]=bytes[i];
 txHead=(uint16_t)((txHead+1)%SST_TX_SLOTS);++txCount;
 return 1;
}
/*==================================================================
 * 函数功能: 在字节预算内向可写的 UART FIFO 推进发送队列
 * 输入参数: 无
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
static void SstSerial_TxService(void)
{
 uint16_t budget=SST_TX_BUDGET; /* budget：本次服务允许处理的最大字节数，避免后台单轮无界阻塞。 */
 if(!txCount) return;
 if(txWaiting) {
  if(!(HWREG(SOC_UART_2_REGS+UART_LSR)&UART_LSR_TEMT)) return;
  GPIOPinWrite(SOC_GPIO_0_REGS,104,0);
  txWaiting=txOffset=0;txTail=(uint16_t)((txTail+1)%SST_TX_SLOTS);--txCount;
  return;
 }
 if(!txOffset) GPIOPinWrite(SOC_GPIO_0_REGS,104,1);
 while(budget-- && txOffset<tx[txTail].length && UARTSpaceAvail(SOC_UART_2_REGS)) {
  UARTCharPutNonBlocking(SOC_UART_2_REGS,tx[txTail].bytes[txOffset++]);
 }
 if(txOffset==tx[txTail].length) txWaiting=1;
}
/*==================================================================
 * 函数功能: 按编译协议及可选维护帧头将 UART 接收字节分流到后台解析器
 * 输入参数: value：本轮 UART 收到的单字节，按当前协议分流状态解释
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
static void SstSerial_RouteByte(uint8_t value)
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
 txHead=txTail=txCount=txOffset=txWaiting=txDropped=0;
 muxState=muxExpected=0;
#if SST_ENABLE_MAINTENANCE
 muxIndex=0;
#endif
 rxTick=SstDebug_GetTick10ms();
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_VOFA
 lastVofaTick=rxTick;
#endif
 /* Board PSC/pinmux precede this call. UART2 has exactly one polling owner. */
 UARTIntDisable(SOC_UART_2_REGS,UART_INT_RXDATA_CTI|UART_INT_TX_EMPTY);
 UARTConfigSetExpClk(SOC_UART_2_REGS,SST_UART_INPUT_CLOCK_HZ,SST_UART_BAUD,
                     UART_WORDL_8BITS,UART_OVER_SAMP_RATE_16);
 UARTFIFOEnable(SOC_UART_2_REGS);
 UARTFIFOLevelSet(SOC_UART_2_REGS,UART_RX_TRIG_LEVEL_1);
 UARTEnable(SOC_UART_2_REGS);
 GPIOPinWrite(SOC_GPIO_0_REGS,104,0);
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
 * 函数功能: 以固定 RX/TX 字节预算轮询 UART 并调度所选协议、可选维护与参数保存服务
 * 输入参数: 无
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
void SstDebug_Service(void)
{
 uint16_t budget=SST_RX_BUDGET; /* budget：本次服务允许处理的最大字节数，避免后台单轮无界阻塞。 */
 uint32_t now=SstDebug_GetTick10ms(); /* now：本轮读取的 10 ms 控制节拍，用于后台超时或逐拍服务。 */
 SstSerial_TxService();
 /* Backpressure leaves room for request replies; stream traffic reserves a slot. */
 while(budget-- && SstSerial_TxFreeFrames()>1 && UARTCharsAvail(SOC_UART_2_REGS))
  SstSerial_RouteByte((uint8_t)UARTCharGetNonBlocking(SOC_UART_2_REGS));
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
 SstParams_Service();
 SstSerial_TxService();
}

