#include "SstControlConfig.h"
//###########################################################################
// FILE: Ft3Recorder.c
// FUNCTION: FT3故障录波通信及原八字接收缓存。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#include "ControlData.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "FpgaRegister.h"
#include "Ft3RecorderData.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstTypes.h"

#include "Ft3Recorder.h"
#include "ChbBuildConfig.h"

/*故障录波装置通信测试用*/
struct _Receive_Data
{
    unsigned short Rec_Data1; /* Rec_Data1：FT3 接收第 1 个原始 16 位字。 */
    unsigned short Rec_Data2; /* Rec_Data2：FT3 接收第 2 个原始 16 位字。 */
    unsigned short Rec_Data3; /* Rec_Data3：FT3 接收第 3 个原始 16 位字。 */
    unsigned short Rec_Data4; /* Rec_Data4：FT3 接收第 4 个原始 16 位字。 */
    unsigned short Rec_Data5; /* Rec_Data5：FT3 接收第 5 个原始 16 位字。 */
    unsigned short Rec_Data6; /* Rec_Data6：FT3 接收第 6 个原始 16 位字。 */
    unsigned short Rec_Data7; /* Rec_Data7：FT3 接收第 7 个原始 16 位字。 */
    unsigned short Rec_Data8; /* Rec_Data8：FT3 接收第 8 个原始 16 位字。 */

} FT3_Rec_Data; /* FT3_Rec_Data：故障录波 FT3 接收的八字原始数据缓存。 */

unsigned int temp_rec; /* temp_rec：FT3 接收通信测试的原临时观察值。 */

/*==================================================================
 * 函数功能: 组装 FT3 采样、状态及故障数据并写入所选 FT3 插件
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 *           dataPackNum：FT3 本次发送的数据包数/控制字字段
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void FT3_PluginUint_Write(
    unsigned short FT3_index,
    unsigned short dataPackNum) //PORT_Index端口号(1,2,3,4分别为HVAC,HVDC,LVDC,LVAC),dataPackNum数据包号
{
    switch (FT3_index)
    {
        case FT3_FaultRecorder:
        {
            /*
            PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16 = DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16;
            if(FR_tri_count1 < 300)
            {
                FR_tri_count1++;
            }
            else
            {
                FR_tri_count1 = 0;
            }

            if(FR_tri_count2 < 250)
            {
                FR_tri_count2++;
            }
            else
            {
                FR_tri_count2 = 0;
            }
            if(FR_tri_count3 < 200)
            {
                FR_tri_count3++;
            }
            else
            {
                FR_tri_count3 = 0;
            }
            */
            /*
            if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x00)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_Initialing = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x01)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_Initial_Success = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x02)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_Selfcheck = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x03)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_Precharge = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x04)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_selfcheck_fail = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x05)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_ready = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x20)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_Run = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x07)
            {
                FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
                FaultRecordStatus.Bits.StatusNum_Stop = 1;
            }
            else
            {
            }
            */
            if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x05)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
                FaultRecordStatus.Bits.StatusNum_ready = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x20)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
                FaultRecordStatus.Bits.StatusNum_Run = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x07)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
                FaultRecordStatus.Bits.StatusNum_Stop = 1;
            }
            else
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
            }

            if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_S1_Mode == 0x01)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0xFFF7);
                FaultRecordStatus.Bits.STATCOM_Q = 1;
            }
            else
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0xFFF7);
            }

            FaultRecordStatus.Bits.Sys_Stop = PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.Sys_Stop;
            FaultRecordStatus.Bits.Outer_Elec = PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.Outer_Elec;

            if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x00)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
                FaultRecordStatus.Bits.voltage_mode = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x01)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
                FaultRecordStatus.Bits.power_mode = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x02)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
                FaultRecordStatus.Bits.droop_mode = 1;
            }
            else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x03)
            {
                FaultRecordStatus.FaultREcord_Word16 = FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
                FaultRecordStatus.Bits.gridfollow_mode = 1;
            }
            else
            {
            }
#ifdef HVAC_online
            //PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16 = DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
            if ((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL & 0x00000001) ==
                0) //根据格欣文档，最低位为0时才可发送数据
            {
                /*
                        if(dataPackNum==0x01)
                        {

                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;

                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x013F;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=HA_Port_Volt_AB;//程序中变量名为HA_Port_Volt_AB？
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=HA_Port_Volt_BC;//程序中变量名为HA_Port_Volt_BC？
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=HA_Port_Volt_CA;//程序中变量名为HA_Port_Volt_CA？

                        }
                        else if(dataPackNum==0x02)
                        {

                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;

                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x023F;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=HA_Port_Current_A;//程序中变量名为HA_Port_Current_A？
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=HA_Port_Current_B;//程序中变量名为HA_Port_Current_B？
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=HA_Port_Current_C;//程序中变量名为HA_Port_Current_C？
                        }

                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FaultRecordStatus_1.FaultREcord_Word16_1;
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FaultRecordStatus.FaultREcord_Word16;
                        //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=faultsignal;

                        FT3_Samp_Cnt++;
                        if(FT3_Samp_Cnt==50000)
                        {
                            FT3_Samp_Cnt=0;
                        }
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=FT3_Samp_Cnt;
                        */

                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16 =
                    FaultRecordStatus.FaultREcord_Word16;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1 = HA_Port_Volt_AB; //
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2 = HA_Port_Volt_BC; //
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3 = HA_Port_Volt_CA; //
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4 = HA_Port_Current_A * 10; //
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5 = HA_Port_Current_B * 10; //
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6 = HA_Port_Current_C * 10; //
#if SST_ENABLE_DPRAM
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt =
                    DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
#else
                /* Current fault word in local RAM; no disabled CS5 read. */
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt = Port_RealFault.PORT_FAULT16;
#endif

                /*
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=FaultRecordStatus.FaultREcord_Word16;
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=FR_tri_count1;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=FR_tri_count2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=FR_tri_count3;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FR_tri_count1*2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FR_tri_count2*2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=FR_tri_count3*2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                        */
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL = 1;
            }
#endif

#ifdef HVDC_online
            if ((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL & 0x00000001) == 0)
            {
                if (dataPackNum == 0x01)
                {
                    /*
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                            */
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16 = 0x0139;
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1 =
                        HD_Port_Volt; //程序中变量名为HD_Port_Volt？
                }
                else if (dataPackNum == 0x02)
                {
                    /*
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                            */
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16 = 0x0239;
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1 =
                        HD_Port_Current; //程序中变量名为HD_Port_Current？
                }

                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4 = FaultRecordStatus_1.FaultREcord_Word16_1;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5 = FaultRecordStatus.FaultREcord_Word16;
                //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6 = faultsignal;
                FT3_Samp_Cnt++;
                if (FT3_Samp_Cnt == 50000)
                {
                    FT3_Samp_Cnt = 0;
                }
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt = FT3_Samp_Cnt;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL = 1;
            }
#endif

#ifdef LVDC_online
            if ((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL & 0x00000001) == 0)
            {
                /*
                        if(dataPackNum==0x01)
                        {
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x0139;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=LD_TotalVDC;//程序中变量名为LD_TotalVDC？

                        }
                        else if(dataPackNum==0x02)
                        {
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x0239;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=LD_Port_PositiveCurrent;//程序中变量名为LD_Port_PositiveCurrent？

                        }

                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FaultRecordStatus_1.FaultREcord_Word16_1;
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FaultRecordStatus.FaultREcord_Word16;
                        //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=faultsignal;
                        FT3_Samp_Cnt++;
                        if(FT3_Samp_Cnt==50000)
                        {
                            FT3_Samp_Cnt=0;
                        }
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=FT3_Samp_Cnt;*/
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16 =
                    FaultRecordStatus.FaultREcord_Word16;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1 = LD_TotalVDC * 10;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4 = LD_Port_PositiveCurrent * 10;
                //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=FR_tri_count1;
                //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FR_tri_count3;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt =
                    DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL = 1;
            }
#endif

#ifdef LVAC_online
            if ((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL & 0x00000001) == 0)
            {
                if (dataPackNum == 0x01)
                {
                    /*
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                            */
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16 = 0x013F;
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1 =
                        LA_Port_Volt_A; //程序中变量名为LA_Port_Volt_A？
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2 =
                        LA_Port_Volt_B; //程序中变量名为LA_Port_Volt_B？
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3 =
                        LA_Port_Volt_C; //程序中变量名为LA_Port_Volt_C？
                }
                else if (dataPackNum == 0x02)
                {
                    /*
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                            FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                            */
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16 = 0x023F;
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1 =
                        LA_Ig_Current_A; //程序中变量名为LA_Ig_Current_A？
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2 =
                        LA_Ig_Current_B; //程序中变量名为LA_Ig_Current_B？
                    FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3 =
                        LA_Ig_Current_C; //程序中变量名为LA_Ig_Current_C？
                }

                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4 = FaultRecordStatus_1.FaultREcord_Word16_1;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5 = FaultRecordStatus.FaultREcord_Word16;
                //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6 = faultsignal;
                FT3_Samp_Cnt++;
                if (FT3_Samp_Cnt == 50000)
                {
                    FT3_Samp_Cnt = 0;
                }
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt = FT3_Samp_Cnt;
                FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL = 1;
            }

#endif
        }

        case FT3_PTCT:
        {
        }
        break;

        case FT3_EER2:
        {
        }
        break;

        case FT3_Backup:
        {
        }
        break;
        default:
            break;
    }
}

/*==================================================================
 * 函数功能: 读取所选 FT3 插件接收区到兼容接收缓存
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void FT3_PluginUint_Read(ushort FT3_index)
{
    switch (FT3_index)
    {
        case FT3_FaultRecorder:
        {
            if ((FT3_FaultRecorder_Register.FT3_Register.FT3_ReadCTL & 0x00000001) ==
                0x00000001) //根据格欣文档，此位为1表示有数据
            {
                FT3_Rec_Data.Rec_Data1 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_1;
                FT3_Rec_Data.Rec_Data2 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_2;
                FT3_Rec_Data.Rec_Data3 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_3;
                FT3_Rec_Data.Rec_Data4 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_4;
                FT3_Rec_Data.Rec_Data5 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_5;
                FT3_Rec_Data.Rec_Data6 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_6;
                FT3_Rec_Data.Rec_Data7 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_7;
                FT3_Rec_Data.Rec_Data8 = FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_8;
                FT3_FaultRecorder_Register.FT3_Register.FT3_ReadCTL = 0;
            }
        }
        break;
        case FT3_PTCT:
        {
        }
        break;

        case FT3_EER2:
        {
        }
        break;

        case FT3_Backup:
        {
        }
        break;

        default:
            break;
    }
}
