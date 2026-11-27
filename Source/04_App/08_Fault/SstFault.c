#include "ControlData.h"
#include "ProtectionData.h"
#include "SstFault.h"
#include "ChbDabControl.h"
#include "FaultCheck.H"
#include "FpgaIo.h"
#include "SstBoardConfig.h"

/* Report state is owned by the main loop. The hardware reads below are diagnostic only. */
typedef struct
{
    SST_FAULT_RECORD raw; /* raw：后台采集的原控制故障、硬件锁存及观察时刻的完整副本。 */
    unsigned short state; /* state：CHB_STATE_* 运行状态编号。 */
    unsigned short software_oc; /* software_oc：原 HVAC 软件过流故障标志的后台副本。 */
    unsigned short software_ov; /* software_ov：原 HVAC 软件过压故障标志的后台副本。 */
    unsigned short reset_pending; /* reset_pending：原两拍故障清除尚未结束的标志。 */
    unsigned short run_enable; /* run_enable：CHB 启动命令，0 停止、1 启动；开环直接解锁，闭环走预充流程。 */
    unsigned short dab_enable; /* dab_enable：DAB 原边使能命令，0 停止、1 启动；开环可独立控制。 */
} SST_FAULT_INPUT;

typedef struct
{
    unsigned short mask; /* mask：故障原始字中与本映射条目对应的位掩码。 */
    unsigned short code; /* code：参考五位 SST_FAULT_* 上报故障码，0 表示无故障。 */
    unsigned short level; /* level：参考故障等级，0 最严重、4 无故障。 */
} SST_FAULT_MAP;

/* FaultTypeFLAG layout is defined by main.H, not the ambiguous PORT_FAULT word. */
static const SST_FAULT_MAP s_type_map[] = {
    {0x0001U, SST_FAULT_HW_PORT_OV, SST_FAULT_LEVEL_1},
    {0x0002U, SST_FAULT_HW_PORT_OC, SST_FAULT_LEVEL_1},
    {0x0004U, SST_FAULT_HW_DAB_BRIDGE, SST_FAULT_LEVEL_1},
    {0x0008U, SST_FAULT_HW_CHB_BRIDGE, SST_FAULT_LEVEL_1},
    {0x0010U, SST_FAULT_HW_MODULE_SINGLE, SST_FAULT_LEVEL_1},
    {0x0020U, SST_FAULT_HW_MODULE_OV, SST_FAULT_LEVEL_1},
    {0x0040U, SST_FAULT_HW_MODULE_OC, SST_FAULT_LEVEL_1},
    {0x0080U, SST_FAULT_HW_MODULE_OT, SST_FAULT_LEVEL_1},
    {0x0100U, SST_FAULT_CONTACTOR, SST_FAULT_LEVEL_3},
    {0x0200U, SST_FAULT_BREAKER, SST_FAULT_LEVEL_3},
    {0x0400U, SST_FAULT_HW_CONTROLLER, SST_FAULT_LEVEL_1},
    {0x0800U, SST_FAULT_SYNC, SST_FAULT_LEVEL_3},
    {0x1000U, SST_FAULT_COMMUNICATION, SST_FAULT_LEVEL_3},
    {0x4000U, SST_FAULT_HW_MODULE_UV, SST_FAULT_LEVEL_1},
    {0x8000U,
     SST_FAULT_HW_MODULE_OV2,
     SST_FAULT_LEVEL_1}}; /* s_type_map：旧 FaultTypeFLAG 各位到五位故障码及等级的映射。 */

static const SST_FAULT_MAP s_port_map[] = {
    {0x0001U, SST_FAULT_CONTACTOR, SST_FAULT_LEVEL_3},
    {0x0002U, SST_FAULT_BREAKER, SST_FAULT_LEVEL_3},
    {0x0004U, SST_FAULT_HW_FAN, SST_FAULT_LEVEL_1},
    {0x0008U, SST_FAULT_HW_CONTROLLER, SST_FAULT_LEVEL_1},
    {0x0010U, SST_FAULT_HW_AUX_SUPPLY, SST_FAULT_LEVEL_1},
    {0x0020U, SST_FAULT_HW_EPO, SST_FAULT_LEVEL_1},
    {0x0080U, SST_FAULT_HW_CHB_BRIDGE, SST_FAULT_LEVEL_1},
    {0x0100U, SST_FAULT_PHASE_SEQUENCE, SST_FAULT_LEVEL_2},
    {0x0200U, SST_FAULT_FREQUENCY, SST_FAULT_LEVEL_2},
    {0x0400U, SST_FAULT_HW_PORT_OC, SST_FAULT_LEVEL_1},
    {0x0800U, SST_FAULT_HW_MODULE_UV, SST_FAULT_LEVEL_1},
    {0x1000U, SST_FAULT_HW_PORT_OV, SST_FAULT_LEVEL_1}
    /* Bits 13/14 are reused by HVAC OV/OC; never label them COMM/SYNC here. */
}; /* s_port_map：旧端口故障字的已核实位映射；重用位另由软件故障判定。 */

static unsigned short s_current[SST_FAULT_CURRENT_CAPACITY]; /* s_current：本轮故障事件去重后的当前五位故障码表。 */
static unsigned short s_current_count; /* s_current_count：当前故障码有效数量，上限 32。 */
static unsigned short s_current_level = SST_FAULT_LEVEL_NONE; /* s_current_level：当前故障中最严重的参考等级。 */
static SST_FAULT_RECORD s_history[SST_FAULT_HISTORY_CAPACITY]; /* s_history：32 条详细故障历史的 RAM 环形缓存。 */
static unsigned short s_history_count; /* s_history_count：详细历史的有效记录数量，上限 32。 */
static unsigned short s_history_next; /* s_history_next：详细历史下一次写入的环形物理下标。 */
static unsigned long s_sequence; /* s_sequence：后台报告事件的累计递增序号。 */
static unsigned short s_legacy_history
    [SST_FAULT_LEGACY_HISTORY_WORDS]; /* s_legacy_history：参考组 19 的 34 字 RAM 历史环，字 0 保存下一写位置。 */
static unsigned short s_legacy_first; /* s_legacy_first：本次会话首次故障前需要插入零分隔符的标志。 */

#ifdef HVAC_online
/* 701 can trip before 403 records its source or 501 decodes the UNIT frame.
 * Allow one retry at least 10 ms later in the background. */
#define SST_FAULT_MODULE_STATUS_SETTLE_TICKS 2UL

/* FanBalance sends these six protection latches in the 501 STATUS word.
 * Live undervoltage and external SHUTDOWN do not drive UNIT FO. */
static const SST_FAULT_MAP s_module_status_map[] = {
    {Module_OC2_L_Mask, SST_FAULT_HW_MODULE_OC, SST_FAULT_LEVEL_1},
    {Module_OC1_L_Mask, SST_FAULT_HW_MODULE_IR_OC, SST_FAULT_LEVEL_1},
    {Module_OV_L_Mask, SST_FAULT_HW_MODULE_OV, SST_FAULT_LEVEL_1},
    {Module_OT_Mask, SST_FAULT_HW_MODULE_OT, SST_FAULT_LEVEL_1},
    {Module_SVG_H_Mask, SST_FAULT_HW_CHB_BRIDGE, SST_FAULT_LEVEL_1},
    {Module_DAB_H_Mask, SST_FAULT_HW_DAB_BRIDGE, SST_FAULT_LEVEL_1}};

#define SST_MODULE_CAUSE_COUNT (sizeof(s_module_status_map) / sizeof(s_module_status_map[0]))

static struct
{
    unsigned short fb;
    unsigned short fo[3];
    unsigned short causes[SST_MODULE_CAUSE_COUNT][3];
    unsigned short unclassified[3];
    unsigned short waiting_for_status;
    unsigned long observed_tick;
} s_module_evidence;

static void SstFault_ClearModuleEvidence(void)
{
    unsigned short cause;
    unsigned short phase;
    s_module_evidence.fb = 0U;
    s_module_evidence.waiting_for_status = 0U;
    for (phase = 0U; phase < 3U; ++phase)
    {
        s_module_evidence.fo[phase] = 0U;
        s_module_evidence.unclassified[phase] = 0U;
        for (cause = 0U; cause < SST_MODULE_CAUSE_COUNT; ++cause)
            s_module_evidence.causes[cause][phase] = 0U;
    }
}

static unsigned short SstFault_CaptureModuleLocations(void)
{
    static const unsigned short input_plugins[3] = {
        Input_A_PluginUnit_8, Input_B_PluginUnit_12, Input_C_PluginUnit_16};
    const unsigned short fb = Port_FBStatusRead(1U) & Port_EffectiveFBEnable();
    unsigned short observed_fo = 0U;
    unsigned short phase;
    for (phase = 0U; phase < 3U; ++phase)
    {
        const unsigned short plugin = input_plugins[phase];
        const unsigned short source = (unsigned short)(1U << (plugin - 2U));
        if ((fb == 0U) || ((fb & source) != 0U))
        {
            const unsigned short enabled = Port_EffectiveFOEnable(plugin);
            const unsigned short fo = enabled ? (Port_FOStatusRead((unsigned char)plugin, 1U) & enabled) : 0U;
            observed_fo |= fo;
            if ((fb & source) != 0U)
            {
                s_module_evidence.fb |= source;
                s_module_evidence.fo[phase] = fo;
            }
        }
    }
    return observed_fo;
}

static unsigned short SstFault_CaptureModuleCauses(void)
{
    static const unsigned char communication_plugins[3][2] = {
        {Com_A1_PluginUnit_5, Com_A2_PluginUnit_6},
        {Com_B1_PluginUnit_9, Com_B2_PluginUnit_10},
        {Com_C1_PluginUnit_13, Com_C2_PluginUnit_14}};
    unsigned short phase;
    for (phase = 0U; phase < 3U; ++phase)
    {
        unsigned short classified = 0U;
        unsigned short cause;
        unsigned short channel;
        for (channel = 0U; channel < 14U; ++channel)
        {
            const unsigned short mask = (unsigned short)(1U << channel);
            if ((s_module_evidence.fo[phase] & mask) != 0U)
            {
                const unsigned char plugin = communication_plugins[phase][channel / 8U];
                const unsigned short status = Module_StatusRead(plugin, (unsigned char)(channel % 8U), 0U);
                /* 501 zero-extends the 14-bit payload in STATUS. */
                if ((status & 0xC000U) == 0U)
                {
                    for (cause = 0U; cause < SST_MODULE_CAUSE_COUNT; ++cause)
                    {
                        if ((status & s_module_status_map[cause].mask) != 0U)
                            s_module_evidence.causes[cause][phase] |= mask;
                    }
                }
            }
        }
        for (cause = 0U; cause < SST_MODULE_CAUSE_COUNT; ++cause)
            classified |= s_module_evidence.causes[cause][phase];
        s_module_evidence.unclassified[phase] = s_module_evidence.fo[phase] & (unsigned short)~classified;
    }
    return s_module_evidence.unclassified[0] | s_module_evidence.unclassified[1] | s_module_evidence.unclassified[2];
}

/* This signal path also covers a fault during open-loop arming, when the
 * legacy ISR records FB but skips the FO scan. */
static unsigned short SstFault_ModuleEvidenceReady(const SST_FAULT_INPUT *input)
{
    if ((input->state != CHB_STATE_FAULT) || (input->raw.control_fault != CHB_FAULT_HARDWARE) ||
        (input->reset_pending != 0U) || (s_current_count != 0U))
        return 1U;

    if (s_module_evidence.waiting_for_status != 0U)
    {
        if ((input->raw.tick_10ms - s_module_evidence.observed_tick) < SST_FAULT_MODULE_STATUS_SETTLE_TICKS)
            return 0U;
        if (s_module_evidence.fb == 0U)
            (void)SstFault_CaptureModuleLocations();
        (void)SstFault_CaptureModuleCauses();
        s_module_evidence.waiting_for_status = 0U;
    }
    else if ((SstFault_CaptureModuleLocations() != 0U) &&
             ((s_module_evidence.fb == 0U) || (SstFault_CaptureModuleCauses() != 0U)))
    {
        s_module_evidence.observed_tick = input->raw.tick_10ms;
        s_module_evidence.waiting_for_status = 1U;
        return 0U;
    }
    return 1U;
}
#endif

/* Legacy globals are not volatile. Read via qualified lvalues without changing
 * their declarations or the original real-time code. Individual scalar reads
 * are intentionally not advertised as one atomic hardware snapshot. */
/* SST_READ_U16：从旧非 volatile 变量按 volatile 16 位左值读取，不改变旧 ISR 声明。 */
#define SST_READ_U16(value) (*(volatile unsigned short *)&(value))
/* SST_READ_U8：从旧非 volatile 变量按 volatile 8 位左值读取，不改变旧 ISR 声明。 */
#define SST_READ_U8(value) (*(volatile unsigned char *)&(value))

/*==================================================================
 * 函数功能: 在后台收集原故障锁存与控制状态，检查采集前后关键状态一致性
 * 输入参数: input：输出目标指针，本函数填写后台故障输入副本
 * 输出参数: input 指向的后台故障输入；不保证各标量为同一硬件瞬间。
 * 返回值: 1 表示本轮可用，0 表示下次主循环再采集
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
static unsigned short SstFault_ReadInput(SST_FAULT_INPUT *input)
{
    unsigned short phase;
    input->raw.code = SST_FAULT_NONE;
    input->raw.level = SST_FAULT_LEVEL_NONE;
    input->raw.sequence = 0UL;
    input->state = g_chb_dab_status.state;
    input->raw.control_fault = g_chb_dab_status.fault_code;
    input->raw.port_fault = SST_READ_U16(Port_RealFault.PORT_FAULT16);
    input->raw.fault_type = SST_READ_U16(FaultTypeFLAG.FaultTypeFLAG16);
    input->raw.port_latch = SST_READ_U16(PortLatch_TAD);
    input->raw.fb_latch = SST_READ_U16(FBLatch_TAD);
    input->raw.fo_a = SST_READ_U16(FOLatch_A_TAD);
    input->raw.fo_b = SST_READ_U16(FOLatch_B_TAD);
    input->raw.fo_c = SST_READ_U16(FOLatch_C_TAD);
    input->raw.module_dispose = SST_READ_U16(ModuleFault_DisposeTimes);
    input->raw.tick_10ms = g_chb_dab_status.tick_10ms;
    /* 保护停机分闸后的普遍失光不能作为其他故障的起因。只记录闭环已有的判断证据。 */
    for (phase = 0U; phase < 3U; ++phase)
        input->raw.unit_link_missing[phase] =
            (input->raw.control_fault == CHB_FAULT_PRECHARGE_TIMEOUT ||
             input->raw.control_fault == CHB_FAULT_UNIT_LINK) ? g_chb_dab_status.unit_link_missing[phase] : 0U;
    input->software_oc = SST_READ_U16(HVAC_OC_Fault);
    input->software_ov = SST_READ_U16(HVAC_OV_Fault);
    input->reset_pending = (unsigned short)(SST_READ_U8(Flag_Clear_0) | SST_READ_U8(Flag_Clear_1));
    input->run_enable = g_chb_dab_command.run_enable;
    input->dab_enable = g_chb_dab_command.dab_enable;
    /* If an ISR changed the control state during collection, try next loop.
     * Do not spin, inhibit IRQs, or reread peripheral registers. */
    return (unsigned short)((input->state == g_chb_dab_status.state) &&
                            (input->raw.control_fault == g_chb_dab_status.fault_code));
}

/*==================================================================
 * 函数功能: 对当前故障去重，记录等级并写入详细历史和参考组 19 历史环
 * 输入参数: code：参考五位 SST_FAULT_* 上报码
 *           level：参考故障等级，0..4
 *           input：已经后台采集并检查关键状态一致性的故障输入
 * 输出参数: 新增当前故障码；更新最高等级、详细历史、事件序号及组 19 历史环。
 * 返回值: 无
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
static void SstFault_Add(unsigned short code, unsigned short level, const SST_FAULT_INPUT *input)
{
    unsigned short i; /* i：当前故障表、位映射表或历史表的遍历索引。 */
    SST_FAULT_RECORD *record; /* record：指向本次写入的详细历史环槽位。 */
    for (i = 0U; i < s_current_count; ++i)
    {
        if (s_current[i] == code)
            return;
    }
    if ((code == SST_FAULT_NONE) || (s_current_count >= SST_FAULT_CURRENT_CAPACITY))
        return;
    s_current[s_current_count++] = code;
    if (level < s_current_level)
        s_current_level = level;
    record = &s_history[s_history_next];
    *record = input->raw;
    record->code = code;
    record->level = level;
    record->sequence = ++s_sequence;
    s_history_next = (unsigned short)((s_history_next + 1U) % SST_FAULT_HISTORY_CAPACITY);
    if (s_history_count < SST_FAULT_HISTORY_CAPACITY)
        ++s_history_count;
    /* Keep the reference group19 ring layout separately from richer records. */
    if (s_legacy_first)
    {
        s_legacy_history[s_legacy_history[0]] = 0U;
        ++s_legacy_history[0];
        if (s_legacy_history[0] == SST_FAULT_LEGACY_HISTORY_WORDS)
            s_legacy_history[0] = 1U;
        s_legacy_first = 0U;
    }
    s_legacy_history[s_legacy_history[0]] = code;
    ++s_legacy_history[0];
    if (s_legacy_history[0] == SST_FAULT_LEGACY_HISTORY_WORDS)
        s_legacy_history[0] = 1U;
}

static void SstFault_AddMappedCode(const SST_FAULT_MAP *mapping, const SST_FAULT_INPUT *input,
                                  unsigned short unlocated)
{
    if (unlocated != 0U)
    {
        SST_FAULT_INPUT entry = *input;
        entry.raw.fo_a = 0U;
        entry.raw.fo_b = 0U;
        entry.raw.fo_c = 0U;
        SstFault_Add(mapping->code, mapping->level, &entry);
        return;
    }
    SstFault_Add(mapping->code, mapping->level, input);
}

#ifdef HVAC_online
static void SstFault_AddModuleCodes(const SST_FAULT_INPUT *input)
{
    SST_FAULT_INPUT entry = *input;
    unsigned short cause;
    for (cause = 0U; cause < SST_MODULE_CAUSE_COUNT; ++cause)
    {
        entry.raw.fo_a = s_module_evidence.causes[cause][0];
        entry.raw.fo_b = s_module_evidence.causes[cause][1];
        entry.raw.fo_c = s_module_evidence.causes[cause][2];
        if ((entry.raw.fo_a | entry.raw.fo_b | entry.raw.fo_c) != 0U)
            SstFault_Add(s_module_status_map[cause].code, s_module_status_map[cause].level, &entry);
    }
    entry.raw.fo_a = s_module_evidence.unclassified[0];
    entry.raw.fo_b = s_module_evidence.unclassified[1];
    entry.raw.fo_c = s_module_evidence.unclassified[2];
    if ((entry.raw.fo_a | entry.raw.fo_b | entry.raw.fo_c) != 0U)
        SstFault_Add(SST_FAULT_HW_MODULE, SST_FAULT_LEVEL_1, &entry);
}
#endif

/*==================================================================
 * 函数功能: 清空当前故障与 RAM 历史，初始化参考历史环写位置
 * 输入参数: 无
 * 输出参数: 当前故障表、详细 RAM 历史及参考组 19 历史环。
 * 返回值: 无
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
void SstFault_Init(void)
{
    unsigned short i; /* i：当前故障表、位映射表或历史表的遍历索引。 */
    for (i = 0U; i < SST_FAULT_CURRENT_CAPACITY; ++i)
        s_current[i] = 0U;
    s_current_count = 0U;
    s_current_level = SST_FAULT_LEVEL_NONE;
    s_history_count = 0U;
    s_history_next = 0U;
    s_sequence = 0UL;
#ifdef HVAC_online
    SstFault_ClearModuleEvidence();
#endif
    for (i = 0U; i < SST_FAULT_LEGACY_HISTORY_WORDS; ++i)
        s_legacy_history[i] = 0U;
    s_legacy_history[0] = 1U;
    s_legacy_first = 1U;
}

/*==================================================================
 * 函数功能: 把原控制/硬件故障映射为参考五位故障码和等级并更新报告
 * 输入参数: 无
 * 输出参数: 当前故障表、详细 RAM 历史及参考组 19 历史环。
 * 返回值: 无
 * 执行说明: 在后台读取模块首锁存和保护原因；只读，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
void SstFault_Service(void)
{
    SST_FAULT_INPUT input; /* input：后台采集的原故障输入及控制状态。 */
    unsigned short code = SST_FAULT_NONE; /* code：参考五位 SST_FAULT_* 上报故障码，0 表示无故障。 */
    unsigned short level = SST_FAULT_LEVEL_2;
    unsigned short module_location = 0U;
    unsigned short has_detail = 0U; /* has_detail：本轮是否已获得可分类的故障明细。 */
    unsigned short i; /* i：当前故障表、位映射表或历史表的遍历索引。 */
    if (!SstFault_ReadInput(&input))
        return;
    /* FaultLatch_Clear is a two-Timer2-tick operation. Wait for it, so stale
     * type flags cannot create a second episode during an explicit reset. */
    if ((input.state == CHB_STATE_STOPPED) && (input.raw.control_fault == CHB_FAULT_NONE) && input.reset_pending)
        return;
    if ((input.state == CHB_STATE_STOPPED) && (input.raw.control_fault == CHB_FAULT_NONE) && !input.run_enable &&
        !input.dab_enable && !input.raw.port_fault && !input.raw.fault_type && !input.software_oc && !input.software_ov)
    {
        s_current_count = 0U;
        s_current_level = SST_FAULT_LEVEL_NONE;
#ifdef HVAC_online
        SstFault_ClearModuleEvidence();
#endif
        return;
    }
#ifdef HVAC_online
    if (!SstFault_ModuleEvidenceReady(&input))
        return;
    /* Read-only bus access must not publish an episode that changed meanwhile. */
    if ((input.state != g_chb_dab_status.state) || (input.raw.control_fault != g_chb_dab_status.fault_code))
        return;
    module_location = s_module_evidence.fo[0] | s_module_evidence.fo[1] | s_module_evidence.fo[2];
    input.raw.fb_latch |= s_module_evidence.fb;
    input.raw.fo_a |= s_module_evidence.fo[0];
    input.raw.fo_b |= s_module_evidence.fo[1];
    input.raw.fo_c |= s_module_evidence.fo[2];
#endif
    switch (input.raw.control_fault)
    {
        case CHB_FAULT_PRECHARGE_TIMEOUT:
            code = SST_FAULT_PRECHARGE_TIMEOUT;
            break;
        case CHB_FAULT_PLL_TIMEOUT:
            code = SST_FAULT_PLL_TIMEOUT;
            break;
        case CHB_FAULT_AC_OVERCURRENT:
            code = SST_FAULT_SW_AC_OC;
            break;
        case CHB_FAULT_DC_OVERVOLTAGE:
            code = SST_FAULT_SW_DC_OV;
            break;
        case CHB_FAULT_BAD_CONFIGURATION:
            code = SST_FAULT_BAD_CONFIGURATION;
            break;
        case CHB_FAULT_SWITCH_FEEDBACK:
            code = SST_FAULT_SWITCH_FEEDBACK;
            break;
        case CHB_FAULT_UNIT_LINK:
            code = SST_FAULT_COMMUNICATION;
            level = SST_FAULT_LEVEL_3;
            break;
        default:
            break;
    }
    if (code != SST_FAULT_NONE)
    {
        SstFault_Add(code, level, &input);
        has_detail = 1U;
    }
    /* Record already-latched software faults; never repeat the trip decision. */
    if (input.software_oc)
    {
        SstFault_Add(SST_FAULT_SW_AC_OC, SST_FAULT_LEVEL_2, &input);
        has_detail = 1U;
    }
    if (input.software_ov)
    {
        SstFault_Add(SST_FAULT_SW_DC_OV, SST_FAULT_LEVEL_2, &input);
        has_detail = 1U;
    }
#ifdef HVAC_online
    if (module_location != 0U)
    {
        SstFault_AddModuleCodes(&input);
        has_detail = 1U;
    }
#endif
    for (i = 0U; i < sizeof(s_type_map) / sizeof(s_type_map[0]); ++i)
    {
        if (input.raw.fault_type & s_type_map[i].mask)
        {
            /* A legacy module summary without its own STATUS match cannot
             * attribute the cause to every UNIT in the FO snapshot. */
            SstFault_AddMappedCode(&s_type_map[i], &input, module_location && (s_type_map[i].mask & 0xC0FCU));
            has_detail = 1U;
        }
    }
    for (i = 0U; i < sizeof(s_port_map) / sizeof(s_port_map[0]); ++i)
    {
        if (input.raw.port_fault & s_port_map[i].mask)
        {
            SstFault_AddMappedCode(&s_port_map[i], &input, module_location && (s_port_map[i].mask & 0x0880U));
            has_detail = 1U;
        }
    }
    /* Use the legacy aggregate when there is no per-UNIT snapshot. */
    if ((module_location == 0U) && ((input.raw.fault_type & 0x2000U) || (input.raw.port_fault & 0x0040U)) &&
        !(input.raw.fault_type & 0xC0FCU) && !(input.raw.port_fault & 0x0880U))
    {
        SstFault_Add(SST_FAULT_HW_MODULE, SST_FAULT_LEVEL_1, &input);
        has_detail = 1U;
    }
    if (!has_detail && ((input.raw.control_fault == CHB_FAULT_HARDWARE) || (input.raw.port_fault & 0x8000U)))
    {
        SstFault_Add(SST_FAULT_HW_UNCLASSIFIED, SST_FAULT_LEVEL_1, &input);
    }
}

/*==================================================================
 * 函数功能: 按当前故障索引读取报告码
 * 输入参数: index：当前故障表索引，范围 0..当前故障数-1
 * 输出参数: 无。
 * 返回值: 故障码；越界返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetCode(unsigned short index)
{
    return (index < s_current_count) ? s_current[index] : 0U;
}

/*==================================================================
 * 函数功能: 读取当前首个故障报告码
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 首故障码；无故障返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetFirstCode(void)
{
    return SstFault_GetCode(0U);
}

/*==================================================================
 * 函数功能: 读取当前故障中的最高严重等级
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 参考等级 0..4，4 表示无故障
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetLevel(void)
{
    return s_current_level;
}

/*==================================================================
 * 函数功能: 读取当前故障去重数量
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 当前故障数量，最多 32
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetActiveCount(void)
{
    return s_current_count;
}

/*==================================================================
 * 函数功能: 读取 RAM 详细历史的有效记录数
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 有效历史数量，最多 32
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetHistoryCount(void)
{
    return s_history_count;
}

unsigned long SstFault_GetHistorySequence(void)
{
    return s_sequence;
}

/*==================================================================
 * 函数功能: 读取参考 FuncCode 组 19 的原布局历史环字
 * 输入参数: index：参考组 19 历史环字索引，范围 0..33
 * 输出参数: 无。
 * 返回值: 字 0 为下次写位置，字 1..33 为故障码；越界返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetLegacyHistoryWord(unsigned short index)
{
    return (index < SST_FAULT_LEGACY_HISTORY_WORDS) ? s_legacy_history[index] : 0U;
}

/*==================================================================
 * 函数功能: 将按新到旧的历史序号换算为环形缓存物理下标
 * 输入参数: index：已检查范围的历史顺序索引，0 为最新记录
 * 输出参数: 无。
 * 返回值: 环形数组下标
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
static unsigned short SstFault_HistoryIndex(unsigned short index)
{
    return (unsigned short)((s_history_next + SST_FAULT_HISTORY_CAPACITY - 1U - index) % SST_FAULT_HISTORY_CAPACITY);
}

/*==================================================================
 * 函数功能: 按新到旧读取详细历史故障码
 * 输入参数: newest_index：历史记录从新到旧的索引，0 为最新
 * 输出参数: 无。
 * 返回值: 故障码；越界返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetHistoryCode(unsigned short newest_index)
{
    return (newest_index < s_history_count) ? s_history[SstFault_HistoryIndex(newest_index)].code : 0U;
}

/*==================================================================
 * 函数功能: 读取 RAM 历史中最高严重等级
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 参考等级 0..4
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
unsigned short SstFault_GetHistoryLevel(void)
{
    unsigned short i; /* i：当前故障表、位映射表或历史表的遍历索引。 */
    unsigned short level = SST_FAULT_LEVEL_NONE; /* level：参考故障等级，0 最严重、4 无故障。 */
    for (i = 0U; i < s_history_count; ++i)
    {
        if (s_history[i].level < level)
            level = s_history[i].level;
    }
    return level;
}

/*==================================================================
 * 函数功能: 按新到旧复制一条详细历史记录给调用者
 * 输入参数: newest_index：历史记录从新到旧的索引，0 为最新
 *           record：输出详细故障记录的目标指针，不可为空
 * 输出参数: record 指向的一条历史记录，newest_index=0 读取最新。
 * 返回值: 1 表示复制成功，0 表示指针为空或越界
 * 执行说明: 后台读取 RAM 历史，不访问硬件、不关中断；输出为后台观察记录。
 *================================================================*/
unsigned short SstFault_GetHistory(unsigned short newest_index, SST_FAULT_RECORD *record)
{
    if ((record == 0) || (newest_index >= s_history_count))
        return 0U;
    *record = s_history[SstFault_HistoryIndex(newest_index)];
    return 1U;
}
