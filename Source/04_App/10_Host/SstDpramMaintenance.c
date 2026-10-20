// 文件职责：仅并口的现代参数维护。请求处理、CRC及EEPROM请求均在后台，不进入100us中断。
#include "SstDpramMaintenance.h"
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstCommand.h"
#include "SstParams.h"
#include "SstParameterTable.h"
#include "SstParameterStore.h"
#include "SstLegacyParameters.h"
#include "SstFault.h"
#include "DPRAM.h"
#include "SstBoardConfig.h"
#include "hw_types.h"

#if SST_ENABLE_DPRAM
typedef struct
{
    volatile uint16_t *target; /* target：固定的并口输出数组，不接受主机任意地址。 */
    uint16_t *data; /* data：主循环内构建的RAM快照，发布期间不修改。 */
    uint16_t count; /* count：快照总字数。 */
    uint16_t next; /* next：下一待复制数据字索引，word0保留给版本戳。 */
    uint16_t epoch; /* epoch：偶数完整版本；写入期间为epoch-1奇数。 */
    uint16_t busy; /* busy：发布未结束标志。 */
} SST_DPRAM_PUBLICATION;

static uint16_t response_data[SST_DPRAM_BLOCK_WORDS]; /* response_data：回复及54点实际值RAM快照。 */
static uint16_t telemetry_data[SST_DPRAM_BLOCK_WORDS]; /* telemetry_data：现代遥测RAM快照。 */
static uint16_t history_data[SST_DPRAM_HISTORY_WORDS]; /* history_data：详细故障历史RAM快照，不复制C结构对齐空洞。 */
static SST_DPRAM_PUBLICATION response_publication; /* response_publication：回复分片发布进度。 */
static SST_DPRAM_PUBLICATION telemetry_publication; /* telemetry_publication：遥测分片发布进度。 */
static SST_DPRAM_PUBLICATION history_publication; /* history_publication：历史分片发布进度。 */
static uint16_t request_pending; /* request_pending：邮箱21已通知，等待前一回复完成后处理。 */
static uint16_t last_request[SST_DPRAM_REQUEST_WORDS]; /* last_request：最近一次合法请求，用于同序号同载荷幂等。 */
static uint16_t last_valid; /* last_valid：是否已记录一个合法序号。 */
static uint16_t last_result; /* last_result：最近合法序号的受理结果，非法请求不能覆盖幂等缓存。 */
static uint16_t ack_sequence, /* ack_sequence：最近回复对应的请求序号，不等同PWM执行完成。 */
    ack_result, /* ack_result：最近请求受理结果，SST_PARAM_*或扩展格式错误。 */
    ack_operation, /* ack_operation：最近回复对应的操作码。 */
    ack_point; /* ack_point：最近回复对应的点表索引。 */
static uint16_t notify_response; /* notify_response：完整回复发布后才写右邮箱21。 */
static uint32_t history_sequence; /* history_sequence：最近发布历史的事件序号。 */
static uint16_t history_count, /* history_count：最近发布历史的有效条数。 */
    history_initialized; /* history_initialized：详细历史是否已发布过一次。 */

/*==================================================================
 * 函数功能: 计算请求前12字的小端CRC16/IBM
 * 输入参数: data：至少12个16位字
 * 输出参数: 无。
 * 返回值: 请求前12字的CRC16/IBM校验值，初值FFFF，多项式A001，无最终异或。
 * 执行说明: 后台固定24字节计算，不读取硬件或关中断。
 *================================================================*/
static uint16_t RequestCrc(const uint16_t *data)
{
    uint16_t crc = 0xFFFFU; /* crc：CRC累积值。 */
    uint16_t i, /* i：当前请求、参数或发布快照中的 16 位字索引。 */
        byte_index, /* byte_index：CRC当前字的低/高字节下标。 */
        bit; /* bit：CRC逐位迭代下标，范围0..7。 */
    for (i = 0U; i < SST_DPRAM_REQUEST_CRC_WORDS; ++i)
        for (byte_index = 0U; byte_index < 2U; ++byte_index)
        {
            crc ^= (uint16_t)((data[i] >> (8U * byte_index)) & 0xFFU);
            for (bit = 0U; bit < 8U; ++bit)
            {
                uint16_t polynomial = 0U; /* polynomial：当前位为1时使用IBM多项式。 */
                if (crc & 1U)
                    polynomial = 0xA001U;
                crc = (uint16_t)((crc >> 1) ^ polynomial);
            }
        }
    return crc;
}

/*==================================================================
 * 函数功能: 校验请求帧的固定字段及提交时序
 * 输入参数: data：主循环复制的请求帧；commit：复制前读取的提交字
 * 输出参数: 无。
 * 返回值: 1表示帧完整且一致，0表示应回复格式错误。
 * 执行说明: 提交字和序号在复制前后各读取一次，防止主机正在改写帧时消费半帧。
 *==================================================================*/
static uint16_t IsRequestFrameValid(const uint16_t *data, uint16_t commit)
{
    if (commit != SST_DPRAM_COMMIT)
        return 0U;
    if (data[SST_DPRAM_REQUEST_COMMIT_WORD] != commit)
        return 0U;
    if (DPRAM_SstRequest[SST_DPRAM_REQUEST_COMMIT_WORD] != commit)
        return 0U;
    if (DPRAM_SstRequest[SST_DPRAM_REQUEST_SEQUENCE_WORD] != data[SST_DPRAM_REQUEST_SEQUENCE_WORD])
        return 0U;
    if (DPRAM_SstRequest[SST_DPRAM_REQUEST_SEQUENCE_COPY_WORD] != data[SST_DPRAM_REQUEST_SEQUENCE_COPY_WORD])
        return 0U;
    if (data[SST_DPRAM_REQUEST_MAGIC_WORD] != SST_DPRAM_MAGIC)
        return 0U;
    if (data[SST_DPRAM_REQUEST_VERSION_WORD] != SST_DPRAM_VERSION)
        return 0U;
    if (data[SST_DPRAM_REQUEST_SEQUENCE_WORD] == 0U)
        return 0U;
    if (data[SST_DPRAM_REQUEST_SEQUENCE_WORD] != data[SST_DPRAM_REQUEST_SEQUENCE_COPY_WORD])
        return 0U;
    if (data[SST_DPRAM_REQUEST_RESERVED_WORD] != 0U)
        return 0U;
    if (data[SST_DPRAM_REQUEST_TAIL_RESERVED_WORD] != 0U)
        return 0U;
    if (RequestCrc(data) != data[SST_DPRAM_REQUEST_CRC_WORD])
        return 0U;
    return 1U;
}

/*==================================================================
 * 函数功能: 按54点表读取当前实际参数编码
 * 输入参数: point：0..53
 * 输出参数: 无。
 * 返回值: 16位线上编码；非法点号返回0。
 * 执行说明: 固定表访问；命令0..7、保护总使能8、参数9..45、原参数46..53。
 *================================================================*/
static uint16_t ReadPoint(uint16_t point)
{
    if (point < SST_DPRAM_COMMAND_POINT_COUNT)
        return SstParameterTable_ReadCommand(point);
    if (point == SST_DPRAM_PROTECTION_ENABLE_POINT)
        return g_chb_dab_protection_enable;
    if (point < SST_DPRAM_PARAMETER_POINT_END)
        return SstParameterTable_ReadParameter((uint16_t)(point - SST_DPRAM_PARAMETER_POINT_START));
    if (point < SST_DPRAM_POINT_COUNT)
        return SstLegacyParameters_Read((uint16_t)(point - SST_DPRAM_LEGACY_POINT_START));
    return 0U;
}

/*==================================================================
 * 函数功能: 以DPRAM控制源身份校验并写入单个点
 * 输入参数: point：点号；value：固定16位编码
 * 输出参数: 合法命令或参数；返回SST_PARAM_*
 * 返回值: SST_PARAM_*操作结果。
 * 执行说明: 启停/复位不借用串口权限；普通参数复用AnyEyes唯一范围与停机校验。
 *================================================================*/
static uint16_t WritePoint(uint16_t point, uint16_t value)
{
#if SST_ALLOW_REMOTE_COMMANDS
    uint16_t n; /* n：现代参数表索引，跨AnyEyes组1与组2。 */
    uint16_t group; /* group：参数表所属组号。 */
    uint16_t index; /* index：组内参数索引。 */
    if (point >= SST_DPRAM_POINT_COUNT)
        return SST_PARAM_BAD_ADDRESS;
    if (point == 0U || point == 1U || point == 3U)
    {
        if (value > 1U)
            return SST_PARAM_RANGE;
        if (SstParams_IsStorageBusy() && (value || point == 3U))
            return SST_PARAM_BUSY;
        if (point == 3U && !SstParams_IsStopped())
            return SST_PARAM_BUSY;
        if (point == 0U)
            g_chb_dab_command.run_enable = value;
        else if (point == 1U)
            g_chb_dab_command.dab_enable = value;
        else
            g_chb_dab_command.fault_reset = value;
        return SST_PARAM_OK;
    }
    if (point < SST_DPRAM_PARAMETER_POINT_START)
        return SstParams_Write(0U, point, value);
    if (point < SST_DPRAM_PARAMETER_POINT_END)
    {
        n = (uint16_t)(point - SST_DPRAM_PARAMETER_POINT_START);
        if (n < 34U)
        {
            group = 1U;
            index = n;
        }
        else
        {
            group = 2U;
            index = (uint16_t)(n - 34U);
        }
        return SstParams_Write(group, index, value);
    }
    return SstLegacyParameters_Write((uint16_t)(point - SST_DPRAM_LEGACY_POINT_START), value);
#else
    (void)point;
    (void)value;
    return SST_PARAM_DENIED;
#endif
}

/*==================================================================
 * 函数功能: 提交五字遥控并返回是否全部满足受理条件
 * 输入参数: data：完整合法请求，其中6..10为start/stop/on/off/reset
 * 输出参数: 统一控制命令；返回OK/BUSY/DENIED/RANGE
 * 返回值: SST_PARAM_OK/BUSY/DENIED/RANGE，表示受理结果。
 * 执行说明: STOP/OFF始终优先；结果表示请求受理，不表示状态机已达到运行状态。
 *================================================================*/
static uint16_t ApplyCommand(const uint16_t *data)
{
#if SST_ALLOW_REMOTE_COMMANDS
    uint16_t result = SST_PARAM_OK; /* result：忙或复位条件不满足时报告部分请求未受理。 */
    if ((data[SST_DPRAM_REQUEST_START_WORD] & 0xFFFEU) != 0U)
        return SST_PARAM_RANGE;
    if ((data[SST_DPRAM_REQUEST_STOP_WORD] & 0xFFFEU) != 0U)
        return SST_PARAM_RANGE;
    if ((data[SST_DPRAM_REQUEST_PULSE_ON_WORD] & 0xFFFCU) != 0U)
        return SST_PARAM_RANGE;
    if ((data[SST_DPRAM_REQUEST_PULSE_OFF_WORD] & 0xFFFCU) != 0U)
        return SST_PARAM_RANGE;
    if ((data[SST_DPRAM_REQUEST_FAULT_RESET_WORD] & 0xFFFEU) != 0U)
        return SST_PARAM_RANGE;
    if ((data[SST_DPRAM_REQUEST_STOP_WORD] & 1U) == 0U)
    {
        if (SstParams_IsStorageBusy() &&
            ((data[SST_DPRAM_REQUEST_START_WORD] != 0U) || (data[SST_DPRAM_REQUEST_PULSE_ON_WORD] != 0U) || (data[SST_DPRAM_REQUEST_FAULT_RESET_WORD] != 0U)))
            result = SST_PARAM_BUSY;
    }
    SstCommand_ApplyDpram(data[SST_DPRAM_REQUEST_START_WORD],
                          data[SST_DPRAM_REQUEST_STOP_WORD],
                          data[SST_DPRAM_REQUEST_PULSE_ON_WORD],
                          data[SST_DPRAM_REQUEST_PULSE_OFF_WORD],
                          data[SST_DPRAM_REQUEST_FAULT_RESET_WORD]);
    /* 先按OFF优先更新命令，再按同一停机判据判断复位是否受理，避免ON/OFF同位误报。 */
    if ((data[SST_DPRAM_REQUEST_STOP_WORD] & 1U) == 0U && data[SST_DPRAM_REQUEST_FAULT_RESET_WORD] != 0U && SstParams_IsStopped() == 0U)
        result = SST_PARAM_BUSY;
    return result;
#else
    (void)data;
    return SST_PARAM_DENIED;
#endif
}

/*==================================================================
 * 函数功能: 开始一份RAM快照的分片发布
 * 输入参数: publication：已固定目标与快照数组的发布对象
 * 输出参数: busy/next/epoch；目标word0写入奇数标记
 * 返回值: 无
 * 执行说明: 调用者保证无旧发布进行；主机只接受前后相同的非零偶数版本。
 *================================================================*/
static void BeginPublication(SST_DPRAM_PUBLICATION *publication)
{
    publication->epoch = (uint16_t)(publication->epoch + 2U);
    if (!publication->epoch)
        publication->epoch = 2U;
    publication->target[SST_DPRAM_PUBLICATION_WORD] = (uint16_t)(publication->epoch - 1U);
    publication->next = SST_DPRAM_PUBLICATION_WORD + 1U;
    publication->busy = 1U;
}

/*==================================================================
 * 函数功能: 在本轮并口访问预算内推进发布
 * 输入参数: publication：发布对象
 * 输出参数: 最多16个数据字；最后发布完整偶数版本
 * 返回值: 1本轮完成，0尚未完成
 * 执行说明: 一次最多写 16 个数据字；完成时额外写 word0 的偶数版本戳，不关中断。
 *================================================================*/
static uint16_t PublishSlice(SST_DPRAM_PUBLICATION *publication)
{
    uint16_t budget = SST_DPRAM_PUBLICATION_BUDGET_WORDS; /* budget：单轮新增并口写入数据字上限。 */
    while (budget && publication->next < publication->count)
    {
        publication->target[publication->next] = publication->data[publication->next];
        ++publication->next;
        --budget;
    }
    if (publication->next < publication->count)
        return 0U;
    publication->target[SST_DPRAM_PUBLICATION_WORD] = publication->epoch;
    publication->busy = 0U;
    return 1U;
}

/*==================================================================
 * 函数功能: 构建回复头与全部54点实际值
 * 输入参数: 无
 * 输出参数: response_data，随后启动分片发布
 * 返回值: 无
 * 执行说明: 只有无前一回复发布时调用；参数读回不是启动完成确认。
 *================================================================*/
static void BuildResponse(void)
{
    uint16_t i; /* i：当前请求、参数或发布快照中的 16 位字索引。 */
    for (i = 0U; i < SST_DPRAM_BLOCK_WORDS; ++i)
        response_data[i] = 0U;
    response_data[SST_DPRAM_RESPONSE_MAGIC_WORD] = SST_DPRAM_MAGIC;
    response_data[SST_DPRAM_RESPONSE_VERSION_WORD] = SST_DPRAM_VERSION;
    response_data[SST_DPRAM_RESPONSE_SEQUENCE_WORD] = ack_sequence;
    response_data[SST_DPRAM_RESPONSE_RESULT_WORD] = ack_result;
    response_data[SST_DPRAM_RESPONSE_OPERATION_WORD] = ack_operation;
    response_data[SST_DPRAM_RESPONSE_POINT_WORD] = ack_point;
    response_data[SST_DPRAM_RESPONSE_VALUE_WORD] = ReadPoint(ack_point);
    response_data[SST_DPRAM_RESPONSE_STORAGE_RESULT_WORD] = SstParams_GetStorageResult();
    response_data[SST_DPRAM_RESPONSE_STORAGE_PENDING_WORD] = SstParameterStore_IsPending();
    response_data[SST_DPRAM_RESPONSE_REMOTE_ALLOWED_WORD] = SST_ALLOW_REMOTE_COMMANDS;
    response_data[SST_DPRAM_RESPONSE_POINT_COUNT_WORD] = SST_DPRAM_POINT_COUNT;
    for (i = 0U; i < SST_DPRAM_POINT_COUNT; ++i)
        response_data[SST_DPRAM_RESPONSE_PARAMETER_START_WORD + i] = ReadPoint(i);
    BeginPublication(&response_publication);
}

/*==================================================================
 * 函数功能: 复制并校验请求，执行单项维护操作并记录回复
 * 输入参数: 无，读取固定请求区前16字
 * 输出参数: 受控参数、命令或异步保存/加载请求
 * 返回值: 无；结果写入回复头
 * 执行说明: 请求序号非零且副本一致，提交标记最后写；同序号同载荷不重复执行。
 *================================================================*/
static void ProcessRequest(void)
{
    uint16_t data[SST_DPRAM_REQUEST_WORDS]; /* data：请求RAM副本，防止消费时反复读取并口。 */
    uint16_t i, /* i：当前请求、参数或发布快照中的 16 位字索引。 */
        same = 1U; /* same：同序号请求载荷完全一致标志。 */
    uint16_t commit = DPRAM_SstRequest[SST_DPRAM_REQUEST_COMMIT_WORD]; /* commit：读取前的末提交标记。 */
    for (i = 0U; i < SST_DPRAM_REQUEST_WORDS; ++i)
        data[i] = DPRAM_SstRequest[i];
    request_pending = 0U;
    ack_sequence = data[SST_DPRAM_REQUEST_SEQUENCE_WORD];
    ack_operation = data[SST_DPRAM_REQUEST_OPERATION_WORD];
    ack_point = data[SST_DPRAM_REQUEST_POINT_WORD];
    if (!IsRequestFrameValid(data, commit))
        ack_result = SST_DPRAM_FRAME_ERROR;
    else if (last_valid && data[SST_DPRAM_REQUEST_SEQUENCE_WORD] == last_request[SST_DPRAM_REQUEST_SEQUENCE_WORD])
    {
        for (i = 0U; i < SST_DPRAM_REQUEST_WORDS; ++i)
            if (data[i] != last_request[i])
                same = 0U;
        if (same != 0U)
            ack_result = last_result;
        else
            ack_result = SST_DPRAM_FRAME_ERROR;
    }
    else
    {
        switch (data[SST_DPRAM_REQUEST_OPERATION_WORD])
        {
            case 0:
                if (data[SST_DPRAM_REQUEST_POINT_WORD] < SST_DPRAM_POINT_COUNT)
                    ack_result = SST_PARAM_OK;
                else
                    ack_result = SST_PARAM_BAD_ADDRESS;
                break;
            case 1:
                ack_result = WritePoint(data[SST_DPRAM_REQUEST_POINT_WORD], data[SST_DPRAM_REQUEST_VALUE_WORD]);
                break;
            case 2:
                ack_result = ApplyCommand(data);
                break;
            case 3:
            case 4:
#if SST_ALLOW_REMOTE_COMMANDS
                if (data[SST_DPRAM_REQUEST_VALUE_WORD] == SST_DPRAM_COMMIT)
                    ack_result = SstParameterStore_Request(
                        (uint16_t)(data[SST_DPRAM_REQUEST_OPERATION_WORD] - 3U));
                else
                    ack_result = SST_PARAM_RANGE;
#else
                ack_result = SST_PARAM_DENIED;
#endif
                break;
            default:
                ack_result = SST_PARAM_BAD_ADDRESS;
                break;
        }
        for (i = 0U; i < SST_DPRAM_REQUEST_WORDS; ++i)
            last_request[i] = data[i];
        last_result = ack_result;
        last_valid = 1U;
    }
    notify_response = 1U;
    BuildResponse();
}

/*==================================================================
 * 函数功能: 量测缩放为有/无符号16位值并饱和
 * 输入参数: value：已乘缩放的物理值；is_signed：二补码编码选择
 * 输出参数: 无。
 * 返回值: 饱和及四舍五入后的16位编码，NaN返回0。
 * 执行说明: 后台显示转换，不反馈到控制参数。
 *================================================================*/
static uint16_t Encode(float value, uint16_t is_signed)
{
    float lower = 0.0F; /* lower：编码下界。 */
    float upper = 65535.0F; /* upper：编码上界。 */
    if (is_signed)
    {
        lower = -32768.0F;
        upper = 32767.0F;
    }
    if (value != value)
        return 0U;
    if (value < lower)
        value = lower;
    if (value > upper)
        value = upper;
    if (value < 0.0F)
        value -= 0.5F;
    else
        value += 0.5F;
    return (uint16_t)(int32_t)value;
}

/*==================================================================
 * 函数功能: 按布尔状态设置遥测 flags 位
 * 输入参数: flags：当前位图；condition：对应状态；mask：协议位掩码
 * 输出参数: 无。
 * 返回值: 更新后的位图。
 * 执行说明: 每个状态单独写出，便于维护和核对 DPRAM_BITS.csv。
 *==================================================================*/
static uint16_t SetTelemetryFlag(uint16_t flags, uint16_t condition, uint16_t mask)
{
    if (condition)
        flags = (uint16_t)(flags | mask);
    return flags;
}

/*==================================================================
 * 函数功能: 将32位节拍或序号拆成小端低高字
 * 输入参数: data：两字目标；value：32位值
 * 输出参数: data[0]低16位，data[1]高16位
 * 返回值: 无
 * 执行说明: 纯RAM打包，不复制编译器结构对齐字。
 *================================================================*/
static void Put32(uint16_t *data, uint32_t value)
{
    data[0] = (uint16_t)value;
    data[1] = (uint16_t)(value >> 16);
}

/*==================================================================
 * 函数功能: 事件数量/序号改变时构建详细历史快照
 * 输入参数: 无
 * 输出参数: 头16字加32条15字记录，未使用字清零
 * 返回值: 无
 * 执行说明: RAM历史仅后台观察，无掉电保存或首次跳闸时刻保证。
 *================================================================*/
static void BuildHistory(void)
{
    SST_FAULT_RECORD record; /* record：当前读取的单条详细历史，不直接结构memcpy。 */
    uint16_t count = SstFault_GetHistoryCount(); /* count：历史有效条数。 */
    uint32_t sequence; /* sequence：最新事件序号。 */
    uint16_t i, /* i：当前请求、参数或发布快照中的 16 位字索引。 */
        base; /* base：当前详细历史记录在目标数组中的起始字偏移。 */
    sequence = 0UL;
    if (SstFault_GetHistory(0U, &record))
        sequence = (uint32_t)record.sequence;
    if (history_publication.busy)
        return;
    if (history_initialized && count == history_count && sequence == history_sequence)
        return;
    history_count = count;
    history_sequence = sequence;
    history_initialized = 1U;
    for (i = 0U; i < SST_DPRAM_HISTORY_WORDS; ++i)
        history_data[i] = 0U;
    history_data[SST_DPRAM_HISTORY_MAGIC_WORD] = SST_DPRAM_MAGIC;
    history_data[SST_DPRAM_HISTORY_VERSION_WORD] = SST_DPRAM_VERSION;
    history_data[SST_DPRAM_HISTORY_COUNT_WORD] = count;
    history_data[SST_DPRAM_HISTORY_RECORD_WORDS_WORD] = SST_DPRAM_HISTORY_RECORD_WORDS;
    Put32(&history_data[SST_DPRAM_HISTORY_SEQUENCE_LOW_WORD], sequence);
    for (i = 0U; i < count && i < SST_DPRAM_HISTORY_RECORD_COUNT; ++i)
    {
        if (!SstFault_GetHistory(i, &record))
            break;
        base = (uint16_t)(SST_DPRAM_HISTORY_RECORD_START_WORD + SST_DPRAM_HISTORY_RECORD_WORDS * i);
        history_data[base + SST_DPRAM_HISTORY_CODE_OFFSET] = record.code;
        history_data[base + SST_DPRAM_HISTORY_LEVEL_OFFSET] = record.level;
        history_data[base + SST_DPRAM_HISTORY_CONTROL_FAULT_OFFSET] = record.control_fault;
        history_data[base + SST_DPRAM_HISTORY_PORT_FAULT_OFFSET] = record.port_fault;
        history_data[base + SST_DPRAM_HISTORY_FAULT_TYPE_OFFSET] = record.fault_type;
        history_data[base + SST_DPRAM_HISTORY_PORT_LATCH_OFFSET] = record.port_latch;
        history_data[base + SST_DPRAM_HISTORY_FB_LATCH_OFFSET] = record.fb_latch;
        history_data[base + SST_DPRAM_HISTORY_FO_A_OFFSET] = record.fo_a;
        history_data[base + SST_DPRAM_HISTORY_FO_B_OFFSET] = record.fo_b;
        history_data[base + SST_DPRAM_HISTORY_FO_C_OFFSET] = record.fo_c;
        history_data[base + SST_DPRAM_HISTORY_MODULE_DISPOSE_OFFSET] = record.module_dispose;
        Put32(&history_data[base + SST_DPRAM_HISTORY_TICK_OFFSET], (uint32_t)record.tick_10ms);
        Put32(&history_data[base + SST_DPRAM_HISTORY_SEQUENCE_OFFSET], (uint32_t)record.sequence);
    }
    BeginPublication(&history_publication);
}
#endif

/*==================================================================
 * 函数功能: 初始化并口扩展发布状态
 * 输入参数: 无
 * 输出参数: 软件状态及三个只读输出头；不写主机请求区
 * 返回值: 无
 * 执行说明: 初始化阶段调用，DPRAM关闭时不执行任何并口访问。
 *================================================================*/
void SstDpramMaintenance_Init(void)
{
#if SST_ENABLE_DPRAM
    response_publication.target = DPRAM_SstResponse;
    response_publication.data = response_data;
    response_publication.count = SST_DPRAM_BLOCK_WORDS;
    telemetry_publication.target = DPRAM_SstTelemetry;
    telemetry_publication.data = telemetry_data;
    telemetry_publication.count = SST_DPRAM_BLOCK_WORDS;
    history_publication.target = DPRAM_SstHistory;
    history_publication.data = history_data;
    history_publication.count = SST_DPRAM_HISTORY_WORDS;
    response_publication.busy = telemetry_publication.busy = history_publication.busy = 0U;
    response_publication.epoch = telemetry_publication.epoch = history_publication.epoch = 0U;
    DPRAM_SstResponse[SST_DPRAM_PUBLICATION_WORD] =
        DPRAM_SstTelemetry[SST_DPRAM_PUBLICATION_WORD] =
            DPRAM_SstHistory[SST_DPRAM_PUBLICATION_WORD] = 1U;
    DPRAM_SstResponse[SST_DPRAM_RESPONSE_MAGIC_WORD] =
        DPRAM_SstTelemetry[SST_DPRAM_TELEMETRY_MAGIC_WORD] =
            DPRAM_SstHistory[SST_DPRAM_HISTORY_MAGIC_WORD] = 0U;
    request_pending = last_valid = notify_response = history_initialized = 0U;
    last_result = 0U;
    ack_sequence = ack_result = ack_operation = ack_point = 0U;
#endif
}

/*==================================================================
 * 函数功能: 标记邮箱21维护请求待后台处理
 * 输入参数: 无
 * 输出参数: request_pending置1；DPRAM关闭时不修改并口状态。
 * 返回值: 无
 * 执行说明: 后台调用；GPIO101 ISR只置原通知标志，不在ISR解析协议。
 *================================================================*/
void SstDpramMaintenance_Request(void)
{
#if SST_ENABLE_DPRAM
    request_pending = 1U;
#endif
}

/*==================================================================
 * 函数功能: 优先回复，其次遥测/历史，每轮发布最多16个数据字
 * 输入参数: 无
 * 输出参数: 并口固定输出区，回复完成后通知邮箱21
 * 返回值: 无
 * 执行说明: 仅主循环调用，所有等待均为跨轮状态，不忙等。
 *================================================================*/
void SstDpramMaintenance_Service(void)
{
#if SST_ENABLE_DPRAM
    if (request_pending && !response_publication.busy)
    {
        ProcessRequest();
        return;
    }
    if (response_publication.busy)
    {
        if (PublishSlice(&response_publication) && notify_response)
        {
            HWREGH(DPRAM_Maibox_Right) = SST_DPRAM_MAINTENANCE_BLOCK;
            notify_response = 0U;
        }
    }
    else if (telemetry_publication.busy)
        PublishSlice(&telemetry_publication);
    else if (history_publication.busy)
        PublishSlice(&history_publication);
#endif
}

/*==================================================================
 * 函数功能: 构建现代状态、量测、故障表及参数回读快照
 * 输入参数: 无
 * 输出参数: RAM发布缓存；如前一发布未完则跳过本轮同类刷新
 * 返回值: 无
 * 执行说明: 旧30ms遥测节拍调用，不增加中断，不声称ISR量测同刻采样。
 *================================================================*/
void SstDpramMaintenance_Publish(void)
{
#if SST_ENABLE_DPRAM
    uint16_t i, /* i：当前请求、参数或发布快照中的 16 位字索引。 */
        flags; /* flags：运行布尔量位图，bit0..8含开关、PWM、PLL、外环、保护及启停给定。 */
    if (!response_publication.busy && !request_pending)
        BuildResponse();
    if (!telemetry_publication.busy)
    {
        for (i = 0U; i < SST_DPRAM_BLOCK_WORDS; ++i)
            telemetry_data[i] = 0U;
        flags = 0U;
        flags = SetTelemetryFlag(flags, g_chb_dab_status.ac_input_closed, SST_DPRAM_FLAG_AC_INPUT_CLOSED);
        flags = SetTelemetryFlag(flags, g_chb_dab_status.precharge_bypassed, SST_DPRAM_FLAG_PRECHARGE_BYPASSED);
        flags = SetTelemetryFlag(flags, g_chb_dab_status.ac_pwm_enabled, SST_DPRAM_FLAG_AC_PWM_ENABLED);
        flags = SetTelemetryFlag(flags, g_chb_dab_status.dab_pwm_enabled, SST_DPRAM_FLAG_DAB_PWM_ENABLED);
        flags = SetTelemetryFlag(flags, g_chb_dab_status.pll_locked, SST_DPRAM_FLAG_PLL_LOCKED);
        flags = SetTelemetryFlag(flags, g_chb_dab_status.voltage_loop_active, SST_DPRAM_FLAG_VOLTAGE_LOOP_ACTIVE);
        flags = SetTelemetryFlag(flags, g_chb_dab_protection_enable, SST_DPRAM_FLAG_PROTECTION_ENABLED);
        flags = SetTelemetryFlag(flags, g_chb_dab_command.run_enable, SST_DPRAM_FLAG_RUN_REQUEST);
        flags = SetTelemetryFlag(flags, g_chb_dab_command.dab_enable, SST_DPRAM_FLAG_DAB_REQUEST);
        telemetry_data[SST_DPRAM_TELEMETRY_MAGIC_WORD] = SST_DPRAM_MAGIC;
        telemetry_data[SST_DPRAM_TELEMETRY_VERSION_WORD] = SST_DPRAM_VERSION;
        telemetry_data[SST_DPRAM_TELEMETRY_STATE_WORD] = g_chb_dab_status.state;
        telemetry_data[SST_DPRAM_TELEMETRY_CONTROL_FAULT_WORD] = g_chb_dab_status.fault_code;
        telemetry_data[SST_DPRAM_TELEMETRY_STARTUP_PHASE_WORD] = g_chb_dab_status.startup_phase;
        telemetry_data[SST_DPRAM_TELEMETRY_FLAGS_WORD] = flags;
        telemetry_data[SST_DPRAM_TELEMETRY_AC_MODE_WORD] = g_chb_dab_command.ac_mode;
        telemetry_data[SST_DPRAM_TELEMETRY_FIRST_FAULT_WORD] = SstFault_GetFirstCode();
        telemetry_data[SST_DPRAM_TELEMETRY_FAULT_LEVEL_WORD] = SstFault_GetLevel();
        telemetry_data[SST_DPRAM_TELEMETRY_ACTIVE_COUNT_WORD] = SstFault_GetActiveCount();
        Put32(&telemetry_data[SST_DPRAM_TELEMETRY_TICK_LOW_WORD], (uint32_t)g_chb_dab_status.tick_10ms);
        Put32(&telemetry_data[SST_DPRAM_TELEMETRY_STARTUP_TAD_LOW_WORD], (uint32_t)g_chb_dab_status.startup_tad_count);
        telemetry_data[SST_DPRAM_TELEMETRY_DC_AVERAGE_WORD] = Encode(g_chb_dab_status.dc_voltage_average_v * 10.0F, 0U);
        telemetry_data[SST_DPRAM_TELEMETRY_DC_MIN_WORD] = Encode(g_chb_dab_status.dc_voltage_min_v * 10.0F, 0U);
        telemetry_data[SST_DPRAM_TELEMETRY_PLL_ANGLE_WORD] = Encode(g_chb_dab_status.pll_angle_rad * 10000.0F, 0U);
        telemetry_data[SST_DPRAM_TELEMETRY_PLL_FREQUENCY_WORD] = Encode(g_chb_dab_status.pll_frequency_hz * 100.0F, 0U);
        telemetry_data[SST_DPRAM_TELEMETRY_ID_FEEDBACK_WORD] = Encode(g_chb_dab_status.id_feedback_a * 100.0F, 1U);
        telemetry_data[SST_DPRAM_TELEMETRY_IQ_FEEDBACK_WORD] = Encode(g_chb_dab_status.iq_feedback_a * 100.0F, 1U);
        telemetry_data[SST_DPRAM_TELEMETRY_ID_REFERENCE_WORD] = Encode(g_chb_dab_status.id_reference_a * 100.0F, 1U);
        telemetry_data[SST_DPRAM_TELEMETRY_STARTUP_ID_REFERENCE_WORD] = Encode(g_chb_dab_status.startup_id_reference_a * 100.0F, 1U);
        telemetry_data[SST_DPRAM_TELEMETRY_STARTUP_VOLTAGE_REFERENCE_WORD] = Encode(g_chb_dab_status.startup_voltage_reference_v * 10.0F, 0U);
        telemetry_data[SST_DPRAM_TELEMETRY_MODULATION_WORD] = Encode(g_chb_dab_status.modulation_actual * 10000.0F, 0U);
        telemetry_data[SST_DPRAM_TELEMETRY_DAB_DUTY_WORD] = g_chb_dab_status.dab_duty_count;
        telemetry_data[SST_DPRAM_TELEMETRY_STORAGE_RESULT_WORD] = SstParams_GetStorageResult();
        telemetry_data[SST_DPRAM_TELEMETRY_STORAGE_PENDING_WORD] = SstParameterStore_IsPending();
        telemetry_data[SST_DPRAM_TELEMETRY_HISTORY_COUNT_WORD] = SstFault_GetHistoryCount();
        for (i = 0U; i < SST_DPRAM_TELEMETRY_ACTIVE_FAULT_COUNT; ++i)
            telemetry_data[SST_DPRAM_TELEMETRY_ACTIVE_FAULT_START_WORD + i] = SstFault_GetCode(i);
        BeginPublication(&telemetry_publication);
    }
    BuildHistory();
#endif
}
