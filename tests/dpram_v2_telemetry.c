#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "ControlData.h"
#include "FpgaIo.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstDpramV2.h"
#include "SstDpramV2Telemetry.h"
#include "SstFault.h"

volatile CHB_DAB_COMMAND g_chb_dab_command;
volatile CHB_DAB_PARAMETERS g_chb_dab_params;
volatile CHB_DAB_STATUS g_chb_dab_status;
volatile unsigned short g_chb_dab_protection_enable;
volatile FPGA_InterfaceBLOCK FPGA_Interface[15];
XXG_SwitchINOUT Port_DOUT;
XXG_DSP_CTL Port_DSP_CTL;
XXG_PM_CTL Module_PM_CTL;
XXG_PortRunFLAG PortRunFLAG;
XXG_PORT_FAULT Port_RealFault;
XXG_FaultTypeFLAG FaultTypeFLAG;
DPRAM_RemMeaSigFloat PORT_RealTime_DATA;
float SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave;
ushort HVAC_OC_Fault, HVAC_OV_Fault, FBLatch_TAD, PortLatch_TAD;
ushort FOLatch_A_TAD, FOLatch_B_TAD, FOLatch_C_TAD, ModuleFault_DisposeTimes;
ushort HA_PM_A1, HA_PM_A2, HA_PM_B1, HA_PM_B2, HA_PM_C1, HA_PM_C2;
uchar Flag_Clear_0, Flag_Clear_1;
static uint16_t dpram[0x2000];
static uint16_t observe[3];
static uint16_t data_words_written;
static uint16_t reads501;

#define SLOT(plugin) FPGA_Interface[(plugin) - 2U]

void SstDpramV2_WriteWord(uint16_t address, uint16_t value)
{
    assert(address >= 0x0502U && address <= 0x0A4FU);
    if (address != SST_DPRAM_V2_LIVE_EPOCH && address != SST_DPRAM_V2_MODULE_EPOCH &&
        address != SST_DPRAM_V2_HISTORY_EPOCH)
    {
        ++data_words_written;
        if (address >= 0x0801U)
            assert(dpram[SST_DPRAM_V2_HISTORY_EPOCH] & 1U);
        else if ((address >= 0x056EU && address <= 0x0570U) ||
                 (address >= 0x0598U && address <= 0x059DU) ||
                 (address >= 0x0600U && address <= 0x06A7U))
            assert(dpram[SST_DPRAM_V2_MODULE_EPOCH] & 1U);
        else if (address != 0x050EU && address != 0x050FU)
            assert(dpram[SST_DPRAM_V2_LIVE_EPOCH] & 1U);
    }
    dpram[address] = value;
}

uint16_t SstDpramV2_ObservationMask(uint16_t phase) { return observe[phase]; }
uint16_t SstDpramV2_SwitchPermission(void) { return 1U; }
ushort Port_UnitLinkRead(ushort plugin)
{
    ++reads501;
    return SLOT(plugin).UNIT_LINK;
}
ushort Port_FOStatusRead(uchar plugin, uchar latch)
{
    return latch ? SLOT(plugin).FO_LATCH : SLOT(plugin).FO;
}
ushort Port_FBStatusRead(uchar latch)
{
    return latch ? SLOT(MainControl_PluginUnit_4).FB_LATCH : SLOT(MainControl_PluginUnit_4).FB;
}
ushort Port_EffectiveFOEnable(ushort plugin)
{
    return SLOT(plugin).FO_EN;
}
ushort Port_EffectiveFBEnable(void)
{
    return SLOT(MainControl_PluginUnit_4).FB_EN;
}
ushort Module_StatusRead(uchar plugin, uchar channel, uchar latch)
{
    assert(latch == 0U && channel < 8U);
    return SLOT(plugin).STATUS[channel].STATUS16;
}
ushort Port_AD_ProtectStatusRead(uchar latch)
{
    return latch ? SLOT(Analog_PluginUnit_3).AD_PROT_LATCH : SLOT(Analog_PluginUnit_3).AD_PROT;
}
void ChbDabControl_ReadStartupStatus(CHB_STARTUP_STATUS *status)
{
    status->active_mode = g_chb_dab_command.ac_mode;
    status->state_elapsed_10ms = 0x12345678U;
    status->precharge_stable_10ms = 2U;
    status->pll_stable_10ms = 3U;
    status->precharge_ready = 0x0BU;
    status->topology_valid = 1U;
}

static void ServiceUntilPublished(void)
{
    uint16_t i;
    for (i = 0U; i < 64U; ++i)
    {
        data_words_written = 0U;
        SstDpramV2Telemetry_Service();
        assert(data_words_written <= 16U);
    }
    assert(!(dpram[SST_DPRAM_V2_LIVE_EPOCH] & 1U));
    assert(!(dpram[SST_DPRAM_V2_MODULE_EPOCH] & 1U));
    assert(!(dpram[SST_DPRAM_V2_HISTORY_EPOCH] & 1U));
}

int main(void)
{
    g_chb_dab_command.ac_mode = CHB_AC_MODE_CLOSED_LOOP;
    HA_PM_A1 = 1U;
    HA_PM_A2 = 3U;
    HA_PM_B2 = 0x20U;
    SLOT(Com_A1_PluginUnit_5).UNIT_LINK = 0x0101U;
    SLOT(Com_A2_PluginUnit_6).UNIT_LINK = 0x0101U;
    SLOT(Com_B2_PluginUnit_10).UNIT_LINK = 0x2020U;
    SLOT(Com_A2_PluginUnit_6).DC[0] = 640;
    SLOT(Com_A2_PluginUnit_6).IR[0] = -13;
    SLOT(Com_A2_PluginUnit_6).IG[0] = 25;
    SLOT(Com_A2_PluginUnit_6).TEMP[0] = -10;
    SLOT(Com_B2_PluginUnit_10).DC[5] = 820;
    SLOT(Input_A_PluginUnit_8).FO_EN = 0x0001U;
    SLOT(Input_A_PluginUnit_8).FO = 0x0301U;
    SLOT(Input_A_PluginUnit_8).FO_LATCH = 0x4100U; /* 首报时的掩码与当前使能不同。 */
    SLOT(Analog_PluginUnit_3).AD_PROT_LATCH = 0x2000U;
    SLOT(Putout_A_PluginUnit_7).SHUTDOWN = 0xFCFEU;
    SLOT(MainControl_PluginUnit_4).FB_LATCH = 0x0040U;
    PORT_RealTime_DATA.PORT_V_A = 1.F;
    PORT_RealTime_DATA.PORT_V_B = 2.F;
    Module_PM_CTL.PM_CTL16 = 0x0F11U;
    observe[0] = 0x0300U;
    observe[1] = 0x2000U;
    g_chb_dab_params.sample_period_s = 0.0001F;
    g_chb_dab_status.tick_10ms = 0x00012345UL;
    g_chb_dab_status.qf1_closed = 1U;

    SstFault_Init();
    SstDpramV2Telemetry_Init();
    assert(!SstDpramV2Telemetry_TakeLiveNotification());
    assert(dpram[SST_DPRAM_V2_LIVE_EPOCH] == 1U);
    ServiceUntilPublished();
    assert(SstDpramV2Telemetry_TakeLiveNotification());
    assert(!SstDpramV2Telemetry_TakeLiveNotification());
    assert(dpram[0x050CU] == 0x2345U && dpram[0x050DU] == 1U);
    assert(dpram[0x050EU] == 100U && dpram[0x050FU] == 1000U);
    assert(dpram[0x0514U] == 8U && dpram[0x0515U] == 1U);
    assert(dpram[0x0518U] == 0x5678U && dpram[0x0519U] == 0x1234U);
    assert(dpram[0x0524U] == 0x0BU);
    assert(dpram[0x0540U] == 0U && dpram[0x0541U] == 0x3F80U);
    assert(dpram[0x0580U] == 0x0301U && dpram[0x0583U] == 0x0101U);
    assert(dpram[0x0586U] == 0x0101U && dpram[0x0589U] == 0x0200U);
    assert(dpram[0x0592U] == 1U && dpram[0x0595U] == 0x0100U);
    assert(dpram[0x05A0U] == 0x0040U && dpram[0x05A8U] == 0x2000U);
    assert(dpram[0x05A3U] == 0x3CFEU && dpram[0x05A9U] == 0x0211U);
    assert(dpram[0x0598U] == 0x0300U && dpram[0x059BU] == 0x0100U);
    assert(dpram[0x0620U] == 640U && dpram[0x0621U] == (uint16_t)-13);
    assert(dpram[0x0622U] == 25U && dpram[0x0623U] == (uint16_t)-10);
    assert(dpram[0x056EU] == (uint16_t)-10 && dpram[0x0624U] == 0U);
#if SST_PHASE_A_ONLY_TEST
    assert(reads501 == 4U && dpram[0x0581U] == 0 && dpram[0x059CU] == 0);
    assert(dpram[0x0543U] == 0U && dpram[0x066CU] == 0U);
#else
    assert(reads501 == 6U && dpram[0x0581U] == 0x2000U && dpram[0x059CU] == 0x2000U);
    assert(dpram[0x0543U] == 0x4000U && dpram[0x066CU] == 820U);
#endif

    /* A9 的 F1 在线保留且 Udc 不可用，模块快照在 100ms 时撤销它的数据资格。 */
    SLOT(Com_A2_PluginUnit_6).UNIT_LINK = 0x0001U;
    g_chb_dab_status.tick_10ms += 9U;
    ServiceUntilPublished();
    assert(dpram[0x0583U] == 0x0101U && dpram[0x0586U] == 1U);
    assert(dpram[0x059BU] == 0x0100U);
    ++g_chb_dab_status.tick_10ms;
    ServiceUntilPublished();
    assert(dpram[0x059BU] == 0U && dpram[0x0620U] == 0U);

    /* 501 故障位置进入当前报警及详细历史，互不受后续实时掉线图改变的影响。 */
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_UNIT_LINK;
    g_chb_dab_status.unit_link_missing[0] = 0x0100U;
    SstFault_Service();
    g_chb_dab_status.tick_10ms += 3U;
    ServiceUntilPublished();
    assert(dpram[0x0700U] == 0xA000U && dpram[0x0701U] == 8U);
    assert(dpram[0x0720U] == 39103U && dpram[0x058CU] == 0x0100U);
    assert(dpram[0x0801U] == 1U && dpram[0x0802U] == 1U);
    assert(dpram[0x0810U] == 39103U && dpram[0x081FU] == 0x0100U);

    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    SstFault_Service();
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    g_chb_dab_status.unit_link_missing[0] = 0x0301U;
    SstFault_Service();
    g_chb_dab_status.tick_10ms += 3U;
    ServiceUntilPublished();
    assert(dpram[0x058CU] == 0U);
    assert(dpram[0x0801U] == 2U && dpram[0x0802U] == 2U);
    assert(dpram[0x0810U] == 19101U && dpram[0x081FU] == 0U);
    assert(dpram[0x0822U] == 39103U && dpram[0x0831U] == 0x0100U);
    /* At open-loop arming, the startup check has entered FAULT before PWM
     * was enabled, and the legacy ISR has captured FB without setting FO_A. */
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    SstFault_Service();
    SLOT(MainControl_PluginUnit_4).FB_EN = 0x0040U;
    SLOT(Input_A_PluginUnit_8).FO_EN = 0x0101U;
    SLOT(Input_A_PluginUnit_8).FO_LATCH = 0x0101U;
    SLOT(Com_A1_PluginUnit_5).STATUS[0].STATUS16 = 0x0202U; /* A1 Ig fault. */
    SLOT(Com_A2_PluginUnit_6).STATUS[0].STATUS16 = 0x0404U; /* A9 Ir fault. */
    g_chb_dab_command.ac_mode = CHB_AC_MODE_OPEN_LOOP;
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = CHB_FAULT_HARDWARE;
    FBLatch_TAD = 0x0040U;
    FOLatch_A_TAD = 0U;
    SstFault_Service();
    g_chb_dab_status.tick_10ms += 3U;
    ServiceUntilPublished();
    assert(dpram[0x0510U] == CHB_STATE_FAULT && dpram[0x0511U] == CHB_AC_MODE_OPEN_LOOP);
    assert((dpram[0x0513U] & 3U) == 0U && dpram[0x0595U] == 0x0101U);
    assert(dpram[0x0701U] == CHB_FAULT_HARDWARE && dpram[0x0702U] == 19105U);
    assert(dpram[0x0704U] == 2U && dpram[0x0720U] == 19105U && dpram[0x0721U] == 19117U);
    assert(dpram[0x0801U] == 4U && dpram[0x0802U] == 4U);
    assert(dpram[0x0810U] == 19117U && dpram[0x0812U] == CHB_FAULT_HARDWARE);
    assert(dpram[0x0814U] == 0U && dpram[0x0816U] == 0x0040U);
    assert(dpram[0x0817U] == 0x0100U && dpram[0x0818U] == 0U && dpram[0x0819U] == 0U);
    assert(dpram[0x0822U] == 19105U && dpram[0x0824U] == CHB_FAULT_HARDWARE);
    assert(dpram[0x0826U] == 0U && dpram[0x0828U] == 0x0040U);
    assert(dpram[0x0829U] == 1U && dpram[0x082AU] == 0U && dpram[0x082BU] == 0U);
    assert(dpram[0x0834U] == 19101U && dpram[0x0846U] == 39103U);
    printf("PASS V2 telemetry phase=%u: snapshot bounds, F1 validity and separate Ig/Ir history\n",
           SST_PHASE_A_ONLY_TEST);
    return 0;
}
