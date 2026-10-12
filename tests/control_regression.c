/* Host-only hardware substitutes and behavioral regression scenarios.
 * control_regression.py inserts the ORIGINAL or MIGRATED control source at
 * the marker below. This file is never linked into the embedded project.
 * The host ABI is not used as evidence of TI binary layout or execution time.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"

typedef struct {
    unsigned short Reference, DAB_LEFTDuty, DAB_RIGHTDuty, DAB_Start;
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
static float HA_Port_Volt_A, HA_Port_Volt_B;
static float HA_Port_Current_A, HA_Port_Current_B, HA_Port_Current_C;
static float SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave;
static unsigned short Port_RunStatus, Port_State;
static struct { struct { unsigned short Breaker_InnerOn, Breaker_InnerOff,
    Contactor_Inner; } Bbits; } Port_DOUT;
static struct { struct { unsigned short PWMPulseState; } Bbits; } FaultDisposeFLAG;
static struct { struct { unsigned short Port_Start, Port_Stop; } Bbits; } ControlFLAG;
static struct { struct { unsigned short P_Status; } Bbits; } Port_MainStatus;
static struct { struct { unsigned short PWMState_Dab; } Bbits; } PortRunFLAG;
static union { unsigned short PORT_FAULT16; struct {
    unsigned short PORT_FAULT_bit15; } Bbits; } Port_RealFault;

static uint64_t trace_hash;
static unsigned long trace_steps;
static unsigned int fake_ac_pwm, fake_dab_pwm;
static unsigned int disable_calls, enable_calls, io_calls, state_calls, reset_calls;
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
    hash_word(0x400U); hash_word(index); hash_word(FaultDisposeFLAG.Bbits.PWMPulseState); }
static void Module_ENdab_PWMEN(unsigned short enable) { fake_dab_pwm = enable;
    hash_word(0x500U); hash_word(enable); }
static void FaultLatch_Clear(unsigned short a, unsigned short b) {
    ++reset_calls; hash_word(0x600U); hash_word(a); hash_word(b); }

/* INSERT_CONTROL_IMPLEMENTATION */

static CHB_DAB_PARAMETERS defaults;
static CHB_DAB_COMMAND command_defaults;
static void hash_float(float value) {
    uint32_t bits;
    assert(isfinite(value));
    memcpy(&bits, &value, sizeof(bits));
    hash_word(bits);
}
static void snapshot(void) {
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
        hash_word(FPGA_Interface[i].Reference);
        hash_word(FPGA_Interface[i].DAB_LEFTDuty);
        hash_word(FPGA_Interface[i].DAB_RIGHTDuty);
        hash_word(FPGA_Interface[i].DAB_Start);
    }
    hash_float(DAC_BUF[0]); hash_word(fake_ac_pwm); hash_word(fake_dab_pwm);
    hash_word(g_chb_dab_command.run_enable); hash_word(g_chb_dab_command.dab_enable);
    ++trace_steps;
}
static void finish(const char *scenario) {
    printf("%s steps=%lu hash=%016llx state=%u fault=%u enables=%u disables=%u io=%u resets=%u\n",
        scenario, trace_steps, (unsigned long long)trace_hash,
        g_chb_dab_status.state, g_chb_dab_status.fault_code,
        enable_calls, disable_calls, io_calls, reset_calls);
}
static void setup(unsigned short mode) {
    trace_hash = UINT64_C(14695981039346656037); trace_steps = 0;
    disable_calls = enable_calls = io_calls = state_calls = reset_calls = 0;
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
    HA_Port_Volt_A = HA_Port_Volt_B = 0.0F;
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
    return 0;
}
