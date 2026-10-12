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
