//###########################################################################
// FILE: SstHardwareParameters.c
// FUNCTION: 把EEPROM接线参数明确连接到501/701/主控的真实变量及FPGA写接口。
//###########################################################################
#include "SstHardwareParameters.h"
#include "ControlData.h"
#include "ProtectionData.h"
#include "PortStatusData.h"
#include "FpgaInit.h"
#include "FpgaIo.h"
#include "SstPortStatus.h"

/*==================================================================
 * 函数功能: 读取16个活动硬件配置字，作为v3镜像的最后一段
 * 输入参数: words：调用者提供的16字缓存，顺序见头文件枚举
 * 输出参数: 当前运行方式、AD/FO/FB使能、六组501位图和旧端口管理方式
 * 返回值: 无
 * 执行说明: 完全停机的后台保存调用；不从旧遥调缓存取值。
 *================================================================*/
void SstHardwareParameters_Read(uint16_t *words)
{
    words[SST_HW_SYSTEM_MODE] = PortRunFLAG.Bbits.Port_RunOrTest |
                               (PortRunFLAG.Bbits.Port_AutoOrManual << 1U);
    words[SST_HW_AD_ENABLE] = Port_AD_PORT_EN;
    words[SST_HW_FO_A_ENABLE] = Port_FO_1_EN;
    words[SST_HW_FO_B_ENABLE] = Port_FO_2_EN;
    words[SST_HW_FO_C_ENABLE] = Port_FO_3_EN;
    words[SST_HW_FB_ENABLE] = Port_FB_EN;
    words[SST_HW_PM_A1] = HA_PM_A1;
    words[SST_HW_PM_A2] = HA_PM_A2;
    words[SST_HW_PM_B1] = HA_PM_B1;
    words[SST_HW_PM_B2] = HA_PM_B2;
    words[SST_HW_PM_C1] = HA_PM_C1;
    words[SST_HW_PM_C2] = HA_PM_C2;
    words[SST_HW_PORT_ON_STATE] = Port_OnState;
    words[SST_HW_MAIN_MODE] = Port_Main_WorkMode;
    words[SST_HW_STATCOM_MODE] = Port_STATCOM_WorkMode;
    words[SST_HW_APF_MODE] = Port_APF_WorkMode;
}

/*==================================================================
 * 函数功能: 校验硬件配置的有效位宽及方式范围，不修改活动对象
 * 输入参数: words：Read定义的完整16字缓存
 * 返回值: SST_PARAM_OK通过，SST_PARAM_RANGE表示格式/范围非法
 * 执行说明: CRC通过后仍需校验内容；全0模块位图允许保留停机配置，
 *           能否启动由控制器的单相/三相拓扑校验决定。
 *================================================================*/
uint16_t SstHardwareParameters_Validate(const uint16_t *words)
{
    uint16_t error_index;
    return SstHardwareParameters_ValidateAt(words, &error_index);
}

uint16_t SstHardwareParameters_ValidateAt(const uint16_t *words, uint16_t *error_index)
{
    /* 按接线限制到物理14路：701低14位，501每相两组分别低8位/低6位。 */
    static const uint16_t allowed[SST_HARDWARE_PARAMETER_WORDS] = {
        0x0003U, 0xF000U, 0x3FFFU, 0x3FFFU, 0x3FFFU, 0xFFFFU,
        0x00FFU, 0x003FU, 0x00FFU, 0x003FU, 0x00FFU, 0x003FU,
        0x000FU, 0x0003U, 0x0003U, 0x0003U
    };
    uint16_t i;
    for (i = 0U; i < SST_HARDWARE_PARAMETER_WORDS; ++i)
    {
        if ((words[i] & ~allowed[i]) != 0U || (i == SST_HW_APF_MODE && words[i] > 2U))
        {
            *error_index = i;
            return SST_PARAM_RANGE;
        }
    }
    *error_index = SST_PARAM_NO_FIELD;
    return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 将通过整幅校验的硬件配置应用至DSP变量和实际FPGA寄存器
 * 输入参数: words：已通过Validate的16字缓存
 * 输出参数: 501 PM/NUM、202 AD、三块701 FO、主控FB及端口方式
 * 返回值: 无
 * 执行说明: 初始化或完全停机调用；不产生任何启停命令或开关动作。
 *================================================================*/
void SstHardwareParameters_Apply(const uint16_t *words)
{
    /* PM -> 六张501的PM/NUM，同时重算NHA_module_*和每相参考基值。
     * 不只恢复HA_PM_*变量，否则FPGA和DSP会使用不同的模块分布。 */
    FpgaInit_ApplyModuleLayout(&words[SST_HW_PM_A1]);

    /* 保护路径逐项明确写入：202 AD、三块701 FO、主控FB。 */
    Port_AD_PORT_EN = words[SST_HW_AD_ENABLE];
    Port_ADProtect_EN();
    Port_FO_1_EN = words[SST_HW_FO_A_ENABLE];
    Port_FO_2_EN = words[SST_HW_FO_B_ENABLE];
    Port_FO_3_EN = words[SST_HW_FO_C_ENABLE];
    Port_FB_EN = words[SST_HW_FB_ENABLE];
    Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
    Port_FB_EN_Set();

    PortRunFLAG.Bbits.Port_RunOrTest = words[SST_HW_SYSTEM_MODE] & 1U;
    PortRunFLAG.Bbits.Port_AutoOrManual = (words[SST_HW_SYSTEM_MODE] >> 1U) & 1U;
    Port_OnState = words[SST_HW_PORT_ON_STATE];
    Port_Main_WorkMode = words[SST_HW_MAIN_MODE];
    Port_STATCOM_WorkMode = words[SST_HW_STATCOM_MODE];
    Port_APF_WorkMode = words[SST_HW_APF_MODE];
    Port_StateUpdate_Write();
    /* run_enable、dab_enable、fault_reset和物理QF1/KT1输出均不在存储对象中。 */
}
