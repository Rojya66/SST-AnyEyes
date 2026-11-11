//###########################################################################
// FILE: SstDpramScreen.c
// FUNCTION: Modbus直读写寄存器事务及有界后台发布；无A5 5A串口维护协议。
//###########################################################################
#include "SstDpramProtocol.h"
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstParameterStore.h"
#include "SstFault.h"
#include "ProtectionData.h"

#pragma DATA_SECTION(DPRAM_SstScreen, ".DPRAM_SstScreen");
volatile uint16_t DPRAM_SstScreen[SST_SCREEN_WORDS]; /* 0x66003300起400字，NOINIT防加载外设。 */

#if SST_ENABLE_DPRAM
static uint16_t s_publication[384]; /* 前128字状态/故障+128字待应用占位+128字实际值；发布跳过待应用区。 */
static uint16_t s_next; /* 后台下一个待发布字号；384表示空闲。 */
static uint16_t s_epoch; /* 完整快照偶数版本，奇数表示DSP正在发布。 */
static uint16_t s_sequence; /* 正在执行/最近一次完成的主机事务序号。 */
static uint16_t s_operation; /* 该序号对应操作，用于识别重复和冲突。 */
static uint16_t s_save_pending; /* 当前屏幕事务正在等待EEPROM完成。 */
static uint16_t s_last_result; /* 最近完成结果，重复事务只回复不重复执行。 */

/*==================================================================
 * 函数功能: 完成一个普通寄存器事务，先写结果，最后写确认序号
 * 输入参数: result：SST_PARAM_*或屏幕扩展结果
 * 输出参数: 事务结果/错误位置/操作号/ack_sequence；busy清零
 * 返回值: 无
 * 执行说明: 后台；主机等ack等于请求序号后才读取该笔结果。
 *================================================================*/
static void Complete(uint16_t result)
{
    s_last_result = result;
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 3U] = result;
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 4U] = 0U;
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 5U] = g_sst_dpram_screen_result.error_word;
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 6U] = s_operation;
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 2U] = s_sequence;
}

/*==================================================================
 * 函数功能: 用当前实际编码刷新待应用区，屏幕明确发REFRESH时才覆盖
 * 输入参数: 无
 * 输出参数: 待应用区前57字；其余保留字清零
 * 返回值: 无
 * 执行说明: 主机在op=0且busy=0时编辑，不与REFRESH并发。
 *================================================================*/
static void RefreshStage(void)
{
    uint16_t actual[SST_SCREEN_PARAMETERS]; /* 实际57字临时编码。 */
    uint16_t index; /* 待应用区索引。 */
    SstDpramParameter_Read(actual);
    for (index = 0U; index < 128U; ++index)
        DPRAM_SstScreen[SST_SCREEN_STAGE + index] = index < SST_SCREEN_PARAMETERS ? actual[index] : 0U;
}
#endif

/*==================================================================
 * 函数功能: 上电建立补充寄存器，不执行DPRAM残留的命令
 * 输入参数: 无
 * 输出参数: 只读区/事务区初值、待应用区实际值；默认停机、合闸许可0
 * 返回值: 无
 * 执行说明: 运行对象初始化后、DPRAM中断开放前调用；禁用DPRAM时无访问。
 *================================================================*/
void SstDpramScreen_Init(void)
{
#if SST_ENABLE_DPRAM
    uint16_t index; /* 初始化字索引。 */
    for (index = 0U; index < SST_SCREEN_WORDS; ++index)
        DPRAM_SstScreen[index] = 0U;
    s_next = 384U;
    s_epoch = s_sequence = s_operation = s_save_pending = s_last_result = 0U;
    g_sst_dpram_screen_result.control_count = 0U;
    g_sst_dpram_screen_result.regulate_count = 0U;
    g_sst_dpram_screen_result.control_result = 0U;
    g_sst_dpram_screen_result.regulate_result = 0U;
    g_sst_dpram_screen_result.error_word = 0xFFFFU;
    g_sst_dpram_screen_result.switch_permission = 0U;
    RefreshStage();
    SstDpramScreen_Publish();
#endif
}

/*==================================================================
 * 函数功能: 每个遥测节拍构建补充状态、全部详细故障码和参数实际值
 * 输入参数: 无
 * 输出参数: RAM发布快照；上一轮发布未完成时跳过
 * 返回值: 无
 * 执行说明: 不关中断；版本戳仅保证后台发布一致性，非同步TAD锁存。
 *================================================================*/
void SstDpramScreen_Publish(void)
{
#if SST_ENABLE_DPRAM
    uint16_t index; /* RAM快照索引。 */
    uint16_t *status; /* 状态区视图。 */
    uint16_t *fault; /* 故障区视图。 */
    if (s_next < 384U)
        return;
    for (index = 0U; index < 384U; ++index)
        s_publication[index] = 0U;
    status = s_publication;
    fault = &s_publication[SST_SCREEN_FAULT];
    status[1] = 0x5353U;
    status[2] = 0x0101U; /* V1.1屏幕适配点表版本。 */
    status[3] = SST_SCREEN_PARAMETERS;
    status[4] = g_sst_dpram_screen_result.control_count;
    status[5] = g_sst_dpram_screen_result.control_result;
    status[6] = g_sst_dpram_screen_result.regulate_count;
    status[7] = g_sst_dpram_screen_result.regulate_result;
    status[8] = g_sst_dpram_screen_result.error_word;
    status[9] = g_chb_dab_status.state;
    status[10] = g_chb_dab_status.fault_code;
    status[11] = g_chb_dab_status.startup_phase;
    status[12] = (uint16_t)g_chb_dab_status.tick_10ms;
    status[13] = (uint16_t)(g_chb_dab_status.tick_10ms >> 16U);
    status[14] = (SST_PHASE_A_ONLY_TEST ? 1U : 0U) | (SST_ENABLE_SWITCH_OUTPUT ? 2U : 0U) |
                 (SST_ENABLE_SWITCH_FEEDBACK ? 4U : 0U) | (SST_ALLOW_REMOTE_COMMANDS ? 8U : 0U);
    status[15] = g_chb_dab_status.ac_pwm_enabled | (g_chb_dab_status.dab_pwm_enabled << 1U) |
                 (g_chb_dab_status.pll_locked << 2U) | (g_chb_dab_status.load_ready << 3U) |
                 (g_chb_dab_status.voltage_loop_active << 4U);
    status[16] = g_chb_dab_status.ac_input_closed | (g_chb_dab_status.precharge_bypassed << 1U);
    status[17] = g_chb_dab_status.switch_din_raw;
    status[18] = g_chb_dab_status.ac_input_feedback | (g_chb_dab_status.bypass_feedback << 1U);
    status[19] = SstParameterStore_GetResult();
    /* float观察量按IEEE754低16位在前存储；ARM仍只透传16位字。 */
    {
        union
        {
            float value; /* 待编码的实际物理浮点值。 */
            uint32_t raw; /* 同一浮点值的IEEE754位模式。 */
        } sample; /* 同一32位物理量的无别名冲突编码。 */

        sample.value = g_chb_dab_status.dc_voltage_average_v;
        status[20] = (uint16_t)sample.raw;
        status[21] = (uint16_t)(sample.raw >> 16U);
        sample.value = g_chb_dab_status.id_reference_a;
        status[22] = (uint16_t)sample.raw;
        status[23] = (uint16_t)(sample.raw >> 16U);
        sample.value = g_chb_dab_status.id_feedback_a;
        status[24] = (uint16_t)sample.raw;
        status[25] = (uint16_t)(sample.raw >> 16U);
        sample.value = g_chb_dab_status.iq_feedback_a;
        status[26] = (uint16_t)sample.raw;
        status[27] = (uint16_t)(sample.raw >> 16U);
        sample.value = g_chb_dab_status.startup_voltage_reference_v;
        status[28] = (uint16_t)sample.raw;
        status[29] = (uint16_t)(sample.raw >> 16U);
        sample.value = g_chb_dab_status.pll_frequency_hz;
        status[30] = (uint16_t)sample.raw;
        status[31] = (uint16_t)(sample.raw >> 16U);
    }
    status[32] = 0x000FU; /* 能力bit0旧遥控、bit1旧组1调参、bit2直接参数、bit3EEPROM。 */
    /* 旧块3有效字段位图：单相无B/C，二次通道/中线/谐波无有效生产者。 */
    status[33] = SST_PHASE_A_ONLY_TEST ? 0x013FU : 0x07FFU;
    status[34] = 0x0204U; /* word18 DC_V和word25 PF。 */
    status[35] = g_sst_dpram_screen_result.switch_permission;
    status[36] = SstDpramTelemetry_PluginMask(); /* 有配置生产者的插件位图。 */
    fault[0] = SstDpramTelemetry_Fault();
    fault[1] = Port_RealFault.PORT_FAULT16;
    fault[2] = FaultTypeFLAG.FaultTypeFLAG16;
    fault[3] = g_chb_dab_status.fault_code;
    fault[4] = SstFault_GetFirstCode();
    fault[5] = SstFault_GetLevel();
    fault[6] = SstFault_GetActiveCount();
    fault[7] = SstFault_GetHistoryCount();
    for (index = 0U; index < 32U; ++index)
        fault[8U + index] = SstFault_GetCode(index);
    fault[40] = HVAC_OC_Fault;
    fault[41] = HVAC_OV_Fault;
    SstDpramParameter_Read(&s_publication[SST_SCREEN_ACTUAL]);
    s_epoch = (uint16_t)(s_epoch + 2U);
    DPRAM_SstScreen[0] = (uint16_t)(s_epoch - 1U);
    s_next = 1U;
#endif
}

/*==================================================================
 * 函数功能: 后台处理最后写入的操作字，每轮发布最多16个只读字
 * 输入参数: 无
 * 输出参数: APPLY/SAVE/LOAD/REFRESH结果及分片快照；待应用区不周期覆盖
 * 返回值: 无
 * 执行说明: 不等待主机、不屏蔽TAD；主机提交后直到ACK不得修改候选区。
 *================================================================*/
void SstDpramScreen_Service(void)
{
#if SST_ENABLE_DPRAM
    uint16_t sequence; /* 本轮请求序号，0保留。 */
    uint16_t operation; /* 最后写入的提交操作字。 */
    uint16_t result; /* 本笔处理结果。 */
    uint16_t index; /* 参数/分片索引。 */
    if (s_save_pending)
    {
        if (!SstParameterStore_IsBusy())
        {
            s_save_pending = 0U;
            Complete(SstParameterStore_GetResult());
        }
    }
    else
    {
        operation = DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 1U];
        sequence = DPRAM_SstScreen[SST_SCREEN_TRANSACTION];
        if (operation && operation == DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 1U] &&
            sequence == DPRAM_SstScreen[SST_SCREEN_TRANSACTION])
        {
            DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 1U] = 0U;
            if (sequence && sequence == s_sequence && operation == s_operation)
                Complete(s_last_result);
            else if (!sequence || sequence == s_sequence)
            {
                /* 序号冲突不覆盖上一笔缓存；主机需换新非零序号。 */
                DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 3U] = SST_SCREEN_CONFLICT;
                DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 2U] = sequence;
            }
            else
            {
                s_sequence = sequence;
                s_operation = operation;
                g_sst_dpram_screen_result.error_word = 0xFFFFU;
                result = SST_PARAM_OK;
                if (!SST_ALLOW_REMOTE_COMMANDS)
                    result = SST_PARAM_DENIED;
                else if (operation == SST_SCREEN_APPLY)
                {
                    uint16_t candidate[SST_SCREEN_PARAMETERS]; /* 提交后主机冻结的候选副本。 */
                    for (index = 0U; index < SST_SCREEN_PARAMETERS; ++index)
                        candidate[index] = DPRAM_SstScreen[SST_SCREEN_STAGE + index];
                    result = SstDpramParameter_Apply(candidate);
                }
                else if (operation == SST_SCREEN_SAVE || operation == SST_SCREEN_LOAD)
                    result = SstParameterStore_Request(operation == SST_SCREEN_LOAD ? 1U : 0U);
                else if (operation == SST_SCREEN_REFRESH)
                    RefreshStage();
                else
                    result = SST_SCREEN_UNSUPPORTED;
                if (result == SST_PARAM_OK && operation == SST_SCREEN_SAVE)
                {
                    s_save_pending = 1U;
                    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 4U] = 1U;
                    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 3U] = SST_PARAM_BUSY;
                }
                else
                    Complete(result);
            }
        }
    }
    /* 只读输出片段不包含0x1A00候选区，也不覆盖主机事务输入。 */
    for (index = 0U; index < 16U && s_next < 384U; ++index)
    {
        if (s_next == SST_SCREEN_STAGE)
            s_next = SST_SCREEN_ACTUAL;
        DPRAM_SstScreen[s_next] = s_publication[s_next];
        ++s_next;
    }
    if (s_next == 384U && DPRAM_SstScreen[0] != s_epoch)
        DPRAM_SstScreen[0] = s_epoch;
#endif
}
