//###########################################################################
// FILE: EEProm.H
// FUNCTION: BL24C512A读写及原64字节逻辑页接口；不访问X1226的RTC/控制区。
//###########################################################################
#ifndef SST_EEPROM_H
#define SST_EEPROM_H
#include "SstTypes.h"
#include "EepromConfig.h"
#ifndef Uint8
/* Uint8：原兼容接口使用的8位无符号类型。 */
#define Uint8 unsigned char
#endif
/* VALIDFLAG：本驱动调用成功，返回1。 */
#define VALIDFLAG 1U
/* INVALIDFLAG：本驱动调用失败，返回0。 */
#define INVALIDFLAG 0U
/* EEPROM_ADDRESS为含R/W位的写寻址字节，供旧代码查看；不是7位地址。 */
#define EEPROM_ADDRESS SST_EEPROM_DEVICE_WRITE

/* 最近一次底层访问结果：无应答与读到FF是不同情况。 */
#define EEPROM_ERROR_NONE 0U
/* EEPROM_ERROR_BUS_LOW：产生START前总线未能回到高电平。 */
#define EEPROM_ERROR_BUS_LOW 1U
/* EEPROM_ERROR_NO_ACK：器件未对当前字节应答。 */
#define EEPROM_ERROR_NO_ACK 2U
/* EEPROM_ERROR_ADDRESS：地址、长度或缓存参数不合法。 */
#define EEPROM_ERROR_ADDRESS 3U
/* EEPROM_ERROR_WRITE_TIMEOUT：有限ACK轮询后仍未结束内部写忙。 */
#define EEPROM_ERROR_WRITE_TIMEOUT 4U

typedef struct
{
    unsigned short error;       /* 最近一次底层错误，0表示I2C事务正常。 */
    unsigned short address;     /* 最近一次访问的EEPROM绝对字节地址。 */
    unsigned short stage;       /* 0空闲、1器件地址、2地址高字节、3低字节、4数据、5读方向。 */
    unsigned short device_byte; /* 实际发送的含R/W位寻址字节，便于CCS核对接线。 */
} SST_EEPROM_STATUS;

#ifdef __cplusplus
extern "C"
{
#endif
    extern volatile SST_EEPROM_STATUS g_sst_eeprom_status; /* 最近一次I2C访问的可观察诊断状态。 */

    /*==================================================================
 * 函数功能: 释放软件I2C总线，并在上电一次启动CPU周期计数延时时基
 * 输入参数: 无
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    void Eeprom_Initialize(void);
    /*==================================================================
 * 函数功能: 按16位字节地址连续读取；重复START切读方向，最后字节NACK
 * 输入参数: address：EEPROM绝对起始字节地址
 *           data：接收缓存，至少length字节
 *           length：读取字节数，非0且不超过器件末尾
 * 返回值: VALIDFLAG读取完成，此时输出缓存有效；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 Eeprom_Read(unsigned short address, Uint8 *data, unsigned short length);
    /*==================================================================
 * 函数功能: 连续写入，按128字节物理页边界分段并有限轮询写完成
 * 输入参数: address：EEPROM绝对起始字节地址
 *           data：待写字节缓存，至少length字节
 *           length：写入字节数，非0且不超过器件末尾
 * 返回值: VALIDFLAG所有段写完成；INVALIDFLAG范围、总线或应答错误
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 Eeprom_Write(unsigned short address, const Uint8 *data, unsigned short length);
    /*==================================================================
 * 函数功能: 发起一字节写入，不等待最长3ms内部写周期
 * 输入参数: address：EEPROM绝对字节地址
 *           value：待写的单字节值
 * 返回值: VALIDFLAG器件已接收；INVALIDFLAG无应答或总线错误；写完成由调用者等待/校验
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 Eeprom_StartByteWrite(unsigned short address, Uint8 value);

    /*==================================================================
 * 函数功能: 按原64字节逻辑页发起单字节写，供每10ms异步保存使用
 * 输入参数: page：逻辑页号，绝对地址的页部分为page乘64
 *           offset：该逻辑页内字节偏移，调用者按0..63组织
 *           value：待写字节
 * 返回值: 底层VALIDFLAG或INVALIDFLAG；不等待内部写周期
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 r_eeprom_byte_write(Uint8 page, Uint8 offset, Uint8 value);
/*==================================================================
 * 函数功能: 原逻辑页接口的连续段写；不执行X1226解锁或RTC寻址
 * 输入参数: page：原64字节逻辑页号
 *           offset：起始字节偏移
 *           length：连续写入字节数
 *           data：至少length字节的待写缓存
 * 返回值: VALIDFLAG全部写完成；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 r_eeprom_section_write(Uint8 page, Uint8 offset, Uint8 length, Uint8 *data);
/*==================================================================
 * 函数功能: 原逻辑页接口的连续段读，显式返回I2C成功状态
 * 输入参数: page：原64字节逻辑页号
 *           offset：起始字节偏移
 *           length：连续读取字节数，可跨逻辑页
 *           data：至少length字节的接收缓存
 * 返回值: VALIDFLAG读取完成，此时缓存有效；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 r_eeprom_section_read(Uint8 page, Uint8 offset, Uint8 length, Uint8 *data);
/*==================================================================
 * 函数功能: 读取完整的64字节逻辑页；不把逻辑页改为128字节
 * 输入参数: page：原64字节逻辑页号
 *           data：至少64字节的接收缓存
 * 返回值: VALIDFLAG读取完成；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 r_eeprom_page_read(Uint8 page, Uint8 *data);
    /*==================================================================
 * 函数功能: 兼容旧调用的单字节读，失败返回FF；新镜像使用带状态的段读
 * 输入参数: page：原64字节逻辑页号
 *           offset：页内字节偏移
 * 返回值: 读取的数据，失败FF；此接口不能区分有效FF和无应答
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    Uint8 r_eeprom_byte_read(Uint8 page, Uint8 offset);
#ifdef __cplusplus
}
#endif
#endif
