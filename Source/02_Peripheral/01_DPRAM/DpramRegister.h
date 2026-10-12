/* 文件职责: DPRAM 固定映射与原块协议布局。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef DPRAMREGISTER_H_
#define DPRAMREGISTER_H_
#include "SstTypes.h"
#include "FpgaRegister.h"
#include "SstWireTypes.h"
#include "DpramConfig.h"

#ifdef __cplusplus
extern "C" {
#endif
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

/* DPRAM_Block4_PluginUnit16：原地址映射的插件16遥测块，仅启用DPRAM时访问。 */

/* DPRAM_Block4_PluginUnit17：原地址映射的插件 17 遥测块，仅启用 DPRAM 时访问。 */

extern volatile XXG_DPRAM_RemMeaSig DPRAM_Block3_remMeaSig; /* DPRAM_Block3_remMeaSig：原地址映射的端口遥测区；访问受 SST_ENABLE_DPRAM 门控。 */

extern volatile XXG_DPRAM_RemControl DPRAM_Block1_remControl; /* DPRAM_Block1_remControl：原地址映射的遥控区；访问受 SST_ENABLE_DPRAM 门控。 */

extern volatile XXG_DPRAM_RemRegulate DPRAM_Block2_remRegulate; /* DPRAM_Block2_remRegulate：原地址映射的遥调区；访问受 SST_ENABLE_DPRAM 门控。 */


#ifdef __cplusplus
}
#endif
#endif
