/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef DPRAMCONFIG_H_
#define DPRAMCONFIG_H_

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif
#endif
