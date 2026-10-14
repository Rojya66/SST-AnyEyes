/* Host-only verification of real bench input and CHB/DAB control code.
 * Hardware access is replaced at its boundary. No circuit dynamics are faked.
 * This translation unit is never linked into the CCS firmware project.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstBenchInputs.h"

/* Calibrated measurement boundaries written by the real bench generator. */
static float HA_Port_Volt_A, HA_Port_Volt_B, HA_Port_Volt_C;
static float HA_Port_Volt_AB, HA_Port_Volt_BC, HA_Port_Volt_CA;
static float HA_Port_Current_A, HA_Port_Current_B, HA_Port_Current_C;
static float HA_Grid_Current_A, HA_Grid_Current_B, HA_Grid_Current_C;
static float SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave, SVG_Udc_Ave;

#if SST_BENCH_INPUT_SIM
/* FPGA and port boundaries used by the unmodified real control functions. */
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

/* Original module and compatibility globals read by the real control code. */
static float DAC_BUF[8];
static float HA_ReferenceDEF_A, HA_ReferenceDEF_B, HA_ReferenceDEF_C;
static unsigned short HA_PM_A1, HA_PM_A2, HA_PM_B1, HA_PM_B2, HA_PM_C1, HA_PM_C2;
static unsigned short NHA_module_A, NHA_module_B, NHA_module_C;
static unsigned short Port_RunStatus, Port_State;
static struct { struct { unsigned short Breaker_InnerOn, Breaker_InnerOff,
    Contactor_Inner; } Bbits; } Port_DOUT;
static struct { struct { unsigned short PWMPulseState; } Bbits; } FaultDisposeFLAG;
static struct { struct { unsigned short Port_Start, Port_Stop; } Bbits; } ControlFLAG;
static struct { struct { unsigned short P_Status; } Bbits; } Port_MainStatus;
static struct { struct { unsigned short PWMState_Dab; } Bbits; } PortRunFLAG;
static union { unsigned short PORT_FAULT16; struct {
    unsigned short reserved:15, PORT_FAULT_bit15:1; } Bbits; } Port_RealFault;

/* Fake PWM state records the observable hardware enable/disable actions. */
static unsigned int fake_ac_pwm, fake_dab_pwm;
static void Port_IO_Out(void) { }
static void Port_StateUpdate_Write(void) { }
static void PWMPluse_Enable(unsigned short bridge) {
    if(bridge == 1U || bridge == 3U) fake_ac_pwm = 1U;
    if(bridge == 2U || bridge == 3U) fake_dab_pwm = 1U;
}
static void PWMPluse_Disable(unsigned short index) {
    (void)index;
    if(FaultDisposeFLAG.Bbits.PWMPulseState & 1U) fake_ac_pwm = 0U;
    if(FaultDisposeFLAG.Bbits.PWMPulseState & 2U) fake_dab_pwm = 0U;
}
static void Module_ENdab_PWMEN(unsigned short enable) { fake_dab_pwm = enable; }
static void FaultLatch_Clear(unsigned short a, unsigned short b) {
    (void)a; (void)b;
}
#endif

/* INSERT_BENCH_IMPLEMENTATION */
/* INSERT_CONTROL_IMPLEMENTATION */

/* Compare generated values with independently calculated expected values. */
static void near_value(float actual, float expected, float tolerance) {
    assert(isfinite(actual));
    if(fabsf(actual - expected) > tolerance)
        fprintf(stderr, "Float mismatch: actual=%.9g expected=%.9g tolerance=%.9g\n",
                actual, expected, tolerance);
    assert(fabsf(actual - expected) <= tolerance);
}

#if SST_BENCH_INPUT_SIM
/* Saved real defaults are restored before each independent control scenario. */
static CHB_DAB_PARAMETERS defaults;
static CHB_DAB_COMMAND command_defaults;

/* Reset fake hardware and start the real closed-loop control in STOPPED. */
static void setup(unsigned short strategy) {
    memset(FPGA_Interface, 0, sizeof(FPGA_Interface));
    memset(&Port_DOUT, 0, sizeof(Port_DOUT));
    memset(&PortRunFLAG, 0, sizeof(PortRunFLAG));
    memset(&FaultDisposeFLAG, 0, sizeof(FaultDisposeFLAG));
    fake_ac_pwm = fake_dab_pwm = 0U;
    Port_RealFault.PORT_FAULT16 = 0U;
    HA_PM_A1 = HA_PM_B1 = HA_PM_C1 = 1U;
    HA_PM_A2 = HA_PM_B2 = HA_PM_C2 = 0U;
    NHA_module_A = NHA_module_B = NHA_module_C = 1U;
    HA_ReferenceDEF_A = HA_ReferenceDEF_B = HA_ReferenceDEF_C = 1000.0F;
    g_chb_dab_params = defaults;
    g_chb_dab_command = command_defaults;
    g_chb_dab_command.ac_mode = CHB_AC_MODE_CLOSED_LOOP;
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 0U;
    g_chb_dab_command.fault_reset = 0U;
    g_chb_dab_params.startup_strategy = strategy;
    g_chb_dab_protection_enable = 1U;
    ChbDabControl_Init();
    SstBenchInputs_Init();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
}

/* Advance one real synthetic sample and one real fast control invocation. */
static void step(void) {
    SstBenchInputs_FastStep();
    ChbDabControl_FastStep();
    if(g_sst_bench_status.tad_count % 100U == 0U) {
        ChbDabControl_Tick10ms();
        ChbDabControl_Service();
    }
    assert(g_sst_bench_status.invalid_input == 0U);
    assert(isfinite(g_chb_dab_status.pll_angle_rad));
    assert(isfinite(g_chb_dab_status.id_reference_a));
}

/* Validate default input, phase ordering, line voltage and live changes. */
static void test_generator(void) {
    unsigned int n; /* Sample index in the generated waveform. */
    float theta; /* Expected angle before each generated sample. */
    float a, b, c; /* Independent three-phase expected voltage values. */
    setup(CHB_STARTUP_SOURCE_COMPATIBLE);
    near_value(g_sst_bench_inputs.dc_a_v, 300.0F, 0.0F);
    near_value(g_sst_bench_inputs.dc_b_v, 300.0F, 0.0F);
    near_value(g_sst_bench_inputs.dc_c_v, 300.0F, 0.0F);
    near_value(g_sst_bench_inputs.grid_peak_v, 325.0F, 0.0F);
    near_value(g_sst_bench_inputs.grid_frequency_hz, 50.0F, 0.0F);
    near_value(g_sst_bench_inputs.current_peak_a, 0.0F, 0.0F);
    assert(g_sst_bench_status.tad_count == 0U);
    assert(g_sst_bench_status.invalid_input == 0U);
    near_value(g_sst_bench_status.angle_rad, 0.0F, 0.0F);
    g_sst_bench_inputs.dc_a_v = 360.0F;
    g_sst_bench_inputs.dc_b_v = 400.0F;
    g_sst_bench_inputs.dc_c_v = 440.0F;
    g_sst_bench_inputs.current_peak_a = 5.0F;
    theta = 0.0F;
    for(n = 0U; n < 450U; ++n) {
        a = 325.0F * cosf(theta);
        b = 325.0F * cosf(theta - CHB_TWO_PI / 3.0F);
        c = 325.0F * cosf(theta + CHB_TWO_PI / 3.0F);
        SstBenchInputs_FastStep();
        near_value(HA_Port_Volt_A, a, 0.02F);
        near_value(HA_Port_Volt_B, b, 0.02F);
        near_value(HA_Port_Volt_C, c, 0.02F);
        near_value(HA_Port_Volt_AB, a - b, 0.03F);
        near_value(HA_Port_Volt_BC, b - c, 0.03F);
        near_value(HA_Port_Volt_CA, c - a, 0.03F);
        near_value(HA_Port_Current_A, a * 5.0F / 325.0F, 0.001F);
        near_value(HA_Port_Current_B, b * 5.0F / 325.0F, 0.001F);
        near_value(HA_Port_Current_C, c * 5.0F / 325.0F, 0.001F);
        near_value(HA_Grid_Current_A, HA_Port_Current_A, 0.0F);
        near_value(HA_Grid_Current_B, HA_Port_Current_B, 0.0F);
        near_value(HA_Grid_Current_C, HA_Port_Current_C, 0.0F);
        near_value(HA_Port_Volt_A + HA_Port_Volt_B + HA_Port_Volt_C, 0.0F, 0.02F);
        near_value(SVG_Udc_A_Ave, 360.0F, 0.0F);
        near_value(SVG_Udc_B_Ave, 400.0F, 0.0F);
        near_value(SVG_Udc_C_Ave, 440.0F, 0.0F);
        near_value(SVG_Udc_Ave, 400.0F, 0.0F);
        near_value(g_sst_bench_status.angle_rad, theta, 0.0001F);
        theta += CHB_TWO_PI * 50.0F * defaults.sample_period_s;
        if(theta >= CHB_TWO_PI) theta -= CHB_TWO_PI;
    }
    assert(g_sst_bench_status.tad_count == 450U);
    g_sst_bench_inputs.grid_peak_v = 200.0F;
    g_sst_bench_inputs.grid_frequency_hz = 0.0F;
    g_sst_bench_inputs.current_peak_a = 0.0F;
    SstBenchInputs_FastStep();
    near_value(HA_Port_Volt_A, 200.0F * cosf(theta), 0.02F);
    near_value(g_sst_bench_status.angle_rad, theta, 0.0001F);
    near_value(HA_Port_Current_A, 0.0F, 0.0F);
    /* External observation/PLL writes cannot move the independent generator. */
    g_sst_bench_status.angle_rad = -10.0F;
    g_chb_dab_status.pll_angle_rad = 2.0F;
    SstBenchInputs_FastStep();
    near_value(HA_Port_Volt_A, 200.0F * cosf(theta), 0.02F);
    near_value(g_sst_bench_status.angle_rad, theta, 0.0001F);
    /* Resetting injected inputs must not start, stop, or configure the core. */
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    g_chb_dab_command.dc_voltage_ref_v = 725.0F;
    g_chb_dab_command.iq_reference_a = 5.0F;
    g_chb_dab_params.voltage_kp = 0.11F;
    SstBenchInputs_Init();
    assert(g_chb_dab_command.run_enable == 1U && g_chb_dab_command.dab_enable == 1U);
    assert(g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP);
    near_value(g_chb_dab_command.dc_voltage_ref_v, 725.0F, 0.0F);
    near_value(g_chb_dab_command.iq_reference_a, 5.0F, 0.0F);
    near_value(g_chb_dab_params.voltage_kp, 0.11F, 0.0F);
    g_sst_bench_status.tad_count = UINT32_MAX;
    SstBenchInputs_FastStep();
    assert(g_sst_bench_status.tad_count == 0U);
    puts("PASS generator: defaults, phase/wrap, line voltages, DC, current, live inputs");
}

/* Verify rejection of invalid floats and range values without stale samples. */
static void test_invalid_inputs(void) {
    unsigned int n; /* Index of each independent malformed input case. */
    for(n = 0U; n < 27U; ++n) {
        setup(CHB_STARTUP_SOURCE_COMPATIBLE);
        SstBenchInputs_FastStep();
        switch(n) {
            case 0: g_sst_bench_inputs.dc_a_v = NAN; break;
            case 1: g_sst_bench_inputs.dc_b_v = INFINITY; break;
            case 2: g_sst_bench_inputs.dc_c_v = -INFINITY; break;
            case 3: g_sst_bench_inputs.grid_peak_v = NAN; break;
            case 4: g_sst_bench_inputs.grid_frequency_hz = INFINITY; break;
            case 5: g_sst_bench_inputs.current_peak_a = NAN; break;
            case 6: g_sst_bench_inputs.dc_a_v = -1.0F; break;
            case 7: g_sst_bench_inputs.dc_b_v = -1.0F; break;
            case 8: g_sst_bench_inputs.dc_c_v = -1.0F; break;
            case 9: g_sst_bench_inputs.grid_peak_v = -1.0F; break;
            case 10: g_sst_bench_inputs.current_peak_a = -1.0F; break;
            case 11: g_sst_bench_inputs.grid_frequency_hz = -1.0F; break;
            case 12: g_sst_bench_inputs.grid_frequency_hz = 100.01F; break;
            case 13: g_sst_bench_inputs.grid_peak_v = INFINITY; break;
            case 14: g_sst_bench_inputs.grid_frequency_hz = NAN; break;
            case 15: g_sst_bench_inputs.current_peak_a = INFINITY; break;
            case 16: g_sst_bench_inputs.dc_a_v = 10001.0F; break;
            case 17: g_sst_bench_inputs.dc_b_v = 10001.0F; break;
            case 18: g_sst_bench_inputs.dc_c_v = 10001.0F; break;
            case 19: g_sst_bench_inputs.grid_peak_v = 20001.0F; break;
            case 20: g_sst_bench_inputs.current_peak_a = 10001.0F; break;
            case 21: g_chb_dab_params.sample_period_s = NAN; break;
            case 22: g_chb_dab_params.sample_period_s = INFINITY; break;
            case 23: g_chb_dab_params.sample_period_s = 0.0F; break;
            case 24: g_chb_dab_params.sample_period_s = -0.001F; break;
            case 25: g_chb_dab_params.sample_period_s = 0.0000009F; break;
            case 26: g_chb_dab_params.sample_period_s = 0.0101F; break;
        }
        SstBenchInputs_FastStep();
        assert(g_sst_bench_status.invalid_input == 1U);
        assert(g_sst_bench_status.tad_count == 2U);
        near_value(HA_Port_Volt_A, 0.0F, 0.0F);
        near_value(HA_Port_Volt_B, 0.0F, 0.0F);
        near_value(HA_Port_Volt_C, 0.0F, 0.0F);
        near_value(HA_Port_Volt_AB, 0.0F, 0.0F);
        near_value(HA_Port_Volt_BC, 0.0F, 0.0F);
        near_value(HA_Port_Volt_CA, 0.0F, 0.0F);
        near_value(HA_Port_Current_A, 0.0F, 0.0F);
        near_value(HA_Port_Current_B, 0.0F, 0.0F);
        near_value(HA_Port_Current_C, 0.0F, 0.0F);
        near_value(HA_Grid_Current_A, 0.0F, 0.0F);
        near_value(HA_Grid_Current_B, 0.0F, 0.0F);
        near_value(HA_Grid_Current_C, 0.0F, 0.0F);
        near_value(SVG_Udc_A_Ave, 0.0F, 0.0F);
        near_value(SVG_Udc_B_Ave, 0.0F, 0.0F);
        near_value(SVG_Udc_C_Ave, 0.0F, 0.0F);
        near_value(SVG_Udc_Ave, 0.0F, 0.0F);
        assert(isfinite(g_sst_bench_status.angle_rad));
        SstBenchInputs_Init();
        g_chb_dab_params.sample_period_s = defaults.sample_period_s;
        SstBenchInputs_FastStep();
        assert(g_sst_bench_status.invalid_input == 0U);
        near_value(HA_Port_Volt_A, 325.0F, 0.001F);
    }
    setup(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_sst_bench_inputs.dc_a_v = g_sst_bench_inputs.dc_b_v =
        g_sst_bench_inputs.dc_c_v = 10000.0F;
    g_sst_bench_inputs.grid_peak_v = 20000.0F;
    g_sst_bench_inputs.current_peak_a = 10000.0F;
    g_sst_bench_inputs.grid_frequency_hz = 100.0F;
    g_chb_dab_params.sample_period_s = 0.01F;
    SstBenchInputs_FastStep();
    SstBenchInputs_FastStep();
    assert(g_sst_bench_status.invalid_input == 0U);
    near_value(g_sst_bench_status.angle_rad, 0.0F, 0.00001F);
    g_chb_dab_params.sample_period_s = 0.000001F;
    SstBenchInputs_FastStep();
    assert(g_sst_bench_status.invalid_input == 0U);
    puts("PASS invalid inputs: NaN/Inf/ranges zero all outputs, count advances, init recovers");
}

/* Start naturally through all closed-loop stages using real synthetic input. */
static void enter_running(unsigned short strategy) {
    unsigned int n; /* Number of sample/control steps attempted. */
    unsigned short previous; /* Previous stage, used to verify transition order. */
    unsigned long stage_tick; /* 10 ms tick at the last state transition. */
    setup(strategy);
    g_sst_bench_inputs.dc_a_v = g_sst_bench_inputs.dc_b_v =
        g_sst_bench_inputs.dc_c_v = 400.0F;
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    previous = CHB_STATE_STOPPED;
    stage_tick = 0UL;
    for(n = 0U; n < 30000U; ++n) {
        step();
        assert(g_chb_dab_status.state != CHB_STATE_FAULT);
        if(g_chb_dab_status.state != previous) {
            assert(g_chb_dab_status.state == previous + 1U);
            if(previous == CHB_STATE_PRECHARGE)
                assert(g_chb_dab_status.tick_10ms - stage_tick == 20UL);
            if(previous == CHB_STATE_BYPASS_DELAY)
                assert(g_chb_dab_status.tick_10ms - stage_tick == 20UL);
            if(previous == CHB_STATE_WAIT_PLL)
                assert(g_chb_dab_status.tick_10ms - stage_tick == 20UL);
            previous = g_chb_dab_status.state;
            stage_tick = g_chb_dab_status.tick_10ms;
        }
        if(g_chb_dab_status.state == CHB_STATE_RUNNING) break;
        assert(!fake_ac_pwm && !fake_dab_pwm);
    }
    assert(n < 30000U && previous == CHB_STATE_RUNNING);
    assert(g_chb_dab_status.pll_locked == 1U);
    near_value(g_chb_dab_status.pll_frequency_hz, 50.0F, 0.1F);
    assert(g_chb_dab_status.precharge_bypassed == 1U);
    assert(fake_ac_pwm && fake_dab_pwm);
    assert(g_chb_dab_status.dab_duty_count == 350U);
    near_value(g_chb_dab_status.startup_voltage_reference_v, 420.0F, 0.001F);
}

/* Verify compatible startup, natural PLL, DAB ramp, live DC and stop. */
static void test_compatible_start(void) {
    unsigned int n; /* Fast samples elapsed after running entry. */
    enter_running(CHB_STARTUP_SOURCE_COMPATIBLE);
    assert(g_chb_dab_status.startup_phase == CHB_STARTUP_NORMAL);
    assert(g_chb_dab_status.voltage_loop_active == 1U);
    for(n = 1U; n <= 2150U; ++n) {
        step();
        assert(g_chb_dab_status.dab_duty_count == 350U + n);
        assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    }
    step();
    assert(g_chb_dab_status.dab_duty_count == 2500U);
    near_value(g_chb_dab_status.startup_voltage_reference_v, 678.0F, 0.01F);
    g_sst_bench_inputs.dc_a_v = g_sst_bench_inputs.dc_b_v =
        g_sst_bench_inputs.dc_c_v = 680.0F;
    step();
    near_value(g_chb_dab_status.dc_voltage_average_v, 680.0F, 0.0F);
    near_value(g_chb_dab_status.startup_voltage_reference_v, 680.0F, 0.001F);
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 0U;
    for(n = 0U; n < 100U; ++n) step();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
    assert(!fake_ac_pwm && !fake_dab_pwm);
    puts("PASS strategy 0: precharge/bypass/PLL/run, 2150-sample DAB ramp, live DC, STOP");
}

/* Verify fixed Id ramp reaches its target and hands control to voltage PI. */
static void test_id_ramp(void) {
    unsigned int n; /* Fast steps since running entry. */
    unsigned int handover_step; /* Observed transition sample to HANDOVER. */
    enter_running(CHB_STARTUP_ID_RAMP);
    assert(g_chb_dab_status.startup_phase == CHB_STARTUP_ID_RAMP_PHASE);
    handover_step = 0U;
    for(n = 1U; n <= 220U; ++n) {
        step();
        if(g_chb_dab_status.startup_phase == CHB_STARTUP_VOLTAGE_HANDOVER) {
            handover_step = n;
            assert(n >= 200U && n <= 201U);
            assert(g_chb_dab_status.voltage_loop_active == 0U);
            near_value(g_chb_dab_status.id_reference_a, 10.0F, 0.0001F);
            near_value(g_chb_dab_status.dc_voltage_average_v, 400.0F, 0.0F);
            near_value(g_chb_dab_status.startup_voltage_reference_v, 420.0F, 0.001F);
            break;
        }
        assert(g_chb_dab_status.startup_phase == CHB_STARTUP_ID_RAMP_PHASE);
        assert(g_chb_dab_status.voltage_loop_active == 0U);
        near_value(g_chb_dab_status.id_reference_a, 0.05F * n, 0.001F);
    }
    assert(handover_step != 0U);
    step();
    assert(g_chb_dab_status.startup_phase == CHB_STARTUP_NORMAL);
    assert(g_chb_dab_status.voltage_loop_active == 1U);
    assert(g_chb_dab_status.startup_tad_count == handover_step);
    near_value(g_chb_dab_status.startup_voltage_reference_v, 422.0F, 0.001F);
    assert(g_chb_dab_status.id_reference_a > 10.0F);
    /* Maximum duration independently triggers takeover before target Id. */
    enter_running(CHB_STARTUP_ID_RAMP);
    g_chb_dab_params.startup_ramp_tad = 3U;
    step(); step(); step();
    assert(g_chb_dab_status.startup_phase == CHB_STARTUP_VOLTAGE_HANDOVER);
    near_value(g_chb_dab_status.id_reference_a, 0.15F, 0.0001F);
    step();
    assert(g_chb_dab_status.startup_phase == CHB_STARTUP_NORMAL);
    puts("PASS strategy 1: Id target OR duration takeover; one-sample HANDOVER then voltage PI");
}

/* Verify real five-sample software protection and first-fault retention. */
static void test_protection(void) {
    unsigned int n; /* Samples since live simulated inputs exceed a threshold. */
    enter_running(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_sst_bench_inputs.current_peak_a = 300.0F;
    for(n = 1U; n <= 5U; ++n) {
        step();
        if(n < 5U) assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    }
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_AC_OVERCURRENT);
    assert(!fake_ac_pwm && !fake_dab_pwm);
    ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_AC_OVERCURRENT);
    g_sst_bench_inputs.current_peak_a = 0.0F;
    g_chb_dab_command.fault_reset = 1U;
    for(n = 0U; n < 100U; ++n) step();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_NONE);
    enter_running(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_sst_bench_inputs.dc_b_v = 800.0F;
    for(n = 1U; n <= 5U; ++n) {
        step();
        if(n < 5U) assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_DC_OVERVOLTAGE);
    assert(!fake_ac_pwm && !fake_dab_pwm);
    puts("PASS protection: live OC/OV inputs trip after five samples, first fault and reset");
}

/* Verify low DC/precharge and absent grid/PLL timeout paths without forcing state. */
static void test_timeouts(void) {
    unsigned int n; /* Sample count until fault or scenario upper bound. */
    setup(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_chb_dab_params.precharge_timeout_10ms = 3U;
    g_chb_dab_command.run_enable = 1U;
    for(n = 0U; n < 500U; ++n) step();
    assert(g_chb_dab_status.fault_code == CHB_FAULT_PRECHARGE_TIMEOUT);
    assert(g_chb_dab_status.dc_voltage_min_v == 300.0F);
    assert(!fake_ac_pwm && !fake_dab_pwm);
    setup(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_sst_bench_inputs.dc_a_v = g_sst_bench_inputs.dc_b_v =
        g_sst_bench_inputs.dc_c_v = 400.0F;
    g_sst_bench_inputs.grid_peak_v = 0.0F;
    g_chb_dab_params.pll_timeout_10ms = 3U;
    g_chb_dab_command.run_enable = 1U;
    for(n = 0U; n < 5000U; ++n) {
        step();
        if(g_chb_dab_status.state == CHB_STATE_FAULT) break;
    }
    assert(n < 5000U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_PLL_TIMEOUT);
    assert(g_chb_dab_status.pll_locked == 0U);
    assert(!fake_ac_pwm && !fake_dab_pwm);
    puts("PASS timeouts: unchanged 300 V fails precharge; zero grid fails PLL after bypass");
}

/* Frequency and initial phase differ from the control PLL, proving true capture. */
static void test_pll_capture(void) {
    unsigned int n; /* Samples for natural frequency/phase convergence. */
    setup(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_sst_bench_inputs.grid_frequency_hz = 52.0F;
    g_sst_bench_inputs.dc_a_v = g_sst_bench_inputs.dc_b_v =
        g_sst_bench_inputs.dc_c_v = 400.0F;
    g_chb_dab_status.pll_angle_rad = 0.8F;
    g_chb_dab_command.run_enable = 1U;
    for(n = 0U; n < 30000U; ++n) {
        step();
        assert(g_chb_dab_status.state != CHB_STATE_FAULT);
        if(g_chb_dab_status.state == CHB_STATE_RUNNING && n > 10000U) break;
    }
    assert(n < 30000U);
    assert(g_chb_dab_status.pll_locked == 1U);
    near_value(g_chb_dab_status.pll_frequency_hz, 52.0F, 0.05F);
    assert(g_chb_dab_status.dab_pwm_enabled == 0U);
    g_sst_bench_inputs.grid_peak_v = 50.0F;
    step();
    assert(g_chb_dab_status.pll_locked == 0U);
    near_value(g_chb_dab_status.pll_frequency_hz, 50.0F, 0.0F);
    /* Runtime loss of lock is observation, not an implemented running-state trip. */
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    puts("PASS PLL: captures independent 52 Hz with initial phase error; low amplitude unlocks");
}

int main(void) {
    defaults = g_chb_dab_params;
    command_defaults = g_chb_dab_command;
    assert(defaults.precharge_stable_10ms == 20U);
    assert(defaults.bypass_delay_10ms == 20U);
    assert(defaults.pll_lock_10ms == 20U);
    assert(defaults.protection_confirm_samples == 5U);
    test_generator();
    test_invalid_inputs();
    test_compatible_start();
    test_id_ramp();
    test_protection();
    test_timeouts();
    test_pll_capture();
    return 0;
}
#else
/* Disabled module must preserve every calibrated input and add no side effects. */
int main(void) {
    HA_Port_Volt_A = 1.0F; HA_Port_Volt_B = 2.0F; HA_Port_Volt_C = 3.0F;
    HA_Port_Volt_AB = 4.0F; HA_Port_Volt_BC = 5.0F; HA_Port_Volt_CA = 6.0F;
    HA_Port_Current_A = 7.0F; HA_Port_Current_B = 8.0F; HA_Port_Current_C = 9.0F;
    HA_Grid_Current_A = 10.0F; HA_Grid_Current_B = 11.0F; HA_Grid_Current_C = 12.0F;
    SVG_Udc_A_Ave = 13.0F; SVG_Udc_B_Ave = 14.0F; SVG_Udc_C_Ave = 15.0F;
    SVG_Udc_Ave = 16.0F;
    SstBenchInputs_Init();
    SstBenchInputs_FastStep();
    near_value(HA_Port_Volt_A, 1.0F, 0.0F);
    near_value(HA_Port_Volt_B, 2.0F, 0.0F);
    near_value(HA_Port_Volt_C, 3.0F, 0.0F);
    near_value(HA_Port_Volt_AB, 4.0F, 0.0F);
    near_value(HA_Port_Volt_BC, 5.0F, 0.0F);
    near_value(HA_Port_Volt_CA, 6.0F, 0.0F);
    near_value(HA_Port_Current_A, 7.0F, 0.0F);
    near_value(HA_Port_Current_B, 8.0F, 0.0F);
    near_value(HA_Port_Current_C, 9.0F, 0.0F);
    near_value(HA_Grid_Current_A, 10.0F, 0.0F);
    near_value(HA_Grid_Current_B, 11.0F, 0.0F);
    near_value(HA_Grid_Current_C, 12.0F, 0.0F);
    near_value(SVG_Udc_A_Ave, 13.0F, 0.0F);
    near_value(SVG_Udc_B_Ave, 14.0F, 0.0F);
    near_value(SVG_Udc_C_Ave, 15.0F, 0.0F);
    near_value(SVG_Udc_Ave, 16.0F, 0.0F);
    puts("PASS macro off: Init/FastStep preserve all calibrated input sentinels");
    return 0;
}
#endif
