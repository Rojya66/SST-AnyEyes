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
    return (g_chb_dab_status.state == CHB_STATE_STOPPED ||
            g_chb_dab_status.state == CHB_STATE_FAULT) &&
           !g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable &&
           !g_chb_dab_status.ac_pwm_enabled && !g_chb_dab_status.dab_pwm_enabled;
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
    if(enable > 1U) return SST_PARAM_RANGE;
    if(enable && SstParameterStore_IsBusy()) return SST_PARAM_BUSY;
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
    if(value > 1U) return SST_PARAM_RANGE;
    if(index == 3U && !SstCommand_IsStopped()) return SST_PARAM_BUSY;
    if(index == 0U) g_chb_dab_command.run_enable = value;
    else if(index == 1U) g_chb_dab_command.dab_enable = value;
    else if(index == 3U) g_chb_dab_command.fault_reset = value;
    else return SST_PARAM_BAD_ADDRESS;
    return SST_PARAM_OK;
#else
    (void)index; (void)value;
    return SST_PARAM_DENIED;
#endif
}

/*==================================================================
 * 函数功能: 按停止和关脉冲优先规则将 DPRAM 字段接入统一控制命令
 * 输入参数: start：DPRAM启动位图，停止/关脉冲优先
 *           stop：DPRAM停止位图，停止优先于启动
 *           pulse_on：脉冲使能位图，bit0 为 DAB、bit1 为 CHB
 *           pulse_off：脉冲禁用位图，bit0 为 DAB、bit1 为 CHB，优先于使能
 *           reset：故障复位请求位图，仅停机时接受
 * 输出参数: g_chb_dab_command 的启停、DAB 使能及故障复位字段；只提交命令。
 * 返回值: 无
 * 执行说明: 后台DPRAM控制入口；只提交请求，实际PWM动作由原控制核心执行。
 *================================================================*/
void SstCommand_ApplyDpram(unsigned short start, unsigned short stop,
                          unsigned short pulse_on, unsigned short pulse_off,
                          unsigned short reset)
{
#if SST_ALLOW_REMOTE_COMMANDS && SST_ENABLE_DPRAM
    /* Only defined command bits participate; all other bits are reserved. */
    pulse_on &= 3U;
    pulse_off &= 3U;
    if(stop & 1U)
    {
        g_chb_dab_command.run_enable = 0U;
        g_chb_dab_command.dab_enable = 0U;
        return;
    }
    /* Off always wins, including while parameter persistence is in progress. */
    if(pulse_off & 1U) g_chb_dab_command.dab_enable = 0U;
    if(pulse_off & 2U) g_chb_dab_command.run_enable = 0U;
    if(SstParameterStore_IsBusy()) return;
    if((start & 1U) && !pulse_off)
        ChbDabControl_ApplyRemoteCommand(start, stop);
    if((pulse_on & 1U) && !(pulse_off & 1U)) g_chb_dab_command.dab_enable = 1U;
    if((pulse_on & 2U) && !(pulse_off & 2U)) g_chb_dab_command.run_enable = 1U;
    if((reset & 1U) && SstCommand_IsStopped()) g_chb_dab_command.fault_reset = 1U;
#else
    (void)start; (void)stop; (void)pulse_on; (void)pulse_off; (void)reset;
#endif
}
