/* Host-only hardware substitutes and behavioral regression scenarios.
 * control_regression.py inserts the ORIGINAL or MIGRATED control source at
 * the marker below. This file is never linked into the embedded project.
 * The host ABI is not used as evidence of TI binary layout or execution time.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"

/* A failed host assertion must return diagnostics instead of opening the
 * Windows CRT assertion dialog and leaving a temporary executable running. */
#undef assert
#define assert(expression) ((expression) ? (void)0 : \
    (fprintf(stderr, "ASSERT %s:%d: %s\n", __FILE__, __LINE__, #expression), exit(2)))

typedef union {
    unsigned short DC_Log16;
    struct {
        unsigned short DC_Order : 1;
        unsigned short Reserved1 : 7;
        unsigned short IG_Orient : 1;
        unsigned short Reserved2 : 7;
    } Bbits;
} FAKE_DC_LOG;
typedef struct {
    unsigned short Reference, DAB_LEFTDuty, DAB_RIGHTDuty, DAB_Start;
    FAKE_DC_LOG DC_Log;
} FAKE_FPGA;
static FAKE_FPGA FPGA_Interface[16];
#define Com_A1_PluginUnit_5 5
#define Com_A2_PluginUnit_6 6
#define Com_B1_PluginUnit_9 9
#define Com_B2_PluginUnit_10 10
#define Com_C1_PluginUnit_13 13
#define Com_C2_PluginUnit_14 14
#define PortStatusNum_Standby 0x06U
#define PortStatusNum_Start 0x10U
#define PortStatusNum_Start_ContactorOnOver 0x14U
#define PortStatusNum_Start_PortStart 0x16U
#define PortStatusNum_Run 0x20U
#define PortStatusNum_Stop 0x0dU

static float DAC_BUF[8];
static float HA_ReferenceDEF_A, HA_ReferenceDEF_B, HA_ReferenceDEF_C;
static unsigned short HA_PM_A1, HA_PM_A2, HA_PM_B1, HA_PM_B2, HA_PM_C1, HA_PM_C2;
static unsigned short NHA_module_A, NHA_module_B, NHA_module_C;
static float HA_Port_Volt_A, HA_Port_Volt_B, HA_Port_Volt_C;
static float HA_Port_Current_A, HA_Port_Current_B, HA_Port_Current_C;
static float SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave;
static unsigned short HA_DC_Log_A, HA_DC_Log_B, HA_DC_Log_C;
static unsigned short HA_IG_Log_A, HA_IG_Log_B, HA_IG_Log_C;
static unsigned short Port_RunStatus, Port_State;
static struct { struct { unsigned short Breaker_InnerOn, Breaker_InnerOff,
    Contactor_Inner; } Bbits; } Port_DOUT;
static struct { struct { unsigned short PWMPulseState, PWMEN_FibreOptical; } Bbits; } FaultDisposeFLAG;
static struct { struct { unsigned short Port_Start, Port_Stop,
    DAB_StartMethod, SVG_PWMEN, DAB_PWMEN; } Bbits; } ControlFLAG;
static struct { struct { unsigned short P_Status; } Bbits; } Port_MainStatus;
static struct { struct { unsigned short PWMState_Dab, PWMState_50Hz; } Bbits; } PortRunFLAG;
static union { unsigned short PORT_FAULT16; struct {
    unsigned short PORT_FAULT_bit15; } Bbits; } Port_RealFault;

static uint64_t trace_hash;
static uint64_t snapshot_hash;
static unsigned long trace_steps;
static unsigned int fake_ac_pwm, fake_dab_pwm;
static unsigned int disable_calls, enable_calls, io_calls, state_calls, reset_calls;
static unsigned int module_pwm_enable_calls, module_50hz_enable_calls;
static unsigned int all_shutdown_calls, variable_reset_calls;
static unsigned int interrupt_disable_calls, interrupt_restore_calls;
static unsigned int fake_interrupt_enabled = 1U;
static unsigned int fault_before_disable;
static unsigned int fault_before_disable_call;
static unsigned int fault_before_variable_reset;
static unsigned int fault_injected_at_disable_call;
static unsigned int enable_outside_critical;
static unsigned int hardware_enable_events;
static void hash_word(uint32_t word) {
    unsigned int i;
    for(i = 0; i < 4; ++i) { trace_hash ^= (word >> (i * 8)) & 0xffU;
        trace_hash *= UINT64_C(1099511628211); }
}
static void Port_IO_Out(void) { ++io_calls; hash_word(0x100U);
    hash_word(Port_DOUT.Bbits.Breaker_InnerOn);
    hash_word(Port_DOUT.Bbits.Breaker_InnerOff);
    hash_word(Port_DOUT.Bbits.Contactor_Inner); }
static void Port_StateUpdate_Write(void) { ++state_calls; hash_word(0x200U);
    hash_word(Port_RunStatus); }
static void PWMPluse_Enable(unsigned short bridge) { ++enable_calls;
    if(bridge == 1U || bridge == 3U) fake_ac_pwm = 1U;
    if(bridge == 2U || bridge == 3U) fake_dab_pwm = 1U;
    hash_word(0x300U); hash_word(bridge); }
static void PWMPluse_Disable(unsigned short index) { ++disable_calls;
    if(FaultDisposeFLAG.Bbits.PWMPulseState & 1U) fake_ac_pwm = 0U;
    if(FaultDisposeFLAG.Bbits.PWMPulseState & 2U) fake_dab_pwm = 0U;
    if (FaultDisposeFLAG.Bbits.PWMPulseState == 3U) FaultDisposeFLAG.Bbits.PWMEN_FibreOptical = 0U;
    hash_word(0x400U); hash_word(index); hash_word(FaultDisposeFLAG.Bbits.PWMPulseState); }
static void Module_ENdab_PWMEN(unsigned short enable) { fake_dab_pwm = enable;
    if (enable != 0U) ++hardware_enable_events;
    if (enable != 0U && fake_interrupt_enabled != 0U) ++enable_outside_critical;
    hash_word(0x500U); hash_word(enable); }
static void Module_PWMEN(unsigned short enable) {
    ++module_pwm_enable_calls;
    if (enable != 0U) ++hardware_enable_events;
    if (enable != 0U && fake_interrupt_enabled != 0U) ++enable_outside_critical;
    fake_ac_pwm = (enable & 1U) != 0U;
    fake_dab_pwm = (enable & 2U) != 0U;
    hash_word(0x510U); hash_word(enable);
}
static void Module_50Hz_PWMEN(unsigned short enable) {
    ++module_50hz_enable_calls;
    if (enable != 0U) ++hardware_enable_events;
    if (enable != 0U && fake_interrupt_enabled != 0U) ++enable_outside_critical;
    fake_ac_pwm = (enable != 0U) ? 1U : 0U;
    hash_word(0x520U); hash_word(enable);
}
static void Port_DSPControl_AllShutdown(unsigned short enable) {
    ++all_shutdown_calls;
    hash_word(0x530U); hash_word(enable);
}
static void Variable_RST(void) {
    ++variable_reset_calls;
    if (fault_before_variable_reset != 0U) {
        fault_before_variable_reset = 0U;
        ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
    }
    hash_word(0x540U);
}
static unsigned int IntGlobalDisable(void) {
    unsigned int previous = fake_interrupt_enabled;
    ++interrupt_disable_calls;
    if ((fault_before_disable != 0U) ||
        ((fault_before_disable_call != 0U) &&
         (interrupt_disable_calls == fault_before_disable_call))) {
        fault_before_disable = 0U;
        fault_before_disable_call = 0U;
        fault_injected_at_disable_call = interrupt_disable_calls;
        ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
    }
    fake_interrupt_enabled = 0U;
    return previous;
}
static void IntGlobalRestore(unsigned int previous) {
    ++interrupt_restore_calls;
    fake_interrupt_enabled = previous;
}
static void FaultLatch_Clear(unsigned short a, unsigned short b) {
    ++reset_calls; hash_word(0x600U); hash_word(a); hash_word(b); }

/* Exercise the production FastStep through this harness's real fake-I/O
 * objects.  All three phases use different input combinations on each TAD,
 * preventing a copy/paste error that incorrectly reads A's flags for B/C.
 * The second cycle uses noncanonical nonzero values, matching the original
 * writer's zero/nonzero contract.  Only bit 0 and bit 8 may change. */
static void assert_dc_log_faststep_updates(void)
{
    static const unsigned short cards[3][2] = {
        {Com_A1_PluginUnit_5 - 2, Com_A2_PluginUnit_6 - 2},
        {Com_B1_PluginUnit_9 - 2, Com_B2_PluginUnit_10 - 2},
        {Com_C1_PluginUnit_13 - 2, Com_C2_PluginUnit_14 - 2}
    };
    unsigned short initial[16];
    unsigned short expected_ig[3];
    unsigned short expected_dc[3];
    unsigned short selected[16] = {0U};
    unsigned int slot;
    unsigned int sample;
    unsigned int phase;
    unsigned int card;
    unsigned short combination;
    unsigned short ig_nonzero;
    unsigned short dc_nonzero;
    unsigned short original_state = g_chb_dab_status.state;

    for (slot = 0U; slot < 16U; ++slot)
    {
        initial[slot] = (unsigned short)(0xA55AU ^ (slot * 0x0111U));
        FPGA_Interface[slot].DC_Log.DC_Log16 = initial[slot];
    }
    for (phase = 0U; phase < 3U; ++phase)
    {
        selected[cards[phase][0]] = 1U;
        selected[cards[phase][1]] = 1U;
    }
    for (sample = 0U; sample < 8U; ++sample)
    {
        for (phase = 0U; phase < 3U; ++phase)
        {
            combination = (unsigned short)((sample + phase) % 4U);
            expected_ig[phase] = (unsigned short)((combination >> 1) & 1U);
            expected_dc[phase] = (unsigned short)(combination & 1U);
        }
        ig_nonzero = (sample < 4U) ? 1U : 0x0101U;
        dc_nonzero = (sample < 4U) ? 1U : 2U;
        HA_IG_Log_A = (expected_ig[0] != 0U) ? ig_nonzero : 0U;
        HA_IG_Log_B = (expected_ig[1] != 0U) ? ig_nonzero : 0U;
        HA_IG_Log_C = (expected_ig[2] != 0U) ? ig_nonzero : 0U;
        HA_DC_Log_A = (expected_dc[0] != 0U) ? dc_nonzero : 0U;
        HA_DC_Log_B = (expected_dc[1] != 0U) ? dc_nonzero : 0U;
        HA_DC_Log_C = (expected_dc[2] != 0U) ? dc_nonzero : 0U;

        ChbDabControl_FastStep();
        assert(g_chb_dab_status.state == original_state);
        assert(g_chb_dab_status.fault_code == CHB_FAULT_NONE);
        for (phase = 0U; phase < 3U; ++phase)
        {
            for (card = 0U; card < 2U; ++card)
            {
                slot = cards[phase][card];
                assert(FPGA_Interface[slot].DC_Log.Bbits.IG_Orient == expected_ig[phase]);
                assert(FPGA_Interface[slot].DC_Log.Bbits.DC_Order ==
                       (unsigned short)((expected_dc[phase] + card) % 2U));
                assert((FPGA_Interface[slot].DC_Log.DC_Log16 & 0xFEFEU) ==
                       (initial[slot] & 0xFEFEU));
            }
        }
        for (slot = 0U; slot < 16U; ++slot)
        {
            if (selected[slot] == 0U)
            {
                assert(FPGA_Interface[slot].DC_Log.DC_Log16 == initial[slot]);
            }
        }
    }
}

/* INSERT_CONTROL_IMPLEMENTATION */

static CHB_DAB_PARAMETERS defaults;
static CHB_DAB_COMMAND command_defaults;
static void hash_float(float value) {
    uint32_t bits;
    assert(isfinite(value));
    memcpy(&bits, &value, sizeof(bits));
    hash_word(bits);
}
static void hash_snapshot_fields(void) {
    unsigned int i;
#define H(member) hash_word((uint32_t)g_chb_dab_status.member)
#define F(member) hash_float(g_chb_dab_status.member)
    H(state); H(fault_code); H(ac_input_closed); H(precharge_bypassed);
    H(ac_pwm_enabled); H(dab_pwm_enabled); H(pll_locked); H(dab_duty_count);
    H(tick_10ms); H(startup_phase); H(startup_tad_count); H(voltage_loop_active);
    F(dc_voltage_average_v); F(dc_voltage_min_v); F(pll_angle_rad);
    F(pll_frequency_hz); F(id_feedback_a); F(iq_feedback_a); F(id_reference_a);
    F(startup_id_reference_a); F(startup_voltage_reference_v); F(modulation_actual);
#undef H
#undef F
    for(i = 0; i < 16U; ++i) {
        /* Compatibility projection: these are exactly the four FPGA fields
         * present in the original trace. DC_Log was never in that trace and
         * now has independent per-TAD, six-card assertions above; adding its
         * repaired behavior to the old hash would compare against an omitted
         * writer, rather than test the existing modulation/DAB contract. */
        hash_word(FPGA_Interface[i].Reference);
        hash_word(FPGA_Interface[i].DAB_LEFTDuty);
        hash_word(FPGA_Interface[i].DAB_RIGHTDuty);
        hash_word(FPGA_Interface[i].DAB_Start);
    }
    hash_float(DAC_BUF[0]); hash_word(fake_ac_pwm); hash_word(fake_dab_pwm);
    hash_word(g_chb_dab_command.run_enable); hash_word(g_chb_dab_command.dab_enable);
}
static void snapshot(void) {
    uint64_t events_and_snapshots;

#ifdef SST_HOST_EXPECT_DC_LOG_UPDATES
    /* The repaired gate transaction changes the concrete helper calls while
     * preserving the physical outputs. Check those outputs and inverse legacy
     * running flags directly at every observation, outside the old hash. */
    assert(fake_ac_pwm == g_chb_dab_status.ac_pwm_enabled);
    assert(fake_dab_pwm == g_chb_dab_status.dab_pwm_enabled);
    assert(PortRunFLAG.Bbits.PWMState_50Hz == (unsigned short)(fake_ac_pwm == 0U));
    assert(PortRunFLAG.Bbits.PWMState_Dab == (unsigned short)(fake_dab_pwm == 0U));
    if ((fake_ac_pwm != 0U) || (fake_dab_pwm != 0U))
    {
        assert(FaultDisposeFLAG.Bbits.PWMEN_FibreOptical != 0U);
    }
    else if ((g_chb_dab_status.state == CHB_STATE_STOPPED) ||
             (g_chb_dab_status.state == CHB_STATE_FAULT))
    {
        assert(FaultDisposeFLAG.Bbits.PWMEN_FibreOptical == 0U);
    }
    assert(enable_outside_critical == 0U);
    assert(interrupt_disable_calls == interrupt_restore_calls);
#endif
    hash_snapshot_fields();
    events_and_snapshots = trace_hash;
    trace_hash = snapshot_hash;
    hash_snapshot_fields();
    snapshot_hash = trace_hash;
    trace_hash = events_and_snapshots;
    ++trace_steps;
}
static void finish(const char *scenario) {
    printf("%s steps=%lu hash=%016llx snapshot=%016llx state=%u fault=%u enables=%u disables=%u io=%u resets=%u\n",
        scenario, trace_steps, (unsigned long long)trace_hash, (unsigned long long)snapshot_hash,
        g_chb_dab_status.state, g_chb_dab_status.fault_code,
        enable_calls, disable_calls, io_calls, reset_calls);
}
static void setup(unsigned short mode) {
    trace_hash = UINT64_C(14695981039346656037); trace_steps = 0;
    snapshot_hash = UINT64_C(14695981039346656037);
    disable_calls = enable_calls = io_calls = state_calls = reset_calls = 0;
    interrupt_disable_calls = interrupt_restore_calls = 0U;
    enable_outside_critical = 0U;
    fake_interrupt_enabled = 1U;
    fake_ac_pwm = fake_dab_pwm = 0;
    memset(FPGA_Interface, 0, sizeof(FPGA_Interface));
    memset(&Port_DOUT, 0, sizeof(Port_DOUT));
    memset(&PortRunFLAG, 0, sizeof(PortRunFLAG));
    Port_RealFault.PORT_FAULT16 = 0;
    HA_PM_A1 = HA_PM_B1 = HA_PM_C1 = 1U;
    HA_PM_A2 = HA_PM_B2 = HA_PM_C2 = 0U;
    NHA_module_A = NHA_module_B = NHA_module_C = 1U;
    HA_ReferenceDEF_A = HA_ReferenceDEF_B = HA_ReferenceDEF_C = 1000.0F;
    SVG_Udc_A_Ave = SVG_Udc_B_Ave = SVG_Udc_C_Ave = 600.0F;
    HA_Port_Volt_A = HA_Port_Volt_B = HA_Port_Volt_C = 0.0F;
    HA_Port_Current_A = HA_Port_Current_B = HA_Port_Current_C = 0.0F;
    g_chb_dab_params = defaults; g_chb_dab_command = command_defaults;
    g_chb_dab_command.ac_mode = mode;
    g_chb_dab_protection_enable = 1U;
    ChbDabControl_Init(); snapshot();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
}
static void input(unsigned long n, float grid_peak, float current_peak) {
    float theta = CHB_TWO_PI * 50.0F * 0.0001F * (float)n;
    HA_Port_Volt_A = grid_peak * cosf(theta);
    HA_Port_Volt_B = grid_peak * cosf(theta - CHB_TWO_PI / 3.0F);
    HA_Port_Volt_C = -HA_Port_Volt_A - HA_Port_Volt_B;
    HA_Port_Current_A = current_peak * cosf(theta - 0.20F);
    HA_Port_Current_B = current_peak * cosf(theta - CHB_TWO_PI / 3.0F - 0.20F);
    HA_Port_Current_C = -HA_Port_Current_A - HA_Port_Current_B;
}
static void step(unsigned long n, float grid_peak, float current_peak) {
    input(n, grid_peak, current_peak);
    ChbDabControl_FastStep();
    if(n % 100UL == 0UL) ChbDabControl_Tick10ms();
    ChbDabControl_Service(); snapshot();
}
static unsigned long enter_closed(unsigned short strategy) {
    unsigned long n;
    setup(CHB_AC_MODE_CLOSED_LOOP);
    g_chb_dab_params.startup_strategy = strategy;
    g_chb_dab_params.precharge_stable_10ms = 2U;
    g_chb_dab_params.bypass_delay_10ms = 2U;
    g_chb_dab_params.pll_lock_10ms = 2U;
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    for(n = 1; n < 20000UL; ++n) {
        step(n, 311.0F, 2.0F);
        if(g_chb_dab_status.state == CHB_STATE_RUNNING) break;
        assert(g_chb_dab_status.state != CHB_STATE_FAULT);
    }
    assert(n < 20000UL && g_chb_dab_status.pll_locked != 0U);
    assert(g_chb_dab_status.precharge_bypassed && fake_ac_pwm && fake_dab_pwm);
    return n + 1UL;
}
int main(void) {
    unsigned long n, first;
    defaults = g_chb_dab_params; command_defaults = g_chb_dab_command;
    assert(fabsf(defaults.sample_period_s - 0.0001F) < 1e-10F);

    setup(CHB_AC_MODE_OPEN_LOOP);
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    ChbDabControl_Service();
    for(n = 1; n <= 3000UL; ++n) step(n, 0.0F, 0.0F);
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    assert(g_chb_dab_status.dab_duty_count == 2500U);
    assert(io_calls == 0U && fake_ac_pwm && fake_dab_pwm);
    assert(FPGA_Interface[4].DAB_LEFTDuty == 0U); /* absent A2 unchanged */
    ChbDabControl_ApplyRemoteCommand(1U, 1U); ChbDabControl_Service(); snapshot();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED && !fake_ac_pwm && !fake_dab_pwm);
    finish("open_loop_and_stop_priority");

    setup(CHB_AC_MODE_OPEN_LOOP); g_chb_dab_command.dab_enable = 1U;
    ChbDabControl_Service();
    for(n = 1; n <= 3000UL; ++n) step(n, 0.0F, 0.0F);
    assert(!fake_ac_pwm && fake_dab_pwm && DAC_BUF[0] == 2.5F);
    g_chb_dab_command.run_enable = 1U; ChbDabControl_Service(); snapshot();
    assert(fake_ac_pwm); g_chb_dab_command.dab_enable = 0U;
    ChbDabControl_Service(); snapshot(); assert(fake_ac_pwm && !fake_dab_pwm);
    finish("dab_only_and_independent_gates");

    first = enter_closed(CHB_STARTUP_SOURCE_COMPATIBLE);
    for(n = first; n < first + 4000UL; ++n) step(n, 311.0F, 3.0F);
    assert(g_chb_dab_status.voltage_loop_active && g_chb_dab_status.pll_locked);
    assert(fabsf(g_chb_dab_status.pll_frequency_hz - 50.0F) < 0.2F);
    finish("closed_source_compatible_pll");

    first = enter_closed(CHB_STARTUP_ID_RAMP);
    assert(g_chb_dab_status.startup_phase == CHB_STARTUP_ID_RAMP_PHASE);
    step(first++, 311.0F, 3.0F);
    assert(g_chb_dab_status.voltage_loop_active == 0U);
    for(n = first; n < first + 4000UL; ++n) step(n, 311.0F, 3.0F);
    assert(g_chb_dab_status.startup_phase == CHB_STARTUP_NORMAL);
    assert(g_chb_dab_status.voltage_loop_active != 0U);
    finish("closed_id_ramp_handover");

    first = enter_closed(CHB_STARTUP_SOURCE_COMPATIBLE);
    for(n = 0; n < 5UL; ++n) {
        input(first + n, 311.0F, 0.0F); HA_Port_Current_A = 130.0F;
        ChbDabControl_FastStep(); snapshot();
        if(n < 4UL) assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_AC_OVERCURRENT);
    assert(!fake_ac_pwm && !fake_dab_pwm);
    ChbDabControl_ForceFault(CHB_FAULT_HARDWARE); snapshot();
    assert(g_chb_dab_status.fault_code == CHB_FAULT_AC_OVERCURRENT);
    g_chb_dab_command.fault_reset = 1U;
    ChbDabControl_Tick10ms(); ChbDabControl_Service(); snapshot();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED && reset_calls == 1U);
    finish("overcurrent_confirmation_first_fault_reset");

    first = enter_closed(CHB_STARTUP_SOURCE_COMPATIBLE);
    SVG_Udc_B_Ave = 800.0F;
    for(n = 0; n < 5UL; ++n) {
        input(first + n, 311.0F, 0.0F); ChbDabControl_FastStep(); snapshot();
        if(n < 4UL) assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_DC_OVERVOLTAGE);
    assert(!fake_ac_pwm && !fake_dab_pwm); finish("overvoltage_confirmation");

    setup(CHB_AC_MODE_OPEN_LOOP); g_chb_dab_command.run_enable = 1U;
    ChbDabControl_Service(); g_chb_dab_protection_enable = 0U;
    ChbDabControl_ForceFault(CHB_FAULT_HARDWARE); snapshot();
    assert(g_chb_dab_status.state == CHB_STATE_FAULT && !fake_ac_pwm);
    assert(!g_chb_dab_command.run_enable); finish("hardware_fault_independent_of_software_switch");

    setup(CHB_AC_MODE_CLOSED_LOOP); g_chb_dab_params.precharge_timeout_10ms = 3U;
    SVG_Udc_A_Ave = SVG_Udc_B_Ave = SVG_Udc_C_Ave = 0.0F;
    g_chb_dab_command.run_enable = 1U;
    for(n = 1; n <= 500UL; ++n) step(n, 0.0F, 0.0F);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_PRECHARGE_TIMEOUT);
    assert(!fake_ac_pwm && !fake_dab_pwm); finish("precharge_timeout");

#ifdef SST_HOST_EXPECT_DC_LOG_UPDATES
    /* The original isolated control source omitted this writer.  Compile
     * these repair assertions only for the migrated implementation; keep the
     * historical compatibility traces unchanged and compare them separately. */
    setup(CHB_AC_MODE_OPEN_LOOP);
    assert_dc_log_faststep_updates();
    g_chb_dab_command.run_enable = 1U;
    ChbDabControl_Service();
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    assert_dc_log_faststep_updates();
    printf("dc_log_faststep phases=3 cards=6 combinations=4 cycles=2 states=stopped/running reserved=preserved\n");
#endif
    return 0;
}
