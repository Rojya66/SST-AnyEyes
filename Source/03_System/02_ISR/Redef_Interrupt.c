#include "FpgaIo.h"
#include "Ft3Recorder.h"
#include "DacOutput.h"
#include "SstPortStatus.h"
#include "SstBoardConfig.h"
#include "soc_C6748.h"
#include "ControlData.h"
#include "DpramHostData.h"
#include "Ft3RecorderData.h"
#include "ModuleVoltageCheck.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SoftTimeData.h"
#include "SstControlConfig.h"
//###########################################################################
// FILE: Redef_Interrupt.c
// FUNCTION: C6748 + FPGA 中断入口。采样控制/快速故障/软件节拍/DPRAM通知分别保留原执行顺序。
//           本模块维护四个完整ISR和中断注册；SystemInit按原安全门槛调用注册接口。
//###########################################################################
#include "Redef_Interrupt.h"
#include "hw_types.h"
#include "interrupt.h"
#include "gpio.h"
#include "timer.h"
#include "Hardware.h"
#include "DPRAM.h"
#include "ADC_Sample.h"
#include "FaultCheck.h"
#include "HVAC_Strategy.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"




/****************************************************************************/
/*                                                                          */
/*              T2 INT routine                      10ms                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 应答 Timer2 中断并更新原软件定时、状态及保护服务
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Timer2Isr(void)
{
    //  float tempInt;
    //  unsigned char i;

    TimerIntDisable(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);

    IntEventClear(SYS_INT_T64P2_TINTALL);
    // TimerIntStatusClear(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);
    TimerIntStatusClear(SOC_TMR_2_REGS, TMR_INTSTAT12_TIMER_NON_CAPT);

    TimerIntEnable(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);

    ChbDabControl_Tick10ms(); // 10 ms节拍，推进状态机和超时计数

    if (Flag_Clear_0 == 1)
    {
        Flag_Clear_0 = 0;
        Flag_Clear_1 = 1;
    }
    else
    {
        if (Flag_Clear_1 == 1)
        {
            Flag_Clear_1 = 0;
            FaultLatch_Clear(1, Flag_Clear_zhi);
        }
    }

    cout_Wait--;
    if (cout_Wait == 0)
    {
        Flag_Wait = 1;
    }

    if (Flag_StopWait)
    {
        coutStopWait--;
        if (coutStopWait == 0)
        {
            Flag_StopWait = 0;
            Flag_StopWaitEnd = 1;
        }
    }

    cout1s++;
    if (cout1s == 100)
    {
        cout1s = 0;
        Flag_1s = 1;
    }

    cout20s++;
    if (cout20s == 2000)
    {
        cout20s = 0;
        Flag_20s = 1;
    }

    if (Flag_50ms)
    {
        cout50ms++;
        if (cout50ms == 5)
        {
            cout50ms = 0;
            Flag_50ms = 0;
        }
    }

    cout100ms++;
    if (cout100ms == 10)
    {
        cout100ms = 0;
        Flag_100ms = 1;
    }

    coutRemoteMeasure++;
    if (coutRemoteMeasure == 3)
    {
        coutRemoteMeasure = 0;
        Flag_RemoteMeasure = 1;
    }

    if (Flag_Switch500ms)
    {
        coutSwitch500ms--;
        if (coutSwitch500ms == 0)
        {
            Flag_Switch500ms = 0;
        }
    }

    if (Flag_Switch1s)
    {
        coutSwitch1s--;
        if (coutSwitch1s == 0)
        {
            Flag_Switch1s = 0;
        }
    }

    if (Flag_Precharge500ms)
    {
        coutPrecharge500ms++;
        if (coutPrecharge500ms == 1000)
        {
            coutPrecharge500ms = 0;
            Flag_Precharge500ms = 0;
        }
    }

    if (Flag_PreStart3s)
    {
        coutPreStart3s++;
        if (coutPreStart3s == 300)
        {
            coutPreStart3s = 0;
            Flag_PreStart3s = 0;
        }
    }

    if (Flag_SVG1s)
    {
        coutSVG1s++;
        if (coutSVG1s == 500)
        {
            coutSVG1s = 0;
            Flag_SVG1s = 0;
        }
    }

    countWaitPowerStart10s++;
    if (countWaitPowerStart10s == 1000)
    {
        countWaitPowerStart10s = 0;
        Flag_WaitPowerStart10s = 1;
    }

#ifdef HVAC_online
    if (Flag_V_L)
    {
        Cout_V_L++;
        if (Cout_V_L > LV_Time_1)
        {
            Flag_V_L1_END = 1;
        }

        if (Cout_V_L > LV_Time_2)
        {
            Flag_V_L2_MID = 1;
        }

        if (Cout_V_L > LV_Time_1)
        {
            Flag_V_L2_END = 1;
            Flag_V_L = 0;
            Cout_V_L = 0;
        }
    }

    if (ISO_counter_on < 500)
    {
        ISO_counter_on++;
    }

    if (ISO_counter_off < 500)
    {
        ISO_counter_off++;
    }
#endif

    // 原HVAC软件保护计时：以下计数在Timer2中断内按10 ms递增，非TAD样本计数。

#ifdef HVAC_online
    if (counter_OC_flag == 1)
    {
        counter_OC_time++;
    }
    if (counter_OC_flag1 == 1)
    {
        counter_OC_time1++;
    }
    if (counter_OV_flag == 1)
    {
        counter_OV_time++;
    }
#endif
}

/****************************************************************************/
/*                                                                          */
/*              TAD INT routine                                              */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 应答 FPGA TAD 外部中断并按原顺序完成 10 kHz 采样、保护及控制
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void FPGAIsr(void)
{
    //  float datemp;
    GPIOBankIntDisable(SOC_GPIO_0_REGS, 4);
    IntEventClear(SYS_INT_GPIO_B4INT);

    if (GPIOPinIntStatus(SOC_GPIO_0_REGS, 69) == GPIO_INT_PEND) // TAD中断：每个采样周期执行一次快速控制
    {
        GPIOPinIntClear(SOC_GPIO_0_REGS, 69);
        GPIOPinWrite(SOC_GPIO_0_REGS, 43, 1);
        GPIOPinWrite(SOC_GPIO_0_REGS, 45, 1);

#ifdef HVAC_online
        Port_StateUpdate_Read(); // 读取接触器、故障和端口状态

        Port_AD_Sample(0); // 采样交流电压、电流及直流电压
        HVAC_RealTimeParameter(); // 累积200点RMS窗口并更新功率；有效值由后台开方发布
        SVG_Udc_Average(); // 计算三相单元直流平均值
        ChbDabControl_FastStep(); // PLL、开/闭环调制、DAB占空比爬坡
        if ((FaultTypeFLAG.Bbits.Module_Fault) && (ModuleFault_DisposeTimes)) // 模块故障立即封波
        {
            FaultDispose();
            ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
        }

        //if(SVG_Udc_Ave > OV_allport_shutdown)
        //{
        //  FaultDisposeFLAG.Bbits.Port_StopAll=1;
        //  PortRunFLAG.Bbits.Stop_AllPort=1;
        //  Port_StateUpdate_Write();
        //}

        //edited by shiqi, end

        if (((PortRunFLAG.Bbits.PWMState_50Hz == 1) && (PortRunFLAG.Bbits.PWMState_Dab == 1)) &&
            (PortRunFLAG.Bbits.Stop_AllPort)) //系统停机成功
        {
            PortController_Communication(1);
            Port_AllStopFlag_RST();
        }

        Port_StateUpdate_Write(); // 写FPGA主控HVAC_STATUS及兼容状态，保持TAD内调用

        //edited by shiqi
        if (Port_RealFault.Bbits.PORT_FAULT_bit15)
        {
            ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
        }
        //edited end

        //edited by shiqi, start, fault recorder
        if (dataPackNum < 2)
        {
            dataPackNum++;
        }
        else
        {
            dataPackNum = 1;
        }
        FT3_PluginUint_Write(2, dataPackNum);
        FT3_PluginUint_Read(2);

#ifdef COM_TEST_PluginUnit
/*
//PHASE A
//          Wrong_LEFT_A=FPGA_Interface[Com_A1_PluginUnit_5-2].Left_Ref;
//          Wrong_LEFT_A=Wrong_LEFT_A+FPGA_Interface[Com_A2_PluginUnit_6-2].Left_Ref;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_LEFT_A-LastWrong_LEFT_A)>1300)
                {
                    WrongTimes_LEFT_A++;
                    if(WrongTimes_LEFT_A==65535)
                    {
                        WrongTimes_LEFT_A=0;
                    }
                }
                else if((Wrong_LEFT_A-LastWrong_LEFT_A)<(-1300))
                {
                    WrongTimes_LEFT_A++;
                    if(WrongTimes_LEFT_A==65535)
                    {
                        WrongTimes_LEFT_A=0;
                    }
                }
            }
            LastWrong_LEFT_A=Wrong_LEFT_A;

//          Wrong_RIGHT_A=FPGA_Interface[Com_A1_PluginUnit_5-2].Right_Ref;
//          Wrong_RIGHT_A=Wrong_RIGHT_A+FPGA_Interface[Com_A2_PluginUnit_6-2].Right_Ref;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_RIGHT_A-LastWrong_RIGHT_A)>1300)
                {
                    WrongTimes_RIGHT_A++;
                    if(WrongTimes_RIGHT_A==65535)
                    {
                        WrongTimes_RIGHT_A=0;
                    }
                }
                else if((Wrong_RIGHT_A-LastWrong_RIGHT_A)<(-1300))
                {
                    WrongTimes_RIGHT_A++;
                    if(WrongTimes_RIGHT_A==65535)
                    {
                        WrongTimes_RIGHT_A=0;
                    }
                }
            }
            LastWrong_RIGHT_A=Wrong_RIGHT_A;

//PHASE C
//          Wrong_LEFT=FPGA_Interface[Com_C1_PluginUnit_13-2].Left_Ref;
//          Wrong_LEFT=Wrong_LEFT+FPGA_Interface[Com_C2_PluginUnit_14-2].Left_Ref;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_LEFT-LastWrong_LEFT)>1300)
                {
                    WrongTimes_LEFT++;
                    if(WrongTimes_LEFT==65535)
                    {
                        WrongTimes_LEFT=0;
                    }
                }
                else if((Wrong_LEFT-LastWrong_LEFT)<(-1300))
                {
                    WrongTimes_LEFT++;
                    if(WrongTimes_LEFT==65535)
                    {
                        WrongTimes_LEFT=0;
                    }
                }
            }
            LastWrong_LEFT=Wrong_LEFT;

//          Wrong_RIGHT=FPGA_Interface[Com_C1_PluginUnit_13-2].Right_Ref;
//          Wrong_RIGHT=Wrong_RIGHT+FPGA_Interface[Com_C2_PluginUnit_14-2].Right_Ref;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_RIGHT-LastWrong_RIGHT)>1300)
                {
                    WrongTimes_RIGHT++;
                    if(WrongTimes_RIGHT==65535)
                    {
                        WrongTimes_RIGHT=0;
                    }
                }
                else if((Wrong_RIGHT-LastWrong_RIGHT)<(-1300))
                {
                    WrongTimes_RIGHT++;
                    if(WrongTimes_RIGHT==65535)
                    {
                        WrongTimes_RIGHT=0;
                    }
                }
            }
            LastWrong_RIGHT=Wrong_RIGHT;

//PHASE A1
//          Wrong_Reference_A1=FPGA_Interface[Com_A1_PluginUnit_5-2].Reference;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_Reference_A1-LastWrong_Reference_A1)>1300)
                {
                    WrongTimes_Reference_A1++;
                    if(WrongTimes_Reference_A1==65535)
                    {
                        WrongTimes_Reference_A1=0;
                    }
                }
                else if((Wrong_Reference_A1-LastWrong_Reference_A1)<(-1300))
                {
                    WrongTimes_Reference_A1++;
                    if(WrongTimes_Reference_A1==65535)
                    {
                        WrongTimes_Reference_A1=0;
                    }
                }
            }
            LastWrong_Reference_A1=Wrong_Reference_A1;
//PHASE A2
//          Wrong_Reference_A2=FPGA_Interface[Com_A2_PluginUnit_6-2].Reference;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_Reference_A2-LastWrong_Reference_A2)>1300)
                {
                    WrongTimes_Reference_A2++;
                    if(WrongTimes_Reference_A2==65535)
                    {
                        WrongTimes_Reference_A2=0;
                    }
                }
                else if((Wrong_Reference_A2-LastWrong_Reference_A2)<(-1300))
                {
                    WrongTimes_Reference_A2++;
                    if(WrongTimes_Reference_A2==65535)
                    {
                        WrongTimes_Reference_A2=0;
                    }
                }
            }
            LastWrong_Reference_A2=Wrong_Reference_A2;
//PHASE C1
            Wrong_Reference_C1=FPGA_Interface[Com_B1_PluginUnit_9-2].Reference;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_Reference_C1-LastWrong_Reference_C1)>1300)
                {
                    WrongTimes_Reference_C1++;
                    if(WrongTimes_Reference_C1==65535)
                    {
                        WrongTimes_Reference_C1=0;
                    }
                }
                else if((Wrong_Reference_C1-LastWrong_Reference_C1)<(-1300))
                {
                    WrongTimes_Reference_C1++;
                    if(WrongTimes_Reference_C1==65535)
                    {
                        WrongTimes_Reference_C1=0;
                    }
                }
            }
            LastWrong_Reference_C1=Wrong_Reference_C1;
            HWREGH(0x60000622)=WrongTimes_Reference_C1+1;
//PHASE C2
            Wrong_Reference_C2=FPGA_Interface[Com_B2_PluginUnit_10-2].Reference;

            if(PortRunFLAG.Bbits.PWMState_50Hz==0)
            {
                if((Wrong_Reference_C2-LastWrong_Reference_C2)>1300)
                {
                    WrongTimes_Reference_C2++;
                    if(WrongTimes_Reference_C2==65535)
                    {
                        WrongTimes_Reference_C2=0;
                    }
                }
                else if((Wrong_Reference_C2-LastWrong_Reference_C2)<(-1300))
                {
                    WrongTimes_Reference_C2++;
                    if(WrongTimes_Reference_C2==65535)
                    {
                        WrongTimes_Reference_C2=0;
                    }
                }
            }
            LastWrong_Reference_C2=Wrong_Reference_C2;
            HWREGH(0x60000624)=WrongTimes_Reference_C2+1;
*/
#endif

#endif

        DAC_Output();

        GPIOPinWrite(SOC_GPIO_0_REGS, 43, 0);
        GPIOPinWrite(SOC_GPIO_0_REGS, 45, 0);
    }

    GPIOBankIntEnable(SOC_GPIO_0_REGS, 4);
}

/****************************************************************************/
/*                                                                          */
/*              FAULT INT routine                                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 应答 FPGA 硬件故障中断并执行原快速故障处置
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void FAULTIsr(void)
{
    GPIOBankIntDisable(SOC_GPIO_0_REGS, 8);
    IntEventClear(SYS_INT_GPIO_B8INT);

    if (GPIOPinIntStatus(SOC_GPIO_0_REGS, 142) == GPIO_INT_PEND) //FAULT  Interrupt routine
    {
        GPIOPinIntClear(SOC_GPIO_0_REGS, 142);
        //          TestINT=!TestINT;
        //          FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led2=TestINT;

        FaultCheckAndDispose_StepOne(); //第1步
        if ((PortLatch_TAD) || (ModuleFault_DisposeTimes)) //端口或模块故障
        {
            FaultDispose(); //第2步
            ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
        }
    }

    GPIOBankIntEnable(SOC_GPIO_0_REGS, 8);
}

/****************************************************************************/
/*                                                                          */
/*              DPRAM INT routine                                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 应答可选 DPRAM GPIO 通知并置后台读取标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void DPRAMIsr(void)
{
    GPIOBankIntDisable(SOC_GPIO_0_REGS, 6);
    IntEventClear(SYS_INT_GPIO_B6INT);

    if (GPIOPinIntStatus(SOC_GPIO_0_REGS, 101) == GPIO_INT_PEND) //DPRAM  Interrupt routine
    {
        GPIOPinIntClear(SOC_GPIO_0_REGS, 101);

        Flag_RemoteControlRegulate = 1;
    }

    GPIOBankIntEnable(SOC_GPIO_0_REGS, 6);
}

/*==================================================================
 * 函数功能: 将 Timer2 中断源与原 ISR 及 CPU 向量绑定
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
void TimerInterruptInit(void)
{
    /* Timer2保留保护计数、清故障和原软件时基推进，完整正文位于ISR模块。 */
    IntRegister(C674X_MASK_INT8, Timer2Isr);
    IntEventMap(C674X_MASK_INT8, SYS_INT_T64P2_TINTALL);
    IntEnable(C674X_MASK_INT8);

    TimerIntEnable(SOC_TMR_2_REGS, TMR_INT_TMR12_NON_CAPT_MODE);
}

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
    GPIOIntTypeSet(SOC_GPIO_0_REGS, 69, GPIO_INT_TYPE_RISEDGE); // TAD采样同步脉冲
//    GPIOIntTypeSet(SOC_GPIO_0_REGS, 70, GPIO_INT_TYPE_RISEDGE);
#if SST_ENABLE_DPRAM
    GPIOIntTypeSet(SOC_GPIO_0_REGS, 101, GPIO_INT_TYPE_FALLEDGE); //DPRAM
#endif

    GPIOIntTypeSet(SOC_GPIO_0_REGS, 142, GPIO_INT_TYPE_RISEDGE); // FPGA故障脉冲

    // 分别使能 TAD、可选 DPRAM 和故障脉冲所在 GPIO Bank 的中断输出。
    GPIOBankIntEnable(SOC_GPIO_0_REGS, 4); // GPIO69 位于 Bank4，接收 FPGA TAD 同步脉冲。
// DPRAM 通知使用 Bank6；FPGA 故障脉冲使用 Bank8。
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
