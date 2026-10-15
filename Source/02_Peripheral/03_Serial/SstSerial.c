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

/* SST_TX_FRAME：发送队列的一个完整帧槽，长度和内容都按字节计。 */
typedef struct
{
    uint16_t length; /* length：当前整帧的有效长度，单位为字节。 */
    uint8_t bytes[SST_FRAME_MAX]; /* bytes：发送帧槽的完整字节缓存。 */
} SST_TX_FRAME;

static SST_TX_FRAME tx[SST_TX_SLOTS]; /* tx：共享整帧发送队列，后台独占维护。 */
static uint16_t txHead, /* 下一写入帧槽索引。 */
    txTail, /* 当前待发送帧槽索引。 */
    txCount, /* 已占用的整帧槽数，范围为 0～SST_TX_SLOTS。 */
    txOffset, /* 当前帧已送入 UART FIFO 的字节数。 */
    txWaiting, /* 当前帧已入 FIFO，等待 TEMT 后切回接收方向的标志。 */
    txDropped; /* 非法帧或队列满导致的 16 位累计丢帧计数。 */

/*==================================================================
 * 函数功能: 查询共享发送队列剩余整帧容量
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 可用帧槽数量
 * 执行说明: 仅供后台调用；队列无中断互斥机制，不在 FPGA ISR 内调用。
 *================================================================*/
uint16_t SstSerial_TxFreeFrames(void)
{
    return (uint16_t)(SST_TX_SLOTS - txCount);
}

/*==================================================================
 * 函数功能: 验证帧长并复制到共享发送队列，不等待 UART 硬件
 * 输入参数: bytes：输入完整帧字节缓存，容量至少 length
 *           length：完整帧字节数，有效范围 1～SST_FRAME_MAX
 * 输出参数: 共享 tx 队列的一个帧槽及队列头/占用数，失败更新 txDropped。
 * 返回值: 1 表示入队成功，0 表示失败
 * 执行说明: 仅供后台调用；队列无中断互斥机制，不在 FPGA ISR 内调用。
 *================================================================*/
int SstSerial_SendFrame(const uint8_t *bytes, uint16_t length)
{
    uint16_t i; /* i：当前待复制的帧字节索引，范围为 0～length-1。 */
    if (!bytes || !length || length > SST_FRAME_MAX || txCount == SST_TX_SLOTS)
    {
        ++txDropped;
        return 0;
    }
    tx[txHead].length = length;
    for (i = 0; i < length; ++i)
        tx[txHead].bytes[i] = bytes[i];
    txHead = (uint16_t)((txHead + 1) % SST_TX_SLOTS);
    ++txCount;
    return 1;
}

/*==================================================================
 * 函数功能: 在字节预算内向可写的 UART FIFO 推进发送队列
 * 输入参数: 无
 * 输出参数: UART2 发送 FIFO、RS485 方向 GPIO104 及队列发送游标/占用数。
 * 返回值: 无
 * 执行说明: 仅供后台调用；队列无中断互斥机制，不在 FPGA ISR 内调用。
 *================================================================*/
void SstSerial_TxService(void)
{
    uint16_t budget = SST_TX_BUDGET; /* budget：本次允许写入 UART FIFO 的剩余字节预算，初值为 16。 */
    if (!txCount)
        return;
    if (txWaiting)
    {
        if (!(HWREG(SOC_UART_2_REGS + UART_LSR) & UART_LSR_TEMT))
            return;
        GPIOPinWrite(SOC_GPIO_0_REGS, 104, 0);
        txWaiting = txOffset = 0;
        txTail = (uint16_t)((txTail + 1) % SST_TX_SLOTS);
        --txCount;
        return;
    }
    if (!txOffset)
        GPIOPinWrite(SOC_GPIO_0_REGS, 104, 1);
    while (budget-- && txOffset < tx[txTail].length && UARTSpaceAvail(SOC_UART_2_REGS))
    {
        UARTCharPutNonBlocking(SOC_UART_2_REGS, tx[txTail].bytes[txOffset++]);
    }
    if (txOffset == tx[txTail].length)
        txWaiting = 1;
}

/*==================================================================
 * 函数功能: 读取因帧非法或队列满导致的累计丢帧数
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 累计丢帧数
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstSerial_GetTxDropped(void)
{
    return txDropped;
}

/*==================================================================
 * 函数功能: 清空发送队列，配置 UART2 为 8 位轮询收发并设 RS485 为接收方向
 * 输入参数: 无
 * 输出参数: 清空队列，禁用 RXDATA_CTI/TX_EMPTY 中断并配置 UART2/RS485 寄存器。
 * 返回值: 无
 * 执行说明: 板级PSC和引脚初始化后调用；UART2只有本模块轮询访问。
 *================================================================*/
void SstSerial_Init(void)
{
    txHead = txTail = txCount = txOffset = txWaiting = txDropped = 0;
    UARTIntDisable(SOC_UART_2_REGS, UART_INT_RXDATA_CTI | UART_INT_TX_EMPTY);
    UARTConfigSetExpClk(
        SOC_UART_2_REGS, SST_UART_INPUT_CLOCK_HZ, SST_UART_BAUD, UART_WORDL_8BITS, UART_OVER_SAMP_RATE_16);
    UARTFIFOEnable(SOC_UART_2_REGS);
    UARTFIFOLevelSet(SOC_UART_2_REGS, UART_RX_TRIG_LEVEL_1);
    UARTEnable(SOC_UART_2_REGS);
    GPIOPinWrite(SOC_GPIO_0_REGS, 104, 0);
}

/*==================================================================
 * 函数功能: 在64字节预算内读取UART并向协议层提交单个字节
 * 输入参数: receiveByte：每个接收字节的协议回调；空指针不执行接收
 * 输出参数: UART FIFO 至多消费 64 字节；接收回调更新协议解析状态。
 *           仅当发送队列至少有两个空帧槽时继续接收，供协议应答入队。
 * 返回值: 无
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
void SstSerial_ReceiveService(SST_SERIAL_RX_CALLBACK receiveByte)
{
    uint16_t budget = 64U; /* budget：本次允许从 UART FIFO 取出的剩余字节预算，初值为 64。 */
    if (!receiveByte)
        return;
    while (budget-- && SstSerial_TxFreeFrames() > 1 && UARTCharsAvail(SOC_UART_2_REGS))
        receiveByte((uint8_t)UARTCharGetNonBlocking(SOC_UART_2_REGS));
}
