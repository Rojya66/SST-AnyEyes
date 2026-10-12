
//###########################################################################
//
// FILE:   PluginUnit.c
//
// FUNCTION:  1)driver of Plug-in Unit
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

#include "main.h"
#include "PluginUnit.h"
#include "ChbBuildConfig.h"

float tr1,tr2,tr3; /* tr1：原 FPGA 通信/调制测试观察量 1。 */
/* tr2：原 FPGA 通信/调制测试观察量 2。 */
/* tr3：原 FPGA 通信/调制测试观察量 3。 */


/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 保留原调制测试入口，当前函数体内的测试代码被注释关闭
 * 输入参数: 无
 * 输出参数: 无；当前函数为空。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Modualation_Test(void);
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void);
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void);
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void);
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga);
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void);
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void);

/*==================================================================
 * 函数功能: 按位图设置模块工频桥、DAB 桥的 PWM 使能
 * 输入参数: IndexM：模块桥类型使能位图，沿用原函数按位解释
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_PWMEN(uchar IndexM);
/*==================================================================
 * 函数功能: 设置模块工频桥 PWM 使能并回写 FPGA 控制字
 * 输入参数: EN50Hz：工频桥使能，0 禁用、非零使能
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_50Hz_PWMEN(uchar EN50Hz);
/*==================================================================
 * 函数功能: 设置模块 DAB 桥 PWM 使能并回写 FPGA 控制字
 * 输入参数: ENdab：DAB 桥使能，0 禁用、非零使能
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_ENdab_PWMEN(uchar ENdab);

/*==================================================================
 * 函数功能: 设置工频桥与 DAB 桥的驱动分频控制位
 * 输入参数: DoublingValue50Hz：工频桥驱动分频字段值
 *           DoublingValueDAB：DAB 驱动分频字段值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void MOSFET_50Hz_FrequencyDoubling(uchar DoublingValue50Hz,uchar DoublingValueDAB);
/*==================================================================
 * 函数功能: 设置工频桥与 DAB 桥驱动复位控制位
 * 输入参数: Rst50Hz：工频桥驱动复位控制值
 *           RstDab：DAB 驱动复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void MOSFET_DriverReset(uchar Rst50Hz,uchar RstDab);
/*==================================================================
 * 函数功能: 设置模块硬件复位控制位
 * 输入参数: runindex：模块硬件复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_HW_RST(uchar runindex);
/*==================================================================
 * 函数功能: 设置模块风扇关闭控制位
 * 输入参数: FANdisable：模块风扇关闭控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_FAN_OFF(uchar FANdisable);
/*==================================================================
 * 函数功能: 设置两类桥的硬件关断有效控制位
 * 输入参数: valid50Hz：工频桥关断有效控制值
 *           validDab：DAB 桥关断有效控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Shutdown_valid(uchar valid50Hz,uchar validDab);
/*==================================================================
 * 函数功能: 设置模块控制字中的测试模式字段
 * 输入参数: TestMode：模块单链测试模式字段值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_TestMode(uchar TestMode);
/*==================================================================
 * 函数功能: 将 Port_DOUT 开关控制字写入端口 FPGA 数字输出寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_IO_Out(void);
/*==================================================================
 * 函数功能: 写入端口 AD 硬件保护使能位图
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_ADProtect_EN(void);
/*==================================================================
 * 函数功能: 按光纤插件编号写入各组模块关断位图
 * 输入参数: OpticalNum：光纤插件/组编号
 *           Shutdown_1：第 1 组模块关断位图
 *           Shutdown_2：第 2 组模块关断位图
 *           Shutdown_3：第 3 组模块关断位图
 *           Shutdown_4：第 4 组模块关断位图
 *           Shutdown_5：第 5 组模块关断位图
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_ModuleShutdown(ushort OpticalNum,ushort Shutdown_1,ushort Shutdown_2,ushort Shutdown_3,ushort Shutdown_4,ushort Shutdown_5);
/*==================================================================
 * 函数功能: 设置光纤故障输入的各相使能位图
 * 输入参数: OpticalNum：光纤插件/组编号
 *           INV_A_FOEN：A 相逆变故障光纤使能位图
 *           INV_B_FOEN：B 相逆变故障光纤使能位图
 *           INV_C_FOEN：C 相逆变故障光纤使能位图
 *           INV_N_FOEN：中性桥故障光纤使能位图
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_FO_EN_Set(ushort OpticalNum,ushort INV_A_FOEN,ushort INV_B_FOEN,ushort INV_C_FOEN,ushort INV_N_FOEN);
/*==================================================================
 * 函数功能: 设置独立逆变桥的 FPGA 死区计数
 * 输入参数: Deadzone_inv：逆变桥死区 FPGA 计数
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void INV_CONTROL_DEADZONE_Set(ushort Deadzone_inv);
/*==================================================================
 * 函数功能: 设置独立逆变桥的 FPGA 周期计数
 * 输入参数: Period_inv：逆变桥周期 FPGA 计数
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void INV_CONTROL_PERIOD_Set(ushort Period_inv);
/*==================================================================
 * 函数功能: 写入 FPGA BKU 故障锁存控制值
 * 输入参数: LatchValue：BKU 故障锁存控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_BKU_Send(uchar LatchValue);
/*==================================================================
 * 函数功能: 写入端口 FB 硬件保护使能位图
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_FB_EN_Set(void);
/*==================================================================
 * 函数功能: 设置端口 FPGA 的硬件故障复位控制位
 * 输入参数: RST_FB：端口硬件故障复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_DSPControl_FBRST(uchar RST_FB);
/*==================================================================
 * 函数功能: 设置端口 FPGA 的全模块关断控制位
 * 输入参数: AllShutdown：全模块关断控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_DSPControl_AllShutdown(uchar AllShutdown);
/*==================================================================
 * 函数功能: 读取指定插件、模块的实时或锁存状态字
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           MK_index：插件内模块编号，从 1 开始，模块数组下标为编号减 1
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 模块的 16 位原始状态字
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Module_StatusRead(uchar CJ_index,uchar MK_index,uchar latch);
/*==================================================================
 * 函数功能: 读取指定插件所有模块的汇总状态
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 插件的 16 位原始汇总状态
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Module_TotalStatusRead(uchar CJ_index,uchar latch);
/*==================================================================
 * 函数功能: 读取端口数字输入的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位数字输入位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_DINRead(uchar latch);
/*==================================================================
 * 函数功能: 读取端口 AD 硬件保护的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位 AD 保护位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_AD_ProtectStatusRead(uchar latch);
/*==================================================================
 * 函数功能: 读取指定插件的光纤故障实时或锁存值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位光纤故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_FOStatusRead(uchar CJ_index,uchar latch);
/*==================================================================
 * 函数功能: 读取端口 FB 故障的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位 FB 故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_FBStatusRead(uchar latch);
/*==================================================================
 * 函数功能: 读取独立逆变桥光纤故障的实时或锁存值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位逆变桥光纤故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_INVFOStatusRead(uchar CJ_index,uchar latch);
/*==================================================================
 * 函数功能: 组装 FT3 采样、状态及故障数据并写入所选 FT3 插件
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 *           dataPackNum：FT3 本次发送的数据包数/控制字字段
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void FT3_PluginUint_Write(unsigned short FT3_index, unsigned short dataPackNum);
/*==================================================================
 * 函数功能: 读取所选 FT3 插件接收区到兼容接收缓存
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void FT3_PluginUint_Read(ushort FT3_index);
/*==================================================================
 * 函数功能: 将 DAC_BUF 四个通道值写入主控插件的 DA 寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void DAC_Output(void);

/*故障录波装置通信测试用*/
struct _Receive_Data
{
    unsigned short Rec_Data1; /* Rec_Data1：FT3 接收第 1 个原始 16 位字。 */
    unsigned short Rec_Data2; /* Rec_Data2：FT3 接收第 2 个原始 16 位字。 */
    unsigned short Rec_Data3; /* Rec_Data3：FT3 接收第 3 个原始 16 位字。 */
    unsigned short Rec_Data4; /* Rec_Data4：FT3 接收第 4 个原始 16 位字。 */
    unsigned short Rec_Data5; /* Rec_Data5：FT3 接收第 5 个原始 16 位字。 */
    unsigned short Rec_Data6; /* Rec_Data6：FT3 接收第 6 个原始 16 位字。 */
    unsigned short Rec_Data7; /* Rec_Data7：FT3 接收第 7 个原始 16 位字。 */
    unsigned short Rec_Data8; /* Rec_Data8：FT3 接收第 8 个原始 16 位字。 */

} FT3_Rec_Data; /* FT3_Rec_Data：故障录波 FT3 接收的八字原始数据缓存。 */

unsigned int temp_rec; /* temp_rec：FT3 接收通信测试的原临时观察值。 */
/****************************************************************************/
/*                                                                          */
/*              Modualation Test For HVAC                                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 保留原调制测试入口，当前函数体内的测试代码被注释关闭
 * 输入参数: 无
 * 输出参数: 无；当前函数为空。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Modualation_Test(void)
{
/*	
	ushort ModualationTemp1,ModualationTemp2;
	ModualationTemp1=FPGA_Interface[Com_A2_PluginUnit_6-2].Debug_L;
	ModualationTemp2=FPGA_Interface[Com_A2_PluginUnit_6-2].Debug_R;
	ModualationTemp1=ModualationTemp1>>4;
	ModualationTemp2=ModualationTemp2>>4;

	DAC_BUF[0]=ModualationTemp2;
  DAC_BUF[1]=ModualationTemp1;

	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA0=(unsigned short)DAC_BUF[0];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA1=(unsigned short)DAC_BUF[1];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA2=(unsigned short)DAC_BUF[2];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA3=(unsigned short)DAC_BUF[3];
*/	
}

#ifdef HVAC_online
/****************************************************************************/
/*                                                                          */
/*              Write control parameter for 50Hz H-bridge                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{
	float ControlParaTemp_1; /* ControlParaTemp_1：参考归一化使用的模块直流电压/基准缓存。 */
	float ControlParaTemp_2; /* ControlParaTemp_2：三相工频桥参考缩放与移位的中间值。 */
		
			
//Phase A	
/*
if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(PortRunFLAG.Bbits.Port_UdcOK==0)&&(Udc_Ref!=HA_Module_RefUdc))
{
	ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
}
else
{
	ControlParaTemp_1=SVG_Udc_A_Ave;
}
*/

ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
//ControlParaTemp_1=SVG_Udc_A_Ave;

	if(flag_INV==1)
	{
		ControlParaTemp_1=SVG_Udc_A_Ave;
	}

	ControlParaTemp_2=HA_Module_RefVolt_A+ControlParaTemp_1;
	
	if(ControlParaTemp_1==0.0)
	{
		ControlParaTemp_2=0.0;
	}
	else
	{
		ControlParaTemp_2=ControlParaTemp_2/ControlParaTemp_1;
	}
	
	if(ControlParaTemp_2>1.98)
	{
		ControlParaTemp_2=1.98;
	}
	else if(ControlParaTemp_2<0.02)
	{
		ControlParaTemp_2=0.02;
	}	

	ControlParaTemp_1=HA_ReferenceDEF_A;
	ControlParaTemp_2=ControlParaTemp_2*ControlParaTemp_1;

	FPGA_Interface[Com_A1_PluginUnit_5-2].Reference=(ushort)ControlParaTemp_2;
	FPGA_Interface[Com_A2_PluginUnit_6-2].Reference=(ushort)ControlParaTemp_2;

#ifdef COM_TEST_PluginUnit

			Wrong_ControlReference_A=(ushort)ControlParaTemp_2;
			if(PortRunFLAG.Bbits.PWMState_50Hz==0)
			{
				if((Wrong_ControlReference_A-LastWrong_ControlReference_A)>1300)
				{
					WrongTimes_ControlReference_A++;
					if(WrongTimes_ControlReference_A==65535)
					{
						WrongTimes_ControlReference_A=0;
					}
				}
				else if((Wrong_ControlReference_A-LastWrong_ControlReference_A)<(-1300))
				{
					WrongTimes_ControlReference_A++;
					if(WrongTimes_ControlReference_A==65535)
					{
						WrongTimes_ControlReference_A=0;
					}
				}
			}		
			LastWrong_ControlReference_A=Wrong_ControlReference_A;	
			
#endif	
		
	if(HA_IG_Log_A==0)
	{
		FPGA_Interface[Com_A1_PluginUnit_5-2].DC_Log.Bbits.IG_Orient=0;      
		FPGA_Interface[Com_A2_PluginUnit_6-2].DC_Log.Bbits.IG_Orient=0;
	}
	else
	{
		FPGA_Interface[Com_A1_PluginUnit_5-2].DC_Log.Bbits.IG_Orient=1;      
		FPGA_Interface[Com_A2_PluginUnit_6-2].DC_Log.Bbits.IG_Orient=1;	
	}

	
	if(HA_DC_Log_A==0)
	{
		FPGA_Interface[Com_A1_PluginUnit_5-2].DC_Log.Bbits.DC_Order=0;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DC_Log.Bbits.DC_Order=1;			
	}
	else
	{
		FPGA_Interface[Com_A1_PluginUnit_5-2].DC_Log.Bbits.DC_Order=1;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DC_Log.Bbits.DC_Order=0;			
	}

//Phase B
/*
if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(PortRunFLAG.Bbits.Port_UdcOK==0)&&(Udc_Ref!=HA_Module_RefUdc))
{
	ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
}
else
{
	ControlParaTemp_1=SVG_Udc_B_Ave;
}
*/

	ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
//	ControlParaTemp_1=SVG_Udc_B_Ave;


	if(flag_INV==1)
	{
		ControlParaTemp_1=SVG_Udc_B_Ave;
	}

	ControlParaTemp_2=HA_Module_RefVolt_B+ControlParaTemp_1;

	if(ControlParaTemp_1==0.0)
	{
		ControlParaTemp_2=0.0;
	}
	else
	{
		ControlParaTemp_2=ControlParaTemp_2/ControlParaTemp_1;
	}
	
	if(ControlParaTemp_2>1.98)
	{
		ControlParaTemp_2=1.98;
	}
	else if(ControlParaTemp_2<0.02)
	{
		ControlParaTemp_2=0.02;
	}	
	
	ControlParaTemp_1=HA_ReferenceDEF_B;
	ControlParaTemp_2=ControlParaTemp_2*ControlParaTemp_1;

			
	FPGA_Interface[Com_B1_PluginUnit_9-2].Reference=(ushort)ControlParaTemp_2;
	FPGA_Interface[Com_B2_PluginUnit_10-2].Reference=(ushort)ControlParaTemp_2;
	
	
	if(HA_IG_Log_B==0)
	{
		FPGA_Interface[Com_B1_PluginUnit_9-2].DC_Log.Bbits.IG_Orient=0;
		FPGA_Interface[Com_B2_PluginUnit_10-2].DC_Log.Bbits.IG_Orient=0;
	}
	else
	{
		FPGA_Interface[Com_B1_PluginUnit_9-2].DC_Log.Bbits.IG_Orient=1;
		FPGA_Interface[Com_B2_PluginUnit_10-2].DC_Log.Bbits.IG_Orient=1;	
	}
	
	if(HA_DC_Log_B==0)
	{
		FPGA_Interface[Com_B1_PluginUnit_9-2].DC_Log.Bbits.DC_Order=0;
		FPGA_Interface[Com_B2_PluginUnit_10-2].DC_Log.Bbits.DC_Order=1;			
	}
	else
	{
		FPGA_Interface[Com_B1_PluginUnit_9-2].DC_Log.Bbits.DC_Order=1;
		FPGA_Interface[Com_B2_PluginUnit_10-2].DC_Log.Bbits.DC_Order=0;			
	}	

//Phase C
/*
if((PortRunFLAG.Bbits.PWMState_50Hz==0)&&(PortRunFLAG.Bbits.Port_UdcOK==0)&&(Udc_Ref!=HA_Module_RefUdc))
{
	ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
}
else
{
	ControlParaTemp_1=SVG_Udc_C_Ave;
}
*/

	ControlParaTemp_1=Udc_Ref;//HA_Module_RefUdc;
//ControlParaTemp_1=SVG_Udc_C_Ave;

	if(flag_INV==1)
	{
		ControlParaTemp_1=SVG_Udc_C_Ave;
	}
		
	ControlParaTemp_2=HA_Module_RefVolt_C+ControlParaTemp_1;
	
	if(ControlParaTemp_1==0.0)
	{
		ControlParaTemp_2=0.0;
	}
	else
	{
		ControlParaTemp_2=ControlParaTemp_2/ControlParaTemp_1;
	}
	
	if(ControlParaTemp_2>1.98)
	{
		ControlParaTemp_2=1.98;
	}
	else if(ControlParaTemp_2<0.02)
	{
		ControlParaTemp_2=0.02;
	}	

		
	ControlParaTemp_1=HA_ReferenceDEF_C;
	ControlParaTemp_2=ControlParaTemp_2*ControlParaTemp_1;

			
	FPGA_Interface[Com_C1_PluginUnit_13-2].Reference=(ushort)ControlParaTemp_2;
	FPGA_Interface[Com_C2_PluginUnit_14-2].Reference=(ushort)ControlParaTemp_2;
	
//	DAC_BUF[0]=(float)ControlParaTemp_2/14000.0;
#ifdef COM_TEST_PluginUnit
/*
			Wrong_ControlReference_C=(ushort)ControlParaTemp_2;
			if(PortRunFLAG.Bbits.PWMState_50Hz==0)
			{
				if((Wrong_ControlReference_C-LastWrong_ControlReference_C)>1300)
				{
					WrongTimes_ControlReference_C++;
					if(WrongTimes_ControlReference_C==65535)
					{
						WrongTimes_ControlReference_C=0;
					}
				}
				else if((Wrong_ControlReference_C-LastWrong_ControlReference_C)<(-1300))
				{
					WrongTimes_ControlReference_C++;
					if(WrongTimes_ControlReference_C==65535)
					{
						WrongTimes_ControlReference_C=0;
					}
				}
			}		
			LastWrong_ControlReference_C=Wrong_ControlReference_C;					
*/
#endif	
	
	if(HA_IG_Log_C==0)
	{
		FPGA_Interface[Com_C1_PluginUnit_13-2].DC_Log.Bbits.IG_Orient=0;
		FPGA_Interface[Com_C2_PluginUnit_14-2].DC_Log.Bbits.IG_Orient=0;
	}
	else
	{
		FPGA_Interface[Com_C1_PluginUnit_13-2].DC_Log.Bbits.IG_Orient=1;
		FPGA_Interface[Com_C2_PluginUnit_14-2].DC_Log.Bbits.IG_Orient=1;	
	}

	if(HA_DC_Log_C==0)
	{
		FPGA_Interface[Com_C1_PluginUnit_13-2].DC_Log.Bbits.DC_Order=0;
		FPGA_Interface[Com_C2_PluginUnit_14-2].DC_Log.Bbits.DC_Order=1;			
	}
	else
	{
		FPGA_Interface[Com_C1_PluginUnit_13-2].DC_Log.Bbits.DC_Order=1;
		FPGA_Interface[Com_C2_PluginUnit_14-2].DC_Log.Bbits.DC_Order=0;			
	}		

/*	
	if(flag_SendDABRef_First==2)
	{
		#ifdef HVAC_online
		PWMPluse_Enable(1);
		#endif  

		#ifdef LVAC_online
		PWMPluse_Enable(1);
		#endif  			
	}
*/
}

/****************************************************************************/
/*                                                                          */
/*              Write control parameter for DAB H-bridge                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{	
//		HA_DABDuty_A=2500;
//		HA_DAB_T=5000;
		HA_DAB_PluseStart_A=1250; 
		
//		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Duty=HA_DABDuty_A;     
//		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Duty=HA_DABDuty_A;     

		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=HA_DAB_PluseStart_A;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=HA_DAB_PluseStart_A;	
		
//		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_T=HA_DAB_T;							
//		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_T=HA_DAB_T;	          


//		HA_DABDuty_B=2500;
//		HA_DAB_T=5000;
		HA_DAB_PluseStart_B=1250; 
//		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Duty=HA_DABDuty_B;
//		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Duty=HA_DABDuty_B;

		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Start=HA_DAB_PluseStart_B;
		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Start=HA_DAB_PluseStart_B;	
		
//		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_T=HA_DAB_T;
//		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_T=HA_DAB_T;				


//		HA_DABDuty_C=2500;
//		HA_DAB_T=5000;
		HA_DAB_PluseStart_C=1250; 
//		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Duty=HA_DABDuty_C;
//		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Duty=HA_DABDuty_C;

		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Start=HA_DAB_PluseStart_C;
		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Start=HA_DAB_PluseStart_C;	
		
//		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_T=HA_DAB_T;
//		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_T=HA_DAB_T;		
}

/****************************************************************************/
/*                                                                          */
/*     Write control parameter for DAB H-bridge at the step of startup      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{			
		if(HA_DABDuty_A<2400)
		{
			HA_DABDuty_A=HA_DABDuty_A+1;	
			
			if(HA_DABDuty_A==2400)
			{
				ControlFLAG.Bbits.DAB_DutyStart=0;
				ControlFLAG.Bbits.DAB_DutyOver=1;
			}		
		}
//	HA_DAB_T=5000;
//	HA_DAB_PluseStart_A=1250; 
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_LEFTDuty=HA_DABDuty_A;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_LEFTDuty=HA_DABDuty_A;     

//	FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=HA_DAB_PluseStart_A;
//	FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=HA_DAB_PluseStart_A;	
		
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_RIGHTDuty=HA_DABDuty_A;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_RIGHTDuty=HA_DABDuty_A;          


//		HA_DABDuty_B=2500;
//		HA_DAB_T=5000;
//		HA_DAB_PluseStart_B=1250; 
		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_LEFTDuty=HA_DABDuty_A;
		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_LEFTDuty=HA_DABDuty_A;

//		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_Start=HA_DAB_PluseStart_B;
//		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_Start=HA_DAB_PluseStart_B;	
		
		FPGA_Interface[Com_B1_PluginUnit_9-2].DAB_RIGHTDuty=HA_DABDuty_A;
		FPGA_Interface[Com_B2_PluginUnit_10-2].DAB_RIGHTDuty=HA_DABDuty_A;		


//		HA_DABDuty_C=2500;
//		HA_DAB_T=5000;
//		HA_DAB_PluseStart_C=1250; //???
		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_LEFTDuty=HA_DABDuty_A;
		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_LEFTDuty=HA_DABDuty_A;

//		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_Start=HA_DAB_PluseStart_C;
//		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_Start=HA_DAB_PluseStart_C;	
		
		FPGA_Interface[Com_C1_PluginUnit_13-2].DAB_RIGHTDuty=HA_DABDuty_A;
		FPGA_Interface[Com_C2_PluginUnit_14-2].DAB_RIGHTDuty=HA_DABDuty_A;		

/*		
		if(flag_SendDABRef_First==2)
		{
				#ifdef HVAC_online
				PWMPluse_Enable(2);
				#endif  

				#ifdef HVDC_online
				PWMPluse_Enable(3);
				#endif  

				#ifdef LVDC_online
				PWMPluse_Enable(3);
				#endif  

				#ifdef LVAC_online
				PWMPluse_Enable(2);
				#endif  			
		}
*/
}

/****************************************************************************/
/*                                                                          */
/*              communication between each other                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{
	if(RW_fpga)
	{
		MainStatus_HVDC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP


		MainStatus_LVDC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP


		MainStatus_LVAC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP		
		
	}
	else
	{
		MainStatus_HVAC.MainStatus16=Port_MainStatus.MainStatus16;
		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_STATUS.MainStatus16=MainStatus_HVAC.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV=
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI=
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP=
	}	
}

/****************************************************************************/
/*                                                                          */
/*              Write Module                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
	/* 只向实际配置的 UNIT 槽位发送 PM_CTL；mask=0 表示槽位未安装。 */
	if(HA_PM_A1 != 0U) FPGA_Interface[Com_A1_PluginUnit_5-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	if(HA_PM_A2 != 0U) FPGA_Interface[Com_A2_PluginUnit_6-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	if(HA_PM_B1 != 0U) FPGA_Interface[Com_B1_PluginUnit_9-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	if(HA_PM_B2 != 0U) FPGA_Interface[Com_B2_PluginUnit_10-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	if(HA_PM_C1 != 0U) FPGA_Interface[Com_C1_PluginUnit_13-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	if(HA_PM_C2 != 0U) FPGA_Interface[Com_C2_PluginUnit_14-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
}

/****************************************************************************/
/*                                                                          */
/*                     TX_EN                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
	FPGA_Interface[Com_A1_PluginUnit_5-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_A2_PluginUnit_6-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_B1_PluginUnit_9-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_B2_PluginUnit_10-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_C1_PluginUnit_13-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_C2_PluginUnit_14-2].TX_EN=0XFFFF;	
}

#endif  


#ifdef HVDC_online
/****************************************************************************/
/*                                                                          */
/*              Write control parameter for 50Hz H-bridge                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{}

/****************************************************************************/
/*                                                                          */
/*              Write control parameter for DAB H-bridge                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{		
		uchar i; /* i：本次数组、参数或模块遍历的索引。 */
		
		HD_DABDuty=2500;
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=HD_DAB_PluseStart;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=HD_DAB_PluseStart;	

		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_LEFTDuty=HD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_LEFTDuty=HD_DABDuty;     
		
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_RIGHTDuty=HD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_RIGHTDuty=HD_DABDuty;		

//#ifdef HD_10KV_JY
//		#ifdef HD_10KV_JY_single 
		
if(Volt_EvenMethod==1)
{				
		for(i=0;i<8;i++)
		{
			FPGA_Interface[Com_A1_PluginUnit_5-2].HD_S_PH[i]=MK_HD_DAB_PluseStart[i];
		}

		for(i=8;i<15;i++)
		{
			FPGA_Interface[Com_A2_PluginUnit_6-2].HD_S_PH[i-8]=MK_HD_DAB_PluseStart[i];
		}
}

if(Volt_EvenMethod==2)
{
//		#else
		
		for(i=0;i<5;i++)
		{
			FPGA_Interface[Com_A1_PluginUnit_5-2].HD_S_PH[i]=MK_HD_DAB_PluseStart[0];
		}

		for(i=5;i<8;i++)
		{
			FPGA_Interface[Com_A1_PluginUnit_5-2].HD_S_PH[i]=MK_HD_DAB_PluseStart[1];
		}		
		for(i=0;i<2;i++)
		{
			FPGA_Interface[Com_A2_PluginUnit_6-2].HD_S_PH[i]=MK_HD_DAB_PluseStart[1];
		}	

		for(i=2;i<7;i++)
		{
			FPGA_Interface[Com_A2_PluginUnit_6-2].HD_S_PH[i]=MK_HD_DAB_PluseStart[2];
		}			
}
//		#endif
//#endif
}

/****************************************************************************/
/*                                                                          */
/*     Write control parameter for DAB H-bridge at the step of startup      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{	
		if(HD_DABDuty<2500)
		{
			HD_DABDuty=HD_DABDuty+1;	
			if(HD_DABDuty==2500)
			{
				ControlFLAG.Bbits.DAB_DutyStart=0;
				ControlFLAG.Bbits.DAB_DutyOver=1;
			}			
		}

		HD_DAB_PluseStart=1250; 

		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=HD_DAB_PluseStart;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=HD_DAB_PluseStart;	
		
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_LEFTDuty=HD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_LEFTDuty=HD_DABDuty;     
		
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_RIGHTDuty=HD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_RIGHTDuty=HD_DABDuty;  	
}

/****************************************************************************/
/*                                                                          */
/*              communication between each other                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{	
	if(RW_fpga)
	{
		MainStatus_HVAC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP


		MainStatus_LVDC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP


		MainStatus_LVAC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP		
		
	}
	else
	{
		MainStatus_HVDC.MainStatus16=Port_MainStatus.MainStatus16;
		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_STATUS.MainStatus16=MainStatus_HVDC.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV=
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI=
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP=
	}	
}

/****************************************************************************/
/*                                                                          */
/*              Write Module                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
	FPGA_Interface[Com_A1_PluginUnit_5-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	FPGA_Interface[Com_A2_PluginUnit_6-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
}

/****************************************************************************/
/*                                                                          */
/*                     TX_EN                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
	FPGA_Interface[Com_A1_PluginUnit_5-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_A2_PluginUnit_6-2].TX_EN=0XFFFF;
}

#endif

#ifdef LVDC_online
/****************************************************************************/
/*                                                                          */
/*              Write control parameter for 50Hz H-bridge                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{}

/****************************************************************************/
/*                                                                          */
/*              Write control parameter for DAB H-bridge                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{	
		LD_DABDuty=2500;
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=LD_DAB_PluseStart;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=LD_DAB_PluseStart;	

		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_LEFTDuty=LD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_LEFTDuty=LD_DABDuty;     
		
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_RIGHTDuty=LD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_RIGHTDuty=LD_DABDuty;		
}
/****************************************************************************/
/*                                                                          */
/*     Write control parameter for DAB H-bridge at the step of startup      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{	
		if(LD_DABDuty<2500)
		{
			LD_DABDuty=LD_DABDuty+1;	
			if(LD_DABDuty==2500)
			{
				ControlFLAG.Bbits.DAB_DutyStart=0;
				ControlFLAG.Bbits.DAB_DutyOver=1;
			}			
		}

		LD_DAB_PluseStart=1250; 

		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=LD_DAB_PluseStart;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=LD_DAB_PluseStart;	
		
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_LEFTDuty=LD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_LEFTDuty=LD_DABDuty;     
		
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_RIGHTDuty=LD_DABDuty;     
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_RIGHTDuty=LD_DABDuty;  	
}


/****************************************************************************/
/*                                                                          */
/*              communication between each other                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{
	if(RW_fpga)
	{
		MainStatus_HVDC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP


		MainStatus_HVAC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP


		MainStatus_LVAC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP		
		
	}
	else
	{
		MainStatus_LVDC.MainStatus16=Port_MainStatus.MainStatus16;
		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_STATUS.MainStatus16=MainStatus_LVDC.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV=
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI=
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP=
	}	
}

/****************************************************************************/
/*                                                                          */
/*              Write Module                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
	FPGA_Interface[Com_A1_PluginUnit_5-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	FPGA_Interface[Com_A2_PluginUnit_6-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
}

/****************************************************************************/
/*                                                                          */
/*                     TX_EN                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
	FPGA_Interface[Com_A1_PluginUnit_5-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_A2_PluginUnit_6-2].TX_EN=0XFFFF;
}

#endif


#ifdef LVAC_online
/****************************************************************************/
/*                                                                          */
/*              Write control parameter for 50Hz H-bridge                   */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将当前端口的工频桥调制给定换算为 FPGA 参考计数并写入各相插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_50Hz(void)
{
	short ReferenceTemp; /* ReferenceTemp：原 FPGA 调制参考换算缓存。 */
	float ControlParaTemp; /* ControlParaTemp：原工频桥参考换算的浮点中间值。 */
	
	ControlParaTemp=LA_RefVolt_A;
	ControlParaTemp=ControlParaTemp*10.0;
	ReferenceTemp=(short)ControlParaTemp;
	FPGA_Interface[Com_B1_PluginUnit_9-2].INV_REF=ReferenceTemp;
	
	ControlParaTemp=LA_RefVolt_B;
	ControlParaTemp=ControlParaTemp*10.0;
	ReferenceTemp=(short)ControlParaTemp;
	FPGA_Interface[Com_B2_PluginUnit_10-2].INV_REF=ReferenceTemp;
	
	ControlParaTemp=LA_RefVolt_C;
	ControlParaTemp=ControlParaTemp*10.0;
	ReferenceTemp=(short)ControlParaTemp;
	FPGA_Interface[Putout_B_PluginUnit_11-2].INV_REF=ReferenceTemp;
	
	FPGA_Interface[Input_B_PluginUnit_12-2].INV_REF=0;		
}

/****************************************************************************/
/*                                                                          */
/*              Write control parameter for DAB H-bridge                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 写入当前端口 DAB 左右桥占空比及脉冲起始计数
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB(void)
{	
//		LA_DABDuty=2500;
//		LA_DAB_T=5000;
		LA_DAB_PluseStart=1250; 
//		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Duty=LA_DABDuty;
//		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Duty=LA_DABDuty;

		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=LA_DAB_PluseStart;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=LA_DAB_PluseStart;	
		
//		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_T=LA_DAB_T;
//		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_T=LA_DAB_T;	
}

/****************************************************************************/
/*                                                                          */
/*     Write control parameter for DAB H-bridge at the step of startup      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 初始化 DAB 的周期和启动时序寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Write_ControlParameter_DAB_Startup(void)
{	
		if(LA_DABDuty<2500)
		{
			LA_DABDuty=LA_DABDuty+1;
			if(LA_DABDuty==2500)
			{
				ControlFLAG.Bbits.DAB_DutyStart=0;
				ControlFLAG.Bbits.DAB_DutyOver=1;
			}				
		}
		

//		LA_DAB_T=5000;
//		LA_DAB_PluseStart=1250; 
		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Duty=LA_DABDuty;
		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Duty=LA_DABDuty;

//		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_Start=LA_DAB_PluseStart;
//		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_Start=LA_DAB_PluseStart;	
		
//		FPGA_Interface[Com_A1_PluginUnit_5-2].DAB_T=LA_DAB_T;
//		FPGA_Interface[Com_A2_PluginUnit_6-2].DAB_T=LA_DAB_T;		
}
/****************************************************************************/
/*                                                                          */
/*              communication between each other                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{	
	if(RW_fpga)
	{
		MainStatus_HVDC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP


		MainStatus_LVDC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP


		MainStatus_HVAC.MainStatus16=FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_STATUS.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI
//		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP		
		
	}
	else
	{
		MainStatus_LVAC.MainStatus16=Port_MainStatus.MainStatus16;
		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_STATUS.MainStatus16=MainStatus_LVAC.MainStatus16;
		
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV=
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI=
//		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP=
	}	
}

/****************************************************************************/
/*                                                                          */
/*              Write Module                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
	FPGA_Interface[Com_A1_PluginUnit_5-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	FPGA_Interface[Com_A2_PluginUnit_6-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
	
//	FPGA_Interface[InvPWM_A_PluginUnit_9-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
//	FPGA_Interface[InvPWM_B_PluginUnit_10-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
//	FPGA_Interface[InvPWM_C_PluginUnit_11-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
//	FPGA_Interface[InvPWM_N_PluginUnit_12-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;	
}

/****************************************************************************/
/*                                                                          */
/*                     TX_EN                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
	FPGA_Interface[Com_A1_PluginUnit_5-2].TX_EN=0XFFFF;
	FPGA_Interface[Com_A2_PluginUnit_6-2].TX_EN=0XFFFF;
}

#endif


/****************************************************************************/
/*                                                                          */
/*              PWM Disable/Enable                                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按位图设置模块工频桥、DAB 桥的 PWM 使能
 * 输入参数: IndexM：模块桥类型使能位图，沿用原函数按位解释
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_PWMEN(uchar IndexM)
{
	if(IndexM==0)
	{
		ControlFLAG.Bbits.SVG_PWMEN=0;
		ControlFLAG.Bbits.DAB_PWMEN=0;
	}
	else if(IndexM==1)
	{
		ControlFLAG.Bbits.SVG_PWMEN=1;
		ControlFLAG.Bbits.DAB_PWMEN=0;
	}
	else if(IndexM==2)
	{
		ControlFLAG.Bbits.SVG_PWMEN=0;
		ControlFLAG.Bbits.DAB_PWMEN=1;		
	}
	else if(IndexM==3)
	{
		ControlFLAG.Bbits.SVG_PWMEN=1;
		ControlFLAG.Bbits.DAB_PWMEN=1;		
	}		
	else{}
		
	Module_PM_CTL.Bbits.SVG_EN=ControlFLAG.Bbits.SVG_PWMEN;
	Module_PM_CTL.Bbits.DAB_EN=ControlFLAG.Bbits.DAB_PWMEN;
	
	Module_Control_Write();	
}

/****************************************************************************/
/*                                                                          */
/*              PWM Disable/Enable                                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置模块工频桥 PWM 使能并回写 FPGA 控制字
 * 输入参数: EN50Hz：工频桥使能，0 禁用、非零使能
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_50Hz_PWMEN(uchar EN50Hz)
{
	if(EN50Hz)
	{
		ControlFLAG.Bbits.SVG_PWMEN=1;
//		Module_PM_CTL.Bbits.SVG_RST=0;
//		Module_PM_CTL.Bbits.HW_RST=0;
	}
	else
	{
		ControlFLAG.Bbits.SVG_PWMEN=0;
	}
	
	Module_PM_CTL.Bbits.SVG_EN=ControlFLAG.Bbits.SVG_PWMEN;
	
	Module_Control_Write();	
}

/****************************************************************************/
/*                                                                          */
/*              PWM Disable/Enable                                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置模块 DAB 桥 PWM 使能并回写 FPGA 控制字
 * 输入参数: ENdab：DAB 桥使能，0 禁用、非零使能
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_ENdab_PWMEN(uchar ENdab)
{
	if(ENdab)
	{
		ControlFLAG.Bbits.DAB_PWMEN=1;
//		Module_PM_CTL.Bbits.DAB_RST=0;
//		Module_PM_CTL.Bbits.HW_RST=0;		
	}
	else
	{
		ControlFLAG.Bbits.DAB_PWMEN=0;
	}

	Module_PM_CTL.Bbits.DAB_EN=ControlFLAG.Bbits.DAB_PWMEN;
	
	Module_Control_Write();	
}

/****************************************************************************/
/*                                                                          */
/*              Switching frequency doubling                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置工频桥与 DAB 桥的驱动分频控制位
 * 输入参数: DoublingValue50Hz：工频桥驱动分频字段值
 *           DoublingValueDAB：DAB 驱动分频字段值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void MOSFET_50Hz_FrequencyDoubling(uchar DoublingValue50Hz,uchar DoublingValueDAB)
{
	Module_PM_CTL.Bbits.SVG_DIV=DoublingValue50Hz;
	Module_PM_CTL.Bbits.DAB_DIV=DoublingValueDAB;
	Module_Control_Write();	
}

/****************************************************************************/
/*                                                                          */
/*              Reset Driver                                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置工频桥与 DAB 桥驱动复位控制位
 * 输入参数: Rst50Hz：工频桥驱动复位控制值
 *           RstDab：DAB 驱动复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void MOSFET_DriverReset(uchar Rst50Hz,uchar RstDab)
{
	if(Rst50Hz)
	{
		Module_PM_CTL.Bbits.SVG_RST=1;
	}
	if(RstDab)
	{
		Module_PM_CTL.Bbits.DAB_RST=1;
	}	
	Module_Control_Write();			
}

/****************************************************************************/
/*                                                                          */
/*              Clear Latch                                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置模块硬件复位控制位
 * 输入参数: runindex：模块硬件复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_HW_RST(uchar runindex)
{
	Module_PM_CTL.Bbits.HW_RST=runindex;	
	Module_Control_Write();				
}

/****************************************************************************/
/*                                                                          */
/*              FAN Control                                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置模块风扇关闭控制位
 * 输入参数: FANdisable：模块风扇关闭控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_FAN_OFF(uchar FANdisable)
{
	if(FANdisable)
	{
		Module_PM_CTL.Bbits.FAN_OFF=1;	
	}
	else
	{
		Module_PM_CTL.Bbits.FAN_OFF=0;	
	}
	Module_Control_Write();			
}

/****************************************************************************/
/*                                                                          */
/*              Shutdown valid set                                          */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置两类桥的硬件关断有效控制位
 * 输入参数: valid50Hz：工频桥关断有效控制值
 *           validDab：DAB 桥关断有效控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Shutdown_valid(uchar valid50Hz,uchar validDab)
{
	if(valid50Hz)
	{
		Module_PM_CTL.Bbits.SVG_SHUT=1;
	}
	else
	{
		Module_PM_CTL.Bbits.SVG_SHUT=0;
	}
	
	if(validDab)
	{
		Module_PM_CTL.Bbits.DAB_SHUT=1;
	}
	else
	{
		Module_PM_CTL.Bbits.DAB_SHUT=0;
	}	
	
	Module_Control_Write();			
}

/****************************************************************************/
/*                                                                          */
/*              MODE set                                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置模块控制字中的测试模式字段
 * 输入参数: TestMode：模块单链测试模式字段值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_TestMode(uchar TestMode)
{
	if(TestMode)
	{
		Module_PM_CTL.Bbits.ONE=1;
	}
	else
	{
		Module_PM_CTL.Bbits.ONE=0;
	}
	
	Module_Control_Write();			
}

/****************************************************************************/
/*                                                                          */
/*              IO out                                                      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 Port_DOUT 开关控制字写入端口 FPGA 数字输出寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_IO_Out(void)
{
	FPGA_Interface[IO_PluginUnit_2-2].DOUT.SwitchINOUT16=Port_DOUT.SwitchINOUT16;
}

/****************************************************************************/
/*                                                                          */
/*              AD Protect EN                                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 写入端口 AD 硬件保护使能位图
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_ADProtect_EN(void)
{
	FPGA_Interface[Analog_PluginUnit_3-2].AD_PROT_EN=Port_AD_PORT_EN;
}

/****************************************************************************/
/*                                                                          */
/*                       Module Shutdown                                    */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 按光纤插件编号写入各组模块关断位图
 * 输入参数: OpticalNum：光纤插件/组编号
 *           Shutdown_1：第 1 组模块关断位图
 *           Shutdown_2：第 2 组模块关断位图
 *           Shutdown_3：第 3 组模块关断位图
 *           Shutdown_4：第 4 组模块关断位图
 *           Shutdown_5：第 5 组模块关断位图
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_ModuleShutdown(ushort OpticalNum,ushort Shutdown_1,ushort Shutdown_2,ushort Shutdown_3,ushort Shutdown_4,ushort Shutdown_5)
{
	if(OpticalNum==0)
	{
		FPGA_Interface[Putout_A_PluginUnit_7-2].SHUTDOWN=Shutdown_1;
	}
	else if(OpticalNum==1)
	{
		FPGA_Interface[Putout_B_PluginUnit_11-2].SHUTDOWN=Shutdown_2;
	}
	else if(OpticalNum==2)
	{
		FPGA_Interface[Putout_C_PluginUnit_15-2].SHUTDOWN=Shutdown_3;
	}
	else if(OpticalNum==3)
	{
		FPGA_Interface[Putout_A_PluginUnit_7-2].SHUTDOWN=Shutdown_1;
		FPGA_Interface[Putout_B_PluginUnit_11-2].SHUTDOWN=Shutdown_2;
		FPGA_Interface[Putout_C_PluginUnit_15-2].SHUTDOWN=Shutdown_3;
	}
	else if(OpticalNum==4)
	{
#ifdef LVAC_online		
//		FPGA_Interface[Putout_A_PluginUnit_7-2].SHUTDOWN=Shutdown_1;
		
		FPGA_Interface[InvPWM_A_PluginUnit_9-2].INV_SHUTDOWN=Shutdown_2;
		FPGA_Interface[InvPWM_B_PluginUnit_10-2].INV_SHUTDOWN=Shutdown_3;
		FPGA_Interface[InvPWM_C_PluginUnit_11-2].INV_SHUTDOWN=Shutdown_4;		
		FPGA_Interface[InvPWM_N_PluginUnit_12-2].INV_SHUTDOWN=Shutdown_5;	
#endif
	}
	else
	{}
}

/****************************************************************************/
/*                                                                          */
/*                       FO  EN                                             */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置光纤故障输入的各相使能位图
 * 输入参数: OpticalNum：光纤插件/组编号
 *           INV_A_FOEN：A 相逆变故障光纤使能位图
 *           INV_B_FOEN：B 相逆变故障光纤使能位图
 *           INV_C_FOEN：C 相逆变故障光纤使能位图
 *           INV_N_FOEN：中性桥故障光纤使能位图
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_FO_EN_Set(ushort OpticalNum,ushort INV_A_FOEN,ushort INV_B_FOEN,ushort INV_C_FOEN,ushort INV_N_FOEN)
{

	FPGA_Interface[Input_A_PluginUnit_8-2].FO_EN=Port_FO_1_EN;
	
	if(OpticalNum==3)
	{
		FPGA_Interface[Input_B_PluginUnit_12-2].FO_EN=Port_FO_2_EN;
		FPGA_Interface[Input_C_PluginUnit_16-2].FO_EN=Port_FO_3_EN;
	}
	else if(OpticalNum==4)
	{
#ifdef LVAC_online	
		FPGA_Interface[InvPWM_A_PluginUnit_9-2].INV_FO_EN=INV_A_FOEN;
		FPGA_Interface[InvPWM_B_PluginUnit_10-2].INV_FO_EN=INV_B_FOEN;
		FPGA_Interface[InvPWM_C_PluginUnit_11-2].INV_FO_EN=INV_C_FOEN;		
		FPGA_Interface[InvPWM_N_PluginUnit_12-2].INV_FO_EN=INV_N_FOEN;	
#endif		
	}
	else
	{}
}

/****************************************************************************/
/*                                                                          */
/*                       INV CONTROL_DEADZONE                               */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置独立逆变桥的 FPGA 死区计数
 * 输入参数: Deadzone_inv：逆变桥死区 FPGA 计数
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void INV_CONTROL_DEADZONE_Set(ushort Deadzone_inv)
{

#ifdef LVAC_online	
	FPGA_Interface[InvPWM_A_PluginUnit_9-2].INV_CTL.Bbits.DEADZONE=Deadzone_inv;
	FPGA_Interface[InvPWM_B_PluginUnit_10-2].INV_CTL.Bbits.DEADZONE=Deadzone_inv;
	FPGA_Interface[InvPWM_C_PluginUnit_11-2].INV_CTL.Bbits.DEADZONE=Deadzone_inv;		
	FPGA_Interface[InvPWM_N_PluginUnit_12-2].INV_CTL.Bbits.DEADZONE=Deadzone_inv;	
#endif	
}

/****************************************************************************/
/*                                                                          */
/*                       INV CONTROL_PERIOD                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置独立逆变桥的 FPGA 周期计数
 * 输入参数: Period_inv：逆变桥周期 FPGA 计数
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void INV_CONTROL_PERIOD_Set(ushort Period_inv)
{

#ifdef LVAC_online	
	FPGA_Interface[InvPWM_A_PluginUnit_9-2].INV_CTL.Bbits.TAD_DIV=Period_inv;
	FPGA_Interface[InvPWM_B_PluginUnit_10-2].INV_CTL.Bbits.TAD_DIV=Period_inv;
	FPGA_Interface[InvPWM_C_PluginUnit_11-2].INV_CTL.Bbits.TAD_DIV=Period_inv;
	FPGA_Interface[InvPWM_N_PluginUnit_12-2].INV_CTL.Bbits.TAD_DIV=Period_inv;	
#endif	
}

/****************************************************************************/
/*                                                                          */
/*                       BKU  Send                                          */
/*                                                                          */
/****************************************************************************/

/*==================================================================
 * 函数功能: 写入 FPGA BKU 故障锁存控制值
 * 输入参数: LatchValue：BKU 故障锁存控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_BKU_Send(uchar LatchValue)
{
	if(LatchValue)       
	{
//		FPGA_Interface[MainControl_PluginUnit_4-2].BKU=0;
//		Delay(1000);
		FPGA_Interface[MainControl_PluginUnit_4-2].BKU=0XFFFF;
	}
	else
	{
//		FPGA_Interface[MainControl_PluginUnit_4-2].BKU=0XFFFF;
//		Delay(1000);
		FPGA_Interface[MainControl_PluginUnit_4-2].BKU=0;
	}
}


/****************************************************************************/
/*                                                                          */
/*                       FB  EN                                             */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 写入端口 FB 硬件保护使能位图
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_FB_EN_Set(void)
{
	FPGA_Interface[MainControl_PluginUnit_4-2].FB_EN=Port_FB_EN;
}

/****************************************************************************/
/*                                                                          */
/*                       DSP  Control                                       */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置端口 FPGA 的硬件故障复位控制位
 * 输入参数: RST_FB：端口硬件故障复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_DSPControl_FBRST(uchar RST_FB)
{
	if(RST_FB)
	{
		Port_DSP_CTL.Bbits.HW_RST=1;
	}
	else
	{
		Port_DSP_CTL.Bbits.HW_RST=0;
	}
	FPGA_Interface[MainControl_PluginUnit_4-2].DSP_CTL.DSP_CTL16=Port_DSP_CTL.DSP_CTL16;
}

/****************************************************************************/
/*                                                                          */
/*                       All Shutdown                                       */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 设置端口 FPGA 的全模块关断控制位
 * 输入参数: AllShutdown：全模块关断控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_DSPControl_AllShutdown(uchar AllShutdown)
{
	if(AllShutdown)
	{
		Port_DSP_CTL.Bbits.SHUTDOWN=1;	
		
		
	}
	else
	{
		Port_DSP_CTL.Bbits.SHUTDOWN=0;
	}
	FPGA_Interface[MainControl_PluginUnit_4-2].DSP_CTL.DSP_CTL16=Port_DSP_CTL.DSP_CTL16;
}

/****************************************************************************/
/*                                                                          */
/*                       Read Module Status                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取指定插件、模块的实时或锁存状态字
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           MK_index：插件内模块编号，从 1 开始，模块数组下标为编号减 1
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 模块的 16 位原始状态字
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Module_StatusRead(uchar CJ_index,uchar MK_index,uchar latch)
{
	ushort ModuleStatus_Temp; /* ModuleStatus_Temp：读取的模块/插件原始状态字。 */
	if(latch==0)
	{
		ModuleStatus_Temp=FPGA_Interface[CJ_index-2].STATUS[MK_index].STATUS16;  		
	}
	else
	{
//		ModuleStatus_Temp=FPGA_Interface[CJ_index-2].STATUS_LATCH[MK_index].STATUS16;				
	}
			
	return ModuleStatus_Temp;
}

/****************************************************************************/
/*                                                                          */
/*                       Read Module Status                                 */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取指定插件所有模块的汇总状态
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 插件的 16 位原始汇总状态
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Module_TotalStatusRead(uchar CJ_index,uchar latch)
{
	ushort ModuleStatus_Temp; /* ModuleStatus_Temp：读取的模块/插件原始状态字。 */
	if(latch==0)
	{
		ModuleStatus_Temp=FPGA_Interface[CJ_index-2].OR_STATUS.STATUS16;  		
	}
	else
	{
		ModuleStatus_Temp=FPGA_Interface[CJ_index-2].OR_STATUS.STATUS16; 				
	}
			
	return ModuleStatus_Temp;
}
/****************************************************************************/
/*                                                                          */
/*                       Read DIN                                           */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取端口数字输入的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位数字输入位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_DINRead(uchar latch)
{
	XXG_SwitchINOUT DIN_Temp; /* DIN_Temp：读取的端口数字输入原始位图。 */
	if(latch==0)
	{
		DIN_Temp=FPGA_Interface[IO_PluginUnit_2-2].DIN;
	}
	else
	{
		DIN_Temp=FPGA_Interface[IO_PluginUnit_2-2].DIN_LATCH;
	}

#ifdef LVDC_online
	if(DIN_Temp.Bbits.Reservedbit)
	{
		SW.Bbits.S_CONTACTOR=1;
	}
	else
	{
		SW.Bbits.S_CONTACTOR=0;
	}
#endif

#ifdef HVAC_online
	if(DIN_Temp.Bbits.Reservedbit)
	{
		SW.Bbits.S_CONTACTOR=1;
	}
	else
	{
		SW.Bbits.S_CONTACTOR=0;
	}

	if(DIN_Temp.Bbits.Breaker_InnerOff)
	{
		ISO_counter_off = 0;
		if(ISO_counter_on == 500)
		{
		   ISOSwitch = 1;
		}
	}
	else
	{
		ISO_counter_on = 0;
		if(ISO_counter_off == 500)
		{
		   ISOSwitch = 0;
		}
	}

	if(DIN_Temp.Bbits.Breaker_InnerOn)
	{
		GNDSwitch = 1;
	}
	else
	{
		GNDSwitch = 0;
	}
#endif


#ifdef HVDC_online
	if(DIN_Temp.Bbits.Breaker2_OuterOff)
	{
		SW.Bbits.S_INT_CB=0;
	}
	else
	{
		SW.Bbits.S_INT_CB=1;
	}
#else
	if(DIN_Temp.Bbits.Breaker2_OuterOn)
	{
		SW.Bbits.S_INT_CB=0;
	}
	else
	{
		SW.Bbits.S_INT_CB=1;
	}
#endif


	if(DIN_Temp.Bbits.Breaker_OuterOff)
	{
		SW.Bbits.S_EXT_CB=0;
	}
	else
	{
		SW.Bbits.S_EXT_CB=1;
	}
			
	return DIN_Temp.SwitchINOUT16;
}

/****************************************************************************/
/*                                                                          */
/*                       Read AD Protect Status                             */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取端口 AD 硬件保护的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位 AD 保护位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_AD_ProtectStatusRead(uchar latch)
{
	ushort AD_ProtectStatus_Temp; /* AD_ProtectStatus_Temp：读取并选择后的端口 AD 保护原始状态字。 */
	if(latch==0)
	{
		AD_ProtectStatus_Temp=FPGA_Interface[Analog_PluginUnit_3-2].AD_PROT;  		
	}
	else
	{
		AD_ProtectStatus_Temp=FPGA_Interface[Analog_PluginUnit_3-2].AD_PROT_LATCH;				
	}
			
	return AD_ProtectStatus_Temp;
}

/****************************************************************************/
/*                                                                          */
/*                       Read FO Status                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取指定插件的光纤故障实时或锁存值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位光纤故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_FOStatusRead(uchar CJ_index,uchar latch)
{
	ushort FOStatus_Temp; /* FOStatus_Temp：读取的插件 FO 原始故障位图。 */
	if(latch==0)
	{
		FOStatus_Temp=FPGA_Interface[CJ_index-2].FO;  		
	}
	else
	{
		FOStatus_Temp=FPGA_Interface[CJ_index-2].FO_LATCH;				
	}
			
	return FOStatus_Temp;
}

/****************************************************************************/
/*                                                                          */
/*                       Read FB Status                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取端口 FB 故障的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位 FB 故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_FBStatusRead(uchar latch)
{
	ushort FBStatus_Temp; /* FBStatus_Temp：读取的端口 FB 原始故障位图。 */
	if(latch==0)
	{
		FBStatus_Temp=FPGA_Interface[MainControl_PluginUnit_4-2].FB;  		
	}
	else
	{
		FBStatus_Temp=FPGA_Interface[MainControl_PluginUnit_4-2].FB_LATCH;				
	}
			
	return FBStatus_Temp;
}

/****************************************************************************/
/*                                                                          */
/*                       Read INVFO Status                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取独立逆变桥光纤故障的实时或锁存值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位逆变桥光纤故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_INVFOStatusRead(uchar CJ_index,uchar latch)
{
	ushort INVFOStatus_Temp; /* INVFOStatus_Temp：读取的独立逆变桥 FO 原始故障位图。 */
	if(latch==0)
	{
		INVFOStatus_Temp=FPGA_Interface[CJ_index-2].INV_FO;  		
	}
	else
	{
		INVFOStatus_Temp=FPGA_Interface[CJ_index-2].INV_FO_LATCH;				
	}
			
	return INVFOStatus_Temp;
}


/****************************************************************************/
/*                                                                          */
/*              备用插件写操作                                              */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 组装 FT3 采样、状态及故障数据并写入所选 FT3 插件
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 *           dataPackNum：FT3 本次发送的数据包数/控制字字段
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void FT3_PluginUint_Write(unsigned short FT3_index,unsigned short dataPackNum) //PORT_Index端口号(1,2,3,4分别为HVAC,HVDC,LVDC,LVAC),dataPackNum数据包号
{
    switch(FT3_index)
    {
        case FT3_FaultRecorder:
        {
        	/*
        	PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16 = DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_RUN_STATUS.MainStatus16;
            if(FR_tri_count1 < 300)
            {
            	FR_tri_count1++;
            }
            else
            {
            	FR_tri_count1 = 0;
            }

            if(FR_tri_count2 < 250)
            {
            	FR_tri_count2++;
            }
            else
            {
            	FR_tri_count2 = 0;
            }
            if(FR_tri_count3 < 200)
            {
            	FR_tri_count3++;
            }
            else
            {
            	FR_tri_count3 = 0;
            }
            */
        	/*
            if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x00)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_Initialing = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x01)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_Initial_Success = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x02)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_Selfcheck = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x03)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_Precharge = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x04)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_selfcheck_fail = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x05)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_ready = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x20)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_Run = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x07)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFF00);
        		FaultRecordStatus.Bits.StatusNum_Stop = 1;
        	}
        	else
        	{
        	}
        	*/
            if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x05)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
        		FaultRecordStatus.Bits.StatusNum_ready = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x20)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
        		FaultRecordStatus.Bits.StatusNum_Run = 1;
        	}
        	else if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_Status == 0x07)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
        		FaultRecordStatus.Bits.StatusNum_Stop = 1;
        	}
        	else
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFFF8);
        	}

        	if (PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_S1_Mode == 0x01)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFFF7);
        		FaultRecordStatus.Bits.STATCOM_Q = 1;
        	}
        	else
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0xFFF7);
        	}

        	FaultRecordStatus.Bits.Sys_Stop=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.Sys_Stop;
        	FaultRecordStatus.Bits.Outer_Elec=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.Outer_Elec;

        	if(PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x00)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
        		FaultRecordStatus.Bits.voltage_mode = 1;
        	}
        	else if(PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x01)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
        		FaultRecordStatus.Bits.power_mode = 1;
        	}
        	else if(PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x02)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
        		FaultRecordStatus.Bits.droop_mode = 1;
        	}
        	else if(PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_RUN_STATUS.Bbits.P_M_Mode == 0x03)
        	{
        		FaultRecordStatus.FaultREcord_Word16=FaultRecordStatus.FaultREcord_Word16 & (0x03FF);
        		FaultRecordStatus.Bits.gridfollow_mode = 1;
        	}
        	else
        	{
        	}
#ifdef HVAC_online
        	//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16 = DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                    if((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL&0x00000001)==0)//根据格欣文档，最低位为0时才可发送数据
					{
                    	/*
						if(dataPackNum==0x01)
                    	{

							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
    	                	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
        	            	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
            	        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                	    	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                    		FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;

                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x013F;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=HA_Port_Volt_AB;//程序中变量名为HA_Port_Volt_AB？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=HA_Port_Volt_BC;//程序中变量名为HA_Port_Volt_BC？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=HA_Port_Volt_CA;//程序中变量名为HA_Port_Volt_CA？

						}
                    	else if(dataPackNum==0x02)
                    	{

							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;

                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x023F;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=HA_Port_Current_A;//程序中变量名为HA_Port_Current_A？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=HA_Port_Current_B;//程序中变量名为HA_Port_Current_B？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=HA_Port_Current_C;//程序中变量名为HA_Port_Current_C？
                        }

						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FaultRecordStatus_1.FaultREcord_Word16_1;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FaultRecordStatus.FaultREcord_Word16;
                        //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=faultsignal;

						FT3_Samp_Cnt++;
						if(FT3_Samp_Cnt==50000)
                    	{
							FT3_Samp_Cnt=0;
						}
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=FT3_Samp_Cnt;
						*/

                    	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=FaultRecordStatus.FaultREcord_Word16;
                    	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=HA_Port_Volt_AB;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=HA_Port_Volt_BC;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=HA_Port_Volt_CA;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=HA_Port_Current_A*10;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=HA_Port_Current_B*10;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=HA_Port_Current_C*10;//
#if SST_ENABLE_DPRAM
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
#else
                        /* Current fault word in local RAM; no disabled CS5 read. */
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=Port_RealFault.PORT_FAULT16;
#endif

                        /*
                    	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=FaultRecordStatus.FaultREcord_Word16;
                    	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=FR_tri_count1;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=FR_tri_count2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=FR_tri_count3;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FR_tri_count1*2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FR_tri_count2*2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=FR_tri_count3*2;//
                        FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
                        */
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL=1;

					}
#endif

#ifdef HVDC_online
                	if((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL&0x00000001)==0)
					{
						if(dataPackNum==0x01)
                    	{
							/*
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
    	                	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=0;
        	            	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=0;
            	        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                	    	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                    		FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                        	*/
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x0139;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=HD_Port_Volt;//程序中变量名为HD_Port_Volt？

						}
                    	else if(dataPackNum==0x02)
                    	{
                        	/*
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=0;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=0;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                        	*/
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x0239;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=HD_Port_Current;//程序中变量名为HD_Port_Current？

                        }

						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FaultRecordStatus_1.FaultREcord_Word16_1;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FaultRecordStatus.FaultREcord_Word16;
                        //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=faultsignal;
					   	FT3_Samp_Cnt++;
						if(FT3_Samp_Cnt==50000)
                    	{
							FT3_Samp_Cnt=0;
						}
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=FT3_Samp_Cnt;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL=1;
					}
#endif

#ifdef LVDC_online
                	if((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL&0x00000001)==0)
					{
                		/*
						if(dataPackNum==0x01)
                    	{
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x0139;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=LD_TotalVDC;//程序中变量名为LD_TotalVDC？

						}
                    	else if(dataPackNum==0x02)
                    	{
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x0239;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=LD_Port_PositiveCurrent;//程序中变量名为LD_Port_PositiveCurrent？

                        }

						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FaultRecordStatus_1.FaultREcord_Word16_1;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FaultRecordStatus.FaultREcord_Word16;
                        //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=faultsignal;
					   	FT3_Samp_Cnt++;
						if(FT3_Samp_Cnt==50000)
                    	{
							FT3_Samp_Cnt=0;
						}
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=FT3_Samp_Cnt;*/
                		FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=FaultRecordStatus.FaultREcord_Word16;
                		FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=LD_TotalVDC*10;
                		FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=LD_Port_PositiveCurrent*10;
                	    //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=FR_tri_count1;
                		//FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FR_tri_count3;
                		FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL=1;
					}
#endif

#ifdef LVAC_online
					if((FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL&0x00000001)==0)
					{
						if(dataPackNum==0x01)
                    	{
							/*
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
    	                	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
        	            	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
            	        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                	    	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                    		FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                        	*/
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x013F;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=LA_Port_Volt_A;//程序中变量名为LA_Port_Volt_A？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=LA_Port_Volt_B;//程序中变量名为LA_Port_Volt_B？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=LA_Port_Volt_C;//程序中变量名为LA_Port_Volt_C？

						}
                    	else if(dataPackNum==0x02)
                    	{
                        	/*
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT1_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT2_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT3_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT4_EN=0;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT5_EN=1;
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Bbits.RT6_EN=1;
                        	*/
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_ControlWord.Control_Word16=0x023F;
							FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT1=LA_Ig_Current_A;//程序中变量名为LA_Ig_Current_A？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT2=LA_Ig_Current_B;//程序中变量名为LA_Ig_Current_B？
                        	FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT3=LA_Ig_Current_C;//程序中变量名为LA_Ig_Current_C？
                        }

						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT4=FaultRecordStatus_1.FaultREcord_Word16_1;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT5=FaultRecordStatus.FaultREcord_Word16;
                        //FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;//PORT_RealTime_DATA_buf.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_RT6=faultsignal;
					   	FT3_Samp_Cnt++;
						if(FT3_Samp_Cnt==50000)
                    	{
							FT3_Samp_Cnt=0;
						}
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendData_SampCnt=FT3_Samp_Cnt;
						FT3_FaultRecorder_Register.FT3_Register.FT3_SendCTL=1;
					}

#endif
        }

        case FT3_PTCT:
        {


		}break;

		case FT3_EER2:
        {


		}break;

		case FT3_Backup:
        {


		}break;
		default:break;
    }

}

/****************************************************************************/
/*                                                                          */
/*              备用插件读操作                                              */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取所选 FT3 插件接收区到兼容接收缓存
 * 输入参数: FT3_index：PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void FT3_PluginUint_Read(ushort FT3_index)
{	

	switch(FT3_index)
		{
			case FT3_FaultRecorder:
			{
				if((FT3_FaultRecorder_Register.FT3_Register.FT3_ReadCTL&0x00000001) == 0x00000001) //根据格欣文档，此位为1表示有数据
				{
					FT3_Rec_Data.Rec_Data1=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_1;
					FT3_Rec_Data.Rec_Data2=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_2;
					FT3_Rec_Data.Rec_Data3=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_3;
					FT3_Rec_Data.Rec_Data4=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_4;
					FT3_Rec_Data.Rec_Data5=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_5;
					FT3_Rec_Data.Rec_Data6=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_6;
					FT3_Rec_Data.Rec_Data7=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_7;
					FT3_Rec_Data.Rec_Data8=FT3_FaultRecorder_Register.FT3_Register.FT3_RecData_8;
					FT3_FaultRecorder_Register.FT3_Register.FT3_ReadCTL=0;
				}

			}break;
			case FT3_PTCT:
			{
			}break;

			case FT3_EER2:
			{
			}break;
			
			case FT3_Backup:
			{
			}break;
			
			default:break;
		}

}

/****************************************************************************/
/*                                                                          */
/*              DAC OUT PUT FOR TEST                                        */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 将 DAC_BUF 四个通道值写入主控插件的 DA 寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void DAC_Output(void)
{
	unsigned char i; /* i：本次数组、参数或模块遍历的索引。 */
	float tempDAC; /* tempDAC：单通道 DA 输出缩放、限幅后的计数缓存。 */
	
	for(i=0;i<4;i++)
	{
		if(DAC_BUF[i]>5.0)
		{
			DAC_BUF[i]=5.0;
		}

		if(DAC_BUF[i]<0)
		{
			DAC_BUF[i]=0;
		}
		
		tempDAC=DAC_BUF[i]*4095.0/5.0;
		
		DAC_BUF[i]=tempDAC;
	}
	
//tempInt=-SIN_Vin;
//tempInt=tempInt*10.0+10.0;
//DAC_BUF[3]=tempInt/2.0;
	
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA0=(unsigned short)DAC_BUF[0];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA1=(unsigned short)DAC_BUF[1];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA2=(unsigned short)DAC_BUF[2];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA3=(unsigned short)DAC_BUF[3];
}



