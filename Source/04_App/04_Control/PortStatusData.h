/* 文件职责: 端口运行状态与联锁兼容镜像。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef PORTSTATUSDATA_H_
#define PORTSTATUSDATA_H_
#include "SstTypes.h"
#include "PortStatusTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

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

extern XXG_PORT_STATUS Port_RealStatus; /* Port_RealStatus：端口开关及桥状态的原位域缓存。 */

extern XXG_SwitchFLAG SW; /* SW：旧开关时序、动作状态及完成标志的位域缓存。 */

extern XXG_PortRunFLAG PortRunFLAG; /* PortRunFLAG：旧端口运行、预充、PWM 及相序状态的位域缓存。 */

extern XXG_ControlFLAG ControlFLAG; /* ControlFLAG：旧启动、停止、PWM、DAB 斜坡流程控制标志。 */

extern unsigned char Flag_ComTest; /* Flag_ComTest：旧端口通信测试标志。 */

extern unsigned short FourPortComZhi; /* FourPortComZhi：旧四端口通信测试状态字。 */

extern ushort SWTable_TotalTime; /* SWTable_TotalTime：旧开关动作表的总定时时间。 */

extern ushort Port_OnState; /* Port_OnState：原端口已投入状态。 */


#ifdef __cplusplus
}
#endif
#endif
