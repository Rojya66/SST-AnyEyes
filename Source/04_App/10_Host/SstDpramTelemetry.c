//###########################################################################
// FILE: SstDpramTelemetry.c
// FUNCTION: 原块3显式编码，隔离DSP内部复用故障位和Word外部位定义。
//###########################################################################
#include "SstDpramProtocol.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "MeasurementData.h"
#include "ProtectionData.h"
#include "PortStatusData.h"
#include "ControlData.h"
#include "FpgaIo.h"
#include "SstBoardConfig.h"

/*==================================================================
 * 函数功能: 给出配置中有真实生产者的插件镜像位图
 * 输入参数: 无
 * 输出参数: 无
 * 返回值: bit0..15对应U2..U17；不代表光链路实时健康
 * 执行说明: 后台配置能力查询，U7/11/15输出卡和U17录波卡不冒充采样镜像。
 *================================================================*/
uint16_t SstDpramTelemetry_GetPluginMask(void)
{
    uint16_t mask; /* 按配置确认的数据生产者位图。 */
    mask = 0x0006U; /* U3采样板、U4主控板。 */
    if (SST_ENABLE_SWITCH_OUTPUT || SST_ENABLE_SWITCH_FEEDBACK)
        mask |= 0x0001U;
    if (NHA_module_A)
        mask |= 0x0018U; /* U5/U6。 */
#if (SST_PHASE_A_ONLY_TEST == 0U)
    if (NHA_module_B)
        mask |= 0x0180U; /* U9/U10。 */
    if (NHA_module_C)
        mask |= 0x1800U; /* U13/U14。 */
#endif
    return mask;
}

/*==================================================================
 * 函数功能: 工程量转换为有符号16位编码，越界饱和而不回绕
 * 输入参数: value：已乘线上比例的工程量；NaN编码0
 * 输出参数: 无
 * 返回值: int16二补码
 * 执行说明: 后台遥测专用，不能用于参数写入的静默截断。
 *================================================================*/
static uint16_t EncodeTelemetrySigned(float value)
{
    if (value != value)
        return 0U;
    if (value > 32767.0F)
        value = 32767.0F;
    if (value < -32768.0F)
        value = -32768.0F;
    return (uint16_t)(int16_t)(value + (value < 0.0F ? -0.5F : 0.5F));
}

/*==================================================================
 * 函数功能: 原内部故障来源转换为Word PORT_FAULT；内部位域不改
 * 输入参数: 无
 * 输出参数: 无
 * 返回值: Word16位故障字；bit13通信、bit14同步，bit15任一实际故障
 * 执行说明: 只读锁存结果，不重复判断阈值，不清除任何故障。
 *================================================================*/
uint16_t SstDpramTelemetry_GetFaultWord(void)
{
    uint16_t port; /* 内部故障原字，bit13/14在HVAC复用为软件OV/OC。 */
    uint16_t type; /* 故障分类原字，通信bit12、同步bit11。 */
    uint16_t fault; /* 按Word语义重新构造的外部故障字。 */
    port = *(volatile unsigned short *)&Port_RealFault.PORT_FAULT16;
    type = *(volatile unsigned short *)&FaultTypeFLAG.FaultTypeFLAG16;
    fault = port & 0x1FFFU;
    if (type & 0x0100U)
        fault |= 0x0001U;
    if (type & 0x0200U)
        fault |= 0x0002U;
    if (type & 0x0400U)
        fault |= 0x0008U;
    if (type & 0xE0F0U)
        fault |= 0x0040U;
    if (type & 0x000CU)
        fault |= 0x0080U;
    if ((port & 0x4000U) || (type & 0x0002U) || HVAC_OC_Fault ||
        g_chb_dab_status.fault_code == CHB_FAULT_AC_OVERCURRENT)
        fault |= 0x0400U;
    if (type & 0x4000U)
        fault |= 0x0800U;
    if ((port & 0x2000U) || (type & 0x0001U) || HVAC_OV_Fault ||
        g_chb_dab_status.fault_code == CHB_FAULT_DC_OVERVOLTAGE)
        fault |= 0x1000U;
    if (type & 0x1000U)
        fault |= 0x2000U;
    if (type & 0x0800U)
        fault |= 0x4000U;
    if (fault || port || type || g_chb_dab_status.fault_code != CHB_FAULT_NONE ||
        g_chb_dab_status.state == CHB_STATE_FAULT)
        fault |= 0x8000U;
    return fault;
}

static uint16_t ReadModuleFaultPositions(uint16_t input_plugin)
{
    uint16_t enabled = Port_EffectiveFOEnable(input_plugin);

    return enabled ? (Port_FOStatusRead(input_plugin, 1U) & enabled) : 0U;
}

/* 将当前状态和量测打包为原 33 字遥测。后台每次最多读取三块 701 的硬件锁存。 */
void SstDpramTelemetry_BuildLegacyBlock(uint16_t *words)
{
    uint16_t index; /* 清零/字段索引。 */
    uint16_t state; /* Word低6位运行状态。 */
    uint16_t status; /* Word开关/PWM/系统方式字。 */
    uint16_t system_mode; /* 0自动运行、1自动测试、2手动运行、3手动测试。 */
    for (index = 0U; index < 33U; ++index)
        words[index] = 0U;
    switch (g_chb_dab_status.state)
    {
        case CHB_STATE_STOPPED:
            state = 0x07U;
            break;
        case CHB_STATE_PRECHARGE:
            state = 0x02U;
            break;
        case CHB_STATE_KT1_SETTLE:
            state = 0x03U;
            break;
        case CHB_STATE_WAIT_PLL:
            state = 0x05U;
            break;
        case CHB_STATE_RUNNING:
            state = 0x20U;
            break;
        default:
            state = 0x08U;
            break;
    }
    words[0] = state | (Port_Outer_Elec ? 0x0080U : 0U);
    system_mode = (PortRunFLAG.Bbits.Port_AutoOrManual ? 0U : 2U) | (PortRunFLAG.Bbits.Port_RunOrTest ? 0U : 1U);
    status = (uint16_t)(system_mode << 13U);
    /* 无真实反馈时，触点合/分都为0(未知)，不拿命令或接地/隔离位替代。 */
#if SST_ENABLE_SWITCH_FEEDBACK
    /* 0501 bit0=KT1合位、bit1=QF1合位、bit2=QF1分位，均来自各自实际触点。
     * QF1动作中合/分位可能都为0，不能用合位取反来伪造分位。 */
    status |= g_chb_dab_status.kt1_closed;
    status |= (uint16_t)(g_chb_dab_status.qf1_closed << 1U);
    status |= (uint16_t)(g_chb_dab_status.qf1_open << 2U);
#endif
    if ((g_chb_dab_status.ac_pwm_enabled == 0U) &&
        (g_chb_dab_status.dab_pwm_enabled == 0U))
        status |= 0x0080U;
    if (g_chb_dab_status.ac_pwm_enabled == 0U)
        status |= 0x0100U;
    if (g_chb_dab_status.dab_pwm_enabled == 0U)
        status |= 0x0200U;
    if (g_sst_dpram_screen_result.switch_permission)
        status |= 0x0400U;
    words[1] = status;
    words[2] = SstDpramTelemetry_GetFaultWord();
    words[3] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_P * 10.0F); /* 0.1kW/count。 */
    words[4] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_Q * 10.0F); /* 0.1kvar/count。 */
    words[5] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_V_A);
    words[8] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_I_A * 10.0F);
#if (SST_PHASE_A_ONLY_TEST == 0U)
    words[6] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_V_B);
    words[7] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_V_C);
    words[9] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_I_B * 10.0F);
    words[10] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_I_C * 10.0F);
#endif
    /* DC_V冻结为A相串联总母线电压；模块均压另见补充区0x1994。 */
    words[18] = EncodeTelemetrySigned(SVG_Udc_A_Ave * (float)NHA_module_A);
    words[25] = EncodeTelemetrySigned(PORT_RealTime_DATA.PORT_PF * 100.0F);
    /* 原F0字段属于下垂，保持0；实际PLL频率在补充区，不偷换字段语义。 */

    /* 051E/051F/0520 保留 A/B/C 相 701 的物理位号。即使 FB 未使能或处于停机，
     * 也使用与 701 硬件一致的有效 FO 掩码报告已锁存位置。 */
    words[30] = ReadModuleFaultPositions(Input_A_PluginUnit_8);
    words[31] = ReadModuleFaultPositions(Input_B_PluginUnit_12);
    words[32] = ReadModuleFaultPositions(Input_C_PluginUnit_16);
}
