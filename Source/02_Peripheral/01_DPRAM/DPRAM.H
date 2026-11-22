/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef DPRAM_H_
#define DPRAM_H_

#include "SstTypes.h"
#ifdef __cplusplus
extern "C"
{
#endif
//###########################################################################
//
// FILE:   DPRAM.h
//
// AUTHOR: Wen Wusong;  Modified by Z.W
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

/**************************Macro definition*********************************/
//Block length
/* DPRAMBlock1_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock1_Length 16
/* DPRAMBlock2_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock2_Length 43
/* DPRAMBlock3_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock3_Length 33
/* DPRAMBlock4_PluginUnit_Length：原 DPRAMBlock4 插件硬件编号，寄存器数组使用编号减 2。 */
#define DPRAMBlock4_PluginUnit_Length 128

//mailbox
/* DPRAM_Maibox_Left：DPRAM 左邮箱原地址；读访问会应答对端通知。 */
#define DPRAM_Maibox_Left 0x66003FFC
/* DPRAM_Maibox_Right：DPRAM 右邮箱原地址；写访问用于通知对端。 */
#define DPRAM_Maibox_Right 0x66003FFE

    /*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: PORT_Regulate_DATA、旧控制系数和对应 FPGA 配置；Inital_Y=0 时保存遥调缓存。
 * 返回值: 无
 * 执行说明: 按 HVAC/HVDC/LVDC 等编译分支选择旧端口的遥调处理。
 *================================================================*/
    extern void RemoteRegulate_Dispose(uchar Inital_Y);
    /*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：0 执行接收后的完成处理，非零仅应用参数；HVAC 的 _2 接口不自动保存
 * 输出参数: 更新端口参数、旧控制系数及启用的 FPGA 配置；Inital_Y=0 时置遥调完成标志。
 * 返回值: 无
 * 执行说明: 当前主循环只在遥调获准且已停机、存储空闲时调用。
 *================================================================*/
    extern void RemoteRegulate_Dispose_2(uchar Inital_Y);

#ifdef __cplusplus
}
#endif
#endif
