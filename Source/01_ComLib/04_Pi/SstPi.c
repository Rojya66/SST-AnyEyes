/*======================================================================================*
 * 文件名称: SstPi.c
 * 文件功能: SST 位置式 PI、条件积分抗饱和及 dq 矢量限幅实现
 *
 * 执行约束: 无 malloc、无循环、无中断屏蔽；适合 10 kHz 快速控制路径。
 *=====================================================================================*/
#include "SstPi.h"

#include <float.h>
#include <math.h>

/*==================================================================
 * 函数功能: 判断浮点值是否为有限数，避免依赖目标编译器的 isfinite 宏。
 * 输入参数: value：待检查的单精度浮点值。
 * 返回值: 1 表示有限数，0 表示 NaN 或无穷大。
 *================================================================*/
static unsigned short SstPi_IsFinite(float value)
{
    return (unsigned short)((value == value) && (value <= FLT_MAX) && (value >= -FLT_MAX));
}

/*==================================================================
 * 函数功能: 对浮点值执行上下限约束。
 * 输入参数: value：待限幅的浮点值；minimum：下限；maximum：上限。
 * 返回值: 位于指定区间内的限幅结果。
 *================================================================*/
static float SstPi_Clamp(float value, float minimum, float maximum)
{
    if (value > maximum)
    {
        return maximum;
    }
    if (value < minimum)
    {
        return minimum;
    }
    return value;
}

/*==================================================================
 * 函数功能: 清除错误输入产生的历史和输出，避免继续使用失效控制量。
 * 输入参数: controller：待清除运行状态的 PI 对象指针。
 * 输出参数: controller：给定、反馈、积分、输出及饱和偏差清零。
 *================================================================*/
static void SstPi_ClearRuntime(SST_PI_CONTROLLER *controller)
{
    controller->reference = 0.0F;
    controller->feedback = 0.0F;
    controller->error = 0.0F;
    controller->feedforward = 0.0F;
    controller->proportional = 0.0F;
    controller->integral_increment = 0.0F;
    controller->integral = 0.0F;
    controller->output_unsaturated = 0.0F;
    controller->output = 0.0F;
    controller->saturation_error = 0.0F;
}

/*==================================================================
 * 函数功能: 检查 PI 增益、采样周期和限幅参数是否有效。
 * 输入参数: controller：待检查的控制器对象指针。
 * 返回值: 1 表示配置有效，0 表示空指针、非有限值或边界反序。
 *================================================================*/
static unsigned short SstPi_ConfigurationValid(const SST_PI_CONTROLLER *controller)
{
    return (unsigned short)(
        (controller != (SST_PI_CONTROLLER *)0) &&
        SstPi_IsFinite(controller->kp) &&
        SstPi_IsFinite(controller->ki) &&
        SstPi_IsFinite(controller->sample_period_s) &&
        (controller->sample_period_s > 0.0F) &&
        SstPi_IsFinite(controller->output_min) &&
        SstPi_IsFinite(controller->output_max) &&
        (controller->output_min <= controller->output_max) &&
        SstPi_IsFinite(controller->integral_min) &&
        SstPi_IsFinite(controller->integral_max) &&
        (controller->integral_min <= controller->integral_max));
}

/*==================================================================
 * 函数功能: 写入指定错误标志并将控制器运行输出置零。
 * 输入参数: controller：待关闭输出的控制器对象指针。
 *           error_flag：本次失败的错误标志。
 * 返回值: 始终返回 SST_PI_FAILURE；空指针时不写内存。
 *================================================================*/
static unsigned short SstPi_Reject(SST_PI_CONTROLLER *controller, unsigned short error_flag)
{
    if (controller == (SST_PI_CONTROLLER *)0)
    {
        return SST_PI_FAILURE;
    }
    SstPi_ClearRuntime(controller);
    controller->flags = error_flag;
    return SST_PI_FAILURE;
}

/*==================================================================
 * 函数功能: 初始化 PI 对象，清零配置、积分历史和运行观察量。
 * 输入参数: controller：待初始化的 PI 对象指针，空指针时直接返回。
 * 输出参数: controller：写入默认配置、清零积分及本拍运行量。
 * 返回值: 无。
 *================================================================*/
void SstPi_Init(SST_PI_CONTROLLER *controller)
{
    if (controller == (SST_PI_CONTROLLER *)0)
    {
        return;
    }

    controller->reference = 0.0F;
    controller->feedback = 0.0F;
    controller->error = 0.0F;
    controller->kp = 0.0F;
    controller->ki = 0.0F;
    controller->sample_period_s = 0.0F;
    controller->feedforward = 0.0F;
    controller->proportional = 0.0F;
    controller->integral_increment = 0.0F;
    controller->integral = 0.0F;
    controller->output_unsaturated = 0.0F;
    controller->output = 0.0F;
    controller->saturation_error = 0.0F;
    controller->output_min = 0.0F;
    controller->output_max = 0.0F;
    controller->integral_min = 0.0F;
    controller->integral_max = 0.0F;
    controller->anti_windup_enable = 1U;
    controller->flags = 0U;
    controller->configuration_valid = 0U;
}

/*==================================================================
 * 函数功能: 配置 PI 参数和边界，保留已有积分并按新边界钳位。
 * 输入参数: controller：待配置的 PI 对象指针。
 *           kp：比例系数，允许正负。
 *           ki：连续域积分系数，执行时与采样周期相乘。
 *           sample_period_s：采样周期，单位 s，必须大于零。
 *           output_min：最终输出下限。
 *           output_max：最终输出上限。
 *           integral_min：积分状态下限。
 *           integral_max：积分状态上限。
 *           anti_windup_enable：条件积分抗饱和开关，非零启用。
 * 返回值: 1 表示配置成功，0 表示对象、参数或历史积分非法。
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
    unsigned short anti_windup_enable)
{
    if (controller == (SST_PI_CONTROLLER *)0)
    {
        return SST_PI_FAILURE;
    }

    controller->kp = kp;
    controller->ki = ki;
    controller->sample_period_s = sample_period_s;
    controller->output_min = output_min;
    controller->output_max = output_max;
    controller->integral_min = integral_min;
    controller->integral_max = integral_max;
    controller->anti_windup_enable = (anti_windup_enable != 0U) ? 1U : 0U;

    if (!SstPi_ConfigurationValid(controller))
    {
        SstPi_ClearRuntime(controller);
        controller->flags = SST_PI_FLAG_INVALID_PARAMETER;
        controller->configuration_valid = 0U;
        return SST_PI_FAILURE;
    }

    /* 配置保留已有积分；每拍动态边界改由 SstPi_UpdateBounds 更新。 */
    if (!SstPi_IsFinite(controller->integral))
    {
        controller->configuration_valid = 0U;
        return SstPi_Reject(controller, SST_PI_FLAG_NUMERIC_ERROR);
    }
    controller->flags = SST_PI_FLAG_VALID;
    controller->configuration_valid = 1U;
    if (controller->integral < integral_min || controller->integral > integral_max)
    {
        controller->integral = SstPi_Clamp(controller->integral, integral_min, integral_max);
        controller->flags |= SST_PI_FLAG_INTEGRAL_CLAMPED;
    }
    return SST_PI_SUCCESS;
}

/*==================================================================
 * 函数功能: 更新已经校验过的 PI 输出和积分边界
 * 输入参数: controller：已完成参数配置的 PI 对象。
 *           output_min、output_max：新的输出上下限；integral_min、integral_max：新的积分上下限。
 * 输出参数: controller：边界及必要的积分钳位结果。
 * 返回值: 无。
 * 执行说明: 快速路径只更新动态边界，不重复执行配置有效性检查。
 *==================================================================*/
void SstPi_UpdateBounds(
    SST_PI_CONTROLLER *controller,
    float output_min,
    float output_max,
    float integral_min,
    float integral_max)
{
    if (controller == (SST_PI_CONTROLLER *)0)
    {
        return;
    }

    controller->output_min = output_min;
    controller->output_max = output_max;
    controller->integral_min = integral_min;
    controller->integral_max = integral_max;
    controller->integral = SstPi_Clamp(controller->integral, integral_min, integral_max);
}

/*==================================================================
 * 函数功能: 将最终物理输出的饱和偏差反馈给单轴 PI，撤销向外积分。
 * 输入参数: controller：本拍已执行的 PI 对象；previous_integral：本拍计算前的积分。
 *           saturation_error：最终限幅值减原指令，再投影到本轴的电压偏差。
 * 输出参数: controller：必要时恢复积分，重算输出并更新限幅/积分保持标志。
 * 返回值: 无。
 * 执行说明: 不重复校验配置；仅撤销把物理输出进一步推向饱和的积分。
 *==================================================================*/
void SstPi_ApplySaturation(SST_PI_CONTROLLER *controller, float previous_integral, float saturation_error)
{
    if (controller == (SST_PI_CONTROLLER *)0 || saturation_error == 0.0F)
    {
        return;
    }
    controller->flags |= SST_PI_FLAG_SATURATED;
    if (controller->anti_windup_enable != 0U &&
        saturation_error * (controller->integral - previous_integral) < 0.0F)
    {
        controller->integral = previous_integral;
        controller->output_unsaturated = controller->feedforward + controller->proportional + controller->integral;
        controller->output = SstPi_Clamp(controller->output_unsaturated, controller->output_min, controller->output_max);
        controller->flags |= SST_PI_FLAG_INTEGRAL_HELD;
    }
    controller->saturation_error = saturation_error;
}

/*==================================================================
 * 函数功能: 清除 PI 历史积分和本拍运行量，保留增益及限幅配置。
 * 输入参数: controller：待复位的 PI 对象指针，空指针时直接返回。
 * 输出参数: controller：积分、输入、输出及饱和误差清零。
 * 返回值: 无。
 *================================================================*/
void SstPi_Reset(SST_PI_CONTROLLER *controller)
{
    if (controller == (SST_PI_CONTROLLER *)0)
    {
        return;
    }
    controller->reference = 0.0F;
    controller->feedback = 0.0F;
    controller->error = 0.0F;
    controller->feedforward = 0.0F;
    controller->proportional = 0.0F;
    controller->integral_increment = 0.0F;
    controller->integral = 0.0F;
    controller->output_unsaturated = 0.0F;
    controller->output = 0.0F;
    controller->saturation_error = 0.0F;
    controller->configuration_valid = SstPi_ConfigurationValid(controller);
    controller->flags = controller->configuration_valid ? SST_PI_FLAG_VALID : SST_PI_FLAG_INVALID_PARAMETER;
}

/*==================================================================
 * 函数功能: 根据已建立的前馈和比例项反算积分，供控制环无冲击接管。
 * 输入参数: controller：已配置且已建立本拍比例项和前馈的 PI 对象。
 *           tracked_output：期望接管时跟踪的输出，与输出同单位。
 * 返回值: 1 表示跟踪计算成功，0 表示参数、输入或数值计算非法。
 *================================================================*/
unsigned short SstPi_Track(SST_PI_CONTROLLER *controller, float tracked_output)
{
    float target_integral; /* target_integral：按跟踪输出反算得到的候选积分状态。 */

    if (controller == (SST_PI_CONTROLLER *)0 || controller->configuration_valid == 0U)
    {
        return SstPi_Reject(controller, SST_PI_FLAG_INVALID_PARAMETER);
    }
    if (!SstPi_IsFinite(tracked_output) || !SstPi_IsFinite(controller->feedforward) ||
        !SstPi_IsFinite(controller->proportional))
    {
        return SstPi_Reject(controller, SST_PI_FLAG_INVALID_INPUT);
    }

    controller->output = SstPi_Clamp(tracked_output, controller->output_min, controller->output_max);
    target_integral = controller->output - controller->feedforward - controller->proportional;
    if (!SstPi_IsFinite(target_integral))
    {
        return SstPi_Reject(controller, SST_PI_FLAG_NUMERIC_ERROR);
    }
    controller->integral = SstPi_Clamp(target_integral, controller->integral_min, controller->integral_max);
    controller->output_unsaturated = controller->feedforward + controller->proportional + controller->integral;
    if (!SstPi_IsFinite(controller->output_unsaturated))
    {
        return SstPi_Reject(controller, SST_PI_FLAG_NUMERIC_ERROR);
    }
    controller->output = SstPi_Clamp(controller->output_unsaturated, controller->output_min, controller->output_max);
    controller->saturation_error = controller->output - controller->output_unsaturated;
    controller->flags = SST_PI_FLAG_VALID | SST_PI_FLAG_TRACKING;
    if (controller->output != tracked_output || controller->integral != target_integral)
    {
        controller->flags |= SST_PI_FLAG_SATURATED | SST_PI_FLAG_INTEGRAL_CLAMPED;
    }
    return SST_PI_SUCCESS;
}

/*==================================================================
 * 函数功能: 计算一个控制器的试算积分、比例项和未限幅候选输出。
 * 输入参数: controller：待计算的 PI 对象；reference：本拍给定；feedback：本拍反馈；
 *           feedforward：本拍前馈；candidate_integral：候选积分输出指针；
 *           base_output：比例与前馈和输出指针；candidate_output：候选总输出指针。
 * 输出参数: controller：更新本拍输入、误差、比例和积分增量；三个指针接收试算结果。
 * 返回值: 无；本函数不提交积分、不执行最终输出限幅。
 *================================================================*/
static void SstPi_Predict(
    SST_PI_CONTROLLER *controller,
    float reference,
    float feedback,
    float feedforward,
    float *candidate_integral,
    float *base_output,
    float *candidate_output)
{
    controller->reference = reference;
    controller->feedback = feedback;
    controller->feedforward = feedforward;
    controller->error = reference - feedback;
    controller->proportional = controller->kp * controller->error;
    controller->integral_increment = controller->ki * controller->sample_period_s * controller->error;
    *candidate_integral = SstPi_Clamp(
        controller->integral + controller->integral_increment,
        controller->integral_min,
        controller->integral_max);
    *base_output = feedforward + controller->proportional;
    *candidate_output = *base_output + *candidate_integral;
}

/*==================================================================
 * 函数功能: 执行单轴 PI，完成试算、条件积分抗饱和和输出限幅。
 * 输入参数: controller：已配置的 PI 对象，保存本拍计算结果。
 *           reference：本拍给定值，与反馈同单位。
 *           feedback：本拍反馈值，与给定同单位。
 *           feedforward：本拍输出前馈，与最终输出同单位。
 * 返回值: 1 表示计算成功，0 表示参数、输入或数值计算非法。
 *================================================================*/
unsigned short SstPi_Run(
    SST_PI_CONTROLLER *controller,
    float reference,
    float feedback,
    float feedforward)
{
    float candidate_integral; /* candidate_integral：本拍试算积分状态。 */
    float base_output; /* base_output：前馈加比例项的输出。 */
    float candidate_output; /* candidate_output：加入候选积分的未限幅输出。 */
    float candidate_limited; /* candidate_limited：按输出边界限幅后的候选输出。 */
    unsigned short flags = SST_PI_FLAG_VALID; /* flags：本拍输出及积分限幅状态。 */
    unsigned short hold_integral = 0U; /* hold_integral：本拍是否冻结积分。 */

    if (controller == (SST_PI_CONTROLLER *)0 || controller->configuration_valid == 0U)
    {
        return SstPi_Reject(controller, SST_PI_FLAG_INVALID_PARAMETER);
    }
    if (!SstPi_IsFinite(reference) || !SstPi_IsFinite(feedback) || !SstPi_IsFinite(feedforward))
    {
        return SstPi_Reject(controller, SST_PI_FLAG_INVALID_INPUT);
    }

    SstPi_Predict(controller, reference, feedback, feedforward,
                  &candidate_integral, &base_output, &candidate_output);
    if (!SstPi_IsFinite(controller->error) || !SstPi_IsFinite(controller->proportional) ||
        !SstPi_IsFinite(controller->integral_increment) || !SstPi_IsFinite(candidate_output))
    {
        return SstPi_Reject(controller, SST_PI_FLAG_NUMERIC_ERROR);
    }
    candidate_limited = SstPi_Clamp(candidate_output, controller->output_min, controller->output_max);
    if (candidate_limited != candidate_output)
    {
        flags |= SST_PI_FLAG_SATURATED;
    }
    if (candidate_integral != controller->integral + controller->integral_increment)
    {
        flags |= SST_PI_FLAG_INTEGRAL_CLAMPED;
    }

    /* 输出饱和且积分增量继续把输出推向饱和边界时，本拍不提交积分。 */
    if (controller->anti_windup_enable != 0U && candidate_limited != candidate_output)
    {
        if ((candidate_output > controller->output_max && controller->integral_increment > 0.0F) ||
            (candidate_output < controller->output_min && controller->integral_increment < 0.0F))
        {
            hold_integral = 1U;
        }
    }
    if (hold_integral != 0U)
    {
        flags |= SST_PI_FLAG_INTEGRAL_HELD;
    }
    else
    {
        controller->integral = candidate_integral;
    }
    controller->output_unsaturated = base_output + controller->integral;
    controller->output = SstPi_Clamp(controller->output_unsaturated, controller->output_min, controller->output_max);
    controller->saturation_error = controller->output - controller->output_unsaturated;
    controller->flags = flags;
    return SST_PI_SUCCESS;
}

/*==================================================================
 * 函数功能: 清除 dq 单轴本拍运行标志，保留积分和输出历史。
 * 输入参数: controller：待处理的 PI 对象指针。
 * 输出参数: controller：本拍 flags 置为有效，其他状态保持不变。
 * 返回值: 无。
 *================================================================*/
static void SstPi_PrepareDq(SST_PI_CONTROLLER *controller)
{
    controller->flags = SST_PI_FLAG_VALID;
}

/*==================================================================
 * 函数功能: 执行 dq 双轴 PI，完成轴向限幅、动态圆限幅和条件积分抗饱和。
 * 输入参数: d_controller：d 轴 PI 对象，output 接收最终 d 轴电压。
 *           q_controller：q 轴 PI 对象，output 接收最终 q 轴电压。
 *           reference_d：d 轴电流给定，单位 A。
 *           feedback_d：d 轴电流反馈，单位 A。
 *           feedforward_d：d 轴电压前馈，单位 V。
 *           reference_q：q 轴电流给定，单位 A。
 *           feedback_q：q 轴电流反馈，单位 A。
 *           feedforward_q：q 轴电压前馈，单位 V。
 *           voltage_limit：动态电压矢量幅值上限，单位 V；零值表示禁止输出。
 * 返回值: 1 表示两轴计算成功，0 表示参数、输入或数值计算非法。
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
    float voltage_limit)
{
    float d_candidate_integral; /* d_candidate_integral：d轴试算积分。 */
    float q_candidate_integral; /* q_candidate_integral：q轴试算积分。 */
    float d_base; /* d_base：d轴前馈加比例项。 */
    float q_base; /* q_base：q轴前馈加比例项。 */
    float d_candidate; /* d_candidate：d轴加入候选积分的输出。 */
    float q_candidate; /* q_candidate：q轴加入候选积分的输出。 */
    float d_unsaturated; /* d_unsaturated：矩形限幅后的d轴值。 */
    float q_unsaturated; /* q_unsaturated：矩形限幅后的q轴值。 */
    float d_limited; /* d_limited：d轴最终圆限幅输入。 */
    float q_limited; /* q_limited：q轴最终圆限幅输入。 */
    float magnitude_squared; /* magnitude_squared：dq矢量幅值平方。 */
    float scale; /* scale：圆限幅的同尺度缩放系数。 */
    float d_increment; /* d_increment：d轴本拍积分增量。 */
    float q_increment; /* q_increment：q轴本拍积分增量。 */
    unsigned short vector_saturated; /* vector_saturated：dq矢量是否超过动态边界。 */
    unsigned short hold_d_integral; /* hold_d_integral：是否冻结d轴积分。 */
    unsigned short hold_q_integral; /* hold_q_integral：是否冻结q轴积分。 */
    unsigned short d_flags = SST_PI_FLAG_VALID; /* d_flags：d轴本拍状态标志。 */
    unsigned short q_flags = SST_PI_FLAG_VALID; /* q_flags：q轴本拍状态标志。 */

    if (d_controller == (SST_PI_CONTROLLER *)0 || q_controller == (SST_PI_CONTROLLER *)0)
    {
        return SST_PI_FAILURE;
    }
    if (d_controller->configuration_valid == 0U || q_controller->configuration_valid == 0U ||
        d_controller->output_min > 0.0F || d_controller->output_max < 0.0F ||
        q_controller->output_min > 0.0F || q_controller->output_max < 0.0F ||
        !SstPi_IsFinite(voltage_limit) || voltage_limit < 0.0F)
    {
        SstPi_Reject(d_controller, SST_PI_FLAG_INVALID_PARAMETER);
        SstPi_Reject(q_controller, SST_PI_FLAG_INVALID_PARAMETER);
        return SST_PI_FAILURE;
    }
    if (!SstPi_IsFinite(reference_d) || !SstPi_IsFinite(feedback_d) || !SstPi_IsFinite(feedforward_d) ||
        !SstPi_IsFinite(reference_q) || !SstPi_IsFinite(feedback_q) || !SstPi_IsFinite(feedforward_q))
    {
        SstPi_Reject(d_controller, SST_PI_FLAG_INVALID_INPUT);
        SstPi_Reject(q_controller, SST_PI_FLAG_INVALID_INPUT);
        return SST_PI_FAILURE;
    }

    SstPi_PrepareDq(d_controller);
    SstPi_PrepareDq(q_controller);
    SstPi_Predict(d_controller, reference_d, feedback_d, feedforward_d,
                  &d_candidate_integral, &d_base, &d_candidate);
    SstPi_Predict(q_controller, reference_q, feedback_q, feedforward_q,
                  &q_candidate_integral, &q_base, &q_candidate);
    if (!SstPi_IsFinite(d_controller->error) || !SstPi_IsFinite(q_controller->error) ||
        !SstPi_IsFinite(d_controller->proportional) || !SstPi_IsFinite(q_controller->proportional) ||
        !SstPi_IsFinite(d_controller->integral_increment) ||
        !SstPi_IsFinite(q_controller->integral_increment) ||
        !SstPi_IsFinite(d_candidate) || !SstPi_IsFinite(q_candidate))
    {
        SstPi_Reject(d_controller, SST_PI_FLAG_NUMERIC_ERROR);
        SstPi_Reject(q_controller, SST_PI_FLAG_NUMERIC_ERROR);
        return SST_PI_FAILURE;
    }
    d_increment = d_controller->integral_increment;
    q_increment = q_controller->integral_increment;

    d_limited = SstPi_Clamp(d_candidate, d_controller->output_min, d_controller->output_max);
    q_limited = SstPi_Clamp(q_candidate, q_controller->output_min, q_controller->output_max);
    if (d_limited != d_candidate)
    {
        d_flags |= SST_PI_FLAG_SATURATED;
    }
    if (q_limited != q_candidate)
    {
        q_flags |= SST_PI_FLAG_SATURATED;
    }
    if (d_candidate_integral != d_controller->integral + d_increment)
    {
        d_flags |= SST_PI_FLAG_INTEGRAL_CLAMPED;
    }
    if (q_candidate_integral != q_controller->integral + q_increment)
    {
        q_flags |= SST_PI_FLAG_INTEGRAL_CLAMPED;
    }

    /* 先处理每轴矩形边界，再对两个轴执行动态圆形电压边界。 */
    d_unsaturated = d_limited;
    q_unsaturated = q_limited;
    magnitude_squared = d_unsaturated * d_unsaturated + q_unsaturated * q_unsaturated;
    if (!SstPi_IsFinite(magnitude_squared) || !SstPi_IsFinite(voltage_limit * voltage_limit))
    {
        SstPi_Reject(d_controller, SST_PI_FLAG_NUMERIC_ERROR);
        SstPi_Reject(q_controller, SST_PI_FLAG_NUMERIC_ERROR);
        return SST_PI_FAILURE;
    }
    /* 电压上限为零是合法的“禁止输出”状态，同时必须冻结积分。 */
    vector_saturated = (unsigned short)((voltage_limit == 0.0F) ||
                                        (magnitude_squared > voltage_limit * voltage_limit));
    hold_d_integral = 0U;
    hold_q_integral = 0U;
    if (d_controller->anti_windup_enable != 0U && d_limited != d_candidate &&
        ((d_candidate > d_controller->output_max && d_increment > 0.0F) ||
         (d_candidate < d_controller->output_min && d_increment < 0.0F)))
    {
        hold_d_integral = 1U;
    }
    if (q_controller->anti_windup_enable != 0U && q_limited != q_candidate &&
        ((q_candidate > q_controller->output_max && q_increment > 0.0F) ||
         (q_candidate < q_controller->output_min && q_increment < 0.0F)))
    {
        hold_q_integral = 1U;
    }
    if (vector_saturated != 0U)
    {
        d_flags |= SST_PI_FLAG_SATURATED | SST_PI_FLAG_VECTOR_LIMIT;
        q_flags |= SST_PI_FLAG_SATURATED | SST_PI_FLAG_VECTOR_LIMIT;
        /* 电压矢量向外继续增大时，两轴同时冻结，避免积分把圆限幅推得更深。 */
        if ((d_controller->anti_windup_enable != 0U || q_controller->anti_windup_enable != 0U) &&
            (voltage_limit == 0.0F ||
             (d_unsaturated * d_increment + q_unsaturated * q_increment > 0.0F)))
        {
            if (d_controller->anti_windup_enable != 0U)
            {
                hold_d_integral = 1U;
            }
            if (q_controller->anti_windup_enable != 0U)
            {
                hold_q_integral = 1U;
            }
        }
    }
    /* 零边界表示上层禁止输出，冻结积分不依赖可选抗饱和开关。 */
    if (voltage_limit == 0.0F)
    {
        hold_d_integral = 1U;
        hold_q_integral = 1U;
    }
    if (hold_d_integral != 0U)
    {
        d_flags |= SST_PI_FLAG_INTEGRAL_HELD;
    }
    else
    {
        d_controller->integral = d_candidate_integral;
    }
    if (hold_q_integral != 0U)
    {
        q_flags |= SST_PI_FLAG_INTEGRAL_HELD;
    }
    else
    {
        q_controller->integral = q_candidate_integral;
    }

    d_controller->output_unsaturated = d_base + d_controller->integral;
    q_controller->output_unsaturated = q_base + q_controller->integral;
    d_limited = SstPi_Clamp(d_controller->output_unsaturated, d_controller->output_min, d_controller->output_max);
    q_limited = SstPi_Clamp(q_controller->output_unsaturated, q_controller->output_min, q_controller->output_max);
    if (voltage_limit == 0.0F)
    {
        d_controller->output = 0.0F;
        q_controller->output = 0.0F;
        /* 零电压边界下积分保持不变，即使本拍增量恰好为零也报告冻结状态。 */
        d_flags |= SST_PI_FLAG_INTEGRAL_HELD;
        q_flags |= SST_PI_FLAG_INTEGRAL_HELD;
        d_flags |= SST_PI_FLAG_SATURATED | SST_PI_FLAG_VECTOR_LIMIT;
        q_flags |= SST_PI_FLAG_SATURATED | SST_PI_FLAG_VECTOR_LIMIT;
    }
    else
    {
        magnitude_squared = d_limited * d_limited + q_limited * q_limited;
        if (!SstPi_IsFinite(magnitude_squared) || !SstPi_IsFinite(voltage_limit * voltage_limit))
        {
            SstPi_Reject(d_controller, SST_PI_FLAG_NUMERIC_ERROR);
            SstPi_Reject(q_controller, SST_PI_FLAG_NUMERIC_ERROR);
            return SST_PI_FAILURE;
        }
        if (magnitude_squared > voltage_limit * voltage_limit)
        {
            scale = voltage_limit / sqrtf(magnitude_squared);
            d_controller->output = d_limited * scale;
            q_controller->output = q_limited * scale;
            d_flags |= SST_PI_FLAG_SATURATED | SST_PI_FLAG_VECTOR_LIMIT;
            q_flags |= SST_PI_FLAG_SATURATED | SST_PI_FLAG_VECTOR_LIMIT;
        }
        else
        {
            d_controller->output = d_limited;
            q_controller->output = q_limited;
        }
    }
    d_controller->saturation_error = d_controller->output - d_controller->output_unsaturated;
    q_controller->saturation_error = q_controller->output - q_controller->output_unsaturated;
    d_controller->flags = d_flags;
    q_controller->flags = q_flags;
    return SST_PI_SUCCESS;
}
