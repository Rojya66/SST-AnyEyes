#include "SstDpramV2Telemetry.h"
#include "ChbBuildConfig.h"

#if SST_ENABLE_DPRAM
#include "ChbDabControl.h"
#include "ControlData.h"
#include "FpgaIo.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstFaultSummary.h"
#include "SstDpramV2.h"
#include "SstFault.h"
#include <string.h>

#define PHASES 3U
#define ACTIVE_PHASES (SST_PHASE_A_ONLY_TEST ? 1U : PHASES)
#define MODULES_PER_PHASE 14U
#define MODULE_WORDS 4U
#define HISTORY_WORDS 18U
#define LIVE_INTERVAL_10MS 3U
#define MODULE_INTERVAL_10MS 10U
#define PUBLISH_WORDS 16U

static const uint16_t s_link_plugin[PHASES][2] = {
    {Com_A1_PluginUnit_5, Com_A2_PluginUnit_6},
    {Com_B1_PluginUnit_9, Com_B2_PluginUnit_10},
    {Com_C1_PluginUnit_13, Com_C2_PluginUnit_14}
};
static const uint16_t s_fo_plugin[PHASES] = {
    Input_A_PluginUnit_8, Input_B_PluginUnit_12, Input_C_PluginUnit_16
};
static const uint16_t s_output_plugin[PHASES] = {
    Putout_A_PluginUnit_7, Putout_B_PluginUnit_11, Putout_C_PluginUnit_15
};

typedef struct
{
    uint16_t pm;
    uint16_t online;
    uint16_t voltage_usable;
} UNIT_LINK_SNAPSHOT;

typedef struct
{
    uint16_t start;
    uint16_t count;
    const uint16_t *words;
} DPRAM_SPAN;

typedef struct
{
    uint16_t epoch_word;
    const DPRAM_SPAN *spans;
    uint16_t span_count;
    uint16_t epoch;
    uint16_t span;
    uint16_t offset;
} DPRAM_PUBLICATION;

#define SPAN(addr, array) { (addr), sizeof(array) / sizeof((array)[0]), (array) }
#define PUBLICATION(addr, spans) { (addr), (spans), sizeof(spans) / sizeof((spans)[0]), 0U, 0U, 0U }

static uint16_t s_uptime[2];          /* 050C–050D */
static uint16_t s_status[0x16];       /* 0510–0525 */
static uint16_t s_electrical[0x2E];   /* 0540–056D */
static uint16_t s_links[0x18];        /* 0580–0597 */
static uint16_t s_protection[0x0C];   /* 059E–05A9 */
static uint16_t s_fault[0x0A];        /* 0700–0709 */
static uint16_t s_fault_codes[SST_FAULT_CURRENT_CAPACITY]; /* 0720–073F */
static const DPRAM_SPAN s_live_spans[] = {
    SPAN(0x050CU, s_uptime), SPAN(0x0510U, s_status), SPAN(0x0540U, s_electrical),
    SPAN(0x0580U, s_links), SPAN(0x059EU, s_protection), SPAN(0x0700U, s_fault),
    SPAN(0x0720U, s_fault_codes)
};
static DPRAM_PUBLICATION s_live = PUBLICATION(SST_DPRAM_V2_LIVE_EPOCH, s_live_spans);

static uint16_t s_max_temp[PHASES];        /* 056E–0570 */
static uint16_t s_observation[2U * PHASES]; /* 0598–059D */
static uint16_t s_module_words[PHASES * MODULES_PER_PHASE * MODULE_WORDS]; /* 0600–06A7 */
static const DPRAM_SPAN s_module_spans[] = {
    SPAN(0x056EU, s_max_temp), SPAN(0x0598U, s_observation), SPAN(0x0600U, s_module_words)
};
static DPRAM_PUBLICATION s_module = PUBLICATION(SST_DPRAM_V2_MODULE_EPOCH, s_module_spans);

static uint16_t s_history_header[3]; /* 0801–0803 */
static uint16_t s_history_words[SST_FAULT_HISTORY_CAPACITY * HISTORY_WORDS];
static DPRAM_SPAN s_history_spans[] = {
    SPAN(0x0801U, s_history_header), SPAN(0x0810U, s_history_words)
};
static DPRAM_PUBLICATION s_history = PUBLICATION(SST_DPRAM_V2_HISTORY_EPOCH, s_history_spans);
static uint32_t s_history_sequence;
static uint32_t s_last_live_tick;
static uint32_t s_last_module_tick;
static uint16_t s_live_pending;

static void WriteU32(uint16_t *words, uint32_t value)
{
    words[0] = (uint16_t)value;
    words[1] = (uint16_t)(value >> 16U);
}

static void WriteFloat(uint16_t *words, float value)
{
    union
    {
        float value;
        uint32_t bits;
    } encoded;
    encoded.value = value;
    WriteU32(words, encoded.bits);
}

/* 501 每组一字，低字节收帧在线、高字节电压可用。按物理口拼接，不读取或清除 F2。 */
static void ReadUnitLinks(UNIT_LINK_SNAPSHOT *links)
{
    const uint16_t group_masks[PHASES][2] = {
        {HA_PM_A1, HA_PM_A2}, {HA_PM_B1, HA_PM_B2}, {HA_PM_C1, HA_PM_C2}
    };
    uint16_t phase;
    for (phase = 0U; phase < ACTIVE_PHASES; ++phase)
    {
        uint16_t pm1 = group_masks[phase][0] & 0x00FFU;
        uint16_t pm2 = group_masks[phase][1] & 0x003FU;
        uint16_t f1_group1 = pm1 ? Port_UnitLinkRead(s_link_plugin[phase][0]) : 0U;
        uint16_t f1_group2 = pm2 ? Port_UnitLinkRead(s_link_plugin[phase][1]) : 0U;
        links[phase].pm = pm1 | (pm2 << 8U);
        links[phase].online = (f1_group1 & pm1) | ((f1_group2 & pm2) << 8U);
        links[phase].voltage_usable = ((f1_group1 >> 8U) & pm1) | (((f1_group2 >> 8U) & pm2) << 8U);
    }
}

static void CaptureStatus(uint32_t now)
{
    CHB_STARTUP_STATUS startup;
    ChbDabControl_ReadStartupStatus(&startup);
    WriteU32(s_uptime, now);

    s_status[0x00U] = g_chb_dab_status.state;
    s_status[0x01U] = startup.active_mode;
    s_status[0x02U] = g_chb_dab_command.run_enable | (g_chb_dab_command.dab_enable << 1U);
    s_status[0x03U] = g_chb_dab_status.ac_pwm_enabled | (g_chb_dab_status.dab_pwm_enabled << 1U) |
                      (g_chb_dab_status.pll_locked << 2U) | (g_chb_dab_status.voltage_loop_active << 3U) |
                      (Port_DSP_CTL.Bbits.SHUTDOWN << 4U);
    s_status[0x04U] = g_chb_dab_status.qf1_close_command | (g_chb_dab_status.kt1_close_command << 1U) |
                      (PortRunFLAG.Bbits.Port_AutoOrManual << 2U) | (SstDpramV2_SwitchPermission() << 3U);
    s_status[0x05U] = g_chb_dab_status.qf1_closed | (g_chb_dab_status.qf1_open << 1U) |
                      (g_chb_dab_status.kt1_closed << 2U);
    s_status[0x06U] = g_chb_dab_status.switch_din_raw;
    s_status[0x07U] = Port_DOUT.SwitchINOUT16;
    WriteU32(&s_status[0x08U], (uint32_t)startup.state_elapsed_10ms);
    s_status[0x0AU] = startup.precharge_stable_10ms;
    s_status[0x0BU] = startup.pll_stable_10ms;
    s_status[0x0CU] = g_chb_dab_status.switch_stable_count;
    s_status[0x0DU] = g_chb_dab_status.switch_mismatch_count;
    s_status[0x0EU] = g_chb_dab_status.startup_phase;
    s_status[0x0FU] = g_chb_dab_status.dab_duty_count;
    WriteU32(&s_status[0x10U], (uint32_t)g_chb_dab_status.startup_tad_count);
    s_status[0x12U] = SST_PHASE_A_ONLY_TEST ? 1U : 7U;
    s_status[0x13U] = SST_ENABLE_SWITCH_OUTPUT | (SST_ENABLE_SWITCH_FEEDBACK << 1U) |
                      (SST_BENCH_INPUT_SIM << 2U) | (SST_ALLOW_REMOTE_COMMANDS << 3U);
    s_status[0x14U] = startup.precharge_ready;
    s_status[0x15U] = startup.topology_valid;
}

static void CaptureElectrical(void)
{
    /* 0540起每项两字。单相仅A相，B/C的线上占位为0，由0522标识。 */
    WriteFloat(&s_electrical[0x00U], PORT_RealTime_DATA.PORT_V_A);
    WriteFloat(&s_electrical[0x06U], PORT_RealTime_DATA.PORT_I_A);
    WriteFloat(&s_electrical[0x0CU], PORT_RealTime_DATA.I_A_2);
    WriteFloat(&s_electrical[0x12U], SVG_Udc_A_Ave);
#if !SST_PHASE_A_ONLY_TEST
    WriteFloat(&s_electrical[0x02U], PORT_RealTime_DATA.PORT_V_B);
    WriteFloat(&s_electrical[0x04U], PORT_RealTime_DATA.PORT_V_C);
    WriteFloat(&s_electrical[0x08U], PORT_RealTime_DATA.PORT_I_B);
    WriteFloat(&s_electrical[0x0AU], PORT_RealTime_DATA.PORT_I_C);
    WriteFloat(&s_electrical[0x0EU], PORT_RealTime_DATA.I_B_2);
    WriteFloat(&s_electrical[0x10U], PORT_RealTime_DATA.I_C_2);
    WriteFloat(&s_electrical[0x14U], SVG_Udc_B_Ave);
    WriteFloat(&s_electrical[0x16U], SVG_Udc_C_Ave);
#endif
    WriteFloat(&s_electrical[0x18U], PORT_RealTime_DATA.PORT_P);
    WriteFloat(&s_electrical[0x1AU], PORT_RealTime_DATA.PORT_Q);
    WriteFloat(&s_electrical[0x1CU], g_chb_dab_status.dc_voltage_average_v);
    WriteFloat(&s_electrical[0x1EU], g_chb_dab_status.dc_voltage_min_v);
    WriteFloat(&s_electrical[0x20U], g_chb_dab_status.pll_frequency_hz);
    WriteFloat(&s_electrical[0x22U], g_chb_dab_status.id_reference_a);
    WriteFloat(&s_electrical[0x24U], g_chb_dab_status.id_feedback_a);
    WriteFloat(&s_electrical[0x26U], g_chb_dab_command.iq_reference_a);
    WriteFloat(&s_electrical[0x28U], g_chb_dab_status.iq_feedback_a);
    WriteFloat(&s_electrical[0x2AU], g_chb_dab_status.startup_voltage_reference_v);
    WriteFloat(&s_electrical[0x2CU], g_chb_dab_status.modulation_actual);
}

static void CaptureProtection(uint16_t core_fault)
{
    UNIT_LINK_SNAPSHOT links[PHASES] = {{0U}};
    uint16_t phase;
    ReadUnitLinks(links);
    for (phase = 0U; phase < ACTIVE_PHASES; ++phase)
    {
        uint16_t plugin = s_fo_plugin[phase];
        uint16_t fo_enable = FPGA_Interface[plugin - 2U].FO_EN & SST_DPRAM_V2_PHASE_MASK;
        s_links[0x00U + phase] = links[phase].pm;                               /* 0580 运行 PM */
        s_links[0x03U + phase] = links[phase].online;                           /* 0583 在线 */
        s_links[0x06U + phase] = links[phase].voltage_usable;                   /* 0586 Udc可用 */
        s_links[0x09U + phase] = links[phase].pm & ~links[phase].online;         /* 0589 当前缺失 */
        s_links[0x0CU + phase] = core_fault == CHB_FAULT_UNIT_LINK
                                     ? g_chb_dab_status.unit_link_missing[phase] & SST_DPRAM_V2_PHASE_MASK
                                     : 0U;                                     /* 058C 跳闸位置 */
        s_links[0x0FU + phase] = fo_enable;                                    /* 058F 使能 */
        s_links[0x12U + phase] = Port_FOStatusRead(plugin, 0U) & fo_enable;     /* 0592 实时失光 */
        /* 701 E5 已在事件时按FO使能锁存，换PM/FO后仍保留首报位置。 */
        s_links[0x15U + phase] = Port_FOStatusRead(plugin, 1U) & SST_DPRAM_V2_PHASE_MASK;
        s_protection[0x05U + phase] = FPGA_Interface[s_output_plugin[phase] - 2U].SHUTDOWN &
                                       SST_DPRAM_V2_PHASE_MASK;
    }
    s_protection[0x00U] = FPGA_Interface[MainControl_PluginUnit_4 - 2U].FB_EN;
    s_protection[0x01U] = Port_FBStatusRead(0U);
    s_protection[0x02U] = Port_FBStatusRead(1U);
    s_protection[0x03U] = FPGA_Interface[Analog_PluginUnit_3 - 2U].AD_PROT_EN & 0xF000U;
    s_protection[0x04U] = g_chb_dab_protection_enable;
    s_protection[0x08U] = CHB_LOAD_EEPROM_PARAMETERS;
    s_protection[0x09U] = Port_AD_ProtectStatusRead(0U) & 0xF000U;
    s_protection[0x0AU] = Port_AD_ProtectStatusRead(1U) & 0xF000U;
    s_protection[0x0BU] = Module_PM_CTL.PM_CTL16 & 0x0211U;
}

static void CaptureFaults(uint16_t core_fault)
{
    uint16_t i;
    s_fault[1] = core_fault;
    s_fault[2] = SstFault_GetFirstCode();
    s_fault[3] = SstFault_GetLevel();
    s_fault[4] = SstFault_GetActiveCount();
    s_fault[5] = *(volatile uint16_t *)&Port_RealFault.PORT_FAULT16;
    s_fault[6] = *(volatile uint16_t *)&FaultTypeFLAG.FaultTypeFLAG16;
    s_fault[7] = *(volatile uint16_t *)&FBLatch_TAD;
    s_fault[8] = *(volatile uint16_t *)&HVAC_OC_Fault;
    s_fault[9] = *(volatile uint16_t *)&HVAC_OV_Fault;
    s_fault[0] = SstFault_EncodeSummary(s_fault[5], s_fault[6], s_fault[8], s_fault[9],
                                      core_fault, s_status[0]);
    if (core_fault == CHB_FAULT_UNIT_LINK)
        s_fault[0] |= 0x2000U;
    for (i = 0U; i < SST_FAULT_CURRENT_CAPACITY; ++i)
        s_fault_codes[i] = SstFault_GetCode(i);
}

static void CaptureLive(uint32_t now)
{
    uint16_t core_fault = g_chb_dab_status.fault_code;
    CaptureStatus(now);
    CaptureElectrical();
    CaptureProtection(core_fault);
    CaptureFaults(core_fault);
}

/* 保留总线原始有符号采样，避免转换为浮点后再量化。仅访问选中且有新鲜Udc的模块。 */
static void CaptureModules(void)
{
    UNIT_LINK_SNAPSHOT links[PHASES] = {{0U}};
    uint16_t phase;
    ReadUnitLinks(links);
    memset(s_module_words, 0, sizeof(s_module_words));
    for (phase = 0U; phase < PHASES; ++phase)
    {
        uint16_t selected = SstDpramV2_ObservationMask(phase);
        uint16_t valid = selected & links[phase].online & links[phase].voltage_usable;
        int16_t max_temp = INT16_MIN;
        uint16_t module;
        s_observation[phase] = selected;
        s_observation[PHASES + phase] = valid;

        for (module = 0U; module < MODULES_PER_PHASE; ++module)
        {
            if (valid & (1U << module))
            {
                uint16_t channel = module % 8U;
                uint16_t plugin = s_link_plugin[phase][module / 8U];
                const volatile FPGA_InterfaceBLOCK *board = &FPGA_Interface[plugin - 2U];
                uint16_t *words = &s_module_words[MODULE_WORDS * (phase * MODULES_PER_PHASE + module)];
                int16_t temperature = board->TEMP[channel];
                words[0] = (uint16_t)board->DC[channel];
                words[1] = (uint16_t)board->IR[channel];
                words[2] = (uint16_t)board->IG[channel];
                words[3] = (uint16_t)temperature;
                if (temperature > max_temp)
                    max_temp = temperature;
            }
        }
        s_max_temp[phase] = valid ? (uint16_t)max_temp : 0U;
    }
}

static void CaptureHistory(uint16_t count, uint32_t sequence)
{
    uint16_t i;
    s_history_header[0] = count;
    WriteU32(&s_history_header[1], sequence);
    s_history_spans[1].count = count * HISTORY_WORDS;
    for (i = 0U; i < count; ++i)
    {
        SST_FAULT_RECORD record;
        uint16_t *words = &s_history_words[i * HISTORY_WORDS];
        (void)SstFault_GetHistory(i, &record);
        words[0] = record.code;
        words[1] = record.level;
        words[2] = record.control_fault;
        words[3] = record.port_fault;
        words[4] = record.fault_type;
        words[5] = record.port_latch;
        words[6] = record.fb_latch;
        words[7] = record.fo_a & SST_DPRAM_V2_PHASE_MASK;
        words[8] = record.fo_b & SST_DPRAM_V2_PHASE_MASK;
        words[9] = record.fo_c & SST_DPRAM_V2_PHASE_MASK;
        words[10] = record.module_dispose;
        WriteU32(&words[11], (uint32_t)record.tick_10ms);
        WriteU32(&words[13], (uint32_t)record.sequence);
        words[15] = record.unit_link_missing[0] & SST_DPRAM_V2_PHASE_MASK;
        words[16] = record.unit_link_missing[1] & SST_DPRAM_V2_PHASE_MASK;
        words[17] = record.unit_link_missing[2] & SST_DPRAM_V2_PHASE_MASK;
    }
}

static uint16_t Publishing(const DPRAM_PUBLICATION *publication)
{
    return publication->span < publication->span_count;
}

static void BeginPublication(DPRAM_PUBLICATION *publication)
{
    ++publication->epoch;
    publication->span = 0U;
    publication->offset = 0U;
    SstDpramV2_WriteWord(publication->epoch_word, publication->epoch);
}

/* 每轮三类发布共享预算。主机见同一偶数戳才拼接跨包数据。 */
static uint16_t PublishWords(DPRAM_PUBLICATION *publication, uint16_t *budget)
{
    if (!Publishing(publication))
        return 0U;

    while (Publishing(publication))
    {
        const DPRAM_SPAN *span = &publication->spans[publication->span];
        if (publication->offset == span->count)
        {
            ++publication->span;
            publication->offset = 0U;
        }
        else
        {
            uint16_t i = publication->offset;
            if (*budget == 0U)
                return 0U;
            SstDpramV2_WriteWord(span->start + i, span->words[i]);
            ++publication->offset;
            --*budget;
        }
    }

    ++publication->epoch;
    SstDpramV2_WriteWord(publication->epoch_word, publication->epoch);
    return 1U;
}

void SstDpramV2Telemetry_Init(void)
{
    uint32_t now = (uint32_t)g_chb_dab_status.tick_10ms;
    s_live.epoch = s_module.epoch = s_history.epoch = 0U;
    s_last_live_tick = now;
    s_last_module_tick = now;
    s_history_sequence = 0U;
    s_live_pending = 0U;
    SstDpramV2_WriteWord(0x050EU, (uint16_t)(g_chb_dab_params.sample_period_s * 1000000.F + 0.5F));
    SstDpramV2_WriteWord(0x050FU, 1000U); /* 配套501位流的回传超时，单位µs。 */
    CaptureLive(now);
    CaptureModules();
    CaptureHistory(0U, 0U);
    BeginPublication(&s_live);
    BeginPublication(&s_module);
    BeginPublication(&s_history);
}

void SstDpramV2Telemetry_Service(void)
{
    uint32_t now = (uint32_t)g_chb_dab_status.tick_10ms;
    uint16_t budget = PUBLISH_WORDS;
    if (!Publishing(&s_live) && (now - s_last_live_tick >= LIVE_INTERVAL_10MS))
    {
        s_last_live_tick = now;
        CaptureLive(now);
        BeginPublication(&s_live);
    }
    if (!Publishing(&s_module) && (now - s_last_module_tick >= MODULE_INTERVAL_10MS))
    {
        s_last_module_tick = now;
        CaptureModules();
        BeginPublication(&s_module);
    }
    if (!Publishing(&s_history))
    {
        uint32_t sequence = (uint32_t)SstFault_GetHistorySequence();
        if (sequence != s_history_sequence)
        {
            s_history_sequence = sequence;
            CaptureHistory(SstFault_GetHistoryCount(), sequence);
            BeginPublication(&s_history);
        }
    }
    if (PublishWords(&s_live, &budget))
        s_live_pending = 1U;
    (void)PublishWords(&s_module, &budget);
    (void)PublishWords(&s_history, &budget);
}

uint16_t SstDpramV2Telemetry_TakeLiveNotification(void)
{
    uint16_t pending = s_live_pending;
    s_live_pending = 0U;
    return pending;
}
#else

void SstDpramV2Telemetry_Init(void) {}
void SstDpramV2Telemetry_Service(void) {}
uint16_t SstDpramV2Telemetry_TakeLiveNotification(void) { return 0U; }

#endif
