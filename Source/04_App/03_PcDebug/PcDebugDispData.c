/*======================================== 函数实现 =====================================*/

/*======================================================================================*
 * 文件名  : PcDebugDispData.c
 * 功能说明: 维护固定显示表、浮点遥测通道与故障状态映射，不拥有控制参数。
 *=====================================================================================*/

/*======================================== 文件包含 =====================================*/
#include "SstControlConfig.h"
#include "MeasurementData.h"
#include "PortStatusData.h"

#include "PcDebugDispData.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstFault.h"
#include "SstParameterTable.h"
#include "SstParameterStore.h"

/*======================================== 私有变量 =====================================*/
static uint16_t display[SST_DISPLAY_COUNT]; /* 固定 200 字显示快照，静态初始化为 0。 */

/*==================================================================
 * 函数名称: Scale
 * 函数功能: 将物理量缩放、限幅并四舍五入为显示表的 16 位编码。
 * 输入参数: value：待编码的物理量；factor：物理量到显示整数的乘数。
 *           signedValue：0 按无符号范围限幅，非 0 按有符号范围限幅。
 * 输出参数: 无
 * 返回值  : 缩放后的 16 位字；有符号值按补码表示，NaN 返回 0。
 *================================================================*/
static uint16_t Scale(float value, float factor, uint16_t signedValue)
{
    float v = value * factor; /* 缩放后的待编码值，用于限幅和四舍五入。 */
    if (v != v)
        return 0;
    if (v > (signedValue ? 32767.0F : 65535.0F))
        v = signedValue ? 32767.0F : 65535.0F;
    if (v < (signedValue ? -32768.0F : 0.0F))
        v = signedValue ? -32768.0F : 0.0F;
    return (uint16_t)(int32_t)(v + (v < 0 ? -0.5F : 0.5F));
}

/*==================================================================
 * 函数名称: SstTelemetry_ReadFloat
 * 函数功能: 按固定通道号读取未经整数缩放的浮点遥测值。
 * 输入参数: index：0/1/2 为 A/B/C 相电压，单位 V；3/4/5 为 A/B/C 相电流，单位 A。
 *           6 为模块直流平均电压，单位 V；7 为端口状态码；8 为 PLL 角度，单位 rad。
 * 输出参数: 无
 * 返回值  : 通道对应的浮点数；index 不在 0～8 时返回 0。
 * 调用说明: 后台波形服务调用；状态码转为浮点数，不改变其枚举含义。
 *================================================================*/
float SstTelemetry_ReadFloat(uint16_t index)
{
    switch (index)
    {
        case 0:
            return PORT_RealTime_DATA.PORT_V_A;
        case 1:
            return HA_Port_Volt_B;
        case 2:
            return HA_Port_Volt_C;
        case 3:
            return HA_Port_Current_A;
        case 4:
            return HA_Port_Current_B;
        case 5:
            return HA_Port_Current_C;
        case 6:
            return g_chb_dab_status.dc_voltage_average_v;
        case 7:
            return (float)Port_State;
        case 8:
            return g_chb_dab_status.pll_angle_rad;
        default:
            return 0;
    }
}

/*==================================================================
 * 函数名称: SstDisplay_Refresh
 * 函数功能: 更新固定 200 字显示表，供 AnyEyes 波形、面板和功能码读取。
 * 输入参数: 无
 * 输出参数: 文件内 display 数组的已绑定项；未绑定项保持原值。
 * 返回值  : 无
 * 调用说明: 只更新显示快照；不写入控制命令或控制参数，不在 10 kHz ISR 内调用。
 *================================================================*/
void SstDisplay_Refresh(void)
{
    uint16_t i; /* 显示表分组的通道、命令、参数或故障条目索引。 */
    /* 0～28：运行状态、启停命令镜像及启动过程遥测。 */
    display[0] = g_chb_dab_status.state;
    display[1] = g_chb_dab_status.fault_code;
    display[2] = SstFault_GetActiveCount();
    display[3] = g_chb_dab_command.run_enable;
    display[4] = g_chb_dab_command.dab_enable;
    display[5] = g_chb_dab_command.ac_mode;
    display[6] = g_chb_dab_status.ac_input_closed;
    display[7] = g_chb_dab_status.precharge_bypassed;
    display[8] = g_chb_dab_status.ac_pwm_enabled;
    display[9] = g_chb_dab_status.dab_pwm_enabled;
    display[10] = g_chb_dab_status.pll_locked;
    display[11] = g_chb_dab_status.dab_duty_count;
    display[12] = (uint16_t)g_chb_dab_status.tick_10ms;
    display[13] = (uint16_t)(g_chb_dab_status.tick_10ms >> 16);
    display[14] = Scale(g_chb_dab_status.dc_voltage_average_v, 10, 0);
    display[15] = Scale(g_chb_dab_status.dc_voltage_min_v, 10, 0);
    display[16] = Scale(g_chb_dab_status.pll_angle_rad, 10000, 0);
    display[17] = Scale(g_chb_dab_status.pll_frequency_hz, 100, 0);
    display[18] = Scale(g_chb_dab_status.id_feedback_a, 100, 1);
    display[19] = Scale(g_chb_dab_status.iq_feedback_a, 100, 1);
    display[20] = Scale(g_chb_dab_status.id_reference_a, 100, 1);
    display[21] = Scale(g_chb_dab_status.startup_id_reference_a, 100, 1);
    display[22] = Scale(g_chb_dab_status.startup_voltage_reference_v, 10, 0);
    display[23] = g_chb_dab_status.startup_phase;
    display[24] = (uint16_t)g_chb_dab_status.startup_tad_count;
    display[25] = (uint16_t)(g_chb_dab_status.startup_tad_count >> 16);
    display[26] = g_chb_dab_status.voltage_loop_active;
    display[27] = Scale(g_chb_dab_status.modulation_actual, 10000, 0);
    display[28] = g_chb_dab_protection_enable;
    /* 30～35：三相电压/电流，均按 0.1 V/0.1 A 的有符号字编码。 */
    for (i = 1; i < 6; ++i)
        display[30 + i] = Scale(SstTelemetry_ReadFloat(i), 10, 1);
    display[30] = Scale(HA_Port_Volt_A, 10, 1);
    /* 40～47：控制命令镜像；100～136：参数表镜像。 */
    for (i = 0; i < 8; ++i)
        display[40 + i] = SstParameterTable_ReadCommand(i);
    for (i = 0; i < 37; ++i)
        display[100 + i] = SstParameterTable_ReadParameter(i);
    /* 79～89：历史/当前故障数量、等级和故障码。 */
    display[79] = SstFault_GetHistoryLevel();
    display[80] = SstFault_GetActiveCount();
    for (i = 0; i < 6; ++i)
        display[81 + i] = SstFault_GetCode(i);
    display[87] = SstFault_GetLevel();
    display[88] = SstFault_GetHistoryCount();
    display[89] = SstFault_GetFirstCode();
    /* 190～194：映射版本、存储结果及调试通信配置。 */
    display[190] = 1; /* SST 显示点表版本。 */
    display[191] = SstParameterStore_GetResult();
    display[192] = SstParameterStore_IsPending();
    display[193] = SST_ALLOW_SERIAL_COMMANDS;
    display[194] = SST_DEBUG_PROTOCOL;
}

/*==================================================================
 * 函数名称: SstDisplay_Read
 * 函数功能: 读取显示表指定索引的一个 16 位字。
 * 输入参数: index：显示表索引，有效范围 0～SST_DISPLAY_COUNT-1。
 * 输出参数: 无
 * 返回值  : 对应显示字；索引越界返回 0。
 * 调用说明: 读取当前快照；需要最新数据时先调用 SstDisplay_Refresh。
 *================================================================*/
uint16_t SstDisplay_Read(uint16_t index)
{
    return index < SST_DISPLAY_COUNT ? display[index] : 0;
}
