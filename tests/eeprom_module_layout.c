/* Real FPGA layout/hardware-parameter adapters, with FPGA windows in host RAM. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "SstControlConfig.h"
#include "ChbDabControl.h"
#include "ControlData.h"
#include "ProtectionData.h"
#include "PortStatusData.h"
#include "DpramHostData.h"
#include "DpramDefaults.h"
#include "FpgaRegister.h"
#include "FpgaInit.h"
#include "FpgaIo.h"
#include "SstBoardConfig.h"
#include "SstHardwareParameters.h"
static unsigned stateWrites;
void Variable_RST(void) {assert(0);}
void Port_StateUpdate_Write(void) {++stateWrites;}
static unsigned Count(unsigned mask)
{unsigned n=0U;for(unsigned i=0U;i<8U;++i)n+=(mask>>i)&1U;return n;}
static void CheckLayout(const unsigned short *m)
{
    const unsigned slot[6]={Com_A1_PluginUnit_5-2,Com_A2_PluginUnit_6-2,
        Com_B1_PluginUnit_9-2,Com_B2_PluginUnit_10-2,Com_C1_PluginUnit_13-2,Com_C2_PluginUnit_14-2};
    for(unsigned i=0U;i<6U;++i)
    {
        assert(FPGA_Interface[slot[i]].PM==m[i]);
        assert(FPGA_Interface[slot[i]].TX_EN==m[i]);
        assert(FPGA_Interface[slot[i]].NUM==Count(m[i&~1U])+Count(m[i|1U]));
    }
    assert(FPGA_Interface[Putout_A_PluginUnit_7-2].SHUTDOWN == (ushort)~(m[0] | (m[1] << 8U)));
    assert(FPGA_Interface[Putout_B_PluginUnit_11-2].SHUTDOWN == (ushort)~(m[2] | (m[3] << 8U)));
    assert(FPGA_Interface[Putout_C_PluginUnit_15-2].SHUTDOWN == (ushort)~(m[4] | (m[5] << 8U)));
    assert(NHA_module_A==Count(m[0])+Count(m[1]));
    assert(NHA_module_B==Count(m[2])+Count(m[3]));
    assert(NHA_module_C==Count(m[4])+Count(m[5]));
    assert(HA_ReferenceDEF_A==NHA_module_A*ReferenceDEF);
    assert(HA_ReferenceDEF_B==NHA_module_B*ReferenceDEF);
    assert(HA_ReferenceDEF_C==NHA_module_C*ReferenceDEF);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[0]==m[0]);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[1]==m[1]);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET==m[2]);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET==m[3]*10U);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET==m[4]*10U);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET==m[5]*10U);
}
int main(void)
{
    const unsigned short defaults[6]={CHB_PM_A1_MASK,CHB_PM_A2_MASK,CHB_PM_B1_MASK,CHB_PM_B2_MASK,CHB_PM_C1_MASK,CHB_PM_C2_MASK};
    unsigned short words[16]={3U,0x7000U,0x0081U,0x0101U,0U,0xFFFFU,0U,0U,0U,0U,0U,0U,0xBU,3U,0U,0U};
    unsigned short readback[16];
    const unsigned short no_modules[6] = {0U, 0U, 0U, 0U, 0U, 0U};
    assert(offsetof(FPGA_InterfaceBLOCK, UNIT_LINK) == 0x01E2U);
    assert(offsetof(FPGA_InterfaceBLOCK, UNIT_LINK_LOSS) == 0x01E4U);
    FPGA_Interface[MainControl_PluginUnit_4-2].FB_EN = 0xFFFFU;
    FPGA_Interface[Input_A_PluginUnit_8-2].FO_EN = 0xFFFFU;
    FPGA_Interface[Input_B_PluginUnit_12-2].FO_EN = 0xFFFFU;
    FPGA_Interface[Input_C_PluginUnit_16-2].FO_EN = 0xFFFFU;
    FpgaInit_SetSafeOutputs();
    CheckLayout(no_modules);
    assert(FPGA_Interface[MainControl_PluginUnit_4-2].DSP_CTL.Bbits.SHUTDOWN == 1U);
    assert(FPGA_Interface[MainControl_PluginUnit_4-2].FB_EN == 0U);
    assert(FPGA_Interface[Input_A_PluginUnit_8-2].FO_EN == 0U);
    assert(FPGA_Interface[Input_B_PluginUnit_12-2].FO_EN == 0U);
    assert(FPGA_Interface[Input_C_PluginUnit_16-2].FO_EN == 0U);
    DpramDefaults_InitializeRegulation();
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16 == 0x3FFFU);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16 == 0x3FFFU);
    assert(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16 == 0x3FFFU);
    FpgaInit_ConfigureModuleLayout();
    CheckLayout(defaults);
    assert(Port_SetModuleFOArmed(7U) == 1U);
    /* Sparse masks must retain connector bit positions, never become 0..N-1. */
    for(unsigned i=0U;i<256U;++i)
    {
        words[6]=(unsigned short)i;words[7]=(unsigned short)((63U-i)&63U);
        words[8]=0x80U;words[9]=1U;words[10]=0xA5U;words[11]=0x22U;
        assert(SstHardwareParameters_Validate(words)==SST_PARAM_OK);
        SstHardwareParameters_Apply(words);
        CheckLayout(words+6);
        assert(FPGA_Interface[Analog_PluginUnit_3-2].AD_PROT_EN == 0x7000U);
        assert(FPGA_Interface[Input_A_PluginUnit_8-2].FO_EN == (i & 0x0081U));
        assert(FPGA_Interface[Input_B_PluginUnit_12-2].FO_EN == 0x0100U);
        assert(FPGA_Interface[Input_C_PluginUnit_16-2].FO_EN == 0U);
        assert(FPGA_Interface[MainControl_PluginUnit_4-2].FB_EN ==
               ((i & 0x0081U) != 0U ? 0xBFFFU : 0xBFBFU));
        SstHardwareParameters_Read(readback);
        assert(memcmp(words,readback,sizeof(words))==0);
    }
    assert(stateWrites==256U);
    assert(Port_SetModuleFOArmed(0U) == 1U);
    assert(FPGA_Interface[Input_A_PluginUnit_8-2].FO_EN == 0U);
    assert(FPGA_Interface[Input_B_PluginUnit_12-2].FO_EN == 0U);
    assert(FPGA_Interface[MainControl_PluginUnit_4-2].FB_EN == 0xBBBfU);
    puts("PASS: startup and 256 sparse PM/NUM/TX_EN/SHUTDOWN layouts, 701 FO and 403 FB masks, EEPROM round trip");
}
