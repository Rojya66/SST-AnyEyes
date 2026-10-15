//###########################################################################
//
// FILE:   Hardware.c
//
// FUNCTION:  1)Pin mux
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

/****************************************************************************/
#include "hw_types.h"

//#include "TL6748.h"
#include "soc_C6748.h"
#include "hw_syscfg0_C6748.h"

/**************************Macro definition*********************************/
/* PINMUX1_GPIO0_0_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX1_GPIO0_0_ENABLE (SYSCFG_PINMUX1_PINMUX1_31_28_GPIO0_0 << SYSCFG_PINMUX1_PINMUX1_31_28_SHIFT)

/* PINMUX1_GPIO0_1_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX1_GPIO0_1_ENABLE (SYSCFG_PINMUX1_PINMUX1_27_24_GPIO0_1 << SYSCFG_PINMUX1_PINMUX1_27_24_SHIFT)

/* PINMUX1_GPIO0_2_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX1_GPIO0_2_ENABLE (SYSCFG_PINMUX1_PINMUX1_23_20_GPIO0_2 << SYSCFG_PINMUX1_PINMUX1_23_20_SHIFT)

/* PINMUX1_GPIO0_3_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX1_GPIO0_3_ENABLE (SYSCFG_PINMUX1_PINMUX1_19_16_GPIO0_3 << SYSCFG_PINMUX1_PINMUX1_19_16_SHIFT)

/* PINMUX1_GPIO0_5_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX1_GPIO0_5_ENABLE (SYSCFG_PINMUX1_PINMUX1_11_8_GPIO0_5 << SYSCFG_PINMUX1_PINMUX1_11_8_SHIFT)

/* PINMUX1_GPIO0_6_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX1_GPIO0_6_ENABLE (SYSCFG_PINMUX1_PINMUX1_7_4_GPIO0_6 << SYSCFG_PINMUX1_PINMUX1_7_4_SHIFT)

/* PINMUX0_GPIO0_8_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX0_GPIO0_8_ENABLE (SYSCFG_PINMUX0_PINMUX0_31_28_GPIO0_8 << SYSCFG_PINMUX0_PINMUX0_31_28_SHIFT)

/* PINMUX0_GPIO0_9_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX0_GPIO0_9_ENABLE (SYSCFG_PINMUX0_PINMUX0_27_24_GPIO0_9 << SYSCFG_PINMUX0_PINMUX0_27_24_SHIFT)

/* PINMUX6_GPIO2_7_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX6_GPIO2_7_ENABLE (SYSCFG_PINMUX6_PINMUX6_3_0_GPIO2_7 << SYSCFG_PINMUX6_PINMUX6_3_0_SHIFT)

/* PINMUX5_GPIO2_10_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX5_GPIO2_10_ENABLE (SYSCFG_PINMUX5_PINMUX5_23_20_GPIO2_10 << SYSCFG_PINMUX5_PINMUX5_23_20_SHIFT)

/* PINMUX5_GPIO2_11_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX5_GPIO2_11_ENABLE (SYSCFG_PINMUX5_PINMUX5_19_16_GPIO2_11 << SYSCFG_PINMUX5_PINMUX5_19_16_SHIFT)

/* PINMUX5_GPIO2_12_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX5_GPIO2_12_ENABLE (SYSCFG_PINMUX5_PINMUX5_15_12_GPIO2_12 << SYSCFG_PINMUX5_PINMUX5_15_12_SHIFT)

/* PINMUX5_GPIO2_13_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX5_GPIO2_13_ENABLE (SYSCFG_PINMUX5_PINMUX5_11_8_GPIO2_13 << SYSCFG_PINMUX5_PINMUX5_11_8_SHIFT)

/* PINMUX10_GPIO4_2_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX10_GPIO4_2_ENABLE (SYSCFG_PINMUX10_PINMUX10_23_20_GPIO4_2 << SYSCFG_PINMUX10_PINMUX10_23_20_SHIFT)

/* PINMUX10_GPIO4_3_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX10_GPIO4_3_ENABLE (SYSCFG_PINMUX10_PINMUX10_19_16_GPIO4_3 << SYSCFG_PINMUX10_PINMUX10_19_16_SHIFT)

/* PINMUX10_GPIO4_4_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX10_GPIO4_4_ENABLE (SYSCFG_PINMUX10_PINMUX10_15_12_GPIO4_4 << SYSCFG_PINMUX10_PINMUX10_15_12_SHIFT)

/* PINMUX10_GPIO4_5_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX10_GPIO4_5_ENABLE (SYSCFG_PINMUX10_PINMUX10_11_8_GPIO4_5 << SYSCFG_PINMUX10_PINMUX10_11_8_SHIFT)

/* PINMUX10_GPIO4_7_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX10_GPIO4_7_ENABLE (SYSCFG_PINMUX10_PINMUX10_3_0_GPIO4_7 << SYSCFG_PINMUX10_PINMUX10_3_0_SHIFT)

/* PINMUX19_GPIO6_0_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX19_GPIO6_0_ENABLE (SYSCFG_PINMUX19_PINMUX19_27_24_GPIO6_0 << SYSCFG_PINMUX19_PINMUX19_27_24_SHIFT)

/* PINMUX19_GPIO6_1_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX19_GPIO6_1_ENABLE (SYSCFG_PINMUX19_PINMUX19_23_20_GPIO6_1 << SYSCFG_PINMUX19_PINMUX19_23_20_SHIFT)

/* PINMUX19_GPIO6_4_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX19_GPIO6_4_ENABLE (SYSCFG_PINMUX19_PINMUX19_11_8_GPIO6_4 << SYSCFG_PINMUX19_PINMUX19_11_8_SHIFT)

/* PINMUX14_GPIO6_6_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX14_GPIO6_6_ENABLE (SYSCFG_PINMUX14_PINMUX14_7_4_GPIO6_6 << SYSCFG_PINMUX14_PINMUX14_7_4_SHIFT)

/* PINMUX14_GPIO6_7_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX14_GPIO6_7_ENABLE (SYSCFG_PINMUX14_PINMUX14_3_0_GPIO6_7 << SYSCFG_PINMUX14_PINMUX14_3_0_SHIFT)

/* PINMUX13_GPIO6_12_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX13_GPIO6_12_ENABLE (SYSCFG_PINMUX13_PINMUX13_15_12_GPIO6_12 << SYSCFG_PINMUX13_PINMUX13_15_12_SHIFT)

/* PINMUX13_GPIO6_13_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX13_GPIO6_13_ENABLE (SYSCFG_PINMUX13_PINMUX13_11_8_GPIO6_13 << SYSCFG_PINMUX13_PINMUX13_11_8_SHIFT)

/* PINMUX3_GPIO8_2_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX3_GPIO8_2_ENABLE (SYSCFG_PINMUX3_PINMUX3_27_24_GPIO8_2 << SYSCFG_PINMUX3_PINMUX3_27_24_SHIFT)

/* PINMUX18_GPIO8_10_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX18_GPIO8_10_ENABLE (SYSCFG_PINMUX18_PINMUX18_31_28_GPIO8_10 << SYSCFG_PINMUX18_PINMUX18_31_28_SHIFT)

/* PINMUX18_GPIO8_11_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX18_GPIO8_11_ENABLE (SYSCFG_PINMUX18_PINMUX18_27_24_GPIO8_11 << SYSCFG_PINMUX18_PINMUX18_27_24_SHIFT)

/* PINMUX18_GPIO8_13_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX18_GPIO8_13_ENABLE (SYSCFG_PINMUX18_PINMUX18_19_16_GPIO8_13 << SYSCFG_PINMUX18_PINMUX18_19_16_SHIFT)

/* PINMUX18_GPIO8_15_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX18_GPIO8_15_ENABLE (SYSCFG_PINMUX18_PINMUX18_11_8_GPIO8_15 << SYSCFG_PINMUX18_PINMUX18_11_8_SHIFT)

// RS485 Enable
/* PINMUX0_RS485_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX0_RS485_ENABLE (SYSCFG_PINMUX0_PINMUX0_19_16_GPIO0_11 << SYSCFG_PINMUX0_PINMUX0_19_16_SHIFT)

// UART0
/* PINMUX3_UART0_CTS_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX3_UART0_CTS_ENABLE (SYSCFG_PINMUX3_PINMUX3_27_24_UART0_CTS << SYSCFG_PINMUX3_PINMUX3_27_24_SHIFT)

/* PINMUX3_UART0_RTS_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX3_UART0_RTS_ENABLE (SYSCFG_PINMUX3_PINMUX3_31_28_UART0_RTS << SYSCFG_PINMUX3_PINMUX3_31_28_SHIFT)

/* PINMUX3_UART0_TXD_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX3_UART0_TXD_ENABLE (SYSCFG_PINMUX3_PINMUX3_23_20_UART0_TXD << SYSCFG_PINMUX3_PINMUX3_23_20_SHIFT)

/* PINMUX3_UART0_RXD_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX3_UART0_RXD_ENABLE (SYSCFG_PINMUX3_PINMUX3_19_16_UART0_RXD << SYSCFG_PINMUX3_PINMUX3_19_16_SHIFT)

// UART1
/* PINMUX0_UART1_CTS_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX0_UART1_CTS_ENABLE (SYSCFG_PINMUX0_PINMUX0_23_20_UART1_CTS << SYSCFG_PINMUX0_PINMUX0_23_20_SHIFT)

/* PINMUX0_UART1_RTS_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX0_UART1_RTS_ENABLE (SYSCFG_PINMUX0_PINMUX0_19_16_UART1_RTS << SYSCFG_PINMUX0_PINMUX0_19_16_SHIFT)

/* PINMUX4_UART1_TXD_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX4_UART1_TXD_ENABLE (SYSCFG_PINMUX4_PINMUX4_31_28_UART1_TXD << SYSCFG_PINMUX4_PINMUX4_31_28_SHIFT)

/* PINMUX4_UART1_RXD_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX4_UART1_RXD_ENABLE (SYSCFG_PINMUX4_PINMUX4_27_24_UART1_RXD << SYSCFG_PINMUX4_PINMUX4_27_24_SHIFT)

// UART2
/* PINMUX0_UART2_CTS_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX0_UART2_CTS_ENABLE (SYSCFG_PINMUX0_PINMUX0_31_28_UART2_CTS << SYSCFG_PINMUX0_PINMUX0_31_28_SHIFT)

/* PINMUX0_UART2_RTS_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX0_UART2_RTS_ENABLE (SYSCFG_PINMUX0_PINMUX0_27_24_UART2_RTS << SYSCFG_PINMUX0_PINMUX0_27_24_SHIFT)

/* PINMUX4_UART2_TXD_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX4_UART2_TXD_ENABLE (SYSCFG_PINMUX4_PINMUX4_23_20_UART2_TXD << SYSCFG_PINMUX4_PINMUX4_23_20_SHIFT)

/* PINMUX4_UART2_RXD_ENABLE：EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接。 */
#define PINMUX4_UART2_RXD_ENABLE (SYSCFG_PINMUX4_PINMUX4_19_16_UART2_RXD << SYSCFG_PINMUX4_PINMUX4_19_16_SHIFT)

/****************************************************************************/
/*                                                                          */
/*                       Pin mux                                            */
/*                                                                          */
/****************************************************************************/
/*
void GPIOBank0Pin0PinMuxSetup(void)
{
     unsigned int savePinmux = 0;

     savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) &
                  ~(SYSCFG_PINMUX1_PINMUX1_31_28));

     HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
          (PINMUX1_GPIO0_0_ENABLE | savePinmux);
}

void GPIOBank0Pin1PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) &
                 ~(SYSCFG_PINMUX1_PINMUX1_27_24));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
         (PINMUX1_GPIO0_1_ENABLE | savePinmux);
}
*/

/*==================================================================
 * 函数功能: 把 GPIO bank 0 的 pin 2 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank0Pin2PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) & ~(SYSCFG_PINMUX1_PINMUX1_23_20));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) = (PINMUX1_GPIO0_2_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 0 的 pin 3 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank0Pin3PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) & ~(SYSCFG_PINMUX1_PINMUX1_19_16));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) = (PINMUX1_GPIO0_3_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 0 的 pin 8 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank0Pin8PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) & ~(SYSCFG_PINMUX0_PINMUX0_31_28));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) = (PINMUX0_GPIO0_8_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 0 的 pin 9 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank0Pin9PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) & ~(SYSCFG_PINMUX0_PINMUX0_27_24));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) = (PINMUX0_GPIO0_9_ENABLE | savePinmux);
}

/*
void GPIOBank0Pin5PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) &
                 ~(SYSCFG_PINMUX1_PINMUX1_11_8));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
         (PINMUX1_GPIO0_5_ENABLE | savePinmux);
}

void GPIOBank0Pin6PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) &
                 ~(SYSCFG_PINMUX1_PINMUX1_7_4));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
         (PINMUX1_GPIO0_6_ENABLE | savePinmux);
}
*/

/*==================================================================
 * 函数功能: 把 GPIO bank 2 的 pin 7 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank2Pin7PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(6)) & ~(SYSCFG_PINMUX6_PINMUX6_3_0));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(6)) = (PINMUX6_GPIO2_7_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 2 的 pin 10 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank2Pin10PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) & ~(SYSCFG_PINMUX5_PINMUX5_23_20));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) = (PINMUX5_GPIO2_10_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 2 的 pin 11 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank2Pin11PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) & ~(SYSCFG_PINMUX5_PINMUX5_19_16));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) = (PINMUX5_GPIO2_11_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 2 的 pin 12 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank2Pin12PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) & ~(SYSCFG_PINMUX5_PINMUX5_15_12));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) = (PINMUX5_GPIO2_12_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 2 的 pin 13 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank2Pin13PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) & ~(SYSCFG_PINMUX5_PINMUX5_11_8));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) = (PINMUX5_GPIO2_13_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 4 的 pin 2 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank4Pin2PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) & ~(SYSCFG_PINMUX10_PINMUX10_23_20));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) = (PINMUX10_GPIO4_2_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 4 的 pin 3 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank4Pin3PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) & ~(SYSCFG_PINMUX10_PINMUX10_19_16));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) = (PINMUX10_GPIO4_3_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 4 的 pin 4 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank4Pin4PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) & ~(SYSCFG_PINMUX10_PINMUX10_15_12));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) = (PINMUX10_GPIO4_4_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 4 的 pin 5 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank4Pin5PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) & ~(SYSCFG_PINMUX10_PINMUX10_11_8));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) = (PINMUX10_GPIO4_5_ENABLE | savePinmux);
}

/*
void GPIOBank4Pin7PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) &
                 ~(SYSCFG_PINMUX10_PINMUX10_3_0));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(10)) =
         (PINMUX10_GPIO4_7_ENABLE | savePinmux);
}
*/

/*==================================================================
 * 函数功能: 把 GPIO bank 6 的 pin 0 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank6Pin0PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) & ~(SYSCFG_PINMUX19_PINMUX19_27_24));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) = (PINMUX19_GPIO6_0_ENABLE | savePinmux);
}

/*
void GPIOBank6Pin1PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) &
                 ~(SYSCFG_PINMUX19_PINMUX19_23_20));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) =
         (PINMUX19_GPIO6_1_ENABLE | savePinmux);
}
*/

/*==================================================================
 * 函数功能: 把 GPIO bank 6 的 pin 4 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank6Pin4PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) & ~(SYSCFG_PINMUX19_PINMUX19_11_8));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) = (PINMUX19_GPIO6_4_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 6 的 pin 6 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank6Pin6PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(14)) & ~(SYSCFG_PINMUX14_PINMUX14_7_4));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(14)) = (PINMUX14_GPIO6_6_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 6 的 pin 7 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank6Pin7PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(14)) & ~(SYSCFG_PINMUX14_PINMUX14_3_0));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(14)) = (PINMUX14_GPIO6_7_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 6 的 pin 8 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank6Pin8PinMuxSetup(void)
{
    unsigned int value; /* value：本次协议/存储处理的值，类型和编码保持原接口约定。 */

    value = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13));
    value &= ~SYSCFG_PINMUX13_PINMUX13_31_28;
    value |= SYSCFG_PINMUX13_PINMUX13_31_28_GPIO6_8 << SYSCFG_PINMUX13_PINMUX13_31_28_SHIFT;

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13)) = value;
}

/*
void GPIOBank6Pin12PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13)) &
                 ~(SYSCFG_PINMUX13_PINMUX13_15_12));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13)) =
         (PINMUX13_GPIO6_12_ENABLE | savePinmux);
}

void GPIOBank6Pin13PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13)) &
                 ~(SYSCFG_PINMUX13_PINMUX13_11_8));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13)) =
         (PINMUX13_GPIO6_13_ENABLE | savePinmux);
}
*/

/*
void GPIOBank8Pin2PinMuxSetup(void)
{
    unsigned int savePinmux = 0;

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(3)) &
                 ~(SYSCFG_PINMUX3_PINMUX3_27_24));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(3)) =
         (PINMUX3_GPIO8_2_ENABLE | savePinmux);
}
*/

/*==================================================================
 * 函数功能: 把 GPIO bank 8 的 pin 10 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank8Pin10PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) & ~(SYSCFG_PINMUX18_PINMUX18_31_28));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) = (PINMUX18_GPIO8_10_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 8 的 pin 11 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank8Pin11PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) & ~(SYSCFG_PINMUX18_PINMUX18_27_24));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) = (PINMUX18_GPIO8_11_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 8 的 pin 13 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank8Pin13PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) & ~(SYSCFG_PINMUX18_PINMUX18_19_16));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) = (PINMUX18_GPIO8_13_ENABLE | savePinmux);
}

/*==================================================================
 * 函数功能: 把 GPIO bank 8 的 pin 15 配置为板级指定复用功能，保留其他引脚位
 * 输入参数: 无
 * 输出参数: 对应 SYSCFG PINMUX 寄存器。
 * 返回值: 无
 * 执行说明: 上电板级引脚配置；寄存器偏移和复用码沿用原工程。
 *================================================================*/
void GPIOBank8Pin15PinMuxSetup(void)
{
    unsigned int savePinmux = 0; /* savePinmux：引脚复用寄存器的读改写缓存，保留非目标引脚字段。 */

    savePinmux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) & ~(SYSCFG_PINMUX18_PINMUX18_11_8));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(18)) = (PINMUX18_GPIO8_15_ENABLE | savePinmux);
}

/*
void RS485PinMuxSetup(void)
{
    unsigned int savePinMux = 0;

    savePinMux = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) & \
                     ~(SYSCFG_PINMUX0_PINMUX0_19_16 | \
                       SYSCFG_PINMUX0_PINMUX0_19_16));

    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) = (PINMUX0_RS485_ENABLE | savePinMux);
}
*/

/*==================================================================
 * 函数功能: 按 UART 实例及流控选择设置串口引脚复用
 * 输入参数: instanceNum：UART 实例编号，沿用板级可用实例
 *           modemCtrlChoice：UART RTS/CTS 流控引脚选择
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void UARTPinMuxSetup(unsigned int instanceNum, unsigned int modemCtrlChoice)
{
    unsigned int svPinMuxRtsCts = 0; /* svPinMuxRtsCts：UART 流控引脚复用寄存器的读改写缓存。 */
    unsigned int svPinMuxTxdRxd = 0; /* svPinMuxTxdRxd：UART 收发引脚复用寄存器的读改写缓存。 */

    if (0 == instanceNum)
    {
        if (TRUE == modemCtrlChoice)
        {
            svPinMuxRtsCts = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(3)) &
                              ~(SYSCFG_PINMUX3_PINMUX3_27_24 | SYSCFG_PINMUX3_PINMUX3_31_28));

            HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(3)) =
                (PINMUX3_UART0_CTS_ENABLE | PINMUX3_UART0_RTS_ENABLE | svPinMuxRtsCts);
        }

        svPinMuxTxdRxd = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(3)) &
                          ~(SYSCFG_PINMUX3_PINMUX3_23_20 | SYSCFG_PINMUX3_PINMUX3_19_16));

        HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(3)) =
            (PINMUX3_UART0_TXD_ENABLE | PINMUX3_UART0_RXD_ENABLE | svPinMuxTxdRxd);
    }

    else if (1 == instanceNum)
    {
        if (TRUE == modemCtrlChoice)
        {
            svPinMuxRtsCts = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) &
                              ~(SYSCFG_PINMUX0_PINMUX0_23_20 | SYSCFG_PINMUX0_PINMUX0_19_16));

            HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) =
                (PINMUX0_UART1_CTS_ENABLE | PINMUX0_UART1_RTS_ENABLE | svPinMuxRtsCts);
        }

        svPinMuxTxdRxd = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(4)) &
                          ~(SYSCFG_PINMUX4_PINMUX4_31_28 | SYSCFG_PINMUX4_PINMUX4_27_24));

        HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(4)) =
            (PINMUX4_UART1_TXD_ENABLE | PINMUX4_UART1_RXD_ENABLE | svPinMuxTxdRxd);
    }

    else if (2 == instanceNum)
    {
        if (TRUE == modemCtrlChoice)
        {
            svPinMuxRtsCts = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) &
                              ~(SYSCFG_PINMUX0_PINMUX0_31_28 | SYSCFG_PINMUX0_PINMUX0_27_24));

            HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) =
                (PINMUX0_UART2_CTS_ENABLE | PINMUX0_UART2_RTS_ENABLE | svPinMuxRtsCts);
        }

        svPinMuxTxdRxd = (HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(4)) &
                          ~(SYSCFG_PINMUX4_PINMUX4_23_20 | SYSCFG_PINMUX4_PINMUX4_19_16));

        HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(4)) =
            (PINMUX4_UART2_TXD_ENABLE | PINMUX4_UART2_RXD_ENABLE | svPinMuxTxdRxd);
    }

    else
    {
    }
}
