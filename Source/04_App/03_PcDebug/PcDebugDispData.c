#include "SstControlConfig.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
//###########################################################################
// FILE: PcDebugDispData.c
// FUNCTION: 固定200字显示表、VOFA浮点遥测和故障状态映射。
//           不改变参考PcDebug的参数/波形/故障地址，不拥有控制参数。
//###########################################################################
#include "PcDebugDispData.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstFault.h"
#include "SstParameterTable.h"
#include "SstParameterStore.h"
static uint16_t display[SST_DISPLAY_COUNT]; /* display：固定200字上位机显示表，未绑定地址维持0。 */
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
 for(i=0;i<8;++i) display[40+i]=SstParameterTable_ReadCommand(i);
 for(i=0;i<37;++i) display[100+i]=SstParameterTable_ReadParameter(i);
 display[79]=SstFault_GetHistoryLevel();
 display[80]=SstFault_GetActiveCount();
 for(i=0;i<6;++i) display[81+i]=SstFault_GetCode(i);
 display[87]=SstFault_GetLevel();
 display[88]=SstFault_GetHistoryCount();
 display[89]=SstFault_GetFirstCode();
 display[190]=1; /* SST mapping version */
 display[191]=SstParameterStore_GetResult();
 display[192]=SstParameterStore_IsPending();
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

