#include <stdint.h>
#include <string.h>
#include "main.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstFault.h"
#include "SstParams.h"
#include "EEProm.H"
#if SST_ENABLE_MAINTENANCE
#include "MaintenanceProtocol.H"
#endif

typedef struct
{
    volatile void *address; /* address：被映射控制成员的地址，只允许表内已绑定的对象。 */
    float scale; /* scale：物理值到 16 位线上值的乘数，写入时使用其倒数。 */
    int32_t minimum,maximum; /* minimum：允许的最小值；映射表中按线上整数范围定义。 */
    /* maximum：允许的最大值；映射表中按线上整数范围定义。 */
    uint16_t isSigned,isWord; /* isSigned：1 表示线上值按有符号 16 位二补码解释。 */
    /* isWord：1 表示目标是 unsigned short，0 表示 float。 */
} SST_PARAMETER;
static const SST_PARAMETER commands[]={
 { (volatile void *)&g_chb_dab_command.run_enable, 1.0F, 0L, 1L, 0U, 1U },
 { (volatile void *)&g_chb_dab_command.dab_enable, 1.0F, 0L, 1L, 0U, 1U },
 { (volatile void *)&g_chb_dab_command.ac_mode, 1.0F, 0L, 1L, 0U, 1U },
 { (volatile void *)&g_chb_dab_command.fault_reset, 1.0F, 0L, 1L, 0U, 1U },
 { (volatile void *)&g_chb_dab_command.open_loop_modulation, 10000.0F, 0L, 9800L, 0U, 0U },
 { (volatile void *)&g_chb_dab_command.open_loop_frequency_hz, 100.0F, 100L, 10000L, 0U, 0U },
 { (volatile void *)&g_chb_dab_command.dc_voltage_ref_v, 10.0F, 1L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_command.iq_reference_a, 100.0F, -32768L, 32767L, 1U, 0U },
}; /* commands：组 0 的八个控制命令/给定映射，顺序即固定组内地址。 */
static const SST_PARAMETER parameters[]={
 { (volatile void *)&g_chb_dab_params.sample_period_s, 10000000.0F, 1L, 10000L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.modulation_max, 10000.0F, 1L, 9800L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.precharge_voltage_v, 10.0F, 0L, 30000L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.precharge_stable_10ms, 1.0F, 1L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.precharge_timeout_10ms, 1.0F, 1L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.bypass_delay_10ms, 1.0F, 0L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.breaker_pulse_10ms, 1.0F, 1L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.pll_nominal_frequency_hz, 100.0F, 100L, 10000L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.pll_kp, 10.0F, 0L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.pll_ki, 1.0F, 0L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.pll_correction_limit_rad_s, 100.0F, 0L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.pll_lock_error_pu, 10000.0F, 1L, 10000L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.pll_min_grid_peak_v, 10.0F, 0L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.pll_lock_10ms, 1.0F, 1L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.pll_timeout_10ms, 1.0F, 1L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.voltage_kp, 100000.0F, -32768L, 32767L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.voltage_ki, 100.0F, -32768L, 32767L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.voltage_current_limit_a, 100.0F, 1L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.startup_strategy, 1.0F, 0L, 1L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.startup_udc_offset_v, 100.0F, -32768L, 32767L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.startup_udc_step_v, 1000.0F, 1L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.startup_id_reference_a, 100.0F, -32768L, 32767L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.startup_id_ramp_step_a, 10000.0F, 1L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.startup_ramp_tad, 1.0F, 1L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.current_kp, 1000.0F, -32768L, 32767L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.current_ki, 1000.0F, -32768L, 32767L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.closed_loop_voltage_limit_factor, 10000.0F, 1L, 30000L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.current_feedback_sign, 1.0F, -1L, 1L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.current_phase_compensation_rad, 10000.0F, -31415L, 31415L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.output_phase_compensation_rad, 10000.0F, -31415L, 31415L, 1U, 0U },
 { (volatile void *)&g_chb_dab_params.ac_overcurrent_trip_a, 100.0F, 1L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.dc_overvoltage_trip_v, 10.0F, 1L, 65535L, 0U, 0U },
 { (volatile void *)&g_chb_dab_params.protection_confirm_samples, 1.0F, 1L, 65535L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.dab_start_duty_count, 1.0F, 0L, 2500L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.dab_target_duty_count, 1.0F, 0L, 2500L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.dab_ramp_step_count, 1.0F, 1L, 2500L, 0U, 1U },
 { (volatile void *)&g_chb_dab_params.dab_start_count, 1.0F, 0L, 4999L, 0U, 1U },
}; /* parameters：37 个控制参数的固定地址、比例及范围映射。 */
static uint16_t display[SST_DISPLAY_COUNT]; /* display：200 字显示表，索引与参考面板及波形地址保持一致。 */
static uint16_t storageResult,storagePending,storagePosition; /* storageResult：最近一次参数保存/加载的 SST_PARAM_* 结果。 */
/* storagePending：异步参数保存正在执行的标志。 */
/* storagePosition：参数镜像写入/提交/读回校验所处的步序。 */
static uint32_t storageTick; /* storageTick：上次推进参数存储任务的 10 ms 节拍。 */
/* SST_STORE_LENGTH：页 6/7 参数镜像总长度，92 字节。 */
#define SST_STORE_LENGTH 92U
static uint8_t storageImage[SST_STORE_LENGTH]; /* storageImage：页 6/7 的 92 字节参数镜像，包含魔数、版本及 CRC。 */
/*==================================================================
 * 函数功能: 读取映射参数并按比例、符号和 16 位范围饱和编码
 * 输入参数: p：已经绑定地址、缩放及范围的 SST_PARAMETER 映射条目
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 16 位线上值，有符号参数使用二补码
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t ReadEntry(const SST_PARAMETER *p)
{
 float value; /* value：物理成员读取值及缩放后的浮点线上值。 */
    int32_t raw; /* raw：参数物理值缩放后的有符号整数中间值。 */
 value=p->isWord?(float)*(volatile unsigned short *)p->address:*(volatile float *)p->address;
 value*=p->scale;
 if(value!=value) return 0;
 if(value>(p->isSigned?32767.0F:65535.0F)) value=p->isSigned?32767.0F:65535.0F;
 if(value<(p->isSigned?-32768.0F:0.0F)) value=p->isSigned?-32768.0F:0.0F;
 raw=(int32_t)(value+(value<0?-0.5F:0.5F));
 return (uint16_t)raw;
}
/*==================================================================
 * 函数功能: 检查线上值范围后，反比例换算并写入映射成员
 * 输入参数: p：已经绑定地址、缩放及范围的 SST_PARAMETER 映射条目
 *           value：待写入的 16 位线上参数编码，有符号量按二补码解释
 * 输出参数: 仅更新 p->address 指定的受控成员。
 * 返回值: SST_PARAM_OK 或 SST_PARAM_RANGE
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t WriteEntry(const SST_PARAMETER *p,uint16_t value)
{
 int32_t raw=p->isSigned?(int32_t)(int16_t)value:(int32_t)value; /* raw：参数物理值缩放后的有符号整数中间值。 */
 if(raw<p->minimum || raw>p->maximum) return SST_PARAM_RANGE;
 if(p->isWord) *(volatile unsigned short *)p->address=(unsigned short)raw;
 else *(volatile float *)p->address=(float)raw/p->scale;
 return SST_PARAM_OK;
}
/*==================================================================
 * 函数功能: 读取控制状态中的 10 ms 节拍
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 32 位 10 ms 计数，允许自然回绕
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint32_t SstControl_GetTick10ms(void) { return (uint32_t)g_chb_dab_status.tick_10ms; }
/*==================================================================
 * 函数功能: 查询 AnyEyes 与可选维护协议共享的存储忙状态
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 1 表示存储正在进行，0 表示空闲
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_IsStorageBusy(void)
{
#if SST_ENABLE_MAINTENANCE
 return (uint16_t)(storagePending || MaintenanceProtocol_IsSaving());
#else
 return storagePending;
#endif
}
/*==================================================================
 * 函数功能: 同时检查状态、启动命令和两类 PWM 的停机条件
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 1 表示可以执行停机专属操作，0 表示不可
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_IsStopped(void)
{
 return (uint16_t)((g_chb_dab_status.state==CHB_STATE_STOPPED || g_chb_dab_status.state==CHB_STATE_FAULT) &&
  !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable &&
  !g_chb_dab_status.ac_pwm_enabled && !g_chb_dab_status.dab_pwm_enabled);
}
/*==================================================================
 * 函数功能: 检查串口控制权限及存储互锁后提交 CHB/DAB 启停命令
 * 输入参数: enable：启停命令，0 停止、1 启动
 * 输出参数: g_chb_dab_command.run_enable、dab_enable；PWM 由控制核心执行。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_RunCommand(uint16_t enable)
{
#if SST_ALLOW_SERIAL_COMMANDS
 if(enable>1) return SST_PARAM_RANGE;
 if(enable && SstParams_IsStorageBusy()) return SST_PARAM_BUSY;
 g_chb_dab_command.run_enable=enable;
 g_chb_dab_command.dab_enable=enable;
 return SST_PARAM_OK;
#else
 (void)enable; return SST_PARAM_DENIED;
#endif
}
/*==================================================================
 * 函数功能: 无条件撤销 CHB 和 DAB 启动命令，由控制核心执行停机
 * 输入参数: 无
 * 输出参数: run_enable、dab_enable 清零；原保护/控制路径完成关断。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParams_EmergencyStop(void)
{
 /* Stop is unconditional; simulator/DPRAM ownership cannot block safe shutdown. */
 g_chb_dab_command.run_enable=0;
 g_chb_dab_command.dab_enable=0;
}
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
  if(index<8) *value=ReadEntry(&commands[index]);
  else if(index==8) *value=g_chb_dab_protection_enable;
  return SST_PARAM_OK;
 }
 if(group==1) { *value=ReadEntry(&parameters[index]);return SST_PARAM_OK; }
 if(group==2 && index<3) { *value=ReadEntry(&parameters[34+index]);return SST_PARAM_OK; }
 if(group>=3 && group<=8) { SstDisplay_Refresh();*value=SstDisplay_Read((uint16_t)((group-3)*34+index));return SST_PARAM_OK; }
 if(group==19) {
  *value=SstFault_GetLegacyHistoryWord(index);
  return SST_PARAM_OK;
 }
 if(group==29) {
  if(index==0) *value=storageResult;
  if(index==1) *value=storagePending;
  if(index==2) *value=1; /* SST mapping version */
  if(index==3) *value=SST_ALLOW_SERIAL_COMMANDS;
  if(index==4) *value=SST_DEBUG_PROTOCOL;
 }
 return SST_PARAM_OK; /* reserved addresses read zero, never alias arbitrary RAM */
}
/*==================================================================
 * 函数功能: 在停机且存储空闲时执行加载，或建立异步保存镜像
 * 输入参数: load：0 请求保存，非零请求加载
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreRequest(uint16_t load);
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
  return StoreRequest(index);
 }
 if(SstParams_IsStorageBusy() && !(group==0 && index<2 && value==0)) return SST_PARAM_BUSY;
 if(group==0 && (index==0 || index==1 || index==3)) {
#if SST_ALLOW_SERIAL_COMMANDS
  if(value>1) return SST_PARAM_RANGE;
  if(index==3 && !SstParams_IsStopped()) return SST_PARAM_BUSY;
  return WriteEntry(&commands[index],value);
#else
  return SST_PARAM_DENIED;
#endif
 }
 if(group==0 && index<9) {
  if(!SstParams_IsStopped()) return SST_PARAM_BUSY;
  if(index==8) {
   if(value>1) return SST_PARAM_RANGE;
   g_chb_dab_protection_enable=value;return SST_PARAM_OK;
  }
  if(index==4 && (float)value>g_chb_dab_params.modulation_max*10000.0F) return SST_PARAM_RANGE;
  return WriteEntry(&commands[index],value);
 }
 if(group==1 || (group==2 && index<3)) {
  if(!SstParams_IsStopped()) return SST_PARAM_BUSY;
  n=(uint16_t)((group-1)*34+index);
  if(n==0) return SST_PARAM_READ_ONLY; /* physical TAD timing remains board-defined */
  if(n==1 && (float)value<g_chb_dab_command.open_loop_modulation*10000.0F) return SST_PARAM_RANGE;
  if(n==27 && value!=1 && value!=0xFFFF) return SST_PARAM_RANGE;
  if(n==33 && value>g_chb_dab_params.dab_target_duty_count) return SST_PARAM_RANGE;
  if(n==34 && value<g_chb_dab_params.dab_start_duty_count) return SST_PARAM_RANGE;
  return WriteEntry(&parameters[n],value);
 }
 return SST_PARAM_READ_ONLY;
}
/*==================================================================
 * 函数功能: 把物理浮点值按指定比例饱和、四舍五入为 16 位线上值
 * 输入参数: value：本次协议/存储处理的值，类型和编码保持原接口约定
 *           factor：物理值到线上整数的缩放乘数
 *           signedValue：非零按有符号 16 位编码，0 按无符号编码
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 16 位编码值，NaN 返回 0
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t Scale(float value,float factor,uint16_t signedValue)
{
 float v=value*factor; /* v：物理量乘比例后的浮点线上值。 */
 if(v!=v) return 0;
 if(v>(signedValue?32767.0F:65535.0F)) v=signedValue?32767.0F:65535.0F;
 if(v<(signedValue?-32768.0F:0.0F)) v=signedValue?-32768.0F:0.0F;
 return (uint16_t)(int32_t)(v+(v<0?-0.5F:0.5F));
}
/*==================================================================
 * 函数功能: 按固定索引读取 VOFA 的原始浮点遥测量
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 对应物理量；未知索引返回 0
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
float SstTelemetry_ReadFloat(uint16_t index)
{
 switch(index) {
 case 0:return PORT_RealTime_DATA.PORT_V_A;case 1:return HA_Port_Volt_B;case 2:return HA_Port_Volt_C;
 case 3:return HA_Port_Current_A;case 4:return HA_Port_Current_B;case 5:return HA_Port_Current_C;
 case 6:return g_chb_dab_status.dc_voltage_average_v;
 case 7:return (float)Port_State;
 case 8:return g_chb_dab_status.pll_angle_rad;
 default:return 0;
 }
}
/*==================================================================
 * 函数功能: 刷新固定 200 字显示表中的状态、波形、参数及故障报告位置
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstDisplay_Refresh(void)
{
 uint16_t i; /* i：本次数组、参数或模块遍历的索引。 */
 display[0]=g_chb_dab_status.state;
 display[1]=g_chb_dab_status.fault_code;
 display[2]=SstFault_GetActiveCount();
 display[3]=g_chb_dab_command.run_enable;
 display[4]=g_chb_dab_command.dab_enable;
 display[5]=g_chb_dab_command.ac_mode;
 display[6]=g_chb_dab_status.ac_input_closed;
 display[7]=g_chb_dab_status.precharge_bypassed;
 display[8]=g_chb_dab_status.ac_pwm_enabled;
 display[9]=g_chb_dab_status.dab_pwm_enabled;
 display[10]=g_chb_dab_status.pll_locked;
 display[11]=g_chb_dab_status.dab_duty_count;
 display[12]=(uint16_t)g_chb_dab_status.tick_10ms;
 display[13]=(uint16_t)(g_chb_dab_status.tick_10ms>>16);
 display[14]=Scale(g_chb_dab_status.dc_voltage_average_v,10,0);
 display[15]=Scale(g_chb_dab_status.dc_voltage_min_v,10,0);
 display[16]=Scale(g_chb_dab_status.pll_angle_rad,10000,0);
 display[17]=Scale(g_chb_dab_status.pll_frequency_hz,100,0);
 display[18]=Scale(g_chb_dab_status.id_feedback_a,100,1);
 display[19]=Scale(g_chb_dab_status.iq_feedback_a,100,1);
 display[20]=Scale(g_chb_dab_status.id_reference_a,100,1);
 display[21]=Scale(g_chb_dab_status.startup_id_reference_a,100,1);
 display[22]=Scale(g_chb_dab_status.startup_voltage_reference_v,10,0);
 display[23]=g_chb_dab_status.startup_phase;
 display[24]=(uint16_t)g_chb_dab_status.startup_tad_count;
 display[25]=(uint16_t)(g_chb_dab_status.startup_tad_count>>16);
 display[26]=g_chb_dab_status.voltage_loop_active;
 display[27]=Scale(g_chb_dab_status.modulation_actual,10000,0);
 display[28]=g_chb_dab_protection_enable;
 for(i=1;i<6;++i) display[30+i]=Scale(SstTelemetry_ReadFloat(i),10,1);
 display[30]=Scale(HA_Port_Volt_A,10,1);
 for(i=0;i<8;++i) display[40+i]=ReadEntry(&commands[i]);
 for(i=0;i<37;++i) display[100+i]=ReadEntry(&parameters[i]);
 display[79]=SstFault_GetHistoryLevel();
 display[80]=SstFault_GetActiveCount();
 for(i=0;i<6;++i) display[81+i]=SstFault_GetCode(i);
 display[87]=SstFault_GetLevel();
 display[88]=SstFault_GetHistoryCount();
 display[89]=SstFault_GetFirstCode();
 display[190]=1; /* SST mapping version */
 display[191]=storageResult;
 display[192]=storagePending;
 display[193]=SST_ALLOW_SERIAL_COMMANDS;
 display[194]=SST_DEBUG_PROTOCOL;
}
/*==================================================================
 * 函数功能: 读取固定显示表的一个 16 位字
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 表内值；越界返回 0
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstDisplay_Read(uint16_t index)
{
 return index<SST_DISPLAY_COUNT?display[index]:0;
}
/*==================================================================
 * 函数功能: 计算参数存储镜像的 CRC16，初值 FFFF、多项式 A001
 * 输入参数: data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           count：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 16 位 CRC 校验值
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreCrc(const uint8_t *data,uint16_t count)
{
 uint16_t crc=0xFFFF,i,b; /* crc：逐字节、逐位累加的 CRC 校验状态。 */
 /* i：本次字节/位循环序号。 */
 /* b：CRC 位循环序号，或三量比较的第二个输入。 */
 for(i=0;i<count;++i) { crc^=data[i];for(b=0;b<8;++b) crc=(crc&1)?(crc>>1)^0xA001:crc>>1; }
 return crc;
}
/*==================================================================
 * 函数功能: 按小端顺序把一个 16 位字写入 storageImage
 * 输入参数: offset：对象中的字偏移或镜像字节偏移，按接口说明解释
 *           value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: storageImage 中 offset、offset+1 两字节。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static void StorePut(uint16_t offset,uint16_t value)
{
 storageImage[offset]=(uint8_t)value;storageImage[offset+1]=(uint8_t)(value>>8);
}
/*==================================================================
 * 函数功能: 按小端顺序从 storageImage 读取一个 16 位字
 * 输入参数: offset：对象中的字偏移或镜像字节偏移，按接口说明解释
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 镜像中的 16 位字
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreGet(uint16_t offset)
{
 return (uint16_t)storageImage[offset]|((uint16_t)storageImage[offset+1]<<8);
}
/*==================================================================
 * 函数功能: 停机时读取页 6/7 的 92 字节镜像，整体校验后恢复可持久化参数
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreLoad(void)
{
 uint16_t i,n,word,result; /* i：本次数组、参数或模块遍历的索引。 */
 /* n：持久化的五个命令给定所对应的 commands 下标。 */
 /* word：镜像解码出的参数线上字。 */
 /* result：已整体校验参数的写入结果。 */
 if(!SstParams_IsStopped()) return SST_PARAM_BUSY;
 for(i=0;i<SST_STORE_LENGTH;++i)
  storageImage[i]=r_eeprom_byte_read((uint8_t)(6+i/64),(uint8_t)(i%64));
 if(storageImage[0]!=0x53 || storageImage[1]!=0x54 || storageImage[2]!=1 ||
    storageImage[3]!=SST_STORE_LENGTH || StoreGet(90)!=StoreCrc(storageImage,90))
  return SST_PARAM_STORAGE;
 /* Validate the whole snapshot before changing any live member. */
 for(i=0;i<42;++i) {
  const SST_PARAMETER *p=i<37?&parameters[i]:&commands[i==37?2:i-34]; /* p：本次校验的参数映射条目。 */
  int32_t raw; /* raw：参数物理值缩放后的有符号整数中间值。 */
  word=StoreGet((uint16_t)(6+2*i));raw=p->isSigned?(int32_t)(int16_t)word:(int32_t)word;
  if(raw<p->minimum || raw>p->maximum) return SST_PARAM_RANGE;
 }
 if(StoreGet(6+2*27)!=1 && StoreGet(6+2*27)!=0xFFFF) return SST_PARAM_RANGE;
 if(StoreGet(6+2*33)>StoreGet(6+2*34)) return SST_PARAM_RANGE;
 if(StoreGet(6+2*38)>StoreGet(6+2*1)) return SST_PARAM_RANGE;
 /* Sample period must match the configured hardware timing. */
 if(StoreGet(6)!=ReadEntry(&parameters[0])) return SST_PARAM_RANGE;
 for(i=1;i<37;++i) { result=WriteEntry(&parameters[i],StoreGet((uint16_t)(6+2*i)));(void)result; }
 for(i=37;i<42;++i) {
  n=i==37?2:i-34;WriteEntry(&commands[n],StoreGet((uint16_t)(6+2*i)));
 }
 return SST_PARAM_OK;
}
/*==================================================================
 * 函数功能: 在停机且存储空闲时执行加载，或建立异步保存镜像
 * 输入参数: load：0 请求保存，非零请求加载
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreRequest(uint16_t load)
{
 uint16_t i,n; /* i：本次数组、参数或模块遍历的索引。 */
 /* n：需要保存的命令给定所对应的 commands 下标。 */
 if(SstParams_IsStorageBusy() || !SstParams_IsStopped()) return SST_PARAM_BUSY;
 if(load) { storageResult=StoreLoad();return storageResult; }
 memset(storageImage,0,sizeof(storageImage));
 storageImage[0]=0x53;storageImage[1]=0x54;storageImage[2]=1;storageImage[3]=SST_STORE_LENGTH;
 for(i=0;i<37;++i) StorePut((uint16_t)(6+2*i),ReadEntry(&parameters[i]));
 for(i=37;i<42;++i) { n=i==37?2:i-34;StorePut((uint16_t)(6+2*i),ReadEntry(&commands[n])); }
 StorePut(90,StoreCrc(storageImage,90));
 storagePending=1;storagePosition=0;storageResult=SST_PARAM_BUSY;
 storageTick=SstControl_GetTick10ms()-1;
 return SST_PARAM_OK;
}
/*==================================================================
 * 函数功能: 每 10 ms 至多推进一次单字节写入或读回校验，最后提交镜像有效标记
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParams_Service(void)
{
 uint32_t now=SstControl_GetTick10ms(); /* now：本轮读取的 10 ms 控制节拍，用于后台超时或逐拍服务。 */uint16_t offset; /* offset：本拍写入或读回的镜像字节偏移。 */uint8_t value,ok; /* value：本拍写入 EEPROM 的单字节值。 */
     /* ok：EEPROM 单字节写入的 VALIDFLAG/INVALIDFLAG 结果。 */
 if(!storagePending || now==storageTick) return;
 storageTick=now;
 if(!SstParams_IsStopped()) { storagePending=0;storageResult=SST_PARAM_BUSY;return; }
 /* Invalidate magic first, commit it last. At most one byte per 10ms tick;
    no global interrupt mask and no EEPROM write-cycle wait in this service. */
 if(storagePosition==0) { offset=0;value=0; }
 else if(storagePosition<SST_STORE_LENGTH) { offset=storagePosition;value=storageImage[offset]; }
 else if(storagePosition==SST_STORE_LENGTH) { offset=0;value=storageImage[0]; }
 else {
  offset=(uint16_t)(storagePosition-SST_STORE_LENGTH-1);
  if(r_eeprom_byte_read((uint8_t)(6+offset/64),(uint8_t)(offset%64))!=storageImage[offset]) {
   storagePending=0;storageResult=SST_PARAM_STORAGE;return;
  }
  ++storagePosition;
  if(offset==SST_STORE_LENGTH-1) { storagePending=0;storageResult=SST_PARAM_OK; }
  return;
 }
 ok=r_eeprom_byte_write((uint8_t)(6+offset/64),(uint8_t)(offset%64),value);
 if(ok!=VALIDFLAG) { storagePending=0;storageResult=SST_PARAM_STORAGE;return; }
 ++storagePosition;
}
/*==================================================================
 * 函数功能: 读取最近一次参数保存/加载的结果
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: SST_PARAM_* 结果码
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParams_GetStorageResult(void) { return storageResult; }
/*==================================================================
 * 函数功能: 在初始化阶段尝试恢复参数并记录存储结果
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParameters_LoadPersistent(void) { storageResult=StoreLoad(); }

