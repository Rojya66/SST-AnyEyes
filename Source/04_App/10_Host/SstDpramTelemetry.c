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

/*==================================================================
 * 函数功能: 给出配置中有真实生产者的插件镜像位图
 * 输入参数: 无
 * 输出参数: 无
 * 返回值: bit0..15对应U2..U17；不代表光链路实时健康
 * 执行说明: 后台配置能力查询，U7/11/15输出卡和U17录波卡不冒充采样镜像。
 *================================================================*/
uint16_t SstDpramTelemetry_PluginMask(void)
{
    uint16_t mask; /* 按配置确认的数据生产者位图。 */
    mask = 0x0006U; /* U3采样板、U4主控板。 */
    if (SST_ENABLE_SWITCH_OUTPUT || SST_ENABLE_SWITCH_FEEDBACK)
        mask |= 0x0001U;
    if (NHA_module_A)
        mask |= 0x0018U; /* U5/U6。 */
#if !SST_PHASE_A_ONLY_TEST
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
static uint16_t EncodeSigned(float value)
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
uint16_t SstDpramTelemetry_Fault(void)
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

/*==================================================================
 * 函数功能: 将当前核心状态和实际量测打包为旧33字遥测
 * 输入参数: words：33字RAM输出区
 * 输出参数: 按V1.1位定义、有效值和比例编码；未支持通道固定0并另报有效性
 * 返回值: 无
 * 执行说明: 后台近实时镜像，各标量不是同一TAD采样的原子快照。
 *================================================================*/
void SstDpramTelemetry_Build(uint16_t *words)
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
        case CHB_STATE_BYPASS_DELAY:
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
    status |= g_chb_dab_status.bypass_feedback ? 1U : 0U;
    status |= g_chb_dab_status.ac_input_feedback ? 2U : 4U;
#endif
    if (!g_chb_dab_status.ac_pwm_enabled && !g_chb_dab_status.dab_pwm_enabled)
        status |= 0x0080U;
    if (!g_chb_dab_status.ac_pwm_enabled)
        status |= 0x0100U;
    if (!g_chb_dab_status.dab_pwm_enabled)
        status |= 0x0200U;
    if (g_sst_dpram_screen_result.switch_permission)
        status |= 0x0400U;
    words[1] = status;
    words[2] = SstDpramTelemetry_Fault();
    words[3] = EncodeSigned(PORT_RealTime_DATA.PORT_P * 10.0F); /* 0.1kW/count。 */
    words[4] = EncodeSigned(PORT_RealTime_DATA.PORT_Q * 10.0F); /* 0.1kvar/count。 */
    words[5] = EncodeSigned(PORT_RealTime_DATA.PORT_V_A);
    words[8] = EncodeSigned(PORT_RealTime_DATA.PORT_I_A * 10.0F);
#if !SST_PHASE_A_ONLY_TEST
    words[6] = EncodeSigned(PORT_RealTime_DATA.PORT_V_B);
    words[7] = EncodeSigned(PORT_RealTime_DATA.PORT_V_C);
    words[9] = EncodeSigned(PORT_RealTime_DATA.PORT_I_B * 10.0F);
    words[10] = EncodeSigned(PORT_RealTime_DATA.PORT_I_C * 10.0F);
#endif
    /* DC_V冻结为A相串联总母线电压；模块均压另见补充区0x1994。 */
    words[18] = EncodeSigned(SVG_Udc_A_Ave * (float)NHA_module_A);
    words[25] = EncodeSigned(PORT_RealTime_DATA.PORT_PF * 100.0F);
    /* 原F0字段属于下垂，保持0；实际PLL频率在补充区，不偷换字段语义。 */
}
