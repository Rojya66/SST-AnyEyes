#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstDpramProtocol.h"
#include "ProtectionTypes.h"
#include "PortStatusTypes.h"

/* REAL_SLOT_NUMBERS */

/* 外围替身：寄存器内容由测试给定；读写访问器、故障解析及遥测均使用真实函数。 */
static struct
{
    ushort FO_EN, FO, FO_LATCH; /* 701使能、实时输入、硬件锁存。 */
    ushort FB_EN, FB, FB_LATCH; /* 主控故障使能、实时输入、硬件锁存。 */
    ushort AD_PROT, AD_PROT_LATCH; /* 采样插件硬件保护状态。 */
} FPGA_Interface[16];
static XXG_PORT_FAULT Port_RealFault;
static XXG_FaultTypeFLAG FaultTypeFLAG;
static XXG_FaultDisposeFLAG FaultDisposeFLAG;
static XXG_ControlFLAG ControlFLAG;
static XXG_PortRunFLAG PortRunFLAG;
static ushort Port_AD_PORT_EN, Port_FB_EN, Port_FO_1_EN, Port_FO_2_EN, Port_FO_3_EN;
static ushort HA_PM_A1 = 0x0005U, HA_PM_A2, HA_PM_B1, HA_PM_B2 = 1U, HA_PM_C1, HA_PM_C2 = 0x0040U;
static ushort FBLatch_TAD, PortLatch_TAD, FOLatch_A_TAD, FOLatch_B_TAD, FOLatch_C_TAD, FOLatch_INV_TAD;
static ushort ModuleFault_DisposeTimes, ModuleIndex_PWMPulseDisable, PORT_Index = 1U;
ushort Port_Outer_Elec, NHA_module_A = 6U, NHA_module_B = 6U, NHA_module_C = 6U;
static ushort HVAC_OC_Fault, HVAC_OV_Fault;
static ushort pwm_disable_calls; /* 验证原故障入口仍先封波。 */
static float SVG_Udc_A_Ave;
static struct
{
    float PORT_P, PORT_Q, PORT_V_A, PORT_V_B, PORT_V_C, PORT_I_A, PORT_I_B, PORT_I_C, PORT_PF;
} PORT_RealTime_DATA;
volatile CHB_DAB_COMMAND g_chb_dab_command;
volatile CHB_DAB_PARAMETERS g_chb_dab_params;
volatile CHB_DAB_STATUS g_chb_dab_status;
SST_DPRAM_SCREEN_RESULT g_sst_dpram_screen_result;

static void PWMPluse_Disable(uchar index) { assert(index == 0U); ++pwm_disable_calls; }
static void Port_BKU_Send(uchar enabled) { assert(enabled == 1U); }
static void FaultLatch_Clear(uchar stage, uchar flags) { (void)stage; (void)flags; }

/* PRODUCTION_ACCESSORS */
/* PRODUCTION_FAULT_STEP */
/* PRODUCTION_TELEMETRY */

int main(void)
{
    uint16_t words[33]; /* 原33字遥测，最后三字保留物理输入编号。 */
    unsigned int channel; /* 逐位验证701的16路位置不被压缩或符号转换。 */

    /* 只有Port_*_EN对象，没有旧PORT_Regulate_DATA副本，确保真实故障函数不再依赖它。 */
    Port_FB_EN = 0x4441U;
    Port_AD_PORT_EN = 0x7000U;
    Port_FO_1_EN = 0x0025U;
    Port_FO_2_EN = 0x0180U;
    Port_FO_3_EN = 0x4000U;
    Port_FB_EN_Set();
    Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
    assert(FPGA_Interface[MainControl_PluginUnit_4 - 2].FB_EN == 0x4441U);
    assert(FPGA_Interface[Input_A_PluginUnit_8 - 2].FO_EN == 0x0005U);
    assert(FPGA_Interface[Input_B_PluginUnit_12 - 2].FO_EN == 0x0100U);
    assert(FPGA_Interface[Input_C_PluginUnit_16 - 2].FO_EN == 0x4000U);

    /* U2与三相701同时上报；故障分类必须使用与硬件写入相同的当前使能量。 */
    FPGA_Interface[MainControl_PluginUnit_4 - 2].FB_LATCH = 0x4441U;
    FPGA_Interface[Analog_PluginUnit_3 - 2].AD_PROT_LATCH = 0x1000U;
    FPGA_Interface[Input_A_PluginUnit_8 - 2].FO_LATCH = 0x0025U;
    FPGA_Interface[Input_B_PluginUnit_12 - 2].FO_LATCH = 0x0180U;
    FPGA_Interface[Input_C_PluginUnit_16 - 2].FO_LATCH = 0x4000U;
    FaultCheckAndDispose_StepOne();
    assert(pwm_disable_calls == 1U);
    assert(FBLatch_TAD == 0x4441U && PortLatch_TAD == 1U);
    assert(FaultTypeFLAG.Bbits.Port_OverCurrent == 1U && FaultTypeFLAG.Bbits.Module_Fault == 1U);
    assert(FOLatch_A_TAD == 0x0005U && FOLatch_B_TAD == 0x0100U && FOLatch_C_TAD == 0x4000U);
    SstDpramTelemetry_BuildLegacyBlock(words);
    assert(words[30] == 0x0005U && words[31] == 0x0100U && words[32] == 0x4000U);
    assert((words[2] & 0x8440U) == 0x8440U);

    /* 模拟遥调改使能后再次故障：屏蔽的插件/通道不能继续进入旧软件故障分类。 */
    Port_FB_EN = 0x0040U;
    Port_FO_1_EN = 0x0004U;
    Port_FB_EN_Set();
    Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
    FaultTypeFLAG.FaultTypeFLAG16 = 0U;
    FaultCheckAndDispose_StepOne();
    assert(FBLatch_TAD == 0x0040U && PortLatch_TAD == 0U);
    assert(FaultTypeFLAG.Bbits.Port_OverCurrent == 0U);
    assert(FOLatch_A_TAD == 0x0004U && FOLatch_B_TAD == 0U && FOLatch_C_TAD == 0U);
    Port_FB_EN = 0x0001U;
    Port_AD_PORT_EN = 0x4000U;
    FaultTypeFLAG.FaultTypeFLAG16 = 0U;
    FaultCheckAndDispose_StepOne();
    assert(PortLatch_TAD == 1U && FaultTypeFLAG.Bbits.Port_OverCurrent == 0U);

    /* 位置诊断独立于FB保护门控、停机状态及旧RAM锁存，避免保护分类未扫描时漏报。 */
    Port_FB_EN = 0U;
    ControlFLAG.Bbits.Port_Stop = 1U;
    FOLatch_A_TAD = FOLatch_B_TAD = FOLatch_C_TAD = 0xFFFFU;
    SstDpramTelemetry_BuildLegacyBlock(words);
    assert(words[30] == 0x0004U && words[31] == 0x0100U && words[32] == 0x4000U);

    /* 撤掉整个 B 相 PM 后，残留的 FO 位置和锁存 FB 均不用于当前模块诊断。 */
    HA_PM_B2 = 0U;
    Port_FB_EN = 0x0400U;
    Port_FB_EN_Set();
    assert(FPGA_Interface[MainControl_PluginUnit_4 - 2].FB_EN == 0U);
    SstDpramTelemetry_BuildLegacyBlock(words);
    assert(words[31] == 0U);

    /* 硬件复位已清701锁存时，旧RAM中残留的模块位不得继续发给屏幕。 */
    FPGA_Interface[Input_A_PluginUnit_8 - 2].FO_LATCH = 0U;
    FPGA_Interface[Input_B_PluginUnit_12 - 2].FO_LATCH = 0U;
    FPGA_Interface[Input_C_PluginUnit_16 - 2].FO_LATCH = 0U;
    SstDpramTelemetry_BuildLegacyBlock(words);
    assert(words[30] == 0U && words[31] == 0U && words[32] == 0U);
    assert(FOLatch_A_TAD == 0xFFFFU);

    /* 包括bit15在内，遥测采用uint16位图；原遥调掩码仍按Word只开放低15位。 */
    Port_FO_1_EN = 0xFFFFU;
    HA_PM_A1 = HA_PM_A2 = 0x00FFU;
    for (channel = 0U; channel < 16U; ++channel)
    {
        FPGA_Interface[Input_A_PluginUnit_8 - 2].FO_LATCH = (ushort)(1U << channel);
        SstDpramTelemetry_BuildLegacyBlock(words);
        assert(words[30] == (uint16_t)(1U << channel));
    }
    Port_FO_1_EN = 0U;
    SstDpramTelemetry_BuildLegacyBlock(words);
    assert(words[30] == 0U);
    puts("PASS: current AD/FO/FB masks, U2 protection, three phase physical positions, stopped diagnosis, hardware reset and uint16 bits");
    return 0;
}
