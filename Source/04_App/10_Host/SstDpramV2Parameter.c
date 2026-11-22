#include <string.h>
#include "SstDpramV2Parameter.h"
#include "ChbDabControl.h"
#include "SstCommand.h"
#include "SstLegacyParameters.h"
#include "SstParameterStore.h"
#include "SstParameterTable.h"

enum
{
    V2_LEGACY_START = 0x28,
    V2_SOFTWARE_PROTECTION = 0x30,
    V2_AD_MASK = 0x31,
    V2_PM_MASK = 0x36,
    V2_SWITCH_STABLE = 0x39,
    V2_SWITCH_TIMEOUT = 0x3A,
    V2_CHB_MODE = 0x3B,
    V2_SWITCH_MODE = 0x3C
};

/* EEPROM沿用原42字快照。V2把采样周期移到只读遥测，把CHB模式移到遥调末尾。 */
#define SNAPSHOT_CHB_MODE 37U

static uint16_t ControlOffset(uint16_t index)
{
    if (index == SNAPSHOT_CHB_MODE)
        return V2_CHB_MODE;
    return index - ((index < SNAPSHOT_CHB_MODE) ? 1U : 2U);
}

static uint16_t HardwareOffset(uint16_t index)
{
    if (index == SST_HW_SYSTEM_MODE)
        return V2_SWITCH_MODE;
    if (index >= SST_HW_AD_ENABLE && index <= SST_HW_FB_ENABLE)
        return V2_AD_MASK + index - SST_HW_AD_ENABLE;
    if (index >= SST_HW_PM_A1 && index <= SST_HW_PM_C2)
        return V2_PM_MASK + (index - SST_HW_PM_A1) / 2U;
    return SST_PARAM_NO_FIELD;
}

static uint16_t WordAddress(uint16_t offset)
{
    return offset == SST_PARAM_NO_FIELD ? SST_PARAM_NO_FIELD : SST_DPRAM_V2_PARAMETER_BASE + offset;
}

void SstDpramV2Parameter_Read(uint16_t *words)
{
    uint16_t hardware[SST_HARDWARE_PARAMETER_WORDS];
    uint16_t index;

    for (index = 1U; index < SST_STORE_LEGACY_OFFSET; ++index)
        words[ControlOffset(index)] = SstParameterTable_SnapshotRead(index);
    for (index = 0U; index < SST_LEGACY_PARAMETER_COUNT; ++index)
        words[V2_LEGACY_START + index] = SstLegacyParameters_Read(index);

    SstHardwareParameters_Read(hardware);
    words[V2_SOFTWARE_PROTECTION] = g_chb_dab_protection_enable;
    words[V2_SWITCH_MODE] = hardware[SST_HW_SYSTEM_MODE] >> 1U;
    for (index = SST_HW_AD_ENABLE; index <= SST_HW_FB_ENABLE; ++index)
        words[HardwareOffset(index)] = hardware[index];

    /* 501组1对应UNIT 1..8，组2对应UNIT 9..14，空口保留物理编号。 */
    for (index = SST_HW_PM_A1; index <= SST_HW_PM_C2; index += 2U)
        words[HardwareOffset(index)] = hardware[index] | (hardware[index + 1U] << 8U);

    words[V2_SWITCH_STABLE] = g_chb_dab_params.switch_stable_10ms;
    words[V2_SWITCH_TIMEOUT] = g_chb_dab_params.switch_timeout_10ms;
}

uint16_t SstDpramV2Parameter_Apply(const uint16_t *words, uint16_t *error_addr)
{
    uint16_t control[SST_STORE_LEGACY_OFFSET];
    uint16_t hardware[SST_HARDWARE_PARAMETER_WORDS];
    uint16_t actual_hardware[SST_HARDWARE_PARAMETER_WORDS];
    uint16_t index;
    uint16_t error_index;
    uint16_t result;

    *error_addr = SST_PARAM_NO_FIELD;
    if (!SstCommand_IsStopped() || SstParameterStore_IsBusy())
        return SST_PARAM_BUSY;

    control[0] = SstParameterTable_SnapshotRead(0U); /* 固定100 µs周期，V2没有写入口。 */
    for (index = 1U; index < SST_STORE_LEGACY_OFFSET; ++index)
        control[index] = words[ControlOffset(index)];
    result = SstParameterTable_SnapshotValidateAt(control, &error_index);
    if (result != SST_PARAM_OK)
    {
        *error_addr = WordAddress(error_index == 0U ? SST_PARAM_NO_FIELD : ControlOffset(error_index));
        return result;
    }
    result = SstLegacyParameters_ValidateSnapshotAt(&words[V2_LEGACY_START], &error_index);
    if (result != SST_PARAM_OK)
    {
        *error_addr = WordAddress(V2_LEGACY_START + error_index);
        return result;
    }
    if (words[V2_SOFTWARE_PROTECTION] > 1U || words[V2_SWITCH_MODE] > 1U)
    {
        *error_addr = WordAddress(words[V2_SOFTWARE_PROTECTION] > 1U ? V2_SOFTWARE_PROTECTION : V2_SWITCH_MODE);
        return SST_PARAM_RANGE;
    }

    SstHardwareParameters_Read(actual_hardware);
    memcpy(hardware, actual_hardware, sizeof(hardware));
    /* 只改手动/自动位，保留既有运行/测试标志和其他未对屏公开的控制字段。 */
    hardware[SST_HW_SYSTEM_MODE] = (hardware[SST_HW_SYSTEM_MODE] & 1U) | (words[V2_SWITCH_MODE] << 1U);
    for (index = SST_HW_AD_ENABLE; index <= SST_HW_FB_ENABLE; ++index)
        hardware[index] = words[HardwareOffset(index)];
    for (index = SST_HW_PM_A1; index <= SST_HW_PM_C2; index += 2U)
    {
        uint16_t phase_mask = words[HardwareOffset(index)];
        hardware[index] = phase_mask & 0x00FFU;
        hardware[index + 1U] = phase_mask >> 8U; /* 接线校验拒绝组2的第7、8口。 */
    }
    result = SstHardwareParameters_ValidateAt(hardware, &error_index);
    if (result != SST_PARAM_OK)
    {
        *error_addr = WordAddress(HardwareOffset(error_index));
        return result;
    }
    error_index = SstParameterTable_SwitchTimingError(words[V2_SWITCH_STABLE], words[V2_SWITCH_TIMEOUT]);
    if (error_index != SST_PARAM_NO_FIELD)
    {
        *error_addr = WordAddress(V2_SWITCH_STABLE + error_index);
        return SST_PARAM_RANGE;
    }

    /* 到这里整组已通过。只更新变化项，未调节的浮点PI保留原始精度。 */
    SstParameterTable_SnapshotApplyChanged(control);
    for (index = 0U; index < SST_LEGACY_PARAMETER_COUNT; ++index)
    {
        if (words[V2_LEGACY_START + index] != SstLegacyParameters_Read(index))
        {
            SstLegacyParameters_ApplySnapshot(&words[V2_LEGACY_START]);
            break;
        }
    }
    if (memcmp(hardware, actual_hardware, sizeof(hardware)) != 0)
        SstHardwareParameters_Apply(hardware); /* 同步501/601布局及202/701/403保护。 */
    g_chb_dab_protection_enable = words[V2_SOFTWARE_PROTECTION];
    g_chb_dab_params.switch_stable_10ms = words[V2_SWITCH_STABLE];
    g_chb_dab_params.switch_timeout_10ms = words[V2_SWITCH_TIMEOUT];
    return SST_PARAM_OK;
}

uint16_t SstDpramV2Parameter_Store(uint16_t load, uint16_t *error_addr)
{
    uint16_t error_index;
    uint16_t result = SstParameterStore_RequestWithError(load, &error_index);
    uint16_t offset = SST_PARAM_NO_FIELD;

    if (error_index != SST_PARAM_NO_FIELD && error_index != 0U)
    {
        if (error_index < SST_STORE_LEGACY_OFFSET)
            offset = ControlOffset(error_index);
        else if (error_index < SST_STORE_HARDWARE_OFFSET)
            offset = V2_LEGACY_START + error_index - SST_STORE_LEGACY_OFFSET;
        else if (error_index < SST_STORE_SWITCH_OFFSET)
            offset = HardwareOffset(error_index - SST_STORE_HARDWARE_OFFSET);
        else
            offset = V2_SWITCH_STABLE + error_index - SST_STORE_SWITCH_OFFSET;
    }
    *error_addr = WordAddress(offset);
    return result;
}
