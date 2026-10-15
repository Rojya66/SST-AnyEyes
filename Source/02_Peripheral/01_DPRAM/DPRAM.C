#include "FpgaIo.h"
#include "SstPortStatus.h"
#include "PwmGate.h"
#include "ControlData.h"
#include "DpramConfig.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "SstTypes.h"
//###########################################################################
//
// FILE:   DPRAM.c
//
// FUNCTION:  1)remote control
//            2)remote regulating
//            3)remote measurement
//            4)remote signalling
//            5)Write DPRAM
//            6)Read DPRAM
// AUTHOR: Wen Wusong;  Modified by Z.W
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################
#include "DPRAM.h"
#include "X1226_RW.h"
#include "FaultCheck.h"
#include "hw_types.h"
/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 按 16 位字复制指定长度的内存数据
 * 输入参数: SourceAddr：源地址，连续数据至少覆盖要求的复制长度
 *           length：待复制的 16 位字数，源/目标容量均须满足
 *           DestAddr：输出目标地址，调用者须提供足够容量
 * 输出参数: DestAddr 指向的 length 个 16 位字；源、目标均按 volatile 访问。
 * 返回值: 无
 * 执行说明: 逐字复制，无边界或重叠处理；调用者保证地址、容量及访问时序。
 *================================================================*/
void MemDataCopy(volatile ushort *SourceAddr, ushort length, volatile ushort *DestAddr);
/*==================================================================
 * 函数功能: 把指定遥测、插件或遥调缓存回写到 DPRAM 并通知对端
 * 输入参数: Flag_R：WriteBlock_* DPRAM 回写块号
 * 输出参数: 指定 DPRAM 数据块；有效块写完后将块号写入右邮箱通知主机。
 * 返回值: 无
 * 执行说明: 主循环调用；每次复制一个固定块，无效块号不产生邮箱通知。
 *================================================================*/
void DPRAM_Write(ushort Flag_R);
/*==================================================================
 * 函数功能: 读取 DPRAM 新块标识以及对应遥控或遥调块
 * 输入参数: 无
 * 输出参数: FLAG_NEW_L；块 1/2 分别复制到 PORT_Control_DATA_Buf / PORT_Regulate_DATA_Buf。
 * 返回值: 无
 * 执行说明: 先读左邮箱块号，再复制受支持的固定块；维护扩展请求由上层单独处理。
 *================================================================*/
void DPRAM_Read(void);
/*==================================================================
 * 函数功能: 分派 DPRAM 缓存中的遥控、遥调数据处理
 * 输入参数: 无
 * 输出参数: 按 FLAG_NEW_L 更新旧控制状态或遥调参数。
 * 返回值: 无
 * 执行说明: 旧版兼容分派入口；当前控制主流程由 SstDpramHost 接入 SstCommand。
 *================================================================*/
void DPRAM_ReadDataDispose(void);
/*==================================================================
 * 函数功能: 按旧版遥控块处理启动、停止、脉冲、开关及复位控制
 * 输入参数: 无
 * 输出参数: 旧端口启停、开关及 PWM 标志；符合条件时操作 FPGA 和故障复位。
 * 返回值: 无
 * 执行说明: 保留的旧控制入口；当前迁移主流程使用 SstCommand_ApplyDpram 提交受控命令。
 *================================================================*/
void RemoteControl_Dispose(void);
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y);
/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
void RemoteRegulate_Dispose_2(uchar Inital_Y);
/*==================================================================
 * 函数功能: 把端口浮点遥测及状态换算为 DPRAM 16 位上报缓存
 * 输入参数: 无
 * 输出参数: PORT_RealTime_DATA_buf 中的 16 位状态及量测；由 DPRAM_Write 复制到块 3。
 * 返回值: 无
 * 执行说明: 按端口编译分支打包；不在本函数内通知主机。
 *================================================================*/
void RemoteSig_Dispose(void);

/****************************************************************************/
/*                                                                          */
/*                    Data copy                                             */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按 16 位字复制指定长度的内存数据
 * 输入参数: SourceAddr：源地址，连续数据至少覆盖要求的复制长度
 *           length：待复制的 16 位字数，源/目标容量均须满足
 *           DestAddr：输出目标地址，调用者须提供足够容量
 * 输出参数: DestAddr 指向的 length 个 16 位字；源、目标均按 volatile 访问。
 * 返回值: 无
 * 执行说明: 逐字复制，无边界或重叠处理；调用者保证地址、容量及访问时序。
 *================================================================*/
void MemDataCopy(volatile ushort *SourceAddr, ushort length, volatile ushort *DestAddr)
{
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    for (i = 0; i < length; i++)
    {
        *DestAddr++ = *SourceAddr++;
    }
}

/*==================================================================
 * 函数功能: 将指定长度的普通内存数据复制到 volatile 外设区
 * 输入参数: SourceAddr：源地址，连续数据至少覆盖要求的复制长度
 *           length：待复制的 16 位字数，源/目标容量均须满足
 *           DestAddr：输出目标地址，调用者须提供足够容量
 * 输出参数: DestAddr 指向的连续 length 个 16 位字，调用者保证容量及有效地址。
 * 返回值: 无
 * 执行说明: 逐字复制，不处理地址重叠；调用者保证源/目标容量及外设访问条件。
 *================================================================*/
void MemDataCopy_1(ushort *SourceAddr, ushort length, volatile ushort *DestAddr)
{
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    for (i = 0; i < length; i++)
    {
        *DestAddr++ = *SourceAddr++;
    }
}

/*==================================================================
 * 函数功能: 从 volatile 外设区复制指定长度数据到普通内存
 * 输入参数: SourceAddr：源地址，连续数据至少覆盖要求的复制长度
 *           length：待复制的 16 位字数，源/目标容量均须满足
 *           DestAddr：输出目标地址，调用者须提供足够容量
 * 输出参数: DestAddr 指向的连续 length 个 16 位字，调用者保证容量及有效地址。
 * 返回值: 无
 * 执行说明: 逐字复制，不处理地址重叠；调用者保证源/目标容量及外设访问条件。
 *================================================================*/
void MemDataCopy_2(volatile ushort *SourceAddr, ushort length, ushort *DestAddr)
{
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    for (i = 0; i < length; i++)
    {
        *DestAddr++ = *SourceAddr++;
    }
}

/****************************************************************************/
/*                                                                          */
/*                    Write DPRAM                                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把指定遥测、插件或遥调缓存回写到 DPRAM 并通知对端
 * 输入参数: Flag_R：WriteBlock_* DPRAM 回写块号
 * 输出参数: 指定 DPRAM 数据块；有效块写完后将块号写入右邮箱通知主机。
 * 返回值: 无
 * 执行说明: 主循环调用；每次复制一个固定块，无效块号不产生邮箱通知。
 *================================================================*/
void DPRAM_Write(ushort Flag_R)
{
    uchar tt; /* tt：DPRAM 写入通知的原临时控制值。 */

    tt = 0;
    switch (Flag_R)
    {
        case WriteBlock_remMeaSig:
        {
            tt = 1;
            RemoteSig_Dispose();
            MemDataCopy_1(&PORT_RealTime_DATA_buf.DPRAM_Block3Buf[0],
                          DPRAMBlock3_Length,
                          &DPRAM_Block3_remMeaSig.DPRAM_Block3Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_2:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[2]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit2.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000500), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit2.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_3:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[0]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit3.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit3.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_4:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[1]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit4.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000300), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit4.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_5:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[3]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit5.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000700), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit5.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_6:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[4]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit6.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000900), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit6.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_7:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[5]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit7.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000B00), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit7.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_8:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[6]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit8.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000D00), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit8.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_9:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[7]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit9.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60000F00), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit9.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_10:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[8]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit10.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60001100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit10.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_11:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[9]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit11.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60001300), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit11.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_12:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[10]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit12.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60001500), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit12.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_13:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[11]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit13.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60001700), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit13.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_14:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[12]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit14.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60001900), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit14.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_15:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[13]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit15.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60001B00), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit15.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_16:
        {
            tt = 1;
            //			MemDataCopy((ushort *)(&FPGA_Interface[14]+0x100), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit16.DPRAM_Block4Buf[0]);
            MemDataCopy(
                (ushort *)(0x60001D00), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit16.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_PluginUnit_17:
        {
            tt = 1;
            //			MemDataCopy(&FPGA_PU_16.DPRAM_Block4Buf[0], DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit17.DPRAM_Block4Buf[0]);
            //			MemDataCopy((ushort *)(0x60001F00), DPRAMBlock4_PluginUnit_Length, &DPRAM_Block4_PluginUnit17.DPRAM_Block4Buf[0]);
        }
        break;

        case WriteBlock_remRegulate:
        {
            if (FLAG_RemRegulate_SetOver == 1)
            {
                tt = 1;

#ifdef EEPROM_ON
                /* 旧Block2回读使用当前RAM，避免EEPROM旧PI覆盖本次遥调缓存。 */
#endif
                MemDataCopy_1(&PORT_Regulate_DATA_Buf.DPRAM_Block2Buf[0],
                              DPRAMBlock2_Length,
                              &DPRAM_Block2_remRegulate.DPRAM_Block2Buf[0]);
                FLAG_RemRegulate_SetOver = 0;
            }
        }
        break;

        default:
            break;
    }

    if (tt == 1)
    {
        HWREGH(DPRAM_Maibox_Right) = Flag_R;
    }
}

/****************************************************************************/
/*                                                                          */
/*                    Read DPRAM                                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取 DPRAM 新块标识以及对应遥控或遥调块
 * 输入参数: 无
 * 输出参数: FLAG_NEW_L；块 1/2 分别复制到 PORT_Control_DATA_Buf / PORT_Regulate_DATA_Buf。
 * 返回值: 无
 * 执行说明: 先读左邮箱块号，再复制受支持的固定块；维护扩展请求由上层单独处理。
 *================================================================*/
void DPRAM_Read(void)
{
    FLAG_NEW_L = HWREGH(DPRAM_Maibox_Left);

    switch (FLAG_NEW_L)
    {
        case ReadBlock_remControl:
        {
            MemDataCopy_2(&DPRAM_Block1_remControl.DPRAM_Block1Buf[0],
                          DPRAMBlock1_Length,
                          &PORT_Control_DATA_Buf.DPRAM_Block1Buf[0]);
        }
        break;

        case ReadBlock_remRegulate:
        {
            MemDataCopy_2(&DPRAM_Block2_remRegulate.DPRAM_Block2Buf[0],
                          DPRAMBlock2_Length,
                          &PORT_Regulate_DATA_Buf.DPRAM_Block2Buf[0]);
        }
        break;

        default:
            break;
    }
}

/****************************************************************************/
/*                                                                          */
/*                    Data Dispose                                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 分派 DPRAM 缓存中的遥控、遥调数据处理
 * 输入参数: 无
 * 输出参数: 按 FLAG_NEW_L 更新旧控制状态或遥调参数。
 * 返回值: 无
 * 执行说明: 旧版兼容分派入口；当前控制主流程由 SstDpramHost 接入 SstCommand。
 *================================================================*/
void DPRAM_ReadDataDispose(void)
{
    switch (FLAG_NEW_L)
    {
        case ReadBlock_remControl:
        {
            RemoteControl_Dispose(); //Remote Control
        }
        break;

        case ReadBlock_remRegulate:
        {
            RemoteRegulate_Dispose_2(0); //Remote Regulate, need for storage
        }
        break;

        default:
            break;
    }
}

/****************************************************************************/
/*                                                                          */
/*              Control command dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按旧版遥控块处理启动、停止、脉冲、开关及复位控制
 * 输入参数: 无
 * 输出参数: 旧端口启停、开关及 PWM 标志；符合条件时操作 FPGA 和故障复位。
 * 返回值: 无
 * 执行说明: 保留的旧控制入口；当前迁移主流程使用 SstCommand_ApplyDpram 提交受控命令。
 *================================================================*/
void RemoteControl_Dispose(void)
{
    uchar tt_flag, /* tt_flag：原遥控处理的临时状态字。 */
        i; /* i：复制字序号或当前编译端口内的模块位索引。 */

    if (Port_RunStatus == PortStatusNum_Nonvaild)
    {
        return;
    }

    if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16 != 0)) //start
    {
        if ((ControlFLAG.Bbits.Port_Stop) &&
            ((Port_RunStatus == PortStatusNum_Standby) || (Port_RunStatus == PortStatusNum_Initial)) &&
            (!PortRunFLAG.Bbits.Stop_AllPort))
        {
            if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.Bbits.HVAC_Start == 1) &&
                (PORT_Index == 1)) //&&(PortRunFLAG.Bbits.PhaseSquence_Error==0))
            {
                ControlFLAG.Bbits.Port_Stop = 0x00;
                ControlFLAG.Bbits.Port_Start = 0x01;
                Port_RunStatus = PortStatusNum_Start;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.Bbits.HVDC_Start == 1) && (PORT_Index == 2))
            {
                ControlFLAG.Bbits.Port_Stop = 0x00;
                ControlFLAG.Bbits.Port_Start = 0x01;
                Port_RunStatus = PortStatusNum_Start;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.Bbits.LVDC_Start == 1) && (PORT_Index == 3))
            {
                ControlFLAG.Bbits.Port_Stop = 0x00;
                ControlFLAG.Bbits.Port_Start = 0x01;
                Port_RunStatus = PortStatusNum_Start;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.Bbits.LVAC_Start == 1) && (PORT_Index == 4))
            {
                ControlFLAG.Bbits.Port_Stop = 0x00;
                ControlFLAG.Bbits.Port_Start = 0x01;
                Port_RunStatus = PortStatusNum_Start;
            }
            else
            {
                Port_COM_START.COM_START_PORT16 =
                    PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16;
            }
        }
        return;
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16 != 0) //stop
    {
        if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.HVAC_STOP == 1) && (PORT_Index == 1))
        {
            //			if(Port_RunStatus==PortStatusNum_Run)
            {
                ControlFLAG.Bbits.Port_Stop = 0x01;
                ControlFLAG.Bbits.Port_Start = 0x00;
                Port_RunStatus = PortStatusNum_Stop;

                ControlFLAG.Bbits.SVG_Start = 0;
                FaultDisposeFLAG.Bbits.FaultDisposeType =
                    2; //0--no fault;      1--PWM pluse disable;   2--port shutdown
                FaultDisposeFLAG.Bbits.PWMPulseState =
                    3; //0--no pulse disable； 1--50Hz H-Bridge/Module pulse disable；2--DAB H-Bridge pulse disable; 3--port pulse disable
                FaultDisposeFLAG.Bbits.Port_StopType =
                    3; //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown
            }
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.HVDC_STOP == 1) && (PORT_Index == 2))
        {
            //			if(Port_RunStatus==PortStatusNum_Run)
            {
                ControlFLAG.Bbits.Port_Stop = 0x01;
                ControlFLAG.Bbits.Port_Start = 0x00;
                Port_RunStatus = PortStatusNum_Stop;

                ControlFLAG.Bbits.SVG_Start = 0;
                FaultDisposeFLAG.Bbits.FaultDisposeType =
                    2; //0--no fault;      1--PWM pluse disable;   2--port shutdown
                FaultDisposeFLAG.Bbits.PWMPulseState =
                    3; //0--no pulse disable； 1--50Hz H-Bridge/Module pulse disable；2--DAB H-Bridge pulse disable; 3--port pulse disable
                FaultDisposeFLAG.Bbits.Port_StopType =
                    3; //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown

                if (Flag_DC_VoltSource == 1)
                {
                    FaultDisposeFLAG.Bbits.Port_StopAll = 1;
                    PortRunFLAG.Bbits.Stop_AllPort = 1; // 请求全端口停机。
                    Port_StateUpdate_Write();
                }
            }
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVDC_STOP == 1) && (PORT_Index == 3))
        {
            //			if(Port_RunStatus==PortStatusNum_Run)
            {
                ControlFLAG.Bbits.Port_Stop = 0x01;
                ControlFLAG.Bbits.Port_Start = 0x00;
                Port_RunStatus = PortStatusNum_Stop;

                ControlFLAG.Bbits.SVG_Start = 0;
                FaultDisposeFLAG.Bbits.FaultDisposeType =
                    2; //0--no fault;      1--PWM pluse disable;   2--port shutdown
                FaultDisposeFLAG.Bbits.PWMPulseState =
                    3; //0--no pulse disable； 1--50Hz H-Bridge/Module pulse disable；2--DAB H-Bridge pulse disable; 3--port pulse disable
                FaultDisposeFLAG.Bbits.Port_StopType =
                    3; //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown

                if (Flag_DC_VoltSource == 1)
                {
                    FaultDisposeFLAG.Bbits.Port_StopAll = 1;
                    PortRunFLAG.Bbits.Stop_AllPort = 1; // 请求全端口停机。
                    Port_StateUpdate_Write();
                }
            }
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.Bbits.LVAC_STOP == 1) && (PORT_Index == 4))
        {
            if (Port_RunStatus == PortStatusNum_Run)
            {
                ControlFLAG.Bbits.Port_Stop = 0x01;
                ControlFLAG.Bbits.Port_Start = 0x00;
                Port_RunStatus = PortStatusNum_Stop;

                FaultDisposeFLAG.Bbits.FaultDisposeType =
                    2; //0--no fault;      1--PWM pluse disable;   2--port shutdown
                FaultDisposeFLAG.Bbits.PWMPulseState =
                    3; //0--no pulse disable； 1--50Hz H-Bridge/Module pulse disable；2--DAB H-Bridge pulse disable; 3--port pulse disable
                FaultDisposeFLAG.Bbits.Port_StopType =
                    3; //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown
            }
        }
        else
        {
            Port_COM_STOP.COM_STOP_PORT16 = PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16;
        }
        return;
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 != 0) //mode
    {
        tt_flag = 0;
        if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x000f) == 0x0001)
        {
            Port_Main_WorkMode = 0; //v/f
            tt_flag = 1;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x000f) == 0x0002)
        {
            Port_Main_WorkMode = 1; //P/Q
            tt_flag = 1;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x000f) == 0x0004)
        {
            Port_Main_WorkMode = 2; //Droop
            tt_flag = 1;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x000f) == 0x0008)
        {
            Port_Main_WorkMode = 3; //Follow
            tt_flag = 1;
        }
        else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x000f)
        {
        }

        else
        {
            //STATCOM
            if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x1000)
            {
                if (Port_STATCOM_WorkMode == 0x00)
                {
                    tt_flag = 2;
                    Port_STATCOM_WorkMode = 0x01; //Q mode
                }
            }
            else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x2000)
            {
                if (Port_STATCOM_WorkMode == 0x01)
                {
                    tt_flag = 2;
                    Port_STATCOM_WorkMode = 0x00;
                }
            }
            else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0400)
            {
                if (Port_STATCOM_WorkMode == 0x00)
                {
                    tt_flag = 2;
                    Port_STATCOM_WorkMode = 0x02; //PF mode
                }
            }
            else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0800)
            {
                if (Port_STATCOM_WorkMode == 0x02)
                {
                    tt_flag = 2;
                    Port_STATCOM_WorkMode = 0x00;
                }
            }
            else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0100)
            {
                if (Port_STATCOM_WorkMode == 0x00)
                {
                    tt_flag = 2;
                    Port_STATCOM_WorkMode = 0x03; //V mode
                }
            }
            else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0200)
            {
                if (Port_STATCOM_WorkMode == 0x03)
                {
                    tt_flag = 2;
                    Port_STATCOM_WorkMode = 0x00;
                }
            }
            else
            {
                //APF
                if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0040)
                {
                    if (Port_APF_WorkMode == 0x00)
                    {
                        tt_flag = 3;
                        Port_APF_WorkMode = 0x02; // Treatment
                    }
                }
                else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0080)
                {
                    if (Port_APF_WorkMode == 0x02)
                    {
                        tt_flag = 3;
                        Port_APF_WorkMode = 0x00;
                    }
                }
                else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0010)
                {
                    if (Port_APF_WorkMode == 0x00)
                    {
                        tt_flag = 3;
                        Port_APF_WorkMode = 0x01; // inject
                    }
                }
                else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 & 0x0020)
                {
                    if (Port_APF_WorkMode == 0x01)
                    {
                        tt_flag = 3;
                        Port_APF_WorkMode = 0x00;
                    }
                }
                else
                {
                }
            }
        }

        if (tt_flag == 1)
        {
            //切换主工作模式后的变量预处理预留位置。
            DSP_2_EEPROM_Page0();
            return;
        }
        else if (tt_flag == 2)
        {
            //切换 STATCOM 模式后的变量预处理预留位置。
            DSP_2_EEPROM_Page0();
            return;
        }
        else if (tt_flag == 3)
        {
            //切换 APF 模式后的变量预处理预留位置。
#ifdef HVAC_online
            for (i = 0; i < 4; i++) //I_load
            {
                I_d_Load_H6_100_Filter.H_Filter_BUF[i] = 0.0;
                I_d_Load_H12_100_Filter.H_Filter_BUF[i] = 0.0;
                I_d_Load_H2_20_Filter.H_Filter_BUF[i] = 0.0;

                I_q_Load_H6_100_Filter.H_Filter_BUF[i] = 0.0;
                I_q_Load_H12_100_Filter.H_Filter_BUF[i] = 0.0;
                I_q_Load_H2_20_Filter.H_Filter_BUF[i] = 0.0;
            }
#endif
            DSP_2_EEPROM_Page0();
            return;
        }
        else
        {
        }
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_ON.COM_SWITCH16 != 0)
    {
        if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_ON.COM_SWITCH16 & 0x0001)
        {
            SW.Bbits.Flag_Switch = 1; //1: 动接触器；2：动内部断路器;3：动外部断路器1；4：动外部断路器2
            SW.Bbits.Flag_SwitchZhi = 1; //0:断；1：合
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;

            SWTable_TotalTime = SwitchTotalNum;

            return;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_ON.COM_SWITCH16 & 0x0002) &&
                 ((PORT_Index == 1) || (PORT_Index == 2)))
        {
            SW.Bbits.Flag_Switch = 2;
            SW.Bbits.Flag_SwitchZhi = 1;
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;
            SWTable_TotalTime = SwitchTotalNum;
            return;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_ON.COM_SWITCH16 & 0x0004) &&
                 (SW.Bbits.S_EXT_CB_ON_EN))
        {
            SW.Bbits.Flag_Switch = 3;
            SW.Bbits.Flag_SwitchZhi = 1;
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;
            SWTable_TotalTime = SwitchTotalNum;
            return;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_ON.COM_SWITCH16 & 0x0008) &&
                 (SW.Bbits.S_EXT_CB_ON_EN) && ((PORT_Index == 2) || (PORT_Index == 3)))
        {
            SW.Bbits.Flag_Switch = 3;
            SW.Bbits.Flag_SwitchZhi = 1;
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;
            SWTable_TotalTime = SwitchTotalNum;
            return;
        }
        else
        {
        }
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_OFF.COM_SWITCH16 != 0)
    {
        if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_OFF.COM_SWITCH16 & 0x0001)
        {
            SW.Bbits.Flag_Switch = 1; //1: 动接触器；2：动内部断路器;3：动外部断路器1；4：动外部断路器2
            SW.Bbits.Flag_SwitchZhi = 0; //0:断；1：合
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;
            SWTable_TotalTime = SwitchTotalNum;
            return;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_OFF.COM_SWITCH16 & 0x0002) &&
                 ((PORT_Index == 1) || (PORT_Index == 2)))
        {
            SW.Bbits.Flag_Switch = 2;
            SW.Bbits.Flag_SwitchZhi = 0;
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;
            SWTable_TotalTime = SwitchTotalNum;
            return;
        }
        else if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_OFF.COM_SWITCH16 & 0x0004)
        {
            SW.Bbits.Flag_Switch = 3;
            SW.Bbits.Flag_SwitchZhi = 0;
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;
            SWTable_TotalTime = SwitchTotalNum;
            return;
        }
        else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_OFF.COM_SWITCH16 & 0x0008) &&
                 ((PORT_Index == 2) || (PORT_Index == 3)))
        {
            SW.Bbits.Flag_Switch = 3;
            SW.Bbits.Flag_SwitchZhi = 0;
            SW.Bbits.Flag_SwitchRun = 0;
            SW.Bbits.Flag_SwitchSuccess = 0;
            SW.Bbits.Flag_SwitchTable = 3;
            SWTable_TotalTime = SwitchTotalNum;
            return;
        }
        else
        {
        }
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 != 0)
    {
        if ((PORT_Index == 1) || (PORT_Index == 4))
        {
            FaultLatch_Clear(0, 1);
            if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 & 0x0003) == 0x01)
            {
                flag_DABOPEN = 1;
                //			PWMPluse_Enable(2);  //Enable DAB H-bridge
                return;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 & 0x0003) == 0x02)
            {
                flag_SVGOPEN = 1;

                //				if(PORT_Index==1)
                //				{
                //					Udc_Ref=SVG_Udc_Ave+20.0;
                //				}
                //				PWMPluse_Enable(1);  //Enable 50Hz H-bridge
                return;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 & 0x0003) == 0x03)
            {
                PWMPluse_Enable(3); //Enable Port H-bridge
                return;
            }
            else
            {
            }
        }
        else if ((PORT_Index == 2) || (PORT_Index == 3))
        {
            if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 & 0x0003) == 0x01)
            {
                Port_DINRead(0);
                //			if(SW.Bbits.S_CONTACTOR==1)
                {
                    FaultLatch_Clear(0, 1);
                    flag_DABOPEN = 1;
                }

                //				PWMPluse_Enable(3);  //Enable DAB H-bridge
                return;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 & 0x0003) == 0x02)
            {
                //				PWMPluse_Enable(3);  //Enable Port H-bridge
                Port_DINRead(0);
                //				if(SW.Bbits.S_CONTACTOR==1)
                {
                    FaultLatch_Clear(0, 1);
                    flag_SVGOPEN = 1;
                    //				Variable_RST();
                }
                return;
            }
            else
            {
            }
        }
        else
        {
        }
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 != 0)
    {
        if ((PORT_Index == 1) || (PORT_Index == 4))
        {
            if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 & 0x0003) == 0x01)
            {
                FaultDisposeFLAG.Bbits.PWMPulseState = 0x02; //Dab H-bridge PWM disable
                PWMPluse_Disable(0);
                return;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 & 0x0003) == 0x02)
            {
                FaultDisposeFLAG.Bbits.PWMPulseState = 0x01; //50Hz H-bridge PWM disable
                PWMPluse_Disable(0);
                return;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 & 0x0003) == 0x03)
            {
                FaultDisposeFLAG.Bbits.PWMPulseState = 0x03; //Port H-bridge PWM disable
                PWMPluse_Disable(0);
                return;
            }
            else
            {
            }
        }
        else if ((PORT_Index == 2) || (PORT_Index == 3))
        {
            if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 & 0x0003) == 0x01)
            {
                //				FaultDisposeFLAG.Bbits.PWMPulseState=0x03;//Port H-bridge PWM disable
                //				PWMPluse_Disable(0);

                FaultDisposeFLAG.Bbits.PWMPulseState = 0x02;
                PWMPluse_Disable(0);

                if (Flag_DC_VoltSource == 1)
                {
                    FaultDisposeFLAG.Bbits.Port_StopAll = 1;
                    PortRunFLAG.Bbits.Stop_AllPort = 1; // 请求全端口停机。
                    Port_StateUpdate_Write();
                }

                return;
            }
            else if ((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 & 0x0003) == 0x03)
            {
                FaultDisposeFLAG.Bbits.PWMPulseState = 0x03; //Port H-bridge PWM disable
                PWMPluse_Disable(0);
                return;
            }
            else
            {
            }
        }
        else
        {
        }
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.COM_FAULT_CLEAR.COM_FAULT_CLEAR16 & 0x0001) //Fault Clear
    {
        FaultLatch_Clear(0, 1); //clear latch & fault flag
        return;
    }

    if (PORT_Control_DATA_Buf.DPRAM_RemControl.PORT_SWITCH_EN.PORT_SWITCH_EN16 != 0)
    {
        if (PORT_Control_DATA_Buf.DPRAM_RemControl.PORT_SWITCH_EN.PORT_SWITCH_EN16 & 0x0001)
        {
            if (SW.Bbits.S_EXT_CB_ON_EN == 0)
            {
                SW.Bbits.S_EXT_CB_ON_EN = 1;
                DSP_2_EEPROM_Page0();
                return;
            }
        }
        else if (PORT_Control_DATA_Buf.DPRAM_RemControl.PORT_SWITCH_EN.PORT_SWITCH_EN16 & 0x0002)
        {
            if (SW.Bbits.S_EXT_CB_ON_EN == 1)
            {
                SW.Bbits.S_EXT_CB_ON_EN = 0;
                DSP_2_EEPROM_Page0();
                return;
            }
        }
        else
        {
        }
    }
}

#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp, /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
        InitialTempFloat; /* InitialTempFloat：原控制系数和给定初始化的浮点计算缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_2_EN = PORT_Regulate_DATA.FO_2_EN.FO_EN16;
    Port_FO_3_EN = PORT_Regulate_DATA.FO_3_EN.FO_EN16;
    Port_FO_EN_Set(3, 0, 0, 0, 0);

    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET; //1kw=10
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0; //w

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET;
    PORT_Regulate_DATA.PORT_Q_SET = ParameterTemp * 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET;
    PORT_Regulate_DATA.PORT_F_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_5TH_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_7TH_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_11TH_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_5TH_SET = ParameterTemp / 1000.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_7TH_SET = ParameterTemp / 1000.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_11TH_SET = ParameterTemp / 1000.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET;
    PORT_Regulate_DATA.STATCOM_V_SET = ParameterTemp;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET;
    PORT_Regulate_DATA.STATCOM_PF_SET = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET; //1kw=10
    PORT_Regulate_DATA.STATCOM_Q_SET = ParameterTemp * 100.0; //w

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp;

    /**********************下垂频率截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET;
    PORT_Regulate_DATA.PORT_F0_SET = ParameterTemp / 10.0;

    /**********************频率下垂斜率**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET;
    PORT_Regulate_DATA.PORT_KF_SET = ParameterTemp / 10.0;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    Har_5_RMS = PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
    Har_5_PH = PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
    Har_7_RMS = PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
    Har_7_PH = PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
    Har_11_RMS = PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
    Har_11_PH = PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET / 200;

    /**************************电压参考**********************/
    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 600.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else
    {
        HA_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    UstOK_HVAC_Port = HA_Module_RefUdc;
    Udc_Ref = HA_Module_RefUdc;

    if (NHA_module_A == 0)
    {
        Ud_st_Max = 10.0;
        Ud_st_Min = -10.0;
        Uq_st_Max = 10.0;
        Uq_st_Min = -10.0;
    }
    else
    {
        InitialTempFloat = (float)NHA_module_A;
        InitialTempFloat = InitialTempFloat * 1.1;
        InitialTempFloat = InitialTempFloat * HA_Module_RefUdc;

        Ud_st_Max = InitialTempFloat;
        Ud_st_Min = InitialTempFloat * (-1.0);
        Uq_st_Max = Ud_st_Max;
        Uq_st_Min = Ud_st_Min;
    }

    Uout_d_Positive_Max = Ud_st_Max;
    Uout_d_Positive_Min = Ud_st_Min;
    Uout_q_Positive_Max = Uq_st_Max;
    Uout_q_Positive_Min = Uq_st_Min;

    /**************************电流范围**********************/

    if (PORT_Regulate_DATA.Reserved_1[3] > 95.0)
    {
        PORT_Regulate_DATA.Reserved_1[3] = 95.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3] = 95;
        Idst_Max = 95.0;
    }
    else if (PORT_Regulate_DATA.Reserved_1[3] < 10.0)
    {
        PORT_Regulate_DATA.Reserved_1[3] = 10.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3] = 10;
        Idst_Max = 10.0;
    }
    else
    {
        Idst_Max = PORT_Regulate_DATA.Reserved_1[3];
    }

    Idst_Min = (-1.0) * Idst_Max;
    Iqst_Max = Idst_Max;
    Iqst_Min = Idst_Min;

    Id_Positive_Max = Idst_Max;
    Id_Positive_Min = Idst_Min;
    Iq_Positive_Max = Iqst_Max;
    Iq_Positive_Min = Iqst_Min;
    /**************************光纤插件模块配置**********************/
    HA_PM_A1 = (ushort)PORT_Regulate_DATA.Reserved_1[0];
    HA_PM_A2 = (ushort)PORT_Regulate_DATA.Reserved_1[1];
    HA_PM_B1 = (ushort)PORT_Regulate_DATA.PORT_V0_SET;
    HA_PM_B2 = (ushort)PORT_Regulate_DATA.PORT_F0_SET;
    HA_PM_C1 = (ushort)PORT_Regulate_DATA.PORT_KF_SET;
    HA_PM_C2 = (ushort)PORT_Regulate_DATA.PORT_KV_SET;

    NHA_module_A = 0;
    NHA_module_B = 0;
    NHA_module_C = 0;

    for (i = 0; i < 8; i++)
    {
        InitialTemp = HA_PM_A1;
        if (InitialTemp & (0x0001 << i))
        {
            NHA_module_A++;
        }
        InitialTemp = HA_PM_A2;
        if (InitialTemp & (0x0001 << i))
        {
            NHA_module_A++;
        }
    }

    for (i = 0; i < 8; i++)
    {
        InitialTemp = HA_PM_B1;
        if (InitialTemp & (0x0001 << i))
        {
            NHA_module_B++;
        }
        InitialTemp = HA_PM_B2;
        if (InitialTemp & (0x0001 << i))
        {
            NHA_module_B++;
        }
    }

    for (i = 0; i < 8; i++)
    {
        InitialTemp = HA_PM_C1;
        if (InitialTemp & (0x0001 << i))
        {
            NHA_module_C++;
        }
        InitialTemp = HA_PM_C2;
        if (InitialTemp & (0x0001 << i))
        {
            NHA_module_C++;
        }
    }

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM = HA_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM = HA_PM_A2;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].PM = HA_PM_B1;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].PM = HA_PM_B2;
    FPGA_Interface[Com_C1_PluginUnit_13 - 2].PM = HA_PM_C1;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].PM = HA_PM_C2;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].NUM = NHA_module_A;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].NUM = NHA_module_A;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].NUM = NHA_module_B;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].NUM = NHA_module_B;
    FPGA_Interface[Com_C1_PluginUnit_13 - 2].NUM = NHA_module_C;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].NUM = NHA_module_C;

    InitialTemp = NHA_module_A;
    HA_ReferenceDEF_A = (float)InitialTemp;
    InitialTempFloat = ReferenceDEF;
    HA_ReferenceDEF_A = HA_ReferenceDEF_A * InitialTempFloat;

    InitialTemp = NHA_module_B;
    HA_ReferenceDEF_B = (float)InitialTemp;
    InitialTempFloat = ReferenceDEF;
    HA_ReferenceDEF_B = HA_ReferenceDEF_B * InitialTempFloat;

    InitialTemp = NHA_module_C;
    HA_ReferenceDEF_C = (float)InitialTemp;
    InitialTempFloat = ReferenceDEF;
    HA_ReferenceDEF_C = HA_ReferenceDEF_C * InitialTempFloat;

    Port_OnState = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2];
    HWREGH(0x6000062e) = Port_OnState;

    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;


    PLL_Factor=PORT_Regulate_DATA.PORT_V_P_1;
    if(PLL_Factor<0.01)
    {
        PLL_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
    }

    Volt_Factor=PORT_Regulate_DATA.PORT_V_I_1;
    if(Volt_Factor<0.01)
    {
        Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    }

    Current_Factor=PORT_Regulate_DATA.PORT_I_P_1;
    if(Current_Factor<0.01)
    {
        Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    }

    Neg_Current_Factor=PORT_Regulate_DATA.PORT_I_I_1;
    if(Neg_Current_Factor<0.01)
    {
        Neg_Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
    }

    PH_Volt_Factor=PORT_Regulate_DATA.PORT_V_P_2;
    if(PH_Volt_Factor<0.01)
    {
        PH_Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
    }

    Har_2_Factor=PORT_Regulate_DATA.PORT_V_I_2;
    if(Har_2_Factor<0.01)
    {
        Har_2_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
    }

    Har_6_Factor=PORT_Regulate_DATA.PORT_I_P_2;
    if(Har_6_Factor<0.01)
    {
        Har_6_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
    }

    Har_12_Factor=PORT_Regulate_DATA.PORT_I_I_2;
    if(Har_12_Factor<0.01)
    {
        Har_12_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
    }

//controller
    PLL_PID_Num_1=PLL_PID_Num_1_def/PLL_Factor;
    PLL_PID_Num_0=PLL_PID_Num_0_def/PLL_Factor;

    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
    Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;

    PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def/PH_Volt_Factor;
    PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def/PH_Volt_Factor;

    //Current loop
    Har_2_Controller_Num_2=Har_2_Controller_Num_2_def/Har_2_Factor;
    Har_2_Controller_Num_1=Har_2_Controller_Num_1_def/Har_2_Factor;
    Har_2_Controller_Num_0=Har_2_Controller_Num_0_def/Har_2_Factor;


    Har_6_Controller_Num_2=Har_6_Controller_Num_2_def/Har_6_Factor;
    Har_6_Controller_Num_1=Har_6_Controller_Num_1_def/Har_6_Factor;
    Har_6_Controller_Num_0=Har_6_Controller_Num_0_def/Har_6_Factor;


    Har_12_Controller_Num_2=Har_12_Controller_Num_2_def/Har_12_Factor;
    Har_12_Controller_Num_1=Har_12_Controller_Num_1_def/Har_12_Factor;
    Har_12_Controller_Num_0=Har_12_Controller_Num_0_def/Har_12_Factor;


    Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
    Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;

    Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def/Neg_Current_Factor;
    Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def/Neg_Current_Factor;
*/

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}

/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
void RemoteRegulate_Dispose_2(uchar Inital_Y)
{
    float ParameterTemp, /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
        InitialTempFloat; /* InitialTempFloat：原控制系数和给定初始化的浮点计算缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    /*
    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HA_CtlMode;

    if((PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=0;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=1;
    }
    else if((PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=2;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=3;
    }
    else {}

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16  =PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;


    Port_AD_PORT_EN=PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN=PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_2_EN=PORT_Regulate_DATA.FO_2_EN.FO_EN16;
    Port_FO_3_EN=PORT_Regulate_DATA.FO_3_EN.FO_EN16;
    Port_FO_EN_Set(3,0,0,0,0);

    Port_FB_EN=PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();
*/
    /**********************端口参数设置**********************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;  //1kw=10
    PORT_Regulate_DATA.PORT_P_SET=ParameterTemp*100.0;                         //w

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET;
    PORT_Regulate_DATA.PORT_Q_SET=ParameterTemp*100.0;
*/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET;
    PORT_Regulate_DATA.PORT_F_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET;
    PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=ParameterTemp/10.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=ParameterTemp/1000.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=ParameterTemp/1000.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET;
    PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=ParameterTemp/1000.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET;
    PORT_Regulate_DATA.STATCOM_V_SET=ParameterTemp;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET;
    PORT_Regulate_DATA.STATCOM_PF_SET=ParameterTemp/100.0;
*/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET; //1kw=10
    PORT_Regulate_DATA.STATCOM_Q_SET = ParameterTemp * 100.0; //w

    /**********************下垂电压截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    //	PORT_Regulate_DATA.PORT_V0_SET=ParameterTemp;

    /**********************下垂频率截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET;
    //	PORT_Regulate_DATA.PORT_F0_SET=ParameterTemp/10.0;

    /**********************频率下垂斜率**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET;
    //	PORT_Regulate_DATA.PORT_KF_SET=ParameterTemp/10.0;

    /************************电压下垂斜率********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    //	PORT_Regulate_DATA.PORT_KV_SET=ParameterTemp/10.0;

    /**************************保留数据**********************/
    /*
    for (i=0;i<4;i++) {
        ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i]=ParameterTemp;
    }
*/
    /*
    Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
    Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
    Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
    Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
    Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
    Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
*/

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET / 200;

    /**************************电压参考**********************/

    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 600.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HA_Module_RefUdc = 700.0;
    }
    else
    {
        HA_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    UstOK_HVAC_Port = HA_Module_RefUdc;
    Udc_Ref = HA_Module_RefUdc;

    if (NHA_module_A == 0)
    {
        Ud_st_Max = 10.0;
        Ud_st_Min = -10.0;
        Uq_st_Max = 10.0;
        Uq_st_Min = -10.0;
    }
    else
    {
        InitialTempFloat = (float)NHA_module_A;
        InitialTempFloat = InitialTempFloat * 1.1;
        InitialTempFloat = InitialTempFloat * HA_Module_RefUdc;

        Ud_st_Max = InitialTempFloat;
        Ud_st_Min = InitialTempFloat * (-1.0);
        Uq_st_Max = Ud_st_Max;
        Uq_st_Min = Ud_st_Min;
    }

    Uout_d_Positive_Max = Ud_st_Max;
    Uout_d_Positive_Min = Ud_st_Min;
    Uout_q_Positive_Max = Uq_st_Max;
    Uout_q_Positive_Min = Uq_st_Min;

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    /**************************电流范围**********************/
    /*
    if(PORT_Regulate_DATA.Reserved_1[3]>95.0)
    {
        PORT_Regulate_DATA.Reserved_1[3]=95.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=95;
        Idst_Max=95.0;
    }
    else if(PORT_Regulate_DATA.Reserved_1[3]<10.0)
    {
        PORT_Regulate_DATA.Reserved_1[3]=10.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=10;
        Idst_Max=10.0;
    }
    else
    {
        Idst_Max=PORT_Regulate_DATA.Reserved_1[3];
    }

    Idst_Min=(-1.0)*Idst_Max;
    Iqst_Max=Idst_Max;
    Iqst_Min=Idst_Min;

    Id_Positive_Max=Idst_Max;
    Id_Positive_Min=Idst_Min;
    Iq_Positive_Max=Iqst_Max;
    Iq_Positive_Min=Iqst_Min;
*/
    /**************************光纤插件模块配置**********************/
    /*
    HA_PM_A1=(ushort)PORT_Regulate_DATA.Reserved_1[0];
    HA_PM_A2=(ushort)PORT_Regulate_DATA.Reserved_1[1];
    HA_PM_B1=(ushort)PORT_Regulate_DATA.PORT_V0_SET;
    HA_PM_B2=(ushort)PORT_Regulate_DATA.PORT_F0_SET;
    HA_PM_C1=(ushort)PORT_Regulate_DATA.PORT_KF_SET;
    HA_PM_C2=(ushort)PORT_Regulate_DATA.PORT_KV_SET;

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

    FPGA_Interface[Com_A1_PluginUnit_5-2].PM=HA_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6-2].PM=HA_PM_A2;
    FPGA_Interface[Com_B1_PluginUnit_9-2].PM=HA_PM_B1;
    FPGA_Interface[Com_B2_PluginUnit_10-2].PM=HA_PM_B2;
    FPGA_Interface[Com_C1_PluginUnit_13-2].PM=HA_PM_C1;
    FPGA_Interface[Com_C2_PluginUnit_14-2].PM=HA_PM_C2;

    FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHA_module_A;
    FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHA_module_A;
    FPGA_Interface[Com_B1_PluginUnit_9-2].NUM=NHA_module_B;
    FPGA_Interface[Com_B2_PluginUnit_10-2].NUM=NHA_module_B;
    FPGA_Interface[Com_C1_PluginUnit_13-2].NUM=NHA_module_C;
    FPGA_Interface[Com_C2_PluginUnit_14-2].NUM=NHA_module_C;

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
*/
    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;


    PLL_Factor=PORT_Regulate_DATA.PORT_V_P_1;
    if(PLL_Factor<0.01)
    {
        PLL_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
    }

    Volt_Factor=PORT_Regulate_DATA.PORT_V_I_1;
    if(Volt_Factor<0.01)
    {
        Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    }

    Current_Factor=PORT_Regulate_DATA.PORT_I_P_1;
    if(Current_Factor<0.01)
    {
        Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    }

    Neg_Current_Factor=PORT_Regulate_DATA.PORT_I_I_1;
    if(Neg_Current_Factor<0.01)
    {
        Neg_Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
    }

    PH_Volt_Factor=PORT_Regulate_DATA.PORT_V_P_2;
    if(PH_Volt_Factor<0.01)
    {
        PH_Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
    }

    Har_2_Factor=PORT_Regulate_DATA.PORT_V_I_2;
    if(Har_2_Factor<0.01)
    {
        Har_2_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
    }

    Har_6_Factor=PORT_Regulate_DATA.PORT_I_P_2;
    if(Har_6_Factor<0.01)
    {
        Har_6_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
    }

    Har_12_Factor=PORT_Regulate_DATA.PORT_I_I_2;
    if(Har_12_Factor<0.01)
    {
        Har_12_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
    }

//controller
    PLL_PID_Num_1=PLL_PID_Num_1_def/PLL_Factor;
    PLL_PID_Num_0=PLL_PID_Num_0_def/PLL_Factor;

    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
    Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;

    PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def/PH_Volt_Factor;
    PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def/PH_Volt_Factor;

    //Current loop
    Har_2_Controller_Num_2=Har_2_Controller_Num_2_def/Har_2_Factor;
    Har_2_Controller_Num_1=Har_2_Controller_Num_1_def/Har_2_Factor;
    Har_2_Controller_Num_0=Har_2_Controller_Num_0_def/Har_2_Factor;


    Har_6_Controller_Num_2=Har_6_Controller_Num_2_def/Har_6_Factor;
    Har_6_Controller_Num_1=Har_6_Controller_Num_1_def/Har_6_Factor;
    Har_6_Controller_Num_0=Har_6_Controller_Num_0_def/Har_6_Factor;


    Har_12_Controller_Num_2=Har_12_Controller_Num_2_def/Har_12_Factor;
    Har_12_Controller_Num_1=Har_12_Controller_Num_1_def/Har_12_Factor;
    Har_12_Controller_Num_0=Har_12_Controller_Num_0_def/Har_12_Factor;


    Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
    Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;

    Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def/Neg_Current_Factor;
    Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def/Neg_Current_Factor;
*/

    /************************EER2的数据************************/
    /*
    for (i=0;i<4;i++) {
        PORT_Regulate_DATA.EER2_DATA[i]=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }
*/

    Flag_PC = 1;

    if (Inital_Y == 0)
    {
        /* 并口兼容遥调仅更新RAM/旧FPGA镜像，不进行阻塞EEPROM写。
         * 新参数需通过邮箱21显式SAVE，后台每10ms推进一字节。 */
        FLAG_RemRegulate_SetOver = 1;
    }
    Flag_PC = 0;
}
#endif

#ifdef HVDC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0, 0, 0, 0, 0);

    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET; //1kw=10
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0; //w

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;

    //-------------------------------------------------------------------
    //	#ifdef HD_2_LD
    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else
    {
        HD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }
    HD_RefUdc = HD_Module_RefUdc;
    /*
    #else
        ParameterTemp=(float)NHD_module;
        ParameterTemp=ParameterTemp*740.0;

        ParameterTemp2=(float)NHD_module;
        ParameterTemp2=ParameterTemp2*600.0;

        if(PORT_Regulate_DATA.PORT_V_SET>ParameterTemp)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp;
            HD_RefUdc=ParameterTemp;
        }
        else if(PORT_Regulate_DATA.PORT_V_SET<ParameterTemp2)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp2;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp2;
            HD_RefUdc=ParameterTemp2;
        }
        else
        {
            HD_RefUdc=PORT_Regulate_DATA.PORT_V_SET;
        }

        ParameterTemp=(float)NHD_module;
        HD_Module_RefUdc=HD_RefUdc/ParameterTemp;
    #endif
*/
    //-------------------------------------------------------------------

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    UstOK_HVDC_Port = HD_Module_RefUdc;
    Udc_Ref = HD_Module_RefUdc;

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    /**************************光纤插件模块配置**********************/
    HD_PM_A1 = (ushort)PORT_Regulate_DATA.Reserved_1[0];
    HD_PM_A2 = (ushort)PORT_Regulate_DATA.Reserved_1[1];

    NHD_module = 0;

    for (i = 0; i < 8; i++)
    {
        InitialTemp = HD_PM_A1;
        if (InitialTemp & (0x0001 << i))
        {
            NHD_module++;
        }
        InitialTemp = HD_PM_A2;
        if (InitialTemp & (0x0001 << i))
        {
            NHD_module++;
        }
    }

    if (HD_2_LD == 1)
    {
        DC_CurrentST_1 = (float)NHD_module;
        DC_CurrentST_1 = DC_CurrentST_1 * 135.0;
        DC_CurrentST_2 = (float)NHD_module;
        DC_CurrentST_2 = DC_CurrentST_2 * (-135.0);
    }
    //#else
    //-------------------------------------------------------------------
    //		InitialTempFloat=(float)NHD_module;
    //		HD_Module_RefUdc=HD_RefUdc/InitialTempFloat;
    //-------------------------------------------------------------------
    //#endif

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM = HD_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM = HD_PM_A2;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].NUM = NHD_module;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].NUM = NHD_module;

    Port_OnState = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2];
    HWREGH(0x6000062e) = Port_OnState;

    /************************PI参数************************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1 = ParameterTemp / 100.0;
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;
*/

    AVolt_Factor = PORT_Regulate_DATA.PORT_V_P_1;
    if (AVolt_Factor < 0.01)
    {
        AVolt_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1 = 100;
    }

    Volt_Factor = PORT_Regulate_DATA.PORT_V_I_1;
    if (Volt_Factor < 0.01)
    {
        Volt_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1 = 100;
    }

    Current_Factor = PORT_Regulate_DATA.PORT_I_P_1;
    if (Current_Factor < 0.01)
    {
        Current_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1 = 100;
    }

    //controller
    //voltage loop
    Volt_PID_Num_1 = Volt_PID_Num_1_def / Volt_Factor;
    Volt_PID_Num_0 = Volt_PID_Num_0_def / Volt_Factor;

    //Current loop
    Current_PID_Num_1 = Current_PID_Num_1_def / Current_Factor;
    Current_PID_Num_0 = Current_PID_Num_0_def / Current_Factor;

    ModuleVolt_PID_Num_1 = ModuleVolt_PID_Num_1_def / AVolt_Factor;
    ModuleVolt_PID_Num_0 = ModuleVolt_PID_Num_0_def / AVolt_Factor;

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}

/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
void RemoteRegulate_Dispose_2(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    /*
    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.HD_CtlMode;

    if((PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=0;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=1;
    }
    else if((PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=2;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=3;
    }
    else {}


    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16  =PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN=PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN=PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0,0,0,0,0);

    Port_FB_EN=PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();
*/
    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET; //1kw=10
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0; //w

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp;

    //-------------------------------------------------------------------
    //	#ifdef HD_2_LD
    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        HD_Module_RefUdc = 700.0;
    }
    else
    {
        HD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }
    HD_RefUdc = HD_Module_RefUdc;
    /*
    #else
        ParameterTemp=(float)NHD_module;
        ParameterTemp=ParameterTemp*740.0;

        ParameterTemp2=(float)NHD_module;
        ParameterTemp2=ParameterTemp2*600.0;

        if(PORT_Regulate_DATA.PORT_V_SET>ParameterTemp)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp;
            HD_RefUdc=ParameterTemp;
        }
        else if(PORT_Regulate_DATA.PORT_V_SET<ParameterTemp2)
        {
            PORT_Regulate_DATA.PORT_V_SET=ParameterTemp2;
            PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)ParameterTemp2;
            HD_RefUdc=ParameterTemp2;
        }
        else
        {
            HD_RefUdc=PORT_Regulate_DATA.PORT_V_SET;
        }

        ParameterTemp=(float)NHD_module;
        HD_Module_RefUdc=HD_RefUdc/ParameterTemp;
    #endif
*/
    //-------------------------------------------------------------------

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

    UstOK_HVDC_Port = HD_Module_RefUdc;
    Udc_Ref = HD_Module_RefUdc;

    /**********************下垂电压截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    //	PORT_Regulate_DATA.PORT_V0_SET=ParameterTemp;

    /************************电压下垂斜率********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    //	PORT_Regulate_DATA.PORT_KV_SET=ParameterTemp/10.0;

    /**************************保留数据**********************/
    /*
    for (i=0;i<4;i++) {
        ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i]=ParameterTemp;
    }
*/
    /**************************光纤插件模块配置**********************/
    /*
    HD_PM_A1=(ushort)PORT_Regulate_DATA.Reserved_1[0];
    HD_PM_A2=(ushort)PORT_Regulate_DATA.Reserved_1[1];

    NHD_module=0;

    for(i=0;i<8;i++)
    {
        InitialTemp=HD_PM_A1;
        if(InitialTemp&(0x0001<<i))
        {
            NHD_module++;
        }
        InitialTemp=HD_PM_A2;
        if(InitialTemp&(0x0001<<i))
        {
            NHD_module++;
        }
    }

    #ifdef HD_2_LD
    #else
        InitialTempFloat=(float)NHD_module;
        HD_Module_RefUdc=HD_RefUdc/InitialTempFloat;
    #endif

    FPGA_Interface[Com_A1_PluginUnit_5-2].PM=HD_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6-2].PM=HD_PM_A2;

    FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHD_module;
    FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHD_module;
*/

    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;

    Volt_Factor=PORT_Regulate_DATA.PORT_V_I_1;
    if(Volt_Factor<0.01)
    {
        Volt_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    }

    Current_Factor=PORT_Regulate_DATA.PORT_I_P_1;
    if(Current_Factor<0.01)
    {
        Current_Factor=1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    }

//controller
    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
    Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;

    //Current loop
    Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
    Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;
*/
    /************************EER2的数据************************/
    /*
    for (i=0;i<4;i++) {
        PORT_Regulate_DATA.EER2_DATA[i]=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }
*/

    Flag_PC = 1;
    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        //		DSP_2_EEPROM_Page2();
        //		DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
    Flag_PC = 0;
}
#endif

#ifdef LVDC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0, 0, 0, 0, 0);

    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0;

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp / 10.0;

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp / 10.0;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else
    {
        LD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    LD_RefUdc = LD_Module_RefUdc;
    UstOK_LVDC_Port = LD_RefUdc;

    Udc_Ref = LD_RefUdc;

    /**************************光纤插件模块配置**********************/
    LD_PM_A1 = (ushort)PORT_Regulate_DATA.Reserved_1[0];
    LD_PM_A2 = (ushort)PORT_Regulate_DATA.Reserved_1[1];

    NLD_module = 0;

    for (i = 0; i < 8; i++)
    {
        InitialTemp = LD_PM_A1;
        if (InitialTemp & (0x0001 << i))
        {
            NLD_module++;
        }
        InitialTemp = LD_PM_A2;
        if (InitialTemp & (0x0001 << i))
        {
            NLD_module++;
        }
    }

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM = LD_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM = LD_PM_A2;

    FPGA_Interface[Com_A1_PluginUnit_5 - 2].NUM = NLD_module;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].NUM = NLD_module;

    Port_OnState = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2];
    HWREGH(0x6000062e) = Port_OnState;
    /************************PI参数************************/

    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    //	PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1 = ParameterTemp / 100.0;

    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;

*/
    Volt_Factor = PORT_Regulate_DATA.PORT_V_I_1;
    if (Volt_Factor < 0.01)
    {
        Volt_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1 = 100;
    }

    Current_Factor = PORT_Regulate_DATA.PORT_I_P_1;
    if (Current_Factor < 0.01)
    {
        Current_Factor = 1.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1 = 100;
    }

    //controller
    //voltage loop
    Volt_PID_Num_1 = Volt_PID_Num_1_def / Volt_Factor;
    Volt_PID_Num_0 = Volt_PID_Num_0_def / Volt_Factor;

    //Current loop
    Current_PID_Num_1 = Current_PID_Num_1_def / Current_Factor;
    Current_PID_Num_0 = Current_PID_Num_0_def / Current_Factor;

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}

/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
void RemoteRegulate_Dispose_2(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */
    /*
    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual=PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LD_CtlMode;

    if((PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=0;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=1;
    }
    else if((PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=2;
    }
    else if((!PortRunFLAG.Bbits.Port_RunOrTest)&&(!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode=3;
    }
    else {}

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16  =PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN=PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN=PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(0,0,0,0,0);

    Port_FB_EN=PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();
*/
    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp * 100.0;

    HWREGH(0x6000062c) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET / 200;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp / 10.0;

    if (PORT_Regulate_DATA.PORT_V_SET > 750.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else if (PORT_Regulate_DATA.PORT_V_SET < 150.0)
    {
        PORT_Regulate_DATA.PORT_V_SET = 700.0;
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 700;
        LD_Module_RefUdc = 700.0;
    }
    else
    {
        LD_Module_RefUdc = PORT_Regulate_DATA.PORT_V_SET;
    }

    HWREGH(0x6000062A) = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    LD_RefUdc = LD_Module_RefUdc;
    UstOK_LVDC_Port = LD_RefUdc;

    Udc_Ref = LD_RefUdc;

    /**********************下垂电压截距**********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    //	PORT_Regulate_DATA.PORT_V0_SET=ParameterTemp/10.0;

    /************************电压下垂斜率********************/
    //	ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    //	PORT_Regulate_DATA.PORT_KV_SET=ParameterTemp/10.0;

    /**************************保留数据**********************/
    /*
    for (i=0;i<4;i++) {
        ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i]=ParameterTemp;
    }
*/
    /**************************光纤插件模块配置**********************/
    /*
    LD_PM_A1=(ushort)PORT_Regulate_DATA.Reserved_1[0];
    LD_PM_A2=(ushort)PORT_Regulate_DATA.Reserved_1[1];

    NLD_module=0;

    for(i=0;i<8;i++)
    {
        InitialTemp=LD_PM_A1;
        if(InitialTemp&(0x0001<<i))
        {
            NLD_module++;
        }
        InitialTemp=LD_PM_A2;
        if(InitialTemp&(0x0001<<i))
        {
            NLD_module++;
        }
    }
*/
    /************************PI参数************************/
    /*
    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3=ParameterTemp/100.0;

    ParameterTemp=(float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3=ParameterTemp/100.0;
*/

    /************************EER2的数据************************/
    /*
    for (i=0;i<4;i++) {
        PORT_Regulate_DATA.EER2_DATA[i]=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }
*/

    Flag_PC = 1;
    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        //		DSP_2_EEPROM_Page2();
        //		DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
    Flag_PC = 0;
}
#endif

#ifdef LVAC_online
/****************************************************************************/
/*                                                                          */
/*              Remote Regulate dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
void RemoteRegulate_Dispose(uchar Inital_Y)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */

    PORT_Regulate_DATA.COM_SYS_MODE.COM_SYS_MODE16 =
        PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;

    PortRunFLAG.Bbits.Port_RunOrTest = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LA_RunMode;
    PortRunFLAG.Bbits.Port_AutoOrManual = PORT_Regulate_DATA.COM_SYS_MODE.Bbits.LA_CtlMode;

    if ((PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 0;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 1;
    }
    else if ((PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 2;
    }
    else if ((!PortRunFLAG.Bbits.Port_RunOrTest) && (!PortRunFLAG.Bbits.Port_AutoOrManual))
    {
        Port_RealStatus.Bbits.S_Mode = 3;
    }
    else
    {
    }

    PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16;

    PORT_Regulate_DATA.FO_1_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16;
    PORT_Regulate_DATA.FO_2_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16;
    PORT_Regulate_DATA.FO_3_EN.FO_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16;

    PORT_Regulate_DATA.FB_EN.FB_EN16 = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16;

    Port_AD_PORT_EN = PORT_Regulate_DATA.AD_PROT_EN.AD_PORT_EN16;
    Port_ADProtect_EN();

    Port_FO_1_EN = PORT_Regulate_DATA.FO_1_EN.FO_EN16;
    Port_FO_EN_Set(4,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16,
                   PORT_Regulate_DATA.FO_2_EN.FO_EN16);

    Port_FB_EN = PORT_Regulate_DATA.FB_EN.FB_EN16;
    Port_FB_EN_Set();

    /**********************端口参数设置**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET;
    PORT_Regulate_DATA.PORT_P_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET;
    PORT_Regulate_DATA.PORT_Q_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;
    PORT_Regulate_DATA.PORT_V_SET = ParameterTemp / 10.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET;
    PORT_Regulate_DATA.PORT_F_SET = ParameterTemp / 10.0;

    /**********************下垂电压截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET;
    PORT_Regulate_DATA.PORT_V0_SET = ParameterTemp / 10.0;

    /**********************下垂频率截距**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET;
    PORT_Regulate_DATA.PORT_F0_SET = ParameterTemp / 10.0;

    /**********************频率下垂斜率**********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET;
    PORT_Regulate_DATA.PORT_KF_SET = ParameterTemp / 10.0;

    /************************电压下垂斜率********************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET;
    PORT_Regulate_DATA.PORT_KV_SET = ParameterTemp / 10.0;

    /**************************保留数据**********************/
    for (i = 0; i < 4; i++)
    {
        ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i];
        PORT_Regulate_DATA.Reserved_1[i] = ParameterTemp;
    }

    /**************************光纤插件模块配置**********************/
    LA_PM_A1 = (ushort)PORT_Regulate_DATA.Reserved_1[0];
    LA_PM_A2 = (ushort)PORT_Regulate_DATA.Reserved_1[1];

    NLA_module = 0;

    for (i = 0; i < 8; i++)
    {
        InitialTemp = LA_PM_A1;
        if (InitialTemp & (0x0001 << i))
        {
            NLA_module++;
        }
        InitialTemp = LA_PM_A2;
        if (InitialTemp & (0x0001 << i))
        {
            NLA_module++;
        }
    }
    /************************PI参数************************/
    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1;
    PORT_Regulate_DATA.PORT_V_P_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1;
    PORT_Regulate_DATA.PORT_V_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1;
    PORT_Regulate_DATA.PORT_I_P_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1;
    PORT_Regulate_DATA.PORT_I_I_1 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2;
    PORT_Regulate_DATA.PORT_V_P_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2;
    PORT_Regulate_DATA.PORT_V_I_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2;
    PORT_Regulate_DATA.PORT_I_P_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2;
    PORT_Regulate_DATA.PORT_I_I_2 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3;
    PORT_Regulate_DATA.PORT_V_P_3 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3;
    PORT_Regulate_DATA.PORT_V_I_3 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3;
    PORT_Regulate_DATA.PORT_I_P_3 = ParameterTemp / 100.0;

    ParameterTemp = (float)PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3;
    PORT_Regulate_DATA.PORT_I_I_3 = ParameterTemp / 100.0;

    /************************EER2的数据************************/
    for (i = 0; i < 4; i++)
    {
        PORT_Regulate_DATA.EER2_DATA[i] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i];
    }

    if (Inital_Y == 0)
    {
        DSP_2_EEPROM_Page1();
        DSP_2_EEPROM_Page2();
        DSP_2_EEPROM_Page3();

        FLAG_RemRegulate_SetOver = 1;
    }
}
#endif

#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*              Remote signal dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把端口浮点遥测及状态换算为 DPRAM 16 位上报缓存
 * 输入参数: 无
 * 输出参数: PORT_RealTime_DATA_buf 中的 16 位状态及量测；由 DPRAM_Write 复制到块 3。
 * 返回值: 无
 * 执行说明: 按端口编译分支打包；不在本函数内通知主机。
 *================================================================*/
void RemoteSig_Dispose(void)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16 = Port_MainStatus.MainStatus16;

    Port_DINRead(0);
    Port_RealStatus.Bbits.Contactor_Inner = SW.Bbits.S_CONTACTOR;
    if (SW.Bbits.S_INT_CB)
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 1;
        Port_RealStatus.Bbits.Breaker_InnerOff = 0;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 0;
        Port_RealStatus.Bbits.Breaker_InnerOff = 1;
    }
    //if(SW.Bbits.S_EXT_CB)
    //{
    //	Port_RealStatus.Bbits.Breaker_OuterOn=1;
    //	Port_RealStatus.Bbits.Breaker_OuterOff=0;
    //}
    //else
    //{
    //	Port_RealStatus.Bbits.Breaker_OuterOn=0;
    //	Port_RealStatus.Bbits.Breaker_OuterOff=1;
    //}
    //edited by shiqi
    if (GNDSwitch == 1)
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 1;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 0;
    }
    if (ISOSwitch == 1)
    {
        Port_RealStatus.Bbits.Breaker_OuterOff = 1;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_OuterOff = 0;
    }
    //end
    Port_RealStatus.Bbits.Module_Hbridge = PortRunFLAG.Bbits.PWMState_Module;
    Port_RealStatus.Bbits.LF_Hbridge = PortRunFLAG.Bbits.PWMState_50Hz;
    Port_RealStatus.Bbits.HF_Hbridge = PortRunFLAG.Bbits.PWMState_Dab;
    Port_RealStatus.Bbits.SWITCH_ON_EN = SW.Bbits.S_EXT_CB_ON_EN;

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_STATUS.PORT_STATUS16 = Port_RealStatus.PORT_STATUS16;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16 = Port_RealFault.PORT_FAULT16;

    ParameterTemp = PORT_RealTime_DATA.PORT_P;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_P = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_Q;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_Q = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_A = (short)ParameterTemp;

    ParameterTemp = PORT_RealTime_DATA.PORT_V_B;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_B = (short)ParameterTemp;

    ParameterTemp = PORT_RealTime_DATA.PORT_V_C;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_C = (short)ParameterTemp;

    ParameterTemp = PORT_RealTime_DATA.PORT_I_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_A = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_I_B;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_B = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_I_C;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_C = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.I_A_2;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.I_A_2 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.I_B_2;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.I_B_2 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.I_C_2;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.I_C_2 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.RMS_CURRENT_5TH;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.RMS_CURRENT_5TH = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.RMS_CURRENT_7TH;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.RMS_CURRENT_7TH = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.RMS_CURRENT_11TH;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.RMS_CURRENT_11TH = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PHA_CURRENT_5TH;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PHA_CURRENT_5TH = (short)(ParameterTemp * 1000.0);

    ParameterTemp = PORT_RealTime_DATA.PHA_CURRENT_7TH;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PHA_CURRENT_7TH = (short)(ParameterTemp * 1000.0);

    ParameterTemp = PORT_RealTime_DATA.PHA_CURRENT_11TH;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PHA_CURRENT_11TH = (short)(ParameterTemp * 1000.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_PF;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_PF = (short)(ParameterTemp * 100.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V0;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V0 = (short)ParameterTemp;

    ParameterTemp = PORT_RealTime_DATA.PORT_F0;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_F0 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_KF;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_KF = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_KV;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_KV = (short)(ParameterTemp * 10.0);

    for (i = 0; i < 3; i++)
    {
        ParameterTemp = PORT_RealTime_DATA.Reserved_1[i];
        PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.Reserved_1[i] = (short)ParameterTemp;
    }
}
#endif

#ifdef HVDC_online
/****************************************************************************/
/*                                                                          */
/*              Remote signal dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把端口浮点遥测及状态换算为 DPRAM 16 位上报缓存
 * 输入参数: 无
 * 输出参数: PORT_RealTime_DATA_buf 中的 16 位状态及量测；由 DPRAM_Write 复制到块 3。
 * 返回值: 无
 * 执行说明: 按端口编译分支打包；不在本函数内通知主机。
 *================================================================*/
void RemoteSig_Dispose(void)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16 = Port_MainStatus.MainStatus16;

    Port_DINRead(0);
    Port_RealStatus.Bbits.Contactor_Inner = SW.Bbits.S_CONTACTOR;
    if (SW.Bbits.S_INT_CB)
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 1;
        Port_RealStatus.Bbits.Breaker_InnerOff = 0;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 0;
        Port_RealStatus.Bbits.Breaker_InnerOff = 1;
    }
    if (SW.Bbits.S_EXT_CB)
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 1;
        Port_RealStatus.Bbits.Breaker_OuterOff = 0;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 0;
        Port_RealStatus.Bbits.Breaker_OuterOff = 1;
    }

    Port_RealStatus.Bbits.Module_Hbridge = PortRunFLAG.Bbits.PWMState_Module;
    Port_RealStatus.Bbits.LF_Hbridge = PortRunFLAG.Bbits.PWMState_50Hz;
    Port_RealStatus.Bbits.HF_Hbridge = PortRunFLAG.Bbits.PWMState_Dab;
    Port_RealStatus.Bbits.SWITCH_ON_EN = SW.Bbits.S_EXT_CB_ON_EN;

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_STATUS.PORT_STATUS16 = Port_RealStatus.PORT_STATUS16;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16 = Port_RealFault.PORT_FAULT16;

    ParameterTemp = PORT_RealTime_DATA.PORT_P;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_P = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_A = (short)ParameterTemp;

    ParameterTemp = PORT_RealTime_DATA.PORT_I_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_A = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V0;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V0 = (short)ParameterTemp;

    ParameterTemp = PORT_RealTime_DATA.PORT_KV;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_KV = (short)(ParameterTemp * 10.0);

    for (i = 0; i < 3; i++)
    {
        ParameterTemp = PORT_RealTime_DATA.Reserved_1[i];
        PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.Reserved_1[i] = (short)ParameterTemp;
    }
}
#endif

#ifdef LVDC_online
/****************************************************************************/
/*                                                                          */
/*              Remote signal dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把端口浮点遥测及状态换算为 DPRAM 16 位上报缓存
 * 输入参数: 无
 * 输出参数: PORT_RealTime_DATA_buf 中的 16 位状态及量测；由 DPRAM_Write 复制到块 3。
 * 返回值: 无
 * 执行说明: 按端口编译分支打包；不在本函数内通知主机。
 *================================================================*/
void RemoteSig_Dispose(void)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16 = Port_MainStatus.MainStatus16;

    Port_DINRead(0);
    Port_RealStatus.Bbits.Contactor_Inner = SW.Bbits.S_CONTACTOR;
    if (SW.Bbits.S_INT_CB)
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 1;
        Port_RealStatus.Bbits.Breaker_InnerOff = 0;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 0;
        Port_RealStatus.Bbits.Breaker_InnerOff = 1;
    }
    if (SW.Bbits.S_EXT_CB)
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 1;
        Port_RealStatus.Bbits.Breaker_OuterOff = 0;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 0;
        Port_RealStatus.Bbits.Breaker_OuterOff = 1;
    }

    Port_RealStatus.Bbits.Module_Hbridge = PortRunFLAG.Bbits.PWMState_Module;
    Port_RealStatus.Bbits.LF_Hbridge = PortRunFLAG.Bbits.PWMState_50Hz;
    Port_RealStatus.Bbits.HF_Hbridge = PortRunFLAG.Bbits.PWMState_Dab;
    Port_RealStatus.Bbits.SWITCH_ON_EN = SW.Bbits.S_EXT_CB_ON_EN;

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_STATUS.PORT_STATUS16 = Port_RealStatus.PORT_STATUS16;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16 = Port_RealFault.PORT_FAULT16;

    ParameterTemp = PORT_RealTime_DATA.PORT_P;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_P = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_A = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_I_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_A = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V0;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V0 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_KV;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_KV = (short)(ParameterTemp * 10.0);

    for (i = 0; i < 3; i++)
    {
        ParameterTemp = PORT_RealTime_DATA.Reserved_1[i];
        PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.Reserved_1[i] = (short)ParameterTemp;
    }
}
#endif

#ifdef LVAC_online
/****************************************************************************/
/*                                                                          */
/*              Remote signal dispose                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把端口浮点遥测及状态换算为 DPRAM 16 位上报缓存
 * 输入参数: 无
 * 输出参数: PORT_RealTime_DATA_buf 中的 16 位状态及量测；由 DPRAM_Write 复制到块 3。
 * 返回值: 无
 * 执行说明: 按端口编译分支打包；不在本函数内通知主机。
 *================================================================*/
void RemoteSig_Dispose(void)
{
    float ParameterTemp; /* ParameterTemp：原遥调/遥测的定点与浮点转换缓存。 */
    ushort i; /* i：复制字序号或当前编译端口内的模块位索引。 */

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16 = Port_MainStatus.MainStatus16;

    Port_DINRead(0);
    Port_RealStatus.Bbits.Contactor_Inner = SW.Bbits.S_CONTACTOR;
    if (SW.Bbits.S_INT_CB)
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 1;
        Port_RealStatus.Bbits.Breaker_InnerOff = 0;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_InnerOn = 0;
        Port_RealStatus.Bbits.Breaker_InnerOff = 1;
    }
    if (SW.Bbits.S_EXT_CB)
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 1;
        Port_RealStatus.Bbits.Breaker_OuterOff = 0;
    }
    else
    {
        Port_RealStatus.Bbits.Breaker_OuterOn = 0;
        Port_RealStatus.Bbits.Breaker_OuterOff = 1;
    }

    Port_RealStatus.Bbits.Module_Hbridge = PortRunFLAG.Bbits.PWMState_Module;
    Port_RealStatus.Bbits.LF_Hbridge = PortRunFLAG.Bbits.PWMState_50Hz;
    Port_RealStatus.Bbits.HF_Hbridge = PortRunFLAG.Bbits.PWMState_Dab;
    Port_RealStatus.Bbits.SWITCH_ON_EN = SW.Bbits.S_EXT_CB_ON_EN;

    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_STATUS.PORT_STATUS16 = Port_RealStatus.PORT_STATUS16;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16 = Port_RealFault.PORT_FAULT16;

    ParameterTemp = PORT_RealTime_DATA.PORT_P;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_P = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_Q;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_Q = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_A = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V_B;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_B = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V_C;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V_C = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_I_A;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_A = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_I_B;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_B = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_I_C;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_I_C = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.V_A_2;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.V_A_2 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.V_B_2;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.V_B_2 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.V_C_2;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.V_C_2 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.DC_V;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.DC_V = ParameterTemp;

    ParameterTemp = PORT_RealTime_DATA.PORT_PF;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_PF = (short)(ParameterTemp * 100.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_V0;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_V0 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_F0;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_F0 = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_KF;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_KF = (short)(ParameterTemp * 10.0);

    ParameterTemp = PORT_RealTime_DATA.PORT_KV;
    PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_KV = (short)(ParameterTemp * 10.0);

    for (i = 0; i < 3; i++)
    {
        ParameterTemp = PORT_RealTime_DATA.Reserved_1[i];
        PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.Reserved_1[i] = (short)ParameterTemp;
    }
}
#endif
