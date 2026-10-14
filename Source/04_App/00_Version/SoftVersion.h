// 文件职责：集中配置SST电子标签与六段软件版本，供AnyEyes匹配和版本应答共用。
#ifndef SST_SOFT_VERSION_H_
#define SST_SOFT_VERSION_H_
#include <stdint.h>

#ifndef SST_PRODUCT_SERIES
/* SST_PRODUCT_SERIES：产品号/项目号，默认255，须与AnyEyes产品配置一致。 */
#define SST_PRODUCT_SERIES 255U
#endif
#ifndef SST_PRODUCT_PART
/* SST_PRODUCT_PART：产品部件号，默认43，保留现有AnyEyes部件匹配。 */
#define SST_PRODUCT_PART 43U
#endif
#ifndef SST_HARDWARE_PLATFORM
/* SST_HARDWARE_PLATFORM：上位机电子标签的硬件平台号，默认1，不是DSP型号。 */
#define SST_HARDWARE_PLATFORM 1U
#endif
#ifndef SST_VERSION_BRANCH
/* SST_VERSION_BRANCH：电子标签其他标记及软件分支号，默认0表示主分支。 */
#define SST_VERSION_BRANCH 0U
#endif
#ifndef SST_VERSION_RELEASE
/* SST_VERSION_RELEASE：对外发布版本号，默认1，沿用迁移后SST版本。 */
#define SST_VERSION_RELEASE 1U
#endif
#ifndef SST_VERSION_BETA
/* SST_VERSION_BETA：内部测试版本号，默认0，沿用迁移后SST版本。 */
#define SST_VERSION_BETA 0U
#endif
/* SST_TAG_FIELD_COUNT：FF握手载荷的电子标签字段数，每字段一个线上字节。 */
#define SST_TAG_FIELD_COUNT 4U
/* SST_VERSION_FIELD_COUNT：7F与7A共用的六段软件版本字段数。 */
#define SST_VERSION_FIELD_COUNT 6U

#if (SST_PRODUCT_SERIES < 0) || (SST_PRODUCT_SERIES > 255) || \
    (SST_PRODUCT_PART < 0) || (SST_PRODUCT_PART > 255) || \
    (SST_HARDWARE_PLATFORM < 0) || (SST_HARDWARE_PLATFORM > 255) || \
    (SST_VERSION_BRANCH < 0) || (SST_VERSION_BRANCH > 255) || \
    (SST_VERSION_RELEASE < 0) || (SST_VERSION_RELEASE > 255) || \
    (SST_VERSION_BETA < 0) || (SST_VERSION_BETA > 255)
#error "SST electronic tag and version fields must be integers in 0..255"
#endif

typedef struct
{
    uint8_t product_series; /* product_series：产品号/项目号，握手标签第1字段。 */
    uint8_t product_part; /* product_part：产品部件号，握手标签第2字段。 */
    uint8_t hardware_platform; /* hardware_platform：硬件平台匹配号，握手标签第3字段。 */
    uint8_t branch_version; /* branch_version：其他标记/软件分支号，握手标签第4字段。 */
    uint8_t release_version; /* release_version：对外发布版本号，版本应答第5字段。 */
    uint8_t beta_version; /* beta_version：内部测试版本号，版本应答第6字段。 */
} SST_SOFT_VERSION;

#ifdef __cplusplus
extern "C" {
#endif
extern const SST_SOFT_VERSION g_sst_soft_version; /* g_sst_soft_version：唯一只读产品/版本数据，不接入参数写入或EEPROM。 */

/*==================================================================
 * 函数功能: 比较上位机四字节电子标签，保留全FF通用连接规则
 * 输入参数: tag：四个线上字节的16位元素数组，每元素范围0..255；空指针不匹配
 * 输出参数: 无
 * 返回值: 1表示四字段精确匹配或全部FF，0表示不匹配
 * 执行说明: 后台AnyEyes解析调用；不是鉴权，不改变控制命令或版本数据。
 *================================================================*/
uint16_t SstVersion_MatchElectronicTag(const uint16_t *tag);

/*==================================================================
 * 函数功能: 按项目、部件、平台、分支、发布、测试顺序复制六段版本
 * 输入参数: bytes：至少六个16位元素的输出数组；空指针不写入
 * 输出参数: bytes每元素承载一个线上低8位字节，禁止直接发送结构体内存
 * 返回值: 无
 * 执行说明: 后台7F握手与7A版本查询共用，不依赖C28x/C6748的结构体布局。
 *================================================================*/
void SstVersion_CopyBytes(uint16_t *bytes);
#ifdef __cplusplus
}
#endif
#endif
