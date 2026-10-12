#include "DpramRegister.h"
#include "SstTypes.h"
/* 文件职责: DPRAM 遥控、遥调及轮询兼容数据；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "DpramHostData.h"
#include "SstControlConfig.h"

XXG_DPRAM_RemMeaSig PORT_RealTime_DATA_buf; /* PORT_RealTime_DATA_buf：端口遥测的 16 位定点打包缓存。 */

XXG_DPRAM_RemControl PORT_Control_DATA_Buf; /* PORT_Control_DATA_Buf：旧遥控协议的本地字缓存。 */

XXG_COM_START_PORT Port_COM_START; /* Port_COM_START：旧遥控启动字的位域解码缓存。 */

XXG_COM_STOP_PORT Port_COM_STOP; /* Port_COM_STOP：旧遥控停止字的位域解码缓存。 */

XXG_DPRAM_RemRegulate PORT_Regulate_DATA_Buf; /* PORT_Regulate_DATA_Buf：旧遥调协议的本地定点字缓存。 */

DPRAM_RemRegulateFloat PORT_Regulate_DATA; /* PORT_Regulate_DATA：旧遥调定点值换算后的浮点给定缓存。 */

unsigned char Flag_RemoteControlRegulate; /* Flag_RemoteControlRegulate：DPRAM 通知 ISR 置位、后台消费的遥控/遥调待处理标志。 */

ushort FLAG_NEW_R; /* FLAG_NEW_R：DPRAM 向对端发布的新数据块标识。 */

ushort FLAG_NEW_L; /* FLAG_NEW_L：DPRAM 从对端读取的新数据块标识。 */

unsigned char WriteBlock_Index; /* WriteBlock_Index：后台 DPRAM 插件遥测轮询的当前块号。 */

ushort rtt; /* rtt：DPRAM 收到通知后的插件 LED 翻转状态。 */

