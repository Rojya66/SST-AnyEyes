/* 文件职责: FT3 映射、录波状态和采样序号。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef FT3RECORDERDATA_H_
#define FT3RECORDERDATA_H_
#include "SstTypes.h"
#include "FpgaRegister.h"

#ifdef __cplusplus
extern "C" {
#endif
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

extern volatile XXG_FT3_Register  FT3_PTCT_Register,FT3_FaultRecorder_Register,FT3_EER2_Register,FT3_Backup_Register; /* FT3_PTCT_Register：PT/CT FT3 插件的原映射寄存器窗口。 */

/* FT3_FaultRecorder_Register：故障录波 FT3 插件的原映射寄存器窗口。 */

/* FT3_EER2_Register：EER2 FT3 插件的原映射寄存器窗口。 */

/* FT3_Backup_Register：备用 FT3 插件的原映射寄存器窗口。 */

extern ushort faultsignal; /* faultsignal：原 FT3 故障触发信号。 */

extern ushort faultRecordStart; /* faultRecordStart：原 FT3 录波启动标志。 */

extern ushort fault_count; /* fault_count：原故障录波触发计数。 */

extern ushort fault_count_start; /* fault_count_start：原故障录波开始计数/状态。 */

extern ushort FR_tri_count1, FR_tri_count2, FR_tri_count3; /* FR_tri_count1：原 FT3 录波第 1 组触发计数。 */

/* FR_tri_count2：原 FT3 录波第 2 组触发计数。 */

/* FR_tri_count3：原 FT3 录波第 3 组触发计数。 */

extern unsigned short FT3_Samp_Cnt; /* FT3_Samp_Cnt：FT3 上报的采样序号。 */

extern unsigned short dataPackNum; /* dataPackNum：FT3 数据包数量/控制字字段。 */


#ifdef __cplusplus
}
#endif
#endif
