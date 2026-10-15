//###########################################################################
// FILE: SstParameterStore.c
// FUNCTION: EEPROM页6/7的v2镜像及分步保存/整体校验加载事务；兼容v1加载。
//           参数表归SstParameterTable；停机判断归SstCommand。
//###########################################################################
#include <string.h>
#include "SstParameterStore.h"
#include "SstParameterTable.h"
#include "SstLegacyParameters.h"
#include "SstCommand.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "EEProm.H"
static uint16_t storageResult, /* storageResult：最近一次参数保存/加载的 SST_PARAM_* 结果。 */
    storagePending, /* storagePending：异步参数保存正在执行的标志。 */
    storagePosition; /* storagePosition：参数镜像写入/提交/读回校验所处的步序。 */
static uint32_t storageTick; /* storageTick：上次推进参数存储任务的 10 ms 节拍。 */
/* SST_STORE_V1_LENGTH：旧版本参数镜像长度，92字节，载荷为42字。 */
#define SST_STORE_V1_LENGTH 92U
/* SST_STORE_SNAPSHOT_WORDS：v2镜像字数，42现代字及8原保护/预充字；不保存运行期保护旁路。 */
#define SST_STORE_SNAPSHOT_WORDS 50U
/* SST_STORE_LENGTH：v2页6/7参数镜像总长度，108字节。 */
#define SST_STORE_LENGTH (8U + 2U * SST_STORE_SNAPSHOT_WORDS)
static uint8_t storageImage[SST_STORE_LENGTH]; /* storageImage：页6/7的v2镜像，包含魔数、版本、50字载荷及CRC。 */

/*==================================================================
 * 函数功能: 查询 AnyEyes 与 DPRAM 共用参数存储服务的忙状态
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 1 表示存储正在进行，0 表示空闲
 * 执行说明: 只读取当前存储状态，不访问 EEPROM。
 *================================================================*/
uint16_t SstParameterStore_IsBusy(void)
{
    return storagePending;
}

/*==================================================================
 * 函数功能: 读取本参数镜像事务正在保存的标志
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 1表示参数镜像事务进行中，0表示空闲
 * 执行说明: 只读取当前存储状态，不访问 EEPROM。
 *================================================================*/
uint16_t SstParameterStore_IsPending(void)
{
    return storagePending;
}

/*==================================================================
 * 函数功能: 计算参数存储镜像的 CRC16，初值 FFFF、多项式 A001
 * 输入参数: data：参数镜像的起始地址，至少包含 count 个字节
 *           count：参与校验的有效字节数
 * 输出参数: 无。
 * 返回值: 16 位 CRC 校验值
 * 执行说明: 依次处理 count 个字节；调用者保证缓存及长度有效。
 *================================================================*/
static uint16_t StoreCrc(const uint8_t *data, uint16_t count)
{
    uint16_t crc = 0xFFFF, /* crc：逐字节、逐位累加的 CRC 校验状态。 */
        i, /* i：参与 CRC 校验的镜像字节索引。 */
        b; /* b：当前字节的 CRC 位循环序号，范围 0..7。 */
    for (i = 0; i < count; ++i)
    {
        crc ^= data[i];
        for (b = 0; b < 8; ++b)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
}

/*==================================================================
 * 函数功能: 按小端顺序把一个 16 位字写入 storageImage
 * 输入参数: offset：storageImage 中低字节的偏移，offset+1 必须在镜像范围内
 *           value：待编码的 16 位参数字
 * 输出参数: storageImage 中 offset、offset+1 两字节。
 * 返回值: 无
 * 执行说明: 只修改 RAM 镜像；实际 EEPROM 写入由 Service 分步完成。
 *================================================================*/
static void StorePut(uint16_t offset, uint16_t value)
{
    storageImage[offset] = (uint8_t)value;
    storageImage[offset + 1] = (uint8_t)(value >> 8);
}

/*==================================================================
 * 函数功能: 按小端顺序从 storageImage 读取一个 16 位字
 * 输入参数: offset：storageImage 中低字节的偏移，offset+1 必须在镜像范围内
 * 输出参数: 无。
 * 返回值: 镜像中的 16 位字
 * 执行说明: 只读取 RAM 镜像；调用者保证偏移合法。
 *================================================================*/
static uint16_t StoreGet(uint16_t offset)
{
    return (uint16_t)storageImage[offset] | ((uint16_t)storageImage[offset + 1] << 8);
}

/*==================================================================
 * 函数功能: 停机时读取页6/7的v1/v2镜像，整体校验后恢复可持久化参数
 * 输入参数: 无
 * 输出参数: 校验通过后更新参数表及 v2 原保护/预充参数；任何校验失败均不应用镜像。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 参数镜像服务在初始化或主循环中调用。
 *================================================================*/
static uint16_t StoreLoad(void)
{
    uint16_t i, /* i：镜像读取及快照解码的索引。 */
        result, /* result：现代及原保护/预充快照的整体校验结果。 */
        length, /* length：由已识别版本决定的合法镜像字节数。 */
        words; /* words：本次版本的快照字数，v1为42、v2为50。 */
    uint16_t snapshot[SST_STORE_SNAPSHOT_WORDS]; /* snapshot：现代参数及v2原保护/预充编码。 */
    if (!SstCommand_IsStopped())
        return SST_PARAM_BUSY;
    /* 先读6字节头并识别固定版本；不接受EEPROM给出的任意长度。 */
    for (i = 0; i < 6U; ++i)
        storageImage[i] = r_eeprom_byte_read((uint8_t)(6 + i / 64), (uint8_t)(i % 64));
    if (storageImage[0] != 0x53 || storageImage[1] != 0x54)
        return SST_PARAM_STORAGE;
    if (storageImage[2] == 1U && storageImage[3] == SST_STORE_V1_LENGTH)
    {
        length = SST_STORE_V1_LENGTH;
        words = 42U;
    }
    else if (storageImage[2] == 2U && storageImage[3] == SST_STORE_LENGTH)
    {
        length = SST_STORE_LENGTH;
        words = SST_STORE_SNAPSHOT_WORDS;
    }
    else
        return SST_PARAM_STORAGE;
    for (i = 6U; i < length; ++i)
        storageImage[i] = r_eeprom_byte_read((uint8_t)(6 + i / 64), (uint8_t)(i % 64));
    if (StoreGet((uint16_t)(length - 2U)) != StoreCrc(storageImage, (uint16_t)(length - 2U)))
        return SST_PARAM_STORAGE;
    /* 先解码并整体校验；v2任一原保护参数非法时，现代参数也不能部分生效。 */
    for (i = 0; i < words; ++i)
        snapshot[i] = StoreGet((uint16_t)(6 + 2 * i));
    result = SstParameterTable_SnapshotValidate(snapshot);
    if (result != SST_PARAM_OK)
        return result;
    if (words == SST_STORE_SNAPSHOT_WORDS)
    {
        result = SstLegacyParameters_ValidateSnapshot(&snapshot[42]);
        if (result != SST_PARAM_OK)
            return result;
    }
    SstParameterTable_SnapshotApply(snapshot);
    if (words == SST_STORE_SNAPSHOT_WORDS)
    {
        SstLegacyParameters_ApplySnapshot(&snapshot[42]);
    }
    return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 在停机且存储空闲时执行加载，或建立异步保存镜像
 * 输入参数: load：0 请求保存，非零请求加载
 * 输出参数: 加载时恢复活动参数；保存时建立 storageImage 并置 storagePending，不立即写 EEPROM。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 参数镜像服务在初始化或主循环中调用。
 *================================================================*/
uint16_t SstParameterStore_Request(uint16_t load)
{
    uint16_t i, /* i：参数快照、原保护/预充项及镜像遍历索引。 */
        result; /* result：完整保存快照的范围校验结果。 */
    uint16_t snapshot[SST_STORE_SNAPSHOT_WORDS]; /* snapshot：保存前整体校验的50字编码快照。 */
    if (SstParameterStore_IsBusy() || !SstCommand_IsStopped())
        return SST_PARAM_BUSY;
    if (load)
    {
        storageResult = StoreLoad();
        return storageResult;
    }
    for (i = 0; i < 42U; ++i)
        snapshot[i] = SstParameterTable_SnapshotRead(i);
    for (i = 0; i < 8U; ++i)
        snapshot[42U + i] = SstLegacyParameters_Read(i);
    result = SstParameterTable_SnapshotValidate(snapshot);
    if (result != SST_PARAM_OK)
        return result;
    result = SstLegacyParameters_ValidateSnapshot(&snapshot[42]);
    if (result != SST_PARAM_OK)
        return result;
    memset(storageImage, 0, sizeof(storageImage));
    storageImage[0] = 0x53;
    storageImage[1] = 0x54;
    storageImage[2] = 2;
    storageImage[3] = SST_STORE_LENGTH;
    for (i = 0; i < SST_STORE_SNAPSHOT_WORDS; ++i)
        StorePut((uint16_t)(6 + 2 * i), snapshot[i]);
    StorePut(SST_STORE_LENGTH - 2U, StoreCrc(storageImage, SST_STORE_LENGTH - 2U));
    storagePending = 1;
    storagePosition = 0;
    storageResult = SST_PARAM_BUSY;
    storageTick = (uint32_t)g_chb_dab_status.tick_10ms - 1;
    return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 每 10 ms 推进一步镜像保存：先使旧镜像无效，写入并提交后逐字节读回校验
 * 输入参数: 无
 * 输出参数: 更新 EEPROM 镜像、storagePosition、storagePending 和 storageResult。
 * 返回值: 无
 * 执行说明: 参数镜像服务在初始化或主循环中调用。
 *================================================================*/
void SstParameterStore_Service(void)
{
    uint32_t now =
        (uint32_t)g_chb_dab_status.tick_10ms; /* now：当前 10 ms 节拍，用于限制每拍只执行一个 EEPROM 字节操作。 */
    uint16_t offset; /* offset：本拍写入或读回的镜像字节偏移。 */
    uint8_t value, /* value：本拍写入 EEPROM 的单字节值。 */
        ok; /* ok：EEPROM 单字节写入的 VALIDFLAG/INVALIDFLAG 结果。 */
    if (!storagePending || now == storageTick)
        return;
    storageTick = now;
    if (!SstCommand_IsStopped())
    {
        storagePending = 0;
        storageResult = SST_PARAM_BUSY;
        return;
    }
    /* 先清除魔数，写完其余字节后恢复魔数，再逐字节读回校验。
     * 每个 10 ms 节拍只处理一个字节，不在本服务内等待 EEPROM 写周期。 */
    if (storagePosition == 0)
    {
        offset = 0;
        value = 0;
    }
    else if (storagePosition < SST_STORE_LENGTH)
    {
        offset = storagePosition;
        value = storageImage[offset];
    }
    else if (storagePosition == SST_STORE_LENGTH)
    {
        offset = 0;
        value = storageImage[0];
    }
    else
    {
        offset = (uint16_t)(storagePosition - SST_STORE_LENGTH - 1);
        if (r_eeprom_byte_read((uint8_t)(6 + offset / 64), (uint8_t)(offset % 64)) != storageImage[offset])
        {
            storagePending = 0;
            storageResult = SST_PARAM_STORAGE;
            return;
        }
        ++storagePosition;
        if (offset == SST_STORE_LENGTH - 1)
        {
            storagePending = 0;
            storageResult = SST_PARAM_OK;
        }
        return;
    }
    ok = r_eeprom_byte_write((uint8_t)(6 + offset / 64), (uint8_t)(offset % 64), value);
    if (ok != VALIDFLAG)
    {
        storagePending = 0;
        storageResult = SST_PARAM_STORAGE;
        return;
    }
    ++storagePosition;
}

/*==================================================================
 * 函数功能: 读取最近一次参数保存/加载的结果
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: SST_PARAM_* 结果码
 * 执行说明: 只读取当前存储状态，不访问 EEPROM。
 *================================================================*/
uint16_t SstParameterStore_GetResult(void)
{
    return storageResult;
}

/*==================================================================
 * 函数功能: 在初始化阶段尝试恢复参数并记录存储结果
 * 输入参数: 无
 * 输出参数: 恢复有效持久化参数，并将加载结果记录到 storageResult。
 * 返回值: 无
 * 执行说明: 参数镜像服务在初始化或主循环中调用。
 *================================================================*/
void SstParameterStore_LoadPersistent(void)
{
    storageResult = StoreLoad();
}
