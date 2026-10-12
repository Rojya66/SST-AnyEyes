//###########################################################################
// FILE: SstSerial.c
// FUNCTION: UART2/RS485 硬件传输和后台独占整帧队列。协议解析位于 PcDebug。
//           每次发送最多16字节；每次接收最多64字节；禁止注册UART中断。
//###########################################################################
#include "SstSerial.h"
#include "ChbBuildConfig.h"
#include "soc_C6748.h"
#include "hw_types.h"
#include "hw_uart.h"
#include "uart.h"
#include "gpio.h"
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
void SstSerial_TxService(void)
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
 * 函数功能: 读取因帧非法或队列满导致的累计丢帧数
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 累计丢帧数
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstSerial_GetTxDropped(void) { return txDropped; }

/*==================================================================
 * 函数功能: 清空UART发送队列并按原顺序配置UART2和RS485方向
 * 输入参数: 无
 * 输出参数: 清空整帧队列、关闭UART中断并设置UART2/RS485寄存器。
 * 返回值: 无
 * 执行说明: 板级PSC和引脚初始化后调用；UART2只有本模块轮询访问。
 *================================================================*/
void SstSerial_Init(void)
{
 txHead=txTail=txCount=txOffset=txWaiting=txDropped=0;
 UARTIntDisable(SOC_UART_2_REGS,UART_INT_RXDATA_CTI|UART_INT_TX_EMPTY);
 UARTConfigSetExpClk(SOC_UART_2_REGS,SST_UART_INPUT_CLOCK_HZ,SST_UART_BAUD,
                     UART_WORDL_8BITS,UART_OVER_SAMP_RATE_16);
 UARTFIFOEnable(SOC_UART_2_REGS);
 UARTFIFOLevelSet(SOC_UART_2_REGS,UART_RX_TRIG_LEVEL_1);
 UARTEnable(SOC_UART_2_REGS);
 GPIOPinWrite(SOC_GPIO_0_REGS,104,0);
}

/*==================================================================
 * 函数功能: 在64字节预算内读取UART并向协议层提交单个字节
 * 输入参数: receiveByte：每个接收字节的协议回调；空指针不执行接收
 * 输出参数: UART FIFO消费至多64字节；保留两个发送帧槽供协议应答。
 * 返回值: 无
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
void SstSerial_ReceiveService(SST_SERIAL_RX_CALLBACK receiveByte)
{
 uint16_t budget=64U; /* budget：本次轮询允许接收的最大字节数。 */
 if(!receiveByte) return;
 while(budget-- && SstSerial_TxFreeFrames()>1 && UARTCharsAvail(SOC_UART_2_REGS))
  receiveByte((uint8_t)UARTCharGetNonBlocking(SOC_UART_2_REGS));
}
