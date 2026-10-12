/* Compile the actual DPRAM command adapter with fake control/storage services. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstCommand.h"

volatile CHB_DAB_COMMAND g_chb_dab_command;
volatile CHB_DAB_STATUS g_chb_dab_status;
static unsigned short storage_busy;
static unsigned int remote_calls;
unsigned short SstParams_IsStorageBusy(void) { return storage_busy; }
unsigned short SstParameterStore_IsBusy(void) { return storage_busy; }
unsigned short SstParams_IsStopped(void) {
    return (unsigned short)((g_chb_dab_status.state == CHB_STATE_STOPPED ||
                           g_chb_dab_status.state == CHB_STATE_FAULT) &&
           !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable &&
           !g_chb_dab_status.ac_pwm_enabled && !g_chb_dab_status.dab_pwm_enabled);
}
void ChbDabControl_ApplyRemoteCommand(unsigned short start, unsigned short stop) {
    ++remote_calls;
    if(stop & 1U) g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 0U;
    else if(start & 1U) g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
}
/* INSERT_COMMAND_IMPLEMENTATION */

static void reset_state(void) {
    memset((void *)&g_chb_dab_command, 0, sizeof(g_chb_dab_command));
    memset((void *)&g_chb_dab_status, 0, sizeof(g_chb_dab_status));
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    storage_busy = 0U; remote_calls = 0U;
}
int main(void) {
    reset_state();
#if SST_ALLOW_REMOTE_COMMANDS && SST_ENABLE_DPRAM
    SstCommand_ApplyDpram(1, 0, 0, 0, 0);
    assert(remote_calls == 1 && g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    SstCommand_ApplyDpram(1, 1, 3, 0, 1);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(!g_chb_dab_command.fault_reset); /* stop dominates all other actions */

    reset_state(); storage_busy = 1U;
    SstCommand_ApplyDpram(1, 0, 3, 0, 1);
    assert(!remote_calls && !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(!g_chb_dab_command.fault_reset);
    /* Model an existing run request; stop/off must not be blocked by storage. */
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    SstCommand_ApplyDpram(1, 0, 3, 1, 0);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    SstCommand_ApplyDpram(1, 0, 3, 2, 0);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);

    reset_state();
    SstCommand_ApplyDpram(0, 0, 1, 0, 0);
    assert(!g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    SstCommand_ApplyDpram(0, 0, 2, 1, 0);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    SstCommand_ApplyDpram(0, 0, 3, 3, 0);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);

    reset_state();
    SstCommand_ApplyDpram(0, 0, 0, 0, 0x8000U);
    assert(!g_chb_dab_command.fault_reset); /* fault reset is bit0 only */
    SstCommand_ApplyDpram(1, 0, 0, 0x8000U, 0);
    assert(g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    reset_state();
    SstCommand_ApplyDpram(0x8000U, 0x8000U, 0x8000U, 0x8000U, 0);
    assert(!remote_calls && !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);

    reset_state(); g_chb_dab_status.state = CHB_STATE_FAULT;
    SstCommand_ApplyDpram(0, 0, 0, 0, 1);
    assert(g_chb_dab_command.fault_reset);
    reset_state(); g_chb_dab_status.state = CHB_STATE_RUNNING;
    g_chb_dab_status.ac_pwm_enabled = 1U;
    SstCommand_ApplyDpram(0, 0, 0, 0, 1);
    assert(!g_chb_dab_command.fault_reset);
    reset_state(); g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.dab_pwm_enabled = 1U;
    SstCommand_ApplyDpram(0, 0, 0, 0, 1);
    assert(!g_chb_dab_command.fault_reset);
    puts("PASS: DPRAM owner, stop/off priority, storage lock, independent gates, reserved bits, stopped reset");
#else
    SstCommand_ApplyDpram(1, 0, 3, 0, 1);
    assert(!remote_calls && !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(!g_chb_dab_command.fault_reset);
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    SstCommand_ApplyDpram(0, 1, 0, 3, 1);
    assert(!remote_calls && g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    assert(!g_chb_dab_command.fault_reset);
    puts("PASS: non-owner DPRAM commands cannot mutate simulator/serial commands");
#endif
    return 0;
}
