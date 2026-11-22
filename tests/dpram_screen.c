#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstDpramProtocol.h"
#include "SstParameterTable.h"
#include "SstLegacyParameters.h"
#include "SstCommand.h"
#include "SstParameterStore.h"
#include "SstHardwareParameters.h"

#define VALIDFLAG 0x55U
static uint8_t eeprom[1024];
static uint16_t fail_write;
static unsigned eeprom_write_attempts; /* 包括失败调用，用于验证再次按保存确实重试EEPROM。 */
static uint16_t hardware_writes, manual_calls;
static uint16_t OC_level_SET=100, OC_time_SET=20, OC_level_SET_L2=120, OC_time_SET_L2=5;
static uint16_t OV_level_SET=300, OV_time_SET=5, HVAC_OC_Fault, HVAC_OV_Fault;
static float HA_L_Voltage_Precharge=10.0F, HA_H_Voltage=400.0F;
static uint16_t Port_AD_PORT_EN=0x7000, Port_FO_1_EN=0x007F, Port_FO_2_EN, Port_FO_3_EN, Port_FB_EN;
static uint16_t HA_PM_A1=7,HA_PM_A2=7,HA_PM_B1=7,HA_PM_B2=7,HA_PM_C1=7,HA_PM_C2=7;
static uint16_t Port_OnState=0xB,Port_Main_WorkMode=3,Port_STATCOM_WorkMode,Port_APF_WorkMode;
static void FpgaInit_ApplyModuleLayout(const uint16_t *m)
{HA_PM_A1=m[0];HA_PM_A2=m[1];HA_PM_B1=m[2];HA_PM_B2=m[3];HA_PM_C1=m[4];HA_PM_C2=m[5];++hardware_writes;}
static void Port_StateUpdate_Write(void) {}
static uint16_t fo_latch_a, fo_latch_b, fo_latch_c; /* 替代701的三组硬件故障锁存。 */
#define Input_A_PluginUnit_8 8U
#define Input_B_PluginUnit_12 12U
#define Input_C_PluginUnit_16 16U
static uint16_t Port_EffectiveFOEnable(uint16_t plugin)
{
    switch (plugin)
    {
        case Input_A_PluginUnit_8: return Port_FO_1_EN & (HA_PM_A1 | (HA_PM_A2 << 8));
        case Input_B_PluginUnit_12: return Port_FO_2_EN & (HA_PM_B1 | (HA_PM_B2 << 8));
        case Input_C_PluginUnit_16: return Port_FO_3_EN & (HA_PM_C1 | (HA_PM_C2 << 8));
        default: assert(0); return 0U;
    }
}
unsigned short Port_FOStatusRead(unsigned char plugin, unsigned char latch)
{
    assert(latch == 1U);
    switch (plugin)
    {
        case Input_A_PluginUnit_8: return fo_latch_a;
        case Input_B_PluginUnit_12: return fo_latch_b;
        case Input_C_PluginUnit_16: return fo_latch_c;
        default: assert(0); return 0U;
    }
}
static struct { struct { unsigned Port_RunOrTest, Port_AutoOrManual; } Bbits; } PortRunFLAG;
static struct { uint16_t PORT_FAULT16; } Port_RealFault;
static struct { uint16_t FaultTypeFLAG16; } FaultTypeFLAG;
static uint16_t Port_Outer_Elec=1, NHA_module_A=6, NHA_module_B=6, NHA_module_C=6;
static float SVG_Udc_A_Ave=60;
static struct {float PORT_P,PORT_Q,PORT_V_A,PORT_V_B,PORT_V_C,PORT_I_A,PORT_I_B,PORT_I_C,PORT_PF;} PORT_RealTime_DATA;
static void Port_ADProtect_EN(void) {++hardware_writes;}
static void Port_FO_EN_Set(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e) {(void)a;(void)b;(void)c;(void)d;(void)e;++hardware_writes;}
static void Port_FB_EN_Set(void) {++hardware_writes;}
static uint8_t r_eeprom_byte_read(uint8_t p,uint8_t i) {return eeprom[p*64U+i];}
static uint8_t r_eeprom_section_read(uint8_t p,uint8_t i,uint8_t n,uint8_t *v)
{memcpy(v,eeprom+p*64U+i,n);return VALIDFLAG;}
static uint8_t r_eeprom_byte_write(uint8_t p,uint8_t i,uint8_t v)
{
    ++eeprom_write_attempts;
    if (fail_write != 0U)
        return 0U;
    eeprom[p*64U+i]=v;
    return VALIDFLAG;
}
uint16_t SstParams_IsStopped(void) {return SstCommand_IsStopped();}
uint16_t SstParams_IsStorageBusy(void) {return SstParameterStore_IsBusy();}
unsigned short ChbDabControl_MaintenanceSwitch(unsigned short a,unsigned short b) {(void)a;(void)b;++manual_calls;return SST_PARAM_DENIED;}
unsigned short SstFault_GetFirstCode(void) {return 29103;}
unsigned short SstFault_GetLevel(void) {return 2;}
unsigned short SstFault_GetActiveCount(void) {return 1;}
unsigned short SstFault_GetHistoryCount(void) {return 1;}
unsigned short SstFault_GetCode(unsigned short i) {return i?0:29103;}
/* DEFAULTS */
/* IMPLEMENTATIONS */

#if SST_ENABLE_DPRAM
static void finish_publication(void) {unsigned i;for(i=0;i<20;++i)SstDpramScreen_ProcessTransactions();}
/* 屏幕只写1B01；测试全程不构造请求序号。 */
static void submit_operation(uint16_t operation)
{
    DPRAM_SstScreen[385] = operation;
    SstDpramScreen_ProcessTransactions();
}
#endif
#undef assert
#define assert(x) ((x)?(void)0:(fprintf(stderr,"ASSERT line %d: %s\n",__LINE__,#x),exit(2)))

#if SST_ENABLE_DPRAM && SST_ALLOW_REMOTE_COMMANDS
/* 验证真实解析与存储模块：无序号应用、重复保存、忙时按键丢弃及失败后重试。 */
static void test_simple_screen_operations(void)
{
    unsigned index; /* 用于推进后台10ms保存服务。 */
    unsigned attempts; /* 保存/重试前的EEPROM调用次数。 */
    uint16_t count; /* 操作前完成计数，验证后台轮询不会自行重复执行。 */

    assert(DPRAM_SstScreen[384] == 0U);
    assert(DPRAM_SstScreen[386] == 0U);
    assert(DPRAM_SstScreen[2] == 0x0102U);
    DPRAM_SstScreen[128U + 24U] = (uint16_t)(int16_t)-1500;
    finish_publication();
    assert(g_chb_dab_params.current_kp == -1.25F);
    submit_operation(SST_SCREEN_APPLY);
    assert(DPRAM_SstScreen[385] == 0U && DPRAM_SstScreen[388] == 0U);
    assert(DPRAM_SstScreen[386] == 1U && DPRAM_SstScreen[387] == SST_PARAM_OK);
    assert(g_chb_dab_params.current_kp == -1.5F);

    /* 再写相同操作号仍重新应用；旧屏幕残留在1B00的值也不会影响受理。 */
    DPRAM_SstScreen[384] = 65535U;
    DPRAM_SstScreen[128U + 24U] = (uint16_t)(int16_t)-1700;
    submit_operation(SST_SCREEN_APPLY);
    assert(g_chb_dab_params.current_kp == -1.7F && DPRAM_SstScreen[386] == 2U);
    DPRAM_SstScreen[384] = 0U;
    submit_operation(SST_SCREEN_REFRESH);
    assert(DPRAM_SstScreen[128U + 24U] == (uint16_t)(int16_t)-1700);
    assert(DPRAM_SstScreen[386] == 3U);

    attempts = eeprom_write_attempts;
    submit_operation(SST_SCREEN_SAVE);
    assert(DPRAM_SstScreen[385] == 0U && DPRAM_SstScreen[388] == 1U);
    assert(DPRAM_SstScreen[387] == SST_PARAM_BUSY && DPRAM_SstScreen[386] == 3U);
    /* 忙时重复SAVE及LOAD均清零丢弃，不改正在保存的操作和最终结果。 */
    submit_operation(SST_SCREEN_SAVE);
    submit_operation(SST_SCREEN_LOAD);
    assert(DPRAM_SstScreen[385] == 0U && DPRAM_SstScreen[388] == 1U);
    assert(DPRAM_SstScreen[386] == 3U && DPRAM_SstScreen[387] == SST_PARAM_BUSY);
    for (index = 0U; index < 300U; ++index)
    {
        ++g_chb_dab_status.tick_10ms;
        SstParameterStore_Service();
        SstDpramScreen_ProcessTransactions();
    }
    assert(eeprom_write_attempts - attempts == 141U);
    assert(DPRAM_SstScreen[386] == 4U && DPRAM_SstScreen[387] == SST_PARAM_OK);
    assert(DPRAM_SstScreen[388] == 0U && DPRAM_SstScreen[390] == SST_SCREEN_SAVE);
    count = DPRAM_SstScreen[386];
    finish_publication();
    assert(DPRAM_SstScreen[386] == count && eeprom_write_attempts - attempts == 141U);

    /* 空闲后再次单写2，必须产生第二次完整保存，而非回放上次成功。 */
    submit_operation(SST_SCREEN_SAVE);
    for (index = 0U; index < 300U; ++index)
    {
        ++g_chb_dab_status.tick_10ms;
        SstParameterStore_Service();
        SstDpramScreen_ProcessTransactions();
    }
    assert(eeprom_write_attempts - attempts == 282U && DPRAM_SstScreen[386] == 5U);
    g_chb_dab_params.current_kp = -2.0F;
    submit_operation(SST_SCREEN_LOAD);
    assert(g_chb_dab_params.current_kp == -1.7F && DPRAM_SstScreen[386] == 6U);
    assert(DPRAM_SstScreen[387] == SST_PARAM_OK && DPRAM_SstScreen[390] == SST_SCREEN_LOAD);

    fail_write = 1U;
    attempts = eeprom_write_attempts;
    for (index = 0U; index < 2U; ++index)
    {
        submit_operation(SST_SCREEN_SAVE);
        ++g_chb_dab_status.tick_10ms;
        SstParameterStore_Service();
        SstDpramScreen_ProcessTransactions();
        assert(DPRAM_SstScreen[387] == SST_PARAM_STORAGE && DPRAM_SstScreen[388] == 0U);
    }
    assert(eeprom_write_attempts - attempts == 2U && DPRAM_SstScreen[386] == 8U);
    fail_write = 0U;
    submit_operation(99U);
    assert(DPRAM_SstScreen[387] == SST_SCREEN_UNSUPPORTED && DPRAM_SstScreen[388] == 0U);
    assert(DPRAM_SstScreen[386] == 9U && DPRAM_SstScreen[390] == 99U);
    g_chb_dab_command.run_enable = 1U;
    submit_operation(SST_SCREEN_SAVE);
    assert(DPRAM_SstScreen[387] == SST_PARAM_BUSY && DPRAM_SstScreen[388] == 0U);
    assert(eeprom_write_attempts - attempts == 2U);
    g_chb_dab_command.run_enable = 0U;
}

/* 从真实16字解析入口验证：HVAC START不联动DAB，DAB只由脉冲bit0单独请求。 */
static void test_independent_dab_commands(void)
{
    uint16_t command[16] = {0U}; /* 本笔控制块，每次只发送本笔有效动作。 */

    g_chb_dab_command.run_enable = 0U;
    g_chb_dab_command.dab_enable = 0U;
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
    command[2] = 49409U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);

    memset(command, 0, sizeof(command));
    command[8] = 0xFFFDU;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);

    memset(command, 0, sizeof(command));
    command[2] = 1U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);

    memset(command, 0, sizeof(command));
    command[9] = 0xFFFDU;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    command[9] = 0U;
    command[2] = 1U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);

    memset(command, 0, sizeof(command));
    command[9] = 2U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    command[9] = 0U;
    command[8] = 1U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);

    memset(command, 0, sizeof(command));
    command[2] = 1U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    command[3] = 1U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
}

/* 验证有效位解析；使用现场启动原字，并保留真实不支持动作的拒绝行为。 */
static void test_control_masks(void)
{
    static const uint16_t protocol_masks[16] = {
        0x0000U, 0x0000U, 0x000FU, 0x000FU,
        0x3FFFU, 0x0000U, 0x000FU, 0x000FU,
        0x0003U, 0x0003U, 0x0001U, 0x0003U,
        0x000FU, 0x00FFU, 0x003FU, 0x0000U
    };
    uint16_t command[16]; /* 本笔16字原始控制块，含保留位噪声。 */
    uint16_t saved_permission; /* 测试前的开关许可，结束时恢复。 */
    uint16_t saved_manual_calls; /* 确认保留位不会误触发开关维护。 */
    unsigned int index; /* 控制块字号。 */

    saved_permission = g_sst_dpram_screen_result.switch_permission;
    saved_manual_calls = manual_calls;
    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;

    /* 复现现场：word0非零曾导致整包返回7，启动原字均仅bit0有效。 */
    memset(command, 0, sizeof(command));
    command[0] = 0xA55AU;
    command[2] = 2385U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    command[2] = 49409U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(g_sst_dpram_screen_result.error_word == 0xFFFFU);
    assert(g_sst_dpram_screen_result.ignored_control_words == 0U);
    assert(command[0] == 0xA55AU && command[2] == 49409U);

    /* 每一字的全部保留位均置1，不得成为STOP、ON、RESET或互斥条件。 */
    for (index = 0U; index < 16U; ++index)
    {
        command[index] = (uint16_t)~protocol_masks[index];
    }
    command[2] |= 1U;
    command[4] |= 1U;
    command[11] |= 1U;
    command[12] |= 1U;
    command[13] |= 0x0055U;
    command[14] |= 0x0025U;
    g_chb_dab_command.fault_reset = 0U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(!g_chb_dab_command.fault_reset && manual_calls == saved_manual_calls);
    assert(g_sst_dpram_screen_result.switch_permission == 1U);
    command[11] = 0xFFFEU;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_sst_dpram_screen_result.switch_permission);
    command[11] = 0xFFFFU;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_RANGE);

    /* 未接入功能只忽略并记字号，不能拦截同包正常HVAC START。 */
    for (index = 0U; index < 16U; ++index)
    {
        uint16_t unsupported; /* 当前字内协议有效、但本机尚未实现的动作位。 */
        unsupported = 0U;
        if (index == 2U || index == 3U || index == 4U || index == 12U)
        {
            unsupported = 2U;
        }
        else if (index == 6U || index == 7U)
        {
            unsupported = 4U;
        }
        if (unsupported == 0U)
        {
            continue;
        }
        memset(command, 0, sizeof(command));
        command[2] = 1U;
        command[index] |= unsupported;
        g_chb_dab_command.run_enable = 0U;
        g_chb_dab_command.dab_enable = 0U;
        assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
        assert(g_sst_dpram_screen_result.error_word == 0xFFFFU);
        assert(g_sst_dpram_screen_result.ignored_control_words == (1U << index));
        assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    }

    /* 多个未接入字段共存，启动仍生效，且不得误动作维护开关。 */
    memset(command, 0, sizeof(command));
    command[2] = 0x000FU;
    command[3] = 0x000EU;
    command[4] = 0x3FFFU;
    command[6] = 0x000CU;
    command[7] = 0x000CU;
    command[12] = 0x000FU;
    saved_manual_calls = manual_calls;
    g_chb_dab_command.run_enable = 0U;
    g_chb_dab_command.dab_enable = 0U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(manual_calls == saved_manual_calls);
    assert(g_sst_dpram_screen_result.ignored_control_words == 0x10DCU);

    /* 只有其他端口START时，本机不启动；忽略并不表示把它路由到HVAC。 */
    memset(command, 0, sizeof(command));
    command[2] = 2U;
    g_chb_dab_command.run_enable = 0U;
    g_chb_dab_command.dab_enable = 0U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(g_sst_dpram_screen_result.ignored_control_words == (1U << 2U));

    /* 掩码处理不能把高位噪声当作启动/复位，也不能绕过故障联锁。 */
    memset(command, 0, sizeof(command));
    command[2] = 0xC100U;
    command[8] = 0xFFFCU;
    command[10] = 0xFFFEU;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(!g_chb_dab_command.fault_reset);
    command[2] = 49409U;
    g_chb_dab_status.state = CHB_STATE_FAULT;
    g_chb_dab_status.fault_code = 3U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_DENIED);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);

    /* STOP/OFF仍优先于启动、其他不支持功能和故障状态。 */
    command[3] = 0xFFF1U;
    command[4] = 2U;
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    command[3] = 0U;
    command[9] = 0xFFFFU;
    g_chb_dab_command.run_enable = g_chb_dab_command.dab_enable = 1U;
    assert(SstDpramCommand_ProcessControlBlock(command) == SST_PARAM_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);

    g_chb_dab_status.state = CHB_STATE_STOPPED;
    g_chb_dab_status.fault_code = CHB_FAULT_NONE;
    g_sst_dpram_screen_result.switch_permission = saved_permission;
}

/* 验证原43字遥调块：保留位/保留字不影响参数，有效数值仍整组校验。 */
static void test_legacy_parameter_masks(void)
{
    uint16_t command[43]; /* 待提交的原遥调块。 */
    uint16_t before[SST_SCREEN_PARAMETERS]; /* 提交前实际参数编码。 */
    uint16_t after[SST_SCREEN_PARAMETERS]; /* 提交后实际参数编码。 */
    uint16_t saved_hardware_writes; /* 保留内容不能触发多余的FPGA使能写入。 */
    float saved_current_kp; /* 未改PI必须保持原浮点精度。 */

    SstDpramParameter_EncodeParameterSnapshot(before);
    SstDpramParameter_EncodeLegacyBlock(command);
    saved_hardware_writes = hardware_writes;
    saved_current_kp = g_chb_dab_params.current_kp;
    command[0] |= 0xFF00U;
    command[1] |= 0x0FFFU;
    command[2] |= 0x8000U;
    command[3] |= 0x8000U;
    command[4] |= 0x8000U;
    command[23] = 0xFFFFU;
    command[24] = 0x1234U;
    command[25] = 0xA55AU;
    command[26] = 0x8000U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_OK);
    SstDpramParameter_EncodeParameterSnapshot(after);
    assert(memcmp(before, after, sizeof(before)) == 0);
    assert(g_chb_dab_params.current_kp == saved_current_kp);
    assert(hardware_writes == saved_hardware_writes);
    assert(command[1] & 0x0FFFU);
    assert(g_sst_dpram_screen_result.error_word == 0xFFFFU);

    /* 有效AD/FO/FB位依然完整写入，回读只返回有效位。 */
    command[1] ^= 0x1000U;
    command[2] ^= 0x0001U;
    command[5] ^= 0x8000U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_OK);
    assert(Port_AD_PORT_EN == (command[1] & 0xF000U));
    assert(Port_FO_1_EN == (command[2] & 0x7FFFU));
    assert(Port_FB_EN == command[5]);
    assert(hardware_writes == saved_hardware_writes + 3U);
    SstDpramParameter_EncodeLegacyBlock(command);
    assert(!(command[0] & 0xFF00U) && !(command[1] & 0x0FFFU));
    assert(!(command[2] & 0x8000U) && command[23] == 0U && command[26] == 0U);
    command[1] ^= 0x1000U;
    command[2] ^= 0x0001U;
    command[5] ^= 0x8000U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_OK);

    /* 系统方式低8位是协议有效位，其他端口仍不可冒充HVAC成功。 */
    SstDpramParameter_EncodeLegacyBlock(command);
    command[0] |= 0xFF04U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_SCREEN_UNSUPPORTED);
    assert(g_sst_dpram_screen_result.error_word == 0U);
    SstDpramParameter_EncodeParameterSnapshot(after);
    assert(memcmp(before, after, sizeof(before)) == 0);

    /* 数值字段不按bit裁剪；非法Udc与未实现PI必须继续整块拒绝。 */
    SstDpramParameter_EncodeLegacyBlock(command);
    command[8] = 0x803CU;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_RANGE);
    SstDpramParameter_EncodeLegacyBlock(command);
    command[8] += 1U;
    command[31] = 1U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_SCREEN_UNSUPPORTED);
    SstDpramParameter_EncodeParameterSnapshot(after);
    assert(memcmp(before, after, sizeof(before)) == 0);
    assert(g_chb_dab_params.current_kp == saved_current_kp);
}

/* 从原43字入口验证直接赋值：独立于扩展参数校验，且保留未改项的float精度。 */
static void test_legacy_parameter_direct_assignment(void)
{
    CHB_DAB_PARAMETERS saved_parameters = g_chb_dab_params; /* 测试后恢复全部控制参数。 */
    CHB_DAB_COMMAND saved_command = g_chb_dab_command; /* 测试后恢复全部命令/给定。 */
    uint16_t command[43]; /* 本笔原协议遥调块，不涉及57项扩展区。 */
    uint16_t readback[43]; /* DSP实际值回读，用于验证无半笔应用及负数编码。 */
    uint16_t saved_ad_enable = Port_AD_PORT_EN; /* 无效请求不得先改FPGA保护使能。 */
    float saved_precharge_low = HA_L_Voltage_Precharge; /* 无关的原保护参数也不能拦截本块。 */

    g_chb_dab_command.dc_voltage_ref_v = 60.25F;
    g_chb_dab_command.open_loop_frequency_hz = 50.037F;
    g_chb_dab_params.pll_nominal_frequency_hz = 49.873F;
    g_chb_dab_params.voltage_kp = -0.001234567F;
    g_chb_dab_params.voltage_ki = 2.34567F;
    g_chb_dab_params.current_kp = -1.249166667F;
    g_chb_dab_params.current_ki = -16.666667F;

    /* 模拟仿真器正在整定无关参数：原接口不能因为未提交的DAB/巡检组合拒绝电压遥调。 */
    g_chb_dab_params.dab_start_duty_count = 2000U;
    g_chb_dab_params.dab_target_duty_count = 1000U;
    HA_L_Voltage_Precharge = HA_H_Voltage;
    SstDpramParameter_EncodeLegacyBlock(command);
    command[8] = 61U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.dc_voltage_ref_v == 61.0F);
    assert(g_chb_dab_params.voltage_kp == -0.001234567F);
    assert(g_chb_dab_params.voltage_ki == 2.34567F);
    assert(g_chb_dab_params.current_kp == -1.249166667F);
    assert(g_chb_dab_params.current_ki == -16.666667F);
    assert(g_chb_dab_command.open_loop_frequency_hz == 50.037F);
    assert(g_chb_dab_params.pll_nominal_frequency_hz == 49.873F);

    /* 频率直接换算为Hz：开环写输出频率，闭环写PLL额定频率，两者不互相覆盖。 */
    g_chb_dab_command.ac_mode = CHB_AC_MODE_OPEN_LOOP;
    SstDpramParameter_EncodeLegacyBlock(command);
    command[9] = 550U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_command.open_loop_frequency_hz == 55.0F);
    assert(g_chb_dab_params.pll_nominal_frequency_hz == 49.873F);
    g_chb_dab_command.ac_mode = CHB_AC_MODE_CLOSED_LOOP;
    SstDpramParameter_EncodeLegacyBlock(command);
    command[9] = 600U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_OK);
    assert(g_chb_dab_params.pll_nominal_frequency_hz == 60.0F);
    assert(g_chb_dab_command.open_loop_frequency_hz == 55.0F);

    /* 四个PI覆盖int16两端和负增益，回读必须保留原二补码。 */
    SstDpramParameter_EncodeLegacyBlock(command);
    command[27] = 0x8000U;
    command[28] = 0x7FFFU;
    command[29] = 0xFB1EU;
    command[30] = 0xC180U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_OK);
    assert(fabsf(g_chb_dab_params.voltage_kp + 0.32768F) < 0.000001F);
    assert(fabsf(g_chb_dab_params.voltage_ki - 327.67F) < 0.0001F);
    assert(g_chb_dab_params.current_kp == -1.25F && g_chb_dab_params.current_ki == -16.0F);
    SstDpramParameter_EncodeLegacyBlock(readback);
    assert(memcmp(command, readback, sizeof(command)) == 0);

    /* 无效频率不能先应用同包的电压/PI/保护使能；原值必须完整保留。 */
    command[1] ^= 0x1000U;
    command[8] = 62U;
    command[9] = 9U;
    command[29] = 1000U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_RANGE);
    assert(g_sst_dpram_screen_result.error_word == 9U);
    assert(Port_AD_PORT_EN == saved_ad_enable);
    SstDpramParameter_EncodeLegacyBlock(command);
    assert(memcmp(command, readback, sizeof(command)) == 0);
    command[8] = 0U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_RANGE);
    assert(g_sst_dpram_screen_result.error_word == 8U);

    /* 原遥调也必须遵守运行互锁，不能在10kHz控制正在用参数时改写PI。 */
    SstDpramParameter_EncodeLegacyBlock(command);
    g_chb_dab_command.run_enable = 1U;
    assert(SstDpramParameter_DecodeAndApplyLegacyBlock(command) == SST_PARAM_BUSY);

    HA_L_Voltage_Precharge = saved_precharge_low;
    g_chb_dab_params = saved_parameters;
    g_chb_dab_command = saved_command;
}
#endif

int main(void) {
 uint16_t w[57], old[43], block[16]={0}, telemetry[33];
 float kp,ki; unsigned i;
 /* Exhaustive signed encoding: both signs, zero and int16 endpoints. */
 for(i=0;i<65536U;++i) {
  assert(SstParameterTable_WriteParameter(24,(uint16_t)i)==0);
  assert(SstParameterTable_ReadParameter(24)==(uint16_t)i);
 }
 g_chb_dab_params.current_kp=-1.249166667F;
 g_chb_dab_status.state=CHB_STATE_STOPPED;
 g_chb_dab_command.run_enable=g_chb_dab_command.dab_enable=0;
 SstDpramParameter_EncodeParameterSnapshot(w);
 assert(SstParameterTable_SnapshotValidate(w)==0);
 kp=g_chb_dab_params.current_kp;ki=g_chb_dab_params.current_ki;
#if SST_ENABLE_DPRAM && SST_ALLOW_REMOTE_COMMANDS
 test_independent_dab_commands();
 test_control_masks();
 test_legacy_parameter_masks();
 test_legacy_parameter_direct_assignment();
 /* A single Udc write must not quantize the untouched current PI. */
 SstDpramParameter_EncodeLegacyBlock(old);old[8]=60;
 assert(SstDpramParameter_DecodeAndApplyLegacyBlock(old)==0);
 assert(g_chb_dab_command.dc_voltage_ref_v==60);
 assert(g_chb_dab_params.current_kp==kp && g_chb_dab_params.current_ki==ki);
 old[29]=(uint16_t)(int16_t)-1250;old[30]=(uint16_t)(int16_t)-16000;
 assert(SstDpramParameter_DecodeAndApplyLegacyBlock(old)==0);
 assert(g_chb_dab_params.current_kp==-1.25F && g_chb_dab_params.current_ki==-16.0F);
 SstDpramParameter_EncodeLegacyBlock(old);assert(old[29]==0xFB1E && old[30]==0xC180);
 /* Reject an unsupported PI group as a whole; Udc must stay 60. */
 old[8]=65;old[31]=10;assert(SstDpramParameter_DecodeAndApplyLegacyBlock(old)==SST_SCREEN_UNSUPPORTED);
 assert(g_chb_dab_command.dc_voltage_ref_v==60);
 SstDpramParameter_EncodeParameterSnapshot(w);w[40]=650;w[48]=w[49];
 assert(SstDpramParameter_ApplyParameterSnapshot(w)==SST_PARAM_RANGE);assert(g_chb_dab_command.dc_voltage_ref_v==60);
 SstDpramParameter_EncodeParameterSnapshot(w);w[33]=2500;w[34]=100;
 assert(SstDpramParameter_ApplyParameterSnapshot(w)==SST_PARAM_RANGE);
 SstDpramParameter_EncodeParameterSnapshot(w);w[52]=0xF001;assert(SstDpramParameter_ApplyParameterSnapshot(w)==SST_PARAM_RANGE);
 g_chb_dab_command.run_enable=1;assert(SstDpramParameter_ApplyParameterSnapshot(w)==SST_PARAM_BUSY);
 block[2]=block[3]=1;assert(SstDpramCommand_ProcessControlBlock(block)==0);assert(!g_chb_dab_command.run_enable);
 memset(block,0,sizeof(block));block[2]=1;assert(SstDpramCommand_ProcessControlBlock(block)==0);
 assert(g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
 block[2]=0;block[3]=1;SstDpramCommand_ProcessControlBlock(block);
 g_chb_dab_status.state=CHB_STATE_FAULT;g_chb_dab_status.fault_code=3;
 memset(block,0,sizeof(block));block[2]=1;assert(SstDpramCommand_ProcessControlBlock(block)==SST_PARAM_DENIED);
 g_chb_dab_status.state=0;g_chb_dab_status.fault_code=0;
 memset(block,0,sizeof(block));block[4]=2;assert(SstDpramCommand_ProcessControlBlock(block)==SST_PARAM_OK);
 assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
 assert(g_sst_dpram_screen_result.ignored_control_words==0x0010U);
 SstDpramScreen_Initialize();finish_publication();assert(!(DPRAM_SstScreen[0]&1));
 assert(DPRAM_SstScreen[34]==0xC204U && DPRAM_SstScreen[38]==1U);
 assert(DPRAM_SstScreen[37]==0U);
 assert(SstDpramCommand_ProcessControlBlock(block)==SST_PARAM_OK);
 SstDpramScreen_PublishStatus();finish_publication();assert(DPRAM_SstScreen[37]==0x0010U);
 test_simple_screen_operations();
#else
 assert(SstDpramParameter_ApplyParameterSnapshot(w)==SST_PARAM_DENIED);
 SstDpramParameter_EncodeLegacyBlock(old);
 old[29] = 1000U;
 assert(SstDpramParameter_DecodeAndApplyLegacyBlock(old)==SST_PARAM_DENIED);
 assert(SstDpramCommand_ProcessControlBlock(block)==SST_PARAM_DENIED);
 assert(g_chb_dab_params.current_kp==kp && g_chb_dab_params.current_ki==ki);
#if SST_ENABLE_DPRAM
 SstDpramScreen_Initialize();
 submit_operation(SST_SCREEN_SAVE);
 assert(DPRAM_SstScreen[385]==0U && DPRAM_SstScreen[388]==0U);
 assert(DPRAM_SstScreen[386]==1U && DPRAM_SstScreen[387]==SST_PARAM_DENIED);
 assert(eeprom_write_attempts==0U);
#endif
 (void)old;(void)i;
#endif
 /* Internal software OV/OC must never appear as COMM/SYNC. */
 Port_RealFault.PORT_FAULT16=0x2000;assert(SstDpramTelemetry_GetFaultWord()==0x9000);
 Port_RealFault.PORT_FAULT16=0x4000;assert(SstDpramTelemetry_GetFaultWord()==0x8400);
 Port_RealFault.PORT_FAULT16=0;FaultTypeFLAG.FaultTypeFLAG16=0x1800;
 assert(SstDpramTelemetry_GetFaultWord()==0xE000);
 FaultTypeFLAG.FaultTypeFLAG16=0;g_chb_dab_status.fault_code=CHB_FAULT_BAD_CONFIGURATION;
 assert(SstDpramTelemetry_GetFaultWord()==0x8000);
 g_chb_dab_status.fault_code=0;PortRunFLAG.Bbits.Port_RunOrTest=0;PortRunFLAG.Bbits.Port_AutoOrManual=0;
 g_chb_dab_status.ac_pwm_enabled=1;g_chb_dab_status.dab_pwm_enabled=0;
 SstDpramTelemetry_BuildLegacyBlock(telemetry);
 assert((telemetry[1]&0xE000)==0x6000);assert((telemetry[1]&0x0300)==0x0200);
#if SST_ENABLE_SWITCH_FEEDBACK
 /* 0501 bit2 comes from the independent DI8 contact, never from NOT DI7. */
 g_chb_dab_status.qf1_closed=0;g_chb_dab_status.qf1_open=0;g_chb_dab_status.kt1_closed=0;
 SstDpramTelemetry_BuildLegacyBlock(telemetry);assert((telemetry[1]&7U)==0U);
 g_chb_dab_status.qf1_open=1;
 SstDpramTelemetry_BuildLegacyBlock(telemetry);assert((telemetry[1]&7U)==4U);
 g_chb_dab_status.qf1_open=0;g_chb_dab_status.qf1_closed=1;g_chb_dab_status.kt1_closed=1;
 SstDpramTelemetry_BuildLegacyBlock(telemetry);assert((telemetry[1]&7U)==3U);
#endif
 PORT_RealTime_DATA.PORT_I_A=100000;SstDpramTelemetry_BuildLegacyBlock(telemetry);assert(telemetry[8]==32767);
 /* 原三个保留字按701物理通道逐位上报；软件总故障字不承担模块位置编码。 */
 HA_PM_A1=0x0005U;HA_PM_B2=0x0001U;HA_PM_C2=0x0040U;
 Port_FO_1_EN=0x001FU;Port_FO_2_EN=0x0100U;Port_FO_3_EN=0x4000U;
 fo_latch_a=0x0025U;fo_latch_b=0x0180U;fo_latch_c=0x4000U;
 SstDpramTelemetry_BuildLegacyBlock(telemetry);
 assert(telemetry[30]==0x0005U && telemetry[31]==0x0100U && telemetry[32]==0x4000U);
 fo_latch_a=fo_latch_b=fo_latch_c=0U;
 SstDpramTelemetry_BuildLegacyBlock(telemetry);
 assert(telemetry[30]==0U && telemetry[31]==0U && telemetry[32]==0U);
 puts("PASS: command masks/ignored fields/2385/49409, independent DAB, signed PI, no-sequence screen operations, save retry/busy handling, stop/fault priority and Word bit packing");
 return 0;
}
