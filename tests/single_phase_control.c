/* Host tests for the optional SST_PHASE_A_ONLY_TEST control path.
 *
 * single_phase_control.py concatenates this file after the real
 * SstSinglePhase.c and ChbDabControl.c implementations.  It therefore tests
 * the firmware source itself, while the small I/O substitutes above it keep
 * the test independent of TI headers and memory-mapped hardware.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

static CHB_DAB_PARAMETERS test_defaults;
static CHB_DAB_COMMAND test_command_defaults;

static void assert_near(float actual, float expected, float tolerance)
{
    assert(isfinite(actual));
    assert(fabsf(actual - expected) <= tolerance);
}

static void reset_test_io(void)
{
    memset(FPGA_Interface, 0, sizeof(FPGA_Interface));
    memset(&Port_DOUT, 0, sizeof(Port_DOUT));
    memset(&FaultDisposeFLAG, 0, sizeof(FaultDisposeFLAG));
    memset(&ControlFLAG, 0, sizeof(ControlFLAG));
    memset(&Port_MainStatus, 0, sizeof(Port_MainStatus));
    memset(&PortRunFLAG, 0, sizeof(PortRunFLAG));
    Port_RealFault.PORT_FAULT16 = 0U;
    memset(DAC_BUF, 0, sizeof(DAC_BUF));
    fake_ac_pwm = 0U;
    fake_dab_pwm = 0U;
    disable_calls = 0U;
    enable_calls = 0U;
    io_calls = 0U;
    state_calls = 0U;
    reset_calls = 0U;
    module_pwm_enable_calls = 0U;
    module_50hz_enable_calls = 0U;
    all_shutdown_calls = 0U;
    variable_reset_calls = 0U;
    interrupt_disable_calls = 0U;
    interrupt_restore_calls = 0U;
    fake_interrupt_enabled = 1U;
    fault_before_disable = 0U;
    fault_before_disable_call = 0U;
    fault_before_variable_reset = 0U;
    fault_injected_at_disable_call = 0U;
    enable_outside_critical = 0U;
    hardware_enable_events = 0U;
    HA_DC_Log_A = HA_DC_Log_B = HA_DC_Log_C = 0U;
    HA_IG_Log_A = HA_IG_Log_B = HA_IG_Log_C = 0U;
    trace_hash = UINT64_C(14695981039346656037);
}

static void setup_single_phase(unsigned short ac_mode)
{
    reset_test_io();
    HA_PM_A1 = 1U;
    HA_PM_A2 = 0U;
    HA_PM_B1 = 0U;
    HA_PM_B2 = 0U;
    HA_PM_C1 = 0U;
    HA_PM_C2 = 0U;
    NHA_module_A = 1U;
    NHA_module_B = 0U;
    NHA_module_C = 0U;
    HA_ReferenceDEF_A = 1250.0F;
    HA_ReferenceDEF_B = 1000.0F;
    HA_ReferenceDEF_C = 1000.0F;
    SVG_Udc_A_Ave = 600.0F;
    SVG_Udc_B_Ave = 0.0F;
    SVG_Udc_C_Ave = 0.0F;
    HA_Port_Volt_A = 0.0F;
    HA_Port_Volt_B = 0.0F;
    HA_Port_Current_A = 0.0F;
    HA_Port_Current_B = 0.0F;
    HA_Port_Current_C = 0.0F;
    g_chb_dab_params = test_defaults;
    g_chb_dab_command = test_command_defaults;
    g_chb_dab_command.ac_mode = ac_mode;
    g_chb_dab_protection_enable = 1U;
    ChbDabControl_Init();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
}

static void feed_single_phase(unsigned long sample,
                              float frequency_hz,
                              float voltage_peak,
                              float current_peak,
                              float dc_voltage)
{
    float theta = CHB_TWO_PI * frequency_hz * 0.0001F * (float)sample;

    HA_Port_Volt_A = voltage_peak * cosf(theta);
    HA_Port_Current_A = current_peak * cosf(theta - 0.20F);
    /* B/C are deliberately zero in the normal single-phase fixture. */
    HA_Port_Volt_B = 0.0F;
    HA_Port_Current_B = 0.0F;
    HA_Port_Current_C = 0.0F;
    SVG_Udc_A_Ave = dc_voltage;
    SVG_Udc_B_Ave = 0.0F;
    SVG_Udc_C_Ave = 0.0F;
}

static void control_step(unsigned long sample,
                         float frequency_hz,
                         float voltage_peak,
                         float current_peak,
                         float dc_voltage)
{
    feed_single_phase(sample, frequency_hz, voltage_peak, current_peak, dc_voltage);
    ChbDabControl_FastStep();
    if ((sample % 100UL) == 0UL)
    {
        ChbDabControl_Tick10ms();
    }
    ChbDabControl_Service();
}

static void test_sogi_quadrature(void)
{
    static const float frequencies[] = {45.0F, 50.0F, 55.0F, 60.0F};
    unsigned int case_index;

    for (case_index = 0U; case_index < sizeof(frequencies) / sizeof(frequencies[0]); ++case_index)
    {
        const float frequency_hz = frequencies[case_index];
        float alpha_cos = 0.0F;
        float alpha_sin = 0.0F;
        float beta_cos = 0.0F;
        float beta_sin = 0.0F;
        float alpha_square = 0.0F;
        float beta_square = 0.0F;
        float current_error_square = 0.0F;
        unsigned long sample;
        unsigned long count = 0UL;

        SstSinglePhase_Init();
        for (sample = 0UL; sample < 30000UL; ++sample)
        {
            float theta = CHB_TWO_PI * frequency_hz * 0.0001F * (float)sample;
            float alpha;
            float beta;
            SstSinglePhase_FastStep(311.0F * cosf(theta),
                                    7.0F * cosf(theta - 0.20F),
                                    frequency_hz,
                                    0.0001F);
            if (sample >= 10000UL)
            {
                alpha = g_sst_single_phase_status.voltage_alpha_v;
                beta = g_sst_single_phase_status.voltage_beta_v;
                alpha_cos += alpha * cosf(theta);
                alpha_sin += alpha * sinf(theta);
                beta_cos += beta * cosf(theta);
                beta_sin += beta * sinf(theta);
                alpha_square += alpha * alpha;
                beta_square += beta * beta;
                current_error_square +=
                    powf(g_sst_single_phase_status.current_alpha_a - 7.0F * cosf(theta - 0.20F), 2.0F) +
                    powf(g_sst_single_phase_status.current_beta_a - 7.0F * sinf(theta - 0.20F), 2.0F);
                ++count;
            }
        }
        assert(g_sst_single_phase_status.valid != 0U);
        assert(g_sst_single_phase_status.ready != 0U);
        assert(g_sst_single_phase_status.settling_count == 1000U);
        alpha_cos /= (float)count;
        alpha_sin /= (float)count;
        beta_cos /= (float)count;
        beta_sin /= (float)count;
        alpha_square = sqrtf(2.0F * alpha_square / (float)count);
        beta_square = sqrtf(2.0F * beta_square / (float)count);
        /* SOGI alpha follows cos(theta), beta follows sin(theta). */
        assert(alpha_cos > 0.48F * 311.0F);
        assert(fabsf(alpha_sin) < 0.03F * 311.0F);
        assert(beta_sin > 0.48F * 311.0F);
        assert(fabsf(beta_cos) < 0.03F * 311.0F);
        assert_near(alpha_square, 311.0F, 2.0F);
        assert_near(beta_square, 311.0F, 2.0F);
        assert(sqrtf(current_error_square / (float)count) < 0.02F);
    }
    printf("PASS sogi_quadrature frequencies=45,50,55,60\n");
}

static void test_sogi_invalid_recovery(void)
{
    static const float invalid[][4] = {
        {NAN, 1.0F, 50.0F, 0.0001F},
        {INFINITY, 1.0F, 50.0F, 0.0001F},
        {-INFINITY, 1.0F, 50.0F, 0.0001F},
        {100.0F, NAN, 50.0F, 0.0001F},
        {100.0F, INFINITY, 50.0F, 0.0001F},
        {1000001.0F, 1.0F, 50.0F, 0.0001F},
        {100.0F, -1000001.0F, 50.0F, 0.0001F},
        {100.0F, 1.0F, 0.5F, 0.0001F},
        {100.0F, 1.0F, 201.0F, 0.0001F},
        {100.0F, 1.0F, NAN, 0.0001F},
        {100.0F, 1.0F, 50.0F, NAN},
        {100.0F, 1.0F, 50.0F, 0.0F},
        {100.0F, 1.0F, 50.0F, -0.0001F},
        {100.0F, 1.0F, 50.0F, 0.0002F},
    };
    unsigned int index;
    float fresh_alpha;

    SstSinglePhase_Init();
    SstSinglePhase_FastStep(100.0F, 1.0F, 50.0F, 0.0001F);
    fresh_alpha = g_sst_single_phase_status.voltage_alpha_v;
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
    {
        SstSinglePhase_FastStep(100.0F, 1.0F, 50.0F, 0.0001F);
        SstSinglePhase_FastStep(invalid[index][0], invalid[index][1], invalid[index][2], invalid[index][3]);
        assert(g_sst_single_phase_status.valid == 0U);
        assert(g_sst_single_phase_status.ready == 0U);
        assert(g_sst_single_phase_status.settling_count == 0U);
        assert(g_sst_single_phase_status.voltage_alpha_v == 0.0F);
        assert(g_sst_single_phase_status.voltage_beta_v == 0.0F);
        assert(g_sst_single_phase_status.current_alpha_a == 0.0F);
        assert(g_sst_single_phase_status.current_beta_a == 0.0F);
        SstSinglePhase_FastStep(100.0F, 1.0F, 50.0F, 0.0001F);
        assert(g_sst_single_phase_status.valid != 0U);
        assert(g_sst_single_phase_status.voltage_alpha_v == fresh_alpha);
    }
    SstSinglePhase_Init();
    for (index = 0U; index < 999U; ++index)
    {
        SstSinglePhase_FastStep(100.0F, 1.0F, 50.0F, 0.0001F);
    }
    assert(g_sst_single_phase_status.settling_count == 999U);
    assert(g_sst_single_phase_status.ready == 0U);
    SstSinglePhase_FastStep(100.0F, 1.0F, 50.0F, 0.0001F);
    assert(g_sst_single_phase_status.settling_count == 1000U);
    assert(g_sst_single_phase_status.ready != 0U);
    printf("PASS sogi_invalid_recovery cases=14 ready_boundary=999/1000\n");
}

static unsigned long start_single_phase_closed_loop(unsigned short startup_strategy)
{
    unsigned long sample;

    setup_single_phase(CHB_AC_MODE_CLOSED_LOOP);
    g_chb_dab_params.startup_strategy = startup_strategy;
    g_chb_dab_params.precharge_stable_10ms = 2U;
    g_chb_dab_params.bypass_delay_10ms = 2U;
    g_chb_dab_params.pll_lock_10ms = 2U;
    g_chb_dab_params.pll_timeout_10ms = 100U;
    g_chb_dab_params.startup_id_reference_a = 5.0F;
    g_chb_dab_params.startup_id_ramp_step_a = 0.5F;
    g_chb_dab_params.startup_ramp_tad = 100U;
    g_chb_dab_params.current_phase_compensation_rad = 0.0F;
    g_chb_dab_params.output_phase_compensation_rad = 0.0F;
    g_chb_dab_command.run_enable = 1U;
    g_chb_dab_command.dab_enable = 1U;
    for (sample = 1UL; sample < 30000UL; ++sample)
    {
        control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
        if (g_chb_dab_status.state == CHB_STATE_FAULT)
        {
            fprintf(stderr, "startup failed sample=%lu fault=%u mode=%u active_mode=%u\n",
                    sample, g_chb_dab_status.fault_code,
                    g_chb_dab_command.ac_mode, s_active_ac_mode);
        }
        assert(g_chb_dab_status.state != CHB_STATE_FAULT);
        if (sample < 1000UL)
        {
            assert(g_chb_dab_status.pll_locked == 0U);
            assert(g_chb_dab_status.state != CHB_STATE_RUNNING);
            assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
        }
        if (g_chb_dab_status.state == CHB_STATE_RUNNING)
        {
            assert(g_chb_dab_status.pll_locked != 0U);
            assert(fake_ac_pwm != 0U && fake_dab_pwm != 0U);
            return sample + 1UL;
        }
    }
    assert(!"single-phase closed loop did not reach RUNNING");
    return 0UL;
}

static void test_closed_loop_takeover_and_neutral_phases(void)
{
    unsigned long sample;
    unsigned short saw_ramp = 0U;
    unsigned short saw_voltage_loop = 0U;
    float id_before_rise = 0.0F;

    sample = start_single_phase_closed_loop(CHB_STARTUP_ID_RAMP);
    for (; sample < 32000UL; ++sample)
    {
        control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
        if (g_chb_dab_status.startup_phase == CHB_STARTUP_ID_RAMP_PHASE)
        {
            saw_ramp = 1U;
        }
        if (g_chb_dab_status.voltage_loop_active != 0U)
        {
            saw_voltage_loop = 1U;
            id_before_rise = g_chb_dab_status.id_reference_a;
            break;
        }
    }
    assert(saw_ramp != 0U && saw_voltage_loop != 0U);
    assert(FPGA_Interface[7].Reference == (unsigned short)HA_ReferenceDEF_B);
    assert(FPGA_Interface[8].Reference == (unsigned short)HA_ReferenceDEF_B);
    assert(FPGA_Interface[11].Reference == (unsigned short)HA_ReferenceDEF_C);
    assert(FPGA_Interface[12].Reference == (unsigned short)HA_ReferenceDEF_C);

    /* Increasing only the A DC bus must be visible to the voltage outer loop. */
    {
        unsigned long end_sample = sample + 100UL;
        for (sample += 1UL; sample < end_sample; ++sample)
        {
            control_step(sample, 50.0F, 311.0F, 7.0F, 700.0F);
        }
    }
    assert(g_chb_dab_status.voltage_loop_active != 0U);
    assert(g_chb_dab_status.id_reference_a < id_before_rise - 5.0F);
    printf("PASS closed_loop_takeover_and_neutral_phases\n");
}

static void hash_test_float(float value)
{
    uint32_t bits;
    assert(isfinite(value));
    memcpy(&bits, &value, sizeof(bits));
    hash_word(bits);
}

static void hash_single_phase_observables(void)
{
    unsigned int index;

#define H(member) hash_word((uint32_t)g_chb_dab_status.member)
#define F(member) hash_test_float(g_chb_dab_status.member)
    H(state); H(fault_code); H(ac_input_closed); H(precharge_bypassed);
    H(ac_pwm_enabled); H(dab_pwm_enabled); H(pll_locked); H(dab_duty_count);
    H(tick_10ms); H(startup_phase); H(startup_tad_count); H(voltage_loop_active);
    F(dc_voltage_average_v); F(dc_voltage_min_v); F(pll_angle_rad);
    F(pll_frequency_hz); F(id_feedback_a); F(iq_feedback_a); F(id_reference_a);
    F(startup_id_reference_a); F(startup_voltage_reference_v); F(modulation_actual);
#undef H
#undef F
    for (index = 0U; index < 16U; ++index)
    {
        hash_word(FPGA_Interface[index].Reference);
        hash_word(FPGA_Interface[index].DAB_LEFTDuty);
        hash_word(FPGA_Interface[index].DAB_RIGHTDuty);
        hash_word(FPGA_Interface[index].DAB_Start);
    }
    hash_word(fake_ac_pwm);
    hash_word(fake_dab_pwm);
    hash_test_float(DAC_BUF[0]);
}

static void assert_unused_phase_references(void)
{
    static const unsigned int slots[] = {7U, 8U, 11U, 12U};
    unsigned int index;

    for (index = 0U; index < sizeof(slots) / sizeof(slots[0]); ++index)
    {
        assert(FPGA_Interface[slots[index]].Reference == 1000U);
        assert(FPGA_Interface[slots[index]].DAB_LEFTDuty == 0U);
        assert(FPGA_Interface[slots[index]].DAB_RIGHTDuty == 0U);
        assert(FPGA_Interface[slots[index]].DAB_Start == 0U);
    }
}

static void test_b_c_isolation(void)
{
    unsigned long sample;
    uint64_t clean_hash;
    uint64_t poisoned_hash;

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    for (; sample < 12000UL; ++sample)
    {
        control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
        assert_unused_phase_references();
        hash_single_phase_observables();
    }
    clean_hash = trace_hash;

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    for (; sample < 12000UL; ++sample)
    {
        feed_single_phase(sample, 50.0F, 311.0F, 7.0F, 600.0F);
        HA_Port_Volt_B = NAN;
        HA_Port_Current_B = 1000000.0F;
        HA_Port_Current_C = -1000000.0F;
        SVG_Udc_B_Ave = 1000000.0F;
        SVG_Udc_C_Ave = 1000000.0F;
        ChbDabControl_FastStep();
        if ((sample % 100UL) == 0UL)
        {
            ChbDabControl_Tick10ms();
        }
        ChbDabControl_Service();
        assert_unused_phase_references();
        hash_single_phase_observables();
    }
    poisoned_hash = trace_hash;
    assert(clean_hash == poisoned_hash);
    printf("PASS b_c_isolation every_step_hash=%016llx\n", (unsigned long long)clean_hash);
}

static void test_physical_protection_and_nan(void)
{
    unsigned long sample;
    unsigned int invalid_case;

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    {
        unsigned long end_sample = sample + 5UL;
        for (; sample < end_sample; ++sample)
        {
            feed_single_phase(sample, 50.0F, 311.0F, 0.0F, 600.0F);
            HA_Port_Current_A = 130.0F;
            ChbDabControl_FastStep();
            if (sample + 1UL < end_sample)
            {
                assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
            }
        }
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_AC_OVERCURRENT);
    assert(fabsf(g_sst_single_phase_status.current_alpha_a) < 120.0F);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    {
        unsigned long end_sample = sample + 5UL;
        for (; sample < end_sample; ++sample)
        {
            feed_single_phase(sample, 50.0F, 311.0F, 0.0F, 800.0F);
            ChbDabControl_FastStep();
            if (sample + 1UL < end_sample)
            {
                assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
            }
        }
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_DC_OVERVOLTAGE);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);

    for (invalid_case = 0U; invalid_case < 6U; ++invalid_case)
    {
        sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
        feed_single_phase(sample, 50.0F, 311.0F, 0.0F, 600.0F);
        switch (invalid_case)
        {
            case 0U: HA_Port_Volt_A = NAN; break;
            case 1U: HA_Port_Current_A = NAN; break;
            case 2U: SVG_Udc_A_Ave = NAN; break;
            case 3U: HA_Port_Volt_A = INFINITY; break;
            case 4U: HA_Port_Current_A = -INFINITY; break;
            default: SVG_Udc_A_Ave = INFINITY; break;
        }
        ChbDabControl_FastStep();
        assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
        assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
        assert(FPGA_Interface[3].Reference == 1250U);
        assert_unused_phase_references();
    }
    printf("PASS physical_protection_and_nan invalid_running_cases=6\n");
}

static void test_current_dq_sign_and_stop_tracking(void)
{
    unsigned long sample;
    unsigned long end_sample;
    float angle;
    float alpha;
    float beta;

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    end_sample = sample + 5000UL;
    for (; sample < end_sample; ++sample)
    {
        control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
    }
    assert_near(g_chb_dab_status.pll_frequency_hz, 50.0F, 0.2F);
    assert_near(g_chb_dab_status.id_feedback_a, 7.0F * cosf(0.20F), 0.2F);
    assert_near(g_chb_dab_status.iq_feedback_a, -7.0F * sinf(0.20F), 0.3F);
    g_chb_dab_params.current_feedback_sign = -1.0F;
    control_step(sample++, 50.0F, 311.0F, 7.0F, 600.0F);
    angle = g_chb_dab_status.pll_angle_rad;
    alpha = -g_sst_single_phase_status.current_alpha_a;
    beta = -g_sst_single_phase_status.current_beta_a;
    assert_near(g_chb_dab_status.id_feedback_a, alpha * cosf(angle) + beta * sinf(angle), 0.0001F);
    assert_near(g_chb_dab_status.iq_feedback_a, -alpha * sinf(angle) + beta * cosf(angle), 0.0001F);
    assert(g_chb_dab_status.id_feedback_a < 0.0F);

    g_chb_dab_command.run_enable = 0U;
    end_sample = sample + 1000UL;
    for (; sample < end_sample; ++sample)
    {
        control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
    }
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    assert(g_sst_single_phase_status.valid != 0U);
    assert(g_sst_single_phase_status.ready != 0U);
    assert(g_sst_single_phase_status.settling_count == 1000U);
    assert_near(g_chb_dab_status.pll_frequency_hz, 50.0F, 0.2F);
    printf("PASS current_dq_sign_and_stop_tracking\n");
}

/* The two 501 cards in a phase share the current-orientation bit but their
 * DC voltage sorting direction is complementary.  This test exercises all
 * four input combinations instead of checking only the normal zero case. */
static void test_sorting_direction_writer(void)
{
    unsigned short dc_order;
    unsigned short ig_orient;
    unsigned short a1;
    unsigned short a2;

    setup_single_phase(CHB_AC_MODE_OPEN_LOOP);
    for (ig_orient = 0U; ig_orient <= 1U; ++ig_orient)
    {
        for (dc_order = 0U; dc_order <= 1U; ++dc_order)
        {
            HA_IG_Log_A = ig_orient;
            HA_DC_Log_A = dc_order;
            ChbDabControl_FastStep();
            a1 = FPGA_Interface[Com_A1_PluginUnit_5 - 2].DC_Log.DC_Log16;
            a2 = FPGA_Interface[Com_A2_PluginUnit_6 - 2].DC_Log.DC_Log16;
            assert(((a1 >> 8) & 1U) == ig_orient);
            assert(((a2 >> 8) & 1U) == ig_orient);
            assert((a1 & 1U) == dc_order);
            assert((a2 & 1U) == (unsigned short)(dc_order ^ 1U));
        }
    }
    printf("PASS sorting_direction_writer combinations=4\n");
}

static void test_pll_rejects_antiphase(void)
{
    unsigned int sample;

    setup_single_phase(CHB_AC_MODE_CLOSED_LOOP);
    g_chb_dab_params.pll_lock_error_pu = 0.02F;
    g_chb_dab_status.pll_locked = 0U;
    /* q is approximately zero for both vectors.  The negative d vector must
     * still be rejected, otherwise a 180-degree phase jump can unlock PWM. */
    for (sample = 0U; sample < 200U; ++sample)
    {
        g_chb_dab_status.pll_angle_rad = 3.14159265358979323846F;
        Chb_UpdatePll(311.0F, 0.0F);
    }
    assert(g_chb_dab_status.pll_locked == 0U);

    for (sample = 0U; sample < 200U; ++sample)
    {
        g_chb_dab_status.pll_angle_rad = 0.0F;
        Chb_UpdatePll(311.0F, 0.0F);
    }
    assert(g_chb_dab_status.pll_locked != 0U);
    printf("PASS pll_antiphase_rejection\n");
}

static void test_mode_change_fault(void)
{
    unsigned long sample;
    unsigned short state;

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_chb_dab_command.ac_mode = CHB_AC_MODE_OPEN_LOOP;
    control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);

    for (state = CHB_STATE_PRECHARGE; state <= CHB_STATE_WAIT_PLL; ++state)
    {
        setup_single_phase(CHB_AC_MODE_CLOSED_LOOP);
        g_chb_dab_command.run_enable = 1U;
        ChbDabControl_Tick10ms();
        ChbDabControl_Service();
        assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);
        Chb_EnterState(state);
        g_chb_dab_command.ac_mode = CHB_AC_MODE_OPEN_LOOP;
        feed_single_phase(1UL, 50.0F, 311.0F, 7.0F, 600.0F);
        ChbDabControl_FastStep();
        assert(g_chb_dab_status.state == CHB_STATE_FAULT);
        assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
        assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    }

    setup_single_phase(CHB_AC_MODE_OPEN_LOOP);
    g_chb_dab_command.run_enable = 1U;
    g_chb_dab_command.dab_enable = 0U;
    ChbDabControl_Service();
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    g_chb_dab_command.ac_mode = CHB_AC_MODE_CLOSED_LOOP;
    ChbDabControl_Service();
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    printf("PASS mode_change_fault startup_and_running_both_directions\n");
}

static void test_running_voltage_loss_and_pll_loss(void)
{
    unsigned long sample;
    unsigned long end_sample;
    unsigned long zeros;
    unsigned int loss_sample;

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    end_sample = sample + 400UL;
    zeros = 0UL;
    for (; sample < end_sample; ++sample)
    {
        control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
        if (fabsf(HA_Port_Volt_A) < 0.02F)
        {
            ++zeros;
        }
        assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
        assert(g_chb_dab_status.pll_locked != 0U);
    }
    assert(zeros >= 3UL);

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    feed_single_phase(sample, 50.0F, 311.0F, 7.0F, 600.0F);
    /* Force an already-established orthogonal input to zero.  A single raw
     * zero sample is a normal sine zero-crossing and must not mean grid loss. */
    single_phase_history.voltage_alpha_v = 0.0F;
    single_phase_history.voltage_beta_v = 0.0F;
    single_phase_history.voltage_previous_v = 0.0F;
    HA_Port_Volt_A = 0.0F;
    for (loss_sample = 1U; loss_sample < 200U; ++loss_sample)
    {
        ChbDabControl_FastStep();
        assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
        assert(g_chb_dab_status.pll_locked == 0U);
    }
    ChbDabControl_FastStep();
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_PLL_TIMEOUT);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    g_chb_dab_params.pll_kp = 0.0F;
    g_chb_dab_params.pll_ki = 0.0F;
    g_chb_dab_params.pll_lock_error_pu = 0.00000001F;
    s_pll_integral = 0.0F;
    g_chb_dab_status.pll_angle_rad = Chb_WrapAngle(g_chb_dab_status.pll_angle_rad + 0.5F);
    end_sample = sample + 199UL;
    for (; sample < end_sample; ++sample)
    {
        control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
        assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
        assert(g_chb_dab_status.fault_code == CHB_FAULT_NONE);
    }
    control_step(sample, 50.0F, 311.0F, 7.0F, 600.0F);
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_PLL_TIMEOUT);
    printf("PASS running_voltage_loss_and_pll_loss boundary=200TAD\n");
}

static void test_nonfinite_control_parameters(void)
{
    static const float invalid_values[] = {NAN, INFINITY, -INFINITY};
    volatile float *const parameters[] = {
        &g_chb_dab_params.sample_period_s,
        &g_chb_dab_params.modulation_max,
        &g_chb_dab_params.precharge_voltage_v,
        &g_chb_dab_params.pll_nominal_frequency_hz,
        &g_chb_dab_params.pll_kp,
        &g_chb_dab_params.pll_ki,
        &g_chb_dab_params.pll_correction_limit_rad_s,
        &g_chb_dab_params.pll_lock_error_pu,
        &g_chb_dab_params.pll_min_grid_peak_v,
        &g_chb_dab_params.voltage_kp,
        &g_chb_dab_params.voltage_ki,
        &g_chb_dab_params.voltage_current_limit_a,
        &g_chb_dab_params.startup_udc_offset_v,
        &g_chb_dab_params.startup_udc_step_v,
        &g_chb_dab_params.startup_id_reference_a,
        &g_chb_dab_params.startup_id_ramp_step_a,
        &g_chb_dab_params.current_kp,
        &g_chb_dab_params.current_ki,
        &g_chb_dab_params.closed_loop_voltage_limit_factor,
        &g_chb_dab_params.current_feedback_sign,
        &g_chb_dab_params.current_phase_compensation_rad,
        &g_chb_dab_params.output_phase_compensation_rad,
        &g_chb_dab_params.ac_overcurrent_trip_a,
        &g_chb_dab_params.dc_overvoltage_trip_v,
        &g_chb_dab_command.open_loop_modulation,
        &g_chb_dab_command.open_loop_frequency_hz,
        &g_chb_dab_command.dc_voltage_ref_v,
        &g_chb_dab_command.iq_reference_a
    };
    unsigned int parameter;
    unsigned int value;
    unsigned long sample;

    for (parameter = 0U; parameter < sizeof(parameters) / sizeof(parameters[0]); ++parameter)
    {
        for (value = 0U; value < sizeof(invalid_values) / sizeof(invalid_values[0]); ++value)
        {
            sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
            *parameters[parameter] = invalid_values[value];
            feed_single_phase(sample, 50.0F, 311.0F, 7.0F, 600.0F);
            ChbDabControl_FastStep();
            if (g_chb_dab_status.fault_code != CHB_FAULT_BAD_CONFIGURATION)
            {
                fprintf(stderr, "nonfinite parameter=%u value=%u fault=%u\n",
                        parameter, value, g_chb_dab_status.fault_code);
            }
            assert(g_chb_dab_status.state == CHB_STATE_FAULT);
            assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
            assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
            assert(FPGA_Interface[3].Reference == 1250U);
        }
    }
    printf("PASS nonfinite_control_parameters floats=28 values=NaN,+Inf,-Inf\n");
}

/* Remove PI action so the observed reference is the voltage feed-forward.
 * This isolates the electrical normalization independently of tuning. */
static void prepare_numerical_control(void)
{
    setup_single_phase(CHB_AC_MODE_CLOSED_LOOP);
    g_chb_dab_params.voltage_kp = 0.0F;
    g_chb_dab_params.voltage_ki = 0.0F;
    g_chb_dab_params.current_kp = 0.0F;
    g_chb_dab_params.current_ki = 0.0F;
    g_chb_dab_params.current_phase_compensation_rad = 0.0F;
    g_chb_dab_params.output_phase_compensation_rad = 0.0F;
    g_chb_dab_command.dc_voltage_ref_v = 600.0F;
    g_chb_dab_status.pll_angle_rad = 0.0F;
    g_chb_dab_status.dc_voltage_average_v = 600.0F;
    s_startup_voltage_reference = 600.0F;
    s_startup_phase = CHB_STARTUP_NORMAL;
    g_sst_single_phase_status.current_alpha_a = 0.0F;
    g_sst_single_phase_status.current_beta_a = 0.0F;
}

static float actual_normalized_reference(void)
{
    return DAC_BUF[0] / 2.5F - 1.0F;
}

static void test_measured_bus_normalization_and_vector_limit(void)
{
    float normalized_reference;

    prepare_numerical_control();
    SVG_Udc_A_Ave = 400.0F;
    Chb_RunClosedLoop(311.0F, 0.0F);
    assert_near(actual_normalized_reference(), 311.0F / 400.0F, 0.00001F);
    normalized_reference = actual_normalized_reference();
    g_chb_dab_command.dc_voltage_ref_v = 800.0F;
    Chb_RunClosedLoop(311.0F, 0.0F);
    assert_near(actual_normalized_reference(), normalized_reference, 0.00001F);
    NHA_module_A = 2U;
    Chb_RunClosedLoop(311.0F, 0.0F);
    assert_near(actual_normalized_reference(), 311.0F / 800.0F, 0.00001F);

    prepare_numerical_control();
    SVG_Udc_A_Ave = 600.0F;
    g_chb_dab_params.modulation_max = 0.8F;
    g_chb_dab_params.closed_loop_voltage_limit_factor = 0.6F;
    Chb_RunClosedLoop(400.0F, 400.0F);
    assert_near(actual_normalized_reference(), 0.6F / sqrtf(2.0F), 0.00001F);
    g_chb_dab_params.output_phase_compensation_rad = -0.7853981633974483F;
    Chb_RunClosedLoop(400.0F, 400.0F);
    assert_near(actual_normalized_reference(), 0.6F, 0.00001F);
    /* Swapping which limit is smaller must preserve the same vector bound. */
    g_chb_dab_params.modulation_max = 0.5F;
    g_chb_dab_params.closed_loop_voltage_limit_factor = 0.9F;
    Chb_RunClosedLoop(400.0F, 400.0F);
    assert_near(actual_normalized_reference(), 0.5F, 0.00001F);
    printf("PASS measured_bus_normalization_and_dq_vector_limit\n");
}

static void test_pi_saturation_recovery(void)
{
    unsigned int sample;
    float output;
    float before_unwind;

    prepare_numerical_control();
    g_chb_dab_params.voltage_kp = 1.0F;
    g_chb_dab_params.voltage_ki = 100.0F;
    g_chb_dab_params.voltage_current_limit_a = 95.0F;
    for (sample = 0U; sample < 20000U; ++sample)
    {
        output = Chb_VoltagePiLimited(1000.0F);
        assert_near(output, 95.0F, 0.00001F);
        assert(fabsf(s_voltage_pi.integral) <= 95.0F);
    }
    output = Chb_VoltagePiLimited(-10.0F);
    assert(output < 0.0F);
    for (sample = 0U; sample < 20000U; ++sample)
    {
        output = Chb_VoltagePiLimited(-1000.0F);
        assert_near(output, -95.0F, 0.00001F);
        assert(fabsf(s_voltage_pi.integral) <= 95.0F);
    }
    output = Chb_VoltagePiLimited(10.0F);
    assert(output > 0.0F);

    prepare_numerical_control();
    g_chb_dab_params.current_kp = -10.0F;
    g_chb_dab_params.current_ki = -100.0F;
    g_chb_dab_command.iq_reference_a = 95.0F;
    SVG_Udc_A_Ave = 400.0F;
    g_chb_dab_params.modulation_max = 0.5F;
    for (sample = 0U; sample < 10000U; ++sample)
    {
        Chb_RunClosedLoop(311.0F, 0.0F);
    }
    assert(fabsf(s_iq_pi.integral) < 400.0F);
    before_unwind = s_iq_pi.integral;
    g_chb_dab_command.iq_reference_a = -1.0F;
    Chb_RunClosedLoop(0.0F, 0.0F);
    assert(s_iq_pi.integral >= before_unwind);
    assert(fabsf(actual_normalized_reference()) <= 0.5F);
    printf("PASS pi_saturation_recovery outer_and_inner\n");
}

static void test_atomic_pwm_enable_fault_race(void)
{
    unsigned int enables_before_fault;
    unsigned int target_disable_call;
    unsigned long sample;

    setup_single_phase(CHB_AC_MODE_OPEN_LOOP);
    g_chb_dab_command.run_enable = 1U;
    g_chb_dab_command.dab_enable = 1U;
    fault_before_variable_reset = 1U;
    ChbDabControl_Service();
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_HARDWARE);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    assert(g_chb_dab_status.ac_pwm_enabled == 0U && g_chb_dab_status.dab_pwm_enabled == 0U);
    assert(g_chb_dab_command.run_enable == 0U && g_chb_dab_command.dab_enable == 0U);
    assert(Port_RunStatus == PortStatusNum_Stop);
    assert(fault_before_variable_reset == 0U);
    assert(variable_reset_calls != 0U);
    assert(ControlFLAG.Bbits.Port_Start == 0U && ControlFLAG.Bbits.Port_Stop == 1U);
    assert(hardware_enable_events == 0U);
    assert(interrupt_disable_calls == interrupt_restore_calls);
    assert(fake_interrupt_enabled == 1U);

    sample = start_single_phase_closed_loop(CHB_STARTUP_SOURCE_COMPATIBLE);
    assert(interrupt_disable_calls != 0U);
    assert(interrupt_disable_calls == interrupt_restore_calls);
    assert(enable_outside_critical == 0U);
    assert(fake_interrupt_enabled == 1U);
    g_chb_dab_command.dab_enable = 0U;
    ChbDabControl_Tick10ms();
    ChbDabControl_Service();
    assert(fake_dab_pwm == 0U);
    enables_before_fault = hardware_enable_events;
    g_chb_dab_command.dab_enable = 1U;
    target_disable_call = interrupt_disable_calls + 1U;
    fault_before_disable_call = target_disable_call;
    fake_interrupt_enabled = 0U;
    /* Call the production enable helper directly to put the injected FAULT
     * at its final disable operation, after all slow-service transactions. */
    assert(Chb_SinglePhaseEnablePwm(2U) == 0U);
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_HARDWARE);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    assert(g_chb_dab_command.run_enable == 0U && g_chb_dab_command.dab_enable == 0U);
    assert(Port_RunStatus == PortStatusNum_Stop);
    assert(fault_before_disable_call == 0U);
    assert(fault_injected_at_disable_call == target_disable_call);
    assert(hardware_enable_events == enables_before_fault);
    assert(interrupt_disable_calls == interrupt_restore_calls);
    assert(fake_interrupt_enabled == 0U);
    (void)sample;
    printf("PASS atomic_pwm_enable_fault_race startup_and_dab_restart\n");
}

int main(void)
{
    test_defaults = g_chb_dab_params;
    test_command_defaults = g_chb_dab_command;
    assert(test_defaults.sample_period_s == 0.0001F);
    test_sogi_quadrature();
    test_sogi_invalid_recovery();
    test_closed_loop_takeover_and_neutral_phases();
    test_b_c_isolation();
    test_physical_protection_and_nan();
    test_current_dq_sign_and_stop_tracking();
    test_sorting_direction_writer();
    test_pll_rejects_antiphase();
    test_mode_change_fault();
    test_running_voltage_loss_and_pll_loss();
    test_nonfinite_control_parameters();
    test_measured_bus_normalization_and_vector_limit();
    test_pi_saturation_recovery();
    test_atomic_pwm_enable_fault_race();
    printf("PASS all_single_phase_control_tests\n");
    return 0;
}
