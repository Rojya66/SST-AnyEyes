/*======================================================================================*
 * 文件名  : LegacySerialData.c
 * 功能说明: 集中定义旧 UART 通信兼容变量，保留符号供初始化和仿真器观察。
 *=====================================================================================*/

/*======================================== 文件包含 =====================================*/
#include "LegacySerialData.h"
#include "SstControlConfig.h"

/*======================================== 兼容变量定义 =====================================*/
unsigned char Flag_VofaSend; /* 旧 VOFA 发送通知标志；当前波形周期由后台节拍调度。 */
unsigned char Flag_Uart; /* 旧 UART 帧状态兼容变量；当前不启用旧维护帧解析器。 */
unsigned char CommNum; /* 旧 UART 帧通信序号兼容变量。 */
unsigned char CommFunc; /* 旧 UART 帧功能码兼容变量。 */
unsigned int StartAddr; /* 旧 UART 请求起始地址兼容变量，不作为当前任意 RAM 访问入口。 */
unsigned short ReadLength; /* 旧 UART 请求读取长度兼容变量。 */
unsigned char CommLength; /* 旧 UART 帧有效长度兼容变量，单位为字节。 */
unsigned char CommInNum; /* 旧 UART 接收位置兼容变量，单位为字节。 */
unsigned char CommTotalNum; /* 旧 UART 预期帧长兼容变量，单位为字节。 */
unsigned char WriteOK; /* 旧 UART 写入完成标志兼容变量。 */
unsigned char PCuartBUFF[140]; /* 旧 PC UART 帧缓存，容量为 140 字节。 */
unsigned char ReceiveuartBUFF[11]; /* 旧 UART 接收帧缓存，容量为 11 字节。 */
