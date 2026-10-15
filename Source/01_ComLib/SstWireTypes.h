/* 文件职责: FPGA/DPRAM 共用的线上位域字；仅定义字段布局，不拥有应用状态或硬件映射对象。 */
#ifndef SST_WIRE_TYPES_H
#define SST_WIRE_TYPES_H
#include "SstTypes.h"

typedef union _XXG_MainStatus
{
    struct _MainStatus
    {
        ushort P_Status : 6; /* P_Status：端口运行状态:端口运行状态：。 */ //端口运行状态:端口运行状态：
        //0x00——初始化中；0x01——初始化成功；0x02——自检开始（有模块上电）；0x03——预充电完成（端口每个模块都上电）；
        //0x04——自检失败；0x05——就绪（自检通过）；0x20——正常运行；0x07——停机；0x08：故障停机。
        ushort Sys_Stop : 1; /* Sys_Stop：FT3 录波/端口主状态的系统停机位。 */ //1:整机停止运行；
        ushort Outer_Elec : 1; /* Outer_Elec：FT3 录波/端口主状态的外部电源接入位。 */ //1:外部母线带电；
        ushort P_S2_Mode : 2;
        /* P_S2_Mode：端口附加模式二:0X01——注入谐波模式; 0X02——治理谐波模式; 0x00——无附加模式二;。 */ //端口附加模式二:0X01——注入谐波模式; 0X02——治理谐波模式; 0x00——无附加模式二;
        ushort P_S1_Mode : 2;
        /* P_S1_Mode：端口附加模式一:0X01——STATCOM无功模式;0X02——STATCOM功率因数模式;0X03——STATCOM电压模式;0x00——无附加模式一。 */ //端口附加模式一:0X01——STATCOM无功模式;0X02——STATCOM功率因数模式;0X03——STATCOM电压模式;0x00——无附加模式一
        ushort P_M_Mode : 2;
        /* P_M_Mode：端口主要工作模式:0X00——恒压模式；0X01——恒功率模式；0X02——下垂模式；0X03——跟随模式。 */ //端口主要工作模式:0X00——恒压模式；0X01——恒功率模式；0X02——下垂模式；0X03——跟随模式
        ushort Label : 2; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */ //标识，值为0x0
    } Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */

    ushort MainStatus16; /* MainStatus16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_MainStatus;

typedef union _XXG_PORT_STATUS
{
    struct _PORT_STATUS
    {
        ushort Contactor_Inner : 1;
        /* Contactor_Inner：内部接触器，0表示断；1表示合；。 */ //内部接触器，0表示断；1表示合；
        ushort Breaker_InnerOn : 1;
        /* Breaker_InnerOn：内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */ //内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
        ushort Breaker_InnerOff : 1;
        /* Breaker_InnerOff：内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */ //内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
        ushort Breaker_OuterOn : 1;
        /* Breaker_OuterOn：外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；。 */ //外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；
        ushort Breaker_OuterOff : 1;
        /* Breaker_OuterOff：外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；。 */ //外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；
        ushort GND_Switch : 1;
        /* GND_Switch：接地开关；HVAC端口：1表示合，0表示分。 */ //接地开关；HVAC端口：1表示合，0表示分
        ushort ISO_Switch : 1;
        /* ISO_Switch：隔离开关；HVAC端口：1表示合，0表示分。 */ //隔离开关；HVAC端口：1表示合，0表示分
        ushort Module_Hbridge : 1;
        /* Module_Hbridge：1:模块H桥闭锁，0：模块H桥解锁；。 */ //1:模块H桥闭锁，0：模块H桥解锁；
        ushort LF_Hbridge : 1; /* LF_Hbridge：1:工频H桥闭锁，0：工频H桥解锁；。 */ //1:工频H桥闭锁，0：工频H桥解锁；
        ushort HF_Hbridge : 1; /* HF_Hbridge：1:高频H桥闭锁，0：高频H桥解锁；。 */ //1:高频H桥闭锁，0：高频H桥解锁；
        ushort SWITCH_ON_EN : 1;
        /* SWITCH_ON_EN：1:端口外部断路器闭合使能，0：禁止；。 */ //1:端口外部断路器闭合使能，0：禁止；
        ushort Reservedbit2 : 3; /* Reservedbit2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
        ushort S_Mode : 2;
        /* S_Mode：系统运行方式：0x00表示自动运行，0x01表示自动测试，0x02表示手动运行，0x03表示手动测试；。 */ //系统运行方式：0x00表示自动运行，0x01表示自动测试，0x02表示手动运行，0x03表示手动测试；
    } Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */

    ushort PORT_STATUS16; /* PORT_STATUS16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_STATUS;

typedef union _XXG_PORT_FAULT
{
    struct _PORT_FAULT
    {
        ushort PORT_FAULT_bit0 : 1; /* PORT_FAULT_bit0：1:表示接触器异常。 */ //1:表示接触器异常
        ushort PORT_FAULT_bit1 : 1; /* PORT_FAULT_bit1：1:表示断路器异常。 */ //1:表示断路器异常
        ushort PORT_FAULT_bit2 : 1; /* PORT_FAULT_bit2：1:表示风机故障。 */ //1:表示风机故障
        ushort PORT_FAULT_bit3 : 1; /* PORT_FAULT_bit3：1:表示控制器故障。 */ //1:表示控制器故障
        ushort PORT_FAULT_bit4 : 1; /* PORT_FAULT_bit4：1:表示辅助供电。 */ //1:表示辅助供电
        ushort PORT_FAULT_bit5 : 1; /* PORT_FAULT_bit5：1:表示EPO急停。 */ //1:表示EPO急停
        ushort PORT_FAULT_bit6 : 1; /* PORT_FAULT_bit6：1:表示模块故障。 */ //1:表示模块故障
        ushort PORT_FAULT_bit7 : 1; /* PORT_FAULT_bit7：1:表示逆变器故障。 */ //1:表示逆变器故障
        ushort PORT_FAULT_bit8 : 1; /* PORT_FAULT_bit8：1:表示相序错误。 */ //1:表示相序错误
        ushort PORT_FAULT_bit9 : 1; /* PORT_FAULT_bit9：1:表示频率越限。 */ //1:表示频率越限
        ushort PORT_FAULT_bit10 : 1; /* PORT_FAULT_bit10：1:表示过流。 */ //1:表示过流
        ushort PORT_FAULT_bit11 : 1; /* PORT_FAULT_bit11：1:表示欠压。 */ //1:表示欠压
        ushort PORT_FAULT_bit12 : 1; /* PORT_FAULT_bit12：1:表示过压。 */ //1:表示过压
        ushort PORT_FAULT_bit13 : 1; /* PORT_FAULT_bit13：1:表示通讯故障。 */ //1:表示通讯故障
        ushort PORT_FAULT_bit14 : 1; /* PORT_FAULT_bit14：1:表示SYNC异常。 */ //1:表示SYNC异常

        ushort PORT_FAULT_bit15 : 1;
        /* PORT_FAULT_bit15：1:表示端口有故障；0：表示端口无故障；。 */ //1:表示端口有故障；0：表示端口无故障；
    } Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */

    ushort PORT_FAULT16; /* PORT_FAULT16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PORT_FAULT;

#endif /* SST_WIRE_TYPES_H */
