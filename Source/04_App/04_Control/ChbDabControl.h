#ifndef CHB_DAB_CONTROL_H_
#define CHB_DAB_CONTROL_H_

/*
 * FPGA501 module channel maps. Change these six masks when the installed
 * CHB module count changes. NUM and the modulation reference scale are
 * derived from the masks by Initial_Variable().
 */
/* CHB_PM_A1_MASK：A1 组在线模块位图，决定模块数及参考尺度。 */
#define CHB_PM_A1_MASK 0x0007U //0x007FU
/* CHB_PM_A2_MASK：A2 组在线模块位图，决定模块数及参考尺度。 */
#define CHB_PM_A2_MASK 0x0007U //0x003FU
/* CHB_PM_B1_MASK：B1 组在线模块位图，决定模块数及参考尺度。 */
#define CHB_PM_B1_MASK 0x0000U
/* CHB_PM_B2_MASK：B2 组在线模块位图，决定模块数及参考尺度。 */
#define CHB_PM_B2_MASK 0x0000U
/* CHB_PM_C1_MASK：C1 组在线模块位图，决定模块数及参考尺度。 */
#define CHB_PM_C1_MASK 0x0000U
/* CHB_PM_C2_MASK：C2 组在线模块位图，决定模块数及参考尺度。 */
#define CHB_PM_C2_MASK 0x0000U

/* CHB_AC_MODE_OPEN_LOOP：工频桥开环模式编号 0。 */
#define CHB_AC_MODE_OPEN_LOOP 0U
/* CHB_AC_MODE_CLOSED_LOOP：工频桥闭环模式编号 1。 */
#define CHB_AC_MODE_CLOSED_LOOP 1U

/* 闭环启动策略：0=兼容原版，1=固定Idref斜坡后再投入电压外环。 */
/* CHB_STARTUP_SOURCE_COMPATIBLE：原闭环启动兼容策略编号 0。 */
#define CHB_STARTUP_SOURCE_COMPATIBLE 0U
/* CHB_STARTUP_ID_RAMP：先固定 Id 斜坡再移交电压环策略编号 1。 */
#define CHB_STARTUP_ID_RAMP 1U

/* 闭环启动阶段，供上位机/调试器观察。 */
/* CHB_STARTUP_IDLE：闭环启动尚未进行阶段编号。 */
#define CHB_STARTUP_IDLE 0U
/* CHB_STARTUP_ID_RAMP_PHASE：闭环固定 Id 斜坡阶段编号。 */
#define CHB_STARTUP_ID_RAMP_PHASE 1U
/* CHB_STARTUP_VOLTAGE_HANDOVER：电压环给定平滑移交阶段编号。 */
#define CHB_STARTUP_VOLTAGE_HANDOVER 2U
/* CHB_STARTUP_NORMAL：闭环正常电压环运行阶段编号。 */
#define CHB_STARTUP_NORMAL 3U

typedef enum
{
    /* CHB/DAB 状态机的停止状态编号。 */ CHB_STATE_STOPPED = 0,
    /* CHB/DAB 状态机的预充状态编号。 */ CHB_STATE_PRECHARGE = 1,
    /* CHB/DAB 状态机的旁路等待状态编号。 */ CHB_STATE_BYPASS_DELAY = 2,
    /* CHB/DAB 状态机的等待 PLL状态编号。 */ CHB_STATE_WAIT_PLL = 3,
    /* CHB/DAB 状态机的运行状态编号。 */ CHB_STATE_RUNNING = 4,
    /* CHB/DAB 状态机的故障状态编号。 */ CHB_STATE_FAULT = 5
} CHB_DAB_STATE;

typedef enum
{
    /* 控制核心无故障编号。 */ CHB_FAULT_NONE = 0,
    /* 控制核心预充超时故障编号。 */ CHB_FAULT_PRECHARGE_TIMEOUT = 1,
    /* PLL 启动超时；单相、三相均用于闭环运行连续失锁/失压 20 ms。 */ CHB_FAULT_PLL_TIMEOUT = 2,
    /* 控制核心交流过流故障编号。 */ CHB_FAULT_AC_OVERCURRENT = 3,
    /* 控制核心直流过压故障编号。 */ CHB_FAULT_DC_OVERVOLTAGE = 4,
    /* 控制核心硬件故障编号。 */ CHB_FAULT_HARDWARE = 5,
    /* 配置非法；包括带启动命令切模式、非有限数值及运行低母线。 */ CHB_FAULT_BAD_CONFIGURATION = 6
} CHB_DAB_FAULT;

/* Direct debugger/upper-computer commands. All commands are level signals. */
typedef struct
{
    unsigned short run_enable;
    /* run_enable：CHB 启动命令，0 停止、1 启动；开环直接解锁，闭环走预充流程。 */ /* 闭环：启动预充/运行；开环：直接解锁CHB PWM */
    unsigned short dab_enable;
    /* dab_enable：DAB 原边使能命令，0 停止、1 启动；开环可独立控制。 */ /* DAB原边使能；开环下可独立于run_enable */
    unsigned short ac_mode; /* ac_mode：工频桥模式，0 开环逆变、1 闭环运行；单相、三相均须停机后切换。 */
    unsigned short fault_reset;
    /* fault_reset：停机时置 1 请求复位控制故障，控制核心消费后清零。 */ /* Set to 1 while stopped; auto-clears */
    float open_loop_modulation;
    /* open_loop_modulation：开环调制比，范围 0..modulation_max。 */ /* 0.0 ... modulation_max */
    float open_loop_frequency_hz; /* open_loop_frequency_hz：开环正弦输出频率，单位 Hz。 */
    float dc_voltage_ref_v; /* dc_voltage_ref_v：单模块直流电压目标，单位 V。 */ /* Per-module DC voltage reference */
    float iq_reference_a; /* iq_reference_a：闭环 q 轴电流目标，单位 A。 */
} CHB_DAB_COMMAND;

/* Runtime-tunable values; debugger writes take effect without rebuilding. */
typedef struct
{
    float sample_period_s;
    /* sample_period_s：TAD 计算周期，单位 s，默认 0.0001；通信只读，不改变硬件中断频率。 */ /* TAD control interrupt period */
    float modulation_max; /* modulation_max：各活动相调制比绝对值上限，默认 0.98。 */

    float precharge_voltage_v;
    /* precharge_voltage_v：三相时取各相模块均压最小值，单相时取 A 相均压的预充阈值，单位 V。 */
    unsigned short precharge_stable_10ms; /* precharge_stable_10ms：预充电压须持续达标的 10 ms 拍数。 */
    unsigned short precharge_timeout_10ms; /* precharge_timeout_10ms：预充允许的最大等待时间，单位 10 ms。 */
    unsigned short bypass_delay_10ms; /* bypass_delay_10ms：投入预充旁路后的等待时间，单位 10 ms。 */
    unsigned short breaker_pulse_10ms; /* breaker_pulse_10ms：交流输入断路器合/分闸脉冲宽度，单位 10 ms。 */

    float pll_nominal_frequency_hz; /* pll_nominal_frequency_hz：PLL 额定电网频率，单位 Hz。 */
    float pll_kp; /* pll_kp：PLL 归一化 q 轴误差比例系数。 */
    float pll_ki; /* pll_ki：PLL 归一化 q 轴误差积分系数，积分乘 sample_period_s。 */
    float pll_correction_limit_rad_s; /* pll_correction_limit_rad_s：PLL 角速度修正绝对值上限，单位 rad/s。 */
    float pll_lock_error_pu; /* pll_lock_error_pu：PLL 锁定允许的归一化 q 轴误差幅值。 */
    float pll_min_grid_peak_v; /* pll_min_grid_peak_v：PLL 有效电网电压矢量幅值下限，单位 V。 */
    unsigned short pll_lock_10ms; /* pll_lock_10ms：PLL 连续锁定确认时间，单位 10 ms。 */
    unsigned short pll_timeout_10ms; /* pll_timeout_10ms：PLL 等待锁定超时，单位 10 ms。 */

    float voltage_kp;
    /* voltage_kp：直流电压外环比例系数，沿用原增量 PI 等效值。 */ /* Converted from source incremental PI */
    float voltage_ki; /* voltage_ki：直流电压外环积分系数，积分乘 sample_period_s。 */
    float voltage_current_limit_a; /* voltage_current_limit_a：电压外环输出 Id 的绝对值上限，单位 A。 */

    unsigned short startup_strategy;
    /* startup_strategy：闭环启动策略，0 原版兼容、1 固定 Id 斜坡后移交电压环。 */ /* 0：兼容原版；1：Idref斜坡后接管 */
    float startup_udc_offset_v;
    /* startup_udc_offset_v：PWM 解锁时叠加在实测电压上的给定偏移，单位 V。 */ /* PWM解锁时当前电压上的初始偏移 */
    float startup_udc_step_v;
    /* startup_udc_step_v：电压环接管后每个 TAD 的电压给定步长，单位 V。 */ /* 电压环接管后的电压给定步长 */
    float startup_id_reference_a;
    /* startup_id_reference_a：启动阶段固定 Id 目标/状态镜像，单位 A。 */ /* 启动阶段固定Id目标 */
    float startup_id_ramp_step_a;
    /* startup_id_ramp_step_a：启动阶段每个 TAD 的 Id 斜坡步长，单位 A。 */ /* 每个TAD的Id给定斜坡步长 */
    unsigned short startup_ramp_tad;
    /* startup_ramp_tad：固定 Id 启动斜坡最长持续拍数，单位 TAD。 */ /* 启动斜坡最长TAD数 */

    float current_kp;
    /* current_kp：dq 电流内环比例系数，保留原反馈方向对应的符号。 */ /* Signed gain: source control direction */
    float current_ki; /* current_ki：dq 电流内环积分系数，积分乘 sample_period_s。 */
    float closed_loop_voltage_limit_factor;
    /* closed_loop_voltage_limit_factor：电压指令相对总母线的限幅比例；单相与 modulation_max 取小者作 dq 矢量限幅。 */
    float current_feedback_sign; /* current_feedback_sign：电流采样方向系数，只允许 +1 或 -1。 */
    float current_phase_compensation_rad; /* current_phase_compensation_rad：电流坐标变换相位补偿，单位 rad。 */
    float output_phase_compensation_rad; /* output_phase_compensation_rad：输出电压坐标逆变换相位补偿，单位 rad。 */

    float ac_overcurrent_trip_a; /* ac_overcurrent_trip_a：快速过流阈值，比较每TAD活动相原始标定电流绝对值，单位A。 */
    float
        dc_overvoltage_trip_v; /* dc_overvoltage_trip_v：快速过压阈值，三相取各相模块均压最大值，单相只取 A 相，单位V。 */
    unsigned short protection_confirm_samples; /* protection_confirm_samples：新增软件保护所需的连续超限 TAD 样本数。 */

    unsigned short dab_start_duty_count; /* dab_start_duty_count：DAB 使能时初始占空比计数，默认 350。 */
    unsigned short dab_target_duty_count; /* dab_target_duty_count：DAB 斜坡目标占空比计数，上限 2500。 */
    unsigned short dab_ramp_step_count; /* dab_ramp_step_count：每个 TAD 的 DAB 占空比递增计数，默认 1。 */
    unsigned short dab_start_count; /* dab_start_count：DAB PWM 起始相位计数，范围 0..4999。 */
} CHB_DAB_PARAMETERS;

typedef struct
{
    unsigned short state; /* state：CHB_STATE_* 运行状态编号。 */
    unsigned short fault_code; /* fault_code：CHB_FAULT_* 控制故障编号，区别于五位上报故障码。 */
    unsigned short ac_input_closed; /* ac_input_closed：交流输入闭合状态镜像，非零表示闭合。 */
    unsigned short precharge_bypassed; /* precharge_bypassed：预充旁路状态镜像，非零表示已旁路。 */
    unsigned short ac_pwm_enabled; /* ac_pwm_enabled：CHB 工频桥 PWM 使能状态镜像。 */
    unsigned short dab_pwm_enabled; /* dab_pwm_enabled：DAB PWM 使能状态镜像。 */
    unsigned short pll_locked; /* pll_locked：PLL 连续锁定已确认标志。 */
    unsigned short dab_duty_count; /* dab_duty_count：当前实际下发 DAB 占空比计数。 */
    unsigned long tick_10ms; /* tick_10ms：10 ms 软件节拍；故障记录中为后台观察时刻，不是精确跳闸时间。 */
    float dc_voltage_average_v; /* dc_voltage_average_v：三相各相模块均压的平均值，单相直接为 A 相模块均压，单位 V。 */
    float dc_voltage_min_v; /* dc_voltage_min_v：三相各相模块均压最小值，单相直接为 A 相模块均压，单位 V。 */
    float pll_angle_rad; /* pll_angle_rad：PLL 当前电网相角，单位 rad。 */
    float pll_frequency_hz; /* pll_frequency_hz：PLL 当前估计电网频率，单位 Hz。 */
    float id_feedback_a; /* id_feedback_a：闭环 d 轴电流反馈，单位 A。 */
    float iq_feedback_a; /* iq_feedback_a：闭环 q 轴电流反馈，单位 A。 */
    float id_reference_a; /* id_reference_a：闭环当前 d 轴电流给定，单位 A。 */
    float startup_id_reference_a; /* startup_id_reference_a：启动阶段固定 Id 目标/状态镜像，单位 A。 */
    float startup_voltage_reference_v; /* startup_voltage_reference_v：启动过程中的电压外环给定，单位 V。 */
    unsigned short startup_phase; /* startup_phase：CHB_STARTUP_* 启动阶段编号。 */
    unsigned long startup_tad_count; /* startup_tad_count：启动斜坡已执行的 TAD 拍数。 */
    unsigned short voltage_loop_active; /* voltage_loop_active：电压外环已接管 Id 给定的标志。 */
    float modulation_actual; /* modulation_actual：当前各活动相调制指令的最大绝对值，单相只计 A 相。 */
} CHB_DAB_STATUS;

extern volatile CHB_DAB_COMMAND
    g_chb_dab_command; /* g_chb_dab_command：调试器/通信提交的控制命令与给定，控制核心读取执行。 */
extern volatile CHB_DAB_PARAMETERS
    g_chb_dab_params; /* g_chb_dab_params：CHB/DAB 控制参数；调试器可观察，通信写入需停机校验。 */
extern volatile CHB_DAB_STATUS
    g_chb_dab_status; /* g_chb_dab_status：控制核心维护的运行状态镜像，后台仅用于读取上报。 */
/* 仿真器可修改的快速软件过流/过压检查开关，不代表现场硬件保护状态。 */
extern volatile unsigned short
    g_chb_dab_protection_enable; /* g_chb_dab_protection_enable：新增软件过流/过压检查使能，0 关闭、1 开启；硬件保护能力取决于现场 FPGA 位流。 */

/*==================================================================
 * 函数功能: 清空控制状态，撤销 PWM 使能并初始化开关及兼容状态
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
extern void ChbDabControl_Init(void);
/*==================================================================
 * 函数功能: 递增控制状态的 10 ms 节拍
 * 输入参数: 无
 * 输出参数: g_chb_dab_status.tick_10ms。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
extern void ChbDabControl_Tick10ms(void);
/*==================================================================
 * 函数功能: 每个 10 ms 节拍推进预充、旁路、PLL 等待、运行和停机状态
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
extern void ChbDabControl_Service(void);
/*==================================================================
 * 函数功能: 在原 TAD 路径执行 PLL、开环/闭环控制、软件保护及 DAB 占空比斜坡
 * 输入参数: 无
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
extern void ChbDabControl_FastStep(void);
/*==================================================================
 * 函数功能: 按停止优先原则处理遥控启停字，受宏及存储互锁约束
 * 输入参数: start_word：原遥控启动字，按控制核心的端口位解释
 *           stop_word：原遥控停止字，停止优先于启动
 * 输出参数: g_chb_dab_command.run_enable、dab_enable；只提交命令。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
extern void ChbDabControl_ApplyRemoteCommand(unsigned short start_word, unsigned short stop_word);
/*==================================================================
 * 函数功能: 锁存控制故障并关闭 PWM，进入故障状态
 * 输入参数: fault_code：控制核心 CHB_FAULT_* 故障编号
 * 输出参数: CHB/DAB 命令、参数派生状态、PI 历史量以及原 FPGA 参考/使能寄存器。
 * 返回值: 无
 * 执行说明: 控制核心；FastStep 及其计算子函数属于原 100 µs TAD 路径，Service 属于主循环。
 *================================================================*/
extern void ChbDabControl_ForceFault(unsigned short fault_code);

#endif
