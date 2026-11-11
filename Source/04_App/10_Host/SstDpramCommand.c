//###########################################################################
// FILE: SstDpramCommand.c
// FUNCTION: V1.1旧块1命令校验、停机优先、受控开关维护；PWM只由核心决定。
//###########################################################################
#include "SstDpramProtocol.h"
#include "SstCommand.h"
#include "SstParams.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "PortStatusData.h"

/*==================================================================
 * 函数功能: 解释旧16字控制块，把合法命令送入现有控制核心
 * 输入参数: words：完整块1；未动作字段应清零，PORT_NAME可为0或HVAC bit0
 * 输出参数: 启停请求、复位请求、许可或受控开关请求；重复START不重置启动
 * 返回值: 受理结果；表示请求通过检查，实际运行仍须读状态机及故障
 * 执行说明: 仅后台；STOP/OFF优先，即使存储忙也允许撤销请求。
 *================================================================*/
uint16_t SstDpramCommand_Apply(const uint16_t *words)
{
    uint16_t index; /* 控制块校验字序号。 */
    uint16_t permission; /* 本笔处理后的外部合闸许可。 */
#if !SST_ENABLE_DPRAM || !SST_ALLOW_REMOTE_COMMANDS
    (void)words;
    return SST_PARAM_DENIED;
#endif
    g_sst_dpram_screen_result.error_word = 0xFFFFU;
    if (words[3] & 1U)
    {
        SstCommand_EmergencyStop();
        ChbDabControl_MaintenanceSwitch(0U, 3U);
        return SST_PARAM_OK;
    }
    if (words[9] & 3U)
    {
        SstCommand_ApplyDpram(0U, 0U, 0U, words[9], 0U);
        return SST_PARAM_OK;
    }
    if (SstParams_IsStorageBusy())
        return SST_PARAM_BUSY;
    for (index = 0U; index < 16U; ++index)
    {
        uint16_t mask; /* 此字支持位；未知动作不能误路由到本机。 */
        mask = 0U;
        if (index == 2U || index == 3U || index == 10U || index == 12U)
            mask = 1U;
        else if (index == 4U)
            mask = 1U; /* 只支持恒压主模式，不把PQ/Vf等伪装成开闭环。 */
        else if (index >= 6U && index <= 9U)
            mask = 3U; /* 本机仅有内部旁路接触器/进线断路器。 */
        else if (index == 11U)
            mask = 3U;
        if (words[index] & (uint16_t)~mask)
        {
            g_sst_dpram_screen_result.error_word = index;
            return SST_SCREEN_UNSUPPORTED;
        }
    }
    if (words[11] == 3U)
        return SST_PARAM_RANGE;
    permission = g_sst_dpram_screen_result.switch_permission;
    if (words[11] == 1U)
        permission = 1U;
    if (words[11] == 2U)
        permission = 0U;
    if ((words[2] || words[8]) &&
        (g_chb_dab_status.fault_code != CHB_FAULT_NONE || g_chb_dab_status.state == CHB_STATE_FAULT))
        return SST_PARAM_DENIED;
    if (words[10] && !SstCommand_IsStopped())
        return SST_PARAM_BUSY;
    if (words[6] || words[7])
    {
        /* 手动方式才允许维护开关；自动启动仍由原预充状态机操作。 */
        if (PortRunFLAG.Bbits.Port_AutoOrManual || (words[6] && !permission))
            return SST_PARAM_DENIED;
        index = ChbDabControl_MaintenanceSwitch(words[6], words[7]);
        if (index != SST_PARAM_OK)
            return index;
    }
    g_sst_dpram_screen_result.switch_permission = permission;
    SstCommand_ApplyDpram(words[2], 0U, words[8], 0U, words[10]);
    return SST_PARAM_OK;
}
