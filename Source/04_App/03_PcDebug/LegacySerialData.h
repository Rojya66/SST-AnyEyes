/* 文件职责: 维护兼容符号；旧 UART ISR 不参与启动。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef LEGACYSERIALDATA_H_
#define LEGACYSERIALDATA_H_
#include "SstTypes.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef union _CRC
{
	unsigned short crc16; /* crc16：CRC 计算缓存的 16 位整字视图。 */
	unsigned char by[2]; /* by：CRC 计算缓存的字节视图，供高低字节访问。 */
} CRC;

extern unsigned char Flag_VofaSend; /* Flag_VofaSend：保留原 VOFA 发送通知变量；迁移发送由后台时基调度。 */

extern unsigned char Flag_Uart; /* Flag_Uart：旧 UART 维护帧兼容状态，保留符号供原路径回读。 */

extern unsigned char CommNum; /* CommNum：旧 UART 兼容通信序号。 */

extern unsigned char CommFunc; /* CommFunc：旧 UART 兼容功能码。 */

extern unsigned int StartAddr; /* StartAddr：旧 UART 兼容请求的起始地址。 */

extern unsigned short ReadLength; /* ReadLength：旧 UART 兼容读取长度。 */

extern unsigned char CommLength; /* CommLength：旧 UART 兼容帧有效长度。 */

extern unsigned char CommInNum; /* CommInNum：旧 UART 兼容帧当前接收位置。 */

extern unsigned char CommTotalNum; /* CommTotalNum：旧 UART 兼容帧预期总长度。 */

extern unsigned char WriteOK; /* WriteOK：旧 UART 兼容写入成功标志。 */

extern unsigned char PCuartBUFF[140]; /* PCuartBUFF：旧 PC UART 帧兼容缓存，容量 140 字节。 */

extern unsigned char ReceiveuartBUFF[11]; /* ReceiveuartBUFF：旧 UART 接收帧兼容缓存，容量 11 字节。 */


#ifdef __cplusplus
}
#endif
#endif
