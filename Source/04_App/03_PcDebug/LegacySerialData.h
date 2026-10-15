/*======================================================================================*
 * 文件名  : LegacySerialData.h
 * 功能说明: 声明旧 UART 兼容变量及 CRC 联合体，数据定义位于 LegacySerialData.c。
 *=====================================================================================*/

#ifndef LEGACYSERIALDATA_H_
#define LEGACYSERIALDATA_H_
#include "SstTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*======================================== 接口类型定义 =====================================*/
    /* CRC：保留旧通信的 CRC16 整字/字节联合视图，不参与当前协议解析。 */
    typedef union _CRC
    {
        unsigned short crc16; /* CRC 结果的 16 位整字视图。 */
        unsigned char by[2]; /* CRC 结果的两个字节视图，沿用目标平台字节序。 */
    } CRC;

    /*======================================== 兼容变量声明 =====================================*/
    extern unsigned char Flag_VofaSend; /* 旧 VOFA 发送通知标志；当前波形周期由后台节拍调度。 */
    extern unsigned char Flag_Uart; /* 旧 UART 帧状态兼容变量；当前不启用旧维护帧解析器。 */
    extern unsigned char CommNum; /* 旧 UART 帧通信序号兼容变量。 */
    extern unsigned char CommFunc; /* 旧 UART 帧功能码兼容变量。 */
    extern unsigned int StartAddr; /* 旧 UART 请求起始地址兼容变量，不作为当前任意 RAM 访问入口。 */
    extern unsigned short ReadLength; /* 旧 UART 请求读取长度兼容变量。 */
    extern unsigned char CommLength; /* 旧 UART 帧有效长度兼容变量，单位为字节。 */
    extern unsigned char CommInNum; /* 旧 UART 接收位置兼容变量，单位为字节。 */
    extern unsigned char CommTotalNum; /* 旧 UART 预期帧长兼容变量，单位为字节。 */
    extern unsigned char WriteOK; /* 旧 UART 写入完成标志兼容变量。 */
    extern unsigned char PCuartBUFF[140]; /* 旧 PC UART 帧缓存，容量为 140 字节。 */
    extern unsigned char ReceiveuartBUFF[11]; /* 旧 UART 接收帧缓存，容量为 11 字节。 */

#ifdef __cplusplus
}
#endif
#endif /* LEGACYSERIALDATA_H_ */
