/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef PROTECTIONTYPES_H_
#define PROTECTIONTYPES_H_
#include "SstTypes.h"
#include "SstWireTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef union _XXG_FaultTypeFLAG
    {
        struct _FaultTypeFLAG
        {
            ushort Port_OverVolt : 1; /* Port_OverVolt：原端口过压故障分类位。 */ //0--no fault;
            ushort Port_OverCurrent : 1; /* Port_OverCurrent：原端口过流故障分类位。 */
            ushort Module_DAB_H : 1; /* Module_DAB_H：原模块 DAB 桥故障分类位。 */
            ushort Module_50Hz_H : 1; /* Module_50Hz_H：原模块工频桥故障分类位。 */
            ushort Module_Single : 1; /* Module_Single：原单模块故障分类位。 */
            ushort Module_OverVolt : 1; /* Module_OverVolt：原模块过压故障分类位。 */
            ushort Module_OverCurrent : 1; /* Module_OverCurrent：原模块过流故障分类位。 */
            ushort Module_OverTemp : 1; /* Module_OverTemp：原模块过温故障分类位。 */

            ushort Contactor : 1; /* Contactor：原接触器动作故障分类位。 */
            ushort Breaker : 1; /* Breaker：原断路器动作故障分类位。 */
            ushort Controller : 1; /* Controller：原控制器故障分类位。 */
            ushort Sync : 1; /* Sync：原同步故障分类位。 */
            ushort Communication : 1; /* Communication：原通信故障分类位。 */

            ushort Module_Fault : 1; /* Module_Fault：原模块汇总故障分类位。 */
            ushort Module_LowerVolt : 1; /* Module_LowerVolt：原模块欠压故障分类位。 */
            ushort Module_OverVoltLevel2 : 1; /* Module_OverVoltLevel2：原模块二级过压故障分类位。 */
        } Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */

        ushort FaultTypeFLAG16; /* FaultTypeFLAG16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
    } XXG_FaultTypeFLAG;

    typedef union _XXG_FaultDisposeFLAG
    {
        struct _FaultDisposeFLAG
        {
            ushort FaultDisposeType : 2;
            /* FaultDisposeType：原故障处置策略类型字段。 */ //0--no fault;      1--PWM pluse disable;   2--port shutdown

            ushort PWMPulseState : 3;
            /* PWMPulseState：原故障处置中的 PWM 状态字段。 */ //0--no pulse disable； 1--50Hz H-Bridge/Module pulse disable；2--DAB H-Bridge pulse disable; 3--port pulse disable
            ushort PWMPulseDisableNum : 1; /* PWMPulseDisableNum：原策略需要禁用 PWM 的模块数/编号字段。 */ //0~2
            ushort PWMEN_FibreOptical : 1; /* PWMEN_FibreOptical：原光纤 PWM 使能控制字段。 */ //0--off;1--on
            //		ushort Port_PWMDisable:    2;   //01: PWM pulse shutting;  10: PWM pulse shutting success; 11: PWM pulse shutting failure
            //		ushort Port_PWMEnable:     2;   //01: PWM pulse opening;  10: PWM pulse opening success; 11: PWM pulse opening failure

            ushort Port_StopType : 3;
            /* Port_StopType：旧端口停机类型字段。 */ //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown
            ushort Port_StopAll : 1; /* Port_StopAll：旧故障策略要求全端口停机的标志。 */ //1: system shutdown

            ushort Reservedbit : 5; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
        } Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */

        ushort FaultDisposeFLAG16; /* FaultDisposeFLAG16：本联合体的完整 16 位字视图，与位域成员共用存储。 */

    } XXG_FaultDisposeFLAG;

#ifdef __cplusplus
}
#endif
#endif
