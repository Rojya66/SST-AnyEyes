//###########################################################################
//
// FILE:   ADC_Sample.h
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

/**************************Macro definition*********************************/
//port
/* HA_PT_Ratio：原 HA_PT 采样量的传感器/工程量换算比例。 */
#define HA_PT_Ratio 100.0   //10k:100
/* HA_CT_Ratio：原 HA_CT 采样量的传感器/工程量换算比例。 */
#define HA_CT_Ratio 20.0    //100:5


/* HA_GridCurrent_Ratio：原 HA_GridCurrent 采样量的传感器/工程量换算比例。 */
#define HA_GridCurrent_Ratio 0.646*HA_CT_Ratio
/* HA_Current_Ratio：原 HA_Current 采样量的传感器/工程量换算比例。 */
#define HA_Current_Ratio 40.0 //外接霍尔

//#define HD_Voltage_Ratio 
/* HD_Current_Ratio：原 HD_Current 采样量的传感器/工程量换算比例。 */
#define HD_Current_Ratio 100.0

/* LD_Volt_Ratio：原 LD_Volt 采样量的传感器/工程量换算比例。 */
#define LD_Volt_Ratio 103.39
/* LD_Current_Ratio：原 LD_Current 采样量的传感器/工程量换算比例。 */
#define LD_Current_Ratio 401.606


/* LA_PT_Ratio：原 LA_PT 采样量的传感器/工程量换算比例。 */
#define LA_PT_Ratio 100.0   //10k:100
/* LA_CT_Ratio：原 LA_CT 采样量的传感器/工程量换算比例。 */
#define LA_CT_Ratio 20.0    //100:5
/* LA_Volt_Ratio：原 LA_Volt 采样量的传感器/工程量换算比例。 */
#define LA_Volt_Ratio 19.386*LA_PT_Ratio
/* LA_Current_Ratio：原 LA_Current 采样量的传感器/工程量换算比例。 */
#define LA_Current_Ratio 0.646*LA_CT_Ratio
/* LA_DCVolt_Ratio：原 LA_DCVolt 采样量的传感器/工程量换算比例。 */
#define LA_DCVolt_Ratio 129.34
/* LA_InvCapVolt_Ratio：原 LA_InvCapVolt 采样量的传感器/工程量换算比例。 */
#define LA_InvCapVolt_Ratio 129.34
/* LA_InvCapCurrent_Ratio：原 LA_InvCapCurrent 采样量的传感器/工程量换算比例。 */
#define LA_InvCapCurrent_Ratio 20.0
/* LA_InvLgCurrent_Ratio：原 LA_InvLgCurrent 采样量的传感器/工程量换算比例。 */
#define LA_InvLgCurrent_Ratio 401.606

//module
/* Module_Volt_Ratio：原 Module_Volt 采样量的传感器/工程量换算比例。 */
#define Module_Volt_Ratio 103.226
/* Module_Current_Ratio：原 Module_Current 采样量的传感器/工程量换算比例。 */
#define Module_Current_Ratio 20.0
/* Module_DCbusCurrent_Ratio：原 Module_DCbusCurrent 采样量的传感器/工程量换算比例。 */
#define Module_DCbusCurrent_Ratio 20.0
//#define Module_Temp_Ratio 129.34

//LPF
//#define Sample_UP_20kHz    0.005168         
//#define Sample_DOWN_20kHz    0.9948	
//#define Sample_UP_20kHz    0.003135         
//#define Sample_DOWN_20kHz    0.9969

/* Sample_UP_20kHz：原 20 kHz 分支采样上沿判定常量，保留兼容定义。 */
#define Sample_UP_20kHz    0.0006278        
/* Sample_DOWN_20kHz：原 20 kHz 分支采样下沿判定常量，保留兼容定义。 */
#define Sample_DOWN_20kHz    0.9994

//Ust

/* Ust_LVAC_Port：旧 LVAC 运行电压参考，单位 V。 */
#define Ust_LVAC_Port  100.0

/* SigUst_LVAC_Port：旧 LVAC 启动电压判据，单位 V。 */
#define SigUst_LVAC_Port  600.0

/* UstOK_LVAC_Port：旧 LVAC 电压达标参考，单位 V。 */
#define UstOK_LVAC_Port  700.0



/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 读取指定模块的 FPGA AD 数据并按所选量转换为工程值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           MK_index：插件内模块编号，从 1 开始，模块数组下标为编号减 1
 *           CS_index：ModuleIndex_* 采样类型编号
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 直流电压 V、电流 A、温度按原计数/10；非法插件、模块或类型返回 65536.0
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
extern float Module_AD_Sample(uchar CJ_index,uchar MK_index,uchar CS_index,uchar latch);

/*==================================================================
 * 函数功能: 采集当前端口 FPGA 的瞬时电压、电流及直流量，并推进原滤波/噪声处理
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
extern void Port_AD_Sample(uchar latch);
/*==================================================================
 * 函数功能: 换算当前端口锁存/滤波采样，更新有效值、功率及公共遥测
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
extern void Port_RealTimeParameter(void);
/*==================================================================
 * 函数功能: 读取模块直流采样汇总并更新各相总电压量
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
extern void Module_TotalUdc_Sample(void);

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 换算 HVAC 插件采样数据并更新端口实时测量缓存
 * 输入参数: 无
 * 输出参数: HA/HD/LD/LA 物理量及 PORT_RealTime_DATA 测量缓存。
 * 返回值: 无
 * 执行说明: 原实时采样路径；采样比例、寄存器偏移及滤波顺序保持原工程。
 *================================================================*/
extern void HVAC_RealTimeParameter(void);
#endif

