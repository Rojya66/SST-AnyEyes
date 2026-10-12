/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef SSTCONTROLCONFIG_H_
#define SSTCONTROLCONFIG_H_
#include "ChbBuildConfig.h"

#ifdef __cplusplus
extern "C" {
#endif
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

 

#ifdef __cplusplus
}
#endif
#endif
