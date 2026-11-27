/* Host-only substitutes. No hardware, IRQ or PWM APIs exist in this test. */
#include <assert.h>
#include <stdio.h>
#include "ChbDabControl.h"
#include "FpgaIo.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstFault.h"

volatile CHB_DAB_STATUS g_chb_dab_status;
volatile CHB_DAB_COMMAND g_chb_dab_command;
XXG_PORT_FAULT Port_RealFault;
XXG_FaultTypeFLAG FaultTypeFLAG;
ushort PortLatch_TAD, FBLatch_TAD, FOLatch_A_TAD,
    FOLatch_B_TAD, FOLatch_C_TAD, ModuleFault_DisposeTimes;
ushort HVAC_OC_Fault, HVAC_OV_Fault;
uchar Flag_Clear_0, Flag_Clear_1;

static unsigned short fb_first, fb_enable;
static unsigned short fo_first[18], fo_enable[18];
static unsigned short unit_status[18][8];

ushort Port_FBStatusRead(uchar latch)
{
    assert(latch == 1U);
    return fb_first;
}
ushort Port_EffectiveFBEnable(void)
{
    return fb_enable;
}
ushort Port_FOStatusRead(uchar plugin, uchar latch)
{
    assert(latch == 1U);
    return fo_first[plugin];
}
ushort Port_EffectiveFOEnable(ushort plugin)
{
    return fo_enable[plugin];
}
ushort Module_StatusRead(uchar plugin, uchar channel, uchar latch)
{
    assert(latch == 0U && channel < 8U);
    return unit_status[plugin][channel];
}

static void stopped_reset(void)
{
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
    g_chb_dab_command.run_enable = 0;
    g_chb_dab_command.dab_enable = 0;
    Port_RealFault.PORT_FAULT16 = 0;
    FaultTypeFLAG.FaultTypeFLAG16 = 0;
    HVAC_OC_Fault = HVAC_OV_Fault = 0;
    Flag_Clear_0 = Flag_Clear_1 = 0;
    SstFault_Service();
}

int main(void)
{
    SST_FAULT_RECORD record;
    unsigned short i;
    static const unsigned short control_codes[] = {
        0U, SST_FAULT_PRECHARGE_TIMEOUT, SST_FAULT_PLL_TIMEOUT,
        SST_FAULT_SW_AC_OC, SST_FAULT_SW_DC_OV,
        SST_FAULT_HW_UNCLASSIFIED, SST_FAULT_BAD_CONFIGURATION, SST_FAULT_SWITCH_FEEDBACK
    };
    SstFault_Init();
    stopped_reset();
    assert(SstFault_GetActiveCount() == 0);
    assert(SstFault_GetLevel() == SST_FAULT_LEVEL_NONE);
    assert(SstFault_GetHistoryLevel() == SST_FAULT_LEVEL_NONE);
    assert(!SstFault_GetHistory(0, &record));
    assert(SstFault_GetLegacyHistoryWord(0) == 1);
    for(i = 1; i < 8; ++i)
    {
        g_chb_dab_status.state = CHB_STATE_FAULT;
        g_chb_dab_status.fault_code = i;
        g_chb_dab_status.tick_10ms = i * 100;
        SstFault_Service();
        assert(SstFault_GetFirstCode() == control_codes[i]);
        assert(SstFault_GetCode(1) == 0);
        assert(SstFault_GetHistoryCount() == i);
        assert(SstFault_GetLegacyHistoryWord(0) == i + 2);
        assert(SstFault_GetLegacyHistoryWord(i + 1) == control_codes[i]);
        assert(SstFault_GetLegacyHistoryWord(1) == 0);
        SstFault_Service(); SstFault_Service();
        assert(SstFault_GetHistoryCount() == i);
        assert(SstFault_GetHistory(0, &record));
        assert(record.control_fault == i && record.tick_10ms == i * 100);
        stopped_reset();
        assert(SstFault_GetActiveCount() == 0);
    }
    puts("PASS: control causes, severity, deduplication, reset and timestamps");

    /* The old software OC/OV path raises CHB_FAULT_HARDWARE: classify by the
     * latched HVAC flags, not the reused PORT_FAULT bit13/14 comments. */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    Port_RealFault.PORT_FAULT16 = 0xE000U;
    HVAC_OC_Fault = HVAC_OV_Fault = 1;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 2);
    assert(SstFault_GetCode(0) == SST_FAULT_SW_AC_OC);
    assert(SstFault_GetCode(1) == SST_FAULT_SW_DC_OV);
    assert(SstFault_GetLevel() == SST_FAULT_LEVEL_2);
    /* Reset is staged; stale flags must not create or clear an episode early. */
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_status.fault_code = 0;
    Flag_Clear_0 = 1;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 2);
    Flag_Clear_0 = 0; Flag_Clear_1 = 1;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 2);
    stopped_reset();
    puts("PASS: legacy OC/OV classification and staged reset");

    /* Port hardware OC represented in both words must produce one event. */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    FaultTypeFLAG.FaultTypeFLAG16 = 0x0002U;
    Port_RealFault.PORT_FAULT16 = 0x8400U;
    PortLatch_TAD = 2; FBLatch_TAD = 2;
    FOLatch_A_TAD = 0x10; ModuleFault_DisposeTimes = 2;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 1);
    assert(SstFault_GetCode(0) == SST_FAULT_HW_PORT_OC);
    assert(SstFault_GetHistory(0, &record));
    assert(record.port_latch == 2 && record.fb_latch == 2 &&
           record.fo_a == 0x10 && record.module_dispose == 2);
    stopped_reset();
    /* Old FO/FB shadow words are not explicitly cleared by the legacy reset.
     * They remain useful raw evidence, but cannot invent a new active fault. */
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 0);
    g_chb_dab_status.state = CHB_STATE_RUNNING;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 0);
    puts("PASS: duplicate sources, raw evidence and stale latch suppression");

    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    FaultTypeFLAG.FaultTypeFLAG16 = 0x2000U;
    SstFault_Service();
    assert(SstFault_GetCode(0) == SST_FAULT_HW_MODULE);
    /* Delayed decode enriches the same episode; its original first cause
     * stays fixed and repeat scans do not duplicate either record. */
    FaultTypeFLAG.FaultTypeFLAG16 = 0x2040U;
    SstFault_Service();
    assert(SstFault_GetCode(0) == SST_FAULT_HW_MODULE);
    assert(SstFault_GetCode(1) == SST_FAULT_HW_MODULE_OC);
    i = SstFault_GetHistoryCount(); SstFault_Service();
    assert(SstFault_GetHistoryCount() == i);
    stopped_reset();
    puts("PASS: aggregate module cause and delayed specific decode");

    /* The arming check has already stopped PWM, but the ISR left FO_A zero. */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    FBLatch_TAD = fb_first = fb_enable = 0x0040U;
    FOLatch_A_TAD = 0U;
    fo_enable[Input_A_PluginUnit_8] = fo_first[Input_A_PluginUnit_8] = 1U;
    unit_status[Com_A1_PluginUnit_5][0] = 0x0202U;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 1U && SstFault_GetFirstCode() == SST_FAULT_HW_MODULE_OC);
    assert(SstFault_GetHistory(0, &record));
    assert(record.code == SST_FAULT_HW_MODULE_OC && record.fo_a == 1U && record.fb_latch == 0x0040U);
    assert(record.fault_type == 0U);
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 1U);
    stopped_reset();

    /* A2 starts at bit8 of FO. A1 reports Ir immediately, but A11's Ig
     * arrives after the first read. Exclude the old Ig status at A2. */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    fo_first[Input_A_PluginUnit_8] = 0x0401U;
    fo_enable[Input_A_PluginUnit_8] = 0x0403U;
    unit_status[Com_A1_PluginUnit_5][0] = 0x0404U;
    unit_status[Com_A1_PluginUnit_5][1] = 0x0202U;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 0U);
    unit_status[Com_A2_PluginUnit_6][2] = 0x0202U;
    ++g_chb_dab_status.tick_10ms;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 0U);
    ++g_chb_dab_status.tick_10ms;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 2U && SstFault_GetFirstCode() == SST_FAULT_HW_MODULE_OC);
    assert(SstFault_GetCode(1) == SST_FAULT_HW_MODULE_IR_OC);
    assert(SstFault_GetHistory(0, &record) && record.code == SST_FAULT_HW_MODULE_IR_OC && record.fo_a == 1U);
    assert(SstFault_GetHistory(1, &record) && record.code == SST_FAULT_HW_MODULE_OC && record.fo_a == 0x0400U);
    stopped_reset();

    /* An invalid STATUS read must not confirm Ig. Keep the enabled E5 bits
     * when valid status is still unavailable at the second read. */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    fo_first[Input_A_PluginUnit_8] = 0x0401U;
    fo_enable[Input_A_PluginUnit_8] = 1U;
    unit_status[Com_A1_PluginUnit_5][0] = 0xFFFFU;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 0U);
    g_chb_dab_status.tick_10ms += 2U;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 1U && SstFault_GetFirstCode() == SST_FAULT_HW_MODULE);
    assert(SstFault_GetHistory(0, &record) && record.fo_a == 1U);
    stopped_reset();

    /* A direct 701 startup trip can precede the 403 first latch. */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    FBLatch_TAD = fb_first = 0U;
    unit_status[Com_A1_PluginUnit_5][0] = 0x0202U;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 0U);
    fb_first = 0x0040U;
    g_chb_dab_status.tick_10ms += 2U;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 1U && SstFault_GetFirstCode() == SST_FAULT_HW_MODULE_OC);
    assert(SstFault_GetHistory(0, &record) && record.fo_a == 1U && record.fb_latch == 0x0040U);
    stopped_reset();

    /* Without a matching 403 source, the 701 latch cannot attribute this trip. */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    fb_first = 0U;
    SstFault_Service();
    g_chb_dab_status.tick_10ms += 2U;
    SstFault_Service();
    assert(SstFault_GetActiveCount() == 1U && SstFault_GetFirstCode() == SST_FAULT_HW_UNCLASSIFIED);
    assert(SstFault_GetHistory(0, &record) && record.fo_a == 0U);
    stopped_reset();
    puts("PASS: first-latched UNIT Ig and Ir, bounded retry, signal pairing and fallback");

    /* A closed-loop trip reports all matched causes. A1 has both OV and CHB,
     * OV is shared with A14, and CHB is shared with C9. UV and SHUTDOWN
     * bits alone leave A5 and A6 unclassified. */
    {
        static const struct
        {
            ushort code, fo_a, fo_c;
        } expected[] = {
            {SST_FAULT_HW_MODULE_OV, 0x2001U, 0U},
            {SST_FAULT_HW_MODULE_OT, 0x0004U, 0U},
            {SST_FAULT_HW_CHB_BRIDGE, 0x0001U, 0x0100U},
            {SST_FAULT_HW_DAB_BRIDGE, 0x0004U, 0U},
            {SST_FAULT_HW_MODULE, 0x0030U, 0U},
            {SST_FAULT_HW_MODULE_OC, 0U, 0U},
        };
        const ushort count = sizeof(expected) / sizeof(expected[0]);
        unsigned long sequence = SstFault_GetHistorySequence();

        g_chb_dab_command.ac_mode = CHB_AC_MODE_CLOSED_LOOP;
        g_chb_dab_status.state = CHB_STATE_RUNNING;
        SstFault_Service();
        assert(SstFault_GetActiveCount() == 0U);
        g_chb_dab_status.state = CHB_STATE_FAULT;
        g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
        FBLatch_TAD = fb_first = 0x4040U;
        fb_enable = 0x4440U;
        FOLatch_A_TAD = fo_first[Input_A_PluginUnit_8] = 0x203DU;
        fo_enable[Input_A_PluginUnit_8] = 0x2035U;
        FOLatch_C_TAD = fo_first[Input_C_PluginUnit_16] = fo_enable[Input_C_PluginUnit_16] = 0x0100U;
        fo_first[Input_B_PluginUnit_12] = fo_enable[Input_B_PluginUnit_12] = 1U;
        unit_status[Com_A1_PluginUnit_5][0] = 0x1111U;
        unit_status[Com_A1_PluginUnit_5][2] = 0x2828U;
        unit_status[Com_A1_PluginUnit_5][3] = 0x0202U; /* Disabled FO. */
        unit_status[Com_A1_PluginUnit_5][4] = 0x0040U;
        unit_status[Com_A1_PluginUnit_5][5] = 0x0080U;
        unit_status[Com_A2_PluginUnit_6][5] = 0x0101U;
        unit_status[Com_B1_PluginUnit_9][0] = 0x0404U; /* No matching FB. */
        unit_status[Com_C2_PluginUnit_14][0] = 0x1010U;
        /* Legacy Ig remains reportable without borrowing another cause's FO. */
        FaultTypeFLAG.FaultTypeFLAG16 = 0x20ECU;
        SstFault_Service();
        assert(SstFault_GetActiveCount() == 0U);
        g_chb_dab_status.tick_10ms += 2U;
        SstFault_Service();
        assert(SstFault_GetActiveCount() == count);
        assert(SstFault_GetHistorySequence() == sequence + count);
        for (i = 0U; i < count; ++i)
        {
            assert(SstFault_GetCode(i) == expected[i].code);
            assert(SstFault_GetHistory(count - i - 1U, &record));
            assert(record.code == expected[i].code && record.level == SST_FAULT_LEVEL_1);
            assert(record.fo_a == expected[i].fo_a && record.fo_b == 0U && record.fo_c == expected[i].fo_c);
            assert(record.fb_latch == 0x4040U && record.sequence == sequence + i + 1U);
        }
        SstFault_Service();
        assert(SstFault_GetHistorySequence() == sequence + count);
        stopped_reset();
        assert(SstFault_GetActiveCount() == 0U);
        assert(SstFault_GetHistorySequence() == sequence + count);

        g_chb_dab_status.state = CHB_STATE_FAULT;
        g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
        FBLatch_TAD = fb_first = 0x0040U;
        FOLatch_A_TAD = fo_first[Input_A_PluginUnit_8] = 4U;
        unit_status[Com_A1_PluginUnit_5][2] = 0x0101U;
        SstFault_Service();
        assert(SstFault_GetActiveCount() == 1U && SstFault_GetFirstCode() == SST_FAULT_HW_MODULE_OV);
        assert(SstFault_GetHistory(0, &record));
        assert(record.fo_a == 4U && record.fo_b == 0U && record.fo_c == 0U);
        assert(record.sequence == sequence + count + 1U);
        stopped_reset();
    }
    puts("PASS: simultaneous UNIT causes, physical positions, deduplication and episode reset");

    for(i = 0; i < 40; ++i)
    {
        g_chb_dab_status.state = CHB_STATE_FAULT;
        g_chb_dab_status.fault_code = CHB_FAULT_PLL_TIMEOUT;
        g_chb_dab_status.tick_10ms = 1000 + i;
        SstFault_Service(); stopped_reset();
    }
    assert(SstFault_GetHistoryCount() == SST_FAULT_HISTORY_CAPACITY);
    assert(SstFault_GetHistory(0, &record) && record.tick_10ms == 1039);
    assert(SstFault_GetHistory(31, &record) && record.tick_10ms == 1008);
    assert(SstFault_GetHistoryLevel() == SST_FAULT_LEVEL_2);
    assert(!SstFault_GetHistory(32, &record));
    assert(!SstFault_GetHistory(0, 0));
    assert(SstFault_GetHistoryCode(32) == 0);
    assert(SstFault_GetLegacyHistoryWord(34) == 0);
    assert(SstFault_GetLegacyHistoryWord(0) > 0 &&
           SstFault_GetLegacyHistoryWord(0) < SST_FAULT_LEGACY_HISTORY_WORDS);
    i = SstFault_GetLegacyHistoryWord(0);
    assert(SstFault_GetLegacyHistoryWord(i == 1 ? 33 : i - 1) ==
           SST_FAULT_PLL_TIMEOUT);
    assert(SstFault_GetCode(0xffffU) == 0);
    puts("PASS: repeated episodes, 32-record wrap, history severity and bounds");
    return 0;
}
