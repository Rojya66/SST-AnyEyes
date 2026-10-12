/*
 * C6748 板级初始化
 *
 * 这里集中放置与芯片外设直接相关的配置。业务状态机和控制算法不应依赖
 * 寄存器细节，只通过 main.h 中声明的初始化接口调用本文件。
 */
#include "hw_types.h"
#include "interrupt.h"
#include "psc.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include "emifa.h"

#include "main.h"
#include "Hardware.h"
#include "EMIFAPinmuxSetup.h"
#include "MaintenanceProtocol.H"
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstBoard.h"

/* Timer2 为原 10 ms 慢速任务节拍；UART2由SstDebug统一配置。 */
/* TMR_PERIOD_LSB32：Timer2 周期计数低 32 位，按原时钟对应 10 ms。 */
#define TMR_PERIOD_LSB32  (228000/0.1)
/* TMR_PERIOD_MSB32：Timer2 周期计数高 32 位。 */
#define TMR_PERIOD_MSB32  (0)
/* SYSCLK_1_FREQ：板级 SYSCLK1 频率，单位 Hz。 */
#define SYSCLK_1_FREQ     (456000000)
/* SYSCLK_2_FREQ：板级 SYSCLK2 频率，单位 Hz。 */
#define SYSCLK_2_FREQ     (SYSCLK_1_FREQ/2)
/* UART_2_FREQ：板级 UART2 时钟频率，单位 Hz。 */
#define UART_2_FREQ       (SYSCLK_2_FREQ)

/*==================================================================
 * 函数功能: 联合读取 Timer2 计数及 10 ms 节拍，生成单调的 1 ms 后台时间基准
 * 输入参数: 无
 * 输出参数: 无指针输出；保存上一输出值以维持单调性。
 * 返回值: 32 位毫秒节拍，允许自然回绕
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
uint32_t SstBoard_GetTick1ms(void)
{
    static uint32_t previous; /* previous：上次输出的单调毫秒值，跨主循环调用保留。 */
    uint32_t tick, check, fraction, result; /* tick：采集的控制核心 10 ms 节拍。 */
    /* check：二次读取的 10 ms 节拍，用于检查 Timer2 回绕附近的一致性。 */
    /* fraction：Timer2 当前周期内的毫秒分量。 */
    /* result：本次组合得到的单调毫秒结果。 */
    /* Only scalar reads in foreground. One bounded retry across Timer2 ISR. */
    tick = (uint32_t)g_chb_dab_status.tick_10ms;
    fraction = TimerCounterGet(SOC_TMR_2_REGS, TMR_TIMER12);
    check = (uint32_t)g_chb_dab_status.tick_10ms;
    if(tick != check)
    {
        tick = check;
        fraction = TimerCounterGet(SOC_TMR_2_REGS, TMR_TIMER12);
    }
    fraction /= (SYSCLK_2_FREQ / 1000U);
    if(fraction > 9U) fraction = 9U;
    result = tick * 10U + fraction;
    /* A pending rollover can briefly precede the software tick. No backwards
     * timeout or repeated burst is allowed; this also handles uint32 rollover. */
    if((int32_t)(result - previous) < 0) return previous;
    previous = result;
    return result;
}

/****************************************************************************/
/*==================================================================
 * 函数功能: 执行板级初始化使用的原循环延时
 * 输入参数: n：本次有效数据数量或循环次数，按调用接口的长度单位解释
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void Delay(unsigned int n)
{
    unsigned int i; /* i：本次数组、参数或模块遍历的索引。 */

    for(i=n;i>0;i--)
    {

    }
}

/****************************************************************************/
/*                                                                          */
/*              PSC Initialization                                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 使能板级外设对应的 PSC 电源/时钟模块
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void PSCInit(void)
{
    /* 先打开GPIO/UART2/EMIFA模块时钟，后续寄存器访问才有效。 */
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);
  PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_UART2, PSC_POWERDOMAIN_ALWAYS_ON,PSC_MDCTL_NEXT_ENABLE);
  PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_EMIFA, PSC_POWERDOMAIN_ALWAYS_ON,PSC_MDCTL_NEXT_ENABLE);
}

/****************************************************************************/
/*                                                                          */
/*              GPIO configuration                                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 配置板级 GPIO 的引脚复用
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void GPIOBankPinMuxSet(void)
{
    GPIOBank0Pin2PinMuxSetup();
    GPIOBank0Pin3PinMuxSetup();

    GPIOBank0Pin8PinMuxSetup();
    GPIOBank0Pin9PinMuxSetup();

    GPIOBank2Pin7PinMuxSetup();

    GPIOBank2Pin10PinMuxSetup();
    GPIOBank2Pin11PinMuxSetup();
    GPIOBank2Pin12PinMuxSetup();
    GPIOBank2Pin13PinMuxSetup();

    GPIOBank4Pin2PinMuxSetup();
    GPIOBank4Pin3PinMuxSetup();
    GPIOBank4Pin4PinMuxSetup();
    GPIOBank4Pin5PinMuxSetup();

    GPIOBank6Pin0PinMuxSetup();
    GPIOBank6Pin4PinMuxSetup();
    GPIOBank6Pin6PinMuxSetup();
    GPIOBank6Pin7PinMuxSetup();
    GPIOBank6Pin8PinMuxSetup();

    GPIOBank8Pin10PinMuxSetup();
    GPIOBank8Pin11PinMuxSetup();
    GPIOBank8Pin13PinMuxSetup();
    GPIOBank8Pin15PinMuxSetup();
    UARTPinMuxSetup(2, FALSE);
}

/****************************************************************************/
/*                                                                          */
/*              GPIO Initialization                                         */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 LED、看门狗、保护和通信相关 GPIO 方向与初值
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void GPIOBankPinInit(void)
{
    GPIODirModeSet(SOC_GPIO_0_REGS, 3, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 4, GPIO_DIR_OUTPUT);

    GPIODirModeSet(SOC_GPIO_0_REGS, 9, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 10, GPIO_DIR_OUTPUT);

    GPIODirModeSet(SOC_GPIO_0_REGS, 40, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 43, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 44, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 45, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 46, GPIO_DIR_OUTPUT);

    GPIODirModeSet(SOC_GPIO_0_REGS, 67, GPIO_DIR_INPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 68, GPIO_DIR_INPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 69, GPIO_DIR_INPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 70, GPIO_DIR_INPUT);

    GPIODirModeSet(SOC_GPIO_0_REGS, 97, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 101, GPIO_DIR_INPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 103, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 104, GPIO_DIR_OUTPUT);

    GPIODirModeSet(SOC_GPIO_0_REGS, 139, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 140, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 142, GPIO_DIR_OUTPUT);
    GPIODirModeSet(SOC_GPIO_0_REGS, 144, GPIO_DIR_OUTPUT);
}

/****************************************************************************/
/*                                                                          */
/*              EMIFA Initialization                                        */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 配置 FPGA/DPRAM 所用 EMIFA 时序及总线引脚
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void EMIFASetup(void)
{
    /* CS2连接FPGA控制区，CS5连接插件/模块区，两者均为16位异步访问。 */
    EMIFAPinMuxSetup();

    EMIFAAsyncDevDataBusWidthSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
                                    EMIFA_DATA_BUSWITTH_16BIT);
    EMIFAAsyncDevDataBusWidthSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_5,
                                    EMIFA_DATA_BUSWITTH_16BIT);


    EMIFAAsyncDevOpModeSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
                               EMIFA_ASYNC_INTERFACE_NORMAL_MODE);
    EMIFAAsyncDevOpModeSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_5,
                               EMIFA_ASYNC_INTERFACE_NORMAL_MODE);

    EMIFAExtendedWaitConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
                             EMIFA_EXTENDED_WAIT_DISABLE);
    EMIFAExtendedWaitConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_5,
                             EMIFA_EXTENDED_WAIT_ENABLE);

    EMIFAWaitTimingConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
                           EMIFA_ASYNC_WAITTIME_CONFIG(2, 2, 1, 3, 12, 6, 3));//2, 3, 2, 2, 3, 2, 0      2, 2, 1, 1, 9, 1, 2
    //                     EMIFA_ASYNC_WAITTIME_CONFIG(2, 10, 2, 2, 10, 6, 2));

    EMIFAWaitTimingConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_5,
                           EMIFA_ASYNC_WAITTIME_CONFIG(1, 4, 1, 1, 4, 1, 2));//2, 3, 2, 2, 3, 2, 0


    EMIFAWaitPinPolaritySet(SOC_EMIFA_0_REGS, EMIFA_EMA_WAIT_PIN1, EMIFA_EMA_WAIT_PIN_POLARITY_LOW);
    EMIFACSWaitPinSelect(SOC_EMIFA_0_REGS, EMIFA_CHIP_SELECT_5, EMIFA_EMA_WAIT_PIN1);
    EMIFAMaxExtWaitCycleSet(SOC_EMIFA_0_REGS, 0);
}

/****************************************************************************/
/*                                                                          */
/*                        T2 Initialization                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 Timer2 周期计数，沿用原 10 ms 定时配置
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void TimerInit(void)
{
    /* Timer2提供10 ms慢速节拍，状态机和维护超时不放入TAD快速中断。 */
    TimerConfigure(SOC_TMR_2_REGS, TMR_CFG_64BIT_CLK_INT);

    TimerPeriodSet(SOC_TMR_2_REGS, TMR_TIMER12, TMR_PERIOD_LSB32);
    TimerPeriodSet(SOC_TMR_2_REGS, TMR_TIMER34, TMR_PERIOD_MSB32);

    TimerEnable(SOC_TMR_2_REGS, TMR_TIMER12, TMR_ENABLE_CONT);
}

/****************************************************************************/
/*                                                                          */
/*                  T2 INT Initialization                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 Timer2 中断源与原 ISR 及 CPU 向量绑定
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void TimerInterruptInit(void)
{
    /* 中断只推进软件时基，具体慢任务由主循环读取标志后执行。 */
    IntRegister(C674X_MASK_INT8, Timer2Isr);
    IntEventMap(C674X_MASK_INT8, SYS_INT_T64P2_TINTALL);
    IntEnable(C674X_MASK_INT8);

    TimerIntEnable(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);
}

/****************************************************************************/
/*                                                                          */
/*                   UART Initialization                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 UART2 波特率、数据格式及 FIFO
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void UARTInit(void)
{
  /* UART2用于上位机485和维护协议；接收触发级别设为1字节。 */
  UARTConfigSetExpClk(SOC_UART_2_REGS, UART_2_FREQ, MAINT_UART_BAUD,
                          UART_WORDL_8BITS, UART_OVER_SAMP_RATE_16);
    UARTEnable(SOC_UART_2_REGS);

  UARTFIFOEnable(SOC_UART_2_REGS);
  UARTFIFOLevelSet(SOC_UART_2_REGS, UART_RX_TRIG_LEVEL_1);

  GPIOPinWrite(SOC_GPIO_0_REGS, 104, 0);
}

/****************************************************************************/
/*                                                                          */
/*              UART INT Initialization                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 保留 UART 初始化兼容入口，调试收发采用后台轮询
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void UARTInterruptInit(void)
{
//  unsigned int intFlag_1ss = 0;

    IntRegister(C674X_MASK_INT6, UART2Isr);
    IntEventMap(C674X_MASK_INT6, SYS_INT_UART2_INT);
    IntEnable(C674X_MASK_INT6);

//    intFlag_1ss |= (UART_INT_LINE_STAT  |  \
//                 UART_INT_TX_EMPTY |    \
//                 UART_INT_RXDATA_CTI);

 //   intFlag_1ss |= (UART_INT_RXDATA_CTI);
 //   UARTIntEnable(SOC_UART_2_REGS, intFlag_1ss);

 UARTIntEnable(SOC_UART_2_REGS, UART_INT_RXDATA_CTI);
}

/****************************************************************************/
/*                                                                          */
/*              GPIO INT Initialization                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 绑定 FPGA TAD、FAULT 以及可选 DPRAM 通知 GPIO 中断
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void GPIOBankPinInterruptInit(void)
{
//    GPIOIntTypeSet(SOC_GPIO_0_REGS, 68, GPIO_INT_TYPE_RISEDGE);
    GPIOIntTypeSet(SOC_GPIO_0_REGS, 69, GPIO_INT_TYPE_RISEDGE);    // TAD采样同步脉冲
//    GPIOIntTypeSet(SOC_GPIO_0_REGS, 70, GPIO_INT_TYPE_RISEDGE);
#if SST_ENABLE_DPRAM
    GPIOIntTypeSet(SOC_GPIO_0_REGS, 101, GPIO_INT_TYPE_FALLEDGE);  //DPRAM
#endif

    GPIOIntTypeSet(SOC_GPIO_0_REGS, 142, GPIO_INT_TYPE_RISEDGE);   // FPGA故障脉冲

//????????????????????????????????????????????????????????????????????????????
    GPIOBankIntEnable(SOC_GPIO_0_REGS, 4);    //????????????????????????????
//????????????????????????????????????????????????????????????????????????????
#if SST_ENABLE_DPRAM
    GPIOBankIntEnable(SOC_GPIO_0_REGS, 6);
#endif
    GPIOBankIntEnable(SOC_GPIO_0_REGS, 8);

        /* INT5/TAD、INT4/故障始终保留；INT7仅在DPRAM模式启用。 */
        IntRegister(C674X_MASK_INT5, FPGAIsr);
#if SST_ENABLE_DPRAM
        IntRegister(C674X_MASK_INT7, DPRAMIsr);
#endif
        IntRegister(C674X_MASK_INT4, FAULTIsr);

        IntEventMap(C674X_MASK_INT5, SYS_INT_GPIO_B4INT);
#if SST_ENABLE_DPRAM
        IntEventMap(C674X_MASK_INT7, SYS_INT_GPIO_B6INT);
#endif
        IntEventMap(C674X_MASK_INT4, SYS_INT_GPIO_B8INT);

        IntEnable(C674X_MASK_INT5);
#if SST_ENABLE_DPRAM
        IntEnable(C674X_MASK_INT7);
#endif
        IntEnable(C674X_MASK_INT4);
}

/****************************************************************************/
/*                                                                          */
/*              DSP INT Initialization                                      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 DSP 中断控制器并使能所需 CPU 中断通道
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void InterruptInit(void)
{
    IntDSPINTCInit();
    IntGlobalEnable();
}
