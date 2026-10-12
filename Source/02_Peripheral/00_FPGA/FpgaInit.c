#include "ChbDabControl.h"
#include "SstBoardConfig.h"
#include "ProtectionData.h"
#include "SstControlConfig.h"
/* 文件职责: FpgaInit 所属数据域的默认初始化；保持原连续语句段。 */
#include "FpgaInit.h"
#include "SstTypes.h"
#include "ControlData.h"
#include "FpgaRegister.h"
#include "Ft3RecorderData.h"
#include "PortStatusData.h"
#include "DpramHostData.h"
#include "FpgaIo.h"
#include "PwmGate.h"
#include "ChbBuildConfig.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 先封锁PWM并写入原上电安全控制字
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void FpgaInit_SetSafeOutputs(void)
{
    PWMPluseEnable_Initial();
    TX_EN_Control_Write();
    Module_PM_CTL.PM_CTL16=0x0F00;
    Module_Control_Write();
    Port_BKU_Send(1);
    Port_FB_EN=0;
    Port_FB_EN_Set();
//  FaultLatch_Clear(0,1);

}

/*==================================================================
 * 函数功能: 写插件ID、有效模块位图、模块数量及调制参考比例
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void FpgaInit_ConfigureModuleLayout(void)
{
    uchar i; /* i：本分域初始化的循环索引，每个循环均从0开始。 */
    ushort InitialTemp; /* InitialTemp：模块位图和模块数量换算的原整字临时值。 */
    float InitialTempFloat; /* InitialTempFloat：参考比例或控制限幅计算的原浮点临时值。 */
    //ID.ID16
    FPGA_Interface[0].ID.ID16 = 0x6050;
    FPGA_Interface[1].ID.ID16 = 0x6041;
    FPGA_Interface[2].ID.ID16 = 0x6002;
    FPGA_Interface[3].ID.ID16 = 0x6013;
    FPGA_Interface[4].ID.ID16 = 0x6014;
    FPGA_Interface[5].ID.ID16 = 0x6035;
    FPGA_Interface[6].ID.ID16 = 0x6026;
    FPGA_Interface[7].ID.ID16 = 0x6017;
    FPGA_Interface[8].ID.ID16 = 0x6018;
    FPGA_Interface[9].ID.ID16 = 0x6039;
    FPGA_Interface[10].ID.ID16 = 0x602A;
    FPGA_Interface[11].ID.ID16 = 0x601B;
    FPGA_Interface[12].ID.ID16 = 0x601C;
    FPGA_Interface[13].ID.ID16 = 0x603D;
    FPGA_Interface[14].ID.ID16 = 0x602E;
    /* Slot 17 uses the dedicated FT3 register section at 0x60001E00. */

/*
 * PM is a bit map, not only a module count.
 * bitX=1 means TX/RX channel X on this FPGA501 communication card is
 * connected to a valid power module.
 *
 * HVAC uses two FPGA501 communication cards per phase:
 * A1/A2, B1/B2, C1/C2. These are module groups, not the left/right
 * H-bridge arms. The left/right bridge-arm PWM is generated inside
 * each module command by FPGA501.
 */
//PM
    HA_PM_A1=CHB_PM_A1_MASK;
    HA_PM_A2=CHB_PM_A2_MASK;
    HA_PM_B1=CHB_PM_B1_MASK;
    HA_PM_B2=CHB_PM_B2_MASK;
    HA_PM_C1=CHB_PM_C1_MASK;
    HA_PM_C2=CHB_PM_C2_MASK;

    FPGA_Interface[Com_A1_PluginUnit_5-2].PM=HA_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6-2].PM=HA_PM_A2;
    FPGA_Interface[Com_B1_PluginUnit_9-2].PM=HA_PM_B1;
    FPGA_Interface[Com_B2_PluginUnit_10-2].PM=HA_PM_B2;
    FPGA_Interface[Com_C1_PluginUnit_13-2].PM=HA_PM_C1;
    FPGA_Interface[Com_C2_PluginUnit_14-2].PM=HA_PM_C2;

    /*
     * Convert PM bit maps into total chain-link module counts.
     * FPGA501 uses PM to know which local optical channels are valid,
     * and uses NUM as the total module count of the whole phase for
     * NLM+PWM allocation.
     *
     * 当前位图：A1=0x0001、A2=0x0000，A相1模块；
     * B1/C1=0x007F、B2/C2=0x003F，B/C各13模块。
     * 按置位数量计算各相NUM，保持修改位图后的对应关系。
     * The popcount form is kept so runtime
     * PM changes keep NUM and ReferenceDEF consistent.
     */
    //module number
    NHA_module_A=0;
    NHA_module_B=0;
    NHA_module_C=0;

    for(i=0;i<8;i++)
    {
        InitialTemp=HA_PM_A1;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_A++;
        }
        InitialTemp=HA_PM_A2;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_A++;
        }
    }

    for(i=0;i<8;i++)
    {
        InitialTemp=HA_PM_B1;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_B++;
        }
        InitialTemp=HA_PM_B2;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_B++;
        }
    }

    for(i=0;i<8;i++)
    {
        InitialTemp=HA_PM_C1;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_C++;
        }
        InitialTemp=HA_PM_C2;
        if(InitialTemp&(0x0001<<i))
        {
            NHA_module_C++;
        }
    }
    FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHA_module_A;
    FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHA_module_A;
    FPGA_Interface[Com_B1_PluginUnit_9-2].NUM=NHA_module_B;
    FPGA_Interface[Com_B2_PluginUnit_10-2].NUM=NHA_module_B;
    FPGA_Interface[Com_C1_PluginUnit_13-2].NUM=NHA_module_C;
    FPGA_Interface[Com_C2_PluginUnit_14-2].NUM=NHA_module_C;

    /*
     * ReferenceDEF is a one-module reference scale. Multiplying by NUM
     * gives the phase-level neutral/mid reference used before writing
     * Reference to both communication cards of the same phase.
     */
    InitialTemp=NHA_module_A;
    HA_ReferenceDEF_A=(float)InitialTemp;
    InitialTempFloat=ReferenceDEF;
    HA_ReferenceDEF_A=HA_ReferenceDEF_A*InitialTempFloat;

    InitialTemp=NHA_module_B;
    HA_ReferenceDEF_B=(float)InitialTemp;
    InitialTempFloat=ReferenceDEF;
    HA_ReferenceDEF_B=HA_ReferenceDEF_B*InitialTempFloat;

    InitialTemp=NHA_module_C;
    HA_ReferenceDEF_C=(float)InitialTemp;
    InitialTempFloat=ReferenceDEF;
    HA_ReferenceDEF_C=HA_ReferenceDEF_C*InitialTempFloat;

    //regulate
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[0]=HA_PM_A1;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[1]=HA_PM_A2;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET=HA_PM_B1;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET=HA_PM_B2;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET*10;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET=HA_PM_C1;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET*10;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET=HA_PM_C2;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET*10;

}

/*==================================================================
 * 函数功能: 写原TAD采样计数和DAB启动占空比/起始计数
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void FpgaInit_InitTimingDuty(void)
{
    //TAD
    FPGA_Interface[Com_A1_PluginUnit_5-2].TAD=ControlTAD;
    FPGA_Interface[Com_A2_PluginUnit_6-2].TAD=ControlTAD;
    FPGA_Interface[Com_B1_PluginUnit_9-2].TAD=ControlTAD;
    FPGA_Interface[Com_B2_PluginUnit_10-2].TAD=ControlTAD;
    FPGA_Interface[Com_C1_PluginUnit_13-2].TAD=ControlTAD;
    FPGA_Interface[Com_C2_PluginUnit_14-2].TAD=ControlTAD;


//  HA_DAB_T=5000;
//  FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_T=HA_DAB_T;
//  FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_T=HA_DAB_T;
//  FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_T=HA_DAB_T;
//  FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_T=HA_DAB_T;
//  FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_T=HA_DAB_T;
//  FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_T=HA_DAB_T;
//
//
//  HA_DABDuty_A=2500;
//  HA_DABDuty_B=2500;
//  HA_DABDuty_C=2500;
//  FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Duty=HA_DABDuty_A;
//  FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Duty=HA_DABDuty_A;
//  FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Duty=HA_DABDuty_B;
//  FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Duty=HA_DABDuty_B;
//  FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Duty=HA_DABDuty_C;
//  FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Duty=HA_DABDuty_C;

/*
        FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_LEFTDuty=2500;
        FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_LEFTDuty=2500;
        FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_RIGHTDuty=2500;
        FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_RIGHTDuty=2500;

        FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=1250;
        FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=1250;

        FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_LEFTDuty=2500;
        FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_LEFTDuty=2500;
        FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_RIGHTDuty=2500;
        FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_RIGHTDuty=2500;
        FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Start=1250;
        FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Start=1250;

        FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_LEFTDuty=250;
        FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_LEFTDuty=250;
        FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_RIGHTDuty=250;
        FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_RIGHTDuty=250;
        FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Start=1250;
        FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Start=1250;
*/
        FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_LEFTDuty=350;
        FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_LEFTDuty=350;
        FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_RIGHTDuty=350;
        FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_RIGHTDuty=350;
        FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=1250;
        FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=1250;

        FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_LEFTDuty=350;
        FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_LEFTDuty=350;
        FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_RIGHTDuty=350;
        FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_RIGHTDuty=350;
        FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Start=1250;
        FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Start=1250;


        FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_LEFTDuty=350;
        FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_LEFTDuty=350;
        FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_RIGHTDuty=350;
        FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_RIGHTDuty=350;
        FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Start=1250;
        FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Start=1250;


}

/*==================================================================
 * 函数功能: 回写模块控制字、GPIO输出和DSP总封锁控制
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void FpgaInit_InitOutputControls(void)
{
    //module control
    Module_PM_CTL.PM_CTL16=0x0F00;
    /*
    HW_RST   = 1   清除模块控制板故障锁存
    FAN_OFF  = 1   关闭模块风扇
    SVG_SHUT = 1   闭锁级联 H 桥
    DAB_SHUT = 1   闭锁 DAB H 桥
    */
    Module_Control_Write();
    Module_HW_RST(0);//避免一直保持清锁存命令

    //IO OUTPUT
    Port_DOUT.SwitchINOUT16=0;
    Port_IO_Out();

    //DSP Control
    Port_DSPControl_FBRST(0);
    Port_DSPControl_AllShutdown(1);


}

/*==================================================================
 * 函数功能: 清除FPGA谐波相位表并设置原风扇和端口在线状态
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void FpgaInit_InitRecorderFanStatus(void)
{
    uchar i; /* i：本分域初始化的循环索引，每个循环均从0开始。 */
    for(i=0;i<8;i++)
    {
        FPGA_Interface[Com_A1_PluginUnit_5-2].HD_S_PH[i]=0;
    }

    for(i=0;i<8;i++)
    {
        FPGA_Interface[Com_A2_PluginUnit_6-2].HD_S_PH[i]=0;
    }

//  Port_RealStatus.Bbits.S_Mode=2;   //手动

    Module_FAN_OFF(0);

    Port_OnState=0x000B;
    /*
    bit0 = 0x0001  LVAC
    bit1 = 0x0002  LVDC
    bit2 = 0x0004  HVDC
    bit3 = 0x0008  HVAC
    */
}

#endif /* HVAC_online */

