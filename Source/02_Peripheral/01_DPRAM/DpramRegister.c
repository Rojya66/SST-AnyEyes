/* 文件职责: DPRAM 固定映射与原块协议布局；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "DpramRegister.h"
#include "SstControlConfig.h"
#pragma DATA_SECTION(DPRAM_Block1_remControl,".DPRAM_Block1_remControl");
#pragma DATA_SECTION(DPRAM_Block2_remRegulate,".DPRAM_Block2_remRegulate");
#pragma DATA_SECTION(DPRAM_Block3_remMeaSig,".DPRAM_Block3_remMeaSig");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit2,".DPRAM_Block4_PluginUnit2");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit3,".DPRAM_Block4_PluginUnit3");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit4,".DPRAM_Block4_PluginUnit4");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit5,".DPRAM_Block4_PluginUnit5");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit6,".DPRAM_Block4_PluginUnit6");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit7,".DPRAM_Block4_PluginUnit7");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit8,".DPRAM_Block4_PluginUnit8");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit9,".DPRAM_Block4_PluginUnit9");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit10,".DPRAM_Block4_PluginUnit10");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit11,".DPRAM_Block4_PluginUnit11");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit12,".DPRAM_Block4_PluginUnit12");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit13,".DPRAM_Block4_PluginUnit13");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit14,".DPRAM_Block4_PluginUnit14");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit15,".DPRAM_Block4_PluginUnit15");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit16,".DPRAM_Block4_PluginUnit16");
#pragma DATA_SECTION(DPRAM_Block4_PluginUnit17,".DPRAM_Block4_PluginUnit17");

volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit2,DPRAM_Block4_PluginUnit3,DPRAM_Block4_PluginUnit4,DPRAM_Block4_PluginUnit5,DPRAM_Block4_PluginUnit6; /* DPRAM_Block4_PluginUnit2：原地址映射的插件 2 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit3：原地址映射的插件 3 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit4：原地址映射的插件 4 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit5：原地址映射的插件 5 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit6：原地址映射的插件 6 遥测块，仅启用 DPRAM 时访问。 */

volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit7,DPRAM_Block4_PluginUnit8,DPRAM_Block4_PluginUnit9,DPRAM_Block4_PluginUnit10,DPRAM_Block4_PluginUnit11; /* DPRAM_Block4_PluginUnit7：原地址映射的插件 7 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit8：原地址映射的插件 8 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit9：原地址映射的插件 9 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit10：原地址映射的插件 10 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit11：原地址映射的插件 11 遥测块，仅启用 DPRAM 时访问。 */

volatile XXG_DPRAM_PluginUnit DPRAM_Block4_PluginUnit12,DPRAM_Block4_PluginUnit13,DPRAM_Block4_PluginUnit14,DPRAM_Block4_PluginUnit15,DPRAM_Block4_PluginUnit16,DPRAM_Block4_PluginUnit17; /* DPRAM_Block4_PluginUnit12：原地址映射的插件 12 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit13：原地址映射的插件 13 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit14：原地址映射的插件 14 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit15：原地址映射的插件 15 遥测块，仅启用 DPRAM 时访问。 */

/* DPRAM_Block4_PluginUnit16：原地址映射的插件16遥测块，仅启用DPRAM时访问。 */

/* DPRAM_Block4_PluginUnit17：原地址映射的插件 17 遥测块，仅启用 DPRAM 时访问。 */

volatile XXG_DPRAM_RemMeaSig DPRAM_Block3_remMeaSig; /* DPRAM_Block3_remMeaSig：原地址映射的端口遥测区；访问受 SST_ENABLE_DPRAM 门控。 */

volatile XXG_DPRAM_RemControl DPRAM_Block1_remControl; /* DPRAM_Block1_remControl：原地址映射的遥控区；访问受 SST_ENABLE_DPRAM 门控。 */

volatile XXG_DPRAM_RemRegulate DPRAM_Block2_remRegulate; /* DPRAM_Block2_remRegulate：原地址映射的遥调区；访问受 SST_ENABLE_DPRAM 门控。 */

