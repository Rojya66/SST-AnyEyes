/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef DPRAM_H_
#define DPRAM_H_

#include "SstTypes.h"
#ifdef __cplusplus
extern "C" {
#endif
//###########################################################################
//
// FILE:   DPRAM.h
//
// AUTHOR: Wen Wusong
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.24
//###########################################################################

/**************************Macro definition*********************************/
//Block length
/* DPRAMBlock1_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock1_Length    	16
/* DPRAMBlock2_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock2_Length    	43
/* DPRAMBlock3_Length：对应 DPRAM 块的连续 16 位字数，保持原布局。 */
#define DPRAMBlock3_Length    	33
/* DPRAMBlock4_PluginUnit_Length：原 DPRAMBlock4 插件硬件编号，寄存器数组使用编号减 2。 */
#define DPRAMBlock4_PluginUnit_Length  128

//mailbox
/* DPRAM_Maibox_Left：DPRAM 左邮箱原地址；读访问会应答对端通知。 */
#define DPRAM_Maibox_Left  0x66003FFC
/* DPRAM_Maibox_Right：DPRAM 右邮箱原地址；写访问用于通知对端。 */
#define DPRAM_Maibox_Right 0x66003FFE

/***********************Function declaration*********************************/
/*==================================================================
 * 函数功能: 按 16 位字复制指定长度的内存数据
 * 输入参数: SourceAddr：源地址，连续数据至少覆盖要求的复制长度
 *           length：待复制的 16 位字数，源/目标容量均须满足
 *           DestAddr：输出目标地址，调用者须提供足够容量
 * 输出参数: DestAddr 指向的连续 length 个 16 位字，调用者保证容量及有效地址。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void MemDataCopy(volatile ushort *SourceAddr, ushort length, volatile ushort *DestAddr);
/*==================================================================
 * 函数功能: 把指定遥测、插件或遥调缓存回写到 DPRAM 并通知对端
 * 输入参数: Flag_R：WriteBlock_* DPRAM 回写块号
 * 输出参数: 目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void DPRAM_Write(ushort Flag_R);
/*==================================================================
 * 函数功能: 读取 DPRAM 新块标识以及对应遥控或遥调块
 * 输入参数: 无
 * 输出参数: 目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void DPRAM_Read(void);
/*==================================================================
 * 函数功能: 分派 DPRAM 缓存中的遥控、遥调数据处理
 * 输入参数: 无
 * 输出参数: 目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void DPRAM_ReadDataDispose(void);
/*==================================================================
 * 函数功能: 按旧版遥控块处理启动、停止、脉冲、开关及复位控制
 * 输入参数: 无
 * 输出参数: 目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void RemoteControl_Dispose(void);
/*==================================================================
 * 函数功能: 把遥调块定点值换算为浮点给定及控制器参数
 * 输入参数: Inital_Y：旧遥调参数来源/初始化选择值，沿用调用点约定
 * 输出参数: 目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void RemoteRegulate_Dispose(uchar Inital_Y);
/*==================================================================
 * 函数功能: 按所选来源更新端口遥调参数、模块配置及控制系数
 * 输入参数: Inital_Y：旧遥调参数来源/初始化选择值，沿用调用点约定
 * 输出参数: 目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void RemoteRegulate_Dispose_2(uchar Inital_Y);
/*==================================================================
 * 函数功能: 把端口浮点遥测及状态换算为 DPRAM 16 位上报缓存
 * 输入参数: 无
 * 输出参数: 目标缓冲区、遥控标志或遥测缓存；地址及字数沿用原 DPRAM 布局。
 * 返回值: 无
 * 执行说明: 旧版 DPRAM 兼容路径；硬件访问由 SST_ENABLE_DPRAM 门控，迁移主流程使用 SstCommand 统一启停。
 *================================================================*/
extern void RemoteSig_Dispose(void);




#ifdef __cplusplus
}
#endif
#endif
