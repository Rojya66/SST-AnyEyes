#include "SstDpramHost.h"
#include "ChbBuildConfig.h"
#include "SstDpramV2.h"

#if SST_ENABLE_DPRAM
#include "ChbDabControl.h"
#include "ControlData.h"
#include "DPRAM.h"
#include "DpramHostData.h"
#include "FpgaRegister.h"
#include "PortStatusData.h"
#include "SstBoardConfig.h"
#include "SstCommand.h"
#include "SstDpramV2Parameter.h"
#include "SstDpramV2Telemetry.h"
#include "SstParameterStore.h"
#include "hw_types.h"

/* 最后两字是硬件邮箱，不放进普通数据区。 */
#pragma DATA_SECTION(DPRAM_V2_Words, ".DPRAM_V2")
static volatile uint16_t DPRAM_V2_Words[0x1FFEU];
#define DPRAM_WORD(addr) DPRAM_V2_Words[(addr)]
#define SST_DPRAM_V2_UNSUPPORTED 7U

enum
{
    CONTROL_CHB_START = 1, CONTROL_CHB_STOP,
    CONTROL_DAB_START, CONTROL_DAB_STOP,
    CONTROL_STOP_ALL, CONTROL_RESET_FAULT,
    CONTROL_QF1_CLOSE, CONTROL_QF1_OPEN,
    CONTROL_KT1_CLOSE, CONTROL_KT1_OPEN,
    CONTROL_PERMIT_CLOSE, CONTROL_INHIBIT_CLOSE,
    CONTROL_SET_OBSERVATION
};

enum
{
    PARAMETER_APPLY = 1, PARAMETER_SAVE, PARAMETER_LOAD, PARAMETER_READ
};

static uint16_t s_observation[3];
static uint16_t s_switch_permission;
static uint16_t s_control_count;
static uint16_t s_parameter_count;
static uint16_t s_save_pending;

void SstDpramV2_WriteWord(uint16_t addr, uint16_t value)
{
    DPRAM_WORD(addr) = value;
}

uint16_t SstDpramV2_ObservationMask(uint16_t phase)
{
    return s_observation[phase];
}

uint16_t SstDpramV2_SwitchPermission(void)
{
    return s_switch_permission;
}

static uint16_t RequestManualSwitch(uint16_t close, uint16_t open)
{
    if (PortRunFLAG.Bbits.Port_AutoOrManual != 0U ||
        (close != 0U && s_switch_permission == 0U))
        return SST_PARAM_DENIED;
    return ChbDabControl_MaintenanceSwitch(close, open);
}

static uint16_t SetObservation(const uint16_t *mask)
{
    uint16_t phase;
    for (phase = 0U; phase < 3U; ++phase)
    {
        if ((mask[phase] & (uint16_t)~SST_DPRAM_V2_PHASE_MASK) != 0U)
            return SST_PARAM_RANGE;
    }
    for (phase = 0U; phase < 3U; ++phase)
        s_observation[phase] = mask[phase];
    return SST_PARAM_OK;
}

static uint16_t ApplyControl(const uint16_t *words)
{
    const uint16_t op = words[0];

    /* 观察选择只影响读数。停机在保存 EEPROM 期间也可受理。 */
    if (op == CONTROL_SET_OBSERVATION)
        return SetObservation(&words[1]);
    if (SST_ALLOW_REMOTE_COMMANDS == 0U)
        return SST_PARAM_DENIED;

    switch (op)
    {
        case CONTROL_STOP_ALL:
            SstCommand_EmergencyStop();
            (void)ChbDabControl_MaintenanceSwitch(0U, 3U);
            return SST_PARAM_OK;
        case CONTROL_CHB_STOP:
        case CONTROL_DAB_STOP:
            SstControlCommand_ApplyRequests(0U, 0U, 0U, op == CONTROL_CHB_STOP ? 2U : 1U, 0U);
            return SST_PARAM_OK;
        case CONTROL_QF1_OPEN:
            return RequestManualSwitch(0U, 2U);
        case CONTROL_KT1_OPEN:
            return RequestManualSwitch(0U, 1U);
        case CONTROL_INHIBIT_CLOSE:
            s_switch_permission = 0U;
            return SST_PARAM_OK;
        default:
            break;
    }

    if (SstParameterStore_IsBusy() != 0U)
        return SST_PARAM_BUSY;

    switch (op)
    {
        case CONTROL_CHB_START:
        case CONTROL_DAB_START:
            if (g_chb_dab_status.state == CHB_STATE_FAULT || g_chb_dab_status.fault_code != CHB_FAULT_NONE)
                return SST_PARAM_DENIED;
            SstControlCommand_ApplyRequests(op == CONTROL_CHB_START, 0U, op == CONTROL_DAB_START, 0U, 0U);
            return SST_PARAM_OK;
        case CONTROL_RESET_FAULT:
            if (SstCommand_IsStopped() == 0U)
                return SST_PARAM_BUSY;
            SstControlCommand_ApplyRequests(0U, 0U, 0U, 0U, 1U);
            return SST_PARAM_OK;
        case CONTROL_QF1_CLOSE:
            return RequestManualSwitch(2U, 0U);
        case CONTROL_KT1_CLOSE:
            return RequestManualSwitch(1U, 0U);
        case CONTROL_PERMIT_CLOSE:
            s_switch_permission = 1U;
            return SST_PARAM_OK;
        default:
            return SST_DPRAM_V2_UNSUPPORTED;
    }
}

static void ProcessControl(void)
{
    uint16_t words[SST_DPRAM_V2_CONTROL_WORDS];
    uint16_t i;
    for (i = 0U; i < SST_DPRAM_V2_CONTROL_WORDS; ++i)
        words[i] = DPRAM_WORD(SST_DPRAM_V2_CONTROL + i);
    DPRAM_WORD(SST_DPRAM_V2_CONTROL) = 0U;
    DPRAM_WORD(SST_DPRAM_V2_CONTROL_OP) = words[0];
    DPRAM_WORD(SST_DPRAM_V2_CONTROL_RESULT) = ApplyControl(words);
    DPRAM_WORD(SST_DPRAM_V2_CONTROL_COUNT) = ++s_control_count;
}

static void PublishParameters(void)
{
    uint16_t words[SST_DPRAM_V2_PARAMETER_WORDS];
    uint16_t i;
    SstDpramV2Parameter_Read(words);
    for (i = 0U; i < SST_DPRAM_V2_PARAMETER_WORDS; ++i)
        DPRAM_WORD(SST_DPRAM_V2_PARAMETER_BASE + i) = words[i];
}

static void CompleteParameter(uint16_t op, uint16_t result, uint16_t error_addr)
{
    PublishParameters();
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_LAST) = op;
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_RESULT) = result;
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_ERROR) = error_addr;
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_COUNT) = ++s_parameter_count;
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_BUSY) = 0U;
}

static uint16_t ProcessParameter(void)
{
    const uint16_t op = DPRAM_WORD(SST_DPRAM_V2_PARAMETER_OP);
    uint16_t result;
    uint16_t error_addr = SST_DPRAM_V2_NO_ERROR_ADDR;

    /* 左邮箱没有队列。等待保存的屏幕不能提交第二笔遥调。 */
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_OP) = 0U;
    if (s_save_pending != 0U)
        return 0U;
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_BUSY) = 1U;
    if (op == PARAMETER_READ)
        result = SST_PARAM_OK;
    else if (SST_ALLOW_REMOTE_COMMANDS == 0U)
        result = SST_PARAM_DENIED;
    else if (op == PARAMETER_APPLY)
    {
        uint16_t words[SST_DPRAM_V2_PARAMETER_WORDS];
        uint16_t i;
        for (i = 0U; i < SST_DPRAM_V2_PARAMETER_WORDS; ++i)
            words[i] = DPRAM_WORD(SST_DPRAM_V2_PARAMETER_BASE + i);
        result = SstDpramV2Parameter_Apply(words, &error_addr);
    }
    else if (op == PARAMETER_SAVE || op == PARAMETER_LOAD)
        result = SstDpramV2Parameter_Store(op == PARAMETER_LOAD, &error_addr);
    else
        result = SST_DPRAM_V2_UNSUPPORTED;

    if (op == PARAMETER_SAVE && result == SST_PARAM_OK)
    {
        s_save_pending = 1U;
        return 0U;
    }
    CompleteParameter(op, result, error_addr);
    return 1U;
}
#endif

void SstDpramHost_Invalidate(void)
{
#if SST_ENABLE_DPRAM
    DPRAM_WORD(SST_DPRAM_V2_SIGNATURE) = 0U;
#endif
}

void SstDpramHost_Initialize(void)
{
#if SST_ENABLE_DPRAM
    uint16_t addr;
    s_control_count = s_parameter_count = s_save_pending = s_switch_permission = 0U;
    s_observation[0] = (HA_PM_A1 | (HA_PM_A2 << 8U)) & SST_DPRAM_V2_PHASE_MASK;
#if SST_PHASE_A_ONLY_TEST
    s_observation[1] = s_observation[2] = 0U;
#else
    s_observation[1] = (HA_PM_B1 | (HA_PM_B2 << 8U)) & SST_DPRAM_V2_PHASE_MASK;
    s_observation[2] = (HA_PM_C1 | (HA_PM_C2 << 8U)) & SST_DPRAM_V2_PHASE_MASK;
#endif
    for (addr = SST_DPRAM_V2_CONTROL; addr < SST_DPRAM_V2_CONTROL + SST_DPRAM_V2_CONTROL_WORDS; ++addr)
        DPRAM_WORD(addr) = 0U;
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_OP) = 0U;
    for (addr = SST_DPRAM_V2_CONTROL_COUNT; addr <= SST_DPRAM_V2_PARAMETER_BUSY; ++addr)
        DPRAM_WORD(addr) = 0U;
    DPRAM_WORD(SST_DPRAM_V2_PARAMETER_ERROR) = SST_DPRAM_V2_NO_ERROR_ADDR;
    PublishParameters();
    SstDpramV2Telemetry_Init();
    /* 先应答上次复位遗留的左邮箱，再发布本次启动签名。 */
    (void)HWREGH(DPRAM_Maibox_Left);
    DPRAM_WORD(SST_DPRAM_V2_VERSION) = 0x0200U;
    DPRAM_WORD(SST_DPRAM_V2_SIGNATURE) = 0x5353U;
    HWREGH(DPRAM_Maibox_Right) = WriteBlock_remRegulate;
#endif
}

void SstDpramHost_Service(void)
{
#if SST_ENABLE_DPRAM
    uint16_t parameter_reply = 0U;
    if (Flag_RemoteControlRegulate != 0U)
    {
        uint16_t block;
        /* 清软件标志，再读左邮箱取消硬件通知。 */
        Flag_RemoteControlRegulate = 0U;
        block = HWREGH(DPRAM_Maibox_Left);
        rtt ^= 1U;
        FPGA_Interface[MainControl_PluginUnit_4 - 2].LED.Bbits.led3 = rtt;
        if (block == ReadBlock_remControl)
            ProcessControl();
        else if (block == ReadBlock_remRegulate)
            parameter_reply = ProcessParameter();
    }

    if (s_save_pending != 0U && SstParameterStore_IsBusy() == 0U)
    {
        s_save_pending = 0U;
        CompleteParameter(PARAMETER_SAVE, SstParameterStore_GetResult(), SST_DPRAM_V2_NO_ERROR_ADDR);
        parameter_reply = 1U;
    }

    SstDpramV2Telemetry_Service();
    if (parameter_reply != 0U)
    {
        /* 让出本轮遥测通知，避免邮箱 20 马上被覆盖。 */
        (void)SstDpramV2Telemetry_TakeLiveNotification();
        HWREGH(DPRAM_Maibox_Right) = WriteBlock_remRegulate;
    }
    else if (SstDpramV2Telemetry_TakeLiveNotification() != 0U)
        HWREGH(DPRAM_Maibox_Right) = WriteBlock_remMeaSig;
#endif
}
