#ifndef SST_DPRAM_EXTENSION_H_
#define SST_DPRAM_EXTENSION_H_
#include <stdint.h>
/* 并口扩展每小块128个16位字，故障历史块512字；物理地址由链接脚本固定。 */
/* SST_DPRAM_BLOCK_WORDS：三个小扩展区各128个16位字，地址由链接脚本固定。 */
#define SST_DPRAM_BLOCK_WORDS 128U
/* SST_DPRAM_HISTORY_WORDS：详细故障历史区容量512个16位字。 */
#define SST_DPRAM_HISTORY_WORDS 512U
/* 左邮箱请求与右邮箱回复均使用21，不占用旧块号1..20。 */
/* SST_DPRAM_MAINTENANCE_BLOCK：扩展请求/回复邮箱块号21，旧块号1..20保持。 */
#define SST_DPRAM_MAINTENANCE_BLOCK 21U
/* 请求格式：magic、版本、序号、操作、点号、值、五个命令字、保留、CRC、序号副本、提交标记、保留。 */
/* SST_DPRAM_REQUEST_WORDS：单个请求长度16字，含CRC、序号副本及最后提交标記。 */
#define SST_DPRAM_REQUEST_WORDS 16U
/* SST_DPRAM_MAGIC：扩展区识别字0x5353。 */
#define SST_DPRAM_MAGIC 0x5353U /* 扩展区识别字。 */
/* SST_DPRAM_VERSION：扩展点表及请求格式版本1。 */
#define SST_DPRAM_VERSION 1U /* 当前扩展点表与请求格式版本。 */
/* SST_DPRAM_COMMIT：主机最后写入的请求提交标记0xA55A。 */
#define SST_DPRAM_COMMIT 0xA55AU /* 主机最后写入的请求提交标记。 */
/* SST_DPRAM_POINT_COUNT：固定维护点表54点：命令9点、现代参数37点、原保护/巡检8点。 */
#define SST_DPRAM_POINT_COUNT 54U /* 命令9点、现代参数37点、原保护/巡检8点。 */
/* SST_DPRAM_FRAME_ERROR：格式、CRC或同序号不同载荷错误结果码7。 */
#define SST_DPRAM_FRAME_ERROR 7U /* 扩展帧格式、CRC或重复序号载荷冲突。 */
#ifdef __cplusplus
extern "C" {
#endif
extern volatile uint16_t DPRAM_SstRequest[SST_DPRAM_BLOCK_WORDS]; /* DPRAM_SstRequest：主机写请求，DSP只读；0x66002C00。 */
extern volatile uint16_t DPRAM_SstResponse[SST_DPRAM_BLOCK_WORDS]; /* DPRAM_SstResponse：DSP回复及54点实际回读；0x66002D00。 */
extern volatile uint16_t DPRAM_SstTelemetry[SST_DPRAM_BLOCK_WORDS]; /* DPRAM_SstTelemetry：现代状态、测量、当前故障表；0x66002E00。 */
extern volatile uint16_t DPRAM_SstHistory[SST_DPRAM_HISTORY_WORDS]; /* DPRAM_SstHistory：32条详细RAM历史；0x66002F00。 */
#ifdef __cplusplus
}
#endif
#endif
