//###########################################################################
// FILE: SstParams.c
// FUNCTION: 30x34固定组地址访问及参数写入权限；不拥有映射表、遥测或存储状态。
//           命令策略归SstCommand，参数表归SstParameterTable，存储归SstParameterStore。
//###########################################################################
#include "SstParams.h"
#include "SstParameterTable.h"
#include "SstParameterStore.h"
#include "SstCommand.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstFault.h"
/*==================================================================
 * 函数功能: 读取控制状态中的 10 ms 节拍
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 32 位 10 ms 计数，允许自然回绕
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint32_t SstControl_GetTick10ms(void) { return (uint32_t)g_chb_dab_status.tick_10ms; }

/*==================================================================
 * 函数功能: 查询 AnyEyes 与 DPRAM 共用参数存储服务的忙状态
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 1 表示存储正在进行，0 表示空闲
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_IsStorageBusy(void)
{ return SstParameterStore_IsBusy(); }

/*==================================================================
 * 函数功能: 同时检查状态、启动命令和两类 PWM 的停机条件
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 1 表示可以执行停机专属操作，0 表示不可
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_IsStopped(void)
{ return SstCommand_IsStopped(); }

/*==================================================================
 * 函数功能: 检查串口控制权限及存储互锁后提交 CHB/DAB 启停命令
 * 输入参数: enable：启停命令，0 停止、1 启动
 * 输出参数: g_chb_dab_command.run_enable、dab_enable；PWM 由控制核心执行。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_RunCommand(uint16_t enable)
{ return SstCommand_RunSerial(enable); }

/*==================================================================
 * 函数功能: 无条件撤销 CHB 和 DAB 启动命令，由控制核心执行停机
 * 输入参数: 无
 * 输出参数: run_enable、dab_enable 清零；原保护/控制路径完成关断。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParams_EmergencyStop(void)
{ SstCommand_EmergencyStop(); }

/*==================================================================
 * 函数功能: 按固定组号及组内地址读取参数、显示量、历史或存储状态
 * 输入参数: group：固定参数组号，范围 0..29
 *           index：所选表/对象的索引；范围按函数对应表容量检查
 *           value：输出指针，用于返回固定地址对应的 16 位线上参数编码
 * 输出参数: value 指向的 16 位线上编码；空指针返回地址错误。
 * 返回值: SST_PARAM_OK 或 SST_PARAM_BAD_ADDRESS
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_Read(uint16_t group,uint16_t index,uint16_t *value)
{
 if(!value || group>=SST_PARAM_GROUPS || index>=SST_PARAM_PER_GROUP) return SST_PARAM_BAD_ADDRESS;
 *value=0;
 if(group==0) {
  if(index<8) *value=SstParameterTable_ReadCommand(index);
  else if(index==8) *value=g_chb_dab_protection_enable;
  return SST_PARAM_OK;
 }
 if(group==1) { *value=SstParameterTable_ReadParameter(index);return SST_PARAM_OK; }
 if(group==2 && index<3) { *value=SstParameterTable_ReadParameter((uint16_t)(34+index));return SST_PARAM_OK; }
 if(group>=3 && group<=8) { SstDisplay_Refresh();*value=SstDisplay_Read((uint16_t)((group-3)*34+index));return SST_PARAM_OK; }
 if(group==19) {
  *value=SstFault_GetLegacyHistoryWord(index);
  return SST_PARAM_OK;
 }
 if(group==29) {
  if(index==0) *value=SstParameterStore_GetResult();
  if(index==1) *value=SstParameterStore_IsPending();
  if(index==2) *value=1; /* SST mapping version */
  if(index==3) *value=SST_ALLOW_SERIAL_COMMANDS;
  if(index==4) *value=SST_DEBUG_PROTOCOL;
 }
 return SST_PARAM_OK; /* reserved addresses read zero, never alias arbitrary RAM */
}

/*==================================================================
 * 函数功能: 校验地址、权限、范围及停机互锁后修改参数或提交存储请求
 * 输入参数: group：固定参数组号，范围 0..29
 *           index：所选表/对象的索引；范围按函数对应表容量检查
 *           value：待写入的 16 位线上参数编码，有符号量按二补码解释
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_Write(uint16_t group,uint16_t index,uint16_t value)
{
 uint16_t n; /* n：组号与索引换算后的 parameters 表下标。 */
 if(group>=SST_PARAM_GROUPS || index>=SST_PARAM_PER_GROUP) return SST_PARAM_BAD_ADDRESS;
 if(group==29 && index<2) {
  if(value!=0xA55A) return SST_PARAM_RANGE;
  return SstParameterStore_Request(index);
 }
 if(SstParams_IsStorageBusy() && !(group==0 && index<2 && value==0)) return SST_PARAM_BUSY;
 if(group==0 && (index==0 || index==1 || index==3)) {
  return SstCommand_WriteSerialField(index,value);
 }
 if(group==0 && index<9) {
  if(!SstParams_IsStopped()) return SST_PARAM_BUSY;
  if(index==8) {
   if(value>1) return SST_PARAM_RANGE;
   g_chb_dab_protection_enable=value;return SST_PARAM_OK;
  }
  if(index==4 && (float)value>g_chb_dab_params.modulation_max*10000.0F) return SST_PARAM_RANGE;
  return SstParameterTable_WriteCommand(index,value);
 }
 if(group==1 || (group==2 && index<3)) {
  if(!SstParams_IsStopped()) return SST_PARAM_BUSY;
  n=(uint16_t)((group-1)*34+index);
  if(n==0) return SST_PARAM_READ_ONLY; /* physical TAD timing remains board-defined */
  if(n==1 && (float)value<g_chb_dab_command.open_loop_modulation*10000.0F) return SST_PARAM_RANGE;
  if(n==27 && value!=1 && value!=0xFFFF) return SST_PARAM_RANGE;
  if(n==33 && value>g_chb_dab_params.dab_target_duty_count) return SST_PARAM_RANGE;
  if(n==34 && value<g_chb_dab_params.dab_start_duty_count) return SST_PARAM_RANGE;
  return SstParameterTable_WriteParameter(n,value);
 }
 return SST_PARAM_READ_ONLY;
}

/*==================================================================
 * 函数功能: 每 10 ms 至多推进一次单字节写入或读回校验，最后提交镜像有效标记
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParams_Service(void)
{ SstParameterStore_Service(); }

/*==================================================================
 * 函数功能: 读取最近一次参数保存/加载的结果
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: SST_PARAM_* 结果码
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_GetStorageResult(void) { return SstParameterStore_GetResult(); }

/*==================================================================
 * 函数功能: 在初始化阶段尝试恢复参数并记录存储结果
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParameters_LoadPersistent(void) { SstParameterStore_LoadPersistent(); }

