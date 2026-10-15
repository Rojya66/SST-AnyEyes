//###########################################################################
// FILE: SstParameterTable.c
// FUNCTION: 唯一的8个命令/37个参数映射表及线上缩放规则。
//           参数地址、范围、显示读取和42字持久化快照均引用本表。
//###########################################################################
#include "SstParameterTable.h"
#include "ChbDabControl.h"

typedef struct
{
    volatile void *address; /* address：被映射控制成员的地址，只允许表内已绑定的对象。 */
    float scale; /* scale：物理值到 16 位线上值的乘数，写入时使用其倒数。 */
    int32_t minimum, /* minimum：允许的最小值；映射表中按线上整数范围定义。 */
        maximum; /* maximum：允许的最大值；映射表中按线上整数范围定义。 */
    uint16_t isSigned, /* isSigned：1 表示线上值按有符号 16 位二补码解释。 */
        isWord; /* isWord：1 表示目标是 unsigned short，0 表示 float。 */
} SST_PARAMETER;

static const SST_PARAMETER commands[] = {
    {(volatile void *)&g_chb_dab_command.run_enable, 1.0F, 0L, 1L, 0U, 1U},
    {(volatile void *)&g_chb_dab_command.dab_enable, 1.0F, 0L, 1L, 0U, 1U},
    {(volatile void *)&g_chb_dab_command.ac_mode, 1.0F, 0L, 1L, 0U, 1U},
    {(volatile void *)&g_chb_dab_command.fault_reset, 1.0F, 0L, 1L, 0U, 1U},
    {(volatile void *)&g_chb_dab_command.open_loop_modulation, 10000.0F, 0L, 9800L, 0U, 0U},
    {(volatile void *)&g_chb_dab_command.open_loop_frequency_hz, 100.0F, 100L, 10000L, 0U, 0U},
    {(volatile void *)&g_chb_dab_command.dc_voltage_ref_v, 10.0F, 1L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_command.iq_reference_a, 100.0F, -32768L, 32767L, 1U, 0U},
}; /* commands：组 0 的八个控制命令/给定映射，顺序即固定组内地址。 */
static const SST_PARAMETER parameters[] = {
    {(volatile void *)&g_chb_dab_params.sample_period_s, 10000000.0F, 1L, 10000L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.modulation_max, 10000.0F, 1L, 9800L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.precharge_voltage_v, 10.0F, 0L, 30000L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.precharge_stable_10ms, 1.0F, 1L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.precharge_timeout_10ms, 1.0F, 1L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.bypass_delay_10ms, 1.0F, 0L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.breaker_pulse_10ms, 1.0F, 1L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.pll_nominal_frequency_hz, 100.0F, 100L, 10000L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.pll_kp, 10.0F, 0L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.pll_ki, 1.0F, 0L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.pll_correction_limit_rad_s, 100.0F, 0L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.pll_lock_error_pu, 10000.0F, 1L, 10000L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.pll_min_grid_peak_v, 10.0F, 0L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.pll_lock_10ms, 1.0F, 1L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.pll_timeout_10ms, 1.0F, 1L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.voltage_kp, 100000.0F, -32768L, 32767L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.voltage_ki, 100.0F, -32768L, 32767L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.voltage_current_limit_a, 100.0F, 1L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.startup_strategy, 1.0F, 0L, 1L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.startup_udc_offset_v, 100.0F, -32768L, 32767L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.startup_udc_step_v, 1000.0F, 1L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.startup_id_reference_a, 100.0F, -32768L, 32767L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.startup_id_ramp_step_a, 10000.0F, 1L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.startup_ramp_tad, 1.0F, 1L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.current_kp, 1000.0F, -32768L, 32767L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.current_ki, 1000.0F, -32768L, 32767L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.closed_loop_voltage_limit_factor, 10000.0F, 1L, 30000L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.current_feedback_sign, 1.0F, -1L, 1L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.current_phase_compensation_rad, 10000.0F, -31415L, 31415L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.output_phase_compensation_rad, 10000.0F, -31415L, 31415L, 1U, 0U},
    {(volatile void *)&g_chb_dab_params.ac_overcurrent_trip_a, 100.0F, 1L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.dc_overvoltage_trip_v, 10.0F, 1L, 65535L, 0U, 0U},
    {(volatile void *)&g_chb_dab_params.protection_confirm_samples, 1.0F, 1L, 65535L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.dab_start_duty_count, 1.0F, 0L, 2500L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.dab_target_duty_count, 1.0F, 0L, 2500L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.dab_ramp_step_count, 1.0F, 1L, 2500L, 0U, 1U},
    {(volatile void *)&g_chb_dab_params.dab_start_count, 1.0F, 0L, 4999L, 0U, 1U},
}; /* parameters：37 个控制参数的固定地址、比例及范围映射。 */

/*==================================================================
 * 函数功能: 读取映射参数并按比例、符号和 16 位范围饱和编码
 * 输入参数: p：已经绑定地址、缩放及范围的 SST_PARAMETER 映射条目
 * 输出参数: 无。
 * 返回值: 16 位线上值，有符号参数使用二补码
 * 执行说明: 后台固定参数表访问，不接受任意 RAM 地址。
 *================================================================*/
static uint16_t ReadEntry(const SST_PARAMETER *p)
{
    float value; /* value：物理成员读取值及缩放后的浮点线上值。 */
    int32_t raw; /* raw：参数物理值缩放后的有符号整数中间值。 */
    value = p->isWord ? (float)*(volatile unsigned short *)p->address : *(volatile float *)p->address;
    value *= p->scale;
    if (value != value)
        return 0;
    if (value > (p->isSigned ? 32767.0F : 65535.0F))
        value = p->isSigned ? 32767.0F : 65535.0F;
    if (value < (p->isSigned ? -32768.0F : 0.0F))
        value = p->isSigned ? -32768.0F : 0.0F;
    raw = (int32_t)(value + (value < 0 ? -0.5F : 0.5F));
    return (uint16_t)raw;
}

/*==================================================================
 * 函数功能: 检查线上值范围后，反比例换算并写入映射成员
 * 输入参数: p：已经绑定地址、缩放及范围的 SST_PARAMETER 映射条目
 *           value：待写入的 16 位线上参数编码，有符号量按二补码解释
 * 输出参数: 仅更新 p->address 指定的受控成员。
 * 返回值: SST_PARAM_OK 或 SST_PARAM_RANGE
 * 执行说明: 仅执行表内范围及编码转换；停机和存储互锁由上层写接口检查。
 *================================================================*/
static uint16_t WriteEntry(const SST_PARAMETER *p, uint16_t value)
{
    int32_t raw =
        p->isSigned ? (int32_t)(int16_t)value : (int32_t)value; /* raw：参数物理值缩放后的有符号整数中间值。 */
    if (raw < p->minimum || raw > p->maximum)
        return SST_PARAM_RANGE;
    if (p->isWord)
        *(volatile unsigned short *)p->address = (unsigned short)raw;
    else
        *(volatile float *)p->address = (float)raw / p->scale;
    return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 按唯一命令表读取线上编码
 * 输入参数: index：命令表索引，范围0..7
 * 输出参数: 无。
 * 返回值: 16位线上编码；越界返回0
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstParameterTable_ReadCommand(uint16_t index)
{
    return index < 8U ? ReadEntry(&commands[index]) : 0;
}

/*==================================================================
 * 函数功能: 按唯一参数表读取线上编码
 * 输入参数: index：参数表索引，范围0..36
 * 输出参数: 无。
 * 返回值: 16位线上编码；越界返回0
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstParameterTable_ReadParameter(uint16_t index)
{
    return index < 37U ? ReadEntry(&parameters[index]) : 0;
}

/*==================================================================
 * 函数功能: 检查范围后写入命令表对应成员
 * 输入参数: index：命令表索引，范围0..7
 *           value：待写入的16位线上编码
 * 输出参数: 更新唯一表绑定的命令成员。
 * 返回值: SST_PARAM_*结果码
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstParameterTable_WriteCommand(uint16_t index, uint16_t value)
{
    return index < 8U ? WriteEntry(&commands[index], value) : SST_PARAM_BAD_ADDRESS;
}

/*==================================================================
 * 函数功能: 检查范围后写入参数表对应成员
 * 输入参数: index：参数表索引，范围0..36
 *           value：待写入的16位线上编码
 * 输出参数: 更新唯一表绑定的参数成员。
 * 返回值: SST_PARAM_*结果码
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstParameterTable_WriteParameter(uint16_t index, uint16_t value)
{
    return index < 37U ? WriteEntry(&parameters[index], value) : SST_PARAM_BAD_ADDRESS;
}

/*==================================================================
 * 函数功能: 按既有EEPROM顺序读取37参数及5命令给定
 * 输入参数: index：快照字索引，0..36为参数，37..41为命令2/4/5/6/7
 * 输出参数: 无。
 * 返回值: 16位线上编码；越界返回0
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstParameterTable_SnapshotRead(uint16_t index)
{
    if (index < 37U)
        return ReadEntry(&parameters[index]);
    if (index < 42U)
        return ReadEntry(&commands[index == 37U ? 2U : index - 34U]);
    return 0;
}

/*==================================================================
 * 函数功能: 整体校验42字镜像，不修改任何活动成员
 * 输入参数: snapshot：至少42个有效16位线上字，顺序与既有EEPROM一致
 * 输出参数: 无。
 * 返回值: SST_PARAM_OK或范围/地址错误
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstParameterTable_SnapshotValidate(const uint16_t *snapshot)
{
    uint16_t i, /* i：快照逐项校验的索引。 */
        word; /* word：当前镜像字的线上编码。 */
    if (!snapshot)
        return SST_PARAM_BAD_ADDRESS;
    for (i = 0; i < 42; ++i)
    {
        const SST_PARAMETER *p =
            i < 37 ? &parameters[i] : &commands[i == 37 ? 2 : i - 34]; /* p：待校验快照字对应的唯一表条目。 */
        int32_t raw; /* raw：按表内符号规则解释的线上整数。 */
        word = snapshot[i];
        raw = p->isSigned ? (int32_t)(int16_t)word : (int32_t)word;
        if (raw < p->minimum || raw > p->maximum)
            return SST_PARAM_RANGE;
    }
    if (snapshot[27] != 1 && snapshot[27] != 0xFFFF)
        return SST_PARAM_RANGE;
    if (snapshot[33] > snapshot[34])
        return SST_PARAM_RANGE;
    if (snapshot[38] > snapshot[1])
        return SST_PARAM_RANGE;
    if (snapshot[0] != ReadEntry(&parameters[0]))
        return SST_PARAM_RANGE;
    return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 将已整体校验镜像恢复到活动参数及5个命令给定
 * 输入参数: snapshot：由SnapshotValidate成功校验的42字镜像
 * 输出参数: 恢复参数1..36和命令2/4/5/6/7；硬件采样周期及启停命令不恢复。
 * 返回值: 无
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
void SstParameterTable_SnapshotApply(const uint16_t *snapshot)
{
    uint16_t i, /* i：快照恢复的索引。 */
        n, /* n：快照命令字对应的唯一命令表索引。 */
        result; /* result：经整体校验后写入参数条目的结果。 */
    for (i = 1; i < 37; ++i)
    {
        result = WriteEntry(&parameters[i], snapshot[i]);
        (void)result;
    }
    for (i = 37; i < 42; ++i)
    {
        n = i == 37 ? 2 : i - 34;
        WriteEntry(&commands[n], snapshot[i]);
    }
}
