#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstParameterStore.h"
#include "SstCommand.h"

/*==================================================================
 * 函数功能: 检查状态、运行命令和实际PWM使能均满足停机条件
 * 输入参数: 无
 * 输出参数: 无
 * 返回值: 完全停止为1，否则为0
 * 执行说明: 原SstParams_IsStopped判据机械迁移，供命令及参数模块共用。
 *================================================================*/
uint16_t SstCommand_IsStopped(void)
{
    return (g_chb_dab_status.state == CHB_STATE_STOPPED || g_chb_dab_status.state == CHB_STATE_FAULT) &&
           (g_chb_dab_command.run_enable == 0U) &&
           (g_chb_dab_command.dab_enable == 0U) &&
           (g_chb_dab_status.ac_pwm_enabled == 0U) &&
           (g_chb_dab_status.dab_pwm_enabled == 0U);
}

/*==================================================================
 * 函数功能: 按原权限和存储互锁提交串口启停请求
 * 输入参数: enable：0停止、1启动
 * 输出参数: run_enable和dab_enable同步更新
 * 返回值: SST_PARAM_OK/RANGE/BUSY/DENIED
 * 执行说明: 后台调用；不直接访问PWM硬件。
 *================================================================*/
uint16_t SstCommand_RunSerial(uint16_t enable)
{
#if SST_ALLOW_SERIAL_COMMANDS
    if (enable > 1U)
        return SST_PARAM_RANGE;
    if (enable && SstParameterStore_IsBusy())
        return SST_PARAM_BUSY;
    g_chb_dab_command.run_enable = enable;
    g_chb_dab_command.dab_enable = enable;
    return SST_PARAM_OK;
#else
    (void)enable;
    return SST_PARAM_DENIED;
#endif
}

/*==================================================================
 * 函数功能: 撤销控制核心的CHB及DAB运行命令
 * 输入参数: 无
 * 输出参数: 两路运行命令清零
 * 返回值: 无
 * 执行说明: 原参数接口的急停动作迁移，保持异步核心关断机制。
 *================================================================*/
void SstCommand_EmergencyStop(void)
{
    g_chb_dab_command.run_enable = 0U;
    g_chb_dab_command.dab_enable = 0U;
}

/*==================================================================
 * 函数功能: 保持原F0单字段命令的权限、范围和故障复位停机判据
 * 输入参数: index：0运行、1DAB使能、3故障复位
 *           value：0撤销、1请求
 * 输出参数: 指定的g_chb_dab_command字段
 * 返回值: SST_PARAM_*结果码
 * 执行说明: 后台参数门面已先处理存储忙；不直接操作PWM，不联动启停字段。
 *================================================================*/
uint16_t SstCommand_WriteSerialField(uint16_t index, uint16_t value)
{
#if SST_ALLOW_SERIAL_COMMANDS
    if (value > 1U)
        return SST_PARAM_RANGE;
    if ((index == 3U) && (SstCommand_IsStopped() == 0U))
        return SST_PARAM_BUSY;
    if (index == 0U)
        g_chb_dab_command.run_enable = value;
    else if (index == 1U)
        g_chb_dab_command.dab_enable = value;
    else if (index == 3U)
        g_chb_dab_command.fault_reset = value;
    else
        return SST_PARAM_BAD_ADDRESS;
    return SST_PARAM_OK;
#else
    (void)index;
    (void)value;
    return SST_PARAM_DENIED;
#endif
}

/*==================================================================
 * 函数功能: 将DPRAM启动、脉冲及复位有效位赋给实际控制命令
 * 输入参数: start：块1 word2 / 00F2，bit0仅请求HVAC/CHB运行
 *           stop：块1 word3 / 00F3，bit0整机停止，优先撤销两路请求
 *           pulse_on：脉冲使能位图，bit0 为 DAB、bit1 为 CHB
 *           pulse_off：脉冲禁用位图，bit0 为 DAB、bit1 为 CHB，优先于使能
 *           reset：块1 word10 / 00FA，bit0请求故障复位，仅完全停机时接受
 * 输出参数: g_chb_dab_command.run_enable、dab_enable、fault_reset
 * 返回值: 无
 * 执行说明: 后台赋值入口。HVAC START不修改dab_enable；实际发波由原状态机决定。
 *================================================================*/
void SstControlCommand_ApplyRequests(
    unsigned short start, unsigned short stop, unsigned short pulse_on, unsigned short pulse_off, unsigned short reset)
{
#if SST_ALLOW_REMOTE_COMMANDS && SST_ENABLE_DPRAM
    /* 1. 再次限定脉冲有效位，兼容旧邮箱21调用；本函数不访问DPRAM或PWM寄存器。 */
    pulse_on &= 3U;
    pulse_off &= 3U;
    if (stop & 1U)
    {
        /* 2. 保留原整机STOP语义；这是撤销两路请求，不是DAB启动联动。 */
        g_chb_dab_command.run_enable = 0U;
        g_chb_dab_command.dab_enable = 0U;
        return;
    }
    /* 3. word9 / 00F9：按桥独立撤销请求，保存参数忙时也必须允许关闭。 */
    if (pulse_off & 1U)
    {
        /* COM_PLUSE_OFF.bit0：仅关闭DAB请求，不修改CHB的run_enable。 */
        g_chb_dab_command.dab_enable = 0U;
    }
    if (pulse_off & 2U)
    {
        /* COM_PLUSE_OFF.bit1：仅关闭CHB请求；闭环停机仍由核心关闭实际PWM。 */
        g_chb_dab_command.run_enable = 0U;
    }
    if (SstParameterStore_IsBusy())
    {
        return;
    }
    if (((start & 1U) != 0U) && (pulse_off == 0U))
    {
        /* 4. COM_START_PORT.HVAC_Start：实际赋值在这里，只请求CHB运行。
         * 不调用旧ChbDabControl_ApplyRemoteCommand，避免其把DAB也置1。
         * dab_enable原为0就保持0，原为1也保持1；重复START不会重置状态机。 */
        g_chb_dab_command.run_enable = 1U;
    }
    if (((pulse_on & 1U) != 0U) && ((pulse_off & 1U) == 0U))
    {
        /* 5. word8 / 00F8 bit0：DPRAM旧控制块唯一的DAB置1入口。
         * 只提交dab_enable；闭环必须到RUNNING且通过原联锁后才真正解DAB脉冲。 */
        g_chb_dab_command.dab_enable = 1U;
    }
    if (((pulse_on & 2U) != 0U) && ((pulse_off & 2U) == 0U))
    {
        /* 6. word8 / 00F8 bit1：CHB解脉冲请求，写run_enable，不联动DAB。 */
        g_chb_dab_command.run_enable = 1U;
    }
    if ((reset & 1U) && SstCommand_IsStopped())
    {
        /* 7. word10 / 00FA bit0：只置复位请求；核心确认故障源后执行复位。 */
        g_chb_dab_command.fault_reset = 1U;
    }
#else
    (void)start;
    (void)stop;
    (void)pulse_on;
    (void)pulse_off;
    (void)reset;
#endif
}
