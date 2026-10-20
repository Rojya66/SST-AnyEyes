/* 文件职责: 主循环控制核心：状态机、接触器与故障复位。 */
#include "FpgaIo.h"
#include "SstPortStatus.h"
#include "PwmGate.h"
#include "ControlData.h"
#include "FpgaRegister.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "ProtectionConfig.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "FaultCheck.h"
#include "ModuleVoltageCheck.h"
#include "ChbDabControl.h"
#include "ChbDabControlInternal.h"
#include "ControlInit.h"
#include "interrupt.h"

/*==================================================================
 * 函数功能: 进入主循环控制路径
 * 输入参数: 无
 * 输出参数: 状态机、接触器命令及DAB/CHB启停状态。
 * 返回值: 无
 * 执行说明: 仅供主循环调用；具体状态机集中在本文件的ServiceCore。
 *==================================================================*/
void ChbDabControl_Service(void)
{
    ChbDabControl_ServiceCore();
}

/*==================================================================
 * 函数功能: 每个 10 ms 节拍推进预充、旁路、PLL 等待、运行和停机状态
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_ServiceCore(void)
{
    unsigned long elapsed; /* elapsed：当前状态已经历的 10 ms 节拍数。 */
    unsigned short state; /* state：CHB_STATE_* 运行状态编号。 */
    unsigned short mode; /* mode：本轮慢任务使用的模式，启动后只读取锁存值。 */
    unsigned int interrupt_state; /* interrupt_state：停机模式锁存的原 CPU 中断状态。 */

    interrupt_state = IntGlobalDisable();
    if ((g_chb_dab_status.state == CHB_STATE_STOPPED) && (g_chb_dab_status.fault_code == CHB_FAULT_NONE))
    {
        /* 只有停止态接受模式配置；运行中改模式不能绕过预充/PLL。 */
        s_active_ac_mode = g_chb_dab_command.ac_mode;
    }
    mode = s_active_ac_mode;
    IntGlobalRestore(interrupt_state);
    if ((g_chb_dab_status.state != CHB_STATE_STOPPED) && (g_chb_dab_status.state != CHB_STATE_FAULT) &&
        (g_chb_dab_command.ac_mode != mode))
    {
        ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
        return;
    }

    /*
     * 开环命令不等待 10 ms 慢任务，仿真器写入后主循环下一次即可生效；
     * 闭环仍严格按 10 ms 节拍运行，避免重复执行接触器和超时计数。
     */
    if ((mode != CHB_AC_MODE_OPEN_LOOP) && (s_last_service_tick == g_chb_dab_status.tick_10ms))
    {
        return;
    }
    s_last_service_tick = g_chb_dab_status.tick_10ms;

    interrupt_state = IntGlobalDisable();
    if ((s_breaker_pulse_end_tick != 0UL) && (g_chb_dab_status.tick_10ms >= s_breaker_pulse_end_tick))
    {
        s_breaker_pulse_end_tick = 0UL;
        Chb_ClearAcInputPulse();
    }
    IntGlobalRestore(interrupt_state);

    state = g_chb_dab_status.state;
    elapsed = g_chb_dab_status.tick_10ms - s_state_enter_tick;

#if SST_ENABLE_SWITCH_FEEDBACK
    if (mode == CHB_AC_MODE_CLOSED_LOOP)
    {
        /* 每个后台 10 ms 节拍只读一次实时 DIN，不在 TAD 中等待机械动作。 */
        g_chb_dab_status.switch_din_raw = Port_DINRead(0U);
        g_chb_dab_status.ac_input_feedback =
            ((g_chb_dab_status.switch_din_raw & SST_SWITCH_AC_CLOSED_MASK) != 0U) ==
            SST_SWITCH_AC_CLOSED_HIGH;
        g_chb_dab_status.bypass_feedback =
            ((g_chb_dab_status.switch_din_raw & SST_SWITCH_BYPASS_CLOSED_MASK) != 0U) ==
            SST_SWITCH_BYPASS_CLOSED_HIGH;
        if ((g_chb_dab_status.ac_input_feedback == g_chb_dab_status.ac_input_closed) &&
            (g_chb_dab_status.bypass_feedback == g_chb_dab_status.precharge_bypassed))
        {
            if (g_chb_dab_status.switch_stable_count < 0xFFFFU)
            {
                g_chb_dab_status.switch_stable_count++;
            }
            g_chb_dab_status.switch_mismatch_count = 0U;
        }
        else
        {
            g_chb_dab_status.switch_stable_count = 0U;
            if (g_chb_dab_status.switch_mismatch_count < 0xFFFFU)
            {
                g_chb_dab_status.switch_mismatch_count++;
            }
        }
    }
#endif

    if ((Port_RealFault.Bbits.PORT_FAULT_bit15 != 0U) && (state != CHB_STATE_FAULT))
    {
        /* FPGA端口故障不等待软件计数，立即进入故障态。 */
        ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
        return;
    }

    if (state == CHB_STATE_FAULT)
    {
        /* 故障态只接受“停机状态下的故障复位”，不会边发波边清故障。 */
        if ((g_chb_dab_command.dab_enable != 0U) || (g_chb_dab_status.ac_pwm_enabled != 0U) ||
            (g_chb_dab_status.dab_pwm_enabled != 0U))
        {
            /* 复位前还须撤销 DAB 命令，并确认两桥的使能镜像均已清零。 */
            return;
        }
        if ((g_chb_dab_command.fault_reset != 0U) && (g_chb_dab_command.run_enable == 0U))
        {
#if SST_ENABLE_SWITCH_FEEDBACK
            /* 闭环故障复位前须实际断开进线和旁路，且反馈连续稳定。 */
            if ((mode == CHB_AC_MODE_CLOSED_LOOP) &&
                (g_chb_dab_status.switch_stable_count < SST_SWITCH_FEEDBACK_STABLE_10MS))
            {
                return;
            }
#endif
            /* 只有PWM已停、启动命令已撤销时才允许清除故障锁存。 */
            g_chb_dab_command.fault_reset = 0U;
            g_chb_dab_status.fault_code = CHB_FAULT_NONE;
            Port_RealFault.PORT_FAULT16 = 0U;
            FaultLatch_Clear(0U, 1U);
            Chb_EnterState(CHB_STATE_STOPPED);
        }
        return;
    }

    if (mode == CHB_AC_MODE_OPEN_LOOP)
    {
        Chb_ServiceOpenLoop();
        return;
    }

    if ((g_chb_dab_command.run_enable == 0U) && (state != CHB_STATE_STOPPED))
    {
        Chb_EnterState(CHB_STATE_STOPPED);
        return;
    }

#if SST_ENABLE_SWITCH_FEEDBACK
    /* 运行中连续丢失反馈 30 ms 停机；机械动作及停机反馈允许等待 1 s。 */
    if (state == CHB_STATE_RUNNING)
    {
        if (g_chb_dab_status.switch_mismatch_count >= SST_SWITCH_FEEDBACK_STABLE_10MS)
        {
            ChbDabControl_ForceFault(CHB_FAULT_SWITCH_FEEDBACK);
            return;
        }
    }
    else if ((g_chb_dab_status.switch_mismatch_count >= SST_SWITCH_ACTION_TIMEOUT_10MS) ||
             ((elapsed >= SST_SWITCH_ACTION_TIMEOUT_10MS) &&
              (g_chb_dab_status.switch_stable_count < SST_SWITCH_FEEDBACK_STABLE_10MS)))
    {
        ChbDabControl_ForceFault(CHB_FAULT_SWITCH_FEEDBACK);
        return;
    }
    /* 未经真实反馈确认，不进入下一个启动步骤；上面的超时检查始终执行。 */
    if (g_chb_dab_status.switch_stable_count < SST_SWITCH_FEEDBACK_STABLE_10MS)
    {
        return;
    }
#endif

    switch (state)
    {
        case CHB_STATE_STOPPED:
        {
            if (g_chb_dab_command.run_enable != 0U)
            {
                /* 闭环启动先做基础检查，合格后才闭合交流进线进入预充。 */
                if (Chb_ConfigurationValid() != 0U)
                {
                    Chb_EnterState(CHB_STATE_PRECHARGE);
                }
                else
                {
                    ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
                }
            }
        }
        break;

        case CHB_STATE_PRECHARGE: // 检查配置相的预充电压是否连续达标
        {
            /*
             * 单相取 A 相模块均压，三相取三相模块均压中的最小值；
             * 必须连续达到门槛才旁路，超时未达标进入故障。
             * 此字段不是相内最弱单模块电压；单模块保护仍沿用原 FaultCheck。
             */
            if (g_chb_dab_status.dc_voltage_min_v >= g_chb_dab_params.precharge_voltage_v)
            {
                if (s_precharge_stable_count < 0xFFFFU)
                {
                    s_precharge_stable_count++;
                }
            }
            else
            {
                s_precharge_stable_count = 0U;
            }

            if (s_precharge_stable_count >= g_chb_dab_params.precharge_stable_10ms)
            {
                Chb_EnterState(CHB_STATE_BYPASS_DELAY);
            }
            else if (elapsed >= g_chb_dab_params.precharge_timeout_10ms)
            {
                ChbDabControl_ForceFault(CHB_FAULT_PRECHARGE_TIMEOUT);
            }
        }
        break;

        case CHB_STATE_BYPASS_DELAY: // 旁路延时结束后，按AC模式选择下一状态
        {
            /* 旁路闭合后留出接触器机械稳定时间，再决定开环直达运行还是闭环等 PLL。 */
            if (elapsed >= g_chb_dab_params.bypass_delay_10ms)
            {
                if (mode == CHB_AC_MODE_OPEN_LOOP)
                {
                    /* 开环测试不依赖电网相位，旁路稳定后直接进入RUNNING。 */
                    Chb_EnterState(CHB_STATE_RUNNING);
                }
                else
                {
                    /* 闭环测试先等PLL锁定，再允许AC PWM。 */
                    Chb_EnterState(CHB_STATE_WAIT_PLL);
                }
            }
        }
        break;

        case CHB_STATE_WAIT_PLL: // PLL连续锁定后才允许进入运行态
        {
            /* PLL 必须连续保持锁定，短暂丢锁会清零计数而不是立即发波。 */
            /* PLL锁定必须连续保持指定的10 ms周期数。 */
            if ((g_chb_dab_status.pll_locked != 0U) &&
                (Chb_DcBusReady() != 0U) &&
                (g_chb_dab_command.run_enable != 0U))
            {
                if (s_pll_lock_10ms_count < 0xFFFFU)
                {
                    s_pll_lock_10ms_count++;
                }
            }
            else
            {
                s_pll_lock_10ms_count = 0U;
            }

            if (s_pll_lock_10ms_count >= g_chb_dab_params.pll_lock_10ms)
            {
                Chb_EnterState(CHB_STATE_RUNNING);
            }
            else if (elapsed >= g_chb_dab_params.pll_timeout_10ms)
            {
                ChbDabControl_ForceFault(CHB_FAULT_PLL_TIMEOUT);
            }
        }
        break;

        case CHB_STATE_RUNNING: // 运行态只处理DAB的独立启停
        {
            /* 闭环运行态不再重复判断预充，只处理 DAB 的独立启动和停止。 */
            if ((g_chb_dab_command.dab_enable != 0U) && (g_chb_dab_status.dab_pwm_enabled == 0U))
            {
                /* DAB只启动原边：先给350计数，再在TAD中逐步爬升到50%。 */
                Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
                (void)Chb_EnablePwm(2U);
            }
            else if ((g_chb_dab_command.dab_enable == 0U) && (g_chb_dab_status.dab_pwm_enabled != 0U))
            {
                /* 只撤销DAB PWM，AC侧仍由run_enable决定。 */
                Chb_DisablePwm(2U);
                g_chb_dab_status.dab_pwm_enabled = 0U;
                Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
            }
        }
        break;

        default:
        {
            ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
        }
        break;
    }
}

/*==================================================================
 * 函数功能: 处理开环模式下独立 CHB/DAB 启停及故障复位
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_ServiceOpenLoop(void)
{
    /*
     * 开环不经过 PRECHARGE/BYPASS/WAIT_PLL：
     * run_enable 只控制 CHB，dab_enable 只控制 DAB，两个命令可独立验证。
     * 仍保留基础模块配置检查和故障封波，避免写入明显非法的 FPGA 参数。
     */
    if ((g_chb_dab_command.run_enable == 0U) && (g_chb_dab_command.dab_enable == 0U))
    {
        if (g_chb_dab_status.state != CHB_STATE_STOPPED)
        {
            Chb_EnterState(CHB_STATE_STOPPED);
        }
        return;
    }

    if (g_chb_dab_status.state != CHB_STATE_RUNNING)
    {
        /* 只保留模块数量/调制参数检查，不读取预充电和锁相状态。 */
        if (Chb_ConfigurationValid() == 0U)
        {
            ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
            return;
        }
        Chb_EnterState(CHB_STATE_RUNNING);
        return;
    }

    /* run_enable 可在 DAB 已运行时单独打开/关闭 CHB。 */
    if ((g_chb_dab_command.run_enable != 0U) && (g_chb_dab_status.ac_pwm_enabled == 0U))
    {
        (void)Chb_EnablePwm(1U);
    }
    else if ((g_chb_dab_command.run_enable == 0U) && (g_chb_dab_status.ac_pwm_enabled != 0U))
    {
        FaultDisposeFLAG.Bbits.PWMPulseState = 1U;
        Chb_DisablePwm(1U);
        g_chb_dab_status.ac_pwm_enabled = 0U;
        Chb_WriteNeutralReferences();
    }

    /* dab_enable 可独立启动/停止DAB，启动后仍按原逻辑逐步爬升到50%。 */
    if ((g_chb_dab_command.dab_enable != 0U) && (g_chb_dab_status.dab_pwm_enabled == 0U))
    {
        Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
        (void)Chb_EnablePwm(2U);
    }
    else if ((g_chb_dab_command.dab_enable == 0U) && (g_chb_dab_status.dab_pwm_enabled != 0U))
    {
        FaultDisposeFLAG.Bbits.PWMPulseState = 2U;
        Chb_DisablePwm(2U);
        g_chb_dab_status.dab_pwm_enabled = 0U;
        Chb_WriteDabDuty(g_chb_dab_params.dab_start_duty_count);
    }
}
