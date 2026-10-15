// 文件职责：声明并口扩展区。NOINIT禁止启动加载镜像，禁用DPRAM时不访问。
#include "SstDpramExtension.h"
#pragma DATA_SECTION(DPRAM_SstRequest, ".DPRAM_SstRequest");
#pragma DATA_SECTION(DPRAM_SstResponse, ".DPRAM_SstResponse");
#pragma DATA_SECTION(DPRAM_SstTelemetry, ".DPRAM_SstTelemetry");
#pragma DATA_SECTION(DPRAM_SstHistory, ".DPRAM_SstHistory");
volatile uint16_t DPRAM_SstRequest[SST_DPRAM_BLOCK_WORDS]; /* DPRAM_SstRequest：主机提交区，固件不主动写入。 */
volatile uint16_t
    DPRAM_SstResponse[SST_DPRAM_BLOCK_WORDS]; /* DPRAM_SstResponse：主机只读的软件执行结果与参数实际值。 */
volatile uint16_t DPRAM_SstTelemetry[SST_DPRAM_BLOCK_WORDS]; /* DPRAM_SstTelemetry：主机只读的后台现代遥测快照。 */
volatile uint16_t DPRAM_SstHistory[SST_DPRAM_HISTORY_WORDS]; /* DPRAM_SstHistory：主机只读的后台历史记录快照。 */
