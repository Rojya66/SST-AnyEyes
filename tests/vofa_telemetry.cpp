
/* Exercise the actual SstDebug_ProtocolService map, not an independent encoder copy. */
static std::vector<uint8_t> ExpectedJustFloat(const float *values, unsigned count)
{
    std::vector<uint8_t> result;
    for (unsigned i = 0; i < count; ++i)
    {
        uint32_t word;
        memcpy(&word, &values[i], sizeof(word));
        result.push_back((uint8_t)word);
        result.push_back((uint8_t)(word >> 8));
        result.push_back((uint8_t)(word >> 16));
        result.push_back((uint8_t)(word >> 24));
    }
    result.insert(result.end(), {0, 0, 0x80, 0x7F});
    return result;
}

int main(void)
{
    Reset();
    g_chb_dab_status.state = CHB_STATE_RUNNING;
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
    g_chb_dab_status.dc_voltage_average_v = 34.75F;
    g_chb_dab_status.id_reference_a = 0.375F;
    g_chb_dab_status.id_feedback_a = 0.25F;
    g_chb_dab_status.iq_feedback_a = -0.125F;
    g_chb_dab_status.startup_phase = CHB_STARTUP_VOLTAGE_HANDOVER;
    g_chb_dab_status.startup_voltage_reference_v = 34.5F;
    g_chb_dab_status.modulation_actual = 0.675F;
    g_chb_dab_status.pll_locked = 1U;
    HA_Port_Volt_A = -123.5F;
    HA_Port_Current_A = -0.625F;
    PORT_RealTime_DATA.PORT_V_A = 100.0F; /* Distinct RMS value detects wrong voltage map. */
    const float values[12] = {4, 0, 34.75F, 0.375F, 0.25F, -0.125F,
                             2, 34.5F, 0.675F, 1, -123.5F, -0.625F};
    SstDebug_ProtocolService(SST_TELEMETRY_PERIOD_10MS - 1U);
    Drain();
    assert(hwTx.empty());
    g_chb_dab_status.tick_10ms = SST_TELEMETRY_PERIOD_10MS;
    SstDebug_ProtocolService(SST_TELEMETRY_PERIOD_10MS);
    Drain();
    assert(hwTx.size() == 52U);
    assert(hwTx == ExpectedJustFloat(values, 12));
    hwTx.clear();
    SstDebug_ProtocolService(SST_TELEMETRY_PERIOD_10MS);
    Drain();
    assert(hwTx.empty()); /* No duplicate frame at the same tick. */

    /* Full queues reject complete frames and account for the dropped frame. */
    Reset();
    for (unsigned i = 0; i < 8U; ++i)
        VOFA_Send12Float(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    assert(SstSerial_TxFreeFrames() == 0U);
    assert(SstDebug_GetTxDropped() == 0U);
    VOFA_Send12Float(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
    assert(SstDebug_GetTxDropped() == 1U);
    SstDebug_ProtocolService(SST_TELEMETRY_PERIOD_10MS);
    assert(SstDebug_GetTxDropped() == 1U); /* Periodic sender skips without flooding. */
    Drain();
    assert(hwTx.size() == 8U * 52U);

    /* The legacy four-channel encoder remains byte compatible. */
    Reset();
    VOFA_Send4Float(1, -2, 0, 3.5F);
    Drain();
    const float legacy[4] = {1, -2, 0, 3.5F};
    assert(hwTx == ExpectedJustFloat(legacy, 4));
    puts("PASS: VOFA 12-channel map/52-byte JustFloat/period gating/queue saturation/legacy four-channel encoder");
    return 0;
}
