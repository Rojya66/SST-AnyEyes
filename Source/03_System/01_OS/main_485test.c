//###########################################################################
//
// FILE:   Main.c
//
// FUNCTION:  1)Variable initialization
//            2)Peripheral initialization
//            3)Main program
//            4)Interrupt
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

//#include "TL6748.h"
#include "hw_types.h"
#include "hw_syscfg0_C6748.h"
#include "interrupt.h"

#include "psc.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"

#include "main.h"
#include "Hardware.h"
#include "EMIFAPinmuxSetup.h"
#include "emifa.h"
#include "X1226_RW.h"
#include "DPRAM.h"
#include "ADC_Sample.h"
#include "PluginUnit.h"
#include "Stateflow.h"
#include "FaultCheck.h"
#include "HVAC_Strategy.h"
#include "MaintenanceProtocol.H"
#include "ChbDabControl.h"
#include "ChbApplication.h"
#include "ChbBuildConfig.h"
#include "vofa.h"
/**************************Macro definition*********************************/
/* ReadController：旧维护协议控制器读取功能码。 */
#define ReadController  0x03
/* WriteController：旧维护协议控制器写入功能码。 */
#define WriteController 0x05
/* ControllerAddr：旧维护协议本控制器地址。 */
#define ControllerAddr  0x10

/* ValidAddr_CS2_S：旧维护 FPGA CS2 合法地址下界。 */
#define ValidAddr_CS2_S  0x60000000
/* ValidAddr_CS2_E：旧维护 FPGA CS2 合法地址上界。 */
#define ValidAddr_CS2_E  0x60001FFE
/* ValidAddr_CS5_S：旧维护 DPRAM CS5 合法地址下界。 */
#define ValidAddr_CS5_S  0x66000000
/* ValidAddr_CS5_E：旧维护 DPRAM CS5 合法地址上界。 */
#define ValidAddr_CS5_E  0x66003FFE

/************************Variable definition********************************/
#pragma DATA_SECTION(FPGA_Interface,".FPGA_InterfaceFile");
/*
#pragma DATA_SECTION(FPGA_PU_1,".FPGA_PU_1");
#pragma DATA_SECTION(FPGA_PU_2,".FPGA_PU_2");
#pragma DATA_SECTION(FPGA_PU_3,".FPGA_PU_3");
#pragma DATA_SECTION(FPGA_PU_4,".FPGA_PU_4");
#pragma DATA_SECTION(FPGA_PU_5,".FPGA_PU_5");
#pragma DATA_SECTION(FPGA_PU_6,".FPGA_PU_6");
#pragma DATA_SECTION(FPGA_PU_7,".FPGA_PU_7");
#pragma DATA_SECTION(FPGA_PU_8,".FPGA_PU_8");
#pragma DATA_SECTION(FPGA_PU_9,".FPGA_PU_9");
#pragma DATA_SECTION(FPGA_PU_10,".FPGA_PU_10");
#pragma DATA_SECTION(FPGA_PU_11,".FPGA_PU_11");
#pragma DATA_SECTION(FPGA_PU_12,".FPGA_PU_12");
#pragma DATA_SECTION(FPGA_PU_13,".FPGA_PU_13");
#pragma DATA_SECTION(FPGA_PU_14,".FPGA_PU_14");
#pragma DATA_SECTION(FPGA_PU_15,".FPGA_PU_15");
#pragma DATA_SECTION(FPGA_PU_16,".FPGA_PU_16");
*/


#pragma DATA_SECTION(DPRAM_Block1_remControl,".DPRAM_Block1_remControl");
#pragma DATA_SECTION(DPRAM_Block2_remRegulate,".DPRAM_Block2_remRegulate");
#pragma DATA_SECTION(DPRAM_Block3_remMeaSig,".DPRAM_Block3_remMeaSig");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit2,".DPRAM_Block4_PluginUnit2");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit3,".DPRAM_Block4_PluginUnit3");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit4,".DPRAM_Block4_PluginUnit4");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit5,".DPRAM_Block4_PluginUnit5");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit6,".DPRAM_Block4_PluginUnit6");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit7,".DPRAM_Block4_PluginUnit7");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit8,".DPRAM_Block4_PluginUnit8");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit9,".DPRAM_Block4_PluginUnit9");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit10,".DPRAM_Block4_PluginUnit10");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit11,".DPRAM_Block4_PluginUnit11");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit12,".DPRAM_Block4_PluginUnit12");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit13,".DPRAM_Block4_PluginUnit13");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit14,".DPRAM_Block4_PluginUnit14");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit15,".DPRAM_Block4_PluginUnit15");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit16,".DPRAM_Block4_PluginUnit16");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit17,".DPRAM_Block4_PluginUnit17");


#pragma DATA_SECTION(FT3_PTCT_Register,".FT3_PTCT_Register");
#pragma DATA_SECTION(FT3_FaultRecorder_Register,".FT3_FaultRecorder_Register");
#pragma DATA_SECTION(FT3_EER2_Register,".FT3_EER2_Register");
#pragma DATA_SECTION(FT3_Backup_Register,".FT3_Backup_Register");


//register of Plug-in unit
volatile FPGA_InterfaceBLOCK FPGA_Interface[15]; /* FPGA_Interface：15 个插件的 EMIFA 映射寄存器窗口，索引为原插件编号减 2。 */
XXG_MainStatus MainStatus_HVAC,MainStatus_HVDC,MainStatus_LVDC,MainStatus_LVAC,Port_MainStatus; /* MainStatus_HVAC：HVAC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* MainStatus_HVDC：HVDC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* MainStatus_LVDC：LVDC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* MainStatus_LVAC：LVAC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* Port_MainStatus：当前端口主状态、主/子工作模式的位域缓存。 */

XXG_MainStatus Port_MainStatus; /* Port_MainStatus：当前端口主状态、主/子工作模式的位域缓存。 */
ushort Port_Main_WorkMode; /* Port_Main_WorkMode：当前端口主工作模式编号。 */
ushort Port_STATCOM_WorkMode; /* Port_STATCOM_WorkMode：旧 STATCOM 无功/功率因数/电压工作模式编号。 */
ushort Port_APF_WorkMode; /* Port_APF_WorkMode：旧 APF 谐波补偿工作模式编号。 */
ushort Port_Outer_Elec; /* Port_Outer_Elec：端口外部电源接入状态。 */
ushort Port_Sys_Stop; /* Port_Sys_Stop：旧系统停机状态。 */
ushort Port_State; /* Port_State：旧端口状态编号，迁移核心维护兼容镜像。 */
ushort Port_RunStatus; /* Port_RunStatus：旧端口运行状态编号。 */

//edited by shiqi, fault recorder
volatile XXG_FaultRecordStatus FaultRecordStatus; /* FaultRecordStatus：FT3 故障录波主运行模式状态字。 */
volatile XXG_FaultRecordStatus_1 FaultRecordStatus_1; /* FaultRecordStatus_1：FT3 故障录波谐波/STATCOM 模式状态字。 */
volatile XXG_FT3_Register  FT3_PTCT_Register,FT3_FaultRecorder_Register,FT3_EER2_Register,FT3_Backup_Register; /* FT3_PTCT_Register：PT/CT FT3 插件的原映射寄存器窗口。 */
/* FT3_FaultRecorder_Register：故障录波 FT3 插件的原映射寄存器窗口。 */
/* FT3_EER2_Register：EER2 FT3 插件的原映射寄存器窗口。 */
/* FT3_Backup_Register：备用 FT3 插件的原映射寄存器窗口。 */
ushort faultsignal; /* faultsignal：原 FT3 故障触发信号。 */
ushort faultRecordStart; /* faultRecordStart：原 FT3 录波启动标志。 */
ushort fault_count; /* fault_count：原故障录波触发计数。 */
ushort fault_count_start; /* fault_count_start：原故障录波开始计数/状态。 */
ushort FR_tri_count1, FR_tri_count2, FR_tri_count3; /* FR_tri_count1：原 FT3 录波第 1 组触发计数。 */
/* FR_tri_count2：原 FT3 录波第 2 组触发计数。 */
/* FR_tri_count3：原 FT3 录波第 3 组触发计数。 */

XXG_DSP_CTL Port_DSP_CTL; /* Port_DSP_CTL：端口 FPGA 全关断及硬件复位控制字缓存。 */
XXG_SwitchINOUT Port_DOUT; /* Port_DOUT：端口断路器、接触器数字输出控制字缓存。 */
XXG_PM_CTL Module_PM_CTL; /* Module_PM_CTL：模块工频桥/DAB 使能、驱动复位及测试控制字缓存。 */

//register of Plug-in unit in DPRAM------remote measurement
volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit2,DPRAM_Block4_PluginUnit3,DPRAM_Block4_PluginUnit4,DPRAM_Block4_PluginUnit5,DPRAM_Block4_PluginUnit6; /* DPRAM_Block4_PluginUnit2：原地址映射的插件 2 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit3：原地址映射的插件 3 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit4：原地址映射的插件 4 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit5：原地址映射的插件 5 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit6：原地址映射的插件 6 遥测块，仅启用 DPRAM 时访问。 */
volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit7,DPRAM_Block4_PluginUnit8,DPRAM_Block4_PluginUnit9,DPRAM_Block4_PluginUnit10,DPRAM_Block4_PluginUnit11; /* DPRAM_Block4_PluginUnit7：原地址映射的插件 7 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit8：原地址映射的插件 8 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit9：原地址映射的插件 9 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit10：原地址映射的插件 10 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit11：原地址映射的插件 11 遥测块，仅启用 DPRAM 时访问。 */
volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit12,DPRAM_Block4_PluginUnit13,DPRAM_Block4_PluginUnit14,DPRAM_Block4_PluginUnit15,DPRAM_Block4_PluginUnit16,DPRAM_Block4_PluginUnit17; /* DPRAM_Block4_PluginUnit12：原地址映射的插件 12 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit13：原地址映射的插件 13 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit14：原地址映射的插件 14 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit15：原地址映射的插件 15 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
/* DPRAM_Block4_PluginUnit17：原地址映射的插件 17 遥测块，仅启用 DPRAM 时访问。 */
//volatile XXG_DPRAM_PluginUnit FPGA_PU_1,FPGA_PU_2,FPGA_PU_3,FPGA_PU_4,FPGA_PU_5,FPGA_PU_6,FPGA_PU_7,FPGA_PU_8,FPGA_PU_9,FPGA_PU_10,FPGA_PU_11,FPGA_PU_12,FPGA_PU_13,FPGA_PU_14,FPGA_PU_15,FPGA_PU_16;

//remote measurement in DPRAM
volatile XXG_DPRAM_RemMeaSig DPRAM_Block3_remMeaSig; /* DPRAM_Block3_remMeaSig：原地址映射的端口遥测区；访问受 SST_ENABLE_DPRAM 门控。 */
XXG_DPRAM_RemMeaSig PORT_RealTime_DATA_buf; /* PORT_RealTime_DATA_buf：端口遥测的 16 位定点打包缓存。 */
DPRAM_RemMeaSigFloat PORT_RealTime_DATA; /* PORT_RealTime_DATA：端口遥测的浮点物理量缓存及兼容状态。 */
XXG_PORT_STATUS Port_RealStatus; /* Port_RealStatus：端口开关及桥状态的原位域缓存。 */
XXG_PORT_FAULT Port_RealFault; /* Port_RealFault：端口故障的原位域缓存，某些位在 HVAC 中复用。 */

//remote control in DPRAM
volatile XXG_DPRAM_RemControl DPRAM_Block1_remControl; /* DPRAM_Block1_remControl：原地址映射的遥控区；访问受 SST_ENABLE_DPRAM 门控。 */
XXG_DPRAM_RemControl PORT_Control_DATA_Buf; /* PORT_Control_DATA_Buf：旧遥控协议的本地字缓存。 */
XXG_COM_START_PORT Port_COM_START; /* Port_COM_START：旧遥控启动字的位域解码缓存。 */
XXG_COM_STOP_PORT Port_COM_STOP; /* Port_COM_STOP：旧遥控停止字的位域解码缓存。 */

//remote regulate in DPRAM
volatile XXG_DPRAM_RemRegulate DPRAM_Block2_remRegulate; /* DPRAM_Block2_remRegulate：原地址映射的遥调区；访问受 SST_ENABLE_DPRAM 门控。 */
XXG_DPRAM_RemRegulate PORT_Regulate_DATA_Buf; /* PORT_Regulate_DATA_Buf：旧遥调协议的本地定点字缓存。 */
DPRAM_RemRegulateFloat PORT_Regulate_DATA; /* PORT_Regulate_DATA：旧遥调定点值换算后的浮点给定缓存。 */

ushort Port_AD_PORT_EN; /* Port_AD_PORT_EN：端口各通道 AD 保护使能位图。 */
ushort Port_FO_1_EN; /* Port_FO_1_EN：第 1 组光纤故障输入使能位图。 */
ushort Port_FO_2_EN; /* Port_FO_2_EN：第 2 组光纤故障输入使能位图。 */
ushort Port_FO_3_EN; /* Port_FO_3_EN：第 3 组光纤故障输入使能位图。 */
ushort Port_FB_EN; /* Port_FB_EN：端口 FB 硬件故障输入使能位图。 */


XXG_SwitchFLAG SW; /* SW：旧开关时序、动作状态及完成标志的位域缓存。 */
XXG_FaultTypeFLAG FaultTypeFLAG; /* FaultTypeFLAG：原故障分类位图，后台故障报告据此映射五位码。 */
XXG_PortRunFLAG PortRunFLAG; /* PortRunFLAG：旧端口运行、预充、PWM 及相序状态的位域缓存。 */
XXG_ControlFLAG ControlFLAG; /* ControlFLAG：旧启动、停止、PWM、DAB 斜坡流程控制标志。 */
XXG_FaultDisposeFLAG FaultDisposeFLAG; /* FaultDisposeFLAG：原故障处理策略及 PWM 禁用范围的位域缓存。 */
ushort ModuleIndex_PWMPulseDisable; /* ModuleIndex_PWMPulseDisable：原故障策略中待禁用 PWM 的模块索引。 */


/******************************************************************************/
//Common Variables
/******************************************************************************/
ushort WrongTimes_LEFT, WrongTimes_RIGHT,WrongTimes_LEFT_A, WrongTimes_RIGHT_A; /* WrongTimes_LEFT：原 FPGA LEFT 参考/通信核对的累计不一致次数。 */
/* WrongTimes_RIGHT：原 FPGA RIGHT 参考/通信核对的累计不一致次数。 */
/* WrongTimes_LEFT_A：原 FPGA LEFT_A 参考/通信核对的累计不一致次数。 */
/* WrongTimes_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的累计不一致次数。 */
ushort LastWrong_LEFT, LastWrong_RIGHT,LastWrong_LEFT_A, LastWrong_RIGHT_A; /* LastWrong_LEFT：原 FPGA LEFT 参考/通信核对的上一轮不一致值。 */
/* LastWrong_RIGHT：原 FPGA RIGHT 参考/通信核对的上一轮不一致值。 */
/* LastWrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的上一轮不一致值。 */
/* LastWrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的上一轮不一致值。 */
ushort Wrong_LEFT, Wrong_RIGHT,Wrong_LEFT_A, Wrong_RIGHT_A; /* Wrong_LEFT：原 FPGA LEFT 参考/通信核对的本轮不一致值。 */
/* Wrong_RIGHT：原 FPGA RIGHT 参考/通信核对的本轮不一致值。 */
/* Wrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的本轮不一致值。 */
/* Wrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的本轮不一致值。 */

ushort WrongTimes_Reference_A1,WrongTimes_Reference_A2,WrongTimes_Reference_C1,WrongTimes_Reference_C2; /* WrongTimes_Reference_A1：原 FPGA Reference_A1 参考/通信核对的累计不一致次数。 */
/* WrongTimes_Reference_A2：原 FPGA Reference_A2 参考/通信核对的累计不一致次数。 */
/* WrongTimes_Reference_C1：原 FPGA Reference_C1 参考/通信核对的累计不一致次数。 */
/* WrongTimes_Reference_C2：原 FPGA Reference_C2 参考/通信核对的累计不一致次数。 */
ushort LastWrong_Reference_A1,LastWrong_Reference_A2,LastWrong_Reference_C1,LastWrong_Reference_C2; /* LastWrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的上一轮不一致值。 */
/* LastWrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的上一轮不一致值。 */
/* LastWrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的上一轮不一致值。 */
/* LastWrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的上一轮不一致值。 */
ushort Wrong_Reference_A1,Wrong_Reference_A2,Wrong_Reference_C1,Wrong_Reference_C2; /* Wrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的本轮不一致值。 */
/* Wrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的本轮不一致值。 */
/* Wrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的本轮不一致值。 */
/* Wrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的本轮不一致值。 */

ushort WrongTimes_ControlReference_A,WrongTimes_ControlReference_C; /* WrongTimes_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的累计不一致次数。 */
/* WrongTimes_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的累计不一致次数。 */
ushort LastWrong_ControlReference_A,LastWrong_ControlReference_C; /* LastWrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的上一轮不一致值。 */
/* LastWrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的上一轮不一致值。 */
ushort Wrong_ControlReference_A,Wrong_ControlReference_C; /* Wrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的本轮不一致值。 */
/* Wrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的本轮不一致值。 */

//FLAG
unsigned char Flag_PC; /* Flag_PC：旧 PC/本地控制兼容标志。 */
unsigned char Flag_DC_FAULT; /* Flag_DC_FAULT：原直流故障总标志。 */
unsigned short Flag_DC_FAULTDISPOSE; /* Flag_DC_FAULTDISPOSE：原直流故障处置状态。 */
unsigned char Flag_DC_FAULTport; /* Flag_DC_FAULTport：原直流故障端口标识。 */

unsigned char Flag_DC_VoltSource; /* Flag_DC_VoltSource：原直流电压源控制模式标志。 */
unsigned char Flag_DC_VoltSource_Set; /* Flag_DC_VoltSource_Set：原直流电压源模式设置完成标志。 */
unsigned char flag_INV; /* flag_INV：原工频桥开环逆变模式标志。 */
unsigned char flag_SVGOPEN; /* flag_SVGOPEN：原工频桥开环投入标志。 */
unsigned char flag_DABOPEN; /* flag_DABOPEN：原 DAB 开环投入标志。 */
unsigned char flag_FaultClose; /* flag_FaultClose：原故障后脉冲关闭标志。 */
unsigned char Flag_FaultOff; /* Flag_FaultOff：原故障停机标志。 */
unsigned char FLAG_RemRegulate_SetOver; /* FLAG_RemRegulate_SetOver：原遥调设置完成标志。 */
unsigned char flag_SendDABRef_First; /* flag_SendDABRef_First：原 DAB 参考首次下发标志。 */
unsigned char flag_SendSVGRef_First; /* flag_SendSVGRef_First：原工频桥参考首次下发标志。 */

unsigned char Flag_testT; /* Flag_testT：原测试定时观察标志。 */
unsigned char Flag_Key; /* Flag_Key：原本地按键扫描状态。 */
unsigned char Flag_temp; /* Flag_temp：后台运行指示灯翻转状态。 */
unsigned char Flag_WDI; /* Flag_WDI：后台外部看门狗 GPIO 翻转状态。 */

/* VOFA数据发送标志 */
unsigned char Flag_VofaSend; /* Flag_VofaSend：保留原 VOFA 发送通知变量；迁移发送由后台时基调度。 */

/* 维护协议兼容状态：协议解析已迁移到 MaintenanceProtocol，保留原符号供
 * 兼容状态回读和旧中断路径链接使用。 */
unsigned char Flag_Uart; /* Flag_Uart：旧 UART 维护帧兼容状态，保留符号供原路径回读。 */
unsigned char CommNum; /* CommNum：旧 UART 兼容通信序号。 */
unsigned char CommFunc; /* CommFunc：旧 UART 兼容功能码。 */
unsigned int StartAddr; /* StartAddr：旧 UART 兼容请求的起始地址。 */
unsigned short ReadLength; /* ReadLength：旧 UART 兼容读取长度。 */
unsigned char CommLength; /* CommLength：旧 UART 兼容帧有效长度。 */
unsigned char CommInNum; /* CommInNum：旧 UART 兼容帧当前接收位置。 */
unsigned char CommTotalNum; /* CommTotalNum：旧 UART 兼容帧预期总长度。 */
unsigned char WriteOK; /* WriteOK：旧 UART 兼容写入成功标志。 */
unsigned char PCuartBUFF[140]; /* PCuartBUFF：旧 PC UART 帧兼容缓存，容量 140 字节。 */
unsigned char ReceiveuartBUFF[11]; /* ReceiveuartBUFF：旧 UART 接收帧兼容缓存，容量 11 字节。 */

unsigned char Flag_WaitPowerStart10s; /* Flag_WaitPowerStart10s：Timer2 更新、后台消费的主电源等待 10 s到时标志。 */

unsigned char Flag_1s; /* Flag_1s：Timer2 更新、后台消费的周期 1 s到时标志。 */
unsigned char Flag_50ms; /* Flag_50ms：Timer2 更新、后台消费的周期 50 ms到时标志。 */
unsigned char Flag_100ms; /* Flag_100ms：Timer2 更新、后台消费的周期 100 ms到时标志。 */
unsigned char Flag_RemoteMeasure; /* Flag_RemoteMeasure：Timer2 更新、后台消费的遥测轮询到时标志。 */

unsigned char Flag_Switch500ms; /* Flag_Switch500ms：Timer2 更新、后台消费的开关操作等待 500 ms到时标志。 */
unsigned char Flag_Switch1s; /* Flag_Switch1s：Timer2 更新、后台消费的开关操作等待 1 s到时标志。 */
unsigned char Flag_Precharge500ms; /* Flag_Precharge500ms：Timer2 更新、后台消费的预充等待 500 ms到时标志。 */
unsigned char Flag_PreStart3s; /* Flag_PreStart3s：Timer2 更新、后台消费的启动准备等待 3 s到时标志。 */
unsigned char Flag_SVG1s; /* Flag_SVG1s：Timer2 更新、后台消费的SVG 投入等待 1 s到时标志。 */
unsigned char Flag_20s; /* Flag_20s：Timer2 更新、后台消费的上电等待 20 s到时标志。 */
unsigned char Flag_Clear_0,Flag_Clear_1; /* Flag_Clear_0：原故障锁存清除第一阶段请求标志。 */
/* Flag_Clear_1：原故障锁存清除第二阶段请求标志。 */
unsigned char Flag_Clear_zhi; /* Flag_Clear_zhi：原故障清除期间的兼容状态。 */

unsigned char Flag_Wait; /* Flag_Wait：原控制流程等待标志。 */
unsigned char Flag_StopWait; /* Flag_StopWait：原停机延时等待标志。 */
unsigned char Flag_StopWaitEnd; /* Flag_StopWaitEnd：原停机等待结束标志。 */

unsigned short countWaitPowerStart10s; /* countWaitPowerStart10s：Timer2 原 主电源等待 10 s计数；实际节拍沿用原判断式。 */
unsigned short cout_Wait; /* cout_Wait：Timer2 原 原控制等待计数；实际节拍沿用原判断式。 */
unsigned short cout200us; /* cout200us：Timer2 原 原名为 200 us 的兼容定时计数；实际节拍沿用原判断式。 */
unsigned short cout1s; /* cout1s：Timer2 原 周期 1 s计数；实际节拍沿用原判断式。 */
unsigned short cout50ms; /* cout50ms：Timer2 原 周期 50 ms计数；实际节拍沿用原判断式。 */
unsigned short cout100ms; /* cout100ms：Timer2 原 周期 100 ms计数；实际节拍沿用原判断式。 */
unsigned short coutRemoteMeasure; /* coutRemoteMeasure：Timer2 原 遥测轮询计数；实际节拍沿用原判断式。 */
unsigned short coutStopWait; /* coutStopWait：Timer2 原 停机等待计数；实际节拍沿用原判断式。 */

unsigned short coutSwitch500ms; /* coutSwitch500ms：Timer2 原 开关操作等待 500 ms计数；实际节拍沿用原判断式。 */
unsigned short coutSwitch1s; /* coutSwitch1s：Timer2 原 开关操作等待 1 s计数；实际节拍沿用原判断式。 */
unsigned short coutPrecharge500ms; /* coutPrecharge500ms：Timer2 原 预充等待 500 ms计数；实际节拍沿用原判断式。 */
unsigned short coutPreStart3s; /* coutPreStart3s：Timer2 原 启动准备等待 3 s计数；实际节拍沿用原判断式。 */
unsigned short coutSVG1s; /* coutSVG1s：Timer2 原 SVG 投入等待 1 s计数；实际节拍沿用原判断式。 */
unsigned short cout20s; /* cout20s：Timer2 原 上电等待 20 s计数；实际节拍沿用原判断式。 */

unsigned char Flag_ComTest; /* Flag_ComTest：旧端口通信测试标志。 */
unsigned short FourPortComZhi; /* FourPortComZhi：旧四端口通信测试状态字。 */


/* 旧版上位机通讯相关变量已废弃，使用VOFA替代 */
/*
unsigned char Flag_Uart;
unsigned char CommNum;
unsigned char CommFunc;
unsigned int StartAddr;
unsigned short ReadLength;
unsigned char CommLength;
unsigned char CommInNum;
unsigned char CommTotalNum;
unsigned char WriteOK;
unsigned char PCuartBUFF[140];
unsigned char ReceiveuartBUFF[11];
*/


//Transformation
float Alpha_Trans,Beta_Trans; /* Alpha_Trans：通用坐标变换 α 轴输出，与输入电压/电流同单位。 */
/* Beta_Trans：通用坐标变换 β 轴输出，与输入电压/电流同单位。 */
float A_Trans,B_Trans,C_Trans; /* A_Trans：通用逆变换 A 相输出，与输入量同单位。 */
/* B_Trans：通用逆变换 B 相输出，与输入量同单位。 */
/* C_Trans：通用逆变换 C 相输出，与输入量同单位。 */
float D_Trans,Q_Trans; /* D_Trans：通用 Park 变换 d 轴输出，与输入量同单位。 */
/* Q_Trans：通用 Park 变换 q 轴输出，与输入量同单位。 */
float Sin_ph_Trans,Cos_ph_Trans,Sin_ph_Trans_NEW,Cos_ph_Trans_NEW; /* Sin_ph_Trans：正弦坐标变换系数。 */
/* Cos_ph_Trans：余弦坐标变换系数。 */
/* Sin_ph_Trans_NEW：正弦坐标变换系数，输出变换使用值。 */
/* Cos_ph_Trans_NEW：余弦坐标变换系数，输出变换使用值。 */

//Fault
ushort FOLatch_A_TAD; /* FOLatch_A_TAD：A 相插件光纤故障的 TAD 锁存值。 */
ushort FOLatch_B_TAD; /* FOLatch_B_TAD：B 相插件光纤故障的 TAD 锁存值。 */
ushort FOLatch_C_TAD; /* FOLatch_C_TAD：C 相插件光纤故障的 TAD 锁存值。 */
ushort FOLatch_INV_TAD; /* FOLatch_INV_TAD：独立逆变桥光纤故障的 TAD 锁存值。 */
ushort PortLatch_TAD; /* PortLatch_TAD：端口 AD 保护故障的 TAD 锁存值。 */
ushort FBLatch_TAD; /* FBLatch_TAD：端口 FB 硬件故障的 TAD 锁存值。 */
ushort ModuleFault_DisposeTimes; /* ModuleFault_DisposeTimes：原模块故障处置已执行次数。 */
unsigned char ClearTimes; /* ClearTimes：原故障清除过程的执行次数。 */
//DPRAM
unsigned char Flag_RemoteControlRegulate; /* Flag_RemoteControlRegulate：DPRAM 通知 ISR 置位、后台消费的遥控/遥调待处理标志。 */
ushort FLAG_NEW_R; /* FLAG_NEW_R：DPRAM 向对端发布的新数据块标识。 */
ushort FLAG_NEW_L; /* FLAG_NEW_L：DPRAM 从对端读取的新数据块标识。 */
ushort SWTable_TotalTime; /* SWTable_TotalTime：旧开关动作表的总定时时间。 */
unsigned char WriteBlock_Index; /* WriteBlock_Index：后台 DPRAM 插件遥测轮询的当前块号。 */

//voltage
float Module_DCbus_Volt; /* Module_DCbus_Volt：当前检查模块的直流母线电压，单位 V。 */   //Module DCbus voltage
float Udc_Ref; /* Udc_Ref：旧直流电压控制参考，单位 V。 */
float Current_Ref; /* Current_Ref：旧直流/交流电流控制参考，单位 A。 */
float Ust_add; /* Ust_add：旧直流电压给定爬升增量，单位 V。 */
uchar Udc_Check_Index; /* Udc_Check_Index：逐模块直流电压巡检的当前索引。 */
uchar Udc_Checked_Num; /* Udc_Checked_Num：已检查模块数量。 */

//regulate
float PLL_Factor; /* PLL_Factor：旧 PLL 系数调整因子。 */
float Volt_Factor; /* Volt_Factor：旧总直流电压环系数调整因子。 */
float PH_Volt_Factor; /* PH_Volt_Factor：旧相间电压平衡环系数调整因子。 */
float Har_2_Factor; /* Har_2_Factor：2 次谐振控制系数调整因子。 */
float Har_6_Factor; /* Har_6_Factor：6 次谐振控制系数调整因子。 */
float Har_12_Factor; /* Har_12_Factor：12 次谐振控制系数调整因子。 */
float Current_Factor; /* Current_Factor：旧正序电流环系数调整因子。 */
float Neg_Current_Factor; /* Neg_Current_Factor：旧负序电流环系数调整因子。 */
float AVolt_Factor; /* AVolt_Factor：旧交流电压相关调整因子。 */
ushort Port_OnState; /* Port_OnState：原端口已投入状态。 */

unsigned char TrigeSignal; /* TrigeSignal：原调试/录波触发信号。 */
unsigned char TestINT; /* TestINT：原中断调试观察标志。 */
float Ref_ph_test; /* Ref_ph_test：原开环测试相角，单位 rad。 */
float Ref_js_test; /* Ref_js_test：原调制测试幅值/计算缓存。 */
ushort rtt; /* rtt：DPRAM 收到通知后的插件 LED 翻转状态。 */
float DAC_BUF[4]; /* DAC_BUF：四路 DA 调试输出缓存，写出时转为 FPGA 原始计数。 */
float TEATabc[4]; /* TEATabc：原三相调试观察缓存，沿用原符号拼写。 */

//edited by shiqi
unsigned short FT3_Samp_Cnt; /* FT3_Samp_Cnt：FT3 上报的采样序号。 */
unsigned short dataPackNum; /* dataPackNum：FT3 数据包数量/控制字字段。 */
/******************************************************************************/
//HVAC Variables
/******************************************************************************/
#ifdef HVAC_online
//PID
XXG_PID_Controller  PLL_PID_Controller,Volt_PID_Controller,Current_d_PID_Controller,Current_q_PID_Controller,PID_Control_buf; /* PLL_PID_Controller：锁相环增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Volt_PID_Controller：直流电压增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Current_d_PID_Controller：d 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Current_q_PID_Controller：q 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
/* PID_Control_buf：通用增量 PI 本拍使用的控制器状态缓存。 */
XXG_PID_Controller  Neg_Current_d_PID_Controller,Neg_Current_q_PID_Controller; /* Neg_Current_d_PID_Controller：负序d 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Neg_Current_q_PID_Controller：负序q 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
XXG_PID_Controller ClusterVolt_A_PID_Controller,ClusterVolt_B_PID_Controller,ClusterVolt_C_PID_Controller; /* ClusterVolt_A_PID_Controller：相间平衡直流电压_A增量 PI 的系数与上一拍输入/输出历史状态。 */
/* ClusterVolt_B_PID_Controller：相间平衡直流电压_B增量 PI 的系数与上一拍输入/输出历史状态。 */
/* ClusterVolt_C_PID_Controller：相间平衡直流电压_C增量 PI 的系数与上一拍输入/输出历史状态。 */
//U Grid
XXG_H_Filter U_d_Grid_H2_20_Filter,U_d_Grid_H2_100_Filter,U_d_Grid_H6_100_Filter,U_d_Grid_H12_100_Filter; /* U_d_Grid_H2_20_Filter：电网 d 轴电压_H2_20滤波器的四个历史输入/输出状态。 */
/* U_d_Grid_H2_100_Filter：电网 d 轴电压_H2_100滤波器的四个历史输入/输出状态。 */
/* U_d_Grid_H6_100_Filter：电网 d 轴电压_H6_100滤波器的四个历史输入/输出状态。 */
/* U_d_Grid_H12_100_Filter：电网 d 轴电压_H12_100滤波器的四个历史输入/输出状态。 */
XXG_H_Filter U_q_Grid_H2_20_Filter,U_q_Grid_H2_100_Filter,U_q_Grid_H6_100_Filter,U_q_Grid_H12_100_Filter; /* U_q_Grid_H2_20_Filter：电网 q 轴电压_H2_20滤波器的四个历史输入/输出状态。 */
/* U_q_Grid_H2_100_Filter：电网 q 轴电压_H2_100滤波器的四个历史输入/输出状态。 */
/* U_q_Grid_H6_100_Filter：电网 q 轴电压_H6_100滤波器的四个历史输入/输出状态。 */
/* U_q_Grid_H12_100_Filter：电网 q 轴电压_H12_100滤波器的四个历史输入/输出状态。 */
//I SVG
XXG_H_Filter I_d_SVG_H2_20_Filter,I_d_SVG_H2_100_Filter,I_d_SVG_H6_100_Filter,I_d_SVG_H12_100_Filter; /* I_d_SVG_H2_20_Filter：端口 d 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
/* I_d_SVG_H2_100_Filter：端口 d 轴电流_H2_100滤波器的四个历史输入/输出状态。 */
/* I_d_SVG_H6_100_Filter：端口 d 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_d_SVG_H12_100_Filter：端口 d 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
XXG_H_Filter I_q_SVG_H2_20_Filter,I_q_SVG_H2_100_Filter,I_q_SVG_H6_100_Filter,I_q_SVG_H12_100_Filter; /* I_q_SVG_H2_20_Filter：端口 q 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
/* I_q_SVG_H2_100_Filter：端口 q 轴电流_H2_100滤波器的四个历史输入/输出状态。 */
/* I_q_SVG_H6_100_Filter：端口 q 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_q_SVG_H12_100_Filter：端口 q 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
//I Load
XXG_H_Filter I_d_Load_H6_100_Filter,I_d_Load_H12_100_Filter,I_d_Load_H2_20_Filter; /* I_d_Load_H6_100_Filter：负载 d 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_d_Load_H12_100_Filter：负载 d 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
/* I_d_Load_H2_20_Filter：负载 d 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
XXG_H_Filter I_q_Load_H6_100_Filter,I_q_Load_H12_100_Filter,I_q_Load_H2_20_Filter; /* I_q_Load_H6_100_Filter：负载 q 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_q_Load_H12_100_Filter：负载 q 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
/* I_q_Load_H2_20_Filter：负载 q 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
//V_Ave
XXG_H_Filter Va_Ave_H2_100_Filter,Vb_Ave_H2_100_Filter,Vc_Ave_H2_100_Filter,Ave_H2_100_Filter,Ave_H6_100_Filter; /* Va_Ave_H2_100_Filter：A 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
/* Vb_Ave_H2_100_Filter：B 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
/* Vc_Ave_H2_100_Filter：C 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
/* Ave_H2_100_Filter：平均量_H2_100滤波器的四个历史输入/输出状态。 */
/* Ave_H6_100_Filter：平均量_H6_100滤波器的四个历史输入/输出状态。 */
XXG_H_Filter I_d_Ref_H2_100_Filter,I_d_Ref_H6_100_Filter; /* I_d_Ref_H2_100_Filter：d 轴电流给定_H2_100滤波器的四个历史输入/输出状态。 */
/* I_d_Ref_H6_100_Filter：d 轴电流给定_H6_100滤波器的四个历史输入/输出状态。 */
//P,Q
XXG_H_Filter P_H2_100_Filter,Q_H2_100_Filter,Q_Load_H2_100_Filter; /* P_H2_100_Filter：P_H2_100滤波器的四个历史输入/输出状态。 */
/* Q_H2_100_Filter：Q_H2_100滤波器的四个历史输入/输出状态。 */
/* Q_Load_H2_100_Filter：负载无功_H2_100滤波器的四个历史输入/输出状态。 */
//buf
XXG_H_Filter H2_100_Filter,H6_100_Filter,H12_100_Filter,H2_20_Filter; /* H2_100_Filter：通用 H2/100 滤波计算历史缓存。 */
/* H6_100_Filter：通用 H6/100 滤波计算历史缓存。 */
/* H12_100_Filter：通用 H12/100 滤波计算历史缓存。 */
/* H2_20_Filter：通用 H2/20 滤波计算历史缓存。 */
//Harmonic Current
XXG_Har_Controller I_d_2_Har_Controller,I_q_2_Har_Controller; /* I_d_2_Har_Controller：I_d_2 次谐振控制器的历史输入/输出状态。 */
/* I_q_2_Har_Controller：I_q_2 次谐振控制器的历史输入/输出状态。 */
XXG_Har_Controller I_d_6_Har_Controller,I_q_6_Har_Controller; /* I_d_6_Har_Controller：I_d_6 次谐振控制器的历史输入/输出状态。 */
/* I_q_6_Har_Controller：I_q_6 次谐振控制器的历史输入/输出状态。 */
XXG_Har_Controller I_d_12_Har_Controller,I_q_12_Har_Controller; /* I_d_12_Har_Controller：I_d_12 次谐振控制器的历史输入/输出状态。 */
/* I_q_12_Har_Controller：I_q_12 次谐振控制器的历史输入/输出状态。 */
//buf
XXG_Har_Controller Har_2_Controller,Har_6_Controller,Har_12_Controller; /* Har_2_Controller：2 次谐振通用历史输入/输出缓存。 */
/* Har_6_Controller：6 次谐振通用历史输入/输出缓存。 */
/* Har_12_Controller：12 次谐振通用历史输入/输出缓存。 */

float Ugrid_d_Positive,Ugrid_d_Negative,Ugrid_hp_d; /* Ugrid_d_Positive：电网电压 d 轴/分量的正序基波值，单位 V。 */
/* Ugrid_d_Negative：电网电压 d 轴/分量的负序基波值，单位 V。 */
/* Ugrid_hp_d：电网电压 hp 轴/分量的d 轴谐波值，单位 V。 */
float Ugrid_q_Positive,Ugrid_q_Negative,Ugrid_hp_q; /* Ugrid_q_Positive：电网电压 q 轴/分量的正序基波值，单位 V。 */
/* Ugrid_q_Negative：电网电压 q 轴/分量的负序基波值，单位 V。 */
/* Ugrid_hp_q：电网电压 hp 轴/分量的q 轴谐波值，单位 V。 */
float Iin_d_Positive,Iin_d_Negative,Iin_hp_d; /* Iin_d_Positive：端口电流 d 轴/分量的正序基波值，单位 A。 */
/* Iin_d_Negative：端口电流 d 轴/分量的负序基波值，单位 A。 */
/* Iin_hp_d：端口电流 hp 轴/分量的d 轴谐波值，单位 A。 */
float Iin_q_Positive,Iin_q_Negative,Iin_hp_q; /* Iin_q_Positive：端口电流 q 轴/分量的正序基波值，单位 A。 */
/* Iin_q_Negative：端口电流 q 轴/分量的负序基波值，单位 A。 */
/* Iin_hp_q：端口电流 hp 轴/分量的q 轴谐波值，单位 A。 */
float Iload_d_Positive,Iload_d_Negative,Iload_har_d; /* Iload_d_Positive：负载电流 d 轴/分量的正序基波值，单位 A。 */
/* Iload_d_Negative：负载电流 d 轴/分量的负序基波值，单位 A。 */
/* Iload_har_d：负载电流 har 轴/分量的d 轴谐波值，单位 A。 */
float Iload_q_Positive,Iload_q_Negative,Iload_har_q; /* Iload_q_Positive：负载电流 q 轴/分量的正序基波值，单位 A。 */
/* Iload_q_Negative：负载电流 q 轴/分量的负序基波值，单位 A。 */
/* Iload_har_q：负载电流 har 轴/分量的q 轴谐波值，单位 A。 */
float Phase_PLL; /* Phase_PLL：旧 HVAC 锁相环电网相角，单位 rad。 */
float Grid_P_Power,Grid_Q_Power,Load_Q_Power,SVG_Q_Power_Ref; /* Grid_P_Power：旧电网有功功率计算值。 */
/* Grid_Q_Power：旧电网无功功率计算值。 */
/* Load_Q_Power：旧负载无功功率计算值。 */
/* SVG_Q_Power_Ref：旧 SVG 无功功率目标。 */
float SVG_Udc_A_Ave,SVG_Udc_B_Ave,SVG_Udc_C_Ave,SVG_Udc_Ave; /* SVG_Udc_A_Ave：A 相在线模块平均直流电压，单位 V。 */
/* SVG_Udc_B_Ave：B 相在线模块平均直流电压，单位 V。 */
/* SVG_Udc_C_Ave：C 相在线模块平均直流电压，单位 V。 */
/* SVG_Udc_Ave：三相单模块直流电压的总平均值，单位 V。 */

float Id_Positive_Ref,Id_Negative_Ref,Id_Positive_Max,Id_Positive_Min; /* Id_Positive_Ref：Id 正序电流给定，单位 A。 */
/* Id_Negative_Ref：Id 负序电流给定，单位 A。 */
/* Id_Positive_Max：Id 正序电流给定上限，单位 A。 */
/* Id_Positive_Min：Id 正序电流给定下限，单位 A。 */
float Iq_Positive_Ref,Iq_Negative_Ref,Iq_Positive_Max,Iq_Positive_Min; /* Iq_Positive_Ref：Iq 正序电流给定，单位 A。 */
/* Iq_Negative_Ref：Iq 负序电流给定，单位 A。 */
/* Iq_Positive_Max：Iq 正序电流给定上限，单位 A。 */
/* Iq_Positive_Min：Iq 正序电流给定下限，单位 A。 */

float Uout_d_Positive,Uout_q_Positive,Uout_d_Negative,Uout_q_Negative,Uout_d_Positive_Max,Uout_d_Positive_Min,Uout_q_Positive_Max,Uout_q_Positive_Min; /* Uout_d_Positive：旧电流环 d_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Positive：旧电流环 q_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_d_Negative：旧电流环 d_负序 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Negative：旧电流环 q_负序 输出电压分量/限幅值，单位 V。 */
/* Uout_d_Positive_Max：旧电流环 d_正序_上限 输出电压分量/限幅值，单位 V。 */
/* Uout_d_Positive_Min：旧电流环 d_正序_下限 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Positive_Max：旧电流环 q_正序_上限 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Positive_Min：旧电流环 q_正序_下限 输出电压分量/限幅值，单位 V。 */
float Uout_A_Positive,Uout_B_Positive,Uout_C_Positive; /* Uout_A_Positive：旧电流环 A_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_B_Positive：旧电流环 B_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_C_Positive：旧电流环 C_正序 输出电压分量/限幅值，单位 V。 */

float Har_5_RMS,Har_7_RMS,Har_11_RMS; /* Har_5_RMS：5 次谐波电流有效值，单位 A。 */
/* Har_7_RMS：7 次谐波电流有效值，单位 A。 */
/* Har_11_RMS：11 次谐波电流有效值，单位 A。 */
float Har_5_PH,Har_7_PH,Har_11_PH; /* Har_5_PH：5 次谐波相位给定，单位 rad。 */
/* Har_7_PH：7 次谐波相位给定，单位 rad。 */
/* Har_11_PH：11 次谐波相位给定，单位 rad。 */
float Har_5OR7_PH0,Har_11_PH0; /* Har_5OR7_PH0：原 5OR7 次谐波初始相位，单位 rad。 */
/* Har_11_PH0：原 11 次谐波初始相位，单位 rad。 */


//controller
float PLL_PID_Num_1; /* PLL_PID_Num_1：锁相环增量 PI 本拍误差系数。 */
float PLL_PID_Num_0; /* PLL_PID_Num_0：锁相环增量 PI 上一拍误差系数。 */

    //voltage loop
float Volt_PID_Num_1; /* Volt_PID_Num_1：电压增量 PI 本拍误差系数。 */
float Volt_PID_Num_0; /* Volt_PID_Num_0：电压增量 PI 上一拍误差系数。 */

float PH_Volt_PID_Num_1; /* PH_Volt_PID_Num_1：相间电压增量 PI 本拍误差系数。 */
float PH_Volt_PID_Num_0; /* PH_Volt_PID_Num_0：相间电压增量 PI 上一拍误差系数。 */

    //Current loop
float Har_2_Controller_Num_2; /* Har_2_Controller_Num_2：2 次谐振控制器差分式分子第 2 项系数。 */
float Har_2_Controller_Num_1; /* Har_2_Controller_Num_1：2 次谐振控制器差分式分子第 1 项系数。 */
float Har_2_Controller_Num_0; /* Har_2_Controller_Num_0：2 次谐振控制器差分式分子第 0 项系数。 */
float Har_2_Controller_Den_1; /* Har_2_Controller_Den_1：2 次谐振控制器差分式分母第 1 项系数。 */

float Har_6_Controller_Num_2; /* Har_6_Controller_Num_2：6 次谐振控制器差分式分子第 2 项系数。 */
float Har_6_Controller_Num_1; /* Har_6_Controller_Num_1：6 次谐振控制器差分式分子第 1 项系数。 */
float Har_6_Controller_Num_0; /* Har_6_Controller_Num_0：6 次谐振控制器差分式分子第 0 项系数。 */
float Har_6_Controller_Den_1; /* Har_6_Controller_Den_1：6 次谐振控制器差分式分母第 1 项系数。 */

float Har_12_Controller_Num_2; /* Har_12_Controller_Num_2：12 次谐振控制器差分式分子第 2 项系数。 */
float Har_12_Controller_Num_1; /* Har_12_Controller_Num_1：12 次谐振控制器差分式分子第 1 项系数。 */
float Har_12_Controller_Num_0; /* Har_12_Controller_Num_0：12 次谐振控制器差分式分子第 0 项系数。 */
float Har_12_Controller_Den_1; /* Har_12_Controller_Den_1：12 次谐振控制器差分式分母第 1 项系数。 */

float Current_PID_Num_1; /* Current_PID_Num_1：电流增量 PI 本拍误差系数。 */
float Current_PID_Num_0; /* Current_PID_Num_0：电流增量 PI 上一拍误差系数。 */
float Current_PID_Num_1_Pres; /* Current_PID_Num_1_Pres：电流增量 PI 本拍误差系数的预设值。 */
float Current_PID_Num_0_Pres; /* Current_PID_Num_0_Pres：电流增量 PI 上一拍误差系数的预设值。 */

float Neg_Current_PID_Num_1; /* Neg_Current_PID_Num_1：负序电流增量 PI 本拍误差系数。 */
float Neg_Current_PID_Num_0; /* Neg_Current_PID_Num_0：负序电流增量 PI 上一拍误差系数。 */

/////////////////////////////////////////

float HA_Port_Volt_A; /* HA_Port_Volt_A：高压交流端口 A 相/相间原采样电压，单位 V。 */
float HA_Port_Volt_B; /* HA_Port_Volt_B：高压交流端口 B 相/相间原采样电压，单位 V。 */
float HA_Port_Volt_C; /* HA_Port_Volt_C：高压交流端口 C 相/相间原采样电压，单位 V。 */

float HA_Port_Volt_AB; /* HA_Port_Volt_AB：高压交流端口 AB 相/相间原采样电压，单位 V。 */
float HA_Port_Volt_BC; /* HA_Port_Volt_BC：高压交流端口 BC 相/相间原采样电压，单位 V。 */
float HA_Port_Volt_CA; /* HA_Port_Volt_CA：高压交流端口 CA 相/相间原采样电压，单位 V。 */

float HA_Port_Current_A; /* HA_Port_Current_A：高压交流端口 A 相/相间端口电流，单位 A。 */
float HA_Port_Current_B; /* HA_Port_Current_B：高压交流端口 B 相/相间端口电流，单位 A。 */
float HA_Port_Current_C; /* HA_Port_Current_C：高压交流端口 C 相/相间端口电流，单位 A。 */

float HA_Grid_Current_A; /* HA_Grid_Current_A：高压交流端口 A 相/相间电网电流，单位 A。 */
float HA_Grid_Current_B; /* HA_Grid_Current_B：高压交流端口 B 相/相间电网电流，单位 A。 */
float HA_Grid_Current_C; /* HA_Grid_Current_C：高压交流端口 C 相/相间电网电流，单位 A。 */


float HA_Port_Volt_AB_LATCH; /* HA_Port_Volt_AB_LATCH：高压交流端口 AB 相/相间原采样电压，单位 V；锁存采样值。 */
float HA_Port_Volt_BC_LATCH; /* HA_Port_Volt_BC_LATCH：高压交流端口 BC 相/相间原采样电压，单位 V；锁存采样值。 */
float HA_Port_Volt_CA_LATCH; /* HA_Port_Volt_CA_LATCH：高压交流端口 CA 相/相间原采样电压，单位 V；锁存采样值。 */

float HA_Port_Current_A_LATCH; /* HA_Port_Current_A_LATCH：高压交流端口 A 相/相间端口电流，单位 A；锁存采样值。 */
float HA_Port_Current_B_LATCH; /* HA_Port_Current_B_LATCH：高压交流端口 B 相/相间端口电流，单位 A；锁存采样值。 */
float HA_Port_Current_C_LATCH; /* HA_Port_Current_C_LATCH：高压交流端口 C 相/相间端口电流，单位 A；锁存采样值。 */

float HA_Grid_Current_A_LATCH; /* HA_Grid_Current_A_LATCH：高压交流端口 A 相/相间电网电流，单位 A；锁存采样值。 */
float HA_Grid_Current_B_LATCH; /* HA_Grid_Current_B_LATCH：高压交流端口 B 相/相间电网电流，单位 A；锁存采样值。 */
float HA_Grid_Current_C_LATCH; /* HA_Grid_Current_C_LATCH：高压交流端口 C 相/相间电网电流，单位 A；锁存采样值。 */

float HA_PortUanRms_LF_1; /* HA_PortUanRms_LF_1：高压交流端口 PortUanRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_PortUanRms_LF_2; /* HA_PortUanRms_LF_2：高压交流端口 PortUanRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
float HA_PortUbnRms_LF_1; /* HA_PortUbnRms_LF_1：高压交流端口 PortUbnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_PortUbnRms_LF_2; /* HA_PortUbnRms_LF_2：高压交流端口 PortUbnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
float HA_PortUcnRms_LF_1; /* HA_PortUcnRms_LF_1：高压交流端口 PortUcnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_PortUcnRms_LF_2; /* HA_PortUcnRms_LF_2：高压交流端口 PortUcnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

float HA_PortIaRms_LF_1; /* HA_PortIaRms_LF_1：高压交流端口 PortIaRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_PortIaRms_LF_2; /* HA_PortIaRms_LF_2：高压交流端口 PortIaRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
float HA_PortIbRms_LF_1; /* HA_PortIbRms_LF_1：高压交流端口 PortIbRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_PortIbRms_LF_2; /* HA_PortIbRms_LF_2：高压交流端口 PortIbRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
float HA_PortIcRms_LF_1; /* HA_PortIcRms_LF_1：高压交流端口 PortIcRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_PortIcRms_LF_2; /* HA_PortIcRms_LF_2：高压交流端口 PortIcRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

float HA_GridIaRms_LF_1; /* HA_GridIaRms_LF_1：高压交流端口 GridIaRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_GridIaRms_LF_2; /* HA_GridIaRms_LF_2：高压交流端口 GridIaRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
float HA_GridIbRms_LF_1; /* HA_GridIbRms_LF_1：高压交流端口 GridIbRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_GridIbRms_LF_2; /* HA_GridIbRms_LF_2：高压交流端口 GridIbRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
float HA_GridIcRms_LF_1; /* HA_GridIcRms_LF_1：高压交流端口 GridIcRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
float HA_GridIcRms_LF_2; /* HA_GridIcRms_LF_2：高压交流端口 GridIcRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

//edited by shiqi
float HA_Port_Volt_A_last, HA_Port_Volt_B_last, HA_Port_Volt_C_last; /* HA_Port_Volt_A_last：高压交流端口 A 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
/* HA_Port_Volt_B_last：高压交流端口 B 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
/* HA_Port_Volt_C_last：高压交流端口 C 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
ushort Samplenoise; /* Samplenoise：原交流电压采样噪声判断状态。 */
float VA_Control, VB_Control, VC_Control; /* VA_Control：A 相原输出电压控制给定，单位 V。 */
/* VB_Control：B 相原输出电压控制给定，单位 V。 */
/* VC_Control：C 相原输出电压控制给定，单位 V。 */
float Iin_d_Positive_last, Iin_q_Positive_last; /* Iin_d_Positive_last：端口电流 d 轴/分量的正序基波值，单位 A；上一拍噪声比较缓存。 */
/* Iin_q_Positive_last：端口电流 q 轴/分量的正序基波值，单位 A；上一拍噪声比较缓存。 */
ushort Samplenoise_Idq; /* Samplenoise_Idq：原 dq 电流采样噪声判断状态。 */
float SVG_Udc_Ave_temp, SVG_Udc_Ave_temp_last; /* SVG_Udc_Ave_temp：旧直流平均电压噪声处理的本拍缓存，单位 V。 */
/* SVG_Udc_Ave_temp_last：旧直流平均电压噪声处理的上一拍缓存，单位 V。 */
ushort Samplenoise_udc; /* Samplenoise_udc：原直流电压采样噪声判断状态。 */
ushort GNDSwitch, ISOSwitch; /* GNDSwitch：原接地开关状态。 */
/* ISOSwitch：原隔离开关状态。 */
ushort SM_write_flag; /* SM_write_flag：旧维护存储写入状态标志。 */
ushort SM_write_count; /* SM_write_count：旧维护存储写入的累计计数。 */
ushort ISO_counter_on, ISO_counter_off; /* ISO_counter_on：原隔离开关闭合确认计数。 */
/* ISO_counter_off：原隔离开关断开确认计数。 */
//end

float HA_PortP_LF_1; /* HA_PortP_LF_1：高压交流端口有功低通滤波第 1 个历史状态。 */
float HA_PortP_LF_2; /* HA_PortP_LF_2：高压交流端口有功低通滤波第 2 个历史状态。 */
float HA_PortQ_LF_1; /* HA_PortQ_LF_1：高压交流端口无功低通滤波第 1 个历史状态。 */
float HA_PortQ_LF_2; /* HA_PortQ_LF_2：高压交流端口无功低通滤波第 2 个历史状态。 */

float LPF_5KHZ_X_Volt_A_1; /* LPF_5KHZ_X_Volt_A_1：原 5KHZ 低通滤波 X 的上一拍输入状态。 */
float LPF_5KHZ_Y_Volt_A_1; /* LPF_5KHZ_Y_Volt_A_1：原 5KHZ 低通滤波 Y 的上一拍输出状态。 */
float LPF_5KHZ_X_Volt_B_1; /* LPF_5KHZ_X_Volt_B_1：原 5KHZ 低通滤波 X 的上一拍输入状态。 */
float LPF_5KHZ_Y_Volt_B_1; /* LPF_5KHZ_Y_Volt_B_1：原 5KHZ 低通滤波 Y 的上一拍输出状态。 */

float LPF_20HZ_X_Ud_1; /* LPF_20HZ_X_Ud_1：原 20HZ 低通滤波 X 的上一拍输入状态。 */
float LPF_20HZ_Y_Ud_1; /* LPF_20HZ_Y_Ud_1：原 20HZ 低通滤波 Y 的上一拍输出状态。 */
float LPF_20HZ_X_Uq_1; /* LPF_20HZ_X_Uq_1：原 20HZ 低通滤波 X 的上一拍输入状态。 */
float LPF_20HZ_Y_Uq_1; /* LPF_20HZ_Y_Uq_1：原 20HZ 低通滤波 Y 的上一拍输出状态。 */

ushort HA_DC_Log_A; /* HA_DC_Log_A：高压交流端口 A 相/相间直流电压排序方向字。 */
ushort HA_DC_Log_B; /* HA_DC_Log_B：高压交流端口 B 相/相间直流电压排序方向字。 */
ushort HA_DC_Log_C; /* HA_DC_Log_C：高压交流端口 C 相/相间直流电压排序方向字。 */

ushort HA_IG_Log_A; /* HA_IG_Log_A：高压交流端口 A 相/相间电流方向控制字。 */
ushort HA_IG_Log_B; /* HA_IG_Log_B：高压交流端口 B 相/相间电流方向控制字。 */
ushort HA_IG_Log_C; /* HA_IG_Log_C：高压交流端口 C 相/相间电流方向控制字。 */

float HA_TotalVDC_A; /* HA_TotalVDC_A：高压交流端口 A 相/相间在线模块直流电压总和，单位 V。 */
float HA_TotalVDC_B; /* HA_TotalVDC_B：高压交流端口 B 相/相间在线模块直流电压总和，单位 V。 */
float HA_TotalVDC_C; /* HA_TotalVDC_C：高压交流端口 C 相/相间在线模块直流电压总和，单位 V。 */

float HVAC_RealVoltage_Sum[3]; /* HVAC_RealVoltage_Sum：HVAC 实时量窗口的电压累加缓存，供原有效值计算。 */
float HVAC_RealCurrent_Sum[3]; /* HVAC_RealCurrent_Sum：HVAC 实时量窗口的端口电流累加缓存，供原有效值计算。 */
float HVAC_RealGridCurrent_Sum[3]; /* HVAC_RealGridCurrent_Sum：HVAC 实时量窗口的电网电流累加缓存，供原有效值计算。 */
uchar HVAC_RealParameter_N; /* HVAC_RealParameter_N：HVAC 实时量窗口的采样采样窗口计数。 */
float HA_ReferenceDEF_A,HA_ReferenceDEF_B,HA_ReferenceDEF_C; /* HA_ReferenceDEF_A：高压交流端口 A 相/相间按在线模块数得到的 FPGA 调制参考基值。 */
/* HA_ReferenceDEF_B：高压交流端口 B 相/相间按在线模块数得到的 FPGA 调制参考基值。 */
/* HA_ReferenceDEF_C：高压交流端口 C 相/相间按在线模块数得到的 FPGA 调制参考基值。 */

ushort NHA_module_A; /* NHA_module_A：HA 端口 A 相在线模块数量，用于采样平均及调制尺度。 */
ushort NHA_module_B; /* NHA_module_B：HA 端口 B 相在线模块数量，用于采样平均及调制尺度。 */
ushort NHA_module_C; /* NHA_module_C：HA 端口 C 相在线模块数量，用于采样平均及调制尺度。 */
ushort HA_PM_A1; /* HA_PM_A1：HA 端口 A1 组在线模块位图，每位对应一个模块。 */
ushort HA_PM_A2; /* HA_PM_A2：HA 端口 A2 组在线模块位图，每位对应一个模块。 */
ushort HA_PM_B1; /* HA_PM_B1：HA 端口 B1 组在线模块位图，每位对应一个模块。 */
ushort HA_PM_B2; /* HA_PM_B2：HA 端口 B2 组在线模块位图，每位对应一个模块。 */
ushort HA_PM_C1; /* HA_PM_C1：HA 端口 C1 组在线模块位图，每位对应一个模块。 */
ushort HA_PM_C2; /* HA_PM_C2：HA 端口 C2 组在线模块位图，每位对应一个模块。 */

ushort HA_A_LV_Precharge; /* HA_A_LV_Precharge：高压交流端口 预充欠压判断标志。 */
ushort HA_B_LV_Precharge; /* HA_B_LV_Precharge：高压交流端口 预充欠压判断标志。 */
ushort HA_C_LV_Precharge; /* HA_C_LV_Precharge：高压交流端口 预充欠压判断标志。 */

ushort HA_A_HV_Protect; /* HA_A_HV_Protect：高压交流端口 原直流高压判断标志。 */
ushort HA_B_HV_Protect; /* HA_B_HV_Protect：高压交流端口 原直流高压判断标志。 */
ushort HA_C_HV_Protect; /* HA_C_HV_Protect：高压交流端口 原直流高压判断标志。 */

float HA_L_Voltage_Precharge; /* HA_L_Voltage_Precharge：原 HVAC 预充欠压判断阈值，单位 V。 */
float HA_H_Voltage; /* HA_H_Voltage：原 HVAC 直流高压判断阈值，单位 V。 */

float Ust_HVAC_Port; /* Ust_HVAC_Port：旧 HVAC 单模块直流电压运行给定，单位 V。 */
float UstOK_HVAC_Port; /* UstOK_HVAC_Port：旧 HVAC 启动电压达标值，单位 V。 */

float OverVolt_AlarmLevel2; /* OverVolt_AlarmLevel2：旧模块二级过压报警阈值，单位 V。 */
float VoltLevel_1; /* VoltLevel_1：旧电压判断第一级阈值。 */
float VoltLevel_2; /* VoltLevel_2：旧电压判断第二级阈值。 */

//HVAC abnormal
unsigned char Port_Abnormal; /* Port_Abnormal：旧端口异常状态标志。 */
unsigned char Port_LV2; /* Port_LV2：旧端口第二级欠压状态。 */
unsigned char Flag_V_L; /* Flag_V_L：旧欠压阶段标志。 */
unsigned char Flag_V_L1_END; /* Flag_V_L1_END：旧一级欠压处理完成标志。 */
unsigned char Flag_V_L2_END; /* Flag_V_L2_END：旧二级欠压处理完成标志。 */
unsigned char Flag_V_L2_MID; /* Flag_V_L2_MID：旧二级欠压处理中间阶段标志。 */
unsigned char Flag_SlopeTime; /* Flag_SlopeTime：旧电压给定斜坡计时使能。 */
ushort SlopeTime_N; /* SlopeTime_N：旧电压斜坡的计时长度/计数。 */
ushort Cout_V_L; /* Cout_V_L：旧欠压处理计时计数。 */

//range
float HA_Module_RefUdc; /* HA_Module_RefUdc：旧 HVAC 单模块直流电压目标，单位 V。 */
float Ud_st_Max; /* Ud_st_Max：Ud 原控制给定上限，电压 V、电流 A。 */
float Ud_st_Min; /* Ud_st_Min：Ud 原控制给定下限，电压 V、电流 A。 */
float Uq_st_Max; /* Uq_st_Max：Uq 原控制给定上限，电压 V、电流 A。 */
float Uq_st_Min; /* Uq_st_Min：Uq 原控制给定下限，电压 V、电流 A。 */
float Idst_Max; /* Idst_Max：Idst 原控制给定上限，电压 V、电流 A。 */
float Idst_Min; /* Idst_Min：Idst 原控制给定下限，电压 V、电流 A。 */
float Iqst_Max; /* Iqst_Max：Iqst 原控制给定上限，电压 V、电流 A。 */
float Iqst_Min; /* Iqst_Min：Iqst 原控制给定下限，电压 V、电流 A。 */
float Ust_add; /* Ust_add：旧直流电压给定爬升增量，单位 V。 */

uchar Udc_Check_Index; /* Udc_Check_Index：逐模块直流电压巡检的当前索引。 */
uchar Udc_Checked_Num; /* Udc_Checked_Num：已检查模块数量。 */

//PLL
float Phase_PLL_First; /* Phase_PLL_First：旧 PLL 首拍相角缓存，单位 rad。 */
float PLL_Phase_Com; /* PLL_Phase_Com：旧电流变换相位补偿，单位 rad。 */
float PLL_Phase_Com_NEW; /* PLL_Phase_Com_NEW：旧输出变换相位补偿，单位 rad。 */
float PhaseCom; /* PhaseCom：旧附加相位补偿量，单位 rad。 */
float Sin_ph_Trans_Com; /* Sin_ph_Trans_Com：正弦坐标变换系数，含原相位补偿。 */
float Cos_ph_Trans_Com; /* Cos_ph_Trans_Com：余弦坐标变换系数，含原相位补偿。 */
float Sin_ph_Trans_Com_NEW; /* Sin_ph_Trans_Com_NEW：正弦坐标变换系数，含原相位补偿，输出变换使用值。 */
float Cos_ph_Trans_Com_NEW; /* Cos_ph_Trans_Com_NEW：余弦坐标变换系数，含原相位补偿，输出变换使用值。 */
float Sin_ph_Trans_next; /* Sin_ph_Trans_next：正弦坐标变换系数，下一拍预测值。 */
float Cos_ph_Trans_next; /* Cos_ph_Trans_next：余弦坐标变换系数，下一拍预测值。 */

float Ugrid_Max; /* Ugrid_Max：旧控制使用的电网电压幅值上限/估计值，单位 V。 */

////////////////////////////////////
//Reference
float HA_Module_RefVolt_A; /* HA_Module_RefVolt_A：高压交流端口 A 相/相间模块电压调制给定，单位 V。 */
float HA_Module_RefVolt_B; /* HA_Module_RefVolt_B：高压交流端口 B 相/相间模块电压调制给定，单位 V。 */
float HA_Module_RefVolt_C; /* HA_Module_RefVolt_C：高压交流端口 C 相/相间模块电压调制给定，单位 V。 */

ushort HA_DABDuty_A; /* HA_DABDuty_A：高压交流端口 A 相/相间DAB 占空比 FPGA 计数。 */
ushort HA_DABDuty_B; /* HA_DABDuty_B：高压交流端口 B 相/相间DAB 占空比 FPGA 计数。 */
ushort HA_DABDuty_C; /* HA_DABDuty_C：高压交流端口 C 相/相间DAB 占空比 FPGA 计数。 */
ushort HA_DAB_PluseStart_A; /* HA_DAB_PluseStart_A：高压交流端口 A 相/相间DAB 脉冲起始 FPGA 计数。 */
ushort HA_DAB_PluseStart_B; /* HA_DAB_PluseStart_B：高压交流端口 B 相/相间DAB 脉冲起始 FPGA 计数。 */
ushort HA_DAB_PluseStart_C; /* HA_DAB_PluseStart_C：高压交流端口 C 相/相间DAB 脉冲起始 FPGA 计数。 */
ushort HA_DAB_T; /* HA_DAB_T：高压交流端口 DAB 周期 FPGA 计数。 */

////////////////////////////////////


unsigned char Add_3th; /* Add_3th：旧三次谐波注入使能标志。 */
unsigned char Method_3th; /* Method_3th：旧三次谐波注入算法选择。 */
unsigned char flag_Standard; /* flag_Standard：旧调制标准化方式选择标志。 */
float StandardValue; /* StandardValue：旧调制参考标准化基值。 */
unsigned char DAB_ONLY; /* DAB_ONLY：旧仅运行 DAB 的测试选择标志。 */

//edited by shiqi
ushort counter_OC_flag; /* counter_OC_flag：原一级过流连续超限样本计数。 */
ushort counter_OC_time; /* counter_OC_time：原一级过流保护确认计时计数。 */
ushort counter_OC_flag1; /* counter_OC_flag1：原二级过流连续超限样本计数。 */
ushort counter_OC_time1; /* counter_OC_time1：原二级过流保护确认计时计数。 */
ushort OC_OV_LV_PortLatch_TAD; /* OC_OV_LV_PortLatch_TAD：原软件过流/过压/欠压的 TAD 故障锁存值。 */
ushort HVAC_OC_Fault; /* HVAC_OC_Fault：原 HVAC 软件过流故障确认标志。 */
ushort OC_level_SET = OC_level_SET_DEFAULT; /* OC_level_SET：原一级过流阈值，沿用采样计数/系数约定。 */
ushort OC_time_SET = OC_time_SET_DEFAULT; /* OC_time_SET：原一级过流确认计数阈值，单位 TAD。 */
ushort OC_level_SET_L2 = OC_level_SET_L2_DEFAULT; /* OC_level_SET_L2：原二级过流阈值，沿用采样计数/系数约定。 */
ushort OC_time_SET_L2 = OC_time_SET_L2_DEFAULT; /* OC_time_SET_L2：原二级过流确认计数阈值，单位 TAD。 */
ushort counter_OV_flag; /* counter_OV_flag：原一级过压连续超限样本计数。 */
ushort counter_OV_time; /* counter_OV_time：原一级过压保护确认计时计数。 */
ushort HVAC_OV_Fault; /* HVAC_OV_Fault：原 HVAC 软件过压故障确认标志。 */
ushort OV_level_SET = OV_level_SET_DEFAULT; /* OV_level_SET：原直流过压阈值，沿用采样计数/系数约定。 */
ushort OV_time_SET = OV_time_SET_DEFAULT; /* OV_time_SET：原过压确认计数阈值，单位 TAD。 */
ushort Contactor_err; /* Contactor_err：原接触器动作异常标志。 */
ushort Breaker_err; /* Breaker_err：原断路器动作异常标志。 */

#endif


/******************************************************************************/
//HVDC Variables
/******************************************************************************/


/******************************************************************************/
//LVDC Variables
/******************************************************************************/

/******************************************************************************/
//LVAC Variables
/******************************************************************************/


/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 按原项目初始化控制系数、模块位图、保护阈值及运行变量
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Initial_Variable(void);
/*==================================================================
 * 函数功能: 复位滤波器、PI、遥控、故障以及原运行流程的历史状态
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Variable_RST(void);
/*==================================================================
 * 函数功能: 依次检查各相在线模块直流电压，更新预充及高压判断标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Udc_check(void);
/*==================================================================
 * 函数功能: 执行板级初始化和上电等待，然后持续调度迁移后的后台服务
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 声明为 int；初始化完成后进入永久主循环，正常运行不返回
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
int main(void);
/* 旧版上位机通讯函数已废弃，使用VOFA替代 */
/*
void Uart2_send(unsigned char PCcomm);
void UARTReceiveDispose(void);
*/
/*==================================================================
 * 函数功能: 计算旧 UART 兼容帧的 CRC16，保持原多项式及交换字节规则
 * 输入参数: ba：CRC 输入缓冲区首地址，调用者提供有效帧字节
 *           size：输入缓存有效长度
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 原实现生成的 16 位 CRC 值
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
unsigned short CRC16(unsigned char *ba, int size);
/*==================================================================
 * 函数功能: 保留原本地按键扫描入口及按键状态处理
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void keyscan(void);
/*==================================================================
 * 函数功能: 应答 Timer2 中断并更新原软件定时、状态及保护服务
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Timer2Isr(void);
/*==================================================================
 * 函数功能: 保留原 UART2 ISR 符号；调试通信主流程不注册此中断
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void UART2Isr(void);
/*==================================================================
 * 函数功能: 应答 FPGA TAD 外部中断并按原顺序完成 10 kHz 采样、保护及控制
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void FPGAIsr(void);
/*==================================================================
 * 函数功能: 应答 FPGA 硬件故障中断并执行原快速故障处置
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void FAULTIsr(void);
/*==================================================================
 * 函数功能: 应答可选 DPRAM GPIO 通知并置后台读取标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void DPRAMIsr(void);

#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*                       Variable initialization                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按原项目初始化控制系数、模块位图、保护阈值及运行变量
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Initial_Variable(void)
{
    uchar i; /* i：本次数组、参数或模块遍历的索引。 */
    ushort InitialTemp; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */
    float InitialTempFloat; /* InitialTempFloat：原控制系数和给定初始化的浮点计算缓存。 */

    //edited by shiqi
    FT3_Samp_Cnt = 0;
    dataPackNum = 1;
    HA_Port_Volt_A_last = 0;
    HA_Port_Volt_B_last = 0;
    HA_Port_Volt_C_last = 0;
    Iin_d_Positive_last = 0;
    Iin_q_Positive_last = 0;
    SVG_Udc_Ave_temp_last = 0;
    Samplenoise = 1;
    Samplenoise_Idq = 1;
    Samplenoise_udc = 1;
    Contactor_err = 0;
    Breaker_err = 0;
    SM_write_flag = 0;
    SM_write_count = 0;
    ISO_counter_on = 0;
    ISO_counter_off = 0;

#ifdef COM_TEST_PluginUnit
    WrongTimes_LEFT=0;
    WrongTimes_RIGHT=0;
    LastWrong_LEFT=0.0;
    LastWrong_RIGHT=0.0;

    WrongTimes_LEFT_A=0;
    WrongTimes_RIGHT_A=0;
    LastWrong_LEFT_A=0.0;
    LastWrong_RIGHT_A=0.0;

    WrongTimes_Reference_A1=0;
    WrongTimes_Reference_A2=0;
    LastWrong_Reference_A1=0.0;
    LastWrong_Reference_A2=0.0;

    WrongTimes_Reference_C1=0;
    WrongTimes_Reference_C2=0;
    LastWrong_Reference_C1=0.0;
    LastWrong_Reference_C2=0.0;

    WrongTimes_ControlReference_A=0;
    WrongTimes_ControlReference_C=0;
    LastWrong_ControlReference_A=0.0;
    LastWrong_ControlReference_C=0.0;

#endif

    HA_IG_Log_A=0;
    HA_IG_Log_B=0;
    HA_IG_Log_C=0;

    DAB_ONLY=0;
    Phase_PLL_First=0.0;
    Add_3th=0;
    Method_3th=1;
    Ref_ph_test=0.0;
    Ref_js_test=0.0;

    Flag_PC=0;
    Flag_FaultOff=0;
    Flag_DC_VoltSource=0;
    flag_Standard=1;
    flag_INV=0;
    flag_FaultClose=0;
    flag_SVGOPEN=0;
    flag_DABOPEN=0;

    flag_SendDABRef_First=0;
    flag_SendSVGRef_First=0;

    Flag_Key=0;
    Flag_temp=0;
    Flag_RemoteControlRegulate=0;

    Flag_1s=0;
    Flag_50ms=0;
    Flag_100ms=0;
    Flag_RemoteMeasure=0;
    Flag_Clear_0=0;
    Flag_Clear_1=0;
    Flag_StopWait=0;
    Flag_StopWaitEnd=0;

    cout200us=0;
    cout1s=0;
    cout20s=0;
    cout50ms=0;
    cout100ms=0;
    coutRemoteMeasure=0;
    coutStopWait=0;

    Flag_20s=0;
    Port_LV2=0;
    Port_Abnormal=0;
    Flag_V_L=0;
    Flag_V_L1_END=0;
    Flag_V_L2_END=0;
    Flag_V_L2_MID=0;
    Flag_SlopeTime=0;
    SlopeTime_N=0;
    Cout_V_L=0;

    Flag_ComTest=0;
    FourPortComZhi=0;

    /* 旧版上位机通讯变量初始化已废弃 */
    /*
    Flag_Uart=0;
    CommNum=0;
    CommFunc=0;
    */
    StartAddr=0;
    ReadLength=0;
    CommLength=0;
    CommInNum=0;
    CommTotalNum=0;
    WriteOK=0;
    ClearTimes=1;

    PWMPluseEnable_Initial();
    TX_EN_Control_Write();
    Module_PM_CTL.PM_CTL16=0x0F00;
    Module_Control_Write();
    Port_BKU_Send(1);
    Port_FB_EN=0;
    Port_FB_EN_Set();
//  FaultLatch_Clear(0,1);

    WriteBlock_Index=WriteBlock_PluginUnit_2;

    Port_MainStatus.MainStatus16=0x003f;
    Port_MainStatus.Bbits.P_M_Mode=0x03;
    Port_MainStatus.Bbits.P_S1_Mode=0x00;
    Port_MainStatus.Bbits.P_S2_Mode=0x00;

    Port_Main_WorkMode=3;
    Port_STATCOM_WorkMode=0;
    Port_APF_WorkMode=0;
    Port_Outer_Elec=0;
    Port_Sys_Stop=0;
    Port_State=PortStatusNum_Nonvaild;
    Port_RunStatus=PortStatusNum_Nonvaild;
    ControlFLAG.ControlFLAG16=0;

    SW.SwitchFLAG16=0x0040;  //EXT_CB_ON_EN=1
    SW.Bbits.S_EXT_CB_ON_EN=1;

    PortRunFLAG.PortRunFLAG16=0x007c; //all port don't need shutdown
    Port_RealStatus.Bbits.S_Mode=0;  //Auto Run

    Port_RealFault.PORT_FAULT16=0;
    FaultTypeFLAG.FaultTypeFLAG16=0;
    FaultDisposeFLAG.FaultDisposeFLAG16=0;

#ifdef EEPROM_FirstW
    DSP_2_EEPROM_Page0();
#endif

#ifdef EEPROM_ON
    EEPROM_Page0_2_DSP();
#endif

    Port_StateUpdate_Write();


    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_RunMode=1;  //运行
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_CtlMode=0;  //手动

    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16=0x7000;


    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16=0x3F7F;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16=0x3F7F;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16=0x3F7F;


    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16=0x4441;

    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=680;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET=500;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=0;


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
     * With the current fixed HVAC values:
     *   A1/B1/C1 0x7F -> 7 valid modules
     *   A2/B2/C2 0x3F -> 6 valid modules
     * so each phase has NUM=13. The popcount form is kept so runtime
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

//range
#ifdef SVG_10KV
    Ust_HVAC_Port=900.0;                     //端口母线有电
    HA_L_Voltage_Precharge=350.0;    //预充直流母线平均电压
    HA_H_Voltage=780.0;

    OverVolt_AlarmLevel2=900.0;
    VoltLevel_1=3000.0;    //0.3Urated
    VoltLevel_2=9000.0;    //0.9Urated

    StandardValue=26.0;
    //HVAC 控制算法里的电压标幺/缩放系数
    //HVAC_Strategy.C 里会把 Alpha_Trans、Beta_Trans、Ugrid_d/q 等除以或乘以
    //StandardValue，用于 10kV 和 380V 两种工程配置共用同一套控制算法
#endif

#ifdef SVG_380V
    Ust_HVAC_Port=200.0;                     //端口母线有电
    HA_L_Voltage_Precharge=80.0;    //预充直流母线平均电压
    HA_H_Voltage=780.0

    OverVolt_AlarmLevel2=900.0;
    VoltLevel_1=114.0;    //0.3Urated
    VoltLevel_2=342.0;    //0.9Urated

    StandardValue=1.0;
#endif
    Udc_Check_Index=0;
    Udc_Checked_Num=0;

    HA_A_LV_Precharge=0;
    HA_B_LV_Precharge=0;
    HA_C_LV_Precharge=0;

    HA_A_HV_Protect=0;
    HA_B_HV_Protect=0;
    HA_C_HV_Protect=0;

    Ust_add=10.0;
    HA_Module_RefUdc=680.0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=680;
    UstOK_HVAC_Port=HA_Module_RefUdc;



    if(NHA_module_A==0)
    {
        Ud_st_Max=10.0;
        Ud_st_Min=-10.0;
        Uq_st_Max=10.0;
        Uq_st_Min=-10.0;
    }
    else
    {
        InitialTempFloat=(float)NHA_module_A;
        InitialTempFloat=InitialTempFloat*1.1;
        InitialTempFloat=InitialTempFloat*HA_Module_RefUdc;

        Ud_st_Max=InitialTempFloat;
        Ud_st_Min=InitialTempFloat*(-1.0);
        Uq_st_Max=Ud_st_Max;
        Uq_st_Min=Ud_st_Min;
    }

    Idst_Max=95.0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=95;
    Idst_Min=(-1.0)*Idst_Max;
    Iqst_Max=Idst_Max;
    Iqst_Min=Idst_Min;


    Udc_Ref=HA_Module_RefUdc;

    Id_Positive_Max=Idst_Max;
    Id_Positive_Min=Idst_Min;
    Iq_Positive_Max=Iqst_Max;
    Iq_Positive_Min=Iqst_Min;
    Uout_d_Positive_Max=Ud_st_Max;
    Uout_d_Positive_Min=Ud_st_Min;
    Uout_q_Positive_Max=Uq_st_Max;
    Uout_q_Positive_Min=Uq_st_Min;

//controller
    PLL_PID_Num_1=PLL_PID_Num_1_def;
    PLL_PID_Num_0=PLL_PID_Num_0_def;

    //voltage loop
    Volt_PID_Num_1=Volt_PID_Num_1_def;
    Volt_PID_Num_0=Volt_PID_Num_0_def;

    PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def;
    PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def;

    //Current loop
    Har_2_Controller_Num_2=Har_2_Controller_Num_2_def;
    Har_2_Controller_Num_1=Har_2_Controller_Num_1_def;
    Har_2_Controller_Num_0=Har_2_Controller_Num_0_def;
    Har_2_Controller_Den_1=Har_2_Controller_Den_1_def;

    Har_6_Controller_Num_2=Har_6_Controller_Num_2_def;
    Har_6_Controller_Num_1=Har_6_Controller_Num_1_def;
    Har_6_Controller_Num_0=Har_6_Controller_Num_0_def;
    Har_6_Controller_Den_1=Har_6_Controller_Den_1_def;

    Har_12_Controller_Num_2=Har_12_Controller_Num_2_def;
    Har_12_Controller_Num_1=Har_12_Controller_Num_1_def;
    Har_12_Controller_Num_0=Har_12_Controller_Num_0_def;
    Har_12_Controller_Den_1=Har_12_Controller_Den_1_def;

    Current_PID_Num_1=Current_PID_Num_1_def;
    Current_PID_Num_0=Current_PID_Num_0_def;

    Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def;
    Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def;


    //regulate
    PLL_Factor=1.0;
    Volt_Factor=1.0;
    PH_Volt_Factor=1.0;
    Har_2_Factor=1.0;
    Har_6_Factor=1.0;
    Har_12_Factor=1.0;
    Current_Factor=1.0;
    Neg_Current_Factor=1.0;


    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3=0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3=0;

#ifdef EEPROM_FirstW
    DSP_2_EEPROM_Page1();
    DSP_2_EEPROM_Page2();
    DSP_2_EEPROM_Page3();
#endif


#ifdef EEPROM_ON
    EEPROM_Page1_2_DSP();
    EEPROM_Page2_2_DSP();
    EEPROM_Page3_2_DSP();
#endif

    RemoteRegulate_Dispose(1);
    FLAG_RemRegulate_SetOver=1;


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


    //Measurement
    HA_PortUanRms_LF_1=0.0;
    HA_PortUanRms_LF_2=0.0;
    HA_PortUbnRms_LF_1=0.0;
    HA_PortUbnRms_LF_2=0.0;
    HA_PortUcnRms_LF_1=0.0;
    HA_PortUcnRms_LF_2=0.0;

    HA_PortIaRms_LF_1=0.0;
    HA_PortIaRms_LF_2=0.0;
    HA_PortIbRms_LF_1=0.0;
    HA_PortIbRms_LF_2=0.0;
    HA_PortIcRms_LF_1=0.0;
    HA_PortIcRms_LF_2=0.0;

    HA_GridIaRms_LF_1=0.0;
    HA_GridIaRms_LF_2=0.0;
    HA_GridIbRms_LF_1=0.0;
    HA_GridIbRms_LF_2=0.0;
    HA_GridIcRms_LF_1=0.0;
    HA_GridIcRms_LF_2=0.0;

    HA_PortP_LF_1=0.0;
    HA_PortP_LF_2=0.0;
    HA_PortQ_LF_1=0.0;
    HA_PortQ_LF_2=0.0;

    LPF_5KHZ_X_Volt_A_1=0.0;
    LPF_5KHZ_Y_Volt_A_1=0.0;
    LPF_5KHZ_X_Volt_B_1=0.0;
    LPF_5KHZ_Y_Volt_B_1=0.0;

    LPF_20HZ_X_Ud_1=0.0;
    LPF_20HZ_Y_Ud_1=0.0;
    LPF_20HZ_X_Uq_1=0.0;
    LPF_20HZ_Y_Uq_1=0.0;


    //PLL
    PLL_Phase_Com=0.0;
    Phase_PLL=0.0;
    PhaseCom=PhaseComValue;

    Cos_ph_Trans=1.0;
    Sin_ph_Trans=0.0;
    Cos_ph_Trans_Com=1.0;
    Sin_ph_Trans_Com=0.0;
    Cos_ph_Trans_next=1.0;
    Sin_ph_Trans_next=0.0;

    for(i=0;i<4;i++)     //PLL
    {
        PLL_PID_Controller.PID_Controller_BUF[i]=0.0;

        U_d_Grid_H2_100_Filter.H_Filter_BUF[i]=0.0;
        U_d_Grid_H6_100_Filter.H_Filter_BUF[i]=0.0;
        U_d_Grid_H12_100_Filter.H_Filter_BUF[i]=0.0;

        U_q_Grid_H2_100_Filter.H_Filter_BUF[i]=0.0;
        U_q_Grid_H6_100_Filter.H_Filter_BUF[i]=0.0;
        U_q_Grid_H12_100_Filter.H_Filter_BUF[i]=0.0;
    }

    for(i=0;i<4;i++)     //Ave, P,Q
    {
        Va_Ave_H2_100_Filter.H_Filter_BUF[i]=0.0;
        Vb_Ave_H2_100_Filter.H_Filter_BUF[i]=0.0;
        Vc_Ave_H2_100_Filter.H_Filter_BUF[i]=0.0;
        Ave_H2_100_Filter.H_Filter_BUF[i]=0.0;
        Ave_H6_100_Filter.H_Filter_BUF[i]=0.0;

        P_H2_100_Filter.H_Filter_BUF[i]=0.0;
        Q_H2_100_Filter.H_Filter_BUF[i]=0.0;
        Q_Load_H2_100_Filter.H_Filter_BUF[i]=0.0;
    }

    for(i=0;i<4;i++)     //SVG Voltage
    {
        U_d_Grid_H2_20_Filter.H_Filter_BUF[i]=0.0;
        U_q_Grid_H2_20_Filter.H_Filter_BUF[i]=0.0;
    }

    for(i=0;i<4;i++)     //SVG Current
    {
        I_d_SVG_H2_20_Filter.H_Filter_BUF[i]=0.0;
        I_d_SVG_H2_100_Filter.H_Filter_BUF[i]=0.0;
        I_d_SVG_H6_100_Filter.H_Filter_BUF[i]=0.0;
        I_d_SVG_H12_100_Filter.H_Filter_BUF[i]=0.0;

        I_q_SVG_H2_20_Filter.H_Filter_BUF[i]=0.0;
        I_q_SVG_H2_100_Filter.H_Filter_BUF[i]=0.0;
        I_q_SVG_H6_100_Filter.H_Filter_BUF[i]=0.0;
        I_q_SVG_H12_100_Filter.H_Filter_BUF[i]=0.0;
    }

    for(i=0;i<4;i++)     //I_load
    {
//      I_SVG_H2_100_Filter.H_Filter_BUF[i]=0.0;
//      I_SVG_H6_100_Filter.H_Filter_BUF[i]=0.0;
//      I_SVG_H12_100_Filter.H_Filter_BUF[i]=0.0;
//      I_SVG_H2_20_Filter.H_Filter_BUF[i]=0.0;

        I_d_Load_H6_100_Filter.H_Filter_BUF[i]=0.0;
        I_d_Load_H12_100_Filter.H_Filter_BUF[i]=0.0;
        I_d_Load_H2_20_Filter.H_Filter_BUF[i]=0.0;

        I_q_Load_H6_100_Filter.H_Filter_BUF[i]=0.0;
        I_q_Load_H12_100_Filter.H_Filter_BUF[i]=0.0;
        I_q_Load_H2_20_Filter.H_Filter_BUF[i]=0.0;
    }

    for(i=0;i<3;i++)
    {
        HVAC_RealVoltage_Sum[i]=0.0;
        HVAC_RealCurrent_Sum[i]=0.0;
        HVAC_RealGridCurrent_Sum[i]=0.0;
    }

    HVAC_RealParameter_N=0;

    Har_5OR7_PH0=TAD_PLL;
    Har_5OR7_PH0=Har_5OR7_PH0*1884.95556;
    Har_11_PH0=TAD_PLL;
    Har_11_PH0=Har_11_PH0*3769.91112;
    Har_5_RMS=0.0;
    Har_7_RMS=0.0;
    Har_11_RMS=0.0;
    Har_5_PH=0.0;
    Har_7_PH=0.0;
    Har_11_PH=0.0;


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
#endif






/****************************************************************************/
/*                                                                          */
/*                       Variable RST                                       */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 复位滤波器、PI、遥控、故障以及原运行流程的历史状态
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Variable_RST(void)
{
    ushort i; /* i：本次数组、参数或模块遍历的索引。 */

#ifdef HVAC_online

    for(i=0;i<4;i++)    //controller
    {
        I_d_6_Har_Controller.Har_Controller_BUF[i]=0.0;
        I_d_12_Har_Controller.Har_Controller_BUF[i]=0.0;

        I_q_6_Har_Controller.Har_Controller_BUF[i]=0.0;
        I_q_12_Har_Controller.Har_Controller_BUF[i]=0.0;

        Volt_PID_Controller.PID_Controller_BUF[i]=0.0;
        ClusterVolt_A_PID_Controller.PID_Controller_BUF[i]=0.0;
        ClusterVolt_B_PID_Controller.PID_Controller_BUF[i]=0.0;
        ClusterVolt_C_PID_Controller.PID_Controller_BUF[i]=0.0;

        Current_d_PID_Controller.PID_Controller_BUF[i]=0.0;
        Current_q_PID_Controller.PID_Controller_BUF[i]=0.0;
        Neg_Current_d_PID_Controller.PID_Controller_BUF[i]=0.0;
        Neg_Current_q_PID_Controller.PID_Controller_BUF[i]=0.0;

        I_d_Ref_H2_100_Filter.H_Filter_BUF[i]=0.0;
        I_d_Ref_H6_100_Filter.H_Filter_BUF[i]=0.0;
    }
#endif



}

/****************************************************************************/
/*                                                                          */
/*                       Udc check                                          */
/*                                                                          */
/****************************************************************************/
#ifdef HVAC_online     //4.8ms
/*==================================================================
 * 函数功能: 依次检查各相在线模块直流电压，更新预充及高压判断标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void Udc_check(void)
{
    ushort index_temp; /* index_temp：本次模块电压检查使用的模块/相索引。 */
    ushort Check_temp_A,Check_temp_B,Check_temp_C; /* Check_temp_A：A 相模块电压巡检的位图/判定缓存。 */
    /* Check_temp_B：B 相模块电压巡检的位图/判定缓存。 */
    /* Check_temp_C：C 相模块电压巡检的位图/判定缓存。 */
    ushort temp_A,temp_B,temp_C; /* temp_A：A 相模块电压巡检的位图/判定缓存。 */
    /* temp_B：B 相模块电压巡检的位图/判定缓存。 */
    /* temp_C：C 相模块电压巡检的位图/判定缓存。 */
    float Udc_Temp; /* Udc_Temp：本次巡检读取的模块直流电压，单位 V。 */

//  if(Udc_Checked_Num<48)
//  {
//      Udc_Checked_Num++;
//  }

    if(Udc_Check_Index<48)
    {
        Udc_Check_Index++;
    }
    else
    {
        HA_A_LV_Precharge=0;
        HA_B_LV_Precharge=0;
        HA_C_LV_Precharge=0;

        HA_A_HV_Protect=0;
        HA_B_HV_Protect=0;
        HA_C_HV_Protect=0;

        Udc_Check_Index=1;
    }

    if(Udc_Check_Index<9)
    {
        Udc_Temp=Module_AD_Sample(Com_A1_PluginUnit_5,Udc_Check_Index,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<17)
    {
        Udc_Temp=Module_AD_Sample(Com_A2_PluginUnit_6,Udc_Check_Index-8,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<25)
    {
        Udc_Temp=Module_AD_Sample(Com_B1_PluginUnit_9,Udc_Check_Index-16,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<33)
    {
        Udc_Temp=Module_AD_Sample(Com_B2_PluginUnit_10,Udc_Check_Index-24,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<41)
    {
        Udc_Temp=Module_AD_Sample(Com_C1_PluginUnit_13,Udc_Check_Index-32,ModuleIndex_DCbus_Volt,0);
    }
    else if(Udc_Check_Index<49)
    {
        Udc_Temp=Module_AD_Sample(Com_C2_PluginUnit_14,Udc_Check_Index-40,ModuleIndex_DCbus_Volt,0);
    }


    if(Udc_Check_Index<17)
    {
        index_temp=Udc_Check_Index-1;
        index_temp=0x0001<<index_temp;

        if(Udc_Temp>HA_H_Voltage)  //过压
        {
            HA_A_HV_Protect=HA_A_HV_Protect|index_temp;
        }

        if(Udc_Temp<HA_L_Voltage_Precharge)//欠压
        {
            HA_A_LV_Precharge=HA_A_LV_Precharge|index_temp;
        }
    }
    else if(Udc_Check_Index<33)
    {
        index_temp=Udc_Check_Index-17;
        index_temp=0x0001<<index_temp;

        if(Udc_Temp>HA_H_Voltage)  //过压
        {
            HA_B_HV_Protect=HA_B_HV_Protect|index_temp;
        }

        if(Udc_Temp<HA_L_Voltage_Precharge)//欠压
        {
            HA_B_LV_Precharge=HA_B_LV_Precharge|index_temp;
        }
    }
    else if(Udc_Check_Index<49)
    {
        index_temp=Udc_Check_Index-33;
        index_temp=0x0001<<index_temp;

        if(Udc_Temp>HA_H_Voltage)  //过压
        {
            HA_C_HV_Protect=HA_C_HV_Protect|index_temp;
        }

        if(Udc_Temp<HA_L_Voltage_Precharge)//欠压
        {
            HA_C_LV_Precharge=HA_C_LV_Precharge|index_temp;
        }
    }

    if(Udc_Check_Index==48)
    {
        Check_temp_A=HA_PM_A2;
        Check_temp_A=Check_temp_A<<8;
        Check_temp_A=Check_temp_A+HA_PM_A1;

        Check_temp_B=HA_PM_B2;
        Check_temp_B=Check_temp_B<<8;
        Check_temp_B=Check_temp_B+HA_PM_B1;

        Check_temp_C=HA_PM_C2;
        Check_temp_C=Check_temp_C<<8;
        Check_temp_C=Check_temp_C+HA_PM_C1;


        temp_A=HA_A_LV_Precharge;
        index_temp=Check_temp_A;
        temp_A=temp_A&index_temp;

        temp_B=HA_B_LV_Precharge;
        index_temp=Check_temp_B;
        temp_B=temp_B&index_temp;

        temp_C=HA_C_LV_Precharge;
        index_temp=Check_temp_C;
        temp_C=temp_C&index_temp;

        if(temp_A||temp_B||temp_C)
        {
            PortRunFLAG.Bbits.Port_SigUdc=0;   //欠压
        }
        else
        {
            PortRunFLAG.Bbits.Port_SigUdc=1;   //charge over
        }


        temp_A=HA_A_HV_Protect;
        index_temp=Check_temp_A;
        temp_A=temp_A&index_temp;

        temp_B=HA_B_HV_Protect;
        index_temp=Check_temp_B;
        temp_B=temp_B&index_temp;

        temp_C=HA_C_HV_Protect;
        index_temp=Check_temp_C;
        temp_C=temp_C&index_temp;

        if(temp_A||temp_B||temp_C)
        {
            PortRunFLAG.Bbits.Port_HighUdc=1;   //过压

            if((PortRunFLAG.Bbits.PWMState_50Hz==0)||(PortRunFLAG.Bbits.PWMState_Dab==0))
            {
//              if(Port_Main_WorkMode==0x03)             //主电源，整个系统停机
//              {
//                  FaultDisposeFLAG.Bbits.Port_StopAll=1;PortRunFLAG.Bbits.Stop_AllPort=1;
//              }
//              else
//              {
//                  FaultDisposeFLAG.Bbits.Port_StopAll=0;
//              }

/*
                FaultDisposeFLAG.Bbits.Port_StopType=0;
                FaultDisposeFLAG.Bbits.FaultDisposeType=1;
                FaultDisposeFLAG.Bbits.PWMPulseState=3;

                PWMPluse_Disable(0);
*/
                //Port_RealFault.Bbits.PORT_FAULT_bit5=1;                           //过压
            }
        }
        else
        {
            PortRunFLAG.Bbits.Port_HighUdc=0;
        }
    }
}
#endif




/****************************************************************************/
/*                                                                          */
/*              main program                                                */
/*                                                                          */
/****************************************************************************/

/*==================================================================
 * 函数功能: 执行板级初始化和上电等待，然后持续调度迁移后的后台服务
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 声明为 int；初始化完成后进入永久主循环，正常运行不返回
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
int main(void)
{
    Flag_testT = 0;
    rtt = 0;

    /* 芯片、EMIFA、GPIO、定时器、UART 和中断初始化。 */
    ChbApplication_InitHardware();

#if CHB_OPEN_LOOP_PULSE_TEST
    /* UART2 到 EMIF 的透明桥测试模式，不进入功率控制状态机。 */
    for(;;)
    {
        ChbApplication_ServiceOpenLoopBridge();
    }
#else
    /* 正常模式：等待上电稳定，初始化变量，然后循环处理慢任务。 */
    ChbApplication_WaitPowerOnDelay(); /* Original Timer2 delay: 20 seconds. */
    ChbApplication_InitRuntime();

    for(;;)
    {
        ChbApplication_ServiceNormal();
    }
#endif
}

/****************************************************************************/
/*                                                                          */
/*              software delay                                              */
/*                                                                          */

/****************************************************************************/
/*                                                                          */
/*              旧版UART通讯函数 - 已废弃，使用VOFA替代                        */
/*                                                                          */
/****************************************************************************/
/*
void Uart2_send(unsigned char PCcomm)
{
        unsigned int txEmptytt;
    unsigned char i;
    unsigned char length;
    unsigned short CRC_Temp;

    GPIOPinWrite(SOC_GPIO_0_REGS, 104, 1);

        length=0;
        PCuartBUFF[length++]=ControllerAddr;
        PCuartBUFF[length++]=CommFunc;

        switch(CommFunc)
        {
            case WriteController:
            {
                PCuartBUFF[length++]=WriteOK;
            }break;

            case ReadController:
            {
                PCuartBUFF[length++]=StartAddr;
                PCuartBUFF[length++]=StartAddr>>8;
                PCuartBUFF[length++]=StartAddr>>16;
                PCuartBUFF[length++]=StartAddr>>24;
                PCuartBUFF[length++]=0;
                PCuartBUFF[length++]=ReadLength;
                length=length+ReadLength+ReadLength;

            }break;

            default: break;
        }

        CRC_Temp=CRC16(&PCuartBUFF[0], length);

        PCuartBUFF[length++]=CRC_Temp>>8;
        PCuartBUFF[length++]=CRC_Temp;


        for(i=0;i<length;i++)
    {
        UARTCharPut(SOC_UART_2_REGS, PCuartBUFF[i]);
    }


   txEmptytt = (UART_THR_TSR_EMPTY | UART_THR_EMPTY);


   while (txEmptytt != (HWREG(SOC_UART_2_REGS + UART_LSR) & txEmptytt));

   GPIOPinWrite(SOC_GPIO_0_REGS, 104, 0);
}
*/

/****************************************************************************/
/*                                                                          */
/*              旧版UART接收处理 - 已废弃，使用VOFA替代                        */
/*                                                                          */
/****************************************************************************/
#ifdef HVAC_online
#if 0
void UARTReceiveDispose(void)
{
{
    unsigned short CRC_Temp_1,CRC_Temp_2,WriteD,i; /* CRC_Temp_1：旧维护帧 CRC 校验的第一缓存字。 */
    /* CRC_Temp_2：旧维护帧 CRC 校验的第二缓存字。 */
    /* WriteD：旧维护写入数据/状态的兼容变量。 */
    /* i：本次数组、参数或模块遍历的索引。 */
    unsigned int Addr_write_temp; /* Addr_write_temp：旧维护帧写操作的临时地址。 */
    unsigned char i_temp; /* i_temp：旧初始化/维护流程临时整字计算值。 */
    ushort InitialTemp,InitialTemp_2; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */
    /* InitialTemp_2：原模块配置初始化的附加整字计算缓存。 */
    float InitialTempFloat; /* InitialTempFloat：原控制系数和给定初始化的浮点计算缓存。 */

#if CHB_OPEN_LOOP_PULSE_TEST
    /* 开环发脉冲测试修改：透明桥模式。
     * 原程序这里包含大量 0x60000620 特殊命令，会改 PM/NUM/闭环参数；
     * 测试版全部绕过，只允许调试界面按实际外部地址读写 CS2。
     */
    CRC_Temp_1=CRC16(&ReceiveuartBUFF[0],CommTotalNum);

    CRC_Temp_2=(unsigned short)ReceiveuartBUFF[CommTotalNum];
    CRC_Temp_2=CRC_Temp_2<<8;
    CRC_Temp_2=CRC_Temp_2|(unsigned short)ReceiveuartBUFF[CommTotalNum+1];

    if(CRC_Temp_1!=CRC_Temp_2)
    {
        return;
    }

    if((StartAddr<ValidAddr_CS2_S)||(StartAddr>ValidAddr_CS2_E))
    {
        return;
    }

    Addr_write_temp=StartAddr;
    if(CommFunc==WriteController)
    {
        WriteD=ReceiveuartBUFF[7];
        WriteD=WriteD<<8;
        WriteD=WriteD+ReceiveuartBUFF[6];

        HWREGH(Addr_write_temp)=WriteD;
        if(WriteD==HWREGH(Addr_write_temp))
        {
            WriteOK=0x28;
        }
        else
        {
            WriteOK=0x19;
        }
        /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(WriteController);
    }
    else
    {
        for(i=0;i<ReadLength;i++)
        {
            WriteD=HWREGH(Addr_write_temp);
            Addr_write_temp=Addr_write_temp+2;

            i_temp=(unsigned char)(i<<1);
            PCuartBUFF[i_temp+8]=WriteD;
            PCuartBUFF[i_temp+9]=WriteD>>8;
        }
        /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(ReadController);
    }
    return;
#endif

    CRC_Temp_1=CRC16(&ReceiveuartBUFF[0],CommTotalNum);

    CRC_Temp_2=(unsigned short)ReceiveuartBUFF[CommTotalNum];
    CRC_Temp_2=CRC_Temp_2<<8;
    CRC_Temp_2=CRC_Temp_2|(unsigned short)ReceiveuartBUFF[CommTotalNum+1];

    if(CRC_Temp_1!=CRC_Temp_2)
    {
        return;
    }

    if((StartAddr<ValidAddr_CS2_S)||(StartAddr>ValidAddr_CS5_E))
    {
        return;
    }
    else if((StartAddr>ValidAddr_CS2_E)&&(StartAddr<ValidAddr_CS5_S))
    {
        return;
    }
    else
    {
        Addr_write_temp=StartAddr;

        if(CommFunc==WriteController)
        {
            WriteD=ReceiveuartBUFF[7];
            WriteD=WriteD<<8;
            WriteD=WriteD+ReceiveuartBUFF[6];

            if((Addr_write_temp==0x60000620)&&(WriteD==0x1100))
            {
                Flag_ComTest=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0x1101))
            {
                Flag_ComTest=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0x1200))
            {
                FourPortComZhi=WriteD&0x00ff;
            }


            //模块配置
            /*
             * Online module-configuration command.
             * 0xe1xx..0xe6xx update PM for A1/A2/B1/B2/C1/C2.
             * After any PM change, recompute the two-card phase NUM and
             * ReferenceDEF, then write PM/NUM back through FPGA_Interface.
             */
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe100))
            {
                HA_PM_A1=WriteD&0x00ff;
                FPGA_Interface[Com_A1_PluginUnit_5-2].PM=HA_PM_A1;

                NHA_module_A=0;
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
                FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHA_module_A;
                FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHA_module_A;

                InitialTemp=NHA_module_A;
                HA_ReferenceDEF_A=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_A=HA_ReferenceDEF_A*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[0]=HA_PM_A1;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif

                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe200))
            {
                HA_PM_A2=WriteD&0x00ff;
                FPGA_Interface[Com_A2_PluginUnit_6-2].PM=HA_PM_A2;

                NHA_module_A=0;
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
                FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHA_module_A;
                FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHA_module_A;

                InitialTemp=NHA_module_A;
                HA_ReferenceDEF_A=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_A=HA_ReferenceDEF_A*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[1]=HA_PM_A2;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe300))
            {
                HA_PM_B1=WriteD&0x00ff;
                FPGA_Interface[Com_B1_PluginUnit_9-2].PM=HA_PM_B1;

                NHA_module_B=0;
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
                FPGA_Interface[Com_B1_PluginUnit_9-2].NUM=NHA_module_B;
                FPGA_Interface[Com_B2_PluginUnit_10-2].NUM=NHA_module_B;

                InitialTemp=NHA_module_B;
                HA_ReferenceDEF_B=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_B=HA_ReferenceDEF_B*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET=HA_PM_B1;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe400))
            {
                HA_PM_B2=WriteD&0x00ff;
                FPGA_Interface[Com_B2_PluginUnit_10-2].PM=HA_PM_B2;

                NHA_module_B=0;
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
                FPGA_Interface[Com_B1_PluginUnit_9-2].NUM=NHA_module_B;
                FPGA_Interface[Com_B2_PluginUnit_10-2].NUM=NHA_module_B;

                InitialTemp=NHA_module_B;
                HA_ReferenceDEF_B=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_B=HA_ReferenceDEF_B*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET=HA_PM_B2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET*10;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe500))
            {
                HA_PM_C1=WriteD&0x00ff;
                FPGA_Interface[Com_C1_PluginUnit_13-2].PM=HA_PM_C1;

                NHA_module_C=0;
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
                FPGA_Interface[Com_C1_PluginUnit_13-2].NUM=NHA_module_C;
                FPGA_Interface[Com_C2_PluginUnit_14-2].NUM=NHA_module_C;

                InitialTemp=NHA_module_C;
                HA_ReferenceDEF_C=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_C=HA_ReferenceDEF_C*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET=HA_PM_C1;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET*10;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe600))
            {
                HA_PM_C2=WriteD&0x00ff;
                FPGA_Interface[Com_C2_PluginUnit_14-2].PM=HA_PM_C2;

                NHA_module_C=0;
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
                FPGA_Interface[Com_C1_PluginUnit_13-2].NUM=NHA_module_C;
                FPGA_Interface[Com_C2_PluginUnit_14-2].NUM=NHA_module_C;

                InitialTemp=NHA_module_C;
                HA_ReferenceDEF_C=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_C=HA_ReferenceDEF_C*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET=HA_PM_C2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET*10;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}

            //Flag_Switch//1: 动接触器；2：动内部断路器;3：动外部断路器1；4：动外部断路器2
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xfa00))
            {
                SW.Bbits.Flag_Switch=1;
                SW.Bbits.Flag_SwitchZhi=WriteD&0x0001;
                SW.Bbits.Flag_SwitchRun=0;
                SW.Bbits.Flag_SwitchSuccess=0;
                SWTable_TotalTime=SwitchTotalNum;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xfb00))
            {
                SW.Bbits.Flag_SwitchZhi=WriteD&0x0001;

                if(SW.Bbits.Flag_SwitchZhi==1)
                {
                    Port_DINRead(0);
                    if(SW.Bbits.S_CONTACTOR==0)
                    {
                        SW.Bbits.Flag_Switch=2;
                        SW.Bbits.Flag_SwitchRun=0;
                        SW.Bbits.Flag_SwitchSuccess=0;
                        SWTable_TotalTime=SwitchTotalNum;
                    }
                }
                else
                {
                    SW.Bbits.Flag_Switch=2;
                    SW.Bbits.Flag_SwitchRun=0;
                    SW.Bbits.Flag_SwitchSuccess=0;
                    SWTable_TotalTime=SwitchTotalNum;
                }
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xfc00))
            {
                SW.Bbits.Flag_Switch=3;
                SW.Bbits.Flag_SwitchZhi=WriteD&0x0001;
                SW.Bbits.Flag_SwitchRun=0;
                SW.Bbits.Flag_SwitchSuccess=0;
                SWTable_TotalTime=SwitchTotalNum;
            }
            else
            {}

            //运行方式
            if((Addr_write_temp==0x60000620)&&(WriteD==0xFF00))
            {
                Port_RealStatus.Bbits.S_Mode=2;   //手动

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_RunMode=1;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_CtlMode=0;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xFF01))
            {
                Port_RealStatus.Bbits.S_Mode=0;   //自动

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_RunMode=1;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_CtlMode=1;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}

            //工作模式
            if((Addr_write_temp==0x60000620)&&(WriteD==0xff10))
            {
                Port_STATCOM_WorkMode=0;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff11))
            {
                Port_STATCOM_WorkMode=1;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff12))
            {
                Port_STATCOM_WorkMode=2;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff20))
            {
                Port_APF_WorkMode=0;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff21))
            {
                Port_APF_WorkMode=1;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff22))
            {
                Port_APF_WorkMode=2;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}


            if((Addr_write_temp==0x60000620)&&(WriteD==0xf2a0))
            {
                Add_3th=0;
                HWREGH(0x60000624)=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2a1))
            {
                Add_3th=1;
                HWREGH(0x60000624)=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2aa))
            {
                Method_3th=1;
                HWREGH(0x60000624)=11;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2ab))
            {
                Method_3th=2;
                HWREGH(0x60000624)=21;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2ac))
            {
                Method_3th=3;
                HWREGH(0x60000624)=31;
            }
            else
            {}

            //风扇启停
            if((Addr_write_temp==0x60000620)&&(WriteD==0xf3a0))
            {
                Module_FAN_OFF(1);
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf3a1))
            {
                Module_FAN_OFF(0);
            }
            else
            {}

            //标幺化
            if((Addr_write_temp==0x60000620)&&(WriteD==0xf4a0))
            {
                flag_Standard=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf4a1))
            {
                flag_Standard=1;
            }
            else
            {}

            //故障闭锁设置
            if((Addr_write_temp==0x60000620)&&(WriteD==0xf5a0))
            {
                flag_FaultClose=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf5a1))
            {
                flag_FaultClose=1;
            }
            else
            {}


            //无功设置
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xc000))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;


                #ifdef SVG_10KV
//////////////////////////////////////////////////////////
                if(InitialTemp_2>60)
                {
                    InitialTemp_2=60;
                }
//////////////////////////////////////////////////////////
                HWREGH(0x6000062c)=InitialTemp_2;
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*200;
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*20000.0;
                #endif

                #ifdef SVG_380V
//////////////////////////////////////////////////////////
                if(InitialTemp_2>4)
                {
                    InitialTemp_2=4;
                }
//////////////////////////////////////////////////////////
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*50;
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*5000.0;
                #endif

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xc100))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;


                #ifdef SVG_10KV
//////////////////////////////////////////////////////////
                if(InitialTemp_2>60)
                {
                    InitialTemp_2=60;
                }
//////////////////////////////////////////////////////////
                HWREGH(0x6000062c)=InitialTemp_2;
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*(-200);
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*(-20000.0);
                #endif

                #ifdef SVG_380V
//////////////////////////////////////////////////////////
                if(InitialTemp_2>4)
                {
                    InitialTemp_2=4;
                }
//////////////////////////////////////////////////////////
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*(-50);
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*(-5000.0);
                #endif

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xc200))
            {
                InitialTemp=WriteD&0x00ff;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }

                InitialTemp_2=InitialTemp_2*10+InitialTemp;
                if(InitialTemp_2==0)
                {
                    InitialTemp_2=100;
                }
                else if(InitialTemp_2<70)
                {
                    InitialTemp_2=100;
                }
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET=InitialTemp_2;
                PORT_Regulate_DATA.STATCOM_PF_SET=(float)InitialTemp_2/100.0;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}


            //参考Udc 和Idmax
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xa000))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2>75)
                {
                    InitialTemp_2=70;
                }
                else if(InitialTemp_2<15)
                {
                    InitialTemp_2=70;
                }
                else
                {}

                HA_Module_RefUdc=(float)InitialTemp_2*10.0;
                UstOK_HVAC_Port=HA_Module_RefUdc;
                Udc_Ref=HA_Module_RefUdc;

                if(NHA_module_A==0)
                {
                    Ud_st_Max=10.0;
                    Ud_st_Min=-10.0;
                    Uq_st_Max=10.0;
                    Uq_st_Min=-10.0;
                }
                else
                {
                    InitialTempFloat=(float)NHA_module_A;
                    InitialTempFloat=InitialTempFloat*1.1;
                    InitialTempFloat=InitialTempFloat*HA_Module_RefUdc;

                    Ud_st_Max=InitialTempFloat;
                    Ud_st_Min=InitialTempFloat*(-1.0);
                    Uq_st_Max=Ud_st_Max;
                    Uq_st_Min=Ud_st_Min;
                }

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)HA_Module_RefUdc;

                HWREGH(0x6000062A)=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

                Uout_d_Positive_Max=Ud_st_Max;
                Uout_d_Positive_Min=Ud_st_Min;
                Uout_q_Positive_Max=Uq_st_Max;
                Uout_q_Positive_Min=Uq_st_Min;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xa100))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2>60)
                {
                    InitialTemp_2=60;
                }
                else if(InitialTemp_2<10)
                {
                    InitialTemp_2=10;
                }
                else
                {}
                HWREGH(0x60000a2C)=InitialTemp_2;
                Idst_Max=(float)InitialTemp_2*2.0;
                Idst_Min=(-1.0)*Idst_Max;
                Iqst_Max=Idst_Max;
                Iqst_Min=Idst_Min;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=(ushort)Idst_Max;


                Id_Positive_Max=Idst_Max;
                Id_Positive_Min=Idst_Min;
                Iq_Positive_Max=Iqst_Max;
                Iq_Positive_Min=Iqst_Min;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else{}

            //PI
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB000))  //PLL
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    PLL_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=InitialTemp_2*10;
                    PLL_Factor=(float)InitialTemp_2/10.0;
                }
                PLL_PID_Num_1=PLL_PID_Num_1_def/PLL_Factor;
                PLL_PID_Num_0=PLL_PID_Num_0_def/PLL_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB100))  //Voltage
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Volt_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=InitialTemp_2*10;
                    Volt_Factor=(float)InitialTemp_2/10.0;
                }
                Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
                Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB200))  //Current
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Current_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=InitialTemp_2*10;
                    Current_Factor=(float)InitialTemp_2/10.0;
                }
                Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
                Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB300))  //Neg_Current
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Neg_Current_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=InitialTemp_2*10;
                    Neg_Current_Factor=(float)InitialTemp_2/10.0;
                }
                Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def/Neg_Current_Factor;
                Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def/Neg_Current_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB400))  //PH_Volt
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    PH_Volt_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=InitialTemp_2*10;
                    PH_Volt_Factor=(float)InitialTemp_2/10.0;
                }
                PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def/PH_Volt_Factor;
                PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def/PH_Volt_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB500))  //Har_2
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Har_2_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=InitialTemp_2*10;
                    Har_2_Factor=(float)InitialTemp_2/10.0;
                }
                Har_2_Controller_Num_2=Har_2_Controller_Num_2_def/Har_2_Factor;
                Har_2_Controller_Num_1=Har_2_Controller_Num_1_def/Har_2_Factor;
                Har_2_Controller_Num_0=Har_2_Controller_Num_0_def/Har_2_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB600))  //Har_6
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Har_6_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=InitialTemp_2*10;
                    Har_6_Factor=(float)InitialTemp_2/10.0;
                }
                Har_6_Controller_Num_2=Har_6_Controller_Num_2_def/Har_6_Factor;
                Har_6_Controller_Num_1=Har_6_Controller_Num_1_def/Har_6_Factor;
                Har_6_Controller_Num_0=Har_6_Controller_Num_0_def/Har_6_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB700))  //Har_12
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Har_12_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=InitialTemp_2*10;
                    Har_12_Factor=(float)InitialTemp_2/10.0;
                }
                Har_12_Controller_Num_2=Har_12_Controller_Num_2_def/Har_12_Factor;
                Har_12_Controller_Num_1=Har_12_Controller_Num_1_def/Har_12_Factor;
                Har_12_Controller_Num_0=Har_12_Controller_Num_0_def/Har_12_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xB85A))
            {
                //Current loop
                flag_INV=1;
                HWREGH(0x60000628)=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xB8EA))
            {
                //Current loop
                flag_INV=0;
                HWREGH(0x60000628)=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xA85A))
            {
                DAB_ONLY=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xA8EA))
            {
                DAB_ONLY=0;
            }
            else{}

            //端口在线
            if((Addr_write_temp==0x60000620)&&((WriteD&0xAAF0)==0xAA00))
            {
                Port_OnState=WriteD&0x000f;
                InitialTemp=4-PORT_Index;
                InitialTemp=0x0001<<InitialTemp;
                Port_OnState=Port_OnState|InitialTemp;
                HWREGH(0x6000062e)=Port_OnState;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2]=Port_OnState;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else{}

            //脉冲操作
            if((Addr_write_temp==0x60000620)&&(WriteD==0xff50))
            {
                ControlFLAG.Bbits.Port_Stop=0x00;
                ControlFLAG.Bbits.Port_Start=0x01;
                Port_RunStatus=PortStatusNum_Start;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff51))
            {
                ControlFLAG.Bbits.Port_Stop=0x01;
                ControlFLAG.Bbits.Port_Start=0x00;

                Port_RunStatus=PortStatusNum_Stop;

                ControlFLAG.Bbits.SVG_Start=0;
                FaultDisposeFLAG.Bbits.FaultDisposeType=2;
                FaultDisposeFLAG.Bbits.PWMPulseState=3;
                FaultDisposeFLAG.Bbits.Port_StopType=3;    //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown

            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff5a))
            {
                FaultLatch_Clear(0,1);
/*
//              EEPROM_Page1_2_DSP();
//              RemoteRegulate_Dispose(1);
//              flag_SendDABRef_First=1;
//              flag_SendSVGRef_First=1;
//              ControlFLAG.Bbits.DAB_DutyStart=1;
//              ControlFLAG.Bbits.DAB_DutyOver=0;
//              HA_DABDuty_A=250;    //5%
//              Write_ControlParameter_DAB_Startup();
//              PWMPluse_Enable(WriteD&0x00ff);
*/

                flag_DABOPEN=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff5b))
            {
                FaultDisposeFLAG.Bbits.PWMPulseState=0x02;
                PWMPluse_Disable(0);

            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffaa))
            {
                Port_DINRead(0);
                if(SW.Bbits.S_CONTACTOR==1)
                {
                    FaultLatch_Clear(0,1);
                    flag_SVGOPEN=1;
//              Variable_RST();
                }
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffab))
            {
                FaultDisposeFLAG.Bbits.PWMPulseState=0x01;
                PWMPluse_Disable(0);
            }
            else
            {}

            if((Addr_write_temp==0x60000620)&&(WriteD==0xffc0))
            {
                Port_BKU_Send(1);    //send BKU
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc1))
            {
                Port_BKU_Send(0);    //send BKU
            }

/*
            if((Addr_write_temp==0x60000620)&&(WriteD==0xffc0))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.0;
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.0;
                PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=0.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
                PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=0.0;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
                Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
                Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc1))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.5*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc2))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.5*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc3))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.5*30.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc4))
            {
                PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=0.02*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=Phase_PLL;

                Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
                Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffd1))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffd2))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffd3))
            {
                PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=0.05*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=Phase_PLL;

                Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
                Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE1))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.2*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE2))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;

                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.05*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE3))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.2*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;

                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffe4))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.2*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE5))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.2*30.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffe6))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.2*30.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else
            {
//              PWMPluse_Disable(WriteD&0x00ff);
            }
*/


            HWREGH(Addr_write_temp)=WriteD;
            if(WriteD==HWREGH(Addr_write_temp))
            {
                WriteOK=0x28;
            }
            else
            {
                WriteOK=0x19;
            }

            /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(WriteController);
        }
        else
        {
            for(i=0;i<ReadLength;i++)
            {
                WriteD=HWREGH(Addr_write_temp);
                Addr_write_temp=Addr_write_temp+2;

                i_temp=(unsigned char)(i<<1);
                PCuartBUFF[i_temp+8]=WriteD;
                PCuartBUFF[i_temp+9]=WriteD>>8;
            }
            /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(ReadController);
        }
    }
}
#endif
#endif


/****************************************************************************/
/*                                                                          */
/*              CRC                                                         */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 计算旧 UART 兼容帧的 CRC16，保持原多项式及交换字节规则
 * 输入参数: ba：CRC 输入缓冲区首地址，调用者提供有效帧字节
 *           size：输入缓存有效长度
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 原实现生成的 16 位 CRC 值
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
unsigned short CRC16(unsigned char *ba, int size)
{
    CRC crc; /* crc：逐字节、逐位累加的 CRC 校验状态。 */
    crc.crc16 = 0xffff;
    int i, l; /* i：本次字节/位循环序号。 */
    /* l：CRC 当前位循环序号。 */

    for (i=0; i<size; i++)
    {
        unsigned char ch = ba[i]; /* ch：当前参与 CRC 的输入字节。 */
        crc.by[0] = crc.by[0] ^ ch;
        for (l=0; l<8; l++)
        {
            if (crc.by[0] & 0x01)
            {
                crc.crc16 = crc.crc16 >> 1;
                crc.crc16 = crc.crc16 ^ 0xa001;
            }
            else
            {
                crc.crc16 = crc.crc16 >> 1;
            }
        }
    }
    unsigned char swap = crc.by[0]; /* swap：原 CRC 高低字节交换的中间值。 */
    crc.by[0] = crc.by[1];
    crc.by[1] = swap;

    return crc.crc16;
}

/****************************************************************************/
/*                                                                          */
/*              keyscan                                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 保留原本地按键扫描入口及按键状态处理
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void keyscan(void)
{
//  ushort keytt;
    if(GPIOPinRead(SOC_GPIO_0_REGS, 67)==0)
    {
        Delay(0xffff);
        Delay(0xffff);

        if(GPIOPinRead(SOC_GPIO_0_REGS, 67)==0)
        {
            GPIOPinWrite(SOC_GPIO_0_REGS, 3, 1);
            while(GPIOPinRead(SOC_GPIO_0_REGS, 67)==0);
            GPIOPinWrite(SOC_GPIO_0_REGS, 3, 0);

            Flag_Key=1;
        }
    }

    if(Flag_Key==1)
    {
        Flag_Key=0;


//      keytt=FPGA_Interface[2].ID.ID16;
        GPIOPinWrite(SOC_GPIO_0_REGS, 104, 1);
//      UARTCharPut(SOC_UART_2_REGS, 0xeb);
//      UARTCharPut(SOC_UART_2_REGS, keytt>>8);
//      UARTCharPut(SOC_UART_2_REGS, keytt);
//      UARTCharPut(SOC_UART_2_REGS, 0x90);
        Delay(0xffff);
        GPIOPinWrite(SOC_GPIO_0_REGS, 104, 0);

/*
        RUN_Status=0;
        ((unsigned short *)SOC_EMIFA_CS2_ADDR)[DABRESET_Addr]= 0xeb;
        Flag_400ms=1;
        while(Flag_400ms);
        ((unsigned short *)SOC_EMIFA_CS2_ADDR)[Port_RESET_Addr]= 0xeb;  //SO RESET


        Flag_Fault=0;

        Delay(0xffff);

        if(Flag_SystemWork==0)
        {
            Flag_SystemWork=0xeb;

                Vd_Out_DCAC=0.0;
//          Flag_SystemWork=0;
                //////////////////////////////////////////
//          Flag_StartACDCOver=0xeb;
//          Work_State=0xc0;
        }
        else
        {
            Flag_SystemWork=0;
        }

        SwitchOut=0xffff&SwitchMask;
        ((unsigned short *)SOC_EMIFA_CS2_ADDR)[SwitchOut_Addr]= SwitchOut;
*/
    }
}


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

  ChbDabControl_Tick10ms();                    // 10 ms节拍，推进状态机和超时计数

  if(Flag_Clear_0==1)
  {
    Flag_Clear_0=0;
    Flag_Clear_1=1;
  }
  else
  {
    if(Flag_Clear_1==1)
    {
      Flag_Clear_1=0;
        FaultLatch_Clear(1,Flag_Clear_zhi);
    }
  }

    cout_Wait--;
    if(cout_Wait==0)
    {
        Flag_Wait=1;
    }

    if(Flag_StopWait)
    {
        coutStopWait--;
        if(coutStopWait==0)
        {
            Flag_StopWait=0;
            Flag_StopWaitEnd=1;
        }
    }

    cout1s++;
  if(cout1s==100)
  {
    cout1s=0;
    Flag_1s=1;
  }

    cout20s++;
  if(cout20s==2000)
  {
    cout20s=0;
    Flag_20s=1;
  }

    if(Flag_50ms)
    {
        cout50ms++;
        if(cout50ms==5)
        {
            cout50ms=0;
            Flag_50ms=0;
        }
    }

    cout100ms++;
    if(cout100ms==10)
    {
        cout100ms=0;
        Flag_100ms=1;
    }


    coutRemoteMeasure++;
    if(coutRemoteMeasure==3)
    {
        coutRemoteMeasure=0;
        Flag_RemoteMeasure=1;
    }


    if(Flag_Switch500ms)
    {
        coutSwitch500ms--;
        if(coutSwitch500ms==0)
        {
            Flag_Switch500ms=0;
        }
    }

    if(Flag_Switch1s)
    {
        coutSwitch1s--;
        if(coutSwitch1s==0)
        {
            Flag_Switch1s=0;
        }
    }

    if(Flag_Precharge500ms)
    {
        coutPrecharge500ms++;
        if(coutPrecharge500ms==1000)
        {
            coutPrecharge500ms=0;
            Flag_Precharge500ms=0;
        }
    }

    if(Flag_PreStart3s)
    {
        coutPreStart3s++;
        if(coutPreStart3s==300)
        {
            coutPreStart3s=0;
            Flag_PreStart3s=0;
        }
    }


    if(Flag_SVG1s)
    {
        coutSVG1s++;
        if(coutSVG1s==500)
        {
            coutSVG1s=0;
            Flag_SVG1s=0;
        }
    }


    countWaitPowerStart10s++;
    if(countWaitPowerStart10s==1000)
    {
        countWaitPowerStart10s=0;
        Flag_WaitPowerStart10s=1;
    }

#ifdef HVAC_online
    if(Flag_V_L)
    {
        Cout_V_L++;
        if(Cout_V_L>LV_Time_1)
        {
            Flag_V_L1_END=1;
        }

        if(Cout_V_L>LV_Time_2)
        {
            Flag_V_L2_MID=1;
        }

        if(Cout_V_L>LV_Time_1)
        {
            Flag_V_L2_END=1;
            Flag_V_L=0;
            Cout_V_L=0;
        }
    }

    if(ISO_counter_on<500)
    {
        ISO_counter_on++;
    }

    if(ISO_counter_off<500)
    {
        ISO_counter_off++;
    }
#endif

//Timer for OC_OV_LV protection,edited by shiqi

#ifdef HVAC_online
    if (counter_OC_flag==1)
    {
       counter_OC_time++;
    }
    if (counter_OC_flag1==1)
    {
       counter_OC_time1++;
    }
    if (counter_OV_flag==1)
    {
       counter_OV_time++;
    }
#endif
}

/****************************************************************************/
/*                                                                          */
/*              UART INT routine                                           */
/*                                                                          */
/****************************************************************************/
volatile unsigned char UartDebugBuf[32]; /* UartDebugBuf：原 UART 调试发送缓存。 */
volatile unsigned int UartDebugIndex; /* UartDebugIndex：原 UART 调试缓存当前位置。 */
/*==================================================================
 * 函数功能: 保留原 UART2 ISR 符号；调试通信主流程不注册此中断
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void UART2Isr(void)
{
    unsigned char rxData = 0; /* rxData：原 UART ISR 读取的接收字符。 */
    unsigned int int_id = 0; /* int_id：原 UART 中断源标识。 */
        unsigned int rxtemp; /* rxtemp：原 UART 接收处理临时字符。 */

    int_id = UARTIntStatus(SOC_UART_2_REGS);

    IntEventClear(SYS_INT_UART2_INT);

    if(UART_INTID_RX_DATA == int_id)
    {
        rxData = UARTCharGetNonBlocking(SOC_UART_2_REGS); // 每次中断只取一个字节

        /* A5 5A维护帧优先交给新协议；普通地址帧继续走原485解析器。 */
        if(MaintenanceProtocol_RxByteForUart(rxData))
        {
            if(UartDebugIndex < 32)
                UartDebugBuf[UartDebugIndex++] = rxData;
            return;
        }
        if(UartDebugIndex < 32) {
            UartDebugBuf[UartDebugIndex++] = rxData;
        }

                CommNum++;
                if(CommNum>8)
                {
                    CommNum=9;
                }

                /* 旧协议固定按：地址、功能、32位地址、长度、数据接收。 */
                switch(CommNum)
                {
                    case 1:
                    {
                        if(rxData!=ControllerAddr)
                        {
                            CommNum=0;
                        }
                        else
                        {
                            ReceiveuartBUFF[0]=rxData;
                        }
                    }break;
                    case 2:
                    {
                        if((rxData!=ReadController)&&(rxData!=WriteController))
                        {
                            CommNum=0;
                        }
                        else
                        {
                            CommFunc=rxData;
                            ReceiveuartBUFF[1]=rxData;
                        }
                    }break;
                case 3:
                    {
                        StartAddr=rxData;
                        ReceiveuartBUFF[2]=rxData;
                    }break;
                case 4:
                    {
                        rxtemp=(unsigned int)rxData;
                        rxtemp=rxtemp<<8;
                        StartAddr=StartAddr|rxtemp;

                        ReceiveuartBUFF[3]=rxData;
                    }break;
                case 5:
                    {
                        rxtemp=(unsigned int)rxData;
                        rxtemp=rxtemp<<16;
                        StartAddr=StartAddr|rxtemp;

                        ReceiveuartBUFF[4]=rxData;
                    }break;
                case 6:
                    {
                        rxtemp=(unsigned int)rxData;
                        rxtemp=rxtemp<<24;
                        StartAddr=StartAddr|rxtemp;

                        ReceiveuartBUFF[5]=rxData;
                    }break;
                    case 7:
                    {
                        ReceiveuartBUFF[6]=rxData;
                    }break;
                    case 8:
                    {
                        ReceiveuartBUFF[7]=rxData;

                        if(CommFunc==ReadController)
                        {
                            ReadLength=ReceiveuartBUFF[6];
                            ReadLength=ReadLength<<8;
                            ReadLength=ReadLength+ReceiveuartBUFF[7];
                        }

                        CommLength=2;
                        CommInNum=0;
                    }break;
                    case 9:
                    {
                        ReceiveuartBUFF[8+CommInNum++]=rxData;
                        if(CommInNum==CommLength)
                        {
                            /* 收齐数据后只置标志，完整解析放到主循环，避免ISR过长。 */
                            CommTotalNum=6+CommLength;
                            Flag_Uart=1;
                            UARTIntDisable(SOC_UART_2_REGS, UART_INT_RXDATA_CTI);
                            CommInNum=0;
                            CommNum=0;
                            CommLength=0;
                        }
                    }break;
                }
    }
    return;
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

    if(GPIOPinIntStatus(SOC_GPIO_0_REGS, 69) == GPIO_INT_PEND)     // TAD中断：每个采样周期执行一次快速控制
    {
            GPIOPinIntClear(SOC_GPIO_0_REGS, 69);
            GPIOPinWrite(SOC_GPIO_0_REGS, 43, 1);
            GPIOPinWrite(SOC_GPIO_0_REGS, 45, 1);

#ifdef HVAC_online
            Port_StateUpdate_Read();               // 读取接触器、故障和端口状态

            Port_AD_Sample(0);                     // 采样交流电压、电流及直流电压
            HVAC_RealTimeParameter();              // 更新RMS、P、Q等慢变量
            SVG_Udc_Average();                     // 计算三相单元直流平均值
            ChbDabControl_FastStep();              // PLL、开/闭环调制、DAB占空比爬坡
            /*
             * Udc_check()/OC_OV_LV_faultcheck() 是原 HVAC 闭环保护链路。
             * 开环调试时外部母线和采样值可能不存在，不能用它们决定是否
             * 允许发波；否则 DAB 第一拍后会被误判为欠压/越限而封锁。
             * UNIT/FPGA 硬件 FO/FB 故障仍在下面的独立故障中断中处理。
             */
            if(g_chb_dab_command.ac_mode != CHB_AC_MODE_OPEN_LOOP)
            {
                Udc_check();
            }

            if((FaultTypeFLAG.Bbits.Module_Fault)&&(ModuleFault_DisposeTimes))     // 模块故障立即封波
            {
                FaultDispose();
                ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
            }

            // Source-project software OC/OV can be disabled for low-power debugging.
            // FPGA/module hardware fault paths remain active outside this switch.
            /* 软件保护可由仿真器关闭，硬件故障路径不受该变量影响。 */
            if(g_chb_dab_command.ac_mode != CHB_AC_MODE_OPEN_LOOP)
            {
                if((g_chb_dab_protection_enable != 0U) &&
                   ((HVAC_OC_Fault == 1)||(HVAC_OV_Fault == 1)) &&
                   (OC_OV_LV_PortLatch_TAD != 1))
                {
                    OC_OV_LV_FaultDispose();
                    ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
                }
                else if(g_chb_dab_protection_enable != 0U)
                {
                    OC_OV_LV_faultcheck();
                }
            }

            //if(SVG_Udc_Ave > OV_allport_shutdown)
            //{
            //  FaultDisposeFLAG.Bbits.Port_StopAll=1;
            //  PortRunFLAG.Bbits.Stop_AllPort=1;
            //  Port_StateUpdate_Write();
            //}

            //edited by shiqi, end

            if(((PortRunFLAG.Bbits.PWMState_50Hz==1)&&(PortRunFLAG.Bbits.PWMState_Dab==1))&&(PortRunFLAG.Bbits.Stop_AllPort))  //系统停机成功
            {
                PortController_Communication(1);
                Port_AllStopFlag_RST();
            }

            Port_StateUpdate_Write();                // 将运行状态和故障状态回写到DPRAM

            //edited by shiqi
            if(Port_RealFault.Bbits.PORT_FAULT_bit15)
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
            FT3_PluginUint_Write(2,dataPackNum);
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

    if(GPIOPinIntStatus(SOC_GPIO_0_REGS, 142) == GPIO_INT_PEND)    //FAULT  Interrupt routine
    {
            GPIOPinIntClear(SOC_GPIO_0_REGS, 142);
//          TestINT=!TestINT;
//          FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led2=TestINT;

            FaultCheckAndDispose_StepOne();             //第1步
            if((PortLatch_TAD)||(ModuleFault_DisposeTimes))  //端口或模块故障
            {
                FaultDispose();                           //第2步
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

    if(GPIOPinIntStatus(SOC_GPIO_0_REGS, 101) == GPIO_INT_PEND)     //DPRAM  Interrupt routine
    {
            GPIOPinIntClear(SOC_GPIO_0_REGS, 101);

            Flag_RemoteControlRegulate=1;
    }

  GPIOBankIntEnable(SOC_GPIO_0_REGS, 6);
}
