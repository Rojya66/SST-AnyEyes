/* Host-only substitutes. No hardware, IRQ or PWM APIs exist in this test. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "SstFault.h"
#include "ChbDabControl.h"

volatile CHB_DAB_STATUS g_chb_dab_status;
volatile CHB_DAB_COMMAND g_chb_dab_command;
static struct { unsigned short PORT_FAULT16; } Port_RealFault;
static struct { unsigned short FaultTypeFLAG16; } FaultTypeFLAG;
static unsigned short PortLatch_TAD, FBLatch_TAD, FOLatch_A_TAD,
    FOLatch_B_TAD, FOLatch_C_TAD, ModuleFault_DisposeTimes;
static unsigned short HVAC_OC_Fault, HVAC_OV_Fault;
static unsigned char Flag_Clear_0, Flag_Clear_1;

/* INSERT_FAULT_IMPLEMENTATION */

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
        SST_FAULT_HW_UNCLASSIFIED, SST_FAULT_BAD_CONFIGURATION
    };
    SstFault_Init();
    stopped_reset();
    assert(SstFault_GetActiveCount() == 0);
    assert(SstFault_GetLevel() == SST_FAULT_LEVEL_NONE);
    assert(SstFault_GetHistoryLevel() == SST_FAULT_LEVEL_NONE);
    assert(!SstFault_GetHistory(0, &record));
    assert(SstFault_GetLegacyHistoryWord(0) == 1);
    for(i = 1; i < 7; ++i)
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
