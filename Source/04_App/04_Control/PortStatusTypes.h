/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef PORTSTATUSTYPES_H_
#define PORTSTATUSTYPES_H_
#include "SstTypes.h"
#include "SstWireTypes.h"

#ifdef __cplusplus
extern "C" {
#endif
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

#ifdef __cplusplus
}
#endif
#endif
