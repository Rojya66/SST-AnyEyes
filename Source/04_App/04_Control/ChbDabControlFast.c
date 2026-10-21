/* 文件职责: TAD中断控制核心：采样控制、保护与调制。 */
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
#include <math.h>
#include <float.h>
#include "FaultCheck.h"
#include "ChbDabControl.h"
#include "SstSinglePhase.h"
#include "SstPi.h"
#include "ChbDabControlInternal.h"
#include "interrupt.h"

/*==================================================================
 * 函数功能: 进入TAD中断控制路径
 * 输入参数: 无
 * 输出参数: PLL、PI、调制参考、保护状态及DAB占空比。
 * 返回值: 无
 * 执行说明: 仅供FPGA采样中断调用；具体计算集中在本文件的FastStepCore。
 *==================================================================*/
void ChbDabControl_FastStep(void)
{
    ChbDabControl_FastStepCore();
}

/*==================================================================
 * 函数功能: 在原 TAD 路径执行 PLL、开环/闭环控制、软件保护及 DAB 占空比斜坡
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void ChbDabControl_FastStepCore(void)
{
    float voltage_alpha; /* voltage_alpha：电网/端口电压 α 轴分量，单位 V。 */
    float voltage_beta; /* voltage_beta：电网/端口电压 β 轴分量，单位 V。 */
    unsigned short duty; /* duty：本拍 DAB 占空比计数。 */
    unsigned short step; /* step：DAB 占空比每个 TAD 的递增计数。 */
    unsigned short angle_valid; /* angle_valid：PLL 相角是否在允许范围内。 */
    unsigned short sampling_valid; /* sampling_valid：本拍采样量是否全部通过有限值检查。 */

    /*
     * TAD 快速任务入口。该函数在 FPGA 采样同步中断中调用，不能放入耗时的
     * 通讯或 EEPROM 操作；这里只做浮点控制计算和 FPGA 寄存器/内存映射写入。
     */

    /* 参数校验由主循环在启动/在线配置入口完成，快速路径只检查运行数据。 */
    if (Port_RealFault.Bbits.PORT_FAULT_bit15 != 0U)
    {
        ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
        return;
    }
    if (g_chb_dab_status.state != CHB_STATE_STOPPED)
    {
        if (g_chb_dab_status.state != CHB_STATE_FAULT)
        {
            if (g_chb_dab_command.ac_mode != s_active_ac_mode)
            {
                /* 模式只在停止态重新锁存，不能从开环直接跳入闭环计算。 */
                ChbDabControl_ForceFault(CHB_FAULT_BAD_CONFIGURATION);
                return;
            }
        }
    }

    angle_valid = Chb_FiniteInRange(g_chb_dab_status.pll_angle_rad, 0.0F, CHB_TWO_PI);
    if (angle_valid == 0U)
    {
        g_chb_dab_status.pll_angle_rad = 0.0F;
        if (g_chb_dab_status.state != CHB_STATE_STOPPED)
        {
            ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
            return;
        }
    }
    else if (g_chb_dab_status.pll_angle_rad >= CHB_TWO_PI)
    {
        g_chb_dab_status.pll_angle_rad = 0.0F;
        if (g_chb_dab_status.state != CHB_STATE_STOPPED)
        {
            ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
            return;
        }
    }
#if SST_PHASE_A_ONLY_TEST
    /* 单相预充、启动给定和电压外环均只读取 A 相模块均压。 */
    g_chb_dab_status.dc_voltage_average_v = SVG_Udc_A_Ave;
    g_chb_dab_status.dc_voltage_min_v = SVG_Udc_A_Ave;
    SstSinglePhase_FastStep(
        HA_Port_Volt_A, HA_Port_Current_A, g_chb_dab_status.pll_frequency_hz, g_chb_dab_params.sample_period_s);
    voltage_alpha = g_sst_single_phase_status.voltage_alpha_v;
    voltage_beta = g_sst_single_phase_status.voltage_beta_v;
    sampling_valid = g_sst_single_phase_status.valid;
    if (Chb_FiniteInRange(SVG_Udc_A_Ave, 0.0F, 1000000.0F) == 0U)
    {
        sampling_valid = 0U;
    }
    if (sampling_valid == 0U)
    {
        s_pll_integral = 0.0F;
        s_pll_lock_fast_count = 0U;
        g_chb_dab_status.pll_locked = 0U;
        g_chb_dab_status.pll_frequency_hz = g_chb_dab_params.pll_nominal_frequency_hz;
        if (g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP)
        {
            if (g_chb_dab_status.state == CHB_STATE_RUNNING)
            {
                ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
            }
            Chb_WriteNeutralReferences();
            return;
        }
        /* 开环脉冲验证仍独立于闭环采样有效性；非法输入不会送入 PLL。 */
        voltage_alpha = 0.0F;
        voltage_beta = 0.0F;
    }
#else
    g_chb_dab_status.dc_voltage_average_v = (SVG_Udc_A_Ave + SVG_Udc_B_Ave + SVG_Udc_C_Ave) / 3.0F;
    g_chb_dab_status.dc_voltage_min_v = Chb_Min3(SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave);

    voltage_alpha = HA_Port_Volt_A;
    voltage_beta = (HA_Port_Volt_A + 2.0F * HA_Port_Volt_B) * CHB_ONE_BY_SQRT3;
    /* 三相输入也必须有限，防止错误采样进入 PLL/PI。
     * 开环仍允许无主回路脉冲测试；闭环运行遇非法采样立即封波。
     */
    sampling_valid = 1U;
    if (Chb_FiniteInRange(HA_Port_Volt_A, -1000000.0F, 1000000.0F) == 0U)
    {
        sampling_valid = 0U;
    }
    if (Chb_FiniteInRange(HA_Port_Volt_B, -1000000.0F, 1000000.0F) == 0U)
    {
        sampling_valid = 0U;
    }
    if (Chb_FiniteInRange(HA_Port_Volt_C, -1000000.0F, 1000000.0F) == 0U)
    {
        sampling_valid = 0U;
    }
    if (Chb_FiniteInRange(HA_Port_Current_A, -1000000.0F, 1000000.0F) == 0U)
    {
        sampling_valid = 0U;
    }
    if (Chb_FiniteInRange(HA_Port_Current_B, -1000000.0F, 1000000.0F) == 0U)
    {
        sampling_valid = 0U;
    }
    if (Chb_FiniteInRange(HA_Port_Current_C, -1000000.0F, 1000000.0F) == 0U)
    {
        sampling_valid = 0U;
    }
    if (NHA_module_A != 0U)
    {
        if (Chb_FiniteInRange(SVG_Udc_A_Ave, 0.0F, 1000000.0F) == 0U)
        {
            sampling_valid = 0U;
        }
    }
    if (NHA_module_B != 0U)
    {
        if (Chb_FiniteInRange(SVG_Udc_B_Ave, 0.0F, 1000000.0F) == 0U)
        {
            sampling_valid = 0U;
        }
    }
    if (NHA_module_C != 0U)
    {
        if (Chb_FiniteInRange(SVG_Udc_C_Ave, 0.0F, 1000000.0F) == 0U)
        {
            sampling_valid = 0U;
        }
    }
    if (sampling_valid == 0U)
    {
        s_pll_integral = 0.0F;
        s_pll_lock_fast_count = 0U;
        g_chb_dab_status.pll_locked = 0U;
        g_chb_dab_status.pll_frequency_hz = g_chb_dab_params.pll_nominal_frequency_hz;
        if (g_chb_dab_command.ac_mode == CHB_AC_MODE_CLOSED_LOOP)
        {
            if (g_chb_dab_status.state == CHB_STATE_RUNNING)
            {
                ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
            }
            Chb_WriteNeutralReferences();
            return;
        }
        voltage_alpha = 0.0F;
        voltage_beta = 0.0F;
    }
#endif
    Chb_UpdatePll(voltage_alpha, voltage_beta);
#if SST_PHASE_A_ONLY_TEST
    if (g_sst_single_phase_status.ready == 0U)
    {
        /* 滤波建立前禁止累计锁定确认；建立后仍需通过原 PLL 连续锁定及 10 ms 状态机确认。 */
        s_pll_lock_fast_count = 0U;
        g_chb_dab_status.pll_locked = 0U;
    }
#endif

    /* 闭环启动阶段也执行统一快速保护和有效值过流；停机/故障态不消费旧采样。 */
    if (s_active_ac_mode == CHB_AC_MODE_CLOSED_LOOP)
    {
        if (g_chb_dab_status.state != CHB_STATE_STOPPED)
        {
            if (g_chb_dab_status.state != CHB_STATE_FAULT)
            {
                Chb_CheckSoftwareProtection();
                if (g_chb_dab_status.state == CHB_STATE_FAULT)
                {
                    return;
                }
            }
        }
    }

    if (g_chb_dab_status.state != CHB_STATE_RUNNING)
    {
        s_pll_loss_count = 0U;
        /* 非运行态写入指令中点；PWM 撤销使能由状态入口动作处理。 */
        Chb_WriteNeutralReferences();
        return;
    }

    if (s_active_ac_mode == CHB_AC_MODE_CLOSED_LOOP)
    {
        /* 母线跌破已校验的预充门槛时立即封波，禁止低母线继续归一化输出。 */
        if (Chb_DcBusReady() == 0U)
        {
            ChbDabControl_ForceFault(CHB_FAULT_CONTROL_RUNTIME);
            return;
        }
        if (g_chb_dab_status.pll_locked == 0U)
        {
            if (s_pll_loss_count < CHB_PLL_LOSS_SAMPLES)
            {
                s_pll_loss_count++;
            }
            if (s_pll_loss_count >= CHB_PLL_LOSS_SAMPLES)
            {
                /* 幅值不足或相位误差连续不合格共 20 ms，复用 PLL 故障及其上报机制。 */
                ChbDabControl_ForceFault(CHB_FAULT_PLL_TIMEOUT);
                return;
            }
        }
        else
        {
            s_pll_loss_count = 0U;
        }
    }

    if (g_chb_dab_status.ac_pwm_enabled == 0U)
    {
        /* DAB 独立开环测试时，已撤销使能的 CHB 保持指令中点。 */
        Chb_WriteNeutralReferences();
    }
    else if (g_chb_dab_command.ac_mode == CHB_AC_MODE_OPEN_LOOP)
    {
        Chb_RunOpenLoop();
    }
    else
    {
        Chb_RunClosedLoop(voltage_alpha, voltage_beta);
    }

    if (g_chb_dab_status.state == CHB_STATE_RUNNING)
    {
        if (g_chb_dab_status.dab_pwm_enabled != 0U)
        {
            /* DAB 每个 TAD 只增加固定步长，避免启动瞬间占空比跳到 50%。 */
            duty = g_chb_dab_status.dab_duty_count;
            step = g_chb_dab_params.dab_ramp_step_count;
            if (duty < g_chb_dab_params.dab_target_duty_count)
            {
                if ((unsigned short)(g_chb_dab_params.dab_target_duty_count - duty) < step)
                {
                    duty = g_chb_dab_params.dab_target_duty_count;
                }
                else
                {
                    duty = (unsigned short)(duty + step);
                }
                Chb_WriteDabDuty(duty);
            }
        }
    }
}

/*==================================================================
 * 函数功能: 依据 αβ 电压执行归一化 q 轴误差 PI，更新相角、频率及锁定计数
 * 输入参数: voltage_alpha：电网/端口电压 α 轴分量，单位 V
 *           voltage_beta：电网/端口电压 β 轴分量，单位 V
 * 输出参数: s_pll_integral、PLL 相角/频率以及锁定计数、状态镜像。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_UpdatePll(float voltage_alpha, float voltage_beta)
{
    float magnitude; /* magnitude：PLL 输入 αβ 电压矢量幅值，单位 V。 */
    float sine_angle; /* sine_angle：当前 PLL 相角的正弦值。 */
    float cosine_angle; /* cosine_angle：当前 PLL 相角的余弦值。 */
    float q_voltage; /* q_voltage：PLL 坐标变换的 q 轴电压，单位 V。 */
    float d_voltage; /* d_voltage：正向锁定判据，防止 q=0、相差 180° 时假锁定。 */
    float q_error; /* q_error：按电压矢量幅值归一化的 PLL q 轴误差。 */
    float correction; /* correction：PLL PI 输出的角速度修正，单位 rad/s。 */
    float omega; /* omega：电网额定角速度加 PLL 修正量，单位 rad/s。 */
    unsigned short lock_samples; /* lock_samples：由锁定时间及 sample_period_s 换算的所需 TAD 样本数。 */

    /*
     * 软件 PLL 输入为电网 alpha/beta 电压：
     * 1) 用当前角度计算 q 轴电压；
     * 2) q_voltage/magnitude 作为归一化相位误差；
     * 3) PI 输出频率修正量，并限制在 +/- correction_limit_rad_s；
     * 4) 积分得到下一采样周期的电网角度。
     * 电压幅值不足时不更新锁定计数，只按额定频率自由运行。
     */
    magnitude = sqrtf(voltage_alpha * voltage_alpha + voltage_beta * voltage_beta);
    sine_angle = sinf(g_chb_dab_status.pll_angle_rad);
    cosine_angle = cosf(g_chb_dab_status.pll_angle_rad);

    if (magnitude >= g_chb_dab_params.pll_min_grid_peak_v)
    {
        q_voltage = -voltage_alpha * sine_angle + voltage_beta * cosine_angle;
        q_error = q_voltage / magnitude; // q轴归一化误差，作为PLL校正输入
        s_pll_integral += g_chb_dab_params.pll_ki * q_error * g_chb_dab_params.sample_period_s;
        s_pll_integral = Chb_Clamp(
            s_pll_integral, -g_chb_dab_params.pll_correction_limit_rad_s, g_chb_dab_params.pll_correction_limit_rad_s);
        correction = g_chb_dab_params.pll_kp * q_error + s_pll_integral;
        correction = Chb_Clamp(
            correction, -g_chb_dab_params.pll_correction_limit_rad_s, g_chb_dab_params.pll_correction_limit_rad_s);
        omega = CHB_TWO_PI * g_chb_dab_params.pll_nominal_frequency_hz + correction;
        g_chb_dab_status.pll_angle_rad =
            Chb_WrapAngle(g_chb_dab_status.pll_angle_rad + omega * g_chb_dab_params.sample_period_s);
        g_chb_dab_status.pll_frequency_hz = omega / CHB_TWO_PI;

        lock_samples = (unsigned short)(0.010F / g_chb_dab_params.sample_period_s);
        if (lock_samples == 0U)
        {
            lock_samples = 1U;
        }
        d_voltage = voltage_alpha * cosine_angle + voltage_beta * sine_angle;
        if (d_voltage > 0.0F)
        {
            if (Chb_Abs(q_error) <= g_chb_dab_params.pll_lock_error_pu)
            {
                if (s_pll_lock_fast_count < lock_samples)
                {
                    s_pll_lock_fast_count++;
                }
            }
            else
            {
                s_pll_lock_fast_count = 0U;
            }
        }
        else
        {
            s_pll_lock_fast_count = 0U;
        }
        if (s_pll_lock_fast_count >= lock_samples)
        {
            g_chb_dab_status.pll_locked = 1U;
        }
        else
        {
            g_chb_dab_status.pll_locked = 0U;
        }
    }
    else
    {
        s_pll_lock_fast_count = 0U;
        g_chb_dab_status.pll_locked = 0U;
        g_chb_dab_status.pll_frequency_hz = g_chb_dab_params.pll_nominal_frequency_hz;
        g_chb_dab_status.pll_angle_rad =
            Chb_WrapAngle(g_chb_dab_status.pll_angle_rad +
                          CHB_TWO_PI * g_chb_dab_params.pll_nominal_frequency_hz * g_chb_dab_params.sample_period_s);
    }
}

/*==================================================================
 * 函数功能: 推进开环正弦相角并写入三相限幅调制参考
 * 输入参数: 无
 * 输出参数: 开环相角、三相调制给定及运行调制幅值状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_RunOpenLoop(void)
{
    float modulation; /* modulation：限幅后的开环调制比。 */
    float phase_a; /* phase_a：A 相归一化参考 1+M*sin(theta)，无量纲。 */
    float phase_b; /* phase_b：B 相归一化参考 1+M*sin(theta)，无量纲。 */
    float phase_c; /* phase_c：C 相归一化参考 1+M*sin(theta)，无量纲。 */

    /*
     * 开环正弦发生器：angle(k+1)=angle(k)+2*pi*f*Ts，
     * 三相相差 +/-120 度，最后交给统一的 Reference 写入函数限幅。
     */
    modulation = Chb_Clamp(g_chb_dab_command.open_loop_modulation, 0.0F, g_chb_dab_params.modulation_max);
    /* 开环模式只使用设定频率和调制比，便于单板验证CHB发波。 */
    s_open_loop_angle = Chb_WrapAngle(s_open_loop_angle + CHB_TWO_PI * g_chb_dab_command.open_loop_frequency_hz *
                                                              g_chb_dab_params.sample_period_s);

    phase_a = 1.0F + modulation * sinf(s_open_loop_angle);
    phase_b = 1.0F + modulation * sinf(s_open_loop_angle - 2.0F * CHB_PI / 3.0F);
    phase_c = 1.0F + modulation * sinf(s_open_loop_angle + 2.0F * CHB_PI / 3.0F);
    Chb_WriteNormalizedReferences(phase_a, phase_b, phase_c);
}

/*==================================================================
 * 函数功能: 执行兼容启动或 Id 斜坡，并在满足条件时移交电压外环
 * 输入参数: 无
 * 输出参数: Id 给定、启动阶段/拍数、外环接管标志及电压 PI 历史状态。
 * 返回值: 本拍闭环 d 轴电流给定，单位 A
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
float Chb_GetClosedLoopIdReference(void)
{
    float startup_target; /* startup_target：启动阶段最终 Id 目标，单位 A。 */
    float ramp_step; /* ramp_step：当前 Id 斜坡每拍增量，单位 A/TAD。 */
    float voltage_error; /* voltage_error：直流电压给定减反馈的误差，单位 V。 */
    float id_target; /* id_target：本拍计算出的 Id 目标，单位 A。 */
    unsigned short handover_now; /* handover_now：本拍需要从启动斜坡移交电压环的标志。 */

    /*
     * 闭环启动给定的三个阶段：
     * ID_RAMP：固定 Id 目标从 0 逐步爬升；
     * HANDOVER：保持一拍，观察电压 PI 接管点；
     * NORMAL：电压外环根据 Udc 误差产生 Idref，并限幅到电流上限。
     */
    handover_now = 0U;
    startup_target = Chb_Clamp(g_chb_dab_params.startup_id_reference_a,
                               -g_chb_dab_params.voltage_current_limit_a,
                               g_chb_dab_params.voltage_current_limit_a);
#if SST_LOW_VOLTAGE_100V_TEST
    g_chb_dab_status.current_limit_active_a =
        Chb_Clamp(g_chb_dab_status.current_limit_active_a, 0.0F, g_chb_dab_params.voltage_current_limit_a);
    startup_target = Chb_Clamp(startup_target, -g_chb_dab_status.current_limit_active_a,
                               g_chb_dab_status.current_limit_active_a);
#endif
    ramp_step = Chb_Abs(g_chb_dab_params.startup_id_ramp_step_a);

    if (s_startup_phase == CHB_STARTUP_ID_RAMP_PHASE)
    {
        /* 固定Idref斜坡：只改变给定，不改变源程序PI方向。 */
        if (s_id_reference < startup_target)
        {
            s_id_reference += ramp_step;
            if (s_id_reference > startup_target)
            {
                s_id_reference = startup_target;
            }
        }
        else if (s_id_reference > startup_target)
        {
            s_id_reference -= ramp_step;
            if (s_id_reference < startup_target)
            {
                s_id_reference = startup_target;
            }
        }

        s_startup_tad_count++;
        if (s_id_reference == startup_target)
        {
            /* 斜坡已经到达目标。 */
            handover_now = 1U;
        }
        else if (s_startup_tad_count >= g_chb_dab_params.startup_ramp_tad)
        {
            /* 到达最长斜坡时间时强制进入接管，避免异常参数导致永久等待。 */
            handover_now = 1U;
        }
        if (handover_now != 0U)
        {
            /* 让电压PI当前输出等于斜坡末值，避免接管时Idref跳变。 */
            voltage_error = s_startup_voltage_reference - g_chb_dab_status.dc_voltage_average_v;
            Chb_AlignVoltagePiToIdReference(voltage_error, s_id_reference);
            if (g_chb_dab_status.state == CHB_STATE_FAULT)
            {
                return 0.0F;
            }
            s_startup_phase = CHB_STARTUP_VOLTAGE_HANDOVER;
            g_chb_dab_status.voltage_loop_active = 0U;
            handover_now = 1U;
        }
    }

    if (s_startup_phase == CHB_STARTUP_VOLTAGE_HANDOVER)
    {
        /* 保持一拍固定Id，下一拍再执行电压PI，便于示波器分辨接管点。 */
        if (handover_now == 0U)
        {
            s_startup_phase = CHB_STARTUP_NORMAL;
            g_chb_dab_status.voltage_loop_active = 1U;
        }
    }

    if (s_startup_phase == CHB_STARTUP_NORMAL)
    {
        Chb_UpdateVoltageReference();
#if SST_LOW_VOLTAGE_100V_TEST
        /* 最终给定已到位且实测母线连续达标 200 ms，才允许逐步放宽电流预算。
         * 本次运行锁存达标状态；带载后的母线暂降不会把限流突然收回 1 A。 */
        if (g_chb_dab_status.load_ready == 0U)
        {
            if (s_startup_voltage_reference == g_chb_dab_command.dc_voltage_ref_v)
            {
                if (Chb_Abs(g_chb_dab_status.dc_voltage_average_v - g_chb_dab_command.dc_voltage_ref_v) <=
                    SST_LV100_READY_ERROR_V)
                {
                    if (g_chb_dab_status.load_ready_count < SST_LV100_READY_TAD)
                    {
                        g_chb_dab_status.load_ready_count++;
                    }
                    if (g_chb_dab_status.load_ready_count >= SST_LV100_READY_TAD)
                    {
                        g_chb_dab_status.load_ready = 1U;
                    }
                }
                else
                {
                    g_chb_dab_status.load_ready_count = 0U;
                }
            }
            else
            {
                g_chb_dab_status.load_ready_count = 0U;
            }
        }
        if (g_chb_dab_status.load_ready != 0U)
        {
            g_chb_dab_status.current_limit_active_a +=
                SST_LV100_CURRENT_LIMIT_SLEW_A_S * g_chb_dab_params.sample_period_s;
        }
        /* 在线降低运行上限时立即收紧；提高上限仍走斜坡。 */
        g_chb_dab_status.current_limit_active_a =
            Chb_Clamp(g_chb_dab_status.current_limit_active_a, 0.0F, g_chb_dab_params.voltage_current_limit_a);
#endif
        voltage_error = s_startup_voltage_reference - g_chb_dab_status.dc_voltage_average_v;
        id_target = Chb_VoltagePiLimited(s_startup_voltage_reference, g_chb_dab_status.dc_voltage_average_v);
        if (g_chb_dab_status.state == CHB_STATE_FAULT)
        {
            return 0.0F;
        }
        s_id_reference =
            Chb_Clamp(id_target, -g_chb_dab_params.voltage_current_limit_a, g_chb_dab_params.voltage_current_limit_a);
        g_chb_dab_status.voltage_loop_active = 1U;
    }

    g_chb_dab_status.startup_id_reference_a = s_id_reference;
#if SST_LOW_VOLTAGE_100V_TEST
    if (s_startup_phase != CHB_STARTUP_NORMAL)
    {
        float iq; /* iq：占用本拍峰值预算的 q 轴电流给定，单位 A。 */
        float id_limit; /* id_limit：扣除 Iq 后剩余的 Id 幅值预算，单位 A。 */
        iq = Chb_Clamp(g_chb_dab_command.iq_reference_a, -g_chb_dab_status.current_limit_active_a,
                       g_chb_dab_status.current_limit_active_a);
        if (g_chb_dab_status.current_limit_active_a > Chb_Abs(iq))
        {
            id_limit = sqrtf(g_chb_dab_status.current_limit_active_a *
                             g_chb_dab_status.current_limit_active_a - iq * iq);
        }
        else
        {
            id_limit = 0.0F;
        }
        s_id_reference = Chb_Clamp(s_id_reference, -id_limit, id_limit);
        g_chb_dab_status.startup_id_reference_a = s_id_reference;
    }
#endif
    g_chb_dab_status.startup_phase = s_startup_phase;
    g_chb_dab_status.startup_tad_count = s_startup_tad_count;
    return s_id_reference;
}

/*==================================================================
 * 函数功能: 按原反馈方向执行 dq 电流闭环及电压前馈，输出三相调制参考
 * 输入参数: voltage_alpha：电网/端口电压 α 轴分量，单位 V
 *           voltage_beta：电网/端口电压 β 轴分量，单位 V
 * 输出参数: dq 电流反馈/PI 历史、三相调制参考以及对应控制状态。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_RunClosedLoop(float voltage_alpha, float voltage_beta)
{
    float current_sine_angle; /* current_sine_angle：加入电流相位补偿后的正弦值。 */
    float current_cosine_angle; /* current_cosine_angle：加入电流相位补偿后的余弦值。 */
    float voltage_sine_angle; /* voltage_sine_angle：电网电压 dq 变换的正弦值。 */
    float voltage_cosine_angle; /* voltage_cosine_angle：电网电压 dq 变换的余弦值。 */
    float output_sine_angle; /* output_sine_angle：加入输出相位补偿后的逆变换正弦值。 */
    float output_cosine_angle; /* output_cosine_angle：加入输出相位补偿后的逆变换余弦值。 */
    float current_alpha; /* current_alpha：端口电流 α 轴分量，单位 A。 */
    float current_beta; /* current_beta：端口电流 β 轴分量，单位 A。 */
    float current_d; /* current_d：端口电流 d 轴分量，单位 A。 */
    float current_q; /* current_q：端口电流 q 轴分量，单位 A。 */
    float iq_reference; /* iq_reference：本拍实际 q 轴给定；低压时与 Id 共用电流矢量预算，单位 A。 */
    float voltage_d; /* voltage_d：电网电压 d 轴前馈分量，单位 V。 */
    float voltage_q; /* voltage_q：电网电压 q 轴前馈分量，单位 V。 */
    float command_d; /* command_d：限幅前/后的 d 轴输出电压指令，单位 V。 */
    float command_q; /* command_q：限幅前/后的 q 轴输出电压指令，单位 V。 */
    float command_alpha; /* command_alpha：输出电压 α 轴指令，单位 V。 */
#if !SST_PHASE_A_ONLY_TEST
    float command_beta; /* command_beta：三相输出电压 β 轴指令，单位 V。 */
#endif
    float phase_a; /* phase_a：A 相输出电压指令，单位 V；写入前换算为归一化参考。 */
#if !SST_PHASE_A_ONLY_TEST
    float phase_b; /* phase_b：B 相输出电压指令，单位 V；写入前换算为归一化参考。 */
    float phase_c; /* phase_c：C 相输出电压指令，单位 V；写入前换算为归一化参考。 */
#endif
    float phase_dc_a; /* phase_dc_a：A 相模块数乘实测模块均压，单位 V。 */
    float voltage_limit; /* voltage_limit：活动相最小实际母线乘有效调制上限，单位 V。 */
    unsigned short current_pi_valid; /* current_pi_valid：d/q 电流 PI 及矢量限幅是否成功。 */
#if !SST_PHASE_A_ONLY_TEST
    float phase_dc_b; /* phase_dc_b：B 相模块数乘实测模块均压的总电压尺度，单位 V。 */
    float phase_dc_c; /* phase_dc_c：C 相模块数乘实测模块均压的总电压尺度，单位 V。 */
#endif

    /*
     * 闭环控制计算顺序：
     * 1) 采样三相电流/电压转换到 alpha-beta，再旋转到 dq；
     * 2) 直流电压外环给出 Idref，Iqref 由命令给出；
     * 3) d/q 电流 PI 输出电压修正量并叠加电网电压前馈；
     * 4) 逆 Park/Clarke 变换成三相电压命令；
     * 5) 按每相模块数和 Udc 限幅，归一化后写入 FPGA。
     */
    current_sine_angle = sinf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.current_phase_compensation_rad);
    current_cosine_angle = cosf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.current_phase_compensation_rad);
    voltage_sine_angle = sinf(g_chb_dab_status.pll_angle_rad);
    voltage_cosine_angle = cosf(g_chb_dab_status.pll_angle_rad);
    output_sine_angle = sinf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.output_phase_compensation_rad);
    output_cosine_angle = cosf(g_chb_dab_status.pll_angle_rad + g_chb_dab_params.output_phase_compensation_rad);

    /* 先把采样量变换到同步dq坐标系，电流极性由可调符号统一处理。 */
#if SST_PHASE_A_ONLY_TEST
    /* 两轴使用同一 SOGI 的滤波输出，并成对应用既有电流极性；虚拟 beta 不写入物理 B/C 采样。 */
    current_alpha = g_chb_dab_params.current_feedback_sign * g_sst_single_phase_status.current_alpha_a;
    current_beta = g_chb_dab_params.current_feedback_sign * g_sst_single_phase_status.current_beta_a;
#else
    current_alpha = g_chb_dab_params.current_feedback_sign * HA_Port_Current_A;
    current_beta =
        g_chb_dab_params.current_feedback_sign * (HA_Port_Current_A + 2.0F * HA_Port_Current_B) * CHB_ONE_BY_SQRT3;
#endif
    current_d = current_alpha * current_cosine_angle + current_beta * current_sine_angle;
    current_q = -current_alpha * current_sine_angle + current_beta * current_cosine_angle;
    voltage_d = voltage_alpha * voltage_cosine_angle + voltage_beta * voltage_sine_angle;
    voltage_q = -voltage_alpha * voltage_sine_angle + voltage_beta * voltage_cosine_angle;

    g_chb_dab_status.id_feedback_a = current_d;
    g_chb_dab_status.iq_feedback_a = current_q;

    /* 启动阶段先按固定Idref斜坡运行，接管后才投入直流电压外环。 */
    s_id_reference = Chb_GetClosedLoopIdReference();
    g_chb_dab_status.id_reference_a = s_id_reference;

    if (g_chb_dab_status.state == CHB_STATE_FAULT)
    {
        return;
    }
    /* 单相与三相均按实测母线归一化；三相取最弱相总母线作为保守矢量边界。 */
    phase_dc_a = (float)NHA_module_A * SVG_Udc_A_Ave;
#if !SST_PHASE_A_ONLY_TEST
    phase_dc_b = (float)NHA_module_B * SVG_Udc_B_Ave;
    phase_dc_c = (float)NHA_module_C * SVG_Udc_C_Ave;
    voltage_limit = Chb_Min3(phase_dc_a, phase_dc_b, phase_dc_c);
#else
    voltage_limit = phase_dc_a;
#endif
    if (g_chb_dab_params.modulation_max < g_chb_dab_params.closed_loop_voltage_limit_factor)
    {
        voltage_limit *= g_chb_dab_params.modulation_max;
    }
    else
    {
        voltage_limit *= g_chb_dab_params.closed_loop_voltage_limit_factor;
    }
    iq_reference = g_chb_dab_command.iq_reference_a;
#if SST_LOW_VOLTAGE_100V_TEST
    iq_reference = Chb_Clamp(iq_reference, -g_chb_dab_status.current_limit_active_a,
                             g_chb_dab_status.current_limit_active_a);
#endif
    current_pi_valid = Chb_CurrentPiLimited(voltage_d, voltage_q, s_id_reference - current_d,
                                            iq_reference - current_q, voltage_limit, output_cosine_angle, output_sine_angle,
                                            &command_d, &command_q);
    if (current_pi_valid == 0U)
    {
        return;
    }

    command_alpha = command_d * output_cosine_angle - command_q * output_sine_angle;
#if SST_PHASE_A_ONLY_TEST
    /* PI 已根据 A 相试算饱和撤销向外积分；此处用修正后的 dq 重算真实 alpha=AN，再执行最终物理限幅。 */
    phase_a = command_alpha;
    if (phase_dc_a > 1.0F)
    {
        phase_a = Chb_Clamp(phase_a, -voltage_limit, voltage_limit);
        Chb_WriteNormalizedReferences(1.0F + phase_a / phase_dc_a, 1.0F, 1.0F);
    }
    else
    {
        Chb_WriteNeutralReferences();
    }
#else
    command_beta = command_d * output_sine_angle + command_q * output_cosine_angle;
    phase_a = command_alpha;
    phase_b = -0.5F * command_alpha + CHB_SQRT3_BY_TWO * command_beta;
    phase_c = -0.5F * command_alpha - CHB_SQRT3_BY_TWO * command_beta;

    if (phase_dc_a > 1.0F)
    {
        if (phase_dc_b > 1.0F)
        {
            if (phase_dc_c > 1.0F)
            {
                /* 按各相模块数进行电压限幅，再换算为FPGA Reference。 */
                phase_a = Chb_Clamp(phase_a,
                                    -g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_a,
                                    g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_a);
                phase_b = Chb_Clamp(phase_b,
                                    -g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_b,
                                    g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_b);
                phase_c = Chb_Clamp(phase_c,
                                    -g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_c,
                                    g_chb_dab_params.closed_loop_voltage_limit_factor * phase_dc_c);
                Chb_WriteNormalizedReferences(
                    1.0F + phase_a / phase_dc_a, 1.0F + phase_b / phase_dc_b, 1.0F + phase_c / phase_dc_c);
            }
            else
            {
                Chb_WriteNeutralReferences();
            }
        }
        else
        {
            Chb_WriteNeutralReferences();
        }
    }
    else
    {
        Chb_WriteNeutralReferences();
    }
#endif
}

/*==================================================================
 * 函数功能: 确认软件交流过流与直流过压连续样本并提交控制故障
 * 输入参数: 无
 * 输出参数: 连续超限计数；满足条件时经 ForceFault 进入故障并关闭 PWM。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
void Chb_CheckSoftwareProtection(void)
{
    float maximum_current; /* maximum_current：三相端口电流绝对值中的最大值，单位 A。 */
    float maximum_dc_voltage; /* maximum_dc_voltage：各相模块平均直流电压中的最大值，单位 V。 */
#if !SST_PHASE_A_ONLY_TEST
    float current_b_abs; /* current_b_abs：B相端口电流绝对值，单位 A。 */
    float current_c_abs; /* current_c_abs：C相端口电流绝对值，单位 A。 */
#endif

    /*
     * 这是新增的软件快速保护；硬件故障回传和封波能力取决于现场 FPGA 位流。
     * NoInterlock 位流不能按完整硬件保护版本解释；本软件检查的判据为：
     * - 过流：每TAD标定相电流绝对值最大值达到门槛并连续确认，非200点RMS；
     * - 过压：三相各自模块平均Udc的最大值达到门槛并连续确认；
     * - 确认次数用于滤除单个 TAD 的采样毛刺；原 HVAC RMS 保护在同一入口更新。
     */
    if (g_chb_dab_protection_enable == 0U)
    {
        /* 关闭软件保护时保留硬件故障链，不推进旧RMS计数或快速保护计数。 */
        s_ac_overcurrent_count = 0U;
        s_dc_overvoltage_count = 0U;
        return;
    }

    /* 更新旧有效值过流计数；直流过压统一由下方快速判据负责。 */
    if (HVAC_OC_Fault == 1U)
    {
        if (OC_OV_LV_PortLatch_TAD == 0U)
        {
            OC_OV_LV_FaultDispose();
            ChbDabControl_ForceFault(CHB_FAULT_HARDWARE);
            return;
        }

        OC_OV_LV_faultcheck();
        return;
    }
    /* 本拍推进原 RMS 计数，已确认故障由下一拍按原顺序消费。 */
    OC_OV_LV_faultcheck();
#if SST_PHASE_A_ONLY_TEST
    /* 快速保护直接检查物理 A 相采样，不使用有滤波延迟的 SOGI 电流。 */
    maximum_current = Chb_Abs(HA_Port_Current_A);
    maximum_dc_voltage = SVG_Udc_A_Ave;
#else
    current_b_abs = Chb_Abs(HA_Port_Current_B);
    current_c_abs = Chb_Abs(HA_Port_Current_C);
    maximum_current = Chb_Max3(Chb_Abs(HA_Port_Current_A), current_b_abs, current_c_abs);
    maximum_dc_voltage = Chb_Max3(SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave);
#endif
    if (maximum_current >= g_chb_dab_params.ac_overcurrent_trip_a)
    {
        if (s_ac_overcurrent_count < 0xFFFFU)
        {
            s_ac_overcurrent_count++;
        }
    }
    else
    {
        s_ac_overcurrent_count = 0U;
    }

    if (maximum_dc_voltage >= g_chb_dab_params.dc_overvoltage_trip_v)
    {
        if (s_dc_overvoltage_count < 0xFFFFU)
        {
            s_dc_overvoltage_count++;
        }
    }
    else
    {
        s_dc_overvoltage_count = 0U;
    }

    if (s_ac_overcurrent_count >= g_chb_dab_params.protection_confirm_samples)
    {
        /* 过流需连续确认若干个TAD，避免单点采样毛刺误停机。 */
        ChbDabControl_ForceFault(CHB_FAULT_AC_OVERCURRENT);
    }
    else if (s_dc_overvoltage_count >= g_chb_dab_params.protection_confirm_samples)
    {
        ChbDabControl_ForceFault(CHB_FAULT_DC_OVERVOLTAGE);
    }

}
