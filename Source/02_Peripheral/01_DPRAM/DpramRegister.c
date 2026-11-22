/* 旧记录器兼容缓存。V2 并口映射由 SstDpramHost 独立拥有。 */
#include "DpramRegister.h"
#include "SstControlConfig.h"

volatile XXG_DPRAM_PluginUnit
    DPRAM_Block4_PluginUnit2, /* DPRAM_Block4_PluginUnit2：旧版的插件 2 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit3, /* DPRAM_Block4_PluginUnit3：旧版的插件 3 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit4, /* DPRAM_Block4_PluginUnit4：旧版的插件 4 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit5, /* DPRAM_Block4_PluginUnit5：旧版的插件 5 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit6; /* DPRAM_Block4_PluginUnit6：旧版的插件 6 遥测块，保留旧类型。 */

volatile XXG_DPRAM_PluginUnit
    DPRAM_Block4_PluginUnit7, /* DPRAM_Block4_PluginUnit7：旧版的插件 7 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit8, /* DPRAM_Block4_PluginUnit8：旧版的插件 8 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit9, /* DPRAM_Block4_PluginUnit9：旧版的插件 9 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit10, /* DPRAM_Block4_PluginUnit10：旧版的插件 10 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit11; /* DPRAM_Block4_PluginUnit11：旧版的插件 11 遥测块，保留旧类型。 */

volatile XXG_DPRAM_PluginUnit
    DPRAM_Block4_PluginUnit12, /* DPRAM_Block4_PluginUnit12：旧版的插件 12 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit13, /* DPRAM_Block4_PluginUnit13：旧版的插件 13 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit14, /* DPRAM_Block4_PluginUnit14：旧版的插件 14 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit15, /* DPRAM_Block4_PluginUnit15：旧版的插件 15 遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit16, /* DPRAM_Block4_PluginUnit16：旧版的插件16遥测块，保留旧类型。 */
    DPRAM_Block4_PluginUnit17; /* DPRAM_Block4_PluginUnit17：旧版的插件 17 遥测块，保留旧类型。 */

volatile XXG_DPRAM_RemMeaSig
    DPRAM_Block3_remMeaSig; /* DPRAM_Block3_remMeaSig：旧版的端口遥测区；访问受 SST_ENABLE_DPRAM 门控。 */

volatile XXG_DPRAM_RemControl
    DPRAM_Block1_remControl; /* DPRAM_Block1_remControl：旧版的遥控区；访问受 SST_ENABLE_DPRAM 门控。 */

volatile XXG_DPRAM_RemRegulate
    DPRAM_Block2_remRegulate; /* DPRAM_Block2_remRegulate：旧版的遥调区；访问受 SST_ENABLE_DPRAM 门控。 */
