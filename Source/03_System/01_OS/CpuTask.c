#include "ChbDabControl.h"
#include "soc_C6748.h"
#include "timer.h"
#include "stdint.h"
#include "CpuTask.h"
#include "SoftTimeData.h"
#include "DpramHostData.h"
#include "FpgaRegister.h"
#include "SstControlConfig.h"
#include "SstBoardConfig.h"
#include "gpio.h"

/*==================================================================
 * 函数功能: 在硬件初始化前恢复原调试和DPRAM指示标志初值
 * 输入参数: 无
 * 输出参数: 所属模块状态；调用顺序和原实现保持。
 * 返回值: 无
 * 执行说明: 系统初始化或主循环；不增加 FPGA 中断调用。
 *================================================================*/
void CpuTask_InitBootFlags(void)
{
    Flag_testT = 0;
    rtt = 0;
}

/*==================================================================
 * 函数功能: 消费 1 s 标志，翻转看门狗及运行指示灯
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void CpuTask_ServiceHeartbeat(void)
{
    if(!Flag_1s)
    {
        return;
    }

    Flag_1s = 0;
    Flag_temp = !Flag_temp;
    Flag_WDI = !Flag_WDI;
    GPIOPinWrite(SOC_GPIO_0_REGS, 40, Flag_WDI);
    GPIOPinWrite(SOC_GPIO_0_REGS, 4, Flag_temp);
    FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led0 = Flag_temp;
}


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
