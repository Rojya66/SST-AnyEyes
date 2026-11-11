//###########################################################################
// FILE: SstDpramParameter.c
// FUNCTION: 原块2和普通Modbus参数区接入实际控制参数，保留负PI及未改项精度。
//###########################################################################
#include "SstDpramProtocol.h"
#include "SstParameterTable.h"
#include "SstLegacyParameters.h"
#include "SstParams.h"
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "ProtectionData.h"
#include "PortStatusData.h"
#include "FpgaIo.h"

SST_DPRAM_SCREEN_RESULT g_sst_dpram_screen_result; /* 仅后台写；遥测层负责只读发布。 */

/*==================================================================
 * 函数功能: 读取57项实际参数编码，符号/比例与唯一参数表一致
 * 输入参数: words：至少57字的RAM缓冲区
 * 输出参数: words；PI先按有符号量缩放，再编码为16位二补码
 * 返回值: 无
 * 执行说明: 主循环调用；不访问DPRAM，不改变控制参数。
 *================================================================*/
void SstDpramParameter_Read(uint16_t *words)
{
    uint16_t index; /* 参数字序号。 */
    for (index = 0U; index < 42U; ++index)
        words[index] = SstParameterTable_SnapshotRead(index);
    for (index = 0U; index < 8U; ++index)
        words[42U + index] = SstLegacyParameters_Read(index);
    words[50] = g_chb_dab_protection_enable;
    /* bit0=1运行/0测试；bit1=1自动/0手动，不映射成ac_mode。 */
    words[51] = (PortRunFLAG.Bbits.Port_RunOrTest ? 1U : 0U) | (PortRunFLAG.Bbits.Port_AutoOrManual ? 2U : 0U);
    words[52] = Port_AD_PORT_EN;
    words[53] = Port_FO_1_EN;
    words[54] = Port_FO_2_EN;
    words[55] = Port_FO_3_EN;
    words[56] = Port_FB_EN;
}

/*==================================================================
 * 函数功能: 停机后先整体校验，再只提交变化项，避免未修改浮点PI被量化
 * 输入参数: words：57字完整候选，顺序见Read；error_word输出到执行结果
 * 输出参数: 成功才更新活动对象及实际硬件保护使能；不自动保存EEPROM
 * 返回值: SST_PARAM_*；不支持/越界/忙时活动值不变
 * 执行说明: 仅后台，所有校验在首次写入前完成；停机ISR不会运行PI。
 *================================================================*/
uint16_t SstDpramParameter_Apply(const uint16_t *words)
{
    uint16_t result; /* 整体校验结果。 */
    uint16_t index; /* 变化项索引。 */
    uint16_t actual[SST_SCREEN_PARAMETERS]; /* 提交前实际编码，兼顾保留原浮点精度。 */
    g_sst_dpram_screen_result.error_word = 0xFFFFU;
#if !SST_ALLOW_REMOTE_COMMANDS || !SST_ENABLE_DPRAM
    (void)words;
    return SST_PARAM_DENIED;
#endif
    if (!SstParams_IsStopped() || SstParams_IsStorageBusy())
        return SST_PARAM_BUSY;
    if (words[0] != SstParameterTable_SnapshotRead(0U))
    {
        g_sst_dpram_screen_result.error_word = 0U;
        return SST_PARAM_READ_ONLY;
    }
    result = SstParameterTable_SnapshotValidate(words);
    if (result != SST_PARAM_OK)
        return result;
    result = SstLegacyParameters_ValidateSnapshot(&words[42]);
    if (result != SST_PARAM_OK)
        return result;
    /* 不接受保留位；FO保留bit15，AD仅支持文档定义的高四位。 */
    for (index = 50U; index < 56U; ++index)
    {
        uint16_t mask; /* 此字段允许的位集合。 */
        mask = index == 50U ? 1U : (index == 51U ? 3U : (index == 52U ? 0xF000U : 0x7FFFU));
        if (words[index] & (uint16_t)~mask)
        {
            g_sst_dpram_screen_result.error_word = index;
            return SST_PARAM_RANGE;
        }
    }
    SstDpramParameter_Read(actual);
    SstParameterTable_SnapshotApplyChanged(words);
    for (index = 42U; index < 50U; ++index)
    {
        if (words[index] != actual[index])
        {
            SstLegacyParameters_ApplySnapshot(&words[42]);
            break;
        }
    }
    g_chb_dab_protection_enable = words[50];
    PortRunFLAG.Bbits.Port_RunOrTest = words[51] & 1U;
    PortRunFLAG.Bbits.Port_AutoOrManual = (words[51] >> 1U) & 1U;
    if (actual[52] != words[52])
    {
        Port_AD_PORT_EN = words[52];
        Port_ADProtect_EN();
    }
    if (actual[53] != words[53] || actual[54] != words[54] || actual[55] != words[55])
    {
        Port_FO_1_EN = words[53];
        Port_FO_2_EN = words[54];
        Port_FO_3_EN = words[55];
        Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
    }
    if (actual[56] != words[56])
    {
        Port_FB_EN = words[56];
        Port_FB_EN_Set();
    }
    return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 原43字遥调块回读实际生效值，未支持字段固定0
 * 输入参数: words：43字RAM输出；PI比例分别1e5、100、1000、1000
 * 输出参数: 原地址的实际编码；V_SET=1V/count，F_SET=0.1Hz/count
 * 返回值: 无
 * 执行说明: 后台；负PI直接沿用唯一参数表二补码，无abs或额外反号。
 *================================================================*/
void SstDpramParameter_ReadLegacy(uint16_t *words)
{
    uint16_t index; /* 旧块字序号。 */
    uint16_t actual[SST_SCREEN_PARAMETERS]; /* 唯一实际参数镜像。 */
    SstDpramParameter_Read(actual);
    for (index = 0U; index < 43U; ++index)
        words[index] = 0U;
    words[0] = actual[51];
    for (index = 1U; index < 6U; ++index)
        words[index] = actual[51U + index];
    words[8] = (uint16_t)(g_chb_dab_command.dc_voltage_ref_v + 0.5F);
    words[9] =
        (uint16_t)((g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP ? g_chb_dab_command.open_loop_frequency_hz
                                                                       : g_chb_dab_params.pll_nominal_frequency_hz) *
                       10.0F +
                   0.5F);
    words[27] = actual[15];
    words[28] = actual[16];
    words[29] = actual[24];
    words[30] = actual[25];
}

/*==================================================================
 * 函数功能: 旧块2接入当前Udc/F/组1PI和保护使能，整块拒绝未支持的更改
 * 输入参数: words：旧43字候选；建议先读实际块，再改所需字段、最后写邮箱2
 * 输出参数: 成功才应用；未改V/F保持其原小数，错误字段号供屏幕查询
 * 返回值: SST_PARAM_*或SST_SCREEN_UNSUPPORTED
 * 执行说明: 不恢复旧缩放因子算法；PI组2/3、PQ/下垂/谐波尚无控制对象。
 *================================================================*/
uint16_t SstDpramParameter_ApplyLegacy(const uint16_t *words)
{
    uint16_t index; /* 旧字段号。 */
    uint16_t legacy[43]; /* 提交前旧块实际值，用于识别变化。 */
    uint16_t candidate[SST_SCREEN_PARAMETERS]; /* 完整候选，不直接写控制成员。 */
    SstDpramParameter_Read(candidate);
    SstDpramParameter_ReadLegacy(legacy);
    for (index = 0U; index < 43U; ++index)
    {
        if (words[index] == legacy[index])
            continue;
        g_sst_dpram_screen_result.error_word = index;
        if (index == 0U)
        {
            if (words[index] > 3U)
                return SST_SCREEN_UNSUPPORTED; /* 本机只支持HVAC两位。 */
            candidate[51] = words[index];
        }
        else if (index < 6U)
            candidate[51U + index] = words[index];
        else if (index == 8U)
        {
            if ((int16_t)words[index] <= 0 || words[index] > 6553U)
                return SST_PARAM_RANGE;
            candidate[40] = (uint16_t)(words[index] * 10U);
        }
        else if (index == 9U)
        {
            if ((int16_t)words[index] <= 0 || words[index] > 1000U)
                return SST_PARAM_RANGE;
            candidate[g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP ? 39U : 7U] = (uint16_t)(words[index] * 10U);
        }
        else if (index >= 27U && index <= 30U)
            candidate[index < 29U ? index - 12U : index - 5U] = words[index];
        else
            return SST_SCREEN_UNSUPPORTED;
    }
    return SstDpramParameter_Apply(candidate);
}
