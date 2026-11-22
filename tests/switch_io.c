/* Host scenarios for the production QF1 / KT1 controller and real register views. */
static void switches(unsigned short qf1_closed, unsigned short kt1_closed, unsigned short qf1_open)
{
    fake_din = (unsigned short)((kt1_closed << 5U) | (qf1_closed << 6U) | (qf1_open << 7U));
}

static void slow(unsigned int count)
{
    while (count-- != 0U)
    {
        ChbDabControl_Tick10ms();
        ChbDabControl_Service();
    }
}

static void prepare_switch_fixture(void)
{
    fixture(CHB_AC_MODE_CLOSED_LOOP);
    switches(0U, 0U, 1U);
    din_reads = 0U;
    g_chb_dab_params.precharge_stable_10ms = 2U;
    g_chb_dab_params.kt1_settle_10ms = 2U;
    g_chb_dab_params.pll_lock_10ms = 2U;
    g_chb_dab_status.dc_voltage_min_v = bus_voltage();
    g_chb_dab_status.pll_locked = 1U;
    slow(3U);
}

static void verify_register_views(void)
{
    XXG_SwitchINOUT value;
    value.SwitchINOUT16 = 0U;
    value.Dout.Qf1ClosePulse = 1U;
    assert(value.SwitchINOUT16 == 0x0001U);
    value.SwitchINOUT16 = 0U;
    value.Dout.Qf1OpenPulse = 1U;
    assert(value.SwitchINOUT16 == 0x0002U);
    value.SwitchINOUT16 = 0U;
    value.Dout.Kt1Hold = 1U;
    assert(value.SwitchINOUT16 == 0x0004U);
    value.SwitchINOUT16 = 0U;
    value.Dout.AcRunning = 1U;
    assert(value.SwitchINOUT16 == 0x0008U);
    value.SwitchINOUT16 = 0U;
    value.Dout.AcFault = 1U;
    assert(value.SwitchINOUT16 == 0x0010U);
    value.SwitchINOUT16 = 0x0020U;
    assert(value.Din.Kt1Closed == 1U && value.Din.Qf1Closed == 0U && value.Din.Qf1Open == 0U);
    value.SwitchINOUT16 = 0x0040U;
    assert(value.Din.Kt1Closed == 0U && value.Din.Qf1Closed == 1U && value.Din.Qf1Open == 0U);
    value.SwitchINOUT16 = 0x0080U;
    assert(value.Din.Kt1Closed == 0U && value.Din.Qf1Closed == 0U && value.Din.Qf1Open == 1U);
}

#if SST_ENABLE_SWITCH_OUTPUT && SST_ENABLE_SWITCH_FEEDBACK
static unsigned int switch_regression_failures;

static void check_switch_regression(unsigned short passed, const char *scenario)
{
    if (passed == 0U)
    {
        ++switch_regression_failures;
        fprintf(stderr, "FAIL switch regression: %s\n", scenario);
    }
}

static void verify_precharge_feedback_interruption(void)
{
    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(1U);
    switches(1U, 0U, 0U);
    slow(4U); /* QF1 confirmed, FO armed, first of two good voltage samples. */
    assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);

    switches(0U, 0U, 0U);
    g_chb_dab_status.dc_voltage_min_v = 0.0F;
    slow(1U); /* Voltage is invalid while QF1 feedback also interrupts startup. */
    switches(1U, 0U, 0U);
    g_chb_dab_status.dc_voltage_min_v = bus_voltage();
    slow(3U); /* Feedback confirmed again; only one fresh good voltage sample. */
    check_switch_regression(
        (g_chb_dab_status.state == CHB_STATE_PRECHARGE) && (Port_DOUT.Dout.Kt1Hold == 0U),
        "PRECHARGE must discard voltage confirmation accumulated before feedback loss");
    slow(1U);
    check_switch_regression(
        (g_chb_dab_status.state == CHB_STATE_KT1_SETTLE) && (Port_DOUT.Dout.Kt1Hold == 1U),
        "PRECHARGE may close KT1 after two fresh consecutive voltage confirmations");
}

static void verify_pll_feedback_interruption(void)
{
    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(1U);
    switches(1U, 0U, 0U);
    slow(5U);
    assert(g_chb_dab_status.state == CHB_STATE_KT1_SETTLE);
    switches(1U, 1U, 0U);
    slow(3U);
    assert(g_chb_dab_status.state == CHB_STATE_WAIT_PLL);
    slow(3U); /* WAIT_PLL reconfirms contacts, then records its first good lock sample. */
    assert(fake_ac_pwm == 0U);
    assert(s_pll_lock_10ms_count == 1U);

    switches(1U, 0U, 0U);
    g_chb_dab_status.pll_locked = 0U;
    slow(1U);
    switches(1U, 1U, 0U);
    g_chb_dab_status.pll_locked = 1U;
    slow(3U); /* Only one fresh lock confirmation after KT1 feedback recovers. */
    check_switch_regression(
        (g_chb_dab_status.state == CHB_STATE_WAIT_PLL) && (fake_ac_pwm == 0U),
        "WAIT_PLL must discard lock confirmation accumulated before feedback loss");
    slow(1U);
    check_switch_regression(
        (g_chb_dab_status.state == CHB_STATE_RUNNING) && (fake_ac_pwm == 1U),
        "WAIT_PLL may enable PWM after two fresh consecutive lock confirmations");
}

static void verify_tuned_switch_times(void)
{
    prepare_switch_fixture();
    g_chb_dab_params.switch_stable_10ms = 5U;
    g_chb_dab_params.switch_timeout_10ms = 8U;
    g_chb_dab_command.run_enable = 1U;
    slow(1U);
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);
    slow(1U);
    assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);
    /* Leave QF1 open after the close command; use the tuned action deadline. */
    slow(7U);
    assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);
    slow(1U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK);
    assert(Port_DOUT.Dout.Qf1OpenPulse == 1U);
}

static void verify_repeated_kt1_request(unsigned short close)
{
    unsigned int tick;

    prepare_switch_fixture();
    assert(ChbDabControl_MaintenanceSwitch(2U, 0U) == CHB_SWITCH_OK);
    switches(1U, 0U, 0U);
    slow(3U);
    if (close == 0U)
    {
        /* Begin release test with KT1 commanded closed and physically confirmed. */
        assert(ChbDabControl_MaintenanceSwitch(1U, 0U) == CHB_SWITCH_OK);
        switches(1U, 1U, 0U);
        slow(3U);
        assert(ChbDabControl_MaintenanceSwitch(0U, 1U) == CHB_SWITCH_OK);
    }
    else
    {
        assert(ChbDabControl_MaintenanceSwitch(1U, 0U) == CHB_SWITCH_OK);
    }

    /* Leave feedback at the OLD position and repeat the SAME request every 100ms.
     * This must not extend the original one-second feedback deadline. */
    for (tick = 0U; tick < 105U; ++tick)
    {
        if (((tick % 10U) == 0U) && (g_chb_dab_status.fault_code == CHB_FAULT_NONE))
        {
            if (close == 0U)
            {
                assert(ChbDabControl_MaintenanceSwitch(0U, 1U) == CHB_SWITCH_OK);
            }
            else
            {
                assert(ChbDabControl_MaintenanceSwitch(1U, 0U) == CHB_SWITCH_OK);
            }
        }
        slow(1U);
    }
    check_switch_regression(
        (g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK) && (Port_DOUT.Dout.Kt1Hold == 0U),
        (close == 0U) ? "repeated KT1 release must not postpone missing-feedback timeout"
                      : "repeated KT1 close must not postpone missing-feedback timeout");
}
#endif

int main(void)
{
    unsigned int word;
    saved_parameters = g_chb_dab_params;
    saved_command = g_chb_dab_command;
    verify_register_views();
#if SST_ENABLE_SWITCH_OUTPUT
    prepare_switch_fixture();
    assert(ChbDabControl_MaintenanceSwitch(2U, 0U) == CHB_SWITCH_OK);
    fake_fo_phases = 1U;
    fake_fo_enable[0] = 1U;
    assert(ChbDabControl_MaintenanceSwitch(0U, 2U) == CHB_SWITCH_OK);
    assert(fake_fo_phases == 0U && g_chb_dab_status.qf1_close_command == 0U);
    assert(ChbDabControl_MaintenanceSwitch(2U, 0U) == CHB_SWITCH_OK);
    fake_fo_phases = 1U;
    fake_fo_input[0][1] = 1U;
    assert(ChbDabControl_MaintenanceSwitch(0U, 2U) == CHB_SWITCH_OK);
    assert(g_chb_dab_status.state == CHB_STATE_FAULT && g_chb_dab_status.qf1_close_command == 0U);
    assert(fake_fo_phases == 1U && g_chb_dab_status.fault_code == CHB_FAULT_HARDWARE);
    fake_fo_input[0][1] = 0U;
    fake_fo_enable[0] = 0U;
#endif
    /* S301 FPGA already converts register 1 to the relay's active-low pin. */
    for (word = 0U; word <= 0xFFFFU; ++word)
    {
        Port_DOUT.SwitchINOUT16 = (unsigned short)word;
        FPGA_Interface[IO_PluginUnit_2 - 2].DOUT.SwitchINOUT16 = 0x5A5AU;
        Port_IO_Out();
#if SST_ENABLE_SWITCH_OUTPUT
        assert(FPGA_Interface[IO_PluginUnit_2 - 2].DOUT.SwitchINOUT16 == word);
#else
        assert(FPGA_Interface[IO_PluginUnit_2 - 2].DOUT.SwitchINOUT16 == 0x5A5AU);
#endif
        assert(Port_DOUT.SwitchINOUT16 == word);
    }

    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(1U);
    assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);
    assert(Port_DOUT.Dout.Qf1ClosePulse == 1U);
    assert(Port_DOUT.Dout.Qf1OpenPulse == 0U && Port_DOUT.Dout.Kt1Hold == 0U);
    assert(fake_ac_pwm == 0U);
#if SST_ENABLE_SWITCH_FEEDBACK
    slow(10U); /* Correct voltage alone must not skip the QF1 position check. */
    assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);
    assert(Port_DOUT.Dout.Kt1Hold == 0U && fake_ac_pwm == 0U);
    switches(1U, 0U, 0U);
    slow(5U);
#else
    slow(3U); /* First eligible tick arms FO, then qualify two samples. */
    assert(din_reads == 0U);
#endif
    assert(g_chb_dab_status.state == CHB_STATE_KT1_SETTLE);
    assert(Port_DOUT.Dout.Kt1Hold == 1U);
#if SST_ENABLE_SWITCH_FEEDBACK
    slow(10U); /* KT1 is commanded closed but its contact has not yet closed. */
    assert(g_chb_dab_status.state == CHB_STATE_KT1_SETTLE && fake_ac_pwm == 0U);
    switches(1U, 1U, 0U);
    slow(3U);
#else
    slow(2U);
#endif
    assert(g_chb_dab_status.state == CHB_STATE_WAIT_PLL);
    slow(5U);
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING && fake_ac_pwm == 1U);
    assert(Port_DOUT.Dout.AcRunning == 1U && Port_DOUT.Dout.AcFault == 0U);
    slow(60U);
    assert(Port_DOUT.Dout.Qf1ClosePulse == 0U && Port_DOUT.Dout.Qf1OpenPulse == 0U);
    assert(Port_DOUT.Dout.Kt1Hold == 1U); /* Pulse expiry cannot release the held coil. */
#if SST_ENABLE_SWITCH_FEEDBACK
    switches(0U, 1U, 0U); /* Both QF1 contacts absent: it is not a valid open position. */
    slow(2U);
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    assert(g_chb_dab_status.qf1_closed == 0U && g_chb_dab_status.qf1_open == 0U);
    slow(1U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    assert(Port_DOUT.Dout.AcRunning == 0U && Port_DOUT.Dout.AcFault == 1U);
    assert(Port_DOUT.Dout.Kt1Hold == 0U && Port_DOUT.Dout.Qf1OpenPulse == 1U);
    g_chb_dab_command.fault_reset = 1U;
    slow(60U);
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(Port_DOUT.Dout.Qf1OpenPulse == 0U);
    switches(0U, 0U, 1U);
    slow(3U);
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);

    /* A welded QF1 or absent position contacts cannot start automatically. */
    prepare_switch_fixture();
    switches(1U, 0U, 0U);
    g_chb_dab_command.run_enable = 1U;
    slow(105U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK && fake_ac_pwm == 0U);
    prepare_switch_fixture();
    switches(0U, 0U, 0U);
    g_chb_dab_command.run_enable = 1U;
    slow(105U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK && fake_ac_pwm == 0U);

    /* QF1 closure never arrives, even though voltage has reached its threshold. */
    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(105U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK && fake_ac_pwm == 0U);

    /* Bouncing QF1 contacts cannot restart the one-second action deadline. */
    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(1U);
    for (word = 0U; word < 105U; ++word)
    {
        switches((word % 3U) != 0U, 0U, (word % 3U) == 0U);
        slow(1U);
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK);

    /* Manual action after a long STOP starts a fresh feedback waiting period. */
    prepare_switch_fixture();
    slow(250U);
    assert(ChbDabControl_MaintenanceSwitch(2U, 0U) == CHB_SWITCH_OK);
    slow(1U);
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED && g_chb_dab_status.fault_code == CHB_FAULT_NONE);
    switches(1U, 0U, 0U);
    slow(3U);
    assert(ChbDabControl_MaintenanceSwitch(1U, 0U) == CHB_SWITCH_OK);
    switches(1U, 1U, 0U);
    slow(3U);
    assert(ChbDabControl_MaintenanceSwitch(1U, 2U) == CHB_SWITCH_OK);
    assert(g_chb_dab_status.qf1_close_command == 0U && g_chb_dab_status.kt1_close_command == 0U);
    assert(Port_DOUT.Dout.Kt1Hold == 0U && Port_DOUT.Dout.Qf1OpenPulse == 1U);
#else
    g_chb_dab_command.run_enable = 0U;
    slow(1U);
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED && fake_ac_pwm == 0U);
    assert(Port_DOUT.Dout.Kt1Hold == 0U && Port_DOUT.Dout.Qf1OpenPulse == 1U);
#endif

#if SST_ENABLE_SWITCH_OUTPUT && SST_ENABLE_SWITCH_FEEDBACK
    verify_precharge_feedback_interruption();
    verify_pll_feedback_interruption();
    verify_repeated_kt1_request(1U);
    verify_repeated_kt1_request(0U);
    verify_tuned_switch_times();
    assert(switch_regression_failures == 0U);
#endif

    /* Open-loop waveform checks do not command QF1/KT1; DIN remains observable. */
    fixture(CHB_AC_MODE_OPEN_LOOP);
    din_reads = 0U;
    switches(0U, 0U, 1U);
    Port_DOUT.SwitchINOUT16 = 0x1200U;
    FPGA_Interface[IO_PluginUnit_2 - 2].DOUT.SwitchINOUT16 = 0xA500U;
    g_chb_dab_command.run_enable = 1U;
    slow(5U);
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    assert(Port_DOUT.Dout.Qf1ClosePulse == 0U && Port_DOUT.Dout.Qf1OpenPulse == 0U);
    assert(Port_DOUT.Dout.Kt1Hold == 0U);
#if SST_ENABLE_SWITCH_FEEDBACK
    assert(din_reads == 5U);
    for (word = 0U; word < 50U; ++word)
    {
        ChbDabControl_Service();
    }
    assert(din_reads == 5U); /* No additional reads within the same 10ms tick. */
#else
    assert(din_reads == 0U);
#endif
    assert(interrupt_disable_calls == interrupt_restore_calls);
    puts("drawing channels; QF1 pulse / KT1 hold; real feedback startup and fault; manual action timing; open-loop isolation");
    return 0;
}
