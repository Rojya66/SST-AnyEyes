/* Exercise the production host dispatcher with in-memory mailbox, control and EEPROM fixtures. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ChbBuildConfig.h"
#include "DPRAM.H"
#include "hw_types.h"

static uint16_t left_mailbox, right_mailbox;
static unsigned left_reads, right_writes;

static volatile uint16_t *Mailbox(unsigned address)
{
    if (address == DPRAM_Maibox_Left)
    {
        ++left_reads;
        return &left_mailbox;
    }
    assert(address == DPRAM_Maibox_Right);
    ++right_writes;
    return &right_mailbox;
}

#undef HWREGH
#define HWREGH(address) (*Mailbox(address))
/* Include the real implementation so the test can act as ARM on its private DPRAM backing words. */
#include "SstDpramHost.c"

#if SST_ENABLE_DPRAM
volatile CHB_DAB_COMMAND g_chb_dab_command;
volatile CHB_DAB_STATUS g_chb_dab_status;
volatile FPGA_InterfaceBLOCK FPGA_Interface[15];
XXG_PortRunFLAG PortRunFLAG;
volatile uchar Flag_RemoteControlRegulate;
ushort rtt;
ushort HA_PM_A1 = 1U, HA_PM_A2 = 1U, HA_PM_B1 = 2U, HA_PM_B2, HA_PM_C1, HA_PM_C2 = 4U;
static uint16_t parameter_value = 6800U;
static uint16_t storage_busy, storage_result;
static uint16_t telemetry_ready;
static unsigned switch_calls, eeprom_requests;
static uint16_t switch_close, switch_open;

unsigned short ChbDabControl_MaintenanceSwitch(unsigned short close, unsigned short open)
{
    ++switch_calls;
    switch_close = close;
    switch_open = open;
    return SstCommand_IsStopped() ? SST_PARAM_OK : SST_PARAM_BUSY;
}

uint16_t SstParameterStore_IsBusy(void) { return storage_busy; }
uint16_t SstParameterStore_GetResult(void) { return storage_result; }

void SstDpramV2Parameter_Read(uint16_t *words)
{
    memset(words, 0, SST_DPRAM_V2_PARAMETER_WORDS * sizeof(*words));
    words[0x26U] = parameter_value;
}

uint16_t SstDpramV2Parameter_Apply(const uint16_t *words, uint16_t *error_addr)
{
    assert(DPRAM_V2_Words[0x050BU] == 1U);
    if (words[0x26U] == 0U)
    {
        *error_addr = 0x0226U;
        return SST_PARAM_RANGE;
    }
    parameter_value = words[0x26U];
    return SST_PARAM_OK;
}

uint16_t SstDpramV2Parameter_Store(uint16_t load, uint16_t *error_addr)
{
    (void)error_addr;
    assert(DPRAM_V2_Words[0x050BU] == 1U);
    ++eeprom_requests;
    if (load != 0U)
        parameter_value = 6800U;
    else
        storage_busy = 1U;
    return SST_PARAM_OK;
}

void SstDpramV2Telemetry_Init(void) {}
void SstDpramV2Telemetry_Service(void) {}
uint16_t SstDpramV2Telemetry_TakeLiveNotification(void)
{
    uint16_t ready = telemetry_ready;
    telemetry_ready = 0U;
    return ready;
}

static void Send(uint16_t block)
{
    unsigned before = left_reads;
    left_mailbox = block;
    Flag_RemoteControlRegulate = 1U;
    SstDpramHost_Service();
    assert(left_reads == before + 1U && Flag_RemoteControlRegulate == 0U);
    assert(FPGA_Interface[MainControl_PluginUnit_4 - 2U].LED.Bbits.led3 == rtt);
}

static void Control(uint16_t op, uint16_t result)
{
    uint16_t count = DPRAM_V2_Words[0x0504U];
    DPRAM_V2_Words[0x0100U] = op;
    Send(1U);
    assert(DPRAM_V2_Words[0x0100U] == 0U);
    assert(DPRAM_V2_Words[0x0504U] == (uint16_t)(count + 1U));
    assert(DPRAM_V2_Words[0x0505U] == op && DPRAM_V2_Words[0x0506U] == result);
}

static void CheckParameter(uint16_t op, uint16_t result, uint16_t count, uint16_t error_addr)
{
    assert(DPRAM_V2_Words[0x01FFU] == 0U && DPRAM_V2_Words[0x0507U] == count);
    assert(DPRAM_V2_Words[0x0508U] == op && DPRAM_V2_Words[0x0509U] == result);
    assert(DPRAM_V2_Words[0x050AU] == error_addr && DPRAM_V2_Words[0x050BU] == 0U);
    assert(DPRAM_V2_Words[0x0226U] == parameter_value && right_mailbox == 20U);
}

#if SST_ALLOW_REMOTE_COMMANDS
static void CheckBridgeAndSwitchCommands(void)
{
    Control(3U, SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    Control(1U, SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    Control(2U, SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    Control(4U, SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    Control(7U, SST_PARAM_DENIED);
    assert(!switch_calls);
    Control(11U, SST_PARAM_OK);
    assert(SstDpramV2_SwitchPermission() == 1U);
    Control(7U, SST_PARAM_OK);
    assert(switch_calls == 1U && switch_close == 2U && switch_open == 0U);
    Control(9U, SST_PARAM_OK);
    assert(switch_close == 1U && switch_open == 0U);
    Control(10U, SST_PARAM_OK);
    assert(switch_close == 0U && switch_open == 1U);
    Control(8U, SST_PARAM_OK);
    assert(switch_close == 0U && switch_open == 2U);
    PortRunFLAG.Bbits.Port_AutoOrManual = 1U;
    Control(7U, SST_PARAM_DENIED);
    Control(8U, SST_PARAM_DENIED);
    assert(switch_calls == 4U);
    PortRunFLAG.Bbits.Port_AutoOrManual = 0U;
    Control(12U, SST_PARAM_OK);
    assert(SstDpramV2_SwitchPermission() == 0U);
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    Control(1U, SST_PARAM_DENIED);
    Control(6U, SST_PARAM_OK);
    assert(g_chb_dab_command.fault_reset == 1U);
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
}

static void CheckObservationWhileRunning(void)
{
    g_chb_dab_status.state = CHB_STATE_RUNNING;
    DPRAM_V2_Words[0x0101U] = 0x0100U;
    DPRAM_V2_Words[0x0102U] = 0x0002U;
    DPRAM_V2_Words[0x0103U] = 0x0008U;
    Control(13U, SST_PARAM_OK);
    assert(SstDpramV2_ObservationMask(0U) == 0x0100U);
    assert(SstDpramV2_ObservationMask(1U) == 0x0002U);
    assert(SstDpramV2_ObservationMask(2U) == 0x0008U);
    DPRAM_V2_Words[0x0101U] = 1U;
    DPRAM_V2_Words[0x0103U] = 0x4000U;
    Control(13U, SST_PARAM_RANGE);
    assert(SstDpramV2_ObservationMask(0U) == 0x0100U);
    assert(SstDpramV2_ObservationMask(2U) == 0x0008U);
    assert(HA_PM_A1 == 1U && HA_PM_A2 == 1U && HA_PM_B1 == 2U && HA_PM_C2 == 4U);
    g_chb_dab_status.state = CHB_STATE_STOPPED;
}

static void CheckSave(uint16_t result, uint16_t previous_count)
{
    unsigned before = right_writes;
    DPRAM_V2_Words[0x01FFU] = 2U;
    Send(2U);
    assert(DPRAM_V2_Words[0x050BU] == 1U);
    assert(DPRAM_V2_Words[0x0507U] == previous_count && right_writes == before);

    Control(3U, SST_PARAM_BUSY);
    assert(!g_chb_dab_command.dab_enable);
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    Control(5U, SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(switch_close == 0U && switch_open == 3U);

    telemetry_ready = 1U;
    SstDpramHost_Service();
    assert(right_writes == before + 1U && right_mailbox == 3U);
    assert(DPRAM_V2_Words[0x0507U] == previous_count);

    storage_busy = 0U;
    storage_result = result;
    telemetry_ready = 1U;
    SstDpramHost_Service();
    CheckParameter(2U, result, previous_count + 1U, 0xFFFFU);
    assert(right_writes == before + 2U);
    SstDpramHost_Service();
    assert(right_writes == before + 2U); /* No pending live notice or repeated reply. */
}

static void CheckParameterTransactions(void)
{
    unsigned before = right_writes;
    DPRAM_V2_Words[0x0226U] = 7050U;
    DPRAM_V2_Words[0x01FFU] = 1U;
    SstDpramHost_Service();
    assert(DPRAM_V2_Words[0x01FFU] == 1U && right_writes == before);
    telemetry_ready = 1U;
    Send(2U);
    CheckParameter(1U, SST_PARAM_OK, 1U, 0xFFFFU);
    assert(parameter_value == 7050U && right_writes == before + 1U);

    DPRAM_V2_Words[0x0226U] = 0U;
    DPRAM_V2_Words[0x01FFU] = 1U;
    Send(2U);
    CheckParameter(1U, SST_PARAM_RANGE, 2U, 0x0226U);
    assert(DPRAM_V2_Words[0x0226U] == 7050U);

    CheckSave(SST_PARAM_OK, 2U);
    CheckSave(SST_PARAM_STORAGE, 3U);
    assert(eeprom_requests == 2U);

    DPRAM_V2_Words[0x01FFU] = 3U;
    Send(2U);
    CheckParameter(3U, SST_PARAM_OK, 5U, 0xFFFFU);
    assert(parameter_value == 6800U && eeprom_requests == 3U);

    DPRAM_V2_Words[0x0226U] = 1U;
    DPRAM_V2_Words[0x01FFU] = 4U;
    Send(2U);
    CheckParameter(4U, SST_PARAM_OK, 6U, 0xFFFFU);
}
#endif
#endif

int main(void)
{
#if SST_ENABLE_DPRAM
    DPRAM_V2_Words[0x0500U] = 0x5353U;
    DPRAM_V2_Words[0x0100U] = 3U;
    DPRAM_V2_Words[0x01FFU] = 1U;
    SstDpramHost_Invalidate();
    assert(DPRAM_V2_Words[0x0500U] == 0U);
    SstDpramHost_Initialize();
    assert(left_reads == 1U && right_writes == 1U);
    assert(DPRAM_V2_Words[0x0500U] == 0x5353U && DPRAM_V2_Words[0x0501U] == 0x0200U);
    assert(DPRAM_V2_Words[0x0100U] == 0U && DPRAM_V2_Words[0x0504U] == 0U);
    assert(SstDpramV2_ObservationMask(0U) == 0x0101U);
    assert(SstDpramV2_ObservationMask(1U) == (SST_PHASE_A_ONLY_TEST ? 0U : 2U));
    assert(SstDpramV2_ObservationMask(2U) == (SST_PHASE_A_ONLY_TEST ? 0U : 0x0400U));
    CheckParameter(0U, SST_PARAM_OK, 0U, 0xFFFFU);
#if SST_ALLOW_REMOTE_COMMANDS
    CheckBridgeAndSwitchCommands();
    CheckObservationWhileRunning();
    CheckParameterTransactions();
    puts("PASS: V2 host boot, mailbox consume, bridge/switch requests, observation, actual replies, save completion and mailbox priority");
#else
    Control(1U, SST_PARAM_DENIED);
    assert(!g_chb_dab_command.run_enable);
    DPRAM_V2_Words[0x0226U] = 1U;
    DPRAM_V2_Words[0x01FFU] = 1U;
    Send(2U);
    CheckParameter(1U, SST_PARAM_DENIED, 1U, 0xFFFFU);
    DPRAM_V2_Words[0x01FFU] = 4U;
    Send(2U);
    CheckParameter(4U, SST_PARAM_OK, 2U, 0xFFFFU);
    puts("PASS: V2 simulator ownership denies mutations and allows readback");
#endif
#else
    (void)Mailbox;
    SstDpramHost_Invalidate();
    SstDpramHost_Initialize();
    SstDpramHost_Service();
    assert(!left_reads && !right_writes);
    puts("PASS: DPRAM-disabled host leaves both mailboxes unaccessed");
#endif
    return 0;
}
