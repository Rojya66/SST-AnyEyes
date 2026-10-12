/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef SSTBOARDCONFIG_H_
#define SSTBOARDCONFIG_H_
#include "soc_C6748.h"

/* SYSCLK_1_FREQ：板级 SYSCLK1 频率，单位 Hz。 */
#define SYSCLK_1_FREQ     (456000000)
/* SYSCLK_2_FREQ：板级 SYSCLK2 频率，单位 Hz。 */
#define SYSCLK_2_FREQ     (SYSCLK_1_FREQ/2)
/* UART_2_FREQ：板级 UART2 时钟频率，单位 Hz。 */
#define UART_2_FREQ       (SYSCLK_2_FREQ)

#ifdef __cplusplus
extern "C" {
#endif
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

#ifdef __cplusplus
}
#endif
#endif
