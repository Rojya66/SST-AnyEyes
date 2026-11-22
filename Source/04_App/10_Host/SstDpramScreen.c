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
static uint16_t s_completed_count; /* DSP完成的操作次数，16位回绕；屏幕无需填写请求序号。 */
static uint16_t s_operation; /* 当前操作号，完成时发布到1B06。 */
static uint16_t s_save_pending; /* 当前屏幕事务正在等待EEPROM完成。 */

/*==================================================================
 * 函数功能: 发布本次操作结果，最后清除busy，通知屏幕操作完成
 * 输入参数: result：SST_PARAM_*或屏幕扩展结果
 * 输出参数: 1B03结果、1B05错误位置、1B06完成操作号、1B02完成计数及1B04忙标志
 * 返回值: 无
 * 执行说明: 后台；屏幕等1B01=0且1B04=0后读取结果，不需要维护序号。
 *================================================================*/
static void CompleteScreenTransaction(uint16_t result)
{
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 3U] = result;
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 5U] = g_sst_dpram_screen_result.error_word;
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 6U] = s_operation;
    s_completed_count = (uint16_t)(s_completed_count + 1U);
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 2U] = s_completed_count;
    /* 结果及完成信息已就绪后再清busy，避免屏幕把上一笔结果当作本笔结果。 */
    DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 4U] = 0U;
}

/*==================================================================
 * 函数功能: 用当前实际编码刷新待应用区，屏幕明确发REFRESH时才覆盖
 * 输入参数: 无
 * 输出参数: 待应用区前57字；其余保留字清零
 * 返回值: 无
 * 执行说明: 主机在op=0且busy=0时编辑，不与REFRESH并发。
 *================================================================*/
static void ApplyRefreshTransaction(void)
{
    uint16_t actual[SST_SCREEN_PARAMETERS]; /* 实际57字临时编码。 */
    uint16_t index; /* 待应用区索引。 */
    SstDpramParameter_EncodeParameterSnapshot(actual);
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
void SstDpramScreen_Initialize(void)
{
#if SST_ENABLE_DPRAM
    uint16_t index; /* 初始化字索引。 */
    for (index = 0U; index < SST_SCREEN_WORDS; ++index)
        DPRAM_SstScreen[index] = 0U;
    s_next = 384U;
    s_epoch = s_completed_count = s_operation = s_save_pending = 0U;
    g_sst_dpram_screen_result.control_count = 0U;
    g_sst_dpram_screen_result.regulate_count = 0U;
    g_sst_dpram_screen_result.control_result = 0U;
    g_sst_dpram_screen_result.regulate_result = 0U;
    g_sst_dpram_screen_result.error_word = 0xFFFFU;
    g_sst_dpram_screen_result.switch_permission = 0U;
    g_sst_dpram_screen_result.ignored_control_words = 0U;
    ApplyRefreshTransaction();
    SstDpramScreen_PublishStatus();
#endif
}

/*==================================================================
 * 函数功能: 每个遥测节拍构建补充状态、全部详细故障码和参数实际值
 * 输入参数: 无
 * 输出参数: RAM发布快照；上一轮发布未完成时跳过
 * 返回值: 无
 * 执行说明: 不关中断；版本戳仅保证后台发布一致性，非同步TAD锁存。
 *================================================================*/
void SstDpramScreen_PublishStatus(void)
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
    status[2] = 0x0102U; /* 补充点表版本1.2：1B01直接提交，1B02为完成计数；Word旧块不变。 */
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
    /* 198F：bit0 CHB PWM、bit1 DAB PWM、bit2 PLL锁定、bit4外环生效。
     * bit3仍占原位；load_ready为兼容保留字段，固定0，不再执行低压就绪逻辑。 */
    status[15] = g_chb_dab_status.ac_pwm_enabled | (g_chb_dab_status.dab_pwm_enabled << 1U) |
                 (g_chb_dab_status.pll_locked << 2U) | (g_chb_dab_status.load_ready << 3U) |
                 (g_chb_dab_status.voltage_loop_active << 4U);
    /* 1990：bit0 QF1目标合位、bit1 KT1保持命令，仅表示命令。 */
    status[16] = g_chb_dab_status.qf1_close_command | (g_chb_dab_status.kt1_close_command << 1U);
    status[17] = g_chb_dab_status.switch_din_raw;
    /* 1992：bit0 DI7 QF1合位、bit1 DI6 KT1合位、bit2 DI8 QF1分位，表示实际反馈。 */
    status[18] = g_chb_dab_status.qf1_closed | (g_chb_dab_status.kt1_closed << 1U) |
                 (g_chb_dab_status.qf1_open << 2U);
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
    status[34] = 0xC204U; /* word18 DC_V、word25 PF、word30/31 A/B相701故障。 */
    status[35] = g_sst_dpram_screen_result.switch_permission;
    status[36] = SstDpramTelemetry_GetPluginMask(); /* 有配置生产者的插件位图。 */
    /* Modbus 19A5 / DSP 6600334A：本笔控制块被忽略的未接入字段，bitN=wordN。
     * 例：word4含PQ位时置bit4，但不因此拒绝同包正常HVAC启动。 */
    status[37] = g_sst_dpram_screen_result.ignored_control_words;
    status[38] = 0x0001U; /* 19A6：bit0表示块3的word32（C相701故障）已接入。 */
    fault[0] = SstDpramTelemetry_GetFaultWord();
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
    SstDpramParameter_EncodeParameterSnapshot(&s_publication[SST_SCREEN_ACTUAL]);
    s_epoch = (uint16_t)(s_epoch + 2U);
    DPRAM_SstScreen[0] = (uint16_t)(s_epoch - 1U);
    s_next = 1U;
#endif
}

/*==================================================================
 * 函数功能: 后台接收1B01操作字，执行一次后反馈结果，每轮发布最多16个只读字
 * 输入参数: 无
 * 输出参数: APPLY/SAVE/LOAD/REFRESH结果及分片快照；待应用区不周期覆盖
 * 返回值: 无
 * 执行说明: 1B00保留且不参与解析；只写1B01即可提交，不缓存上次结果来跳过新请求。
 *           保存忙时清除额外操作字，不排队、不重启保存；屏幕等busy=0后再提交。
 *           不等待主机、不屏蔽TAD；APPLY提交至完成期间不得修改候选区。
 *================================================================*/
void SstDpramScreen_ProcessTransactions(void)
{
#if SST_ENABLE_DPRAM
    uint16_t operation; /* 1B01：0无操作，1应用、2保存、3加载、4刷新候选区。 */
    uint16_t result; /* 本笔处理结果。 */
    uint16_t index; /* 参数/分片索引。 */
    operation = DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 1U];
    if (s_save_pending != 0U)
    {
        /* EEPROM尚未发布完成结果：清除忙时重复按键，避免保存结束后自动再执行。 */
        if (operation != 0U)
            DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 1U] = 0U;
        if (SstParameterStore_IsBusy() == 0U)
        {
            s_save_pending = 0U;
            CompleteScreenTransaction(SstParameterStore_GetResult());
        }
    }
    else if (operation != 0U)
    {
        /* 先置busy和处理中结果，再清操作字，屏幕不会在受理间隙读到旧的成功结果。 */
        DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 4U] = 1U;
        DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 3U] = SST_PARAM_BUSY;
        DPRAM_SstScreen[SST_SCREEN_TRANSACTION + 1U] = 0U;
        s_operation = operation;
        g_sst_dpram_screen_result.error_word = 0xFFFFU;
        result = SST_PARAM_OK;
        if (SST_ALLOW_REMOTE_COMMANDS == 0U)
            result = SST_PARAM_DENIED;
        else if (operation == SST_SCREEN_APPLY)
        {
            uint16_t candidate[SST_SCREEN_PARAMETERS]; /* 本次APPLY完整的57字候选参数。 */
            for (index = 0U; index < SST_SCREEN_PARAMETERS; ++index)
                candidate[index] = DPRAM_SstScreen[SST_SCREEN_STAGE + index];
            result = SstDpramParameter_ApplyParameterSnapshot(candidate);
        }
        else if ((operation == SST_SCREEN_SAVE) || (operation == SST_SCREEN_LOAD))
            result = SstParameterStore_Request(operation == SST_SCREEN_LOAD ? 1U : 0U);
        else if (operation == SST_SCREEN_REFRESH)
            ApplyRefreshTransaction();
        else
            result = SST_SCREEN_UNSUPPORTED;

        if ((result == SST_PARAM_OK) && (operation == SST_SCREEN_SAVE))
            s_save_pending = 1U; /* 写入及读回校验完成后，再统一发布最终结果并清busy。 */
        else
            CompleteScreenTransaction(result);
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
