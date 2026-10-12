#include "SstBoardConfig.h"
#include "soc_C6748.h"
/*
 * C6748 板级初始化
 *
 * 这里集中放置与芯片外设直接相关的配置。业务状态机和控制算法不应依赖
 * 寄存器细节，通过 SstBoard.h 中的板级接口调用本文件。
 */
#include "hw_types.h"
#include "interrupt.h"
#include "psc.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include "emifa.h"

#include "Hardware.h"
#include "EMIFAPinmuxSetup.h"
#include "ChbBuildConfig.h"
#include "SstBoard.h"

/* Timer2保留原10 ms节拍；UART2的寄存器配置由SstSerial_Init负责。 */
/* TMR_PERIOD_LSB32：Timer2 周期计数低 32 位，按原时钟对应 10 ms。 */
#define TMR_PERIOD_LSB32  (228000/0.1)
/* TMR_PERIOD_MSB32：Timer2 周期计数高 32 位。 */
#define TMR_PERIOD_MSB32  (0)



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


/****************************************************************************/
/*                                                                          */
/*                   UART Initialization                                    */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*              UART INT Initialization                                     */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*              GPIO INT Initialization                                     */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/*              DSP INT Initialization                                      */
/*                                                                          */
/****************************************************************************/

