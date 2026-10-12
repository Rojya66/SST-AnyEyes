/* 文件职责: 维护兼容符号；旧 UART ISR 不参与启动；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "LegacySerialData.h"
#include "SstControlConfig.h"

unsigned char Flag_VofaSend; /* Flag_VofaSend：保留原 VOFA 发送通知变量；迁移发送由后台时基调度。 */

unsigned char Flag_Uart; /* Flag_Uart：旧 UART 维护帧兼容状态，保留符号供原路径回读。 */

unsigned char CommNum; /* CommNum：旧 UART 兼容通信序号。 */

unsigned char CommFunc; /* CommFunc：旧 UART 兼容功能码。 */

unsigned int StartAddr; /* StartAddr：旧 UART 兼容请求的起始地址。 */

unsigned short ReadLength; /* ReadLength：旧 UART 兼容读取长度。 */

unsigned char CommLength; /* CommLength：旧 UART 兼容帧有效长度。 */

unsigned char CommInNum; /* CommInNum：旧 UART 兼容帧当前接收位置。 */

unsigned char CommTotalNum; /* CommTotalNum：旧 UART 兼容帧预期总长度。 */

unsigned char WriteOK; /* WriteOK：旧 UART 兼容写入成功标志。 */

unsigned char PCuartBUFF[140]; /* PCuartBUFF：旧 PC UART 帧兼容缓存，容量 140 字节。 */

unsigned char ReceiveuartBUFF[11]; /* ReceiveuartBUFF：旧 UART 接收帧兼容缓存，容量 11 字节。 */

