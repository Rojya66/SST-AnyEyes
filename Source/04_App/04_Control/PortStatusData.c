#include "FpgaRegister.h"
#include "PortStatusTypes.h"
#include "SstTypes.h"
/* 文件职责: 端口运行状态与联锁兼容镜像；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "PortStatusData.h"
#include "SstControlConfig.h"

XXG_MainStatus MainStatus_HVAC, /* MainStatus_HVAC：HVAC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
    MainStatus_HVDC, /* MainStatus_HVDC：HVDC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
    MainStatus_LVDC, /* MainStatus_LVDC：LVDC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
    MainStatus_LVAC, /* MainStatus_LVAC：LVAC 端口主状态及模式缓存；仅对应条件编译分支使用。 */
    Port_MainStatus; /* Port_MainStatus：当前端口主状态、主/子工作模式的位域缓存。 */

XXG_MainStatus Port_MainStatus; /* Port_MainStatus：当前端口主状态、主/子工作模式的位域缓存。 */

ushort Port_Main_WorkMode; /* Port_Main_WorkMode：当前端口主工作模式编号。 */

ushort Port_STATCOM_WorkMode; /* Port_STATCOM_WorkMode：旧 STATCOM 无功/功率因数/电压工作模式编号。 */

ushort Port_APF_WorkMode; /* Port_APF_WorkMode：旧 APF 谐波补偿工作模式编号。 */

ushort Port_Outer_Elec; /* Port_Outer_Elec：端口外部电源接入状态。 */

ushort Port_Sys_Stop; /* Port_Sys_Stop：旧系统停机状态。 */

ushort Port_State; /* Port_State：旧端口状态编号，迁移核心维护兼容镜像。 */

ushort Port_RunStatus; /* Port_RunStatus：旧端口运行状态编号。 */

XXG_PORT_STATUS Port_RealStatus; /* Port_RealStatus：端口开关及桥状态的原位域缓存。 */

XXG_SwitchFLAG SW; /* SW：旧开关时序、动作状态及完成标志的位域缓存。 */

XXG_PortRunFLAG PortRunFLAG; /* PortRunFLAG：旧端口运行、预充、PWM 及相序状态的位域缓存。 */

XXG_ControlFLAG ControlFLAG; /* ControlFLAG：旧启动、停止、PWM、DAB 斜坡流程控制标志。 */

unsigned char Flag_ComTest; /* Flag_ComTest：旧端口通信测试标志。 */

unsigned short FourPortComZhi; /* FourPortComZhi：旧四端口通信测试状态字。 */

ushort SWTable_TotalTime; /* SWTable_TotalTime：旧开关动作表的总定时时间。 */

ushort Port_OnState; /* Port_OnState：原端口已投入状态。 */
