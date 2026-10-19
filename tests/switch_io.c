/* Host-only switch board state-machine scenarios using production functions. */
static void switches(unsigned short ac, unsigned short bypass)
{
    fake_din = 0U;
    if ((ac != 0U) == SST_SWITCH_AC_CLOSED_HIGH)
        fake_din |= SST_SWITCH_AC_CLOSED_MASK;
    if ((bypass != 0U) == SST_SWITCH_BYPASS_CLOSED_HIGH)
        fake_din |= SST_SWITCH_BYPASS_CLOSED_MASK;
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
    switches(0U, 0U);
    din_reads = 0U;
    g_chb_dab_params.precharge_stable_10ms = 2U;
    g_chb_dab_params.bypass_delay_10ms = 2U;
    g_chb_dab_params.pll_lock_10ms = 2U;
    g_chb_dab_status.dc_voltage_min_v = bus_voltage();
    g_chb_dab_status.pll_locked = 1U;
    slow(3U);
}

int main(void)
{
    unsigned int word;
    saved_parameters = g_chb_dab_params;
    saved_command = g_chb_dab_command;
    /* Exhaustive 16-bit logical words prove unrelated bits survive polarity conversion. */
    for (word = 0U; word <= 0xFFFFU; ++word)
    {
        unsigned short expected = (unsigned short)word;
        Port_DOUT.SwitchINOUT16 = (unsigned short)word;
        FPGA_Interface[0].DOUT.SwitchINOUT16 = 0x5A5AU;
        Port_IO_Out();
#if SST_ENABLE_SWITCH_OUTPUT
        if (!SST_SWITCH_CLOSE_ACTIVE_HIGH) expected ^= 0x0020U;
        if (!SST_SWITCH_OPEN_ACTIVE_HIGH) expected ^= 0x0010U;
        if (!SST_SWITCH_BYPASS_ACTIVE_HIGH) expected ^= 0x0080U;
        assert(FPGA_Interface[0].DOUT.SwitchINOUT16 == expected);
#else
        assert(FPGA_Interface[0].DOUT.SwitchINOUT16 == 0x5A5AU);
        (void)expected;
#endif
        assert(Port_DOUT.SwitchINOUT16 == word);
    }
    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(1U);
    assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);
    assert(Port_DOUT.Bbits.Breaker_InnerOn == 1U);
    assert(Port_DOUT.Bbits.Breaker_InnerOff == 0U);
    assert(Port_DOUT.Bbits.Contactor_Inner == 0U);
    assert(fake_ac_pwm == 0U);
#if SST_ENABLE_SWITCH_FEEDBACK
    slow(10U); /* Voltage alone cannot bypass the precharge resistor. */
    assert(g_chb_dab_status.state == CHB_STATE_PRECHARGE);
    switches(1U, 0U);
    slow(5U);
#else
    slow(2U);
    assert(din_reads == 0U);
#endif
    assert(g_chb_dab_status.state == CHB_STATE_BYPASS_DELAY);
    assert(Port_DOUT.Bbits.Contactor_Inner == 1U);
#if SST_ENABLE_SWITCH_FEEDBACK
    slow(10U);
    assert(g_chb_dab_status.state == CHB_STATE_BYPASS_DELAY && fake_ac_pwm == 0U);
    switches(1U, 1U);
    slow(3U);
#else
    slow(2U);
#endif
    assert(g_chb_dab_status.state == CHB_STATE_WAIT_PLL);
    slow(5U);
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING && fake_ac_pwm == 1U);
    slow(60U);
    assert(Port_DOUT.Bbits.Breaker_InnerOn == 0U && Port_DOUT.Bbits.Breaker_InnerOff == 0U);
#if SST_ENABLE_SWITCH_FEEDBACK
    switches(0U, 1U);
    slow(2U);
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    slow(1U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK);
    assert(fake_ac_pwm == 0U && fake_dab_pwm == 0U);
    assert(Port_DOUT.Bbits.Contactor_Inner == 0U);
    assert(Port_DOUT.Bbits.Breaker_InnerOff == 1U);
    g_chb_dab_command.fault_reset = 1U;
    slow(60U);
    assert(g_chb_dab_status.state == CHB_STATE_FAULT);
    assert(Port_DOUT.Bbits.Breaker_InnerOff == 0U);
    switches(0U, 0U);
    slow(3U);
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED);

    /* Welded contact at STOP and missing AC closure each produce fault7. */
    prepare_switch_fixture();
    switches(1U, 0U);
    g_chb_dab_command.run_enable = 1U;
    slow(105U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK && fake_ac_pwm == 0U);
    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(105U);
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK && fake_ac_pwm == 0U);
    /* Bouncing feedback must not extend the one-second action deadline indefinitely. */
    prepare_switch_fixture();
    g_chb_dab_command.run_enable = 1U;
    slow(1U);
    for (word = 0U; word < 105U; ++word)
    {
        switches((word % 3U) != 0U, 0U);
        slow(1U);
    }
    assert(g_chb_dab_status.fault_code == CHB_FAULT_SWITCH_FEEDBACK);
#else
    g_chb_dab_command.run_enable = 0U;
    slow(1U);
    assert(g_chb_dab_status.state == CHB_STATE_STOPPED && fake_ac_pwm == 0U);
    assert(Port_DOUT.Bbits.Contactor_Inner == 0U && Port_DOUT.Bbits.Breaker_InnerOff == 1U);
#endif
    /* Open-loop waveform checks leave external contactors and DIN service alone. */
    fixture(CHB_AC_MODE_OPEN_LOOP);
    din_reads = 0U;
    Port_DOUT.SwitchINOUT16 = 0x1200U;
    FPGA_Interface[0].DOUT.SwitchINOUT16 = 0xA500U;
    g_chb_dab_command.run_enable = 1U;
    slow(5U);
    assert(g_chb_dab_status.state == CHB_STATE_RUNNING);
    assert(Port_DOUT.SwitchINOUT16 == 0x1200U);
    assert(FPGA_Interface[0].DOUT.SwitchINOUT16 == 0xA500U);
    assert(din_reads == 0U);
    assert(interrupt_disable_calls == interrupt_restore_calls);
    puts("polarity/full-word preservation; precharge/bypass/PWM/pulse/stop; feedback timeout/loss/reset; open-loop isolation");
    return 0;
}
