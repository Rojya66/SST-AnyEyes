#include "hw_types.h"
#include "hw_syscfg0_C6748.h"
#include "gpio.h"
#include "uart.h"

#include "main.h"
#include "ADC_Sample.h"
#include "DPRAM.h"
#include "FaultCheck.h"
#include "MaintenanceProtocol.H"
#include "ChbDabControl.h"
#include "ChbApplication.h"
#include "ChbBuildConfig.h"
#include "SstDebug.h"
#include "SstFault.h"
#include "SstParams.h"
#include "SstCommand.h"

/* 原头文件大小写拼写不一致，这里声明实际使用的变量。 */
extern unsigned char Flag_RemoteControlRegulate; /* Flag_RemoteControlRegulate：DPRAM 通知 ISR 置位、后台消费的遥控/遥调待处理标志。 */

extern DPRAM_RemMeaSigFloat PORT_RealTime_DATA; /* PORT_RealTime_DATA：端口遥测的浮点物理量缓存及兼容状态。 */

extern unsigned short Port_State; /* Port_State：旧端口状态编号，迁移核心维护兼容镜像。 */

/*
 * 完成芯片和板级外设初始化。
 *
 * 芯片、引脚和定时器顺序沿用原工程。TAD/FAULT 和调试通信在运行对象
 * 初始化后开放，避免 GPIO 或控制变量尚未准备好时进入控制中断。
 */
/*==================================================================
 * 函数功能: 按原顺序完成时钟、EMIFA、GPIO、Timer2 和指示灯初始化
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void ChbApplication_InitHardware(void)
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

/*
 * 保留原 20 秒上电等待，所有模式均只等待定时器到时；
 * 此阶段不解析通信命令、不接收远程启停。
 */
/*==================================================================
 * 函数功能: 等待原 20 s 上电延时到达并恢复指示灯、看门狗初值
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void ChbApplication_WaitPowerOnDelay(void)
{
    while(!Flag_20s)
    {

    }

    Flag_20s = 0;
    Flag_WDI = 0;
    GPIOPinWrite(SOC_GPIO_0_REGS, 40, Flag_WDI);
    GPIOPinWrite(SOC_GPIO_0_REGS, 3, 0);
    GPIOPinWrite(SOC_GPIO_0_REGS, 4, 0);
}

/*
 * 初始化 HVAC 运行变量、持久化参数、GPIO 中断和简化后的 CHB/DAB 状态机。
 */
/*==================================================================
 * 函数功能: 初始化运行对象及可选持久化参数，随后开放 TAD/FAULT 中断
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void ChbApplication_InitRuntime(void)
{
    Initial_Variable();
    Variable_RST();
#if CHB_LOAD_EEPROM_PARAMETERS && SST_ENABLE_MAINTENANCE
    /* 由编译宏决定是否使用EEPROM覆盖本地默认参数。 */
    MaintenanceProtocol_LoadPersistent();
#endif
    FaultLatch_Clear(0, 1);

    Port_State = PortStatusNum_Initial;
    Port_RunStatus = PortStatusNum_Initial;
    ChbDabControl_Init();
    SstFault_Init();
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

/*==================================================================
 * 函数功能: 消费 1 s 标志，翻转看门狗及运行指示灯
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
static void ChbApplication_ServiceHeartbeat(void)
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

#if SST_ENABLE_DPRAM
/*==================================================================
 * 函数功能: 读取可选 DPRAM 遥控/遥调块并交给统一命令入口
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
static void ChbApplication_ServiceRemoteControl(void)
{
#if SST_ENABLE_DPRAM
    if(!Flag_RemoteControlRegulate)
    {
        return;
    }

    Flag_RemoteControlRegulate = 0;
    rtt = !rtt;
    FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led3 = rtt;

    DPRAM_Read();                         // 读取上位机写入的控制块
    if(FLAG_NEW_L == ReadBlock_remControl)
    {
        /* Preserve the DPRAM addresses; all power gates belong to the core. */
        SstCommand_ApplyDpram(
            PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16,
            PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16,
            PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16,
            PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16,
            PORT_Control_DATA_Buf.DPRAM_RemControl.COM_FAULT_CLEAR.COM_FAULT_CLEAR16);
    }
#if SST_ALLOW_REMOTE_COMMANDS
    else if((FLAG_NEW_L == ReadBlock_remRegulate) &&
            SstParams_IsStopped() && !SstParams_IsStorageBusy())
    {
        RemoteRegulate_Dispose_2(0);
    }
#endif
#else
    /* 仿真器和485模式不读取DPRAM控制块。 */
    return;
#endif
}

/* HVAC 只需要轮询本端口实际存在的插件数据块。 */
/*==================================================================
 * 函数功能: 按轮询顺序回写可选 DPRAM 端口遥测及有效插件块
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
static void ChbApplication_ServiceTelemetry(void)
{
#if SST_ENABLE_DPRAM
    if(!Flag_RemoteMeasure)
    {
        return;
    }

    Flag_RemoteMeasure = 0;
    DPRAM_Write(WriteBlock_remMeaSig);    // 先回写端口公共状态

    if(WriteBlock_Index < WriteBlock_PluginUnit_7)
    {
        DPRAM_Write(WriteBlock_Index++);
        if(WriteBlock_Index == WriteBlock_PluginUnit_7)
        {
            WriteBlock_Index++;
        }
    }
    else if(WriteBlock_Index < WriteBlock_PluginUnit_11)
    {
        DPRAM_Write(WriteBlock_Index++);
        if(WriteBlock_Index == WriteBlock_PluginUnit_11)
        {
            WriteBlock_Index++;
        }
    }
    else if(WriteBlock_Index < WriteBlock_PluginUnit_15)
    {
        DPRAM_Write(WriteBlock_Index++);
        if(WriteBlock_Index == WriteBlock_PluginUnit_15)
        {
            WriteBlock_Index++;
        }
    }
    else if(WriteBlock_Index == WriteBlock_PluginUnit_17)
    {
        DPRAM_Write(WriteBlock_Index++);
    }
    else if(WriteBlock_Index == WriteBlock_remRegulate)
    {
        DPRAM_Write(WriteBlock_Index);
        WriteBlock_Index = WriteBlock_PluginUnit_2;
    }
    else
    {
        /* 每次只发送一个插件块，避免主循环被连续EMIFA访问长时间占用。 */
        DPRAM_Write(WriteBlock_Index++);
    }
#else
    /* 仿真器和485模式不主动回写DPRAM遥测块。 */
    return;
#endif
}

#endif /* SST_ENABLE_DPRAM: helpers have no users in serial-only builds. */

/* Cooperative background work: no protocol or fault reporting in TAD. */
/*==================================================================
 * 函数功能: 执行主循环的正常状态、通信、故障上报及参数存储任务
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void ChbApplication_ServiceNormal(void)
{
    ChbApplication_ServiceHeartbeat();
    ChbDabControl_Service();
    SstFault_Service();
#if SST_ENABLE_DPRAM
    ChbApplication_ServiceRemoteControl();
    ChbApplication_ServiceTelemetry();
#endif
    SstDebug_Service();
    Port_RealTimeParameter();
}

/*==================================================================
 * 函数功能: 保留原开环桥接服务入口并交给迁移后的主循环服务
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void ChbApplication_ServiceOpenLoopBridge(void)
{
    ChbApplication_ServiceNormal();
}
