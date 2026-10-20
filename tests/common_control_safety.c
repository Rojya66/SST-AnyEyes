/* Host-only tests of common production control behavior, never target-linked. */
#if SST_LOW_VOLTAGE_100V_TEST != SST_HOST_EXPECT_LOW_PROFILE
#error "Derived low-voltage profile differs from the requested test fixture"
#endif
static CHB_DAB_PARAMETERS saved_parameters;
static CHB_DAB_COMMAND saved_command;
static unsigned long sample_number;
static unsigned short capture_first_shutdown;
static unsigned short shutdown_observed;
static float id_integral_at_shutdown;
static SST_PI_CONTROLLER id_pi_at_shutdown;

/* Called only by the host's fake Module_PWMEN(0), before the production
 * shutdown routine clears its PI objects. No production hook is required. */
static void common_capture_shutdown(void)
{
    if ((capture_first_shutdown != 0U) && (shutdown_observed == 0U))
    {
        shutdown_observed = 1U;
        id_integral_at_shutdown = s_id_pi.integral;
        memcpy(&id_pi_at_shutdown, &s_id_pi, sizeof(id_pi_at_shutdown));
    }
}

static float grid_peak(void)
{
    return SST_LOW_VOLTAGE_100V_TEST ? 141.421356F : 311.0F;
}

static float bus_voltage(void)
{
    return SST_LOW_VOLTAGE_100V_TEST ? 23.570226F : 600.0F;
}

static void fixture(unsigned short mode)
{
    memset(FPGA_Interface, 0, sizeof(FPGA_Interface));
    memset(&Port_DOUT, 0, sizeof(Port_DOUT));
    memset(&FaultDisposeFLAG, 0, sizeof(FaultDisposeFLAG));
    memset(&ControlFLAG, 0, sizeof(ControlFLAG));
    memset(&Port_MainStatus, 0, sizeof(Port_MainStatus));
    memset(&PortRunFLAG, 0, sizeof(PortRunFLAG));
    Port_RealFault.PORT_FAULT16 = 0U;
    fake_ac_pwm = fake_dab_pwm = 0U;
    disable_calls = enable_calls = io_calls = state_calls = reset_calls = 0U;
    interrupt_disable_calls = interrupt_restore_calls = 0U;
    fake_interrupt_enabled = 1U;
    fault_before_disable = fault_before_disable_call = fault_before_variable_reset = 0U;
    hardware_enable_events = enable_outside_critical = 0U;
    capture_first_shutdown = shutdown_observed = 0U;
    id_integral_at_shutdown = 0.0F;
    HA_PM_A1 = HA_PM_A2 = 7U;
    HA_PM_B1 = HA_PM_C1 = SST_PHASE_A_ONLY_TEST ? 0U : 7U;
    HA_PM_B2 = HA_PM_C2 = SST_PHASE_A_ONLY_TEST ? 0U : 7U;
    NHA_module_A = 6U;
    NHA_module_B = NHA_module_C = SST_PHASE_A_ONLY_TEST ? 0U : 6U;
    HA_ReferenceDEF_A = HA_ReferenceDEF_B = HA_ReferenceDEF_C = 1000.0F;
    HA_Port_Volt_A = HA_Port_Volt_B = HA_Port_Volt_C = 0.0F;
    HA_Port_Current_A = HA_Port_Current_B = HA_Port_Current_C = 0.0F;
    SVG_Udc_A_Ave = bus_voltage();
    SVG_Udc_B_Ave = SVG_Udc_C_Ave = SST_PHASE_A_ONLY_TEST ? 0.0F : bus_voltage();
    g_chb_dab_params = saved_parameters;
    g_chb_dab_command = saved_command;
    g_chb_dab_command.ac_mode = mode;
    g_chb_dab_protection_enable = 1U;
    sample_number = 0UL;
    ChbDabControl_Init();
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
}

static void feed(void)
{
    float angle = CHB_TWO_PI * 50.0F * 0.0001F * (float)sample_number;
    HA_Port_Volt_A = grid_peak() * cosf(angle);
    HA_Port_Volt_B = SST_PHASE_A_ONLY_TEST ? 0.0F : grid_peak() * cosf(angle - CHB_TWO_PI / 3.0F);
    HA_Port_Volt_C = SST_PHASE_A_ONLY_TEST ? 0.0F : -HA_Port_Volt_A - HA_Port_Volt_B;
    HA_Port_Current_A = HA_Port_Current_B = HA_Port_Current_C = 0.0F;
}

static void tick(void)
{
    ++sample_number;
    feed();
    ChbDabControl_FastStep();
    if (sample_number % 100UL == 0UL)
    {
        ChbDabControl_Tick10ms();
    }
    ChbDabControl_Service();
}

static void start_closed(void)
{
    fixture(CHB_AC_MODE_CLOSED_LOOP);
    g_chb_dab_command.run_enable = 1U;
    while (sample_number < 20000UL && g_chb_dab_status.state != CHB_STATE_RUNNING)
    {
        tick();
        assert(g_chb_dab_status.state != CHB_STATE_FAULT);
    }
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    assert(g_chb_dab_status.pll_locked != 0U);
    assert(fake_ac_pwm != 0U);
    assert(enable_outside_critical == 0U);
}

static void test_mode_latch(void)
{
    unsigned short state;
    fixture(CHB_AC_MODE_OPEN_LOOP);
    g_chb_dab_command.run_enable = 1U;
    ChbDabControl_Service();
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    g_chb_dab_command.ac_mode = CHB_AC_MODE_CLOSED_LOOP;
    ChbDabControl_Service();
    assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    for (state = CHB_STATE_PRECHARGE; state <= CHB_STATE_WAIT_PLL; ++state)
    {
        fixture(CHB_AC_MODE_CLOSED_LOOP);
        g_chb_dab_command.run_enable = 1U;
        ChbDabControl_Service();
        Chb_EnterState(state);
        g_chb_dab_command.ac_mode = CHB_AC_MODE_OPEN_LOOP;
        tick();
        assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
        assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    }
    puts("PASS common mode latch in startup and running");
}

static void test_pll_and_bus(void)
{
    unsigned int i;
    volatile float *buses[] = {&SVG_Udc_A_Ave, &SVG_Udc_B_Ave, &SVG_Udc_C_Ave};
    fixture(CHB_AC_MODE_CLOSED_LOOP);
    for (i = 0U; i < 200U; ++i)
    {
        g_chb_dab_status.pll_angle_rad = CHB_PI;
        Chb_UpdatePll(grid_peak(), 0.0F);
    }
    assert(g_chb_dab_status.pll_locked == 0U);
    for (i = 0U; i < (SST_PHASE_A_ONLY_TEST ? 1U : 3U); ++i)
    {
        start_closed();
        *buses[i] = g_chb_dab_params.precharge_voltage_v - 1.0F;
        tick();
        assert(g_chb_dab_status.state == CHB_STATE_FAULT);
        assert(fake_ac_pwm == 0U);
    }
    start_closed();
    for (i = 1U; i <= 200U; ++i)
    {
        HA_Port_Volt_A = HA_Port_Volt_B = HA_Port_Volt_C = 0.0F;
#if SST_PHASE_A_ONLY_TEST
        single_phase_history.voltage_alpha_v = 0.0F;
        single_phase_history.voltage_beta_v = 0.0F;
        single_phase_history.voltage_previous_v = 0.0F;
#endif
        ChbDabControl_FastStep();
        if (i < 200U)
        {
            assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
        }
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_PLL_TIMEOUT);
    assert(fake_ac_pwm == 0U);
    puts("PASS common positive PLL, each active bus and 200-TAD loss");
}

static void test_fault_races_and_first_control_step(void)
{
    unsigned int before;
    fixture(CHB_AC_MODE_OPEN_LOOP);
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    fault_before_variable_reset = 1U;
    ChbDabControl_Service();
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(hardware_enable_events == 0U);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    fixture(CHB_AC_MODE_OPEN_LOOP);
    g_chb_dab_command.run_enable = 1U;
    ChbDabControl_Service();
    before = hardware_enable_events;
    g_chb_dab_command.dab_enable = 1U;
    fault_before_disable_call = interrupt_disable_calls + 1U;
    assert(Chb_EnablePwm(2U) == 0U);
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(hardware_enable_events == before);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    assert(interrupt_disable_calls == interrupt_restore_calls);
    fixture(CHB_AC_MODE_OPEN_LOOP);
    g_chb_dab_command.run_enable = 1U;
    ChbDabControl_Service();
    s_id_pi.integral = 7.0F;
    tick();
    assert(FPGA_Interface[3].Reference != 1000U ||
           FPGA_Interface[7].Reference != 1000U ||
           FPGA_Interface[11].Reference != 1000U);
    assert(s_id_pi.integral == 7.0F);
    puts("PASS common fault ISR races and immediate first control step");
}

static void test_invalid_values_and_protection_order(void)
{
    unsigned int i, j;
    SST_PI_CONTROLLER expected_id_pi;
    const float invalid[] = {NAN, INFINITY, -INFINITY};
    volatile float *parameters[] = {
        &g_chb_dab_params.sample_period_s, &g_chb_dab_params.current_kp,
        &g_chb_dab_params.pll_kp, &g_chb_dab_command.dc_voltage_ref_v,
        &g_chb_dab_params.voltage_current_limit_a, &g_chb_dab_params.startup_udc_step_v,
        &g_chb_dab_params.output_phase_compensation_rad};
    volatile float *sampling[] = {
        &HA_Port_Volt_A, &HA_Port_Current_A, &SVG_Udc_A_Ave,
#if !SST_PHASE_A_ONLY_TEST
        &HA_Port_Volt_B, &HA_Port_Volt_C,
        &HA_Port_Current_B, &HA_Port_Current_C, &SVG_Udc_B_Ave, &SVG_Udc_C_Ave,
#endif
    };
    for (i = 0U; i < sizeof(parameters) / sizeof(parameters[0]); ++i)
    {
        for (j = 0U; j < 3U; ++j)
        {
            start_closed();
            *parameters[i] = invalid[j];
            tick();
            assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
            assert(fake_ac_pwm == 0U);
        }
    }
    for (i = 0U; i < sizeof(sampling) / sizeof(sampling[0]); ++i)
    {
        for (j = 0U; j < 3U; ++j)
        {
            start_closed();
            *sampling[i] = invalid[j];
            ChbDabControl_FastStep();
            assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
            assert(fake_ac_pwm == 0U);
        }
    }
    assert(Chb_WrapAngle(INFINITY) == 0.0F);
    assert(Chb_WrapAngle(NAN) == 0.0F);
    start_closed();
    g_chb_dab_params.protection_confirm_samples = 1U;
    s_id_pi.integral = 0.123F;
    /* A PI step can legitimately hold its integral under saturation.  Snapshot
     * the entire object as well, using an observation-only reference sentinel
     * that any real current-loop execution would overwrite. */
    s_id_pi.reference = 17.5F;
    memcpy(&expected_id_pi, &s_id_pi, sizeof(expected_id_pi));
    capture_first_shutdown = 1U;
    HA_Port_Current_A = g_chb_dab_params.ac_overcurrent_trip_a + 1.0F;
    ChbDabControl_FastStep();
    assert(g_chb_dab_status.fault_code == CHB_FAULT_AC_OVERCURRENT);
    assert(fake_ac_pwm == 0U);
    assert(shutdown_observed != 0U);
    assert(id_integral_at_shutdown == 0.123F); /* No PI step before first shutdown. */
    assert(memcmp(&id_pi_at_shutdown, &expected_id_pi, sizeof(expected_id_pi)) == 0);
    assert(s_id_pi.integral == 0.0F); /* Normal shutdown then resets controllers. */
    puts("PASS common NaN/Inf and protection before PI");
}

static void test_low_voltage_startup(void)
{
#if SST_LOW_VOLTAGE_100V_TEST
    unsigned int i;
    float previous;
    assert(saved_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP);
    assert(saved_command.dc_voltage_ref_v == 60.0F);
    assert(saved_parameters.startup_strategy == CHB_STARTUP_ID_RAMP);
    fixture(CHB_AC_MODE_CLOSED_LOOP);
    g_chb_dab_command.run_enable = 1U;
    SVG_Udc_A_Ave = 19.0F;
    for (i = 0U; i < 2500U; ++i)
    {
        tick();
        /* Closed-loop commands are accepted on the next 10 ms service tick. */
        assert(g_chb_dab_status.state ==
               ((sample_number < 100UL) ? CHB_STATE_STOPPED : CHB_STATE_PRECHARGE));
        assert(fake_ac_pwm == 0U);
    }
    start_closed();
    assert(s_startup_phase == CHB_STARTUP_ID_RAMP_PHASE);
    assert(fabsf(s_startup_voltage_reference - bus_voltage()) < 0.001F);
    previous = 0.0F;
    for (i = 0U; i < 10000U && s_startup_phase == CHB_STARTUP_ID_RAMP_PHASE; ++i)
    {
        tick();
        assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
        assert(s_id_reference >= previous);
        assert(s_id_reference - previous <= saved_parameters.startup_id_ramp_step_a + 0.000001F);
        previous = s_id_reference;
    }
    assert(i >= 990U && i <= 1002U);
    assert(s_startup_phase == CHB_STARTUP_VOLTAGE_HANDOVER);
    tick();
    assert(s_startup_phase == CHB_STARTUP_NORMAL);
    assert(fabsf(s_id_reference - previous) < 0.001F);
    for (i = 0U; i < 12000U; ++i)
    {
        previous = s_startup_voltage_reference;
        tick();
        assert(s_startup_voltage_reference - previous <= saved_parameters.startup_udc_step_v + 0.00001F);
        assert(fabsf(s_id_reference) <= 1.00001F);
        assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    }
    assert(fabsf(s_startup_voltage_reference - 60.0F) < 0.001F);
    assert(g_chb_dab_status.load_ready == 0U);
    assert(g_chb_dab_status.current_limit_active_a == 1.0F);
    SVG_Udc_A_Ave = 60.0F;
    for (i = 0U; i < SST_LV100_READY_TAD - 1U; ++i)
    {
        tick();
        assert(g_chb_dab_status.load_ready == 0U);
    }
    tick();
    assert(g_chb_dab_status.load_ready == 1U);
    for (i = 0U; i < 50000U; ++i)
    {
        previous = g_chb_dab_status.current_limit_active_a;
        tick();
        assert(g_chb_dab_status.current_limit_active_a >= previous);
        assert(g_chb_dab_status.current_limit_active_a - previous <= 0.00101F);
        assert(g_chb_dab_status.current_limit_active_a <= 50.0F);
    }
    assert(g_chb_dab_status.current_limit_active_a == 50.0F);
    /* A load dip retains permission. Iq shares the same circular budget. */
    SVG_Udc_A_Ave = 33.0F;
    g_chb_dab_command.iq_reference_a = 30.0F;
    s_voltage_pi.integral = 50.0F;
    tick();
    assert(g_chb_dab_status.load_ready == 1U);
    assert(g_chb_dab_status.current_limit_active_a == 50.0F);
    assert(fabsf(s_id_reference) <= 40.0001F);
    assert(fabsf(s_iq_pi.reference - 30.0F) < 0.001F);
    g_chb_dab_command.iq_reference_a = 0.0F;
    g_chb_dab_params.voltage_current_limit_a = 5.0F;
    tick();
    assert(g_chb_dab_status.current_limit_active_a == 5.0F);
    assert(fabsf(s_id_reference) <= 5.0F);
    g_chb_dab_command.run_enable = 0U;
    for (i = 0U; i < 100U && g_chb_dab_status.state != CHB_STATE_STOPPED; ++i)
    {
        tick();
    }
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED && fake_ac_pwm == 0U);
    assert(g_chb_dab_status.load_ready == 0U);
    assert(g_chb_dab_status.current_limit_active_a == 1.0F);
    start_closed();
    g_chb_dab_command.iq_reference_a = 30.0F;
    tick();
    assert(fabsf(s_iq_pi.reference) <= 1.00001F);
    assert(s_id_reference * s_id_reference + s_iq_pi.reference * s_iq_pi.reference <= 1.0001F);
    start_closed();
    g_chb_dab_params.voltage_current_limit_a = 50.1F;
    tick();
    assert(g_chb_dab_status.fault_code == CHB_FAULT_BAD_CONFIGURATION);
    puts("PASS load-ready debounce, 1-to-50A slew, dq budget, live derating and restart reset");
    puts("PASS actual 100Vac defaults, insufficient precharge, 100ms Id ramp, voltage slew and stop");
#endif
}

int main(void)
{
    saved_parameters = g_chb_dab_params;
    saved_command = g_chb_dab_command;
    test_mode_latch();
    test_pll_and_bus();
    test_fault_races_and_first_control_step();
    test_invalid_values_and_protection_order();
    test_low_voltage_startup();
    return 0;
}
