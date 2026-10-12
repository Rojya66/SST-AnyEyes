
//###########################################################################
//
// FILE:   main.h
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################
#include <math.h>
//#include <fastrts67x.h>
#include "soc_C6748.h"              
#include "ChbBuildConfig.h"

/**************************Macro definition*********************************/
/* ushort：旧代码无符号 16 位整数类型别名。 */
#define ushort unsigned short
/* uchar：旧代码无符号 8 位整数类型别名。 */
#define uchar unsigned char
/* Uint16：旧代码无符号 16 位整数类型别名。 */
#define Uint16 unsigned short
/* Uint8：旧代码无符号 8 位整数类型别名。 */
#define Uint8 unsigned char

/* 本工程只面向 HVAC 端口，不再保留 HVDC/LVDC 构建选择。 */
/* HVAC_online：选择高压交流端口的旧代码条件编译分支。 */
#define HVAC_online
/* PORT_Index：旧端口编号，HVAC 工程为 1。 */
#define PORT_Index  1


/* ControlTAD：原 FPGA PWM/TAD 周期计数，保持 5000。 */
#define ControlTAD  5000   
/* SwitchTotalNum：原开关动作表条目数量。 */
#define SwitchTotalNum  2
/* OV_Stop：原过压停机策略条件编译开关。 */
#define OV_Stop
//#define EEPROM_FirstW  // first
#if CHB_LOAD_EEPROM_PARAMETERS
/* 上位机版本保留EEPROM读写；仿真器模式默认不访问EEPROM。 */
/* EEPROM_ON：原 EEPROM 兼容访问开关，受上电加载配置控制。 */
#define EEPROM_ON
#endif
/* FaultOutTest：原故障输出测试编译标识。 */
#define FaultOutTest


#ifdef HVAC_online
//HVAC
/* ReferenceDEF：旧 FPGA 模块参考基数，乘在线模块数得到相基值。 */
#define ReferenceDEF 1250.0//17500.0      //1250*NHA_module
/* TAD_PLL：原 PLL 控制周期，0.0001 s，即 10 kHz。 */
#define TAD_PLL  0.0001   //10KHz

/* LV_TotalTime：原欠压处理总延时，单位原 10 ms 节拍。 */
#define LV_TotalTime  200       //2s
/* LV_Time_1：原欠压处理第一阶段时间，单位原 10 ms 节拍。 */
#define LV_Time_1       0       //0s
/* LV_Time_2：原欠压处理第二阶段时间，单位原 10 ms 节拍。 */
#define LV_Time_2      62       //0.62s
/* Vslope：原欠压处理电压给定斜坡步长。 */
#define Vslope     0.1255       //((VoltLevel_2-VoltLevel_1)/(LV_TotalTime-LV_Time_2))*TAD


/* SVG_1：原 SVG 控制算法条件编译选择。 */
#define SVG_1
/* SimHVAC：原 HVAC 仿真/测试兼容编译标识。 */
#define SimHVAC
/* SVG_10KV：原 10 kV 参数方案编译选择。 */
#define SVG_10KV
//#define SVG_380V
/* COM_TEST_PluginUnit：原插件通信测试编译选择。 */
#define COM_TEST_PluginUnit


/* PhaseComValue：原电流/锁相变换相位补偿默认值，单位 rad。 */
#define PhaseComValue 0.16
/* SoftwarePhaseComValue：原输出变换软件相位补偿默认值，单位 rad。 */
#define SoftwarePhaseComValue 0.1085


/* PLL_PID_Num_1_def：锁相环增量 PI 本拍误差系数的上电默认值。 */
#define PLL_PID_Num_1_def  -2.001     //-10.00  //-5.003  //-3.002  
/* PLL_PID_Num_0_def：锁相环增量 PI 上一拍误差系数的上电默认值。 */
#define PLL_PID_Num_0_def 1.999       //9.996 //4.998   //2.998  		

	//voltage loop
/* Volt_PID_Num_1_def：电压增量 PI 本拍误差系数的上电默认值。 */
#define Volt_PID_Num_1_def     0.805/8.0    //0.8025 (380V)              wws10kv
/* Volt_PID_Num_0_def：电压增量 PI 上一拍误差系数的上电默认值。 */
#define Volt_PID_Num_0_def     -0.795/8.0   //-0.7975 (380V)                      

/* PH_Volt_PID_Num_1_def：相间电压增量 PI 本拍误差系数的上电默认值。 */
#define PH_Volt_PID_Num_1_def 0.201 
/* PH_Volt_PID_Num_0_def：相间电压增量 PI 上一拍误差系数的上电默认值。 */
#define PH_Volt_PID_Num_0_def -0.199

	//Current loop
/* Har_2_Controller_Num_2_def：2 次谐振控制器差分式分子第 2 项系数的上电默认值。 */
#define Har_2_Controller_Num_2_def -0.05/10.0        //-0.4/4*0.5 
/* Har_2_Controller_Num_1_def：2 次谐振控制器差分式分子第 1 项系数的上电默认值。 */
#define Har_2_Controller_Num_1_def 0.0999/10.0       //0.7996/4*0.5 
/* Har_2_Controller_Num_0_def：2 次谐振控制器差分式分子第 0 项系数的上电默认值。 */
#define Har_2_Controller_Num_0_def -0.05/10.0				//-0.3996/4*0.5
/* Har_2_Controller_Den_1_def：2 次谐振控制器差分式分母第 1 项系数的上电默认值。 */
#define Har_2_Controller_Den_1_def -1.996

/* Har_6_Controller_Num_2_def：6 次谐振控制器差分式分子第 2 项系数的上电默认值。 */
#define Har_6_Controller_Num_2_def -0.05          		//-0.6/6*0.5
/* Har_6_Controller_Num_1_def：6 次谐振控制器差分式分子第 1 项系数的上电默认值。 */
#define Har_6_Controller_Num_1_def 0.0999        		//1.199/6*0.5
/* Har_6_Controller_Num_0_def：6 次谐振控制器差分式分子第 0 项系数的上电默认值。 */
#define Har_6_Controller_Num_0_def -0.05       			//-0.5994/6*0.5
/* Har_6_Controller_Den_1_def：6 次谐振控制器差分式分母第 1 项系数的上电默认值。 */
#define Har_6_Controller_Den_1_def -1.965

/* Har_12_Controller_Num_2_def：12 次谐振控制器差分式分子第 2 项系数的上电默认值。 */
#define Har_12_Controller_Num_2_def -0.05/10.0				//-0.8/8*0.5 
/* Har_12_Controller_Num_1_def：12 次谐振控制器差分式分子第 1 项系数的上电默认值。 */
#define Har_12_Controller_Num_1_def 0.0999/10.0     	//1.599/8*0.5 
/* Har_12_Controller_Num_0_def：12 次谐振控制器差分式分子第 0 项系数的上电默认值。 */
#define Har_12_Controller_Num_0_def -0.05/10.0   	  //-0.7992/8*0.5
/* Har_12_Controller_Den_1_def：12 次谐振控制器差分式分母第 1 项系数的上电默认值。 */
#define Har_12_Controller_Den_1_def -1.86

/* Current_PID_Num_1_def：电流增量 PI 本拍误差系数的上电默认值。 */
#define Current_PID_Num_1_def -15.01/12.0	         //-15.01/3.0 (380V)
/* Current_PID_Num_0_def：电流增量 PI 上一拍误差系数的上电默认值。 */
#define Current_PID_Num_0_def 14.99/12.0           //14.99/3.0  (380V)         wws10kv  /4 is edited by shiqi

/* Neg_Current_PID_Num_1_def：负序电流增量 PI 本拍误差系数的上电默认值。 */
#define Neg_Current_PID_Num_1_def -1.501 	      		//-3.002*0.5 
/* Neg_Current_PID_Num_0_def：负序电流增量 PI 上一拍误差系数的上电默认值。 */
#define Neg_Current_PID_Num_0_def 1.499         		//2.998*0.5 

/* WL_decouple：原 dq 电流控制交叉耦合补偿系数。 */
#define WL_decouple 1.884   

#endif





//Block length
/* DPRAMBlock1_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock1_Length    	16
/* DPRAMBlock2_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock2_Length    	43
/* DPRAMBlock3_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock3_Length    	33
/* DPRAMBlock4_PluginUnit_Length：原 DPRAMBlock4 插件硬件编号，寄存器数组使用编号减 2。 */
#define DPRAMBlock4_PluginUnit_Length  128

//DPRAM read or write
/* ReadBlock_remControl：DPRAM remControl 读取数据块标识，保持原协议编号。 */
#define ReadBlock_remControl  				1
/* ReadBlock_remRegulate：DPRAM remRegulate 读取数据块标识，保持原协议编号。 */
#define ReadBlock_remRegulate 				2
/* WriteBlock_remMeaSig：DPRAM remMeaSig 回写数据块标识，保持原协议编号。 */
#define WriteBlock_remMeaSig   				3
/* WriteBlock_PluginUnit_2：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_2     	4
/* WriteBlock_PluginUnit_3：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_3     	5 
/* WriteBlock_PluginUnit_4：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_4     	6 
/* WriteBlock_PluginUnit_5：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_5     	7 
/* WriteBlock_PluginUnit_6：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_6     	8 
/* WriteBlock_PluginUnit_7：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_7     	9 
/* WriteBlock_PluginUnit_8：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_8     	10 
/* WriteBlock_PluginUnit_9：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_9     	11
/* WriteBlock_PluginUnit_10：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_10      12
/* WriteBlock_PluginUnit_11：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_11      13 
/* WriteBlock_PluginUnit_12：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_12      14 
/* WriteBlock_PluginUnit_13：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_13      15 
/* WriteBlock_PluginUnit_14：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_14      16 
/* WriteBlock_PluginUnit_15：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_15      17 
/* WriteBlock_PluginUnit_16：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_16      18 
/* WriteBlock_PluginUnit_17：原 WriteBlock 插件硬件编号，寄存器数组使用编号减 2。 */
#define WriteBlock_PluginUnit_17      19 

//YTHS
/* WriteBlock_remRegulate：DPRAM remRegulate 回写数据块标识，保持原协议编号。 */
#define WriteBlock_remRegulate   	  20


//Plug-in Unit Index
/* IO_PluginUnit_2：原 IO 插件硬件编号，寄存器数组使用编号减 2。 */
#define IO_PluginUnit_2     			3
/* Analog_PluginUnit_3：原 Analog 插件硬件编号，寄存器数组使用编号减 2。 */
#define Analog_PluginUnit_3     	2 
/* MainControl_PluginUnit_4：原 MainControl 插件硬件编号，寄存器数组使用编号减 2。 */
#define MainControl_PluginUnit_4  4 
/* Com_A1_PluginUnit_5：原 Com_A1 插件硬件编号，寄存器数组使用编号减 2。 */
#define Com_A1_PluginUnit_5     	5 
/* Com_A2_PluginUnit_6：原 Com_A2 插件硬件编号，寄存器数组使用编号减 2。 */
#define Com_A2_PluginUnit_6     	6
/* Putout_A_PluginUnit_7：原 Putout_A 插件硬件编号，寄存器数组使用编号减 2。 */
#define Putout_A_PluginUnit_7     7 
/* Input_A_PluginUnit_8：原 Input_A 插件硬件编号，寄存器数组使用编号减 2。 */
#define Input_A_PluginUnit_8     	8 
/* Com_B1_PluginUnit_9：原 Com_B1 插件硬件编号，寄存器数组使用编号减 2。 */
#define Com_B1_PluginUnit_9     	9
/* Com_B2_PluginUnit_10：原 Com_B2 插件硬件编号，寄存器数组使用编号减 2。 */
#define Com_B2_PluginUnit_10      10
/* Putout_B_PluginUnit_11：原 Putout_B 插件硬件编号，寄存器数组使用编号减 2。 */
#define Putout_B_PluginUnit_11    11 
/* Input_B_PluginUnit_12：原 Input_B 插件硬件编号，寄存器数组使用编号减 2。 */
#define Input_B_PluginUnit_12     12 
/* Com_C1_PluginUnit_13：原 Com_C1 插件硬件编号，寄存器数组使用编号减 2。 */
#define Com_C1_PluginUnit_13      13 
/* Com_C2_PluginUnit_14：原 Com_C2 插件硬件编号，寄存器数组使用编号减 2。 */
#define Com_C2_PluginUnit_14      14 
/* Putout_C_PluginUnit_15：原 Putout_C 插件硬件编号，寄存器数组使用编号减 2。 */
#define Putout_C_PluginUnit_15    15 
/* Input_C_PluginUnit_16：原 Input_C 插件硬件编号，寄存器数组使用编号减 2。 */
#define Input_C_PluginUnit_16     16 
/* ComOther_PluginUnit_17：原 ComOther 插件硬件编号，寄存器数组使用编号减 2。 */
#define ComOther_PluginUnit_17    17

/* InvPWM_A_PluginUnit_9：原 InvPWM_A 插件硬件编号，寄存器数组使用编号减 2。 */
#define InvPWM_A_PluginUnit_9     9
/* InvPWM_B_PluginUnit_10：原 InvPWM_B 插件硬件编号，寄存器数组使用编号减 2。 */
#define InvPWM_B_PluginUnit_10    10
/* InvPWM_C_PluginUnit_11：原 InvPWM_C 插件硬件编号，寄存器数组使用编号减 2。 */
#define InvPWM_C_PluginUnit_11    11
/* InvPWM_N_PluginUnit_12：原 InvPWM_N 插件硬件编号，寄存器数组使用编号减 2。 */
#define InvPWM_N_PluginUnit_12    12

//module parameter index
/* ModuleIndex_DCbus_Volt：模块采样类型 直流电压 的索引编号。 */
#define ModuleIndex_DCbus_Volt      1
/* ModuleIndex_DCbus_Current：模块采样类型 直流电流 的索引编号。 */
#define ModuleIndex_DCbus_Current   2
/* ModuleIndex_Port_Current：模块采样类型 端口电流 的索引编号。 */
#define ModuleIndex_Port_Current    3
/* ModuleIndex_Temp：模块采样类型 温度 的索引编号。 */
#define ModuleIndex_Temp            4

//Status
/* PortStatusNum_Initial：原端口状态机 Initial 状态编号，保持兼容数值。 */
#define PortStatusNum_Initial    0x05
/* PortStatusNum_PluseClose：原端口状态机 PluseClose 状态编号，保持兼容数值。 */
#define PortStatusNum_PluseClose 0x07
/* PortStatusNum_Standby：原端口状态机 Standby 状态编号，保持兼容数值。 */
#define PortStatusNum_Standby    0x06
/* PortStatusNum_Run：原端口状态机 Run 状态编号，保持兼容数值。 */
#define PortStatusNum_Run        0x20
/* PortStatusNum_Nonvaild：原端口状态机 Nonvaild 状态编号，保持兼容数值。 */
#define PortStatusNum_Nonvaild   0x3f

/* PortStatusNum_Start：原端口状态机 Start 状态编号，保持兼容数值。 */
#define PortStatusNum_Start                 0x10
/* PortStatusNum_WaitPowerStart：原端口状态机 WaitPowerStart 状态编号，保持兼容数值。 */
#define PortStatusNum_WaitPowerStart        0x09
/* PortStatusNum_Start_SWTable：原端口状态机 Start_SWTable 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_SWTable         0x11
/* PortStatusNum_Start_SWOver：原端口状态机 Start_SWOver 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_SWOver          0x12
/* PortStatusNum_Start_ContactorOn：原端口状态机 Start_ContactorOn 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_ContactorOn     0x13
/* PortStatusNum_Start_ContactorOnOver：原端口状态机 Start_ContactorOnOver 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_ContactorOnOver 0x14
/* PortStatusNum_Start_DABDutyStart：原端口状态机 Start_DABDutyStart 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_DABDutyStart    0x15
/* PortStatusNum_Start_PortStart：原端口状态机 Start_PortStart 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_PortStart       0x16
/* PortStatusNum_Start_JudgeStatus：原端口状态机 Start_JudgeStatus 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_JudgeStatus     0x17
/* PortStatusNum_Start_SVGRun：原端口状态机 Start_SVGRun 状态编号，保持兼容数值。 */
#define PortStatusNum_Start_SVGRun       	0x21

/* PortStatusNum_Stop：原端口状态机 Stop 状态编号，保持兼容数值。 */
#define PortStatusNum_Stop                  0x0d
/* PortStatusNum_Stop_EXTBreakerOff：原端口状态机 Stop_EXTBreakerOff 状态编号，保持兼容数值。 */
#define PortStatusNum_Stop_EXTBreakerOff    0x0e
/* PortStatusNum_Stop_ContactorOff：原端口状态机 Stop_ContactorOff 状态编号，保持兼容数值。 */
#define PortStatusNum_Stop_ContactorOff     0x0f

/* PortStatusNum_ManualStart_PortStart：原端口状态机 ManualStart_PortStart 状态编号，保持兼容数值。 */
#define PortStatusNum_ManualStart_PortStart     0x0a
/* PortStatusNum_ManualStart_SVGStart：原端口状态机 ManualStart_SVGStart 状态编号，保持兼容数值。 */
#define PortStatusNum_ManualStart_SVGStart      0x0b
/* PortStatusNum_ManualStart_SWOver：原端口状态机 ManualStart_SWOver 状态编号，保持兼容数值。 */
#define PortStatusNum_ManualStart_SWOver        0x0c

/* PTCT_FT3：PT/CT FT3 插件选择编号。 */
#define PTCT_FT3  1            //FT3 INDEX
/* FaultSwitch_FT3：故障录波 FT3 插件选择编号。 */
#define FaultSwitch_FT3  2
/* EER2_FT3：EER2 FT3 插件选择编号。 */
#define EER2_FT3  3
/* Backup_FT3：备用 FT3 插件选择编号。 */
#define Backup_FT3  4

 
/************************Variable definition********************************/
typedef union _CRC
{
	unsigned short crc16; /* crc16：CRC 计算缓存的 16 位整字视图。 */
	unsigned char by[2]; /* by：CRC 计算缓存的字节视图，供高低字节访问。 */
} CRC;

//register of Plug-in unit
typedef union _XXG_ID				
{
	struct _ID
	{
		ushort Order:   4; /* Order：位置：0x0~0xF，分别对应着机箱中插件的2~17号位。 */  //位置：0x0~0xF，分别对应着机箱中插件的2~17号位
		ushort Type:    4; /* Type：类型：0x0，主控插件；0x1，光纤通讯插件；0x2，光纤输入插件；0x3，光纤输出插件；0x4，开入开出插件；0x5，AD采样插件；0x6，逆变PWM脉冲插件；0x7，60044-8插件；。 */  //类型：0x0，主控插件；0x1，光纤通讯插件；0x2，光纤输入插件；0x3，光纤输出插件；0x4，开入开出插件；0x5，AD采样插件；0x6，逆变PWM脉冲插件；0x7，60044-8插件；
		ushort Port:    4; /* Port：端口：0x0，高压交流端口HVAC；0x1，高压直流端口HVDC；0x2，低压直流端口LVDC；0x3，低压交流端口LVAC；。 */  //端口：0x0，高压交流端口HVAC；0x1，高压直流端口HVDC；0x2，低压直流端口LVDC；0x3，低压交流端口LVAC；
		ushort Label:   4; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */  //插件标识，读数！=0x6表示此地址没有相应插件
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort ID16; /* ID16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_ID;

typedef union _XXG_LED				
{
	struct _LED
	{
		ushort led0:           1; /* led0：第 0 路插件 LED 控制位，1 点亮、0 熄灭。 */  //当取1时，表示此插件上第x个LED指示灯点亮；
		ushort led1:           1; /* led1：第 1 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led2:           1; /* led2：第 2 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led3:           1; /* led3：第 3 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led4:           1; /* led4：第 4 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led5:           1; /* led5：第 5 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led6:           1; /* led6：第 6 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led7:           1; /* led7：第 7 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort Reservedbit:    8; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort LED16; /* LED16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_LED;

typedef union _XXG_DC_Log
{
	struct _DC_Log
	{
		ushort DC_Order:     	1; /* DC_Order：直流平均电压排序：0——高，1——低。 */  //直流平均电压排序：0——高，1——低
		ushort Reservedbit1:  7; /* Reservedbit1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
		ushort IG_Orient:    	1; /* IG_Orient：电流方向：1:正向 0:负向。 */  //电流方向：1:正向 0:负向
		ushort Reservedbit2:  7; /* Reservedbit2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort DC_Log16; /* DC_Log16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_DC_Log;

typedef union _XXG_PM_CTL
{
	struct _PM_CTL
	{
		ushort SVG_EN:     1; /* SVG_EN：当取1时，表示级联H桥PWM输出使能，取0时表示PWM输出禁止；。 */  //当取1时，表示级联H桥PWM输出使能，取0时表示PWM输出禁止；
		ushort SVG_DIV:    2; /* SVG_DIV：级联H桥开关频率=同步频率*2的SVG_DIV次幂。 */ // 级联H桥开关频率=同步频率*2的SVG_DIV次幂
		ushort SVG_RST:    1; /* SVG_RST：当取1时，级联H桥驱动板RESET；当取0时，无操作；。 */ // 当取1时，级联H桥驱动板RESET；当取0时，无操作；
		ushort DAB_EN:     1; /* DAB_EN：1:DAB H桥PWM输出使能;0:DAB H桥PWM输出禁止;。 */  //1:DAB H桥PWM输出使能;0:DAB H桥PWM输出禁止;
		ushort DAB_DIV:    2; /* DAB_DIV：DAB H桥开关频率=同步频率*2的DAB_DIV次幂。 */ //DAB H桥开关频率=同步频率*2的DAB_DIV次幂
		ushort DAB_RST:    1; /* DAB_RST：1:DAB H桥驱动板RESET;0:无操作；。 */ //1:DAB H桥驱动板RESET;0:无操作；
		ushort HW_RST:     1; /* HW_RST：1:模块控制板故障锁存清除;0:无操作；。 */  //1:模块控制板故障锁存清除;0:无操作；
		ushort FAN_OFF:    1; /* FAN_OFF：1：关闭模块风扇。 */  //1：关闭模块风扇
		ushort SVG_SHUT:   1; /* SVG_SHUT：1:SHUTDOWN光纤熄灭时，级联H桥闭锁。 */  //1:SHUTDOWN光纤熄灭时，级联H桥闭锁
		ushort DAB_SHUT:   1; /* DAB_SHUT：1:SHUTDOWN光纤熄灭时，DAB H桥闭锁。 */  //1:SHUTDOWN光纤熄灭时，DAB H桥闭锁
		ushort ONE:        1; /* ONE：1:单链测试模式，级联H桥有T1和T2两个交点;0:工作模式，级联H桥在TAD内只有一个交点;。 *///1:单链测试模式，级联H桥有T1和T2两个交点;0:工作模式，级联H桥在TAD内只有一个交点;
		ushort CTL:        1; /* CTL：值为0，串行通讯指示，指令为PM_CTL；。 *///值为0，串行通讯指示，指令为PM_CTL；
		ushort LABLE:      2; /* LABLE：值为00，串行通讯指示，控制指令；。 *///值为00，串行通讯指示，控制指令；
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PM_CTL16; /* PM_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PM_CTL;


typedef union _XXG_SwitchINOUT				
{
	struct _SwitchINOUT
	{                                 //四个端口的开关配置情况：四个端口均有1个接触器；HVAC端口有1个内部断路器和1个外部断路器；HVDC端口有1个内部断路器和2个故障电流控制器；LVDC端口有2个故障电流控制器；LVAC端口有1个外部断路器；
		ushort Breaker_OuterOff:  1; /* Breaker_OuterOff：外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；。 */  //外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；
		ushort Breaker_OuterOn:   1; /* Breaker_OuterOn：外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；。 */  //外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；
		ushort Breaker2_OuterOff: 1; /* Breaker2_OuterOff：外部断路器2。 */	//外部断路器2
		ushort Breaker2_OuterOn:   1; /* Breaker2_OuterOn：原第二外部断路器/故障电流控制器合闸控制位。 */  
		ushort Breaker_InnerOff:  1; /* Breaker_InnerOff：内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */  //内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
		ushort Breaker_InnerOn:   1; /* Breaker_InnerOn：内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */  //内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
		
		ushort Reservedbit:      1; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
		ushort Contactor_Inner:   1; /* Contactor_Inner：内部接触器，0表示断；1表示合；。 */  //内部接触器，0表示断；1表示合；

		ushort Invalidbit:        8; /* Invalidbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort SwitchINOUT16; /* SwitchINOUT16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_SwitchINOUT;

typedef union _XXG_INV_CTL
{
	struct _INV_CTL
	{
		ushort TAD_DIV:           3; /* TAD_DIV：逆变模块开关周期=同步周期50us*(TAD_DIV+1)。 */	  //逆变模块开关周期=同步周期50us*(TAD_DIV+1)
		ushort DEADZONE:          10; /* DEADZONE：逆变模块死区时间，0~10.23us。 */	  //逆变模块死区时间，0~10.23us
		ushort Invalidbit:        3; /* Invalidbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort INV_CTL16; /* INV_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_INV_CTL;


typedef union _XXG_BUCK_CTL
{
	struct _BUCK_CTL
	{
		ushort TAD_DIV:           3; /* TAD_DIV：同步周期50us*(TAD_DIV+1)。 */	  //同步周期50us*(TAD_DIV+1)
		ushort Reservedbit:       12; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	  //逆变模块死区时间，0~10.23us
		ushort BUCK_EN:           1; /* BUCK_EN：BUCK PWM 使能位，沿用原 FPGA 有效电平。 */	  //BUCK使能
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort BUCK_CTL16; /* BUCK_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_BUCK_CTL;

typedef union _XXG_DSP_CTL
{
	struct _DSP_CTL
	{	
		ushort SHUTDOWN:           1; /* SHUTDOWN：模块硬件关断位图，保持原 FPGA/DPRAM 字布局。 */	  //1:光纤SHUTDOWN信号，脉冲闭锁
		ushort HW_RST:             1; /* HW_RST：1:RESET主控插件上的FB信号；。 */	  //1:RESET主控插件上的FB信号；
		ushort Invalidbit:        14; /* Invalidbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort DSP_CTL16; /* DSP_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_DSP_CTL;


typedef union _XXG_STATUS
{
	struct _STATUS
	{
		ushort OV:    		1; /* OV：模块过压；。 */  //模块过压；
		ushort OC2:  			1; /* OC2：HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流。 */  //HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流
		ushort OC1:   		1; /* OC1：HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流；。 */  //HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流；
		ushort OT:  	    1; /* OT：模块过温。 */  //模块过温
		ushort SVG_H:  		1; /* SVG_H：工频H桥/Buck桥的驱动板故障。 */  //工频H桥/Buck桥的驱动板故障
		ushort DAB_H:  		1; /* DAB_H：DAB H桥驱动板故障。 */  //DAB H桥驱动板故障
		ushort UV:   			1; /* UV：模块欠压；。 */  //模块欠压；
		ushort SHUT:   		1; /* SHUT：SHUTDOWN光纤闭锁；。 */  //SHUTDOWN光纤闭锁；
		
		ushort OV_L:    	1; /* OV_L：模块过压锁存；。 */  //模块过压锁存；
		ushort OC2_L:			1; /* OC2_L：HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流锁存。 */  //HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流锁存
		ushort OC1_L:			1; /* OC1_L：HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流锁存。 */  //HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流锁存
		ushort OT_L:  		1; /* OT_L：模块过温锁存。 */  //模块过温锁存
		ushort SVG_L:  		1; /* SVG_L：工频H桥/Buck桥的驱动板故障锁存。 */  //工频H桥/Buck桥的驱动板故障锁存
		ushort DAB_L:  		1; /* DAB_L：DAB H桥驱动板故障锁存。 */  //DAB H桥驱动板故障锁存

		ushort Label:   	2; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */	//标识
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort STATUS16; /* STATUS16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_STATUS;


typedef union _XXG_DRIVER
{
	struct _DRIVER
	{
		ushort Driver1_Status:  3; /* Driver1_Status：模块第 1 路驱动状态位。 */  //第一个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Driver2_Status:  3; /* Driver2_Status：模块第 2 路驱动状态位。 */  //第二个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Driver3_Status:  3; /* Driver3_Status：模块第 3 路驱动状态位。 */  //第三个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Driver4_Status:  3; /* Driver4_Status：模块第 4 路驱动状态位。 */  //第四个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Reservedbit:  	2; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
		ushort Label:   	    2; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */	//标识
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort DRIVER16; /* DRIVER16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_DRIVER;


typedef union _XXG_MainStatus				
{
	struct _MainStatus
	{
		ushort P_Status:    6; /* P_Status：端口运行状态:端口运行状态：。 */  //端口运行状态:端口运行状态：
		                       //0x00——初始化中；0x01——初始化成功；0x02——自检开始（有模块上电）；0x03——预充电完成（端口每个模块都上电）；
		                       //0x04——自检失败；0x05——就绪（自检通过）；0x20——正常运行；0x07——停机；0x08：故障停机。
		ushort Sys_Stop:    1; /* Sys_Stop：FT3 录波/端口主状态的系统停机位。 */  //1:整机停止运行；
		ushort Outer_Elec:  1; /* Outer_Elec：FT3 录波/端口主状态的外部电源接入位。 */  //1:外部母线带电；
		ushort P_S2_Mode:  	2; /* P_S2_Mode：端口附加模式二:0X01——注入谐波模式; 0X02——治理谐波模式; 0x00——无附加模式二;。 */  //端口附加模式二:0X01——注入谐波模式; 0X02——治理谐波模式; 0x00——无附加模式二;
		ushort P_S1_Mode:  	2; /* P_S1_Mode：端口附加模式一:0X01——STATCOM无功模式;0X02——STATCOM功率因数模式;0X03——STATCOM电压模式;0x00——无附加模式一。 */  //端口附加模式一:0X01——STATCOM无功模式;0X02——STATCOM功率因数模式;0X03——STATCOM电压模式;0x00——无附加模式一
		ushort P_M_Mode:  	2; /* P_M_Mode：端口主要工作模式:0X00——恒压模式；0X01——恒功率模式；0X02——下垂模式；0X03——跟随模式。 */  //端口主要工作模式:0X00——恒压模式；0X01——恒功率模式；0X02——下垂模式；0X03——跟随模式
		ushort Label:   	2; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */	//标识，值为0x0
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort MainStatus16; /* MainStatus16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_MainStatus;


typedef struct _FPGA_InterfaceBLOCK {
  	XXG_ID ID; /* ID：插件身份、类型、端口及标识寄存器，保持原 FPGA/DPRAM 字布局。 */		            //15:8——0x68,否则表示无插件;7:4——表示插件类型（0：主控；1：光纤通讯；2：光纤输入；3：光纤输出；4：开入开出；5：采样；6：逆变PWM）；3:0——表示插件在机箱中的位置（0~F：对应2~17号位置）
  	XXG_LED LED; /* LED：插件八路 LED 控制寄存器，保持原 FPGA/DPRAM 字布局。 */              //插件上的LED，0~7位有效。主控插件上：bit0——LEDR0  POWER；bit1——LEDB0 RUN；bit2——LEDR1 FAULT；bit3——LEDB1 ALARM
  	ushort NUM; /* NUM：原调制算法在线模块数量寄存器，保持原 FPGA/DPRAM 字布局。 */								//每相总的功率模块链节数量
  	ushort PM; /* PM：本插件在线模块位图，保持原 FPGA/DPRAM 字布局。 */								//光纤插件上连接的功率模块  7:0有效，bitX=1：插件上第X对TX/RX光纤与功率模块连接
  	ushort Reference; /* Reference：工频桥原始调制参考计数，保持原 FPGA/DPRAM 字布局。 */					//参考电压
  	ushort Orient; /* Orient：模块调制/电流方向字段，保持原 FPGA/DPRAM 字布局。 */						
  	XXG_DC_Log DC_Log; /* DC_Log：模块直流电压排序及电流方向控制字，保持原 FPGA/DPRAM 字布局。 */						
  	ushort TAD; /* TAD：FPGA 控制周期计数，保持原 FPGA/DPRAM 字布局。 */								//AD采样周期(us)   控制周期
  	ushort Reserved_1[2]; /* Reserved_1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
  	ushort DAB_LEFTDuty; /* DAB_LEFTDuty：DAB 左桥占空比计数，保持原 FPGA/DPRAM 字布局。 */      //DAB脉冲占空比；
  	ushort DAB_RIGHTDuty; /* DAB_RIGHTDuty：DAB 右桥占空比计数，保持原 FPGA/DPRAM 字布局。 */      //DAB脉冲占空比；
  	ushort DAB_Start; /* DAB_Start：DAB 脉冲起始计数，保持原 FPGA/DPRAM 字布局。 */         // DAB脉冲起始时刻；
    XXG_PM_CTL PM_CTL; /* PM_CTL：工频桥/DAB 使能、分频、复位及测试控制字，保持原 FPGA/DPRAM 字布局。 */                  // 功率模块H桥设置；
  	ushort TX_EN; /* TX_EN：插件发送模块使能位图，保持原 FPGA/DPRAM 字布局。 */  						//串口发送使能   7:0有效，bitX=1：插件上第X号TX串口输出使能
  	ushort TX_CTL; /* TX_CTL：插件发送控制寄存器，保持原 FPGA/DPRAM 字布局。 */						//串口发送控制   7:0有效，bitX=1：插件上第X号TX串口发送数据START,自动清零
  	ushort TX[8]; /* TX：插件原始发送数据区，保持原 FPGA/DPRAM 字布局。 */					    //光纤插件串口发送数据
  	ushort RX[8]; /* RX：插件原始接收数据区，保持原 FPGA/DPRAM 字布局。 */                       //光纤插件串口接收数据；
  	XXG_SwitchINOUT DOUT; /* DOUT：数字输出控制字，保持原 FPGA/DPRAM 字布局。 */  		//开入开出插件开关量输出  7:0有效，bitX=1：插件上第X通道输出继电器闭合
  	ushort AD_PROT_EN; /* AD_PROT_EN：端口 AD 保护使能位图，保持原 FPGA/DPRAM 字布局。 */        //AD插件通道保护使能, bitX=1：AD采样插件上第X通道硬件保护使能
  	ushort SHUTDOWN; /* SHUTDOWN：模块硬件关断位图，保持原 FPGA/DPRAM 字布局。 */          //光纤输出插件SHUTDOWN输出, bitX=1：光纤输出插件上第X通道光纤熄灭
  	ushort FO_EN; /* FO_EN：光纤故障输入使能位图，保持原 FPGA/DPRAM 字布局。 */			  //光纤输入插件FO使能, bitX=1：光纤输入插件上第X通道光纤FO信号使能
  	ushort INV_SHUTDOWN; /* INV_SHUTDOWN：独立逆变桥关断位图，保持原 FPGA/DPRAM 字布局。 */       //逆变单元插件PWM闭锁；
  	ushort INV_FO_EN; /* INV_FO_EN：独立逆变桥光纤故障使能位图，保持原 FPGA/DPRAM 字布局。 */          //逆变单元插件FO使能；
  	ushort INV_REF; /* INV_REF：独立逆变桥原始调制参考计数，保持原 FPGA/DPRAM 字布局。 */            //逆变ABCN插件PWM参考值；
  	XXG_INV_CTL INV_CTL; /* INV_CTL：独立逆变桥周期/死区控制字，保持原 FPGA/DPRAM 字布局。 */			//逆变控制
  	ushort BUCK_DUTY; /* BUCK_DUTY：BUCK 占空比计数，保持原 FPGA/DPRAM 字布局。 */         //BUCK桥占空比
  	XXG_BUCK_CTL BUCK_CTL; /* BUCK_CTL：BUCK 使能及周期控制字，保持原 FPGA/DPRAM 字布局。 */    //BUCK桥控制
  	ushort Reserved_2[6]; /* Reserved_2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */    //预留
  	ushort DA_DATA0; /* DA_DATA0：第 0 路 DA 调试输出原始计数寄存器。 */         	//DA0
  	ushort DA_DATA1; /* DA_DATA1：第 1 路 DA 调试输出原始计数寄存器。 */         	//DA1
  	ushort DA_DATA2; /* DA_DATA2：第 2 路 DA 调试输出原始计数寄存器。 */         	//DA2
  	ushort DA_DATA3; /* DA_DATA3：第 3 路 DA 调试输出原始计数寄存器。 */         	//DA3
  	ushort Reserved_3[4]; /* Reserved_3：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */    //预留
  	ushort BKU; /* BKU：BKU 故障锁存控制寄存器，保持原 FPGA/DPRAM 字布局。 */               //主控插件至各插件输出BK信号，bit0~15对应2~17号插件
  	ushort FB_EN; /* FB_EN：FB 硬件故障输入使能位图，保持原 FPGA/DPRAM 字布局。 */  	          //主控插件接收各插件FB信号使能,bit0~15对应2~17号插件
  	ushort TEST3A; /* TEST3A：原 FPGA 测试寄存器 A，保持原 FPGA/DPRAM 字布局。 */            //读写测试单元；
  	ushort TEST3B; /* TEST3B：原 FPGA 测试寄存器 B，保持原 FPGA/DPRAM 字布局。 */            //读写测试单元；
  	ushort Debug_L; /* Debug_L：左桥 FPGA 调制调试寄存器，保持原 FPGA/DPRAM 字布局。 */           //调试输出左桥臂的脉冲和；
  	ushort Debug_R; /* Debug_R：右桥 FPGA 调制调试寄存器，保持原 FPGA/DPRAM 字布局。 */           //调试输出右桥臂的脉冲和；
  	ushort Reserved_4[1]; /* Reserved_4：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
  	XXG_DSP_CTL DSP_CTL; /* DSP_CTL：DSP 全关断及硬件复位控制字，保持原 FPGA/DPRAM 字布局。 */  	  //DSP控制字
   	XXG_MainStatus HVAC_STATUS; /* HVAC_STATUS：主控插件HVAC端口运行状态。 */       //主控插件HVAC端口运行状态
  	short HVAC_DV; /* HVAC_DV：主控插件HVAC端口数据1。 */            //主控插件HVAC端口数据1
  	short HVAC_DI; /* HVAC_DI：主控插件HVAC端口数据2。 */						//主控插件HVAC端口数据2
  	short HVAC_DP; /* HVAC_DP：主控插件HVAC端口数据3。 */						//主控插件HVAC端口数据3
  	short HVAC_DV1; /* HVAC_DV1：主控插件HVAC端口数据1。 */           //主控插件HVAC端口数据1
  	short HVAC_DI1; /* HVAC_DI1：主控插件HVAC端口数据2。 */						//主控插件HVAC端口数据2
  	short HVAC_DP1; /* HVAC_DP1：主控插件HVAC端口数据3。 */						//主控插件HVAC端口数据3
  	short HVAC_DV2; /* HVAC_DV2：主控插件HVAC端口数据1。 */           //主控插件HVAC端口数据1
  	XXG_MainStatus HVDC_STATUS; /* HVDC_STATUS：主控插件HVDC端口运行状态。 */		  //主控插件HVDC端口运行状态
  	short HVDC_DV; /* HVDC_DV：主控插件HVDC端口数据1。 */						//主控插件HVDC端口数据1
  	short HVDC_DI; /* HVDC_DI：主控插件HVDC端口数据2。 */						//主控插件HVDC端口数据2
  	short HVDC_DP; /* HVDC_DP：主控插件HVDC端口数据3。 */						//主控插件HVDC端口数据3
  	short HVDC_DV1; /* HVDC_DV1：主控插件HVDC端口数据1。 */						//主控插件HVDC端口数据1
  	short HVDC_DI1; /* HVDC_DI1：主控插件HVDC端口数据2。 */						//主控插件HVDC端口数据2
  	short HVDC_DP1; /* HVDC_DP1：主控插件HVDC端口数据3。 */						//主控插件HVDC端口数据3
  	short HVDC_DV2; /* HVDC_DV2：主控插件HVDC端口数据1。 */						//主控插件HVDC端口数据1
  	XXG_MainStatus LVDC_STATUS; /* LVDC_STATUS：主控插件LVDC端口运行状态。 */		  //主控插件LVDC端口运行状态
  	short LVDC_DV; /* LVDC_DV：主控插件LVDC端口数据1。 */						//主控插件LVDC端口数据1
  	short LVDC_DI; /* LVDC_DI：主控插件LVDC端口数据2。 */						//主控插件LVDC端口数据2
  	short LVDC_DP; /* LVDC_DP：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVDC_DV1; /* LVDC_DV1：主控插件LVDC端口数据1。 */						//主控插件LVDC端口数据1
  	short LVDC_DI1; /* LVDC_DI1：主控插件LVDC端口数据2。 */						//主控插件LVDC端口数据2
  	short LVDC_DP1; /* LVDC_DP1：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVDC_DV2; /* LVDC_DV2：主控插件LVDC端口数据1。 */						//主控插件LVDC端口数据1
  	XXG_MainStatus LVAC_STATUS; /* LVAC_STATUS：主控插件LVAC端口运行状态。 */		  //主控插件LVAC端口运行状态
  	short LVAC_DV; /* LVAC_DV：主控插件LVAC端口数据1。 */						//主控插件LVAC端口数据1
  	short LVAC_DI; /* LVAC_DI：主控插件LVAC端口数据2。 */						//主控插件LVAC端口数据2
  	short LVAC_DP; /* LVAC_DP：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVAC_DV1; /* LVAC_DV1：主控插件LVAC端口数据1。 */						//主控插件LVAC端口数据1
  	short LVAC_DI1; /* LVAC_DI1：主控插件LVAC端口数据2。 */						//主控插件LVAC端口数据2
  	short LVAC_DP1; /* LVAC_DP1：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVAC_DV2; /* LVAC_DV2：主控插件LVAC端口数据1。 */						//主控插件LVAC端口数据1
  	short AD[16]; /* AD：端口原始 AD 实时采样数组，保持原 FPGA/DPRAM 字布局。 */            //AD插件AD各通道数据
  	short AD_LATCH[16]; /* AD_LATCH：端口原始 AD 锁存采样数组，供故障/同步采样使用。 */      //故障锁存AD插件各通道数据——插件接收到BKU信号时，锁存的采样数据；
  	
  	
  	short DC[8]; /* DC：模块直流电压原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */			        //模块直流电压；16位有符号整数；
  	short DC_LATCH[8]; /* DC_LATCH：模块直流电压原始锁存采样数组，供故障/同步采样使用。 */	      //故障锁存模块直流电压；16位有符号整数；
  	short IR[8]; /* IR：模块直流电流原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */              //备用通道测量的电流；16位有符号整数；                      在LVDC模块中，上送的第二路直流电容电压
  	short IR_LATCH[8]; /* IR_LATCH：模块直流电流原始锁存采样数组，供故障/同步采样使用。 */        //故障锁存备用通道测量的电流；16位有符号整数；              在LVDC模块中，上送的第二路直流电容电压
  	short IG[8]; /* IG：模块交流/端口电流原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */              //模块电流；16位有符号整数；
  	short IG_LATCH[8]; /* IG_LATCH：模块交流/端口电流原始锁存采样数组，供故障/同步采样使用。 */        //故障锁存模块电流；16位有符号整数；
  	short TEMP[8]; /* TEMP：模块温度原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */            //模块温度；16位有符号整数；
  	short TEMP_LATCH[8]; /* TEMP_LATCH：模块温度原始锁存采样数组，供故障/同步采样使用。 */      //故障锁存模块温度；16位有符号整数；
  	XXG_STATUS STATUS[8]; /* STATUS：各模块原始实时状态字数组，保持原 FPGA/DPRAM 字布局。 */         //模块状态
//  	XXG_STATUS STATUS_LATCH[8];   //故障锁存模块状态
  	short HD_S_PH[8]; /* HD_S_PH：原 HVDC 同步相位字段，保持原 FPGA/DPRAM 字布局。 */ 
  	
  	XXG_DRIVER DRIVER[8]; /* DRIVER：各模块驱动原始状态字数组，保持原 FPGA/DPRAM 字布局。 */         //驱动板状态
//  	XXG_DRIVER DRIVER_LATCH[8];   //故障锁存驱动板状态
  	ushort Reserved_6[8]; /* Reserved_6：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
  	
  	XXG_SwitchINOUT DIN; /* DIN：端口数字输入实时位图，保持原 FPGA/DPRAM 字布局。 */             //开入开出插件开关量输入状态
  	XXG_SwitchINOUT DIN_LATCH; /* DIN_LATCH：端口数字输入锁存位图，供故障/同步采样使用。 */       //故障锁存开入开出插件输入状态
  	ushort AD_PROT; /* AD_PROT：端口 AD 保护实时故障位图，保持原 FPGA/DPRAM 字布局。 */                  //AD插件通道保护状态
  	ushort AD_PROT_LATCH; /* AD_PROT_LATCH：端口 AD 保护锁存故障位图，供故障/同步采样使用。 */			 //故障锁存AD插件通道保护状态
  	ushort FO; /* FO：光纤故障输入实时位图，保持原 FPGA/DPRAM 字布局。 */                //光纤输入插件FO状态
  	ushort FO_LATCH; /* FO_LATCH：光纤故障输入锁存位图，供故障/同步采样使用。 */				  //故障锁存光纤输入插件FO状态，即插件接收到BKU信号时，锁存的FO状态
  	ushort FB; /* FB：FB 硬件故障输入实时位图，保持原 FPGA/DPRAM 字布局。 */						    //主控插件接收各插件FB状态,bit0~15对应2~17号插件
  	ushort FB_LATCH; /* FB_LATCH：FB 硬件故障输入锁存位图，供故障/同步采样使用。 */				  //故障锁存主控插件FB状态,bit0~15对应2~17号插件
  	ushort Reserved_7[8]; /* Reserved_7：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
    XXG_STATUS OR_STATUS; /* OR_STATUS：模块状态按位或的汇总值，保持原 FPGA/DPRAM 字布局。 */
		ushort Reserved_8[3]; /* Reserved_8：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
		ushort Left_Ref; /* Left_Ref：左桥实际参考回读，保持原 FPGA/DPRAM 字布局。 */
		ushort Right_Ref; /* Right_Ref：右桥实际参考回读，保持原 FPGA/DPRAM 字布局。 */
		ushort Reserved_9[2]; /* Reserved_9：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

  	ushort INV_FO; /* INV_FO：独立逆变桥光纤故障实时位图，保持原 FPGA/DPRAM 字布局。 */				        //A/B/C/N逆变桥驱动板FO状态,依次表示16个IGBT的FO状态
  	ushort INV_FO_LATCH; /* INV_FO_LATCH：独立逆变桥光纤故障锁存位图，供故障/同步采样使用。 */          //逆变桥驱动板FO状态锁存
  	short Sum_DC; /* Sum_DC：本插件模块直流电压采样总和，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件直流电压之和
  	short Avg_DC; /* Avg_DC：本插件模块直流电压采样平均值，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件上直流电压平均值
  	short Sum_IG; /* Sum_IG：本插件模块交流电流采样总和，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件模块电流之和
  	short Avg_IG; /* Avg_IG：本插件模块交流电流采样平均值，保持原 FPGA/DPRAM 字布局。 */                      //各光纤插件模块电流平均值
  	short Sum_IR; /* Sum_IR：本插件模块直流电流采样总和，保持原 FPGA/DPRAM 字布局。 */                      //各光纤插件备用通道电流之和
  	short Avg_IR; /* Avg_IR：本插件模块直流电流采样平均值，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件备用通道电流平均值

} FPGA_InterfaceBLOCK, *P_FPGA_InterfaceBLOCK;//FPGA参数设置区

extern volatile FPGA_InterfaceBLOCK FPGA_Interface[15]; /* FPGA_Interface：15 个插件的 EMIFA 映射寄存器窗口，索引为原插件编号减 2。 */
extern XXG_MainStatus MainStatus_HVAC,MainStatus_HVDC,MainStatus_LVDC,MainStatus_LVAC,Port_MainStatus; /* MainStatus_HVAC：HVAC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* MainStatus_HVDC：HVDC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* MainStatus_LVDC：LVDC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* MainStatus_LVAC：LVAC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
/* Port_MainStatus：当前端口主状态、主/子工作模式的位域缓存。 */

extern XXG_MainStatus Port_MainStatus; /* Port_MainStatus：当前端口主状态、主/子工作模式的位域缓存。 */
extern ushort Port_Main_WorkMode; /* Port_Main_WorkMode：当前端口主工作模式编号。 */
extern ushort Port_STATCOM_WorkMode; /* Port_STATCOM_WorkMode：旧 STATCOM 无功/功率因数/电压工作模式编号。 */
extern ushort Port_APF_WorkMode; /* Port_APF_WorkMode：旧 APF 谐波补偿工作模式编号。 */
extern ushort Port_Outer_Elec; /* Port_Outer_Elec：端口外部电源接入状态。 */
extern ushort Port_Sys_Stop; /* Port_Sys_Stop：旧系统停机状态。 */
extern ushort Port_State; /* Port_State：旧端口状态编号，迁移核心维护兼容镜像。 */
extern ushort Port_RunStatus; /* Port_RunStatus：旧端口运行状态编号。 */

extern XXG_DSP_CTL Port_DSP_CTL; /* Port_DSP_CTL：端口 FPGA 全关断及硬件复位控制字缓存。 */
extern XXG_PM_CTL Module_PM_CTL; /* Module_PM_CTL：模块工频桥/DAB 使能、驱动复位及测试控制字缓存。 */
extern XXG_SwitchINOUT Port_DOUT; /* Port_DOUT：端口断路器、接触器数字输出控制字缓存。 */


//register of Plug-in unit in DPRAM------remote measurement
typedef union _XXG_DPRAM_PluginUnit{
	struct _DPRAM_PluginUnit {

  	short DC[8]; /* DC：模块直流电压原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */			        //模块直流电压；16位有符号整数；
  	short DC_LATCH[8]; /* DC_LATCH：模块直流电压原始锁存采样数组，供故障/同步采样使用。 */	      //故障锁存模块直流电压；16位有符号整数；
  	short IR[8]; /* IR：模块直流电流原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */              //备用通道测量的电流；16位有符号整数；                      在LVDC模块中，上送的第二路直流电容电压
  	short IR_LATCH[8]; /* IR_LATCH：模块直流电流原始锁存采样数组，供故障/同步采样使用。 */        //故障锁存备用通道测量的电流；16位有符号整数；              在LVDC模块中，上送的第二路直流电容电压
  	short IG[8]; /* IG：模块交流/端口电流原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */              //模块电流；16位有符号整数；
  	short IG_LATCH[8]; /* IG_LATCH：模块交流/端口电流原始锁存采样数组，供故障/同步采样使用。 */        //故障锁存模块电流；16位有符号整数；
  	short TEMP[8]; /* TEMP：模块温度原始实时采样数组，保持原 FPGA/DPRAM 字布局。 */            //模块温度；16位有符号整数；
  	short TEMP_LATCH[8]; /* TEMP_LATCH：模块温度原始锁存采样数组，供故障/同步采样使用。 */      //故障锁存模块温度；16位有符号整数；
  	XXG_STATUS STATUS[8]; /* STATUS：各模块原始实时状态字数组，保持原 FPGA/DPRAM 字布局。 */         //模块状态
//  	XXG_STATUS STATUS_LATCH[8];   //故障锁存模块状态
  	short HD_S_PH[8]; /* HD_S_PH：原 HVDC 同步相位字段，保持原 FPGA/DPRAM 字布局。 */ 
  	XXG_DRIVER DRIVER[8]; /* DRIVER：各模块驱动原始状态字数组，保持原 FPGA/DPRAM 字布局。 */         //驱动板状态
//  	XXG_DRIVER DRIVER_LATCH[8];   //故障锁存驱动板状态
  	ushort Reserved_6[8]; /* Reserved_6：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
  	
  	XXG_SwitchINOUT DIN; /* DIN：端口数字输入实时位图，保持原 FPGA/DPRAM 字布局。 */             //开入开出插件开关量输入状态
  	XXG_SwitchINOUT DIN_LATCH; /* DIN_LATCH：端口数字输入锁存位图，供故障/同步采样使用。 */       //故障锁存开入开出插件输入状态
  	ushort AD_PROT; /* AD_PROT：端口 AD 保护实时故障位图，保持原 FPGA/DPRAM 字布局。 */                  //AD插件通道保护状态
  	ushort AD_PROT_LATCH; /* AD_PROT_LATCH：端口 AD 保护锁存故障位图，供故障/同步采样使用。 */			 //故障锁存AD插件通道保护状态
  	ushort FO; /* FO：光纤故障输入实时位图，保持原 FPGA/DPRAM 字布局。 */                //光纤输入插件FO状态
  	ushort FO_LATCH; /* FO_LATCH：光纤故障输入锁存位图，供故障/同步采样使用。 */				  //故障锁存光纤输入插件FO状态，即插件接收到BKU信号时，锁存的FO状态
  	ushort FB; /* FB：FB 硬件故障输入实时位图，保持原 FPGA/DPRAM 字布局。 */						    //主控插件接收各插件FB状态,bit0~15对应2~17号插件
  	ushort FB_LATCH; /* FB_LATCH：FB 硬件故障输入锁存位图，供故障/同步采样使用。 */				  //故障锁存主控插件FB状态,bit0~15对应2~17号插件
  	ushort Reserved_7[8]; /* Reserved_7：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
    XXG_STATUS OR_STATUS; /* OR_STATUS：模块状态按位或的汇总值，保持原 FPGA/DPRAM 字布局。 */
		ushort Reserved_8[3]; /* Reserved_8：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
		ushort Left_Ref; /* Left_Ref：左桥实际参考回读，保持原 FPGA/DPRAM 字布局。 */
		ushort Right_Ref; /* Right_Ref：右桥实际参考回读，保持原 FPGA/DPRAM 字布局。 */
		ushort Reserved_9[2]; /* Reserved_9：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

  	ushort INV_FO; /* INV_FO：独立逆变桥光纤故障实时位图，保持原 FPGA/DPRAM 字布局。 */				        //A/B/C/N逆变桥驱动板FO状态,依次表示16个IGBT的FO状态
  	ushort INV_FO_LATCH; /* INV_FO_LATCH：独立逆变桥光纤故障锁存位图，供故障/同步采样使用。 */          //逆变桥驱动板FO状态锁存
  	short Sum_DC; /* Sum_DC：本插件模块直流电压采样总和，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件直流电压之和
  	short Avg_DC; /* Avg_DC：本插件模块直流电压采样平均值，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件上直流电压平均值
  	short Sum_IG; /* Sum_IG：本插件模块交流电流采样总和，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件模块电流之和
  	short Avg_IG; /* Avg_IG：本插件模块交流电流采样平均值，保持原 FPGA/DPRAM 字布局。 */                      //各光纤插件模块电流平均值
  	short Sum_IR; /* Sum_IR：本插件模块直流电流采样总和，保持原 FPGA/DPRAM 字布局。 */                      //各光纤插件备用通道电流之和
  	short Avg_IR; /* Avg_IR：本插件模块直流电流采样平均值，保持原 FPGA/DPRAM 字布局。 */						//各光纤插件备用通道电流平均值

	} DPRAM_PluginUnit; /* DPRAM_PluginUnit：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */
	ushort DPRAM_Block4Buf[128]; /* DPRAM_Block4Buf：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
}XXG_DPRAM_PluginUnit;//DPRAM的2-17号插件结构体

extern volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit2,DPRAM_Block4_PluginUnit3,DPRAM_Block4_PluginUnit4,DPRAM_Block4_PluginUnit5,DPRAM_Block4_PluginUnit6; /* DPRAM_Block4_PluginUnit2：原地址映射的插件 2 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit3：原地址映射的插件 3 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit4：原地址映射的插件 4 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit5：原地址映射的插件 5 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit6：原地址映射的插件 6 遥测块，仅启用 DPRAM 时访问。 */
extern volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit7,DPRAM_Block4_PluginUnit8,DPRAM_Block4_PluginUnit9,DPRAM_Block4_PluginUnit10,DPRAM_Block4_PluginUnit11; /* DPRAM_Block4_PluginUnit7：原地址映射的插件 7 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit8：原地址映射的插件 8 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit9：原地址映射的插件 9 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit10：原地址映射的插件 10 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit11：原地址映射的插件 11 遥测块，仅启用 DPRAM 时访问。 */
extern volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit12,DPRAM_Block4_PluginUnit13,DPRAM_Block4_PluginUnit14,DPRAM_Block4_PluginUnit15,DPRAM_Block4_PluginUnit16,DPRAM_Block4_PluginUnit17; /* DPRAM_Block4_PluginUnit12：原地址映射的插件 12 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit13：原地址映射的插件 13 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit14：原地址映射的插件 14 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit15：原地址映射的插件 15 遥测块，仅启用 DPRAM 时访问。 */
/* DPRAM_Block4_PluginUnit16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
/* DPRAM_Block4_PluginUnit17：原地址映射的插件 17 遥测块，仅启用 DPRAM 时访问。 */

//extern volatile XXG_DPRAM_PluginUnit FPGA_PU_1,FPGA_PU_2,FPGA_PU_3,FPGA_PU_4,FPGA_PU_5,FPGA_PU_6,FPGA_PU_7,FPGA_PU_8,FPGA_PU_9,FPGA_PU_10,FPGA_PU_11,FPGA_PU_12,FPGA_PU_13,FPGA_PU_14,FPGA_PU_15,FPGA_PU_16;

//remote measurement in DPRAM
typedef union _XXG_PORT_STATUS
{
	struct _PORT_STATUS
	{
		ushort Contactor_Inner:    1; /* Contactor_Inner：内部接触器，0表示断；1表示合；。 */  //内部接触器，0表示断；1表示合；
		ushort Breaker_InnerOn:    1; /* Breaker_InnerOn：内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */  //内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
		ushort Breaker_InnerOff:   1; /* Breaker_InnerOff：内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */  //内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
		ushort Breaker_OuterOn:    1; /* Breaker_OuterOn：外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；。 */  //外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；
		ushort Breaker_OuterOff:   1; /* Breaker_OuterOff：外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；。 */  //外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；
    ushort GND_Switch:   1; /* GND_Switch：接地开关；HVAC端口：1表示合，0表示分。 */            //接地开关；HVAC端口：1表示合，0表示分
    ushort ISO_Switch:   1; /* ISO_Switch：隔离开关；HVAC端口：1表示合，0表示分。 */            //隔离开关；HVAC端口：1表示合，0表示分
    ushort Module_Hbridge:     1; /* Module_Hbridge：1:模块H桥闭锁，0：模块H桥解锁；。 */  //1:模块H桥闭锁，0：模块H桥解锁；
		ushort LF_Hbridge:         1; /* LF_Hbridge：1:工频H桥闭锁，0：工频H桥解锁；。 */  //1:工频H桥闭锁，0：工频H桥解锁；
		ushort HF_Hbridge:  	   1; /* HF_Hbridge：1:高频H桥闭锁，0：高频H桥解锁；。 */  //1:高频H桥闭锁，0：高频H桥解锁；
		ushort SWITCH_ON_EN:       1; /* SWITCH_ON_EN：1:端口外部断路器闭合使能，0：禁止；。 */  //1:端口外部断路器闭合使能，0：禁止；
		ushort Reservedbit2:       3; /* Reservedbit2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
		ushort S_Mode:  	       2; /* S_Mode：系统运行方式：0x00表示自动运行，0x01表示自动测试，0x02表示手动运行，0x03表示手动测试；。 */  //系统运行方式：0x00表示自动运行，0x01表示自动测试，0x02表示手动运行，0x03表示手动测试；
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PORT_STATUS16; /* PORT_STATUS16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_STATUS;

typedef union _XXG_PORT_FAULT
{
	struct _PORT_FAULT
	{
		ushort PORT_FAULT_bit0:         1; /* PORT_FAULT_bit0：1:表示接触器异常。 */      //1:表示接触器异常
		ushort PORT_FAULT_bit1:         1; /* PORT_FAULT_bit1：1:表示断路器异常。 */      //1:表示断路器异常
		ushort PORT_FAULT_bit2:         1; /* PORT_FAULT_bit2：1:表示风机故障。 */      //1:表示风机故障
		ushort PORT_FAULT_bit3:  	    1; /* PORT_FAULT_bit3：1:表示控制器故障。 */      //1:表示控制器故障
		ushort PORT_FAULT_bit4:         1; /* PORT_FAULT_bit4：1:表示辅助供电。 */      //1:表示辅助供电
		ushort PORT_FAULT_bit5:         1; /* PORT_FAULT_bit5：1:表示EPO急停。 */      //1:表示EPO急停
		ushort PORT_FAULT_bit6:  	    1; /* PORT_FAULT_bit6：1:表示模块故障。 */      //1:表示模块故障
		ushort PORT_FAULT_bit7:         1; /* PORT_FAULT_bit7：1:表示逆变器故障。 */      //1:表示逆变器故障
		ushort PORT_FAULT_bit8:         1; /* PORT_FAULT_bit8：1:表示相序错误。 */      //1:表示相序错误
		ushort PORT_FAULT_bit9:  	    1; /* PORT_FAULT_bit9：1:表示频率越限。 */      //1:表示频率越限
		ushort PORT_FAULT_bit10:        1; /* PORT_FAULT_bit10：1:表示过流。 */      //1:表示过流
		ushort PORT_FAULT_bit11:        1; /* PORT_FAULT_bit11：1:表示欠压。 */      //1:表示欠压
		ushort PORT_FAULT_bit12:  	    1; /* PORT_FAULT_bit12：1:表示过压。 */      //1:表示过压
		ushort PORT_FAULT_bit13:        1; /* PORT_FAULT_bit13：1:表示通讯故障。 */      //1:表示通讯故障
		ushort PORT_FAULT_bit14:        1; /* PORT_FAULT_bit14：1:表示SYNC异常。 */      //1:表示SYNC异常

		ushort PORT_FAULT_bit15:        1; /* PORT_FAULT_bit15：1:表示端口有故障；0：表示端口无故障；。 */      //1:表示端口有故障；0：表示端口无故障；
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PORT_FAULT16; /* PORT_FAULT16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_FAULT;

typedef union _XXG_DPRAM_RemMeaSig{
	struct _DPRAM_RemMeaSig {

	XXG_MainStatus PORT_RUN_STATUS; /* PORT_RUN_STATUS：端口运行状态。 */              //端口运行状态
	XXG_PORT_STATUS PORT_STATUS; /* PORT_STATUS：端口开关及桥状态的原 16 位位域字。 */
	XXG_PORT_FAULT PORT_FAULT; /* PORT_FAULT：端口故障的原 16 位位域字，位定义随旧端口分支解释。 */
  	short PORT_P; /* PORT_P：端口有功功率；float 为物理值，16 位版本为旧遥测缩放编码。 */
  	short PORT_Q; /* PORT_Q：端口无功功率；float 为物理值，16 位版本为旧遥测缩放编码。 */
  	short PORT_V_A; /* PORT_V_A：端口 A 相/中线电压遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	short PORT_V_B; /* PORT_V_B：端口 B 相/中线电压遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	short PORT_V_C; /* PORT_V_C：端口 C 相/中线电压遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	short PORT_I_A; /* PORT_I_A：端口 A 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	short PORT_I_B; /* PORT_I_B：端口 B 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	short PORT_I_C; /* PORT_I_C：端口 C 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	short PORT_I_N; /* PORT_I_N：端口 N 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	short V_A_2; /* V_A_2：端口第二组 A 相电压遥测；16 位版比例见 RemoteSig_Dispose。 */
  	short V_B_2; /* V_B_2：端口第二组 B 相电压遥测；16 位版比例见 RemoteSig_Dispose。 */
  	short V_C_2; /* V_C_2：端口第二组 C 相电压遥测；16 位版比例见 RemoteSig_Dispose。 */
  	short I_A_2; /* I_A_2：端口第二组 A 相电流遥测；16 位版比例见 RemoteSig_Dispose。 */
  	short I_B_2; /* I_B_2：端口第二组 B 相电流遥测；16 位版比例见 RemoteSig_Dispose。 */
  	short I_C_2; /* I_C_2：端口第二组 C 相电流遥测；16 位版比例见 RemoteSig_Dispose。 */
  	short DC_V; /* DC_V：端口直流电压；float 为 V，16 位版使用旧协议缩放格式。 */
  	short RMS_CURRENT_5TH; /* RMS_CURRENT_5TH：5 次谐波电流有效值遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	short RMS_CURRENT_7TH; /* RMS_CURRENT_7TH：7 次谐波电流有效值遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	short RMS_CURRENT_11TH; /* RMS_CURRENT_11TH：11 次谐波电流有效值遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	short PHA_CURRENT_5TH; /* PHA_CURRENT_5TH：5 次谐波电流相位遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	short PHA_CURRENT_7TH; /* PHA_CURRENT_7TH：7 次谐波电流相位遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	short PHA_CURRENT_11TH; /* PHA_CURRENT_11TH：11 次谐波电流相位遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	short PORT_PF; /* PORT_PF：端口功率因数；16 位版使用旧协议缩放格式。 */
  	short PORT_V0; /* PORT_V0：旧下垂控制的电压基准量，编码比例见 RemoteSig_Dispose。 */
  	short PORT_F0; /* PORT_F0：旧下垂控制的频率基准量，编码比例见 RemoteSig_Dispose。 */
  	short PORT_KF; /* PORT_KF：旧频率下垂系数，编码比例见 RemoteSig_Dispose。 */
  	short PORT_KV; /* PORT_KV：旧电压下垂系数，编码比例见 RemoteSig_Dispose。 */
  	short Reserved_1[3]; /* Reserved_1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
  } DPRAM_RemMeaSig; /* DPRAM_RemMeaSig：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 *///DPRAM的第3分区的结构体
  ushort DPRAM_Block3Buf[DPRAMBlock3_Length]; /* DPRAM_Block3Buf：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
}XXG_DPRAM_RemMeaSig;


typedef struct _DPRAM_RemMeaSigFloat {

	XXG_MainStatus PORT_RUN_STATUS; /* PORT_RUN_STATUS：端口运行状态。 */              //端口运行状态
	XXG_PORT_STATUS PORT_STATUS; /* PORT_STATUS：端口开关及桥状态的原 16 位位域字。 */
	XXG_PORT_FAULT PORT_FAULT; /* PORT_FAULT：端口故障的原 16 位位域字，位定义随旧端口分支解释。 */
  	float PORT_P; /* PORT_P：端口有功功率；float 为物理值，16 位版本为旧遥测缩放编码。 */
  	float PORT_Q; /* PORT_Q：端口无功功率；float 为物理值，16 位版本为旧遥测缩放编码。 */
  	float PORT_V_A; /* PORT_V_A：端口 A 相/中线电压遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	float PORT_V_B; /* PORT_V_B：端口 B 相/中线电压遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	float PORT_V_C; /* PORT_V_C：端口 C 相/中线电压遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	float PORT_I_A; /* PORT_I_A：端口 A 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	float PORT_I_B; /* PORT_I_B：端口 B 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	float PORT_I_C; /* PORT_I_C：端口 C 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	float PORT_I_N; /* PORT_I_N：端口 N 相/中线电流遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose。 */
  	float V_A_2; /* V_A_2：端口第二组 A 相电压遥测；16 位版比例见 RemoteSig_Dispose。 */
  	float V_B_2; /* V_B_2：端口第二组 B 相电压遥测；16 位版比例见 RemoteSig_Dispose。 */
  	float V_C_2; /* V_C_2：端口第二组 C 相电压遥测；16 位版比例见 RemoteSig_Dispose。 */
  	float I_A_2; /* I_A_2：端口第二组 A 相电流遥测；16 位版比例见 RemoteSig_Dispose。 */
  	float I_B_2; /* I_B_2：端口第二组 B 相电流遥测；16 位版比例见 RemoteSig_Dispose。 */
  	float I_C_2; /* I_C_2：端口第二组 C 相电流遥测；16 位版比例见 RemoteSig_Dispose。 */
  	float DC_V; /* DC_V：端口直流电压；float 为 V，16 位版使用旧协议缩放格式。 */
  	float RMS_CURRENT_5TH; /* RMS_CURRENT_5TH：5 次谐波电流有效值遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	float RMS_CURRENT_7TH; /* RMS_CURRENT_7TH：7 次谐波电流有效值遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	float RMS_CURRENT_11TH; /* RMS_CURRENT_11TH：11 次谐波电流有效值遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	float PHA_CURRENT_5TH; /* PHA_CURRENT_5TH：5 次谐波电流相位遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	float PHA_CURRENT_7TH; /* PHA_CURRENT_7TH：7 次谐波电流相位遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	float PHA_CURRENT_11TH; /* PHA_CURRENT_11TH：11 次谐波电流相位遥测；定点/浮点比例见旧遥调/遥测转换。 */
  	float PORT_PF; /* PORT_PF：端口功率因数；16 位版使用旧协议缩放格式。 */
  	float PORT_V0; /* PORT_V0：旧下垂控制的电压基准量，编码比例见 RemoteSig_Dispose。 */
  	float PORT_F0; /* PORT_F0：旧下垂控制的频率基准量，编码比例见 RemoteSig_Dispose。 */
  	float PORT_KF; /* PORT_KF：旧频率下垂系数，编码比例见 RemoteSig_Dispose。 */
  	float PORT_KV; /* PORT_KV：旧电压下垂系数，编码比例见 RemoteSig_Dispose。 */
  	float Reserved_1[3]; /* Reserved_1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
} DPRAM_RemMeaSigFloat, *P_DPRAM_RemMeaSigFloat;//DPRAM的第3分区的结构体

extern volatile XXG_DPRAM_RemMeaSig DPRAM_Block3_remMeaSig; /* DPRAM_Block3_remMeaSig：原地址映射的端口遥测区；访问受 SST_ENABLE_DPRAM 门控。 */
extern XXG_DPRAM_RemMeaSig PORT_RealTime_DATA_buf; /* PORT_RealTime_DATA_buf：端口遥测的 16 位定点打包缓存。 */
extern DPRAM_RemMeaSigFloat PORT_RealTime_DATA; /* PORT_RealTime_DATA：端口遥测的浮点物理量缓存及兼容状态。 */
extern XXG_PORT_STATUS Port_RealStatus; /* Port_RealStatus：端口开关及桥状态的原位域缓存。 */
extern XXG_PORT_FAULT Port_RealFault; /* Port_RealFault：端口故障的原位域缓存，某些位在 HVAC 中复用。 */

//remote control in DPRAM
typedef union _XXG_COM_START_PORT
{
	struct _COM_START_PORT
	{
		ushort HVAC_Start:       1; /* HVAC_Start：1:端口启动；0：端口不动作；。 */  //1:端口启动；0：端口不动作；
		ushort HVDC_Start:       1; /* HVDC_Start：HVDC 端口的原遥控启动位，仅保留协议布局。 */
		ushort LVDC_Start:       1; /* LVDC_Start：LVDC 端口的原遥控启动位，仅保留协议布局。 */
		ushort LVAC_Start:       1; /* LVAC_Start：LVAC 端口的原遥控启动位，仅保留协议布局。 */
		ushort Reservedbit:  	12; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort COM_START_PORT16; /* COM_START_PORT16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_COM_START_PORT;

typedef union _XXG_COM_STOP_PORT
{
	struct _COM_STOP_PORT
	{
		ushort HVAC_STOP:       1; /* HVAC_STOP：1:端口停机；0：端口不动作；。 */  //1:端口停机；0：端口不动作；
		ushort HVDC_STOP:       1; /* HVDC_STOP：HVDC 端口的原遥控停止位，仅保留协议布局。 */
		ushort LVDC_STOP:       1; /* LVDC_STOP：LVDC 端口的原遥控停止位，仅保留协议布局。 */
		ushort LVAC_STOP:       1; /* LVAC_STOP：LVAC 端口的原遥控停止位，仅保留协议布局。 */
		ushort Reservedbit:  	12; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort COM_STOP_PORT16; /* COM_STOP_PORT16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_COM_STOP_PORT;

typedef union _XXG_COM_PORT_MODE
{
	struct _COM_PORT_MODE
	{
		ushort M_MODE_1:         1; /* M_MODE_1：1:恒压模式使能；0：不动作；。 */  //1:恒压模式使能；0：不动作；
		ushort M_MODE_2:         1; /* M_MODE_2：1:恒功率模式使能；0：不动作；。 */  //1:恒功率模式使能；0：不动作；
		ushort M_MODE_3:         1; /* M_MODE_3：1:下垂模式使能；0：不动作；。 */  //1:下垂模式使能；0：不动作；
		ushort M_MODE_4:         1; /* M_MODE_4：1:跟随模式使能；0：不动作；。 */  //1:跟随模式使能；0：不动作；

		ushort S2_MODE_1:        1; /* S2_MODE_1：1:注入谐波模式使能；0：不动作；。 */  //1:注入谐波模式使能；0：不动作；
		ushort S2_MODE_2:        1; /* S2_MODE_2：1:注入谐波模式禁止；0：不动作；。 */  //1:注入谐波模式禁止；0：不动作；
		ushort S2_MODE_3:        1; /* S2_MODE_3：1:治理谐波模式使能；0：不动作；。 */  //1:治理谐波模式使能；0：不动作；
		ushort S2_MODE_4:        1; /* S2_MODE_4：1:治理谐波模式禁止；0：不动作；。 */  //1:治理谐波模式禁止；0：不动作；

		ushort S1_MODE_1:        1; /* S1_MODE_1：1:STATCOM电压模式使能；0：不动作；。 */  //1:STATCOM电压模式使能；0：不动作；
		ushort S1_MODE_2:        1; /* S1_MODE_2：1:STATCOM电压模式禁止；0：不动作；。 */  //1:STATCOM电压模式禁止；0：不动作；
		ushort S1_MODE_3:        1; /* S1_MODE_3：1:STATCOM功率因数模式使能；0：不动作；。 */  //1:STATCOM功率因数模式使能；0：不动作；
		ushort S1_MODE_4:        1; /* S1_MODE_4：1:STATCOM功率因数模式禁止；0：不动作；。 */  //1:STATCOM功率因数模式禁止；0：不动作；
		ushort S1_MODE_5:        1; /* S1_MODE_5：1:STATCOM无功模式使能；0：不动作；。 */  //1:STATCOM无功模式使能；0：不动作；
		ushort S1_MODE_6:        1; /* S1_MODE_6：1:STATCOM无功模式禁止；0：不动作；。 */  //1:STATCOM无功模式禁止；0：不动作；
		ushort Reservedbit:  	 2; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort COM_PORT_MODE16; /* COM_PORT_MODE16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_COM_PORT_MODE;

typedef union _XXG_COM_SWITCH
{
	struct _COM_SWITCH
	{
		ushort Contactor_Inner:        1; /* Contactor_Inner：1:内部接触器动作；0：不动作；。 */  //1:内部接触器动作；0：不动作；
		ushort Breaker_Inner:          1; /* Breaker_Inner：1:内部断路器动作；0：不动作；。 */  //1:内部断路器动作；0：不动作；
		ushort Breaker_Outer1:         1; /* Breaker_Outer1：1:外部断路器1动作；0：不动作；。 */  //1:外部断路器1动作；0：不动作；
		ushort Breaker_Outer2:         1; /* Breaker_Outer2：1:外部断路器2动作；0：不动作；。 */  //1:外部断路器2动作；0：不动作；
		ushort Reservedbit:  	      12; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort COM_SWITCH16; /* COM_SWITCH16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_COM_SWITCH;

typedef union _XXG_COM_PLUSE
{
	struct _COM_PLUSE
	{
		ushort HF_Hbridge:             1; /* HF_Hbridge：1:高频H桥解锁或闭锁动作；0：不动作；。 */  //1:高频H桥解锁或闭锁动作；0：不动作；
		ushort LF_Hbridge:             1; /* LF_Hbridge：1:工频H桥解锁或闭锁动作；0：不动作；。 */  //1:工频H桥解锁或闭锁动作；0：不动作；
		ushort Reservedbit:  	      14; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort COM_PLUSE16; /* COM_PLUSE16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_COM_PLUSE;

typedef union _XXG_COM_FAULT_CLEAR
{
	struct _COM_FAULT_CLEAR
	{
		ushort FAULT_CLEAR:            1; /* FAULT_CLEAR：1:清除故障；0：不动作；。 */  //1:清除故障；0：不动作；
		ushort Reservedbit:  	      15; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort COM_FAULT_CLEAR16; /* COM_FAULT_CLEAR16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_COM_FAULT_CLEAR;

typedef union _XXG_PORT_SWITCH_EN
{
	struct _PORT_SWITCH_EN
	{
		ushort SWITCH_ON_EN:              1; /* SWITCH_ON_EN：1:外部断路器闭合使能；0：不动作；。 */  //1:外部断路器闭合使能；0：不动作；
		ushort SWITCH_ON_DEN:             1; /* SWITCH_ON_DEN：1:外部断路器闭合禁止；0：不动作；。 */  //1:外部断路器闭合禁止；0：不动作；
		ushort Reservedbit:  	         14; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PORT_SWITCH_EN16; /* PORT_SWITCH_EN16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_SWITCH_EN;

typedef union _XXG_PORT_NAME
{
	struct _PORT_NAME
	{
		ushort HVAC:                      1; /* HVAC：1:本端口为HVAC；0：不动作；。 */  //1:本端口为HVAC；0：不动作；
		ushort HVDC:                      1; /* HVDC：1:本端口为HVDC；0：不动作；。 */  //1:本端口为HVDC；0：不动作；
		ushort LVDC:                      1; /* LVDC：1:本端口为LVDC；0：不动作；。 */  //1:本端口为LVDC；0：不动作；
		ushort LVAC:                      1; /* LVAC：1:本端口为LVAC；0：不动作；。 */  //1:本端口为LVAC；0：不动作；
		ushort Reservedbit:  	         12; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PORT_NAME16; /* PORT_NAME16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_NAME;

typedef union _XXG_PORT_SENSORTYPE
{
	struct _PORT_SENSORTYPE
	{
		ushort PT_Ratio_1:         1; /* PT_Ratio_1：1:PT变比1启用；0：不动作；。 */  //1:PT变比1启用；0：不动作；
		ushort PT_Ratio_2:         1; /* PT_Ratio_2：1:PT变比2启用；0：不动作；。 */  //1:PT变比2启用；0：不动作；
		ushort PT_Conn_1:          1; /* PT_Conn_1：1:PT接线方式1启用；0：不动作；。 */  //1:PT接线方式1启用；0：不动作；
		ushort PT_Conn_2:          1; /* PT_Conn_2：1:PT接线方式2启用；0：不动作；。 */  //1:PT接线方式2启用；0：不动作；
		ushort CT_Ratio_1:         1; /* CT_Ratio_1：1:CT变比1启用；0：不动作；。 */  //1:CT变比1启用；0：不动作；
		ushort CT_Ratio_2:         1; /* CT_Ratio_2：1:CT变比2启用；0：不动作；。 */  //1:CT变比2启用；0：不动作；
		ushort CT_Conn_1:          1; /* CT_Conn_1：1:CT接线方式1启用；0：不动作；。 */  //1:CT接线方式1启用；0：不动作；
		ushort CT_Conn_2:          1; /* CT_Conn_2：1:CT接线方式2启用；0：不动作；。 */  //1:CT接线方式2启用；0：不动作；

		ushort Reservedbit:  	 8; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PORT_SENSORTYPE16; /* PORT_SENSORTYPE16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_SENSORTYPE;

typedef union _XXG_PORT_GRIDTYPE
{
	struct _PORT_GRIDTYPE
	{
		ushort PORT_TYPE_1:        1; /* PORT_TYPE_1：1:端口为电网；0：不动作；。 */  //1:端口为电网；0：不动作；
		ushort PORT_TYPE_2:        1; /* PORT_TYPE_2：1:端口为负载；0：不动作；。 */  //1:端口为负载；0：不动作；
		ushort GRID_STRONG:        1; /* GRID_STRONG：1:电网为强；0：不动作；。 */  //1:电网为强；0：不动作；
		ushort GRID_WEAK:          1; /* GRID_WEAK：1:电网为弱；0：不动作；。 */  //1:电网为弱；0：不动作；
		ushort GRID_FAULT:         1; /* GRID_FAULT：1:电网为故障；0：不动作；。 */  //1:电网为故障；0：不动作；
		ushort GRID_HEALTHY:       1; /* GRID_HEALTHY：1:电网为正常；0：不动作；。 */  //1:电网为正常；0：不动作；

		ushort Reservedbit:  	 10; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PORT_GRIDTYPE16; /* PORT_GRIDTYPE16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_GRIDTYPE;

typedef union _XXG_DPRAM_RemControl{
	struct _DPRAM_RemControl {

  	ushort Reserved_1[2]; /* Reserved_1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

  	XXG_COM_START_PORT COM_START_PORT; /* COM_START_PORT：启动命令。 */	      //启动命令
  	XXG_COM_STOP_PORT  COM_STOP_PORT; /* COM_STOP_PORT：停机命令。 */	      //停机命令
  	XXG_COM_PORT_MODE  COM_PORT_MODE; /* COM_PORT_MODE：端口工作模式切换命令。 */	      //端口工作模式切换命令
  	ushort Reserved_2; /* Reserved_2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

  	XXG_COM_SWITCH COM_SWITCH_ON; /* COM_SWITCH_ON：合端口开关命令；。 */             //合端口开关命令；
  	XXG_COM_SWITCH COM_SWITCH_OFF; /* COM_SWITCH_OFF：分端口开关命令；。 */            //分端口开关命令；
  	XXG_COM_PLUSE COM_PLUSE_ON; /* COM_PLUSE_ON：解脉冲命令；。 */               //解脉冲命令；
  	XXG_COM_PLUSE COM_PLUSE_OFF; /* COM_PLUSE_OFF：封脉冲命令；。 */              //封脉冲命令；
  	XXG_COM_FAULT_CLEAR COM_FAULT_CLEAR; /* COM_FAULT_CLEAR：故障清除命令；15:1,备用；0位，1表示清除故障，0表示不动作；。 */      //故障清除命令；15:1,备用；0位，1表示清除故障，0表示不动作；
  	XXG_PORT_SWITCH_EN PORT_SWITCH_EN; /* PORT_SWITCH_EN：外部断路器闭合使能；。 */        //外部断路器闭合使能；
  	XXG_PORT_NAME PORT_NAME; /* PORT_NAME：端口名称；。 */                  //端口名称；
  	XXG_PORT_SENSORTYPE PORT_SENSORTYPE; /* PORT_SENSORTYPE：PT/CT配置情况；。 */      //PT/CT配置情况；
  	XXG_PORT_GRIDTYPE PORT_GRIDTYPE; /* PORT_GRIDTYPE：电网类型；。 */         //电网类型；

  	ushort Reserved_3; /* Reserved_3：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

	} DPRAM_RemControl; /* DPRAM_RemControl：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 *///DPRAM的第1分区的结构体
	ushort DPRAM_Block1Buf[DPRAMBlock1_Length]; /* DPRAM_Block1Buf：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
}XXG_DPRAM_RemControl;

extern volatile XXG_DPRAM_RemControl DPRAM_Block1_remControl; /* DPRAM_Block1_remControl：原地址映射的遥控区；访问受 SST_ENABLE_DPRAM 门控。 */ 
extern XXG_DPRAM_RemControl PORT_Control_DATA_Buf; /* PORT_Control_DATA_Buf：旧遥控协议的本地字缓存。 */
extern XXG_COM_START_PORT Port_COM_START; /* Port_COM_START：旧遥控启动字的位域解码缓存。 */
extern XXG_COM_STOP_PORT Port_COM_STOP; /* Port_COM_STOP：旧遥控停止字的位域解码缓存。 */

//remote regulate in DPRAM
typedef union _XXG_COM_SYS_MODE
{
	struct _COM_SYS_MODE
	{
		ushort HA_RunMode:         1; /* HA_RunMode：HVAC端口，1:运行；0：测试；。 */  //HVAC端口，1:运行；0：测试；
		ushort HA_CtlMode:         1; /* HA_CtlMode：HVAC端口，1:自动；0：手动；。 */  //HVAC端口，1:自动；0：手动；
		ushort HD_RunMode:         1; /* HD_RunMode：HVDC端口，1:运行；0：测试。 */  //HVDC端口，1:运行；0：测试
		ushort HD_CtlMode:         1; /* HD_CtlMode：HVDC端口，1:自动；0：手动；。 */  //HVDC端口，1:自动；0：手动；
		ushort LD_RunMode:         1; /* LD_RunMode：LVDC端口，1:运行；0：测试；。 */  //LVDC端口，1:运行；0：测试；
		ushort LD_CtlMode:         1; /* LD_CtlMode：LVDC端口，1:自动；0：手动；。 */  //LVDC端口，1:自动；0：手动；
		ushort LA_RunMode:         1; /* LA_RunMode：LVAC端口，1:运行；0：测试。 */  //LVAC端口，1:运行；0：测试
		ushort LA_CtlMode:         1; /* LA_CtlMode：LVAC端口，1:自动；0：手动；。 */  //LVAC端口，1:自动；0：手动；

		ushort Reservedbit:  	   8; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	
	ushort COM_SYS_MODE16; /* COM_SYS_MODE16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_COM_SYS_MODE;

typedef union _XXG_AD_PORT_EN
{
	struct _AD_PORT_EN
	{
		ushort Reservedbit1:      12; /* Reservedbit1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

		ushort IA_Prot_EN:         1; /* IA_Prot_EN：HVAC端口，1：A相过流保护使能，0：禁止；HVDC端口，1：端口直流电压保护使能，0：禁止；LVDC端口，1：+375V极到0电压保护使能，0：禁止；LVAC端口，1：A相过流保护使能，0：禁止；。 */  //HVAC端口，1：A相过流保护使能，0：禁止；HVDC端口，1：端口直流电压保护使能，0：禁止；LVDC端口，1：+375V极到0电压保护使能，0：禁止；LVAC端口，1：A相过流保护使能，0：禁止；
		ushort IB_Prot_EN:         1; /* IB_Prot_EN：HVAC端口，1：B相过流保护使能，0：禁止；HVDC端口，1：系统直流电压保护使能，0：禁止；LVDC端口，1：0到-375V极电压保护使能，0：禁止； LVAC端口，1：B相过流保护使能，0：禁止；。 */  //HVAC端口，1：B相过流保护使能，0：禁止；HVDC端口，1：系统直流电压保护使能，0：禁止；LVDC端口，1：0到-375V极电压保护使能，0：禁止； LVAC端口，1：B相过流保护使能，0：禁止；
		ushort IC_Prot_EN:         1; /* IC_Prot_EN：HVAC端口，1：C相过流保护使能，0：禁止；HVDC端口，1：直流电流1保护使能，0：禁止；     LVDC端口，1：正极电流保护使能，0：禁止；                LVAC端口，1：C相过流保护使能，0：禁止；。 */  //HVAC端口，1：C相过流保护使能，0：禁止；HVDC端口，1：直流电流1保护使能，0：禁止；     LVDC端口，1：正极电流保护使能，0：禁止；                LVAC端口，1：C相过流保护使能，0：禁止；
		ushort IN_Port_EN:  	   1; /* IN_Port_EN：HVAC端口，备用；                                              HVDC端口，1：直流电流2保护使能，0：禁止；     LVDC端口，1：负极电流保护使能，0：禁止；                LVAC端口，1：N相过流保护使能，0：禁止；。 */  //HVAC端口，备用；                                              HVDC端口，1：直流电流2保护使能，0：禁止；     LVDC端口，1：负极电流保护使能，0：禁止；                LVAC端口，1：N相过流保护使能，0：禁止；
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort AD_PORT_EN16; /* AD_PORT_EN16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_AD_PORT_EN;

typedef union _XXG_FO_EN
{
	struct _FO_EN
	{
		//HVAC端口，1：A/B/C相的第1~14个模块FO保护使能，0：禁止；HVDC/LVDC/LVAC端口，1：第1~15个模块FO保护使能，0：禁止;逆变器侧，1：对应器件的FO使能，0：禁止；
		ushort FO_EN_bit0:         1; /* FO_EN_bit0：FO_EN 第 0 路硬件故障输入使能位。 */
		ushort FO_EN_bit1:         1; /* FO_EN_bit1：FO_EN 第 1 路硬件故障输入使能位。 */
		ushort FO_EN_bit2:         1; /* FO_EN_bit2：FO_EN 第 2 路硬件故障输入使能位。 */
		ushort FO_EN_bit3:  	   1; /* FO_EN_bit3：FO_EN 第 3 路硬件故障输入使能位。 */
		ushort FO_EN_bit4:         1; /* FO_EN_bit4：FO_EN 第 4 路硬件故障输入使能位。 */
		ushort FO_EN_bit5:         1; /* FO_EN_bit5：FO_EN 第 5 路硬件故障输入使能位。 */
		ushort FO_EN_bit6:  	   1; /* FO_EN_bit6：FO_EN 第 6 路硬件故障输入使能位。 */
		ushort FO_EN_bit7:         1; /* FO_EN_bit7：FO_EN 第 7 路硬件故障输入使能位。 */
		ushort FO_EN_bit8:         1; /* FO_EN_bit8：FO_EN 第 8 路硬件故障输入使能位。 */
		ushort FO_EN_bit9:  	   1; /* FO_EN_bit9：FO_EN 第 9 路硬件故障输入使能位。 */
		ushort FO_EN_bit10:        1; /* FO_EN_bit10：FO_EN 第 10 路硬件故障输入使能位。 */
		ushort FO_EN_bit11:        1; /* FO_EN_bit11：FO_EN 第 11 路硬件故障输入使能位。 */
		ushort FO_EN_bit12:  	   1; /* FO_EN_bit12：FO_EN 第 12 路硬件故障输入使能位。 */
		ushort FO_EN_bit13:        1; /* FO_EN_bit13：FO_EN 第 13 路硬件故障输入使能位。 */
		ushort FO_EN_bit14:        1; /* FO_EN_bit14：FO_EN 第 14 路硬件故障输入使能位。 */

		ushort Reservedbit1:       1; /* Reservedbit1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort FO_EN16; /* FO_EN16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_FO_EN;

typedef union _XXG_FB_EN
{
	struct _FB_EN
	{
		//bit0~15对应2~17号插件,1表示使能，0表示禁止；
		ushort FB_EN_bit0:         1; /* FB_EN_bit0：FB_EN 第 0 路硬件故障输入使能位。 */
		ushort FB_EN_bit1:         1; /* FB_EN_bit1：FB_EN 第 1 路硬件故障输入使能位。 */
		ushort FB_EN_bit2:         1; /* FB_EN_bit2：FB_EN 第 2 路硬件故障输入使能位。 */
		ushort FB_EN_bit3:  	   1; /* FB_EN_bit3：FB_EN 第 3 路硬件故障输入使能位。 */
		ushort FB_EN_bit4:         1; /* FB_EN_bit4：FB_EN 第 4 路硬件故障输入使能位。 */
		ushort FB_EN_bit5:         1; /* FB_EN_bit5：FB_EN 第 5 路硬件故障输入使能位。 */
		ushort FB_EN_bit6:  	   1; /* FB_EN_bit6：FB_EN 第 6 路硬件故障输入使能位。 */
		ushort FB_EN_bit7:         1; /* FB_EN_bit7：FB_EN 第 7 路硬件故障输入使能位。 */
		ushort FB_EN_bit8:         1; /* FB_EN_bit8：FB_EN 第 8 路硬件故障输入使能位。 */
		ushort FB_EN_bit9:  	   1; /* FB_EN_bit9：FB_EN 第 9 路硬件故障输入使能位。 */
		ushort FB_EN_bit10:        1; /* FB_EN_bit10：FB_EN 第 10 路硬件故障输入使能位。 */
		ushort FB_EN_bit11:        1; /* FB_EN_bit11：FB_EN 第 11 路硬件故障输入使能位。 */
		ushort FB_EN_bit12:  	   1; /* FB_EN_bit12：FB_EN 第 12 路硬件故障输入使能位。 */
		ushort FB_EN_bit13:        1; /* FB_EN_bit13：FB_EN 第 13 路硬件故障输入使能位。 */
		ushort FB_EN_bit14:        1; /* FB_EN_bit14：FB_EN 第 14 路硬件故障输入使能位。 */
		ushort FB_EN_bit15:        1; /* FB_EN_bit15：FB_EN 第 15 路硬件故障输入使能位。 */
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort FB_EN16; /* FB_EN16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_FB_EN;

typedef union _XXG_DPRAM_RemRegulate{
	struct _DPRAM_RemRegulate {

	XXG_COM_SYS_MODE COM_SYS_MODE; /* COM_SYS_MODE：系统运行方式。 */          //系统运行方式
	XXG_AD_PORT_EN AD_PROT_EN; /* AD_PROT_EN：端口 AD 保护使能位图，保持原 FPGA/DPRAM 字布局。 */	            //插件通道保护使能
	XXG_FO_EN FO_1_EN; /* FO_1_EN：第 1 组光纤故障输入使能位图。 */	                    //光纤输入插件FO使能
	XXG_FO_EN FO_2_EN; /* FO_2_EN：第 2 组光纤故障输入使能位图。 */
	XXG_FO_EN FO_3_EN; /* FO_3_EN：第 3 组光纤故障输入使能位图。 */
	XXG_FB_EN FB_EN; /* FB_EN：FB 硬件故障输入使能位图，保持原 FPGA/DPRAM 字布局。 */                        //主控插件接收各插件FB使能
  	short PORT_P_SET; /* PORT_P_SET：旧遥调有功功率给定；定点与浮点转换见 RemoteRegulate_Dispose。 */
  	short PORT_Q_SET; /* PORT_Q_SET：旧遥调无功功率给定；定点与浮点转换见 RemoteRegulate_Dispose。 */
  	short PORT_V_SET; /* PORT_V_SET：旧遥调电压给定；float 为 V，定点编码见 RemoteRegulate_Dispose。 */
  	short PORT_F_SET; /* PORT_F_SET：旧遥调频率给定；float 为 Hz，定点编码见 RemoteRegulate_Dispose。 */
  	short RMS_CURRENT_5TH_SET; /* RMS_CURRENT_5TH_SET：5 次谐波电流有效值给定；定点/浮点比例见旧遥调/遥测转换。 */
  	short RMS_CURRENT_7TH_SET; /* RMS_CURRENT_7TH_SET：7 次谐波电流有效值给定；定点/浮点比例见旧遥调/遥测转换。 */
  	short RMS_CURRENT_11TH_SET; /* RMS_CURRENT_11TH_SET：11 次谐波电流有效值给定；定点/浮点比例见旧遥调/遥测转换。 */
  	short PHA_CURRENT_5TH_SET; /* PHA_CURRENT_5TH_SET：5 次谐波电流相位给定；定点/浮点比例见旧遥调/遥测转换。 */
  	short PHA_CURRENT_7TH_SET; /* PHA_CURRENT_7TH_SET：7 次谐波电流相位给定；定点/浮点比例见旧遥调/遥测转换。 */
  	short PHA_CURRENT_11TH_SET; /* PHA_CURRENT_11TH_SET：11 次谐波电流相位给定；定点/浮点比例见旧遥调/遥测转换。 */
  	short STATCOM_V_SET; /* STATCOM_V_SET：旧 STATCOM 电压控制给定；编码见 RemoteRegulate_Dispose。 */
  	short STATCOM_PF_SET; /* STATCOM_PF_SET：旧 STATCOM 功率因数给定；编码见 RemoteRegulate_Dispose。 */
  	short STATCOM_Q_SET; /* STATCOM_Q_SET：旧 STATCOM 无功功率给定；编码见 RemoteRegulate_Dispose。 */
  	short PORT_V0_SET; /* PORT_V0_SET：旧下垂控制电压基准给定；编码见 RemoteRegulate_Dispose。 */
  	short PORT_F0_SET; /* PORT_F0_SET：旧下垂控制频率基准给定；编码见 RemoteRegulate_Dispose。 */
  	short PORT_KF_SET; /* PORT_KF_SET：旧频率下垂系数给定；编码见 RemoteRegulate_Dispose。 */
  	short PORT_KV_SET; /* PORT_KV_SET：旧电压下垂系数给定；编码见 RemoteRegulate_Dispose。 */
  	short Reserved_1[4]; /* Reserved_1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

  	short PORT_V_P_1; /* PORT_V_P_1：旧第 1 组电压环比例遥调系数，保持原缩放约定。 */            //PI参数
  	short PORT_V_I_1; /* PORT_V_I_1：旧第 1 组电压环积分遥调系数，保持原缩放约定。 */
  	short PORT_I_P_1; /* PORT_I_P_1：旧第 1 组电流环比例遥调系数，保持原缩放约定。 */
  	short PORT_I_I_1; /* PORT_I_I_1：旧第 1 组电流环积分遥调系数，保持原缩放约定。 */
  	short PORT_V_P_2; /* PORT_V_P_2：旧第 2 组电压环比例遥调系数，保持原缩放约定。 */
  	short PORT_V_I_2; /* PORT_V_I_2：旧第 2 组电压环积分遥调系数，保持原缩放约定。 */
  	short PORT_I_P_2; /* PORT_I_P_2：旧第 2 组电流环比例遥调系数，保持原缩放约定。 */
  	short PORT_I_I_2; /* PORT_I_I_2：旧第 2 组电流环积分遥调系数，保持原缩放约定。 */
  	short PORT_V_P_3; /* PORT_V_P_3：旧第 3 组电压环比例遥调系数，保持原缩放约定。 */
  	short PORT_V_I_3; /* PORT_V_I_3：旧第 3 组电压环积分遥调系数，保持原缩放约定。 */
  	short PORT_I_P_3; /* PORT_I_P_3：旧第 3 组电流环比例遥调系数，保持原缩放约定。 */
  	short PORT_I_I_3; /* PORT_I_I_3：旧第 3 组电流环积分遥调系数，保持原缩放约定。 */

  	ushort EER2_DATA[4]; /* EER2_DATA：旧 EER2 兼容数据字段，按原 16 位字数组布局解释。 */

	} DPRAM_RemRegulate; /* DPRAM_RemRegulate：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 *///DPRAM的第2分区的结构体
	ushort DPRAM_Block2Buf[DPRAMBlock2_Length]; /* DPRAM_Block2Buf：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
}XXG_DPRAM_RemRegulate;


typedef struct _DPRAM_RemRegulateFloat {

	XXG_COM_SYS_MODE COM_SYS_MODE; /* COM_SYS_MODE：系统运行方式。 */          //系统运行方式
	XXG_AD_PORT_EN AD_PROT_EN; /* AD_PROT_EN：端口 AD 保护使能位图，保持原 FPGA/DPRAM 字布局。 */	            //插件通道保护使能
	XXG_FO_EN FO_1_EN; /* FO_1_EN：第 1 组光纤故障输入使能位图。 */	                    //光纤输入插件FO使能
	XXG_FO_EN FO_2_EN; /* FO_2_EN：第 2 组光纤故障输入使能位图。 */
	XXG_FO_EN FO_3_EN; /* FO_3_EN：第 3 组光纤故障输入使能位图。 */
	XXG_FB_EN FB_EN; /* FB_EN：FB 硬件故障输入使能位图，保持原 FPGA/DPRAM 字布局。 */                        //主控插件接收各插件FB使能
  	float PORT_P_SET; /* PORT_P_SET：旧遥调有功功率给定；定点与浮点转换见 RemoteRegulate_Dispose。 */
  	float PORT_Q_SET; /* PORT_Q_SET：旧遥调无功功率给定；定点与浮点转换见 RemoteRegulate_Dispose。 */
  	float PORT_V_SET; /* PORT_V_SET：旧遥调电压给定；float 为 V，定点编码见 RemoteRegulate_Dispose。 */
  	float PORT_F_SET; /* PORT_F_SET：旧遥调频率给定；float 为 Hz，定点编码见 RemoteRegulate_Dispose。 */
  	float RMS_CURRENT_5TH_SET; /* RMS_CURRENT_5TH_SET：5 次谐波电流有效值给定；定点/浮点比例见旧遥调/遥测转换。 */
  	float RMS_CURRENT_7TH_SET; /* RMS_CURRENT_7TH_SET：7 次谐波电流有效值给定；定点/浮点比例见旧遥调/遥测转换。 */
  	float RMS_CURRENT_11TH_SET; /* RMS_CURRENT_11TH_SET：11 次谐波电流有效值给定；定点/浮点比例见旧遥调/遥测转换。 */
  	float PHA_CURRENT_5TH_SET; /* PHA_CURRENT_5TH_SET：5 次谐波电流相位给定；定点/浮点比例见旧遥调/遥测转换。 */
  	float PHA_CURRENT_7TH_SET; /* PHA_CURRENT_7TH_SET：7 次谐波电流相位给定；定点/浮点比例见旧遥调/遥测转换。 */
  	float PHA_CURRENT_11TH_SET; /* PHA_CURRENT_11TH_SET：11 次谐波电流相位给定；定点/浮点比例见旧遥调/遥测转换。 */
  	float STATCOM_V_SET; /* STATCOM_V_SET：旧 STATCOM 电压控制给定；编码见 RemoteRegulate_Dispose。 */
  	float STATCOM_PF_SET; /* STATCOM_PF_SET：旧 STATCOM 功率因数给定；编码见 RemoteRegulate_Dispose。 */
  	float STATCOM_Q_SET; /* STATCOM_Q_SET：旧 STATCOM 无功功率给定；编码见 RemoteRegulate_Dispose。 */
  	float PORT_V0_SET; /* PORT_V0_SET：旧下垂控制电压基准给定；编码见 RemoteRegulate_Dispose。 */
  	float PORT_F0_SET; /* PORT_F0_SET：旧下垂控制频率基准给定；编码见 RemoteRegulate_Dispose。 */
  	float PORT_KF_SET; /* PORT_KF_SET：旧频率下垂系数给定；编码见 RemoteRegulate_Dispose。 */
  	float PORT_KV_SET; /* PORT_KV_SET：旧电压下垂系数给定；编码见 RemoteRegulate_Dispose。 */
  	float Reserved_1[4]; /* Reserved_1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */

  	float PORT_V_P_1; /* PORT_V_P_1：旧第 1 组电压环比例遥调系数，保持原缩放约定。 */            //PI参数
  	float PORT_V_I_1; /* PORT_V_I_1：旧第 1 组电压环积分遥调系数，保持原缩放约定。 */
  	float PORT_I_P_1; /* PORT_I_P_1：旧第 1 组电流环比例遥调系数，保持原缩放约定。 */
  	float PORT_I_I_1; /* PORT_I_I_1：旧第 1 组电流环积分遥调系数，保持原缩放约定。 */
  	float PORT_V_P_2; /* PORT_V_P_2：旧第 2 组电压环比例遥调系数，保持原缩放约定。 */
  	float PORT_V_I_2; /* PORT_V_I_2：旧第 2 组电压环积分遥调系数，保持原缩放约定。 */
  	float PORT_I_P_2; /* PORT_I_P_2：旧第 2 组电流环比例遥调系数，保持原缩放约定。 */
  	float PORT_I_I_2; /* PORT_I_I_2：旧第 2 组电流环积分遥调系数，保持原缩放约定。 */
  	float PORT_V_P_3; /* PORT_V_P_3：旧第 3 组电压环比例遥调系数，保持原缩放约定。 */
  	float PORT_V_I_3; /* PORT_V_I_3：旧第 3 组电压环积分遥调系数，保持原缩放约定。 */
  	float PORT_I_P_3; /* PORT_I_P_3：旧第 3 组电流环比例遥调系数，保持原缩放约定。 */
  	float PORT_I_I_3; /* PORT_I_I_3：旧第 3 组电流环积分遥调系数，保持原缩放约定。 */

  	ushort EER2_DATA[4]; /* EER2_DATA：旧 EER2 兼容数据字段，按原 16 位字数组布局解释。 */

} DPRAM_RemRegulateFloat, *P_DPRAM_RemRegulateFloat;//DPRAM的第2分区的结构体

extern volatile XXG_DPRAM_RemRegulate DPRAM_Block2_remRegulate; /* DPRAM_Block2_remRegulate：原地址映射的遥调区；访问受 SST_ENABLE_DPRAM 门控。 */ 
extern XXG_DPRAM_RemRegulate PORT_Regulate_DATA_Buf; /* PORT_Regulate_DATA_Buf：旧遥调协议的本地定点字缓存。 */
extern DPRAM_RemRegulateFloat PORT_Regulate_DATA; /* PORT_Regulate_DATA：旧遥调定点值换算后的浮点给定缓存。 */

extern ushort Port_AD_PORT_EN; /* Port_AD_PORT_EN：端口各通道 AD 保护使能位图。 */
extern ushort Port_FO_1_EN; /* Port_FO_1_EN：第 1 组光纤故障输入使能位图。 */
extern ushort Port_FO_2_EN; /* Port_FO_2_EN：第 2 组光纤故障输入使能位图。 */
extern ushort Port_FO_3_EN; /* Port_FO_3_EN：第 3 组光纤故障输入使能位图。 */
extern ushort Port_FB_EN; /* Port_FB_EN：端口 FB 硬件故障输入使能位图。 */
	  
typedef union _XXG_SwitchFLAG				
{
	struct _SwitchFLAG
	{
		ushort S_EXT_CB_ON_EN:  1; /* S_EXT_CB_ON_EN：旧开关流程的外部断路器合闸允许条件。 */  
		ushort S_EXT_CB:   			2; /* S_EXT_CB：旧开关流程的外部断路器目标/判定状态。 */
		ushort S_INT_CB:   			2; /* S_INT_CB：旧开关流程的内部断路器目标/判定状态。 */ 
		ushort S_CONTACTOR:     1; /* S_CONTACTOR：旧开关流程的接触器目标/判定状态。 */ 
			
		ushort EXT_CB_ON_EN:    		1; /* EXT_CB_ON_EN：外部断路器合闸允许标志。 */  
		ushort Flag_Switch:   			3; /* Flag_Switch：1: 动接触器；2：动内部断路器;3：动外部断路器1；4：动外部断路器2。 */   //1: 动接触器；2：动内部断路器;3：动外部断路器1；4：动外部断路器2
		ushort Flag_SwitchZhi:      1; /* Flag_SwitchZhi：旧开关动作时序状态值。 */   //0:断；1：合
		ushort Flag_SwitchSuccess:  1; /* Flag_SwitchSuccess：旧开关动作成功标志。 */ 
		ushort Flag_SwitchRun:      1; /* Flag_SwitchRun：旧开关动作正在执行标志。 */ 
		ushort Flag_SwitchTable:    2; /* Flag_SwitchTable：0：操作失败；1:无需继续操作1；2：无需继续操作2；3：继续操作。 */   //0：操作失败；1:无需继续操作1；2：无需继续操作2；3：继续操作
		ushort Reservedbit:     1; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */				
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort SwitchFLAG16; /* SwitchFLAG16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_SwitchFLAG;
extern XXG_SwitchFLAG SW; /* SW：旧开关时序、动作状态及完成标志的位域缓存。 */

typedef union _XXG_FaultTypeFLAG				
{
	struct _FaultTypeFLAG
	{
		ushort Port_OverVolt:      1; /* Port_OverVolt：原端口过压故障分类位。 */   //0--no fault;      
		ushort Port_OverCurrent:   1; /* Port_OverCurrent：原端口过流故障分类位。 */   
 		ushort Module_DAB_H:   		 1; /* Module_DAB_H：原模块 DAB 桥故障分类位。 */
		ushort Module_50Hz_H:      1; /* Module_50Hz_H：原模块工频桥故障分类位。 */
		ushort Module_Single:      1; /* Module_Single：原单模块故障分类位。 */
		ushort Module_OverVolt:    1; /* Module_OverVolt：原模块过压故障分类位。 */
		ushort Module_OverCurrent: 1; /* Module_OverCurrent：原模块过流故障分类位。 */
		ushort Module_OverTemp:    1; /* Module_OverTemp：原模块过温故障分类位。 */

		ushort Contactor:    			 1; /* Contactor：原接触器动作故障分类位。 */	
		ushort Breaker:            1; /* Breaker：原断路器动作故障分类位。 */		
		ushort Controller:   			 1; /* Controller：原控制器故障分类位。 */	
		ushort Sync:               1; /* Sync：原同步故障分类位。 */			
		ushort Communication:      1; /* Communication：原通信故障分类位。 */			
		
		ushort Module_Fault:       1; /* Module_Fault：原模块汇总故障分类位。 */
		ushort Module_LowerVolt:   1; /* Module_LowerVolt：原模块欠压故障分类位。 */
		ushort Module_OverVoltLevel2:  1; /* Module_OverVoltLevel2：原模块二级过压故障分类位。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort FaultTypeFLAG16; /* FaultTypeFLAG16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_FaultTypeFLAG;
extern XXG_FaultTypeFLAG FaultTypeFLAG; /* FaultTypeFLAG：原故障分类位图，后台故障报告据此映射五位码。 */

typedef union _XXG_PortRunFLAG				
{
	struct _PortRunFLAG
	{
		ushort Port_SigUdc:           1; /* Port_SigUdc：旧端口直流电压单独检查/信号标志。 */  
		ushort Port_UdcOK:            1; /* Port_UdcOK：旧端口模块直流电压预充达标标志。 */
		ushort Port_AutoOrManual:     1; /* Port_AutoOrManual：旧端口自动/手动运行选择字段。 */     //1: Auto
		ushort Port_RunOrTest:        1; /* Port_RunOrTest：旧端口正常运行/测试选择字段。 */		 //1: Run
		
		ushort PWMState_50Hz:					1; /* PWMState_50Hz：旧工频桥实际 PWM 状态镜像。 */		 //1: Disable
		ushort PWMState_Dab:					1; /* PWMState_Dab：旧 DAB 实际 PWM 状态镜像。 */		 //1: Disable	
		ushort PWMState_Module:       1; /* PWMState_Module：旧模块桥 PWM 汇总状态镜像。 */		 //1: Disable	
		ushort Stop_AllPort:					1; /* Stop_AllPort：旧全端口停机请求标志。 */     //1: all port need shutdown
		ushort PhaseSquence_Error:    1; /* PhaseSquence_Error：旧相序错误标志，保留原字段拼写。 */
		ushort Port_HighUdc:          1; /* Port_HighUdc：旧端口模块直流电压过高标志。 */	
		ushort Reservedbit:           6; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PortRunFLAG16; /* PortRunFLAG16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PortRunFLAG;
extern XXG_PortRunFLAG PortRunFLAG; /* PortRunFLAG：旧端口运行、预充、PWM 及相序状态的位域缓存。 */

typedef union _XXG_ControlFLAG				
{
	struct _ControlFLAG
	{
		ushort SVG_PWMEN:       		1; /* SVG_PWMEN：旧工频桥 PWM 使能命令标志。 */  
		ushort DAB_PWMEN:       		1; /* DAB_PWMEN：旧 DAB PWM 使能命令标志。 */
			
		ushort Port_Start:      		2; /* Port_Start：旧端口启动流程请求。 */    //01: starting; 11: start failure; 10: start success
		ushort Port_Stop:       		2; /* Port_Stop：旧端口停止流程请求。 */    //01: stoping; 11: stop failure; 10: stop success
		ushort Port_FaultDispose:   2; /* Port_FaultDispose：旧端口故障处置待执行标志。 */    //01: disposing; 11: failure; 10: success
					
		ushort DAB_DutyStart:   		1; /* DAB_DutyStart：旧 DAB 占空比启动斜坡进行标志。 */ 
		ushort SVG_Start:       		1; /* SVG_Start：旧工频桥启动流程进行标志。 */
		
		ushort Port_StartValid: 		1; /* Port_StartValid：旧端口启动条件检查通过标志。 */
		ushort DAB_DutyOver:    		1; /* DAB_DutyOver：旧 DAB 占空比启动斜坡完成标志。 */		
		ushort SVG_StartOver:   		1; /* SVG_StartOver：旧工频桥启动流程完成标志。 */
		ushort DAB_StartMethod:   	1; /* DAB_StartMethod：旧 DAB 启动策略字段。 */
						
		ushort Reservedbit:     		2; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort ControlFLAG16; /* ControlFLAG16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_ControlFLAG;
extern XXG_ControlFLAG ControlFLAG; /* ControlFLAG：旧启动、停止、PWM、DAB 斜坡流程控制标志。 */


typedef union _XXG_FaultDisposeFLAG				
{
	struct _FaultDisposeFLAG
	{
		ushort FaultDisposeType:   2; /* FaultDisposeType：原故障处置策略类型字段。 */   //0--no fault;      1--PWM pluse disable;   2--port shutdown
			
		ushort PWMPulseState:      3; /* PWMPulseState：原故障处置中的 PWM 状态字段。 */   //0--no pulse disable； 1--50Hz H-Bridge/Module pulse disable；2--DAB H-Bridge pulse disable; 3--port pulse disable
		ushort PWMPulseDisableNum: 1; /* PWMPulseDisableNum：原策略需要禁用 PWM 的模块数/编号字段。 */		//0~2
		ushort PWMEN_FibreOptical: 1; /* PWMEN_FibreOptical：原光纤 PWM 使能控制字段。 */   //0--off;1--on
//		ushort Port_PWMDisable:    2;   //01: PWM pulse shutting;  10: PWM pulse shutting success; 11: PWM pulse shutting failure
//		ushort Port_PWMEnable:     2;   //01: PWM pulse opening;  10: PWM pulse opening success; 11: PWM pulse opening failure
				
		ushort Port_StopType:      3; /* Port_StopType：旧端口停机类型字段。 */ 	//000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown
		ushort Port_StopAll:       1; /* Port_StopAll：旧故障策略要求全端口停机的标志。 */ 	//1: system shutdown   
		
		ushort Reservedbit:        5; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort FaultDisposeFLAG16; /* FaultDisposeFLAG16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
	
} XXG_FaultDisposeFLAG;

extern XXG_FaultDisposeFLAG FaultDisposeFLAG; /* FaultDisposeFLAG：原故障处理策略及 PWM 禁用范围的位域缓存。 */
extern ushort ModuleIndex_PWMPulseDisable; /* ModuleIndex_PWMPulseDisable：原故障策略中待禁用 PWM 的模块索引。 */   //Index of Module which is disabled   

typedef union _XXG_FT3_FaultRecorder_Control_Word
{
    struct _Control_Word
    {
        unsigned short RT1_EN:             1; /* RT1_EN：FT3 第 1 路遥测发送使能位。 */   //为1时RT1数据有效
        unsigned short RT2_EN:             1; /* RT2_EN：FT3 第 2 路遥测发送使能位。 */   //为1时RT2数据有效
        unsigned short RT3_EN:             1; /* RT3_EN：FT3 第 3 路遥测发送使能位。 */   //为1时RT3数据有效
        unsigned short RT4_EN:             1; /* RT4_EN：FT3 第 4 路遥测发送使能位。 */   //为1时RT4数据有效
        unsigned short RT5_EN:             1; /* RT5_EN：FT3 第 5 路遥测发送使能位。 */   //为1时RT5数据有效
        unsigned short RT6_EN:             1; /* RT6_EN：FT3 第 6 路遥测发送使能位。 */   //为1时RT6数据有效
        unsigned short Reservedbit:        2; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */   //保留
        unsigned short DataPackNum:        8; /* DataPackNum：0x01--1号数据包;      0x02--2号数据包；  0x03--3号数据包。 */   //0x01--1号数据包;      0x02--2号数据包；  0x03--3号数据包
    } Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
    unsigned short Control_Word16; /* Control_Word16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_FT3_FaultRecorder_Control_Word;

typedef union _XXG_FT3_Register				
{
	struct _FT3_Register
	{
		unsigned int  FT3_SendCTL:                               32; /* FT3_SendCTL：FT3 发送启动/控制寄存器。 */
	        unsigned int  FT3_ReadCTL:                               32; /* FT3_ReadCTL：FT3 接收读取/控制寄存器。 */
	        unsigned int  Reservedbit1:                              32; /* Reservedbit1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	        unsigned int  Reservedbit2:                              32; /* Reservedbit2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	        XXG_FT3_FaultRecorder_Control_Word FT3_SendData_ControlWord; /* FT3_SendData_ControlWord：上传的控制字16位。 */ //上传的控制字16位
	        unsigned short FT3_SendData_RT1:                         16; /* FT3_SendData_RT1：FT3 第 1 路遥测发送数据字。 */ //上传的第一个数据
	        unsigned short FT3_SendData_RT2:                         16; /* FT3_SendData_RT2：FT3 第 2 路遥测发送数据字。 */ //上传的第二个数据
	        unsigned short FT3_SendData_RT3:                         16; /* FT3_SendData_RT3：FT3 第 3 路遥测发送数据字。 */ //上传的第三个数据
	        unsigned short FT3_SendData_RT4:                         16; /* FT3_SendData_RT4：FT3 第 4 路遥测发送数据字。 */ //上传的第四个数据
	        unsigned short FT3_SendData_RT5:                         16; /* FT3_SendData_RT5：FT3 第 5 路遥测发送数据字。 */ //上传的第五个数据
	        unsigned short FT3_SendData_RT6:                         16; /* FT3_SendData_RT6：FT3 第 6 路遥测发送数据字。 */ //上传的第六个数据
	        unsigned short FT3_SendData_SampCnt:                     16; /* FT3_SendData_SampCnt：循环计数器。 */ //循环计数器
	        unsigned short FT3_RecData_1:                            16; /* FT3_RecData_1：FT3 第 1 个接收数据字。 */ //读取数据1，对于故障录波装置，无实际意义，自读自写测试用
	        unsigned short FT3_RecData_2:                            16; /* FT3_RecData_2：FT3 第 2 个接收数据字。 */ //读取数据2，对于故障录波装置，无实际意义，自读自写测试用
	        unsigned short FT3_RecData_3:                            16; /* FT3_RecData_3：FT3 第 3 个接收数据字。 */ //读取数据3，对于故障录波装置，无实际意义，自读自写测试用
	        unsigned short FT3_RecData_4:                            16; /* FT3_RecData_4：FT3 第 4 个接收数据字。 */ //读取数据4，对于故障录波装置，无实际意义，自读自写测试用
	        unsigned short FT3_RecData_5:                            16; /* FT3_RecData_5：FT3 第 5 个接收数据字。 */ //读取数据5，对于故障录波装置，无实际意义，自读自写测试用
	        unsigned short FT3_RecData_6:                            16; /* FT3_RecData_6：FT3 第 6 个接收数据字。 */ //读取数据6，对于故障录波装置，无实际意义，自读自写测试用
	        unsigned short FT3_RecData_7:                            16; /* FT3_RecData_7：FT3 第 7 个接收数据字。 */ //读取数据7，对于故障录波装置，无实际意义，自读自写测试用
	        unsigned short FT3_RecData_8:                            16; /* FT3_RecData_8：FT3 第 8 个接收数据字。 */ //读取数据8，对于故障录波装置，无实际意义，自读自写测试用
			unsigned int  Reservedbit3:                              32; /* Reservedbit3：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	        unsigned int  Reservedbit4:                              32; /* Reservedbit4：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	        unsigned int  Reservedbit5:                              32; /* Reservedbit5：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	        unsigned int  Reservedbit6:                              32; /* Reservedbit6：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	}FT3_Register; /* FT3_Register：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */

	ushort FT3_Register_BUF[32]; /* FT3_Register_BUF：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */

} XXG_FT3_Register;

extern volatile XXG_FT3_Register  FT3_PTCT_Register,FT3_FaultRecorder_Register,FT3_EER2_Register,FT3_Backup_Register; /* FT3_PTCT_Register：PT/CT FT3 插件的原映射寄存器窗口。 */
/* FT3_FaultRecorder_Register：故障录波 FT3 插件的原映射寄存器窗口。 */
/* FT3_EER2_Register：EER2 FT3 插件的原映射寄存器窗口。 */
/* FT3_Backup_Register：备用 FT3 插件的原映射寄存器窗口。 */



typedef union _XXG_H_Filter{	
	struct _H_Filter {
  	float IN_n_1; /* IN_n_1：前一拍输入历史值，单位与该控制器输入/输出一致。 */
  	float IN_n_2; /* IN_n_2：前两拍输入历史值，单位与该控制器输入/输出一致。 */
  	float OUT_n_1; /* OUT_n_1：前一拍输出历史值，单位与该控制器输入/输出一致。 */
  	float OUT_n_2; /* OUT_n_2：前两拍输出历史值，单位与该控制器输入/输出一致。 */
	} H_Filter; /* H_Filter：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */
	float H_Filter_BUF[4]; /* H_Filter_BUF：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
}XXG_H_Filter;

typedef union _XXG_Har_Controller{	
	struct _Har_Controller {
  	float IN_n_1; /* IN_n_1：前一拍输入历史值，单位与该控制器输入/输出一致。 */
  	float IN_n_2; /* IN_n_2：前两拍输入历史值，单位与该控制器输入/输出一致。 */
  	float OUT_n_1; /* OUT_n_1：前一拍输出历史值，单位与该控制器输入/输出一致。 */
  	float OUT_n_2; /* OUT_n_2：前两拍输出历史值，单位与该控制器输入/输出一致。 */
	} Har_Controller; /* Har_Controller：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */
	float Har_Controller_BUF[4]; /* Har_Controller_BUF：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
}XXG_Har_Controller;

typedef union _XXG_PID_Controller{	
	struct _PID_Controller {
		float Num_1; /* Num_1：增量 PI 的本拍误差系数。 */
		float Num_0; /* Num_0：增量 PI 的前一拍误差系数。 */
  	float IN_n_1; /* IN_n_1：前一拍输入历史值，单位与该控制器输入/输出一致。 */
  	float OUT_n_1; /* OUT_n_1：前一拍输出历史值，单位与该控制器输入/输出一致。 */
	} PID_Controller; /* PID_Controller：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */
	float PID_Controller_BUF[4]; /* PID_Controller_BUF：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
}XXG_PID_Controller;

//edited by shiqi, fault record
typedef union _XXG_FaultRecordStatus{
	struct _FaultRecordStatus {
	  	ushort StatusNum_ready:                1; /* StatusNum_ready：FT3 录波的准备状态位。 */
	  	ushort StatusNum_Run:                  1; /* StatusNum_Run：FT3 录波的运行状态位。 */
	  	ushort StatusNum_Stop:                 1; /* StatusNum_Stop：FT3 录波的停止状态位。 */
	  	ushort STATCOM_Q:                      1; /* STATCOM_Q：FT3 录波的无功控制模式标志。 */
	  	ushort Reservedbit:                         4; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	  	ushort Sys_Stop:                            1; /* Sys_Stop：FT3 录波/端口主状态的系统停机位。 */
	  	ushort Outer_Elec:                          1; /* Outer_Elec：FT3 录波/端口主状态的外部电源接入位。 */
	  	ushort voltage_mode:                        1; /* voltage_mode：FT3 录波的电压源模式标志。 */
	  	ushort power_mode:                          1; /* power_mode：FT3 录波的功率控制模式标志。 */
	  	ushort droop_mode:                          1; /* droop_mode：FT3 录波的下垂控制模式标志。 */
	  	ushort gridfollow_mode:                     1; /* gridfollow_mode：FT3 录波的跟网控制模式标志。 */
	  	ushort Reservedbit2:                         2; /* Reservedbit2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	} Bits; /* Bits：与整字重叠的位域视图，字段位置对应原录波协议布局。 */
	unsigned short FaultREcord_Word16; /* FaultREcord_Word16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
}XXG_FaultRecordStatus;

typedef union _XXG_FaultRecordStatus_1{
	struct _FaultRecordStatus_1 {
  	ushort no_harmonic:                    1; /* no_harmonic：FT3 录波的无谐波补偿模式标志。 */
	ushort harmonic_inject:                1; /* harmonic_inject：FT3 录波的谐波注入模式标志。 */
  	ushort harmonic_limit:                 1; /* harmonic_limit：FT3 录波的谐波限制模式标志。 */
  	ushort no_STATCOM:                     1; /* no_STATCOM：FT3 录波的无 STATCOM 控制模式标志。 */
  	ushort STATCOM_Q:                      1; /* STATCOM_Q：FT3 录波的无功控制模式标志。 */
  	ushort STATCOM_PF:                     1; /* STATCOM_PF：FT3 录波的功率因数控制模式标志。 */
  	ushort STATCOM_V:                      1; /* STATCOM_V：FT3 录波的电压控制模式标志。 */
  	ushort Reservedbit:                    9; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
	} Bits; /* Bits：与整字重叠的位域视图，字段位置对应原录波协议布局。 */
	unsigned short FaultREcord_Word16_1; /* FaultREcord_Word16_1：本联合体的完整 16 位字视图，与位域成员共用存储。 */
}XXG_FaultRecordStatus_1;

extern volatile XXG_FaultRecordStatus FaultRecordStatus; /* FaultRecordStatus：FT3 故障录波主运行模式状态字。 */
extern volatile XXG_FaultRecordStatus_1 FaultRecordStatus_1; /* FaultRecordStatus_1：FT3 故障录波谐波/STATCOM 模式状态字。 */

/******************************************************************************/
//Common Variables
/******************************************************************************/
extern ushort WrongTimes_LEFT, WrongTimes_RIGHT,WrongTimes_LEFT_A, WrongTimes_RIGHT_A; /* WrongTimes_LEFT：原 FPGA LEFT 参考/通信核对的累计不一致次数。 */
/* WrongTimes_RIGHT：原 FPGA RIGHT 参考/通信核对的累计不一致次数。 */
/* WrongTimes_LEFT_A：原 FPGA LEFT_A 参考/通信核对的累计不一致次数。 */
/* WrongTimes_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的累计不一致次数。 */
extern ushort LastWrong_LEFT, LastWrong_RIGHT,LastWrong_LEFT_A, LastWrong_RIGHT_A; /* LastWrong_LEFT：原 FPGA LEFT 参考/通信核对的上一轮不一致值。 */
/* LastWrong_RIGHT：原 FPGA RIGHT 参考/通信核对的上一轮不一致值。 */
/* LastWrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的上一轮不一致值。 */
/* LastWrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的上一轮不一致值。 */
extern ushort Wrong_LEFT, Wrong_RIGHT,Wrong_LEFT_A, Wrong_RIGHT_A; /* Wrong_LEFT：原 FPGA LEFT 参考/通信核对的本轮不一致值。 */
/* Wrong_RIGHT：原 FPGA RIGHT 参考/通信核对的本轮不一致值。 */
/* Wrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的本轮不一致值。 */
/* Wrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的本轮不一致值。 */

extern ushort WrongTimes_Reference_A1,WrongTimes_Reference_A2,WrongTimes_Reference_C1,WrongTimes_Reference_C2; /* WrongTimes_Reference_A1：原 FPGA Reference_A1 参考/通信核对的累计不一致次数。 */
/* WrongTimes_Reference_A2：原 FPGA Reference_A2 参考/通信核对的累计不一致次数。 */
/* WrongTimes_Reference_C1：原 FPGA Reference_C1 参考/通信核对的累计不一致次数。 */
/* WrongTimes_Reference_C2：原 FPGA Reference_C2 参考/通信核对的累计不一致次数。 */
extern ushort LastWrong_Reference_A1,LastWrong_Reference_A2,LastWrong_Reference_C1,LastWrong_Reference_C2; /* LastWrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的上一轮不一致值。 */
/* LastWrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的上一轮不一致值。 */
/* LastWrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的上一轮不一致值。 */
/* LastWrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的上一轮不一致值。 */
extern ushort Wrong_Reference_A1,Wrong_Reference_A2,Wrong_Reference_C1,Wrong_Reference_C2; /* Wrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的本轮不一致值。 */
/* Wrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的本轮不一致值。 */
/* Wrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的本轮不一致值。 */
/* Wrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的本轮不一致值。 */	

extern ushort WrongTimes_ControlReference_A,WrongTimes_ControlReference_C; /* WrongTimes_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的累计不一致次数。 */
/* WrongTimes_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的累计不一致次数。 */
extern ushort LastWrong_ControlReference_A,LastWrong_ControlReference_C; /* LastWrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的上一轮不一致值。 */
/* LastWrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的上一轮不一致值。 */
extern ushort Wrong_ControlReference_A,Wrong_ControlReference_C; /* Wrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的本轮不一致值。 */
/* Wrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的本轮不一致值。 */	

//FLAG
extern unsigned char Flag_PC; /* Flag_PC：旧 PC/本地控制兼容标志。 */
extern unsigned char Flag_DC_FAULT; /* Flag_DC_FAULT：原直流故障总标志。 */
extern unsigned short Flag_DC_FAULTDISPOSE; /* Flag_DC_FAULTDISPOSE：原直流故障处置状态。 */
extern unsigned char Flag_DC_FAULTport; /* Flag_DC_FAULTport：原直流故障端口标识。 */

extern unsigned char Flag_DC_VoltSource; /* Flag_DC_VoltSource：原直流电压源控制模式标志。 */
extern unsigned char Flag_DC_VoltSource_Set; /* Flag_DC_VoltSource_Set：原直流电压源模式设置完成标志。 */
extern unsigned char flag_INV; /* flag_INV：原工频桥开环逆变模式标志。 */
extern unsigned char flag_SVGOPEN; /* flag_SVGOPEN：原工频桥开环投入标志。 */
extern unsigned char flag_DABOPEN; /* flag_DABOPEN：原 DAB 开环投入标志。 */
extern unsigned char flag_FaultClose; /* flag_FaultClose：原故障后脉冲关闭标志。 */
extern unsigned char Flag_FaultOff; /* Flag_FaultOff：原故障停机标志。 */
extern unsigned char FLAG_RemRegulate_SetOver; /* FLAG_RemRegulate_SetOver：原遥调设置完成标志。 */
extern unsigned char flag_SendDABRef_First; /* flag_SendDABRef_First：原 DAB 参考首次下发标志。 */
extern unsigned char flag_SendSVGRef_First; /* flag_SendSVGRef_First：原工频桥参考首次下发标志。 */

extern unsigned char Flag_Key; /* Flag_Key：原本地按键扫描状态。 */
extern unsigned char Flag_temp; /* Flag_temp：后台运行指示灯翻转状态。 */
extern unsigned char Flag_WDI; /* Flag_WDI：后台外部看门狗 GPIO 翻转状态。 */

extern unsigned char Flag_WaitPowerStart10s; /* Flag_WaitPowerStart10s：Timer2 更新、后台消费的主电源等待 10 s到时标志。 */

extern unsigned char Flag_1s; /* Flag_1s：Timer2 更新、后台消费的周期 1 s到时标志。 */
extern unsigned char Flag_50ms; /* Flag_50ms：Timer2 更新、后台消费的周期 50 ms到时标志。 */
extern unsigned char Flag_100ms; /* Flag_100ms：Timer2 更新、后台消费的周期 100 ms到时标志。 */
extern unsigned char Flag_RemoteMeasure; /* Flag_RemoteMeasure：Timer2 更新、后台消费的遥测轮询到时标志。 */

extern unsigned char Flag_Switch500ms; /* Flag_Switch500ms：Timer2 更新、后台消费的开关操作等待 500 ms到时标志。 */
extern unsigned char Flag_Switch1s; /* Flag_Switch1s：Timer2 更新、后台消费的开关操作等待 1 s到时标志。 */
extern unsigned char Flag_Precharge500ms; /* Flag_Precharge500ms：Timer2 更新、后台消费的预充等待 500 ms到时标志。 */
extern unsigned char Flag_PreStart3s; /* Flag_PreStart3s：Timer2 更新、后台消费的启动准备等待 3 s到时标志。 */
extern unsigned char Flag_SVG1s; /* Flag_SVG1s：Timer2 更新、后台消费的SVG 投入等待 1 s到时标志。 */
extern unsigned char Flag_20s; /* Flag_20s：Timer2 更新、后台消费的上电等待 20 s到时标志。 */
extern unsigned char Flag_Clear_0,Flag_Clear_1; /* Flag_Clear_0：原故障锁存清除第一阶段请求标志。 */
/* Flag_Clear_1：原故障锁存清除第二阶段请求标志。 */
extern unsigned char Flag_Clear_zhi; /* Flag_Clear_zhi：原故障清除期间的兼容状态。 */
extern unsigned char Flag_Wait; /* Flag_Wait：原控制流程等待标志。 */
extern unsigned char Flag_StopWait; /* Flag_StopWait：原停机延时等待标志。 */
extern unsigned char Flag_StopWaitEnd; /* Flag_StopWaitEnd：原停机等待结束标志。 */

extern unsigned short countWaitPowerStart10s; /* countWaitPowerStart10s：Timer2 原 主电源等待 10 s计数；实际节拍沿用原判断式。 */
extern unsigned short cout_Wait; /* cout_Wait：Timer2 原 原控制等待计数；实际节拍沿用原判断式。 */
extern unsigned short cout200us; /* cout200us：Timer2 原 原名为 200 us 的兼容定时计数；实际节拍沿用原判断式。 */
extern unsigned short cout1s; /* cout1s：Timer2 原 周期 1 s计数；实际节拍沿用原判断式。 */
extern unsigned short cout50ms; /* cout50ms：Timer2 原 周期 50 ms计数；实际节拍沿用原判断式。 */
extern unsigned short cout100ms; /* cout100ms：Timer2 原 周期 100 ms计数；实际节拍沿用原判断式。 */
extern unsigned short coutRemoteMeasure; /* coutRemoteMeasure：Timer2 原 遥测轮询计数；实际节拍沿用原判断式。 */
extern unsigned short coutStopWait; /* coutStopWait：Timer2 原 停机等待计数；实际节拍沿用原判断式。 */

extern unsigned short coutSwitch500ms; /* coutSwitch500ms：Timer2 原 开关操作等待 500 ms计数；实际节拍沿用原判断式。 */
extern unsigned short coutSwitch1s; /* coutSwitch1s：Timer2 原 开关操作等待 1 s计数；实际节拍沿用原判断式。 */
extern unsigned short coutPrecharge500ms; /* coutPrecharge500ms：Timer2 原 预充等待 500 ms计数；实际节拍沿用原判断式。 */
extern unsigned short coutPreStart3s; /* coutPreStart3s：Timer2 原 启动准备等待 3 s计数；实际节拍沿用原判断式。 */
extern unsigned short coutSVG1s; /* coutSVG1s：Timer2 原 SVG 投入等待 1 s计数；实际节拍沿用原判断式。 */
extern unsigned short cout20s; /* cout20s：Timer2 原 上电等待 20 s计数；实际节拍沿用原判断式。 */

extern unsigned char Flag_ComTest; /* Flag_ComTest：旧端口通信测试标志。 */
extern unsigned short FourPortComZhi; /* FourPortComZhi：旧四端口通信测试状态字。 */
		
extern unsigned char Flag_Uart; /* Flag_Uart：旧 UART 维护帧兼容状态，保留符号供原路径回读。 */
extern unsigned char CommNum; /* CommNum：旧 UART 兼容通信序号。 */
extern unsigned char CommFunc; /* CommFunc：旧 UART 兼容功能码。 */
extern unsigned int StartAddr; /* StartAddr：旧 UART 兼容请求的起始地址。 */
extern unsigned short ReadLength; /* ReadLength：旧 UART 兼容读取长度。 */
extern unsigned char CommLength; /* CommLength：旧 UART 兼容帧有效长度。 */
extern unsigned char CommInNum; /* CommInNum：旧 UART 兼容帧当前接收位置。 */
extern unsigned char CommTotalNum; /* CommTotalNum：旧 UART 兼容帧预期总长度。 */
extern unsigned char WriteOK; /* WriteOK：旧 UART 兼容写入成功标志。 */
extern unsigned char PCuartBUFF[140]; /* PCuartBUFF：旧 PC UART 帧兼容缓存，容量 140 字节。 */
extern unsigned char ReceiveuartBUFF[11]; /* ReceiveuartBUFF：旧 UART 接收帧兼容缓存，容量 11 字节。 */

//Transformation
extern float Alpha_Trans,Beta_Trans; /* Alpha_Trans：通用坐标变换 α 轴输出，与输入电压/电流同单位。 */
/* Beta_Trans：通用坐标变换 β 轴输出，与输入电压/电流同单位。 */
extern float A_Trans,B_Trans,C_Trans; /* A_Trans：通用逆变换 A 相输出，与输入量同单位。 */
/* B_Trans：通用逆变换 B 相输出，与输入量同单位。 */
/* C_Trans：通用逆变换 C 相输出，与输入量同单位。 */
extern float D_Trans,Q_Trans; /* D_Trans：通用 Park 变换 d 轴输出，与输入量同单位。 */
/* Q_Trans：通用 Park 变换 q 轴输出，与输入量同单位。 */
extern float Sin_ph_Trans,Cos_ph_Trans,Sin_ph_Trans_NEW,Cos_ph_Trans_NEW; /* Sin_ph_Trans：正弦坐标变换系数。 */
/* Cos_ph_Trans：余弦坐标变换系数。 */
/* Sin_ph_Trans_NEW：正弦坐标变换系数，输出变换使用值。 */
/* Cos_ph_Trans_NEW：余弦坐标变换系数，输出变换使用值。 */

//Fault
extern ushort FOLatch_A_TAD; /* FOLatch_A_TAD：A 相插件光纤故障的 TAD 锁存值。 */
extern ushort FOLatch_B_TAD; /* FOLatch_B_TAD：B 相插件光纤故障的 TAD 锁存值。 */
extern ushort FOLatch_C_TAD; /* FOLatch_C_TAD：C 相插件光纤故障的 TAD 锁存值。 */
extern ushort FOLatch_INV_TAD; /* FOLatch_INV_TAD：独立逆变桥光纤故障的 TAD 锁存值。 */
extern ushort PortLatch_TAD; /* PortLatch_TAD：端口 AD 保护故障的 TAD 锁存值。 */
extern ushort FBLatch_TAD; /* FBLatch_TAD：端口 FB 硬件故障的 TAD 锁存值。 */
extern ushort ModuleFault_DisposeTimes; /* ModuleFault_DisposeTimes：原模块故障处置已执行次数。 */
extern unsigned char ClearTimes; /* ClearTimes：原故障清除过程的执行次数。 */
//DPRAM
extern unsigned char flag_RemoteControlRegulate; /* flag_RemoteControlRegulate：原头文件小写兼容声明，实际使用符号为 Flag_RemoteControlRegulate。 */
extern ushort FLAG_NEW_R; /* FLAG_NEW_R：DPRAM 向对端发布的新数据块标识。 */
extern ushort FLAG_NEW_L; /* FLAG_NEW_L：DPRAM 从对端读取的新数据块标识。 */
extern ushort SWTable_TotalTime; /* SWTable_TotalTime：旧开关动作表的总定时时间。 */
extern unsigned char WriteBlock_Index; /* WriteBlock_Index：后台 DPRAM 插件遥测轮询的当前块号。 */

//voltage
extern float Module_DCbus_Volt; /* Module_DCbus_Volt：当前检查模块的直流母线电压，单位 V。 */   //Module DCbus voltage
extern float Udc_Ref; /* Udc_Ref：旧直流电压控制参考，单位 V。 */
extern float Current_Ref; /* Current_Ref：旧直流/交流电流控制参考，单位 A。 */
extern float Ust_add; /* Ust_add：旧直流电压给定爬升增量，单位 V。 */
extern uchar Udc_Check_Index; /* Udc_Check_Index：逐模块直流电压巡检的当前索引。 */
extern uchar Udc_Checked_Num; /* Udc_Checked_Num：已检查模块数量。 */

//regulate
extern float PLL_Factor; /* PLL_Factor：旧 PLL 系数调整因子。 */
extern float Volt_Factor; /* Volt_Factor：旧总直流电压环系数调整因子。 */
extern float PH_Volt_Factor; /* PH_Volt_Factor：旧相间电压平衡环系数调整因子。 */
extern float Har_2_Factor; /* Har_2_Factor：2 次谐振控制系数调整因子。 */
extern float Har_6_Factor; /* Har_6_Factor：6 次谐振控制系数调整因子。 */
extern float Har_12_Factor; /* Har_12_Factor：12 次谐振控制系数调整因子。 */
extern float Current_Factor; /* Current_Factor：旧正序电流环系数调整因子。 */
extern float Neg_Current_Factor; /* Neg_Current_Factor：旧负序电流环系数调整因子。 */
extern float AVolt_Factor; /* AVolt_Factor：旧交流电压相关调整因子。 */
extern ushort Port_OnState; /* Port_OnState：原端口已投入状态。 */

extern unsigned char TrigeSignal; /* TrigeSignal：原调试/录波触发信号。 */
extern unsigned char TestINT; /* TestINT：原中断调试观察标志。 */
extern float Ref_js_test; /* Ref_js_test：原调制测试幅值/计算缓存。 */
extern float Ref_ph_test; /* Ref_ph_test：原开环测试相角，单位 rad。 */
extern ushort rtt; /* rtt：DPRAM 收到通知后的插件 LED 翻转状态。 */
extern float DAC_BUF[4]; /* DAC_BUF：四路 DA 调试输出缓存，写出时转为 FPGA 原始计数。 */
extern float TEATabc[4]; /* TEATabc：原三相调试观察缓存，沿用原符号拼写。 */

//edited by shiqi
extern unsigned short FT3_Samp_Cnt; /* FT3_Samp_Cnt：FT3 上报的采样序号。 */
extern unsigned short dataPackNum; /* dataPackNum：FT3 数据包数量/控制字字段。 */

//edited by shiqi, fault recorder
extern ushort faultsignal; /* faultsignal：原 FT3 故障触发信号。 */
extern ushort faultRecordStart; /* faultRecordStart：原 FT3 录波启动标志。 */
extern ushort fault_count; /* fault_count：原故障录波触发计数。 */
extern ushort fault_count_start; /* fault_count_start：原故障录波开始计数/状态。 */
extern ushort FR_tri_count1, FR_tri_count2, FR_tri_count3; /* FR_tri_count1：原 FT3 录波第 1 组触发计数。 */
/* FR_tri_count2：原 FT3 录波第 2 组触发计数。 */
/* FR_tri_count3：原 FT3 录波第 3 组触发计数。 */

/******************************************************************************/
//HVAC Variables
/******************************************************************************/
#ifdef HVAC_online
//PID
extern XXG_PID_Controller PLL_PID_Controller,Volt_PID_Controller,Current_d_PID_Controller,Current_q_PID_Controller,PID_Control_buf; /* PLL_PID_Controller：锁相环增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Volt_PID_Controller：直流电压增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Current_d_PID_Controller：d 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Current_q_PID_Controller：q 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
/* PID_Control_buf：通用增量 PI 本拍使用的控制器状态缓存。 */
extern XXG_PID_Controller Neg_Current_d_PID_Controller,Neg_Current_q_PID_Controller; /* Neg_Current_d_PID_Controller：负序d 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
/* Neg_Current_q_PID_Controller：负序q 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
extern XXG_PID_Controller ClusterVolt_A_PID_Controller,ClusterVolt_B_PID_Controller,ClusterVolt_C_PID_Controller; /* ClusterVolt_A_PID_Controller：相间平衡直流电压_A增量 PI 的系数与上一拍输入/输出历史状态。 */
/* ClusterVolt_B_PID_Controller：相间平衡直流电压_B增量 PI 的系数与上一拍输入/输出历史状态。 */
/* ClusterVolt_C_PID_Controller：相间平衡直流电压_C增量 PI 的系数与上一拍输入/输出历史状态。 */
//U Grid
extern XXG_H_Filter U_d_Grid_H2_20_Filter,U_d_Grid_H2_100_Filter,U_d_Grid_H6_100_Filter,U_d_Grid_H12_100_Filter; /* U_d_Grid_H2_20_Filter：电网 d 轴电压_H2_20滤波器的四个历史输入/输出状态。 */
/* U_d_Grid_H2_100_Filter：电网 d 轴电压_H2_100滤波器的四个历史输入/输出状态。 */
/* U_d_Grid_H6_100_Filter：电网 d 轴电压_H6_100滤波器的四个历史输入/输出状态。 */
/* U_d_Grid_H12_100_Filter：电网 d 轴电压_H12_100滤波器的四个历史输入/输出状态。 */
extern XXG_H_Filter U_q_Grid_H2_20_Filter,U_q_Grid_H2_100_Filter,U_q_Grid_H6_100_Filter,U_q_Grid_H12_100_Filter; /* U_q_Grid_H2_20_Filter：电网 q 轴电压_H2_20滤波器的四个历史输入/输出状态。 */
/* U_q_Grid_H2_100_Filter：电网 q 轴电压_H2_100滤波器的四个历史输入/输出状态。 */
/* U_q_Grid_H6_100_Filter：电网 q 轴电压_H6_100滤波器的四个历史输入/输出状态。 */
/* U_q_Grid_H12_100_Filter：电网 q 轴电压_H12_100滤波器的四个历史输入/输出状态。 */
//I SVG
extern XXG_H_Filter I_d_SVG_H2_20_Filter,I_d_SVG_H2_100_Filter,I_d_SVG_H6_100_Filter,I_d_SVG_H12_100_Filter; /* I_d_SVG_H2_20_Filter：端口 d 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
/* I_d_SVG_H2_100_Filter：端口 d 轴电流_H2_100滤波器的四个历史输入/输出状态。 */
/* I_d_SVG_H6_100_Filter：端口 d 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_d_SVG_H12_100_Filter：端口 d 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
extern XXG_H_Filter I_q_SVG_H2_20_Filter,I_q_SVG_H2_100_Filter,I_q_SVG_H6_100_Filter,I_q_SVG_H12_100_Filter; /* I_q_SVG_H2_20_Filter：端口 q 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
/* I_q_SVG_H2_100_Filter：端口 q 轴电流_H2_100滤波器的四个历史输入/输出状态。 */
/* I_q_SVG_H6_100_Filter：端口 q 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_q_SVG_H12_100_Filter：端口 q 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
//I Load
extern XXG_H_Filter I_d_Load_H6_100_Filter,I_d_Load_H12_100_Filter,I_d_Load_H2_20_Filter; /* I_d_Load_H6_100_Filter：负载 d 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_d_Load_H12_100_Filter：负载 d 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
/* I_d_Load_H2_20_Filter：负载 d 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
extern XXG_H_Filter I_q_Load_H6_100_Filter,I_q_Load_H12_100_Filter,I_q_Load_H2_20_Filter; /* I_q_Load_H6_100_Filter：负载 q 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
/* I_q_Load_H12_100_Filter：负载 q 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
/* I_q_Load_H2_20_Filter：负载 q 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
//V_Ave
extern XXG_H_Filter Va_Ave_H2_100_Filter,Vb_Ave_H2_100_Filter,Vc_Ave_H2_100_Filter,Ave_H2_100_Filter,Ave_H6_100_Filter; /* Va_Ave_H2_100_Filter：A 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
/* Vb_Ave_H2_100_Filter：B 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
/* Vc_Ave_H2_100_Filter：C 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
/* Ave_H2_100_Filter：平均量_H2_100滤波器的四个历史输入/输出状态。 */
/* Ave_H6_100_Filter：平均量_H6_100滤波器的四个历史输入/输出状态。 */
extern XXG_H_Filter I_d_Ref_H2_100_Filter,I_d_Ref_H6_100_Filter; /* I_d_Ref_H2_100_Filter：d 轴电流给定_H2_100滤波器的四个历史输入/输出状态。 */
/* I_d_Ref_H6_100_Filter：d 轴电流给定_H6_100滤波器的四个历史输入/输出状态。 */
//P,Q
extern XXG_H_Filter P_H2_100_Filter,Q_H2_100_Filter,Q_Load_H2_100_Filter; /* P_H2_100_Filter：P_H2_100滤波器的四个历史输入/输出状态。 */
/* Q_H2_100_Filter：Q_H2_100滤波器的四个历史输入/输出状态。 */
/* Q_Load_H2_100_Filter：负载无功_H2_100滤波器的四个历史输入/输出状态。 */
//buf
extern XXG_H_Filter H2_100_Filter,H6_100_Filter,H12_100_Filter,H2_20_Filter; /* H2_100_Filter：通用 H2/100 滤波计算历史缓存。 */
/* H6_100_Filter：通用 H6/100 滤波计算历史缓存。 */
/* H12_100_Filter：通用 H12/100 滤波计算历史缓存。 */
/* H2_20_Filter：通用 H2/20 滤波计算历史缓存。 */
//Harmonic Current
extern XXG_Har_Controller I_d_2_Har_Controller,I_q_2_Har_Controller; /* I_d_2_Har_Controller：I_d_2 次谐振控制器的历史输入/输出状态。 */
/* I_q_2_Har_Controller：I_q_2 次谐振控制器的历史输入/输出状态。 */
extern XXG_Har_Controller I_d_6_Har_Controller,I_q_6_Har_Controller; /* I_d_6_Har_Controller：I_d_6 次谐振控制器的历史输入/输出状态。 */
/* I_q_6_Har_Controller：I_q_6 次谐振控制器的历史输入/输出状态。 */
extern XXG_Har_Controller I_d_12_Har_Controller,I_q_12_Har_Controller; /* I_d_12_Har_Controller：I_d_12 次谐振控制器的历史输入/输出状态。 */
/* I_q_12_Har_Controller：I_q_12 次谐振控制器的历史输入/输出状态。 */
//buf
extern XXG_Har_Controller Har_2_Controller,Har_6_Controller,Har_12_Controller; /* Har_2_Controller：2 次谐振通用历史输入/输出缓存。 */
/* Har_6_Controller：6 次谐振通用历史输入/输出缓存。 */
/* Har_12_Controller：12 次谐振通用历史输入/输出缓存。 */


extern float Ugrid_d_Positive,Ugrid_d_Negative,Ugrid_hp_d; /* Ugrid_d_Positive：电网电压 d 轴/分量的正序基波值，单位 V。 */
/* Ugrid_d_Negative：电网电压 d 轴/分量的负序基波值，单位 V。 */
/* Ugrid_hp_d：电网电压 hp 轴/分量的d 轴谐波值，单位 V。 */
extern float Ugrid_q_Positive,Ugrid_q_Negative,Ugrid_hp_q; /* Ugrid_q_Positive：电网电压 q 轴/分量的正序基波值，单位 V。 */
/* Ugrid_q_Negative：电网电压 q 轴/分量的负序基波值，单位 V。 */
/* Ugrid_hp_q：电网电压 hp 轴/分量的q 轴谐波值，单位 V。 */
extern float Iin_d_Positive,Iin_d_Negative,Iin_hp_d; /* Iin_d_Positive：端口电流 d 轴/分量的正序基波值，单位 A。 */
/* Iin_d_Negative：端口电流 d 轴/分量的负序基波值，单位 A。 */
/* Iin_hp_d：端口电流 hp 轴/分量的d 轴谐波值，单位 A。 */
extern float Iin_q_Positive,Iin_q_Negative,Iin_hp_q; /* Iin_q_Positive：端口电流 q 轴/分量的正序基波值，单位 A。 */
/* Iin_q_Negative：端口电流 q 轴/分量的负序基波值，单位 A。 */
/* Iin_hp_q：端口电流 hp 轴/分量的q 轴谐波值，单位 A。 */
extern float Iload_d_Positive,Iload_d_Negative,Iload_har_d; /* Iload_d_Positive：负载电流 d 轴/分量的正序基波值，单位 A。 */
/* Iload_d_Negative：负载电流 d 轴/分量的负序基波值，单位 A。 */
/* Iload_har_d：负载电流 har 轴/分量的d 轴谐波值，单位 A。 */
extern float Iload_q_Positive,Iload_q_Negative,Iload_har_q; /* Iload_q_Positive：负载电流 q 轴/分量的正序基波值，单位 A。 */
/* Iload_q_Negative：负载电流 q 轴/分量的负序基波值，单位 A。 */
/* Iload_har_q：负载电流 har 轴/分量的q 轴谐波值，单位 A。 */
extern float Phase_PLL; /* Phase_PLL：旧 HVAC 锁相环电网相角，单位 rad。 */
extern float Grid_P_Power,Grid_Q_Power,Load_Q_Power,SVG_Q_Power_Ref; /* Grid_P_Power：旧电网有功功率计算值。 */
/* Grid_Q_Power：旧电网无功功率计算值。 */
/* Load_Q_Power：旧负载无功功率计算值。 */
/* SVG_Q_Power_Ref：旧 SVG 无功功率目标。 */
extern float SVG_Udc_A_Ave,SVG_Udc_B_Ave,SVG_Udc_C_Ave,SVG_Udc_Ave; /* SVG_Udc_A_Ave：A 相在线模块平均直流电压，单位 V。 */
/* SVG_Udc_B_Ave：B 相在线模块平均直流电压，单位 V。 */
/* SVG_Udc_C_Ave：C 相在线模块平均直流电压，单位 V。 */
/* SVG_Udc_Ave：三相单模块直流电压的总平均值，单位 V。 */

extern float Id_Positive_Ref,Id_Negative_Ref,Id_Positive_Max,Id_Positive_Min; /* Id_Positive_Ref：Id 正序电流给定，单位 A。 */
/* Id_Negative_Ref：Id 负序电流给定，单位 A。 */
/* Id_Positive_Max：Id 正序电流给定上限，单位 A。 */
/* Id_Positive_Min：Id 正序电流给定下限，单位 A。 */
extern float Iq_Positive_Ref,Iq_Negative_Ref,Iq_Positive_Max,Iq_Positive_Min; /* Iq_Positive_Ref：Iq 正序电流给定，单位 A。 */
/* Iq_Negative_Ref：Iq 负序电流给定，单位 A。 */
/* Iq_Positive_Max：Iq 正序电流给定上限，单位 A。 */
/* Iq_Positive_Min：Iq 正序电流给定下限，单位 A。 */

extern float Uout_d_Positive,Uout_q_Positive,Uout_d_Negative,Uout_q_Negative,Uout_d_Positive_Max,Uout_d_Positive_Min,Uout_q_Positive_Max,Uout_q_Positive_Min; /* Uout_d_Positive：旧电流环 d_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Positive：旧电流环 q_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_d_Negative：旧电流环 d_负序 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Negative：旧电流环 q_负序 输出电压分量/限幅值，单位 V。 */
/* Uout_d_Positive_Max：旧电流环 d_正序_上限 输出电压分量/限幅值，单位 V。 */
/* Uout_d_Positive_Min：旧电流环 d_正序_下限 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Positive_Max：旧电流环 q_正序_上限 输出电压分量/限幅值，单位 V。 */
/* Uout_q_Positive_Min：旧电流环 q_正序_下限 输出电压分量/限幅值，单位 V。 */
extern float Uout_A_Positive,Uout_B_Positive,Uout_C_Positive; /* Uout_A_Positive：旧电流环 A_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_B_Positive：旧电流环 B_正序 输出电压分量/限幅值，单位 V。 */
/* Uout_C_Positive：旧电流环 C_正序 输出电压分量/限幅值，单位 V。 */

extern float Har_5_RMS,Har_7_RMS,Har_11_RMS; /* Har_5_RMS：5 次谐波电流有效值，单位 A。 */
/* Har_7_RMS：7 次谐波电流有效值，单位 A。 */
/* Har_11_RMS：11 次谐波电流有效值，单位 A。 */
extern float Har_5_PH,Har_7_PH,Har_11_PH; /* Har_5_PH：5 次谐波相位给定，单位 rad。 */
/* Har_7_PH：7 次谐波相位给定，单位 rad。 */
/* Har_11_PH：11 次谐波相位给定，单位 rad。 */
extern float Har_5OR7_PH0,Har_11_PH0; /* Har_5OR7_PH0：原 5OR7 次谐波初始相位，单位 rad。 */
/* Har_11_PH0：原 11 次谐波初始相位，单位 rad。 */


//controller
extern float PLL_PID_Num_1; /* PLL_PID_Num_1：锁相环增量 PI 本拍误差系数。 */ 
extern float PLL_PID_Num_0; /* PLL_PID_Num_0：锁相环增量 PI 上一拍误差系数。 */		

	//voltage loop
extern float Volt_PID_Num_1; /* Volt_PID_Num_1：电压增量 PI 本拍误差系数。 */    
extern float Volt_PID_Num_0; /* Volt_PID_Num_0：电压增量 PI 上一拍误差系数。 */   

extern float PH_Volt_PID_Num_1; /* PH_Volt_PID_Num_1：相间电压增量 PI 本拍误差系数。 */ 
extern float PH_Volt_PID_Num_0; /* PH_Volt_PID_Num_0：相间电压增量 PI 上一拍误差系数。 */

	//Current loop
extern float Har_2_Controller_Num_2; /* Har_2_Controller_Num_2：2 次谐振控制器差分式分子第 2 项系数。 */         
extern float Har_2_Controller_Num_1; /* Har_2_Controller_Num_1：2 次谐振控制器差分式分子第 1 项系数。 */        
extern float Har_2_Controller_Num_0; /* Har_2_Controller_Num_0：2 次谐振控制器差分式分子第 0 项系数。 */				
extern float Har_2_Controller_Den_1; /* Har_2_Controller_Den_1：2 次谐振控制器差分式分母第 1 项系数。 */

extern float Har_6_Controller_Num_2; /* Har_6_Controller_Num_2：6 次谐振控制器差分式分子第 2 项系数。 */          		
extern float Har_6_Controller_Num_1; /* Har_6_Controller_Num_1：6 次谐振控制器差分式分子第 1 项系数。 */        		
extern float Har_6_Controller_Num_0; /* Har_6_Controller_Num_0：6 次谐振控制器差分式分子第 0 项系数。 */       			
extern float Har_6_Controller_Den_1; /* Har_6_Controller_Den_1：6 次谐振控制器差分式分母第 1 项系数。 */

extern float Har_12_Controller_Num_2; /* Har_12_Controller_Num_2：12 次谐振控制器差分式分子第 2 项系数。 */				 
extern float Har_12_Controller_Num_1; /* Har_12_Controller_Num_1：12 次谐振控制器差分式分子第 1 项系数。 */     	 
extern float Har_12_Controller_Num_0; /* Har_12_Controller_Num_0：12 次谐振控制器差分式分子第 0 项系数。 */   	  
extern float Har_12_Controller_Den_1; /* Har_12_Controller_Den_1：12 次谐振控制器差分式分母第 1 项系数。 */

extern float Current_PID_Num_1; /* Current_PID_Num_1：电流增量 PI 本拍误差系数。 */ 	           
extern float Current_PID_Num_0; /* Current_PID_Num_0：电流增量 PI 上一拍误差系数。 */
extern float Current_PID_Num_1_Pres; /* Current_PID_Num_1_Pres：电流增量 PI 本拍误差系数的预设值。 */
extern float Current_PID_Num_0_Pres; /* Current_PID_Num_0_Pres：电流增量 PI 上一拍误差系数的预设值。 */

extern float Neg_Current_PID_Num_1; /* Neg_Current_PID_Num_1：负序电流增量 PI 本拍误差系数。 */ 	      		
extern float Neg_Current_PID_Num_0; /* Neg_Current_PID_Num_0：负序电流增量 PI 上一拍误差系数。 */         		 

///////////////////////////////////////////////////////

extern float HA_Port_Volt_A; /* HA_Port_Volt_A：高压交流端口 A 相/相间原采样电压，单位 V。 */      //port voltage
extern float HA_Port_Volt_B; /* HA_Port_Volt_B：高压交流端口 B 相/相间原采样电压，单位 V。 */      
extern float HA_Port_Volt_C; /* HA_Port_Volt_C：高压交流端口 C 相/相间原采样电压，单位 V。 */      

extern float HA_Port_Volt_AB; /* HA_Port_Volt_AB：高压交流端口 AB 相/相间原采样电压，单位 V。 */
extern float HA_Port_Volt_BC; /* HA_Port_Volt_BC：高压交流端口 BC 相/相间原采样电压，单位 V。 */
extern float HA_Port_Volt_CA; /* HA_Port_Volt_CA：高压交流端口 CA 相/相间原采样电压，单位 V。 */

extern float HA_Port_Current_A; /* HA_Port_Current_A：高压交流端口 A 相/相间端口电流，单位 A。 */   //port current
extern float HA_Port_Current_B; /* HA_Port_Current_B：高压交流端口 B 相/相间端口电流，单位 A。 */   
extern float HA_Port_Current_C; /* HA_Port_Current_C：高压交流端口 C 相/相间端口电流，单位 A。 */   

extern float HA_Grid_Current_A; /* HA_Grid_Current_A：高压交流端口 A 相/相间电网电流，单位 A。 */   //system current
extern float HA_Grid_Current_B; /* HA_Grid_Current_B：高压交流端口 B 相/相间电网电流，单位 A。 */   
extern float HA_Grid_Current_C; /* HA_Grid_Current_C：高压交流端口 C 相/相间电网电流，单位 A。 */   


extern float HA_Port_Volt_AB_LATCH; /* HA_Port_Volt_AB_LATCH：高压交流端口 AB 相/相间原采样电压，单位 V；锁存采样值。 */      
extern float HA_Port_Volt_BC_LATCH; /* HA_Port_Volt_BC_LATCH：高压交流端口 BC 相/相间原采样电压，单位 V；锁存采样值。 */      
extern float HA_Port_Volt_CA_LATCH; /* HA_Port_Volt_CA_LATCH：高压交流端口 CA 相/相间原采样电压，单位 V；锁存采样值。 */      

extern float HA_Port_Current_A_LATCH; /* HA_Port_Current_A_LATCH：高压交流端口 A 相/相间端口电流，单位 A；锁存采样值。 */   
extern float HA_Port_Current_B_LATCH; /* HA_Port_Current_B_LATCH：高压交流端口 B 相/相间端口电流，单位 A；锁存采样值。 */   
extern float HA_Port_Current_C_LATCH; /* HA_Port_Current_C_LATCH：高压交流端口 C 相/相间端口电流，单位 A；锁存采样值。 */   

extern float HA_Grid_Current_A_LATCH; /* HA_Grid_Current_A_LATCH：高压交流端口 A 相/相间电网电流，单位 A；锁存采样值。 */   
extern float HA_Grid_Current_B_LATCH; /* HA_Grid_Current_B_LATCH：高压交流端口 B 相/相间电网电流，单位 A；锁存采样值。 */   
extern float HA_Grid_Current_C_LATCH; /* HA_Grid_Current_C_LATCH：高压交流端口 C 相/相间电网电流，单位 A；锁存采样值。 */   

extern float HA_PortUanRms_LF_1; /* HA_PortUanRms_LF_1：高压交流端口 PortUanRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_PortUanRms_LF_2; /* HA_PortUanRms_LF_2：高压交流端口 PortUanRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float HA_PortUbnRms_LF_1; /* HA_PortUbnRms_LF_1：高压交流端口 PortUbnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_PortUbnRms_LF_2; /* HA_PortUbnRms_LF_2：高压交流端口 PortUbnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float HA_PortUcnRms_LF_1; /* HA_PortUcnRms_LF_1：高压交流端口 PortUcnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_PortUcnRms_LF_2; /* HA_PortUcnRms_LF_2：高压交流端口 PortUcnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

extern float HA_PortIaRms_LF_1; /* HA_PortIaRms_LF_1：高压交流端口 PortIaRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_PortIaRms_LF_2; /* HA_PortIaRms_LF_2：高压交流端口 PortIaRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float HA_PortIbRms_LF_1; /* HA_PortIbRms_LF_1：高压交流端口 PortIbRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_PortIbRms_LF_2; /* HA_PortIbRms_LF_2：高压交流端口 PortIbRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float HA_PortIcRms_LF_1; /* HA_PortIcRms_LF_1：高压交流端口 PortIcRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_PortIcRms_LF_2; /* HA_PortIcRms_LF_2：高压交流端口 PortIcRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

extern float HA_GridIaRms_LF_1; /* HA_GridIaRms_LF_1：高压交流端口 GridIaRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_GridIaRms_LF_2; /* HA_GridIaRms_LF_2：高压交流端口 GridIaRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float HA_GridIbRms_LF_1; /* HA_GridIbRms_LF_1：高压交流端口 GridIbRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_GridIbRms_LF_2; /* HA_GridIbRms_LF_2：高压交流端口 GridIbRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float HA_GridIcRms_LF_1; /* HA_GridIcRms_LF_1：高压交流端口 GridIcRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float HA_GridIcRms_LF_2; /* HA_GridIcRms_LF_2：高压交流端口 GridIcRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

extern float HA_PortP_LF_1; /* HA_PortP_LF_1：高压交流端口有功低通滤波第 1 个历史状态。 */
extern float HA_PortP_LF_2; /* HA_PortP_LF_2：高压交流端口有功低通滤波第 2 个历史状态。 */
extern float HA_PortQ_LF_1; /* HA_PortQ_LF_1：高压交流端口无功低通滤波第 1 个历史状态。 */
extern float HA_PortQ_LF_2; /* HA_PortQ_LF_2：高压交流端口无功低通滤波第 2 个历史状态。 */

extern float LPF_5KHZ_X_Volt_A_1; /* LPF_5KHZ_X_Volt_A_1：原 5KHZ 低通滤波 X 的上一拍输入状态。 */
extern float LPF_5KHZ_Y_Volt_A_1; /* LPF_5KHZ_Y_Volt_A_1：原 5KHZ 低通滤波 Y 的上一拍输出状态。 */
extern float LPF_5KHZ_X_Volt_B_1; /* LPF_5KHZ_X_Volt_B_1：原 5KHZ 低通滤波 X 的上一拍输入状态。 */
extern float LPF_5KHZ_Y_Volt_B_1; /* LPF_5KHZ_Y_Volt_B_1：原 5KHZ 低通滤波 Y 的上一拍输出状态。 */

extern float LPF_20HZ_X_Ud_1; /* LPF_20HZ_X_Ud_1：原 20HZ 低通滤波 X 的上一拍输入状态。 */
extern float LPF_20HZ_Y_Ud_1; /* LPF_20HZ_Y_Ud_1：原 20HZ 低通滤波 Y 的上一拍输出状态。 */
extern float LPF_20HZ_X_Uq_1; /* LPF_20HZ_X_Uq_1：原 20HZ 低通滤波 X 的上一拍输入状态。 */
extern float LPF_20HZ_Y_Uq_1; /* LPF_20HZ_Y_Uq_1：原 20HZ 低通滤波 Y 的上一拍输出状态。 */

extern ushort HA_DC_Log_A; /* HA_DC_Log_A：高压交流端口 A 相/相间直流电压排序方向字。 */
extern ushort HA_DC_Log_B; /* HA_DC_Log_B：高压交流端口 B 相/相间直流电压排序方向字。 */
extern ushort HA_DC_Log_C; /* HA_DC_Log_C：高压交流端口 C 相/相间直流电压排序方向字。 */

extern ushort HA_IG_Log_A; /* HA_IG_Log_A：高压交流端口 A 相/相间电流方向控制字。 */
extern ushort HA_IG_Log_B; /* HA_IG_Log_B：高压交流端口 B 相/相间电流方向控制字。 */
extern ushort HA_IG_Log_C; /* HA_IG_Log_C：高压交流端口 C 相/相间电流方向控制字。 */

extern float HA_TotalVDC_A; /* HA_TotalVDC_A：高压交流端口 A 相/相间在线模块直流电压总和，单位 V。 */
extern float HA_TotalVDC_B; /* HA_TotalVDC_B：高压交流端口 B 相/相间在线模块直流电压总和，单位 V。 */
extern float HA_TotalVDC_C; /* HA_TotalVDC_C：高压交流端口 C 相/相间在线模块直流电压总和，单位 V。 */

extern float HVAC_RealVoltage_Sum[3]; /* HVAC_RealVoltage_Sum：HVAC 实时量窗口的电压累加缓存，供原有效值计算。 */
extern float HVAC_RealCurrent_Sum[3]; /* HVAC_RealCurrent_Sum：HVAC 实时量窗口的端口电流累加缓存，供原有效值计算。 */
extern float HVAC_RealGridCurrent_Sum[3]; /* HVAC_RealGridCurrent_Sum：HVAC 实时量窗口的电网电流累加缓存，供原有效值计算。 */
extern uchar HVAC_RealParameter_N; /* HVAC_RealParameter_N：HVAC 实时量窗口的采样采样窗口计数。 */

extern float HA_ReferenceDEF_A,HA_ReferenceDEF_B,HA_ReferenceDEF_C; /* HA_ReferenceDEF_A：高压交流端口 A 相/相间按在线模块数得到的 FPGA 调制参考基值。 */
/* HA_ReferenceDEF_B：高压交流端口 B 相/相间按在线模块数得到的 FPGA 调制参考基值。 */
/* HA_ReferenceDEF_C：高压交流端口 C 相/相间按在线模块数得到的 FPGA 调制参考基值。 */

extern ushort NHA_module_A; /* NHA_module_A：HA 端口 A 相在线模块数量，用于采样平均及调制尺度。 */
extern ushort NHA_module_B; /* NHA_module_B：HA 端口 B 相在线模块数量，用于采样平均及调制尺度。 */
extern ushort NHA_module_C; /* NHA_module_C：HA 端口 C 相在线模块数量，用于采样平均及调制尺度。 */
extern ushort HA_PM_A1; /* HA_PM_A1：HA 端口 A1 组在线模块位图，每位对应一个模块。 */
extern ushort HA_PM_A2; /* HA_PM_A2：HA 端口 A2 组在线模块位图，每位对应一个模块。 */
extern ushort HA_PM_B1; /* HA_PM_B1：HA 端口 B1 组在线模块位图，每位对应一个模块。 */
extern ushort HA_PM_B2; /* HA_PM_B2：HA 端口 B2 组在线模块位图，每位对应一个模块。 */
extern ushort HA_PM_C1; /* HA_PM_C1：HA 端口 C1 组在线模块位图，每位对应一个模块。 */
extern ushort HA_PM_C2; /* HA_PM_C2：HA 端口 C2 组在线模块位图，每位对应一个模块。 */

extern ushort HA_A_LV_Precharge; /* HA_A_LV_Precharge：高压交流端口 预充欠压判断标志。 */
extern ushort HA_B_LV_Precharge; /* HA_B_LV_Precharge：高压交流端口 预充欠压判断标志。 */
extern ushort HA_C_LV_Precharge; /* HA_C_LV_Precharge：高压交流端口 预充欠压判断标志。 */

extern ushort HA_A_HV_Protect; /* HA_A_HV_Protect：高压交流端口 原直流高压判断标志。 */
extern ushort HA_B_HV_Protect; /* HA_B_HV_Protect：高压交流端口 原直流高压判断标志。 */
extern ushort HA_C_HV_Protect; /* HA_C_HV_Protect：高压交流端口 原直流高压判断标志。 */

extern float HA_L_Voltage_Precharge; /* HA_L_Voltage_Precharge：原 HVAC 预充欠压判断阈值，单位 V。 */
extern float HA_H_Voltage; /* HA_H_Voltage：原 HVAC 直流高压判断阈值，单位 V。 */

extern float Ust_HVAC_Port; /* Ust_HVAC_Port：旧 HVAC 单模块直流电压运行给定，单位 V。 */
extern float UstOK_HVAC_Port; /* UstOK_HVAC_Port：旧 HVAC 启动电压达标值，单位 V。 */

extern float OverVolt_AlarmLevel2; /* OverVolt_AlarmLevel2：旧模块二级过压报警阈值，单位 V。 */
extern float VoltLevel_1; /* VoltLevel_1：旧电压判断第一级阈值。 */  
extern float VoltLevel_2; /* VoltLevel_2：旧电压判断第二级阈值。 */

//HVAC abnormal
extern unsigned char Port_Abnormal; /* Port_Abnormal：旧端口异常状态标志。 */
extern unsigned char Port_LV2; /* Port_LV2：旧端口第二级欠压状态。 */
extern unsigned char Flag_V_L; /* Flag_V_L：旧欠压阶段标志。 */
extern unsigned char Flag_V_L1_END; /* Flag_V_L1_END：旧一级欠压处理完成标志。 */
extern unsigned char Flag_V_L2_END; /* Flag_V_L2_END：旧二级欠压处理完成标志。 */
extern unsigned char Flag_V_L2_MID; /* Flag_V_L2_MID：旧二级欠压处理中间阶段标志。 */
extern unsigned char Flag_SlopeTime; /* Flag_SlopeTime：旧电压给定斜坡计时使能。 */
extern ushort SlopeTime_N; /* SlopeTime_N：旧电压斜坡的计时长度/计数。 */
extern ushort Cout_V_L; /* Cout_V_L：旧欠压处理计时计数。 */

//range
extern float HA_Module_RefUdc; /* HA_Module_RefUdc：旧 HVAC 单模块直流电压目标，单位 V。 */
extern float Ud_st_Max; /* Ud_st_Max：Ud 原控制给定上限，电压 V、电流 A。 */
extern float Ud_st_Min; /* Ud_st_Min：Ud 原控制给定下限，电压 V、电流 A。 */
extern float Uq_st_Max; /* Uq_st_Max：Uq 原控制给定上限，电压 V、电流 A。 */
extern float Uq_st_Min; /* Uq_st_Min：Uq 原控制给定下限，电压 V、电流 A。 */
extern float Idst_Max; /* Idst_Max：Idst 原控制给定上限，电压 V、电流 A。 */
extern float Idst_Min; /* Idst_Min：Idst 原控制给定下限，电压 V、电流 A。 */
extern float Iqst_Max; /* Iqst_Max：Iqst 原控制给定上限，电压 V、电流 A。 */
extern float Iqst_Min; /* Iqst_Min：Iqst 原控制给定下限，电压 V、电流 A。 */

//PLL
extern float Phase_PLL_First; /* Phase_PLL_First：旧 PLL 首拍相角缓存，单位 rad。 */
extern float PLL_Phase_Com; /* PLL_Phase_Com：旧电流变换相位补偿，单位 rad。 */
extern float PLL_Phase_Com_NEW; /* PLL_Phase_Com_NEW：旧输出变换相位补偿，单位 rad。 */
extern float PhaseCom; /* PhaseCom：旧附加相位补偿量，单位 rad。 */
extern float Sin_ph_Trans_Com; /* Sin_ph_Trans_Com：正弦坐标变换系数，含原相位补偿。 */
extern float Cos_ph_Trans_Com; /* Cos_ph_Trans_Com：余弦坐标变换系数，含原相位补偿。 */	
extern float Sin_ph_Trans_Com_NEW; /* Sin_ph_Trans_Com_NEW：正弦坐标变换系数，含原相位补偿，输出变换使用值。 */
extern float Cos_ph_Trans_Com_NEW; /* Cos_ph_Trans_Com_NEW：余弦坐标变换系数，含原相位补偿，输出变换使用值。 */
extern float Sin_ph_Trans_next; /* Sin_ph_Trans_next：正弦坐标变换系数，下一拍预测值。 */
extern float Cos_ph_Trans_next; /* Cos_ph_Trans_next：余弦坐标变换系数，下一拍预测值。 */
extern float Ugrid_Max; /* Ugrid_Max：旧控制使用的电网电压幅值上限/估计值，单位 V。 */	
////////////////////////////////////////////
//Reference
extern float HA_Module_RefVolt_A; /* HA_Module_RefVolt_A：高压交流端口 A 相/相间模块电压调制给定，单位 V。 */
extern float HA_Module_RefVolt_B; /* HA_Module_RefVolt_B：高压交流端口 B 相/相间模块电压调制给定，单位 V。 */
extern float HA_Module_RefVolt_C; /* HA_Module_RefVolt_C：高压交流端口 C 相/相间模块电压调制给定，单位 V。 */

extern ushort HA_DABDuty_A; /* HA_DABDuty_A：高压交流端口 A 相/相间DAB 占空比 FPGA 计数。 */
extern ushort HA_DABDuty_B; /* HA_DABDuty_B：高压交流端口 B 相/相间DAB 占空比 FPGA 计数。 */
extern ushort HA_DABDuty_C; /* HA_DABDuty_C：高压交流端口 C 相/相间DAB 占空比 FPGA 计数。 */
extern ushort HA_DAB_PluseStart_A; /* HA_DAB_PluseStart_A：高压交流端口 A 相/相间DAB 脉冲起始 FPGA 计数。 */
extern ushort HA_DAB_PluseStart_B; /* HA_DAB_PluseStart_B：高压交流端口 B 相/相间DAB 脉冲起始 FPGA 计数。 */
extern ushort HA_DAB_PluseStart_C; /* HA_DAB_PluseStart_C：高压交流端口 C 相/相间DAB 脉冲起始 FPGA 计数。 */
extern ushort HA_DAB_T; /* HA_DAB_T：高压交流端口 DAB 周期 FPGA 计数。 */
/////////////////////////////////////////////

extern unsigned char Add_3th; /* Add_3th：旧三次谐波注入使能标志。 */
extern unsigned char Method_3th; /* Method_3th：旧三次谐波注入算法选择。 */
extern unsigned char flag_Standard; /* flag_Standard：旧调制标准化方式选择标志。 */
extern float StandardValue; /* StandardValue：旧调制参考标准化基值。 */
extern unsigned char DAB_ONLY; /* DAB_ONLY：旧仅运行 DAB 的测试选择标志。 */

//edited by shiqi
extern ushort counter_OC_flag; /* counter_OC_flag：原一级过流连续超限样本计数。 */
extern ushort counter_OC_time; /* counter_OC_time：原一级过流保护确认计时计数。 */
extern ushort counter_OC_flag1; /* counter_OC_flag1：原二级过流连续超限样本计数。 */
extern ushort counter_OC_time1; /* counter_OC_time1：原二级过流保护确认计时计数。 */
extern ushort OC_OV_LV_PortLatch_TAD; /* OC_OV_LV_PortLatch_TAD：原软件过流/过压/欠压的 TAD 故障锁存值。 */
extern ushort HVAC_OC_Fault; /* HVAC_OC_Fault：原 HVAC 软件过流故障确认标志。 */
extern ushort OC_level_SET; /* OC_level_SET：原一级过流阈值，沿用采样计数/系数约定。 */
extern ushort OC_time_SET; /* OC_time_SET：原一级过流确认计数阈值，单位 TAD。 */
extern ushort OC_level_SET_L2; /* OC_level_SET_L2：原二级过流阈值，沿用采样计数/系数约定。 */
extern ushort OC_time_SET_L2; /* OC_time_SET_L2：原二级过流确认计数阈值，单位 TAD。 */
/* OC_level_SET_DEFAULT：原一级过流保护默认阈值。 */
#define OC_level_SET_DEFAULT 70
/* OC_time_SET_DEFAULT：原一级过流确认默认 TAD 计数。 */
#define OC_time_SET_DEFAULT 100
/* OC_level_SET_L2_DEFAULT：原二级过流保护默认阈值。 */
#define OC_level_SET_L2_DEFAULT 75
/* OC_time_SET_L2_DEFAULT：原二级过流确认默认 TAD 计数。 */
#define OC_time_SET_L2_DEFAULT 5
extern ushort counter_OV_flag; /* counter_OV_flag：原一级过压连续超限样本计数。 */
extern ushort counter_OV_time; /* counter_OV_time：原一级过压保护确认计时计数。 */
extern ushort HVAC_OV_Fault; /* HVAC_OV_Fault：原 HVAC 软件过压故障确认标志。 */
extern ushort OV_level_SET; /* OV_level_SET：原直流过压阈值，沿用采样计数/系数约定。 */
extern ushort OV_time_SET; /* OV_time_SET：原过压确认计数阈值，单位 TAD。 */
/* OV_level_SET_DEFAULT：原过压保护默认阈值。 */
#define OV_level_SET_DEFAULT 830
/* OV_time_SET_DEFAULT：原过压确认默认 TAD 计数。 */
#define OV_time_SET_DEFAULT 5
/* OV_allport_shutdown：原过压时全端口关断策略开关。 */
#define OV_allport_shutdown 900
extern float HA_Port_Volt_A_last, HA_Port_Volt_B_last, HA_Port_Volt_C_last; /* HA_Port_Volt_A_last：高压交流端口 A 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
/* HA_Port_Volt_B_last：高压交流端口 B 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
/* HA_Port_Volt_C_last：高压交流端口 C 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
extern float VA_Control, VB_Control, VC_Control; /* VA_Control：A 相原输出电压控制给定，单位 V。 */
/* VB_Control：B 相原输出电压控制给定，单位 V。 */
/* VC_Control：C 相原输出电压控制给定，单位 V。 */
extern ushort Samplenoise; /* Samplenoise：原交流电压采样噪声判断状态。 */
extern float Iin_d_Positive_last, Iin_q_Positive_last; /* Iin_d_Positive_last：端口电流 d 轴/分量的正序基波值，单位 A；上一拍噪声比较缓存。 */
/* Iin_q_Positive_last：端口电流 q 轴/分量的正序基波值，单位 A；上一拍噪声比较缓存。 */
extern ushort Samplenoise_Idq; /* Samplenoise_Idq：原 dq 电流采样噪声判断状态。 */
extern float SVG_Udc_Ave_temp, SVG_Udc_Ave_temp_last; /* SVG_Udc_Ave_temp：旧直流平均电压噪声处理的本拍缓存，单位 V。 */
/* SVG_Udc_Ave_temp_last：旧直流平均电压噪声处理的上一拍缓存，单位 V。 */
extern ushort Samplenoise_udc; /* Samplenoise_udc：原直流电压采样噪声判断状态。 */
extern ushort GNDSwitch, ISOSwitch; /* GNDSwitch：原接地开关状态。 */
/* ISOSwitch：原隔离开关状态。 */
extern ushort Contactor_err; /* Contactor_err：原接触器动作异常标志。 */
extern ushort Breaker_err; /* Breaker_err：原断路器动作异常标志。 */
extern ushort ISO_counter_on, ISO_counter_off; /* ISO_counter_on：原隔离开关闭合确认计数。 */
/* ISO_counter_off：原隔离开关断开确认计数。 */
//end
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
#ifdef LVAC_online
extern float LA_Grid_Volt_A; /* LA_Grid_Volt_A：低压交流端口 A 相/相间原采样电压，单位 V。 */   	 //system voltage
extern float LA_Grid_Volt_B; /* LA_Grid_Volt_B：低压交流端口 B 相/相间原采样电压，单位 V。 */      
extern float LA_Grid_Volt_C; /* LA_Grid_Volt_C：低压交流端口 C 相/相间原采样电压，单位 V。 */      

extern float LA_Grid_Current_A; /* LA_Grid_Current_A：低压交流端口 A 相/相间电网电流，单位 A。 */   //system current
extern float LA_Grid_Current_B; /* LA_Grid_Current_B：低压交流端口 B 相/相间电网电流，单位 A。 */   
extern float LA_Grid_Current_C; /* LA_Grid_Current_C：低压交流端口 C 相/相间电网电流，单位 A。 */   

extern float LA_Port_Volt_A; /* LA_Port_Volt_A：低压交流端口 A 相/相间原采样电压，单位 V。 */      //port voltage
extern float LA_Port_Volt_B; /* LA_Port_Volt_B：低压交流端口 B 相/相间原采样电压，单位 V。 */      
extern float LA_Port_Volt_C; /* LA_Port_Volt_C：低压交流端口 C 相/相间原采样电压，单位 V。 */      

extern float LA_Lg_Current_A; /* LA_Lg_Current_A：低压交流端口 A 相/相间滤波电感电流，单位 A。 */   	 //inductance current
extern float LA_Lg_Current_B; /* LA_Lg_Current_B：低压交流端口 B 相/相间滤波电感电流，单位 A。 */     
extern float LA_Lg_Current_C; /* LA_Lg_Current_C：低压交流端口 C 相/相间滤波电感电流，单位 A。 */     

extern float LA_Cap_Current_A; /* LA_Cap_Current_A：低压交流端口 A 相/相间电容电流，单位 A。 */    //capacitance current
extern float LA_Cap_Current_B; /* LA_Cap_Current_B：低压交流端口 B 相/相间电容电流，单位 A。 */    
extern float LA_Cap_Current_C; /* LA_Cap_Current_C：低压交流端口 C 相/相间电容电流，单位 A。 */    

extern float LA_DCbus_Volt; /* LA_DCbus_Volt：低压交流端口 原采样电压，单位 V。 */       //DCbus voltage


extern float LA_Grid_Volt_A_LATCH; /* LA_Grid_Volt_A_LATCH：低压交流端口 A 相/相间原采样电压，单位 V；锁存采样值。 */   	 
extern float LA_Grid_Volt_B_LATCH; /* LA_Grid_Volt_B_LATCH：低压交流端口 B 相/相间原采样电压，单位 V；锁存采样值。 */      
extern float LA_Grid_Volt_C_LATCH; /* LA_Grid_Volt_C_LATCH：低压交流端口 C 相/相间原采样电压，单位 V；锁存采样值。 */      

extern float LA_Grid_Current_A_LATCH; /* LA_Grid_Current_A_LATCH：低压交流端口 A 相/相间电网电流，单位 A；锁存采样值。 */   
extern float LA_Grid_Current_B_LATCH; /* LA_Grid_Current_B_LATCH：低压交流端口 B 相/相间电网电流，单位 A；锁存采样值。 */   
extern float LA_Grid_Current_C_LATCH; /* LA_Grid_Current_C_LATCH：低压交流端口 C 相/相间电网电流，单位 A；锁存采样值。 */   

extern float LA_Port_Volt_A_LATCH; /* LA_Port_Volt_A_LATCH：低压交流端口 A 相/相间原采样电压，单位 V；锁存采样值。 */      
extern float LA_Port_Volt_B_LATCH; /* LA_Port_Volt_B_LATCH：低压交流端口 B 相/相间原采样电压，单位 V；锁存采样值。 */      
extern float LA_Port_Volt_C_LATCH; /* LA_Port_Volt_C_LATCH：低压交流端口 C 相/相间原采样电压，单位 V；锁存采样值。 */      

extern float LA_Lg_Current_A_LATCH; /* LA_Lg_Current_A_LATCH：低压交流端口 A 相/相间滤波电感电流，单位 A；锁存采样值。 */   	 
extern float LA_Lg_Current_B_LATCH; /* LA_Lg_Current_B_LATCH：低压交流端口 B 相/相间滤波电感电流，单位 A；锁存采样值。 */     
extern float LA_Lg_Current_C_LATCH; /* LA_Lg_Current_C_LATCH：低压交流端口 C 相/相间滤波电感电流，单位 A；锁存采样值。 */     

extern float LA_Cap_Current_A_LATCH; /* LA_Cap_Current_A_LATCH：低压交流端口 A 相/相间电容电流，单位 A；锁存采样值。 */    
extern float LA_Cap_Current_B_LATCH; /* LA_Cap_Current_B_LATCH：低压交流端口 B 相/相间电容电流，单位 A；锁存采样值。 */    
extern float LA_Cap_Current_C_LATCH; /* LA_Cap_Current_C_LATCH：低压交流端口 C 相/相间电容电流，单位 A；锁存采样值。 */    

extern float LA_DCbus_Volt_LATCH; /* LA_DCbus_Volt_LATCH：低压交流端口 原采样电压，单位 V；锁存采样值。 */       

extern float LA_GridUanRms_LF_1; /* LA_GridUanRms_LF_1：低压交流端口 GridUanRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_GridUanRms_LF_2; /* LA_GridUanRms_LF_2：低压交流端口 GridUanRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float LA_GridUbnRms_LF_1; /* LA_GridUbnRms_LF_1：低压交流端口 GridUbnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_GridUbnRms_LF_2; /* LA_GridUbnRms_LF_2：低压交流端口 GridUbnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float LA_GridUcnRms_LF_1; /* LA_GridUcnRms_LF_1：低压交流端口 GridUcnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_GridUcnRms_LF_2; /* LA_GridUcnRms_LF_2：低压交流端口 GridUcnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

extern float LA_GridIaRms_LF_1; /* LA_GridIaRms_LF_1：低压交流端口 GridIaRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_GridIaRms_LF_2; /* LA_GridIaRms_LF_2：低压交流端口 GridIaRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float LA_GridIbRms_LF_1; /* LA_GridIbRms_LF_1：低压交流端口 GridIbRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_GridIbRms_LF_2; /* LA_GridIbRms_LF_2：低压交流端口 GridIbRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float LA_GridIcRms_LF_1; /* LA_GridIcRms_LF_1：低压交流端口 GridIcRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_GridIcRms_LF_2; /* LA_GridIcRms_LF_2：低压交流端口 GridIcRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

extern float LA_PortUanRms_LF_1; /* LA_PortUanRms_LF_1：低压交流端口 PortUanRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_PortUanRms_LF_2; /* LA_PortUanRms_LF_2：低压交流端口 PortUanRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float LA_PortUbnRms_LF_1; /* LA_PortUbnRms_LF_1：低压交流端口 PortUbnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_PortUbnRms_LF_2; /* LA_PortUbnRms_LF_2：低压交流端口 PortUbnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */
extern float LA_PortUcnRms_LF_1; /* LA_PortUcnRms_LF_1：低压交流端口 PortUcnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */
extern float LA_PortUcnRms_LF_2; /* LA_PortUcnRms_LF_2：低压交流端口 PortUcnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

extern float LA_PortP_LF_1; /* LA_PortP_LF_1：低压交流端口有功低通滤波第 1 个历史状态。 */
extern float LA_PortP_LF_2; /* LA_PortP_LF_2：低压交流端口有功低通滤波第 2 个历史状态。 */
extern float LA_PortQ_LF_1; /* LA_PortQ_LF_1：低压交流端口无功低通滤波第 1 个历史状态。 */
extern float LA_PortQ_LF_2; /* LA_PortQ_LF_2：低压交流端口无功低通滤波第 2 个历史状态。 */

//extern float LA_TotalVDC;

extern ushort NLA_module; /* NLA_module：LA 端口在线模块数量，用于采样平均及调制尺度。 */
extern ushort LA_PM_A1; /* LA_PM_A1：LA 端口 A1 组在线模块位图，每位对应一个模块。 */
extern ushort LA_PM_A2; /* LA_PM_A2：LA 端口 A2 组在线模块位图，每位对应一个模块。 */

extern float LA_RefVolt_A; /* LA_RefVolt_A：低压交流端口 A 相/相间模块电压调制给定，单位 V。 */
extern float LA_RefVolt_B; /* LA_RefVolt_B：低压交流端口 B 相/相间模块电压调制给定，单位 V。 */
extern float LA_RefVolt_C; /* LA_RefVolt_C：低压交流端口 C 相/相间模块电压调制给定，单位 V。 */

//Reference
extern ushort LA_DABDuty; /* LA_DABDuty：低压交流端口 DAB 占空比 FPGA 计数。 */
extern ushort LA_DAB_PluseStart; /* LA_DAB_PluseStart：低压交流端口 DAB 脉冲起始 FPGA 计数。 */
extern ushort LA_DAB_T; /* LA_DAB_T：低压交流端口 DAB 周期 FPGA 计数。 */
#endif

/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 按原项目初始化控制系数、模块位图、保护阈值及运行变量
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void Initial_Variable(void);
/*==================================================================
 * 函数功能: 复位滤波器、PI、遥控、故障以及原运行流程的历史状态
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void Variable_RST(void);
/*==================================================================
 * 函数功能: 依次检查各相在线模块直流电压，更新预充及高压判断标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void Udc_check(void);
/*==================================================================
 * 函数功能: 执行板级初始化使用的原循环延时
 * 输入参数: n：本次有效数据数量或循环次数，按调用接口的长度单位解释
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void Delay(unsigned int n);
/*==================================================================
 * 函数功能: 使能板级外设对应的 PSC 电源/时钟模块
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void PSCInit(void);
/*==================================================================
 * 函数功能: 配置板级 GPIO 的引脚复用
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void GPIOBankPinMuxSet(void);
/*==================================================================
 * 函数功能: 初始化 LED、看门狗、保护和通信相关 GPIO 方向与初值
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void GPIOBankPinInit(void);
/*==================================================================
 * 函数功能: 配置 FPGA/DPRAM 所用 EMIFA 时序及总线引脚
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void EMIFASetup(void);
/*==================================================================
 * 函数功能: 初始化 Timer2 周期计数，沿用原 10 ms 定时配置
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void TimerInit(void);
/*==================================================================
 * 函数功能: 将 Timer2 中断源与原 ISR 及 CPU 向量绑定
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void TimerInterruptInit(void);
/*==================================================================
 * 函数功能: 初始化 UART2 波特率、数据格式及 FIFO
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void UARTInit(void);
/*==================================================================
 * 函数功能: 保留 UART 初始化兼容入口，调试收发采用后台轮询
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void UARTInterruptInit(void);
/*==================================================================
 * 函数功能: 绑定 FPGA TAD、FAULT 以及可选 DPRAM 通知 GPIO 中断
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void GPIOBankPinInterruptInit(void);
/*==================================================================
 * 函数功能: 初始化 DSP 中断控制器并使能所需 CPU 中断通道
 * 输入参数: 无
 * 输出参数: 相关芯片配置寄存器；毫秒读函数只读取计数及维护本地单调结果。
 * 返回值: 无
 * 执行说明: 板级初始化或后台时基服务；不引入新的实时 ISR 工作。
 *================================================================*/
extern void InterruptInit(void);
/*==================================================================
 * 函数功能: 保留旧 UART 发送声明；迁移工程中无对应实现或调用
 * 输入参数: PCcomm：旧 UART 通信请求缓存
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void Uart2_send(unsigned char PCcomm);
/*==================================================================
 * 函数功能: 保留旧 UART 接收处理声明；迁移工程中无对应实现或调用
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void UARTReceiveDispose(void);
/*==================================================================
 * 函数功能: 计算旧 UART 兼容帧的 CRC16，保持原多项式及交换字节规则
 * 输入参数: ba：CRC 输入缓冲区首地址，调用者提供有效帧字节
 *           size：输入缓存有效长度
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 原实现生成的 16 位 CRC 值
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern unsigned short CRC16(unsigned char *ba, int size);
/*==================================================================
 * 函数功能: 保留原本地按键扫描入口及按键状态处理
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void keyscan(void);
/*==================================================================
 * 函数功能: 应答 Timer2 中断并更新原软件定时、状态及保护服务
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void Timer2Isr(void);
/*==================================================================
 * 函数功能: 保留原 UART2 ISR 符号；调试通信主流程不注册此中断
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void UART2Isr(void);
/*==================================================================
 * 函数功能: 应答 FPGA TAD 外部中断并按原顺序完成 10 kHz 采样、保护及控制
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void FPGAIsr(void);
/*==================================================================
 * 函数功能: 应答 FPGA 硬件故障中断并执行原快速故障处置
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void FAULTIsr(void);
/*==================================================================
 * 函数功能: 应答可选 DPRAM GPIO 通知并置后台读取标志
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
extern void DPRAMIsr(void);
