#include "Redef_Interrupt.h"
#include "SystemInit.h"
#include "ControlInit.h"
#include "DpramConfig.h"
#include "DPRAM.h"
#include "PortStatusData.h"
#include "SoftTimeData.h"
#include "SstBoard.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "hw_types.h"
#include "hw_syscfg0_C6748.h"
#include "gpio.h"
#include "FaultCheck.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstDebug.h"
#include "SstFault.h"
#include "SstParams.h"
#include "SstDpramMaintenance.h"
#include "SstBenchInputs.h"

/*==================================================================
 * 函数功能: 按原顺序完成时钟、EMIFA、GPIO、Timer2 和指示灯初始化
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void SystemInit_Hardware(void)
{
    PSCInit();

    /* 解锁系统配置寄存器，后续才能修改引脚复用和 EMIFA 配置。 */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_KICK0R) = SYSCFG_KICK0R_UNLOCK;
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_KICK1R) = SYSCFG_KICK1R_UNLOCK;

    EMIFASetup();
    GPIOBankPinMuxSet();
    GPIOBankPinInit();

    InterruptInit();
    TimerInit();
    TimerInterruptInit();

    /* 上电等待阶段的指示灯和看门狗初值。 */
    GPIOPinWrite(SOC_GPIO_0_REGS, 3, 1);
    GPIOPinWrite(SOC_GPIO_0_REGS, 4, 1);
    Flag_WDI = 1;
    GPIOPinWrite(SOC_GPIO_0_REGS, 40, Flag_WDI);
}

/*==================================================================
 * 函数功能: 等待原 20 s 上电延时到达并恢复指示灯、看门狗初值
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void SystemInit_WaitPowerOnDelay(void)
{
    while (!Flag_20s)
    {
    }

    Flag_20s = 0;
    Flag_WDI = 0;
    GPIOPinWrite(SOC_GPIO_0_REGS, 40, Flag_WDI);
    GPIOPinWrite(SOC_GPIO_0_REGS, 3, 0);
    GPIOPinWrite(SOC_GPIO_0_REGS, 4, 0);
}

/*==================================================================
 * 函数功能: 初始化运行对象及可选持久化参数，随后开放 TAD/FAULT 中断
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void SystemInit_Runtime(void)
{
    Initial_Variable();
    Variable_RST();
    FaultLatch_Clear(0, 1);

    Port_State = PortStatusNum_Initial;
    Port_RunStatus = PortStatusNum_Initial;
    ChbDabControl_Init();
#if SST_BENCH_INPUT_SIM
    SstBenchInputs_Init(); // 测试输入初始化，保持启动命令为0
#endif
    SstFault_Init();
    SstDpramMaintenance_Init();
    SstDebug_Init();
#if CHB_LOAD_EEPROM_PARAMETERS
    SstParameters_LoadPersistent();
#endif
    /* Control objects and safe outputs exist before TAD/FAULT enable. */
    GPIOBankPinInterruptInit();

    /* 只有DPRAM模式需要清除mailbox并接收上位机通知。 */
#if SST_ENABLE_DPRAM
    HWREGH(DPRAM_Maibox_Left);
    GPIOPinWrite(SOC_GPIO_0_REGS, 144, 0);
#endif
}
