/* Real parallel maintenance/parameter/command/store modules; fake DPRAM mailbox,
 * EEPROM and controller/fault input boundaries. No replacement protocol logic. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstDpramMaintenance.h"
#include "SstParams.h"
#include "SstParameterTable.h"
#include "SstParameterStore.h"
#include "SstLegacyParameters.h"
#include "SstFault.h"
#include "DPRAM.h"
#define CHECK(condition) do { if(!(condition)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); } } while(0)

/* DEFAULTS */
volatile CHB_DAB_STATUS g_chb_dab_status;
volatile unsigned short g_chb_dab_protection_enable = 1U;
uint16_t OC_level_SET = 70U, OC_time_SET = 100U, OC_level_SET_L2 = 75U;
uint16_t OC_time_SET_L2 = 5U, OV_level_SET = 830U, OV_time_SET = 5U;
float HA_L_Voltage_Precharge = 350.0F, HA_H_Voltage = 780.0F;
static volatile uint16_t mailbox;
static unsigned mailbox_accesses, remote_calls, eeprom_writes;
static uint8_t eeprom[512];
static SST_FAULT_RECORD history[32];
static uint16_t fault_codes[32], fault_count, history_count;

volatile uint16_t *HostHalfword(uint32_t address)
{
    CHECK(address == DPRAM_Maibox_Right);
    ++mailbox_accesses;
    return &mailbox;
}
uint8_t r_eeprom_byte_read(uint8_t page, uint8_t offset)
{ CHECK(page >= 6U && page <= 7U); return eeprom[page * 64U + offset]; }
uint8_t r_eeprom_byte_write(uint8_t page, uint8_t offset, uint8_t value)
{ CHECK(page >= 6U && page <= 7U); eeprom[page * 64U + offset] = value; ++eeprom_writes; return 1U; }
void ChbDabControl_ApplyRemoteCommand(unsigned short start, unsigned short stop)
{
    ++remote_calls;
    if(stop & 1U) g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 0U;
    else if(start & 1U) g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
}
void SstDisplay_Refresh(void) {}
uint16_t SstDisplay_Read(uint16_t index) { (void)index; return 0U; }
unsigned short SstFault_GetCode(unsigned short index) { return index < fault_count ? fault_codes[index] : 0U; }
unsigned short SstFault_GetFirstCode(void) { return SstFault_GetCode(0U); }
unsigned short SstFault_GetLevel(void) { return fault_count ? 1U : 4U; }
unsigned short SstFault_GetActiveCount(void) { return fault_count; }
unsigned short SstFault_GetHistoryCount(void) { return history_count; }
unsigned short SstFault_GetLegacyHistoryWord(unsigned short index) { (void)index; return 0U; }
unsigned short SstFault_GetHistory(unsigned short index, SST_FAULT_RECORD *record)
{ if(index >= history_count || !record) return 0U; *record = history[index]; return 1U; }

static void Stopped(void)
{
    memset((void *)&g_chb_dab_status, 0, sizeof(g_chb_dab_status));
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = g_chb_dab_command.fault_reset = 0U;
}
static uint16_t RequestCrcReference(const uint16_t *words)
{
    uint16_t crc = 0xFFFFU;
    unsigned i, b;
    for(i = 0U; i < 24U; ++i)
    {
        crc ^= (uint16_t)((words[i / 2U] >> ((i % 2U) * 8U)) & 0xFFU);
        for(b = 0U; b < 8U; ++b) crc = (uint16_t)((crc & 1U) ? (crc >> 1) ^ 0xA001U : crc >> 1);
    }
    return crc;
}
static uint16_t ImageCrcReference(const uint8_t *bytes, unsigned count)
{
    uint16_t crc = 0xFFFFU;
    unsigned i, b;
    for(i = 0U; i < count; ++i)
    {
        crc ^= bytes[i];
        for(b = 0U; b < 8U; ++b) crc = (uint16_t)((crc & 1U) ? (crc >> 1) ^ 0xA001U : crc >> 1);
    }
    return crc;
}
static unsigned Changed(const uint16_t *before, const volatile uint16_t *after, unsigned count)
{
    unsigned i, changed = 0U;
    for(i = 0U; i < count; ++i) if(before[i] != after[i]) ++changed;
    return changed;
}
static void CopyWords(uint16_t *target, const volatile uint16_t *source, unsigned count)
{ unsigned i; for(i = 0U; i < count; ++i) target[i] = source[i]; }
static void StepBudget(void)
{
    uint16_t before_response[128], before_telemetry[128], before_history[512];
    unsigned changed;
    CopyWords(before_response, DPRAM_SstResponse, 128U);
    CopyWords(before_telemetry, DPRAM_SstTelemetry, 128U);
    CopyWords(before_history, DPRAM_SstHistory, 512U);
    SstDpramMaintenance_Service();
    changed = Changed(before_response, DPRAM_SstResponse, 128U) +
              Changed(before_telemetry, DPRAM_SstTelemetry, 128U) +
              Changed(before_history, DPRAM_SstHistory, 512U);
    CHECK(changed <= 18U); /* Sixteen data words plus begin/commit version stamps. */
}
static void FinishPublication(void)
{ unsigned i; for(i = 0U; i < 64U; ++i) StepBudget(); }
static void Frame(uint16_t *data, uint16_t sequence, uint16_t op, uint16_t point, uint16_t value)
{
    memset(data, 0, 16U * sizeof(*data));
    data[0] = SST_DPRAM_MAGIC; data[1] = SST_DPRAM_VERSION;
    data[2] = data[13] = sequence; data[3] = op; data[4] = point; data[5] = value;
    data[14] = SST_DPRAM_COMMIT;
}
static uint16_t SendFrame(uint16_t *data, unsigned regenerate_crc)
{
    unsigned i, before = mailbox_accesses;
    if(regenerate_crc) data[12] = RequestCrcReference(data);
    for(i = 0U; i < 16U; ++i) DPRAM_SstRequest[i] = data[i];
    SstDpramMaintenance_Request(); StepBudget();
#if SST_ENABLE_DPRAM
    CHECK(DPRAM_SstResponse[0] & 1U); /* Partial responses are visibly invalid. */
    CHECK(mailbox_accesses == before); /* No ACK before the complete response. */
#else
    (void)before;
#endif
    FinishPublication();
#if SST_ENABLE_DPRAM
    CHECK(DPRAM_SstResponse[0] && !(DPRAM_SstResponse[0] & 1U));
    CHECK(DPRAM_SstResponse[1] == SST_DPRAM_MAGIC && DPRAM_SstResponse[2] == SST_DPRAM_VERSION);
    CHECK(DPRAM_SstResponse[3] == data[2]);
    CHECK(mailbox_accesses == before + 1U && mailbox == SST_DPRAM_MAINTENANCE_BLOCK);
#endif
    return DPRAM_SstResponse[4];
}
static void FinishSave(void)
{
    unsigned i;
    for(i = 0U; i < 240U; ++i)
    {
        unsigned before = eeprom_writes;
        ++g_chb_dab_status.tick_10ms; SstParameterStore_Service();
        CHECK(eeprom_writes - before <= 1U);
        SstParameterStore_Service(); CHECK(eeprom_writes - before <= 1U);
    }
    CHECK(!SstParameterStore_IsBusy() && SstParameterStore_GetResult() == SST_PARAM_OK);
}
static void CheckLegacy(void)
{
    uint16_t snapshot[8], before;
    unsigned i;
    Stopped();
    for(i = 0U; i < 8U; ++i) snapshot[i] = SstLegacyParameters_Read((uint16_t)i);
    CHECK(snapshot[0] == 70U && snapshot[6] == 3500U && snapshot[7] == 7800U);
    CHECK(SstLegacyParameters_ValidateSnapshot(snapshot) == SST_PARAM_OK);
    CHECK(SstLegacyParameters_ValidateSnapshot(NULL) == SST_PARAM_BAD_ADDRESS);
    before = OC_level_SET;
    CHECK(SstLegacyParameters_Write(0U, 0U) == SST_PARAM_RANGE && OC_level_SET == before);
    CHECK(SstLegacyParameters_Write(8U, 1U) == SST_PARAM_BAD_ADDRESS);
    CHECK(SstLegacyParameters_Write(6U, 7800U) == SST_PARAM_RANGE && HA_L_Voltage_Precharge == 350.0F);
    CHECK(SstLegacyParameters_Write(7U, 3000U) == SST_PARAM_RANGE && HA_H_Voltage == 780.0F);
    CHECK(SstLegacyParameters_Write(1U, 0U) == SST_PARAM_OK && OC_time_SET == 0U);
    SstLegacyParameters_ApplySnapshot(snapshot);
    CHECK(SstLegacyParameters_Write(1U, 65535U) == SST_PARAM_RANGE && OC_time_SET == snapshot[1]);
    CHECK(SstLegacyParameters_Write(3U, 65535U) == SST_PARAM_RANGE && OC_time_SET_L2 == snapshot[3]);
    CHECK(SstLegacyParameters_Write(5U, 65535U) == SST_PARAM_RANGE && OV_time_SET == snapshot[5]);
    CHECK(SstLegacyParameters_Write(1U, 65534U) == SST_PARAM_OK && OC_time_SET == 65534U);
    SstLegacyParameters_ApplySnapshot(snapshot);
    for(i = 1U; i <= 5U; i += 2U)
    {
        uint16_t invalid[8];
        memcpy(invalid, snapshot, sizeof(invalid)); invalid[i] = 65535U;
        CHECK(SstLegacyParameters_ValidateSnapshot(invalid) == SST_PARAM_RANGE);
        CHECK(OC_time_SET == snapshot[1] && OC_time_SET_L2 == snapshot[3] && OV_time_SET == snapshot[5]);
    }
    g_chb_dab_status.state = CHB_STATE_RUNNING;
    CHECK(SstLegacyParameters_Write(0U, 71U) == SST_PARAM_BUSY && OC_level_SET == before);
    Stopped(); g_chb_dab_command.run_enable = 1U;
    CHECK(SstLegacyParameters_Write(0U, 71U) == SST_PARAM_BUSY);
    Stopped(); g_chb_dab_status.dab_pwm_enabled = 1U;
    CHECK(SstLegacyParameters_Write(0U, 71U) == SST_PARAM_BUSY);
    Stopped(); HA_H_Voltage = INFINITY; CHECK(SstLegacyParameters_Read(7U) == 65535U);
    HA_H_Voltage = NAN; CHECK(SstLegacyParameters_Read(7U) == 0U);
    SstLegacyParameters_ApplySnapshot(snapshot);
}

int main(void)
{
    uint16_t data[16];
    unsigned i;
    CheckLegacy();
    memset(eeprom, 0xFF, sizeof(eeprom)); Stopped();
    for(i = 0U; i < 128U; ++i) DPRAM_SstRequest[i] = 0x55AAU;
    SstDpramMaintenance_Init();
    for(i = 0U; i < 128U; ++i) CHECK(DPRAM_SstRequest[i] == 0x55AAU); /* Initialization never overwrites host input. */
#if !SST_ENABLE_DPRAM
    for(i = 0U; i < 128U; ++i) DPRAM_SstResponse[i] = DPRAM_SstTelemetry[i] = 0xCAFEU;
    for(i = 0U; i < 512U; ++i) DPRAM_SstHistory[i] = 0xCAFEU;
    Frame(data, 1U, 2U, 0U, 0U); data[6] = 1U; SendFrame(data, 1U);
    SstDpramMaintenance_Init(); SstDpramMaintenance_Publish(); FinishPublication();
    CHECK(!mailbox_accesses && !remote_calls && !g_chb_dab_command.run_enable);
    for(i = 0U; i < 128U; ++i) CHECK(DPRAM_SstResponse[i] == 0xCAFEU && DPRAM_SstTelemetry[i] == 0xCAFEU);
    for(i = 0U; i < 512U; ++i) CHECK(DPRAM_SstHistory[i] == 0xCAFEU);
    puts("PASS: disabled DPRAM has no MMIO or fixed-array writes; real legacy bounds and stopped interlocks");
#else
    Frame(data, 1U, 0U, 46U, 0U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && DPRAM_SstResponse[7] == 70U);
    CHECK(DPRAM_SstResponse[11] == 54U && DPRAM_SstResponse[32U + 52U] == 3500U);
    /* Corrupted frames never reach the parameter/command service. */
    Frame(data, 2U, 1U, 46U, 72U); data[12] = (uint16_t)(RequestCrcReference(data) ^ 1U);
    CHECK(SendFrame(data, 0U) == SST_DPRAM_FRAME_ERROR && OC_level_SET == 70U);
    Frame(data, 2U, 1U, 46U, 72U); data[0] ^= 1U;
    CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR);
    Frame(data, 2U, 1U, 46U, 72U); data[1] = 2U;
    CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR);
    Frame(data, 2U, 1U, 46U, 72U); data[13] = 3U;
    CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR);
    Frame(data, 2U, 1U, 46U, 72U); data[14] = 0U;
    CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR);
    Frame(data, 2U, 1U, 46U, 72U); data[11] = 1U;
    CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR);
    Frame(data, 0U, 1U, 46U, 72U);
    CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR && OC_level_SET == 70U);
#if SST_ALLOW_REMOTE_COMMANDS
    {
        uint16_t good[16];
        unsigned before;
        Frame(good, 3U, 2U, 0U, 0U); good[6] = 1U;
        CHECK(SendFrame(good, 1U) == SST_PARAM_OK && remote_calls == 1U);
        Stopped(); CHECK(SendFrame(good, 1U) == SST_PARAM_OK && remote_calls == 1U && !g_chb_dab_command.run_enable);
        memcpy(data, good, sizeof(data)); data[6] = 0U;
        CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR && remote_calls == 1U);
        /* An invalid conflicting retry must not poison the valid request's cached result. */
        CHECK(SendFrame(good, 1U) == SST_PARAM_OK && remote_calls == 1U);
        memcpy(data, good, sizeof(data)); data[12] ^= 1U;
        CHECK(SendFrame(data, 0U) == SST_DPRAM_FRAME_ERROR && remote_calls == 1U);
        CHECK(SendFrame(good, 1U) == SST_PARAM_OK && remote_calls == 1U);
        memcpy(data, good, sizeof(data)); data[2] = data[13] = 0U;
        CHECK(SendFrame(data, 1U) == SST_DPRAM_FRAME_ERROR && remote_calls == 1U);
        CHECK(SendFrame(good, 1U) == SST_PARAM_OK && remote_calls == 1U);
        before = remote_calls;
        Frame(data, 4U, 2U, 0U, 0U); data[6] = data[7] = data[10] = 1U; data[8] = 3U;
        CHECK(SendFrame(data, 1U) == SST_PARAM_OK && remote_calls == before);
        CHECK(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable && !g_chb_dab_command.fault_reset);
        Frame(data, 5U, 2U, 0U, 0U); data[8] = data[9] = 3U;
        CHECK(SendFrame(data, 1U) == SST_PARAM_OK && !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
        Frame(data, 50U, 2U, 0U, 0U); data[8] = data[9] = 3U; data[10] = 1U;
        CHECK(SendFrame(data, 1U) == SST_PARAM_OK && g_chb_dab_command.fault_reset);
        CHECK(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
        Stopped();
        Frame(data, 6U, 2U, 0U, 0U); data[8] = 4U;
        CHECK(SendFrame(data, 1U) == SST_PARAM_RANGE);
    }
    /* Real modern table and actual legacy objects, including cross-field limits. */
    Frame(data, 7U, 1U, 24U, 0xFED4U); /* voltage Kp, signed -300 / 100000. */
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && fabsf(g_chb_dab_params.voltage_kp + 0.003F) < 0.0000001F);
    Frame(data, 8U, 1U, 9U, 1000U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_READ_ONLY);
    Frame(data, 9U, 1U, 36U, 0U); /* feedback sign cannot be zero. */
    CHECK(SendFrame(data, 1U) == SST_PARAM_RANGE);
    Frame(data, 10U, 1U, 52U, 8000U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_RANGE);
    Frame(data, 11U, 1U, 54U, 1U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_BAD_ADDRESS);
    Frame(data, 12U, 1U, 46U, 72U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && OC_level_SET == 72U && DPRAM_SstResponse[7] == 72U);
    g_chb_dab_status.state = CHB_STATE_PRECHARGE;
    Frame(data, 13U, 1U, 46U, 73U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY && OC_level_SET == 72U);
    Frame(data, 14U, 1U, 2U, 0U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY);
    Frame(data, 15U, 1U, 3U, 1U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY && !g_chb_dab_command.fault_reset);
    Frame(data, 16U, 3U, 0U, SST_DPRAM_COMMIT);
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY);
    Stopped();
    /* Saving is asynchronous, consumes one byte per tick, and duplicate SAVE is idempotent. */
    Frame(data, 17U, 3U, 0U, SST_DPRAM_COMMIT);
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && SstParameterStore_IsBusy());
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && !eeprom_writes);
    Frame(data, 18U, 1U, 46U, 73U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY && OC_level_SET == 72U);
    Frame(data, 19U, 1U, 0U, 1U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY && !g_chb_dab_command.run_enable);
    Frame(data, 20U, 2U, 0U, 0U); data[6] = data[10] = 1U; data[8] = 3U;
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY && !g_chb_dab_command.fault_reset);
    Frame(data, 201U, 2U, 0U, 0U); data[8] = data[9] = 3U; data[10] = 1U;
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY && !g_chb_dab_command.fault_reset);
    CHECK(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    Frame(data, 200U, 2U, 0U, 0U); data[8] = 3U; data[9] = 1U;
    CHECK(SendFrame(data, 1U) == SST_PARAM_BUSY && g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    Frame(data, 21U, 2U, 0U, 0U); data[7] = 1U;
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    FinishSave(); CHECK(eeprom_writes == 109U);
    OC_level_SET = 77U;
    {
        uint8_t saved_image[108];
        memcpy(saved_image, &eeprom[6U * 64U], sizeof(saved_image));
        g_chb_dab_params.voltage_kp = 0.0F;
        for(i = 1U; i <= 5U; i += 2U)
        {
            unsigned offset = 6U + 2U * (42U + i);
            uint16_t crc;
            memcpy(&eeprom[6U * 64U], saved_image, sizeof(saved_image));
            eeprom[6U * 64U + offset] = eeprom[6U * 64U + offset + 1U] = 0xFFU;
            crc = ImageCrcReference(&eeprom[6U * 64U], 106U);
            eeprom[6U * 64U + 106U] = (uint8_t)crc;
            eeprom[6U * 64U + 107U] = (uint8_t)(crc >> 8);
            Frame(data, (uint16_t)(400U + i), 4U, 0U, SST_DPRAM_COMMIT);
            CHECK(SendFrame(data, 1U) == SST_PARAM_RANGE);
            CHECK(OC_level_SET == 77U && g_chb_dab_params.voltage_kp == 0.0F); /* No partial mirror apply. */
        }
        memcpy(&eeprom[6U * 64U], saved_image, sizeof(saved_image));
    }
    Frame(data, 22U, 4U, 0U, SST_DPRAM_COMMIT);
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && OC_level_SET == 72U &&
          fabsf(g_chb_dab_params.voltage_kp + 0.003F) < 0.0000001F);
    Frame(data, 23U, 1U, 3U, 1U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_OK && g_chb_dab_command.fault_reset);
    Stopped();
#else
    Frame(data, 3U, 1U, 46U, 72U);
    CHECK(SendFrame(data, 1U) == SST_PARAM_DENIED && OC_level_SET == 70U);
    Frame(data, 4U, 2U, 0U, 0U); data[6] = 1U;
    CHECK(SendFrame(data, 1U) == SST_PARAM_DENIED && !remote_calls && !g_chb_dab_command.run_enable);
    Frame(data, 5U, 3U, 0U, SST_DPRAM_COMMIT);
    CHECK(SendFrame(data, 1U) == SST_PARAM_DENIED && !SstParameterStore_IsBusy());
#endif
    /* Fault/history is supplied by the report boundary; real serializer must preserve all fields. */
    fault_count = history_count = 32U;
    for(i = 0U; i < 32U; ++i)
    {
        SST_FAULT_RECORD *r = &history[i];
        memset(r, 0, sizeof(*r)); fault_codes[i] = r->code = (uint16_t)(29103U + i);
        r->level = 1U; r->control_fault = 3U; r->port_fault = 0x1234U;
        r->fault_type = 0x5678U; r->port_latch = 0x1111U; r->fb_latch = 0x2222U;
        r->fo_a = 0x3333U; r->fo_b = 0x4444U; r->fo_c = 0x5555U; r->module_dispose = 0x6666U;
        r->tick_10ms = 0x12345678UL + i; r->sequence = 0x87654321UL - i;
    }
    g_chb_dab_status.state = CHB_STATE_FAULT; g_chb_dab_status.fault_code = CHB_FAULT_AC_OVERCURRENT;
    g_chb_dab_status.tick_10ms = 0x1234ABCDUL; g_chb_dab_status.startup_tad_count = 0x7890EF12UL;
    g_chb_dab_status.ac_input_closed = g_chb_dab_status.precharge_bypassed = 1U;
    g_chb_dab_status.pll_locked = g_chb_dab_status.voltage_loop_active = 1U;
    g_chb_dab_status.dc_voltage_average_v = 710.25F; g_chb_dab_status.dc_voltage_min_v = 699.25F;
    g_chb_dab_status.id_feedback_a = -12.34F; g_chb_dab_status.iq_feedback_a = 400.0F;
    SstDpramMaintenance_Publish();
    CHECK((DPRAM_SstTelemetry[0] & 1U) && (DPRAM_SstHistory[0] & 1U));
    FinishPublication();
    CHECK(!(DPRAM_SstTelemetry[0] & 1U) && !(DPRAM_SstHistory[0] & 1U));
    CHECK(DPRAM_SstTelemetry[3] == CHB_STATE_FAULT && DPRAM_SstTelemetry[4] == CHB_FAULT_AC_OVERCURRENT);
    CHECK(DPRAM_SstTelemetry[6] == (1U | 2U | 16U | 32U | 64U));
    CHECK(DPRAM_SstTelemetry[8] == 29103U && DPRAM_SstTelemetry[10] == 32U);
    CHECK(DPRAM_SstTelemetry[12] == 0xABCDU && DPRAM_SstTelemetry[13] == 0x1234U);
    CHECK(DPRAM_SstTelemetry[14] == 0xEF12U && DPRAM_SstTelemetry[15] == 0x7890U);
    CHECK(DPRAM_SstTelemetry[16] == 7103U && DPRAM_SstTelemetry[17] == 6993U);
    CHECK(DPRAM_SstTelemetry[20] == (uint16_t)(int16_t)-1234 && DPRAM_SstTelemetry[21] == 32767U);
    CHECK(DPRAM_SstTelemetry[63] == 29134U);
    CHECK(DPRAM_SstHistory[3] == 32U && DPRAM_SstHistory[4] == 15U);
    CHECK(DPRAM_SstHistory[5] == 0x4321U && DPRAM_SstHistory[6] == 0x8765U);
    CHECK(DPRAM_SstHistory[16] == 29103U && DPRAM_SstHistory[26] == 0x6666U);
    CHECK(DPRAM_SstHistory[27] == 0x5678U && DPRAM_SstHistory[28] == 0x1234U);
    CHECK(DPRAM_SstHistory[29] == 0x4321U && DPRAM_SstHistory[30] == 0x8765U);
    CHECK(DPRAM_SstHistory[16U + 31U * 15U] == 29134U);
    CHECK(DPRAM_SstHistory[496] == 0U && DPRAM_SstHistory[511] == 0U);
    {
        uint16_t history_epoch = DPRAM_SstHistory[0];
        SstDpramMaintenance_Publish(); FinishPublication();
        CHECK(DPRAM_SstHistory[0] == history_epoch); /* Stable history is not republished. */
        ++history[0].sequence; SstDpramMaintenance_Publish(); FinishPublication();
        CHECK(DPRAM_SstHistory[0] != history_epoch && !(DPRAM_SstHistory[0] & 1U));
    }
#if SST_ALLOW_REMOTE_COMMANDS
    puts("PASS: real DPRAM CRC/commit/sequence checks, owner permissions, idempotency, stop/off priority, stopped/busy bounds, actual parameter readback, shared EEPROM, sliced epochs and 32 packed fault histories");
#else
    puts("PASS: simulator denies parallel mutations but serves reads, telemetry and 32 packed histories; CRC/commit guards and sliced publication budgets");
#endif
#endif
    return 0;
}
