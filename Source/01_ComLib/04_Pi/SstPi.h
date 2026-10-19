/*======================================================================================*
 * 文件名称: SstPi.h
 * 文件功能: SST 控制器用位置式 PI 公共接口
 *
 * 设计依据:
 *   1. 参考 F28377S_Plat/Source/01_ComLib/04_Pid 的显式 Ref/Fdb/Err/Up/Ui/Out 结构；
 *   2. 参考 BidirecConverter/pi_reg.c 的离散增量实现，Ki 在本模块中按 Ki*Ts 使用；
 *   3. 支持 CHB 单轴及 dq 双轴控制，所有函数均为无动态内存、无关中断实现。
 *
 * 说明:
 *   - Kp、Ki 的符号由控制方向决定，允许 Ki 为负值；
 *   - Run/RunDq 在同一次调用中完成试算、抗饱和提交和输出限幅；
 *   - Configure 可以在停机或每拍更新边界，集成层应保证参数写入具有一致性。
 *=====================================================================================*/
#ifndef SST_PI_H_
#define SST_PI_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* SST_PI_SUCCESS：本次配置、跟踪或计算成功。 */
#define SST_PI_SUCCESS ((unsigned short)1U)
/* SST_PI_FAILURE：本次配置、输入或数值计算无效。 */
#define SST_PI_FAILURE ((unsigned short)0U)

/*
 * flags：公开给 CCS、VOFA 和上层故障判断的运行标志。
 * INVALID_* 和 NUMERIC_ERROR 为错误标志；错误发生时 VALID 被清除且输出置零。
 */
#define SST_PI_FLAG_VALID             ((unsigned short)0x0001U) /* 控制器配置和本拍输入均有效。 */
/* SST_PI_FLAG_SATURATED：本拍试算或最终输出触及输出限幅。 */
#define SST_PI_FLAG_SATURATED         ((unsigned short)0x0002U)
/* SST_PI_FLAG_INTEGRAL_HELD：本拍为阻止饱和加深而保持积分状态。 */
#define SST_PI_FLAG_INTEGRAL_HELD     ((unsigned short)0x0004U)
/* SST_PI_FLAG_INTEGRAL_CLAMPED：积分试算或配置更新触及积分硬边界。 */
#define SST_PI_FLAG_INTEGRAL_CLAMPED  ((unsigned short)0x0008U)
/* SST_PI_FLAG_TRACKING：本拍通过指定输出反算积分，供环路无冲击接管。 */
#define SST_PI_FLAG_TRACKING          ((unsigned short)0x0010U)
/* SST_PI_FLAG_VECTOR_LIMIT：本拍触及 dq 电压矢量幅值边界。 */
#define SST_PI_FLAG_VECTOR_LIMIT      ((unsigned short)0x0020U)
/* SST_PI_FLAG_INVALID_PARAMETER：增益、周期或限幅配置非法。 */
#define SST_PI_FLAG_INVALID_PARAMETER ((unsigned short)0x0100U)
/* SST_PI_FLAG_INVALID_INPUT：给定、反馈或前馈出现非有限数。 */
#define SST_PI_FLAG_INVALID_INPUT     ((unsigned short)0x0200U)
/* SST_PI_FLAG_NUMERIC_ERROR：有效输入的计算中间量出现溢出或非有限数。 */
#define SST_PI_FLAG_NUMERIC_ERROR     ((unsigned short)0x0400U)

typedef struct
{
    /* 输入量和配置参数：单位由调用者定义，但同一控制器内必须一致。 */
    float reference;              /* 本拍给定值。 */
    float feedback;               /* 本拍反馈值。 */
    float error;                  /* 控制误差，等于 reference - feedback。 */
    float kp;                     /* 比例系数，允许正负。 */
    float ki;                     /* 连续域积分系数，允许正负。 */
    float sample_period_s;        /* 采样周期 Ts，单位 s。 */
    float feedforward;             /* 本拍前馈量。 */

    /* 运行中间量和输出：保留用于 CCS/VOFA 观察。 */
    float proportional;            /* 比例项，等于 kp * error。 */
    float integral_increment;      /* 本拍积分增量，等于 ki * Ts * error。 */
    float integral;                /* 积分状态。 */
    float output_unsaturated;      /* 未限幅输出，等于前馈、比例项和积分项之和。 */
    float output;                  /* 经过限幅的最终输出。 */
    float saturation_error;        /* 饱和偏差，等于最终输出减未限幅输出。 */

    /* 输出及积分状态边界，可由上层每拍配置。 */
    float output_min;              /* 最终输出下限，与 output 同单位。 */
    float output_max;              /* 最终输出上限，与 output 同单位。 */
    float integral_min;            /* 积分状态下限，与 integral 同单位。 */
    float integral_max;            /* 积分状态上限，与 integral 同单位。 */

    /* 1 时启用条件积分抗饱和；集成层的 CHB 三相和单相均应置1。 */
    unsigned short anti_windup_enable;
    unsigned short flags;          /* 本拍有效性、限幅、积分保持及数值错误位标志。 */
} SST_PI_CONTROLLER;

/*==================================================================
 * 函数功能: 清零 PI 对象并建立未配置状态。
 * 输入参数: controller：待初始化的控制器对象指针，空指针直接返回。
 * 输出参数: controller：参数、历史状态和运行观察量全部清零。
 *================================================================*/
void SstPi_Init(SST_PI_CONTROLLER *controller);

/*==================================================================
 * 函数功能: 更新 PI 参数和边界，保留历史积分并按新边界钳位。
 * 输入参数: controller：待配置的控制器对象指针。
 *           kp：比例系数，允许正负。
 *           ki：连续域积分系数，本模块计算时只乘一次采样周期。
 *           sample_period_s：控制采样周期，单位 s，必须大于零。
 *           output_min：最终输出下限。
 *           output_max：最终输出上限。
 *           integral_min：积分状态硬下限。
 *           integral_max：积分状态硬上限。
 *           anti_windup_enable：条件积分抗饱和开关，非零启用。
 * 返回值: 1 表示配置有效，0 表示参数或原积分状态非法。
 *================================================================*/
unsigned short SstPi_Configure(
    SST_PI_CONTROLLER *controller,
    float kp,
    float ki,
    float sample_period_s,
    float output_min,
    float output_max,
    float integral_min,
    float integral_max,
    unsigned short anti_windup_enable);

/*==================================================================
 * 函数功能: 清除历史积分和本拍运行量，保留增益及限幅配置。
 * 输入参数: controller：待复位的控制器对象指针，空指针直接返回。
 *================================================================*/
void SstPi_Reset(SST_PI_CONTROLLER *controller);

/*==================================================================
 * 函数功能: 根据已建立的前馈和比例项反算积分，供控制环无冲击接管。
 * 输入参数: controller：已配置且已建立本拍比例项和前馈的控制器对象。
 *           tracked_output：期望接管时跟踪的输出，与输出同单位。
 * 返回值: 1 表示跟踪计算有效，0 表示参数、输入或数值计算非法。
 *================================================================*/
unsigned short SstPi_Track(SST_PI_CONTROLLER *controller, float tracked_output);

/*==================================================================
 * 函数功能: 执行单轴 PI，前馈参与同拍限幅及条件积分抗饱和。
 * 输入参数: controller：已配置的 PI 对象，保存本拍计算结果。
 *           reference：本拍给定值，与反馈同单位。
 *           feedback：本拍反馈值，与给定同单位。
 *           feedforward：本拍输出前馈，与最终输出同单位。
 * 返回值: 1 表示计算有效，0 表示参数、输入或数值计算非法。
 *================================================================*/
unsigned short SstPi_Run(
    SST_PI_CONTROLLER *controller,
    float reference,
    float feedback,
    float feedforward);

/*==================================================================
 * 函数功能: 执行 dq PI，并将电网前馈纳入同拍矢量限幅和抗饱和。
 * 输入参数: d_controller：d 轴 PI 对象，output 接收最终 d 轴电压。
 *           q_controller：q 轴 PI 对象，output 接收最终 q 轴电压。
 *           reference_d：d 轴电流给定，单位 A。
 *           feedback_d：d 轴电流反馈，单位 A。
 *           feedforward_d：d 轴电压前馈，单位 V。
 *           reference_q：q 轴电流给定，单位 A。
 *           feedback_q：q 轴电流反馈，单位 A。
 *           feedforward_q：q 轴电压前馈，单位 V。
 *           voltage_limit：动态电压矢量幅值上限，单位 V；零值合法。
 * 返回值: 1 表示两轴计算有效，0 表示参数、输入或数值计算非法。
 *================================================================*/
unsigned short SstPi_RunDq(
    SST_PI_CONTROLLER *d_controller,
    SST_PI_CONTROLLER *q_controller,
    float reference_d,
    float feedback_d,
    float feedforward_d,
    float reference_q,
    float feedback_q,
    float feedforward_q,
    float voltage_limit);

#ifdef __cplusplus
}
#endif

#endif /* SST_PI_H_ */
