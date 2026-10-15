// 文件职责：实现统一电子标签匹配及六段软件版本读取，不拥有UART或控制状态。
#include "SoftVersion.h"

const SST_SOFT_VERSION g_sst_soft_version = {
    SST_PRODUCT_SERIES,
    SST_PRODUCT_PART,
    SST_HARDWARE_PLATFORM,
    SST_VERSION_BRANCH,
    SST_VERSION_RELEASE,
    SST_VERSION_BETA}; /* g_sst_soft_version：编译期确定的唯一产品/版本数据，前四字段同时用于握手。 */

/*==================================================================
 * 函数功能: 比较上位机四字节电子标签，保留全FF通用连接规则
 * 输入参数: tag：四个线上字节的16位元素数组，每元素范围0..255；空指针不匹配
 * 输出参数: 无
 * 返回值: 1表示四字段精确匹配或全部FF，0表示不匹配
 * 执行说明: 后台AnyEyes解析调用；不是鉴权，不改变控制命令或版本数据。
 *================================================================*/
uint16_t SstVersion_MatchElectronicTag(const uint16_t *tag)
{
    if (!tag)
        return 0U;
    if ((tag[0] == 255U) && (tag[1] == 255U) && (tag[2] == 255U) && (tag[3] == 255U))
        return 1U;
    return (uint16_t)((tag[0] == g_sst_soft_version.product_series) && (tag[1] == g_sst_soft_version.product_part) &&
                      (tag[2] == g_sst_soft_version.hardware_platform) &&
                      (tag[3] == g_sst_soft_version.branch_version));
}

/*==================================================================
 * 函数功能: 按项目、部件、平台、分支、发布、测试顺序复制六段版本
 * 输入参数: bytes：至少六个16位元素的输出数组；空指针不写入
 * 输出参数: bytes每元素承载一个线上低8位字节，禁止直接发送结构体内存
 * 返回值: 无
 * 执行说明: 后台7F握手与7A版本查询共用，不依赖C28x/C6748的结构体布局。
 *================================================================*/
void SstVersion_CopyBytes(uint16_t *bytes)
{
    if (!bytes)
        return;
    bytes[0] = g_sst_soft_version.product_series;
    bytes[1] = g_sst_soft_version.product_part;
    bytes[2] = g_sst_soft_version.hardware_platform;
    bytes[3] = g_sst_soft_version.branch_version;
    bytes[4] = g_sst_soft_version.release_version;
    bytes[5] = g_sst_soft_version.beta_version;
}
