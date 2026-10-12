/* 文件职责: FPGA 固定映射与硬件控制字缓存。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef FPGAREGISTER_H_
#define FPGAREGISTER_H_
#include "SstTypes.h"
#include "SstWireTypes.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef union _XXG_ID				
{
	struct _ID
	{
		ushort Order:   4; /* Order：位置：0x0~0xF，分别对应着机箱中插件的2~17号位。 */  //位置：0x0~0xF，分别对应着机箱中插件的2~17号位
		ushort Type:    4; /* Type：类型：0x0，主控插件；0x1，光纤通讯插件；0x2，光纤输入插件；0x3，光纤输出插件；0x4，开入开出插件；0x5，AD采样插件；0x6，逆变PWM脉冲插件；0x7，60044-8插件；。 */  //类型：0x0，主控插件；0x1，光纤通讯插件；0x2，光纤输入插件；0x3，光纤输出插件；0x4，开入开出插件；0x5，AD采样插件；0x6，逆变PWM脉冲插件；0x7，60044-8插件；
		ushort Port:    4; /* Port：端口：0x0，高压交流端口HVAC；0x1，高压直流端口HVDC；0x2，低压直流端口LVDC；0x3，低压交流端口LVAC；。 */  //端口：0x0，高压交流端口HVAC；0x1，高压直流端口HVDC；0x2，低压直流端口LVDC；0x3，低压交流端口LVAC；
		ushort Label:   4; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */  //插件标识，读数！=0x6表示此地址没有相应插件
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort ID16; /* ID16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_ID;
typedef union _XXG_LED				
{
	struct _LED
	{
		ushort led0:           1; /* led0：第 0 路插件 LED 控制位，1 点亮、0 熄灭。 */  //当取1时，表示此插件上第x个LED指示灯点亮；
		ushort led1:           1; /* led1：第 1 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led2:           1; /* led2：第 2 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led3:           1; /* led3：第 3 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led4:           1; /* led4：第 4 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led5:           1; /* led5：第 5 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led6:           1; /* led6：第 6 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort led7:           1; /* led7：第 7 路插件 LED 控制位，1 点亮、0 熄灭。 */
		ushort Reservedbit:    8; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort LED16; /* LED16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_LED;
typedef union _XXG_DC_Log
{
	struct _DC_Log
	{
		ushort DC_Order:     	1; /* DC_Order：直流平均电压排序：0——高，1——低。 */  //直流平均电压排序：0——高，1——低
		ushort Reservedbit1:  7; /* Reservedbit1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
		ushort IG_Orient:    	1; /* IG_Orient：电流方向：1:正向 0:负向。 */  //电流方向：1:正向 0:负向
		ushort Reservedbit2:  7; /* Reservedbit2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */ 
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort DC_Log16; /* DC_Log16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_DC_Log;
typedef union _XXG_PM_CTL
{
	struct _PM_CTL
	{
		ushort SVG_EN:     1; /* SVG_EN：当取1时，表示级联H桥PWM输出使能，取0时表示PWM输出禁止；。 */  //当取1时，表示级联H桥PWM输出使能，取0时表示PWM输出禁止；
		ushort SVG_DIV:    2; /* SVG_DIV：级联H桥开关频率=同步频率*2的SVG_DIV次幂。 */ // 级联H桥开关频率=同步频率*2的SVG_DIV次幂
		ushort SVG_RST:    1; /* SVG_RST：当取1时，级联H桥驱动板RESET；当取0时，无操作；。 */ // 当取1时，级联H桥驱动板RESET；当取0时，无操作；
		ushort DAB_EN:     1; /* DAB_EN：1:DAB H桥PWM输出使能;0:DAB H桥PWM输出禁止;。 */  //1:DAB H桥PWM输出使能;0:DAB H桥PWM输出禁止;
		ushort DAB_DIV:    2; /* DAB_DIV：DAB H桥开关频率=同步频率*2的DAB_DIV次幂。 */ //DAB H桥开关频率=同步频率*2的DAB_DIV次幂
		ushort DAB_RST:    1; /* DAB_RST：1:DAB H桥驱动板RESET;0:无操作；。 */ //1:DAB H桥驱动板RESET;0:无操作；
		ushort HW_RST:     1; /* HW_RST：1:模块控制板故障锁存清除;0:无操作；。 */  //1:模块控制板故障锁存清除;0:无操作；
		ushort FAN_OFF:    1; /* FAN_OFF：1：关闭模块风扇。 */  //1：关闭模块风扇
		ushort SVG_SHUT:   1; /* SVG_SHUT：1:SHUTDOWN光纤熄灭时，级联H桥闭锁。 */  //1:SHUTDOWN光纤熄灭时，级联H桥闭锁
		ushort DAB_SHUT:   1; /* DAB_SHUT：1:SHUTDOWN光纤熄灭时，DAB H桥闭锁。 */  //1:SHUTDOWN光纤熄灭时，DAB H桥闭锁
		ushort ONE:        1; /* ONE：1:单链测试模式，级联H桥有T1和T2两个交点;0:工作模式，级联H桥在TAD内只有一个交点;。 *///1:单链测试模式，级联H桥有T1和T2两个交点;0:工作模式，级联H桥在TAD内只有一个交点;
		ushort CTL:        1; /* CTL：值为0，串行通讯指示，指令为PM_CTL；。 *///值为0，串行通讯指示，指令为PM_CTL；
		ushort LABLE:      2; /* LABLE：值为00，串行通讯指示，控制指令；。 *///值为00，串行通讯指示，控制指令；
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort PM_CTL16; /* PM_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_PM_CTL;
typedef union _XXG_SwitchINOUT				
{
	struct _SwitchINOUT
	{                                 //四个端口的开关配置情况：四个端口均有1个接触器；HVAC端口有1个内部断路器和1个外部断路器；HVDC端口有1个内部断路器和2个故障电流控制器；LVDC端口有2个故障电流控制器；LVAC端口有1个外部断路器；
		ushort Breaker_OuterOff:  1; /* Breaker_OuterOff：外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；。 */  //外部断路器关；HVAC端口：0表示外部断路器合，1表示外部断路器分；HVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVDC端口：1表示负极故障电流控制器合，0表示负极故障电流控制器分；LVAC端口：0表示外部断路器合，1表示外部断路器分；
		ushort Breaker_OuterOn:   1; /* Breaker_OuterOn：外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；。 */  //外部断路器合；HVAC端口：1表示外部断路器合，0表示外部断路器分；HVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVDC端口：1表示正极故障电流控制器合，0表示正极故障电流控制器分；LVAC端口：1表示外部断路器合，0表示外部断路器分；
		ushort Breaker2_OuterOff: 1; /* Breaker2_OuterOff：外部断路器2。 */	//外部断路器2
		ushort Breaker2_OuterOn:   1; /* Breaker2_OuterOn：原第二外部断路器/故障电流控制器合闸控制位。 */  
		ushort Breaker_InnerOff:  1; /* Breaker_InnerOff：内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */  //内部断路器关；HVAC端口：0表示内部断路器合，1表示内部断路器分；HVDC端口：0表示内部断路器合，1表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
		ushort Breaker_InnerOn:   1; /* Breaker_InnerOn：内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；。 */  //内部断路器合；HVAC端口：1表示内部断路器合，0表示内部断路器分；HVDC端口：1表示内部断路器合，0表示内部断路器分；LVDC端口无内部断路器；LVAC端口：无内部断路器；
		
		ushort Reservedbit:      1; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
		ushort Contactor_Inner:   1; /* Contactor_Inner：内部接触器，0表示断；1表示合；。 */  //内部接触器，0表示断；1表示合；

		ushort Invalidbit:        8; /* Invalidbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort SwitchINOUT16; /* SwitchINOUT16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_SwitchINOUT;
typedef union _XXG_INV_CTL
{
	struct _INV_CTL
	{
		ushort TAD_DIV:           3; /* TAD_DIV：逆变模块开关周期=同步周期50us*(TAD_DIV+1)。 */	  //逆变模块开关周期=同步周期50us*(TAD_DIV+1)
		ushort DEADZONE:          10; /* DEADZONE：逆变模块死区时间，0~10.23us。 */	  //逆变模块死区时间，0~10.23us
		ushort Invalidbit:        3; /* Invalidbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort INV_CTL16; /* INV_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_INV_CTL;
typedef union _XXG_BUCK_CTL
{
	struct _BUCK_CTL
	{
		ushort TAD_DIV:           3; /* TAD_DIV：同步周期50us*(TAD_DIV+1)。 */	  //同步周期50us*(TAD_DIV+1)
		ushort Reservedbit:       12; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	  //逆变模块死区时间，0~10.23us
		ushort BUCK_EN:           1; /* BUCK_EN：BUCK PWM 使能位，沿用原 FPGA 有效电平。 */	  //BUCK使能
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort BUCK_CTL16; /* BUCK_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_BUCK_CTL;
typedef union _XXG_DSP_CTL
{
	struct _DSP_CTL
	{	
		ushort SHUTDOWN:           1; /* SHUTDOWN：模块硬件关断位图，保持原 FPGA/DPRAM 字布局。 */	  //1:光纤SHUTDOWN信号，脉冲闭锁
		ushort HW_RST:             1; /* HW_RST：1:RESET主控插件上的FB信号；。 */	  //1:RESET主控插件上的FB信号；
		ushort Invalidbit:        14; /* Invalidbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */	
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort DSP_CTL16; /* DSP_CTL16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_DSP_CTL;
typedef union _XXG_STATUS
{
	struct _STATUS
	{
		ushort OV:    		1; /* OV：模块过压；。 */  //模块过压；
		ushort OC2:  			1; /* OC2：HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流。 */  //HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流
		ushort OC1:   		1; /* OC1：HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流；。 */  //HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流；
		ushort OT:  	    1; /* OT：模块过温。 */  //模块过温
		ushort SVG_H:  		1; /* SVG_H：工频H桥/Buck桥的驱动板故障。 */  //工频H桥/Buck桥的驱动板故障
		ushort DAB_H:  		1; /* DAB_H：DAB H桥驱动板故障。 */  //DAB H桥驱动板故障
		ushort UV:   			1; /* UV：模块欠压；。 */  //模块欠压；
		ushort SHUT:   		1; /* SHUT：SHUTDOWN光纤闭锁；。 */  //SHUTDOWN光纤闭锁；
		
		ushort OV_L:    	1; /* OV_L：模块过压锁存；。 */  //模块过压锁存；
		ushort OC2_L:			1; /* OC2_L：HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流锁存。 */  //HVAC模块工频电流/HVDC模块直流电流/LVDC模块负极电流/LVAC模块直流电流——过流锁存
		ushort OC1_L:			1; /* OC1_L：HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流锁存。 */  //HVAC模块直流电流/HVDC模块备用/LVDC模块正极电流/LVAC模块备用——过流锁存
		ushort OT_L:  		1; /* OT_L：模块过温锁存。 */  //模块过温锁存
		ushort SVG_L:  		1; /* SVG_L：工频H桥/Buck桥的驱动板故障锁存。 */  //工频H桥/Buck桥的驱动板故障锁存
		ushort DAB_L:  		1; /* DAB_L：DAB H桥驱动板故障锁存。 */  //DAB H桥驱动板故障锁存

		ushort Label:   	2; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */	//标识
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort STATUS16; /* STATUS16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_STATUS;
typedef union _XXG_DRIVER
{
	struct _DRIVER
	{
		ushort Driver1_Status:  3; /* Driver1_Status：模块第 1 路驱动状态位。 */  //第一个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Driver2_Status:  3; /* Driver2_Status：模块第 2 路驱动状态位。 */  //第二个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Driver3_Status:  3; /* Driver3_Status：模块第 3 路驱动状态位。 */  //第三个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Driver4_Status:  3; /* Driver4_Status：模块第 4 路驱动状态位。 */  //第四个驱动板，111：无故障；010：上管故障；100：下管故障；110：驱动板过压故障、过温故障、直流母线电压故障；000：上电配置故障；
		ushort Reservedbit:  	2; /* Reservedbit：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */  //备用
		ushort Label:   	    2; /* Label：原状态/驱动字中的标识字段，保持 FPGA 返回格式。 */	//标识
	} Bbits; /* Bbits：与整字重叠的位域视图，字段位置对应原硬件/协议布局。 */
	ushort DRIVER16; /* DRIVER16：本联合体的完整 16 位字视图，与位域成员共用存储。 */
} XXG_DRIVER;
typedef struct _FPGA_InterfaceBLOCK {
  	XXG_ID ID; /* ID：插件身份、类型、端口及标识寄存器，保持原 FPGA/DPRAM 字布局。 */		            //15:8——0x68,否则表示无插件;7:4——表示插件类型（0：主控；1：光纤通讯；2：光纤输入；3：光纤输出；4：开入开出；5：采样；6：逆变PWM）；3:0——表示插件在机箱中的位置（0~F：对应2~17号位置）
  	XXG_LED LED; /* LED：插件八路 LED 控制寄存器，保持原 FPGA/DPRAM 字布局。 */              //插件上的LED，0~7位有效。主控插件上：bit0——LEDR0  POWER；bit1——LEDB0 RUN；bit2——LEDR1 FAULT；bit3——LEDB1 ALARM
  	ushort NUM; /* NUM：原调制算法在线模块数量寄存器，保持原 FPGA/DPRAM 字布局。 */								//每相总的功率模块链节数量
  	ushort PM; /* PM：本插件在线模块位图，保持原 FPGA/DPRAM 字布局。 */								//光纤插件上连接的功率模块  7:0有效，bitX=1：插件上第X对TX/RX光纤与功率模块连接
  	ushort Reference; /* Reference：工频桥原始调制参考计数，保持原 FPGA/DPRAM 字布局。 */					//参考电压
  	ushort Orient; /* Orient：模块调制/电流方向字段，保持原 FPGA/DPRAM 字布局。 */						
  	XXG_DC_Log DC_Log; /* DC_Log：模块直流电压排序及电流方向控制字，保持原 FPGA/DPRAM 字布局。 */						
  	ushort TAD; /* TAD：FPGA 控制周期计数，保持原 FPGA/DPRAM 字布局。 */								//AD采样周期(us)   控制周期
  	ushort Reserved_1[2]; /* Reserved_1：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
  	ushort DAB_LEFTDuty; /* DAB_LEFTDuty：DAB 左桥占空比计数，保持原 FPGA/DPRAM 字布局。 */      //DAB脉冲占空比；
  	ushort DAB_RIGHTDuty; /* DAB_RIGHTDuty：DAB 右桥占空比计数，保持原 FPGA/DPRAM 字布局。 */      //DAB脉冲占空比；
  	ushort DAB_Start; /* DAB_Start：DAB 脉冲起始计数，保持原 FPGA/DPRAM 字布局。 */         // DAB脉冲起始时刻；
    XXG_PM_CTL PM_CTL; /* PM_CTL：工频桥/DAB 使能、分频、复位及测试控制字，保持原 FPGA/DPRAM 字布局。 */                  // 功率模块H桥设置；
  	ushort TX_EN; /* TX_EN：插件发送模块使能位图，保持原 FPGA/DPRAM 字布局。 */  						//串口发送使能   7:0有效，bitX=1：插件上第X号TX串口输出使能
  	ushort TX_CTL; /* TX_CTL：插件发送控制寄存器，保持原 FPGA/DPRAM 字布局。 */						//串口发送控制   7:0有效，bitX=1：插件上第X号TX串口发送数据START,自动清零
  	ushort TX[8]; /* TX：插件原始发送数据区，保持原 FPGA/DPRAM 字布局。 */					    //光纤插件串口发送数据
  	ushort RX[8]; /* RX：插件原始接收数据区，保持原 FPGA/DPRAM 字布局。 */                       //光纤插件串口接收数据；
  	XXG_SwitchINOUT DOUT; /* DOUT：数字输出控制字，保持原 FPGA/DPRAM 字布局。 */  		//开入开出插件开关量输出  7:0有效，bitX=1：插件上第X通道输出继电器闭合
  	ushort AD_PROT_EN; /* AD_PROT_EN：端口 AD 保护使能位图，保持原 FPGA/DPRAM 字布局。 */        //AD插件通道保护使能, bitX=1：AD采样插件上第X通道硬件保护使能
  	ushort SHUTDOWN; /* SHUTDOWN：模块硬件关断位图，保持原 FPGA/DPRAM 字布局。 */          //光纤输出插件SHUTDOWN输出, bitX=1：光纤输出插件上第X通道光纤熄灭
  	ushort FO_EN; /* FO_EN：光纤故障输入使能位图，保持原 FPGA/DPRAM 字布局。 */			  //光纤输入插件FO使能, bitX=1：光纤输入插件上第X通道光纤FO信号使能
  	ushort INV_SHUTDOWN; /* INV_SHUTDOWN：独立逆变桥关断位图，保持原 FPGA/DPRAM 字布局。 */       //逆变单元插件PWM闭锁；
  	ushort INV_FO_EN; /* INV_FO_EN：独立逆变桥光纤故障使能位图，保持原 FPGA/DPRAM 字布局。 */          //逆变单元插件FO使能；
  	ushort INV_REF; /* INV_REF：独立逆变桥原始调制参考计数，保持原 FPGA/DPRAM 字布局。 */            //逆变ABCN插件PWM参考值；
  	XXG_INV_CTL INV_CTL; /* INV_CTL：独立逆变桥周期/死区控制字，保持原 FPGA/DPRAM 字布局。 */			//逆变控制
  	ushort BUCK_DUTY; /* BUCK_DUTY：BUCK 占空比计数，保持原 FPGA/DPRAM 字布局。 */         //BUCK桥占空比
  	XXG_BUCK_CTL BUCK_CTL; /* BUCK_CTL：BUCK 使能及周期控制字，保持原 FPGA/DPRAM 字布局。 */    //BUCK桥控制
  	ushort Reserved_2[6]; /* Reserved_2：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */    //预留
  	ushort DA_DATA0; /* DA_DATA0：第 0 路 DA 调试输出原始计数寄存器。 */         	//DA0
  	ushort DA_DATA1; /* DA_DATA1：第 1 路 DA 调试输出原始计数寄存器。 */         	//DA1
  	ushort DA_DATA2; /* DA_DATA2：第 2 路 DA 调试输出原始计数寄存器。 */         	//DA2
  	ushort DA_DATA3; /* DA_DATA3：第 3 路 DA 调试输出原始计数寄存器。 */         	//DA3
  	ushort Reserved_3[4]; /* Reserved_3：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */    //预留
  	ushort BKU; /* BKU：BKU 故障锁存控制寄存器，保持原 FPGA/DPRAM 字布局。 */               //主控插件至各插件输出BK信号，bit0~15对应2~17号插件
  	ushort FB_EN; /* FB_EN：FB 硬件故障输入使能位图，保持原 FPGA/DPRAM 字布局。 */  	          //主控插件接收各插件FB信号使能,bit0~15对应2~17号插件
  	ushort TEST3A; /* TEST3A：原 FPGA 测试寄存器 A，保持原 FPGA/DPRAM 字布局。 */            //读写测试单元；
  	ushort TEST3B; /* TEST3B：原 FPGA 测试寄存器 B，保持原 FPGA/DPRAM 字布局。 */            //读写测试单元；
  	ushort Debug_L; /* Debug_L：左桥 FPGA 调制调试寄存器，保持原 FPGA/DPRAM 字布局。 */           //调试输出左桥臂的脉冲和；
  	ushort Debug_R; /* Debug_R：右桥 FPGA 调制调试寄存器，保持原 FPGA/DPRAM 字布局。 */           //调试输出右桥臂的脉冲和；
  	ushort Reserved_4[1]; /* Reserved_4：原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量。 */
  	XXG_DSP_CTL DSP_CTL; /* DSP_CTL：DSP 全关断及硬件复位控制字，保持原 FPGA/DPRAM 字布局。 */  	  //DSP控制字
   	XXG_MainStatus HVAC_STATUS; /* HVAC_STATUS：主控插件HVAC端口运行状态。 */       //主控插件HVAC端口运行状态
  	short HVAC_DV; /* HVAC_DV：主控插件HVAC端口数据1。 */            //主控插件HVAC端口数据1
  	short HVAC_DI; /* HVAC_DI：主控插件HVAC端口数据2。 */						//主控插件HVAC端口数据2
  	short HVAC_DP; /* HVAC_DP：主控插件HVAC端口数据3。 */						//主控插件HVAC端口数据3
  	short HVAC_DV1; /* HVAC_DV1：主控插件HVAC端口数据1。 */           //主控插件HVAC端口数据1
  	short HVAC_DI1; /* HVAC_DI1：主控插件HVAC端口数据2。 */						//主控插件HVAC端口数据2
  	short HVAC_DP1; /* HVAC_DP1：主控插件HVAC端口数据3。 */						//主控插件HVAC端口数据3
  	short HVAC_DV2; /* HVAC_DV2：主控插件HVAC端口数据1。 */           //主控插件HVAC端口数据1
  	XXG_MainStatus HVDC_STATUS; /* HVDC_STATUS：主控插件HVDC端口运行状态。 */		  //主控插件HVDC端口运行状态
  	short HVDC_DV; /* HVDC_DV：主控插件HVDC端口数据1。 */						//主控插件HVDC端口数据1
  	short HVDC_DI; /* HVDC_DI：主控插件HVDC端口数据2。 */						//主控插件HVDC端口数据2
  	short HVDC_DP; /* HVDC_DP：主控插件HVDC端口数据3。 */						//主控插件HVDC端口数据3
  	short HVDC_DV1; /* HVDC_DV1：主控插件HVDC端口数据1。 */						//主控插件HVDC端口数据1
  	short HVDC_DI1; /* HVDC_DI1：主控插件HVDC端口数据2。 */						//主控插件HVDC端口数据2
  	short HVDC_DP1; /* HVDC_DP1：主控插件HVDC端口数据3。 */						//主控插件HVDC端口数据3
  	short HVDC_DV2; /* HVDC_DV2：主控插件HVDC端口数据1。 */						//主控插件HVDC端口数据1
  	XXG_MainStatus LVDC_STATUS; /* LVDC_STATUS：主控插件LVDC端口运行状态。 */		  //主控插件LVDC端口运行状态
  	short LVDC_DV; /* LVDC_DV：主控插件LVDC端口数据1。 */						//主控插件LVDC端口数据1
  	short LVDC_DI; /* LVDC_DI：主控插件LVDC端口数据2。 */						//主控插件LVDC端口数据2
  	short LVDC_DP; /* LVDC_DP：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVDC_DV1; /* LVDC_DV1：主控插件LVDC端口数据1。 */						//主控插件LVDC端口数据1
  	short LVDC_DI1; /* LVDC_DI1：主控插件LVDC端口数据2。 */						//主控插件LVDC端口数据2
  	short LVDC_DP1; /* LVDC_DP1：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVDC_DV2; /* LVDC_DV2：主控插件LVDC端口数据1。 */						//主控插件LVDC端口数据1
  	XXG_MainStatus LVAC_STATUS; /* LVAC_STATUS：主控插件LVAC端口运行状态。 */		  //主控插件LVAC端口运行状态
  	short LVAC_DV; /* LVAC_DV：主控插件LVAC端口数据1。 */						//主控插件LVAC端口数据1
  	short LVAC_DI; /* LVAC_DI：主控插件LVAC端口数据2。 */						//主控插件LVAC端口数据2
  	short LVAC_DP; /* LVAC_DP：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVAC_DV1; /* LVAC_DV1：主控插件LVAC端口数据1。 */						//主控插件LVAC端口数据1
  	short LVAC_DI1; /* LVAC_DI1：主控插件LVAC端口数据2。 */						//主控插件LVAC端口数据2
  	short LVAC_DP1; /* LVAC_DP1：主控插件LVDC端口数据3。 */						//主控插件LVDC端口数据3
  	short LVAC_DV2; /* LVAC_DV2：主控插件LVAC端口数据1。 */						//主控插件LVAC端口数据1
  	short AD[16]; /* AD：端口原始 AD 实时采样数组，保持原 FPGA/DPRAM 字布局。 */            //AD插件AD各通道数据
  	short AD_LATCH[16]; /* AD_LATCH：端口原始 AD 锁存采样数组，供故障/同步采样使用。 */      //故障锁存AD插件各通道数据——插件接收到BKU信号时，锁存的采样数据；
  	
  	
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

} FPGA_InterfaceBLOCK, *P_FPGA_InterfaceBLOCK;//FPGA参数设置区

extern volatile FPGA_InterfaceBLOCK FPGA_Interface[15]; /* FPGA_Interface：15 个插件的 EMIFA 映射寄存器窗口，索引为原插件编号减 2。 */

extern XXG_DSP_CTL Port_DSP_CTL; /* Port_DSP_CTL：端口 FPGA 全关断及硬件复位控制字缓存。 */

extern XXG_SwitchINOUT Port_DOUT; /* Port_DOUT：端口断路器、接触器数字输出控制字缓存。 */

extern XXG_PM_CTL Module_PM_CTL; /* Module_PM_CTL：模块工频桥/DAB 使能、驱动复位及测试控制字缓存。 */

extern ushort WrongTimes_LEFT, WrongTimes_RIGHT,WrongTimes_LEFT_A, WrongTimes_RIGHT_A; /* WrongTimes_LEFT：原 FPGA LEFT 参考/通信核对的累计不一致次数。 */

/* WrongTimes_RIGHT：原 FPGA RIGHT 参考/通信核对的累计不一致次数。 */

/* WrongTimes_LEFT_A：原 FPGA LEFT_A 参考/通信核对的累计不一致次数。 */

/* WrongTimes_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的累计不一致次数。 */

extern ushort LastWrong_LEFT, LastWrong_RIGHT,LastWrong_LEFT_A, LastWrong_RIGHT_A; /* LastWrong_LEFT：原 FPGA LEFT 参考/通信核对的上一轮不一致值。 */

/* LastWrong_RIGHT：原 FPGA RIGHT 参考/通信核对的上一轮不一致值。 */

/* LastWrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的上一轮不一致值。 */

/* LastWrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的上一轮不一致值。 */

extern ushort Wrong_LEFT, Wrong_RIGHT,Wrong_LEFT_A, Wrong_RIGHT_A; /* Wrong_LEFT：原 FPGA LEFT 参考/通信核对的本轮不一致值。 */

/* Wrong_RIGHT：原 FPGA RIGHT 参考/通信核对的本轮不一致值。 */

/* Wrong_LEFT_A：原 FPGA LEFT_A 参考/通信核对的本轮不一致值。 */

/* Wrong_RIGHT_A：原 FPGA RIGHT_A 参考/通信核对的本轮不一致值。 */

extern ushort WrongTimes_Reference_A1,WrongTimes_Reference_A2,WrongTimes_Reference_C1,WrongTimes_Reference_C2; /* WrongTimes_Reference_A1：原 FPGA Reference_A1 参考/通信核对的累计不一致次数。 */

/* WrongTimes_Reference_A2：原 FPGA Reference_A2 参考/通信核对的累计不一致次数。 */

/* WrongTimes_Reference_C1：原 FPGA Reference_C1 参考/通信核对的累计不一致次数。 */

/* WrongTimes_Reference_C2：原 FPGA Reference_C2 参考/通信核对的累计不一致次数。 */

extern ushort LastWrong_Reference_A1,LastWrong_Reference_A2,LastWrong_Reference_C1,LastWrong_Reference_C2; /* LastWrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的上一轮不一致值。 */

/* LastWrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的上一轮不一致值。 */

/* LastWrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的上一轮不一致值。 */

/* LastWrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的上一轮不一致值。 */

extern ushort Wrong_Reference_A1,Wrong_Reference_A2,Wrong_Reference_C1,Wrong_Reference_C2; /* Wrong_Reference_A1：原 FPGA Reference_A1 参考/通信核对的本轮不一致值。 */

/* Wrong_Reference_A2：原 FPGA Reference_A2 参考/通信核对的本轮不一致值。 */

/* Wrong_Reference_C1：原 FPGA Reference_C1 参考/通信核对的本轮不一致值。 */

/* Wrong_Reference_C2：原 FPGA Reference_C2 参考/通信核对的本轮不一致值。 */

extern ushort WrongTimes_ControlReference_A,WrongTimes_ControlReference_C; /* WrongTimes_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的累计不一致次数。 */

/* WrongTimes_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的累计不一致次数。 */

extern ushort LastWrong_ControlReference_A,LastWrong_ControlReference_C; /* LastWrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的上一轮不一致值。 */

/* LastWrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的上一轮不一致值。 */

extern ushort Wrong_ControlReference_A,Wrong_ControlReference_C; /* Wrong_ControlReference_A：原 FPGA ControlReference_A 参考/通信核对的本轮不一致值。 */

/* Wrong_ControlReference_C：原 FPGA ControlReference_C 参考/通信核对的本轮不一致值。 */

extern float DAC_BUF[4]; /* DAC_BUF：四路 DA 调试输出缓存，写出时转为 FPGA 原始计数。 */


#ifdef __cplusplus
}
#endif
#endif
