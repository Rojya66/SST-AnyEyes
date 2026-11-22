//###########################################################################
// FILE: SstDpramParameter.c
// FUNCTION: Word原43字遥调直接赋值；57项扩展参数区独立处理，地址和比例不变。
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
 * 函数功能: 读取Word V1.1原43字遥调块的实际参数
 * 输入参数: words：43字RAM输出，word号=Modbus地址-0x01FF
 * 输出参数: 系统方式、保护使能、Udc、频率、组1电压/电流PI；未接入项回读0
 * 返回值: 无
 * 执行说明: 不经过57项扩展区，不修改任何参数；PI只借用参数表的舍入编码。
 *================================================================*/
void SstDpramParameter_EncodeLegacyBlock(uint16_t *words)
{
    uint16_t index; /* 原遥调块字号，0..42。 */
    float frequency_hz; /* 开环输出频率或闭环PLL额定频率，单位Hz。 */

    for (index = 0U; index < 43U; ++index)
    {
        words[index] = 0U;
    }

    /* 01FF：bit0运行/测试，bit1自动/手动；这些管理位不是启停命令。 */
    words[0] = (uint16_t)(PortRunFLAG.Bbits.Port_RunOrTest |
                         (PortRunFLAG.Bbits.Port_AutoOrManual << 1U));
    words[1] = Port_AD_PORT_EN; /* 0200：AD保护使能。 */
    words[2] = Port_FO_1_EN;    /* 0201：光纤1组保护使能。 */
    words[3] = Port_FO_2_EN;    /* 0202：光纤2组保护使能。 */
    words[4] = Port_FO_3_EN;    /* 0203：光纤3组保护使能。 */
    words[5] = Port_FB_EN;      /* 0204：插件反馈使能。 */
    words[8] = (uint16_t)(g_chb_dab_command.dc_voltage_ref_v + 0.5F); /* 0207：1V/count。 */

    if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
    {
        frequency_hz = g_chb_dab_command.open_loop_frequency_hz;
    }
    else
    {
        frequency_hz = g_chb_dab_params.pll_nominal_frequency_hz;
    }
    words[9] = (uint16_t)(frequency_hz * 10.0F + 0.5F); /* 0208：0.1Hz/count。 */

    /* 参数表在这里只负责按既定比例舍入、编码负数，没有43转57或间接赋值。 */
    words[27] = SstParameterTable_ReadParameter(15U); /* 021A：voltage_kp * 100000。 */
    words[28] = SstParameterTable_ReadParameter(16U); /* 021B：voltage_ki * 100。 */
    words[29] = SstParameterTable_ReadParameter(24U); /* 021C：current_kp * 1000。 */
    words[30] = SstParameterTable_ReadParameter(25U); /* 021D：current_ki * 1000。 */
}

/*==================================================================
 * 函数功能: 按Word原43字直接修改系统方式、保护使能、Udc、频率及组1PI
 * 输入参数: words：邮箱2收到的43字RAM副本，word号=Modbus地址-0x01FF
 * 输出参数: 下方明写的活动成员及FPGA保护使能；不修改启停或DAB使能
 * 返回值: SST_PARAM_*；有效但未实现的功能返回SST_SCREEN_UNSUPPORTED
 * 执行说明: 先检查停机/存储状态和本块字段，再直接赋值；不构造57项候选。
 *           保留位、word23..26忽略；未变化浮点值保留原精度；不自动保存。
 *================================================================*/
uint16_t SstDpramParameter_DecodeAndApplyLegacyBlock(const uint16_t *words)
{
    uint16_t actual[43]; /* 本次修改前的原协议编码；比较变化以免未修改PI被量化。 */
    uint16_t index; /* 未接入字段的原协议字号，错误时供屏幕定位。 */
    uint16_t ad_enable; /* 0200：只取AD保护高四位。 */
    uint16_t fo1_enable; /* 0201：光纤1组有效低15位。 */
    uint16_t fo2_enable; /* 0202：光纤2组有效低15位。 */
    uint16_t fo3_enable; /* 0203：光纤3组有效低15位。 */
    uint16_t fo_changed; /* 任一FO配置变化时重算对应701的汇总FB使能。 */

    g_sst_dpram_screen_result.error_word = 0xFFFFU;
#if (SST_ALLOW_REMOTE_COMMANDS == 0U) || (SST_ENABLE_DPRAM == 0U)
    (void)words;
    return SST_PARAM_DENIED;
#endif
    /* 1. PI等参数只在两路命令/PWM均停止、EEPROM空闲时修改。 */
    if ((SstParams_IsStopped() == 0U) || (SstParams_IsStorageBusy() != 0U))
    {
        return SST_PARAM_BUSY;
    }
    SstDpramParameter_EncodeLegacyBlock(actual);

    /* 2. 只检查原协议中本次接入的内容，不再校验无关PLL/启动/DAB/保护参数。
     * 01FF的bit2..7属于其他端口管理方式，尚无本机对象；高8位保留并忽略。 */
    if ((words[0] & 0x00FCU) != 0U)
    {
        g_sst_dpram_screen_result.error_word = 0U;
        return SST_SCREEN_UNSUPPORTED;
    }
    /* P/Q、谐波/下垂、组2/3 PI及EEPROM扩展没有本机控制对象。
     * 这些有效数值非零时明确报不支持；word23..26是保留字，不参与检查。 */
    for (index = 6U; index < 43U; ++index)
    {
        if ((index == 6U) || (index == 7U) ||
            ((index >= 10U) && (index <= 22U)) || (index >= 31U))
        {
            if (words[index] != 0U)
            {
                g_sst_dpram_screen_result.error_word = index;
                return SST_SCREEN_UNSUPPORTED;
            }
        }
    }
    /* 与既有参数接口保持同一范围：Udc为1..6553V，频率为1..100Hz。
     * 只检查变化项，未改字段不因回读的整数精度而重写原float。 */
    if ((words[8] != actual[8]) && ((words[8] == 0U) || (words[8] > 6553U)))
    {
        g_sst_dpram_screen_result.error_word = 8U;
        return SST_PARAM_RANGE;
    }
    if ((words[9] != actual[9]) && ((words[9] < 10U) || (words[9] > 1000U)))
    {
        g_sst_dpram_screen_result.error_word = 9U;
        return SST_PARAM_RANGE;
    }

    /* 3. 01FF的管理位直接赋值；不会改变run_enable、dab_enable或ac_mode。 */
    PortRunFLAG.Bbits.Port_RunOrTest = words[0] & 0x0001U;
    PortRunFLAG.Bbits.Port_AutoOrManual = (words[0] >> 1U) & 0x0001U;

    /* 4. 0200..0204先屏蔽保留位，再更新保护使能并调用真实FPGA写接口。 */
    ad_enable = words[1] & 0xF000U;
    fo1_enable = words[2] & 0x7FFFU;
    fo2_enable = words[3] & 0x7FFFU;
    fo3_enable = words[4] & 0x7FFFU;
    if (ad_enable != Port_AD_PORT_EN)
    {
        Port_AD_PORT_EN = ad_enable;
        Port_ADProtect_EN();
    }
    fo_changed = (fo1_enable != Port_FO_1_EN) || (fo2_enable != Port_FO_2_EN) || (fo3_enable != Port_FO_3_EN);
    if (fo_changed != 0U)
    {
        Port_FO_1_EN = fo1_enable;
        Port_FO_2_EN = fo2_enable;
        Port_FO_3_EN = fo3_enable;
        Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
    }
    if ((fo_changed != 0U) || (words[5] != Port_FB_EN))
    {
        Port_FB_EN = words[5]; /* FB的16位均为有效位。 */
        Port_FB_EN_Set();
    }

    /* 5. 0207直接作为单模块Udc目标；0208除10得到Hz，无中间十倍转换。 */
    if (words[8] != actual[8])
    {
        g_chb_dab_command.dc_voltage_ref_v = (float)words[8];
    }
    if (words[9] != actual[9])
    {
        if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
        {
            g_chb_dab_command.open_loop_frequency_hz = (float)words[9] / 10.0F;
        }
        else
        {
            g_chb_dab_params.pll_nominal_frequency_hz = (float)words[9] / 10.0F;
        }
    }

    /* 6. 021A..021D就是PI的最终赋值位置；int16解码负数，再除协议比例。
     * d/q共用current_kp/ki；Ki仍为连续积分增益，控制器执行时乘Ts一次。
     * 例：021C=0xFB1E -> int16为-1250 -> current_kp=-1.25，不额外反号。 */
    if (words[27] != actual[27])
    {
        g_chb_dab_params.voltage_kp = (float)(int16_t)words[27] / 100000.0F;
    }
    if (words[28] != actual[28])
    {
        g_chb_dab_params.voltage_ki = (float)(int16_t)words[28] / 100.0F;
    }
    if (words[29] != actual[29])
    {
        g_chb_dab_params.current_kp = (float)(int16_t)words[29] / 1000.0F;
    }
    if (words[30] != actual[30])
    {
        g_chb_dab_params.current_ki = (float)(int16_t)words[30] / 1000.0F;
    }
    return SST_PARAM_OK;
}

/* 以下仅供0x1A00待应用区的57项扩展事务使用，与上方原43字遥调独立。 */
/*==================================================================
 * 函数功能: 读取57项实际参数编码，符号/比例与唯一参数表一致
 * 输入参数: words：至少57字的RAM缓冲区
 * 输出参数: words；PI先按有符号量缩放，再编码为16位二补码
 * 返回值: 无
 * 执行说明: 主循环调用；不访问DPRAM，不改变控制参数。
 *================================================================*/
void SstDpramParameter_EncodeParameterSnapshot(uint16_t *words)
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
 * 输入参数: words：57字完整候选，顺序见SstDpramParameter_EncodeParameterSnapshot；error_word输出到执行结果
 * 输出参数: 成功才更新活动对象及实际硬件保护使能；不自动保存EEPROM
 * 返回值: SST_PARAM_*；不支持/越界/忙时活动值不变
 * 执行说明: 仅后台，所有校验在首次写入前完成；停机ISR不会运行PI。
 *================================================================*/
uint16_t SstDpramParameter_ApplyParameterSnapshot(const uint16_t *words)
{
    uint16_t result; /* 整体校验结果。 */
    uint16_t index; /* 变化项索引。 */
    uint16_t fo_changed; /* 任一FO配置变化时重算对应701的汇总FB使能。 */
    uint16_t actual[SST_SCREEN_PARAMETERS]; /* 提交前实际编码，兼顾保留原浮点精度。 */
    g_sst_dpram_screen_result.error_word = 0xFFFFU;
#if (SST_ALLOW_REMOTE_COMMANDS == 0U) || (SST_ENABLE_DPRAM == 0U)
    (void)words;
    return SST_PARAM_DENIED;
#endif
    /* 提交第1步：两路命令与实际PWM均已停止、存储空闲，才允许整组遥调。
     * words[0]为硬件100us采样周期，必须保持实际值，不能借遥调改变10kHz。 */
    if ((SstParams_IsStopped() == 0U) || (SstParams_IsStorageBusy() != 0U))
        return SST_PARAM_BUSY;
    if (words[0] != SstParameterTable_SnapshotRead(0U))
    {
        g_sst_dpram_screen_result.error_word = 0U;
        return SST_PARAM_READ_ONLY;
    }
    /* 提交第2步：检查0..41唯一参数、42..49原保护，以及50..56运行期配置。
     * 所有检查在第一次写活动成员前完成，失败不会只改一半参数。
     * 本函数只接收57项扩展区；原43字遥调由DecodeAndApplyLegacyBlock直接处理。 */
    result = SstParameterTable_SnapshotValidate(words);
    if (result != SST_PARAM_OK)
        return result;
    result = SstLegacyParameters_ValidateSnapshot(&words[42]);
    if (result != SST_PARAM_OK)
        return result;
    /* 扩展区各字段自己的范围；原43字的保留位在原协议入口直接屏蔽。 */
    for (index = 50U; index < 56U; ++index)
    {
        if (((index == 50U) && (words[index] > 1U)) ||
            ((index == 51U) && (words[index] > 3U)) ||
            ((index == 52U) && ((words[index] & 0x0FFFU) != 0U)) ||
            ((index >= 53U) && ((words[index] & 0x8000U) != 0U)))
        {
            g_sst_dpram_screen_result.error_word = index;
            return SST_PARAM_RANGE;
        }
    }
    /* 提交第3步：比较实际编码，只写发生变化的0..41项。
     * 最终写入在SstParameterTable.c的SnapshotApplyChanged -> WriteEntry内。
     * 15/16 -> voltage_kp/ki；24/25 -> current_kp/ki；40 -> dc_voltage_ref_v。
     * 负PI经int16解码、除比例后直接写float，不取绝对值、不额外反号。
     * 快照不含run_enable、dab_enable、fault_reset，因此遥调不能触发启动。 */
    SstDpramParameter_EncodeParameterSnapshot(actual);
    SstParameterTable_SnapshotApplyChanged(words);
    /* 提交第4步：42..49对应原软件OC/OV、预充巡检阈值；有变化才整组赋值。
     * 真实成员依次为OC_level_SET、OC_time_SET、OC_level_SET_L2、OC_time_SET_L2、
     * OV_level_SET、OV_time_SET、HA_L_Voltage_Precharge、HA_H_Voltage。 */
    for (index = 42U; index < 50U; ++index)
    {
        if (words[index] != actual[index])
        {
            SstLegacyParameters_ApplySnapshot(&words[42]);
            break;
        }
    }
    /* 提交第5步：运行期配置及Word旧块2的系统方式/保护使能落到真实变量。
     * words[50]只在补充区设置快速软件保护总使能，旧43字块保留当前值。 */
    g_chb_dab_protection_enable = words[50];
    /* 扩展参数51的两个管理位不写run_enable、dab_enable或ac_mode。 */
    PortRunFLAG.Bbits.Port_RunOrTest = words[51] & 1U;
    PortRunFLAG.Bbits.Port_AutoOrManual = (words[51] >> 1U) & 1U;
    if (actual[52] != words[52])
    {
        /* 扩展参数52：AD保护使能变量 -> FPGA硬件写接口。 */
        Port_AD_PORT_EN = words[52];
        Port_ADProtect_EN();
    }
    fo_changed = (actual[53] != words[53]) || (actual[54] != words[54]) || (actual[55] != words[55]);
    if (fo_changed != 0U)
    {
        /* 扩展参数53..55：三组光纤保护使能 -> FPGA硬件写接口。 */
        Port_FO_1_EN = words[53];
        Port_FO_2_EN = words[54];
        Port_FO_3_EN = words[55];
        Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
    }
    if ((fo_changed != 0U) || (actual[56] != words[56]))
    {
        /* 扩展参数56：插件反馈使能；16位均有效。 */
        Port_FB_EN = words[56];
        Port_FB_EN_Set();
    }
    return SST_PARAM_OK;
}
