// TI File $Revision: /main/1 $
// Checkin $Date: August 18, 2006   13:51:50 $
//###########################################################################
//
// FILE:   EEProm.h
//
// TITLE:  EEProm & Time Read/Write
// AUTHOR: Li Kai
//
//###########################################################################
// $TI Release: DSP2833x Header Files V1.10 $
// $Release Date: February 15, 2008 $
//###########################################################################

#ifndef _EEPROM_H_
#define _EEPROM_H_

#ifdef __cplusplus
extern "C"
{ // 按 C 语言链接导出接口，供 C++ 通信模块调用。
#endif

/****************************************************************************/
/*                                                                          */
/*              宏定义                                                                                                                                                         */
/*                                                                          */
/****************************************************************************/
/* Uint8：旧代码无符号 8 位整数类型别名。 */
#define Uint8 unsigned char

//IIC
//#define X4643	//使用X4643
#ifdef X4643
/* EEPROM_ADDRESS：EEPROM 软件 I²C 器件寻址字节，保持原器件定义。 */
#define EEPROM_ADDRESS 0xA0
/* CONTROL_ADDRESS：EEPROM/RTC 控制区软件 I²C 器件寻址字节。 */
#define CONTROL_ADDRESS 0xA0
/* CONTROL_ADDRESS_HIGH：EEPROM/RTC 控制寄存器地址高字节。 */
#define CONTROL_ADDRESS_HIGH 0xFF
/* CONTROL_ADDRESS_LOW：EEPROM/RTC 控制寄存器地址低字节。 */
#define CONTROL_ADDRESS_LOW 0xFF
#else //使用X1226
/* EEPROM_ADDRESS：EEPROM 软件 I²C 器件寻址字节，保持原器件定义。 */
#define EEPROM_ADDRESS 0xAE
/* CONTROL_ADDRESS：EEPROM/RTC 控制区软件 I²C 器件寻址字节。 */
#define CONTROL_ADDRESS 0xDE
/* CONTROL_ADDRESS_HIGH：EEPROM/RTC 控制寄存器地址高字节。 */
/* 高、低地址字节共同构成寄存器地址；CCR 的 0x003F 为状态寄存器。 */
#define CONTROL_ADDRESS_HIGH 0x00
/* CONTROL_ADDRESS_LOW：EEPROM/RTC 控制寄存器地址低字节。 */
#define CONTROL_ADDRESS_LOW 0x3F
/* TIME_ADDRESS：RTC 时间区软件 I²C 器件寻址字节。 */
#define TIME_ADDRESS 0xDE
/* TIME_ADDRESS_HIGH：RTC 时间寄存器地址高字节。 */
#define TIME_ADDRESS_HIGH 0x00
/* TIME_ADDRESS_LOW：RTC 时间寄存器地址低字节。 */
#define TIME_ADDRESS_LOW 0x30
#endif

/* VALIDFLAG：原 EEPROM/I²C 操作成功标志值。 */
#define VALIDFLAG 1
/* INVALIDFLAG：原 EEPROM/I²C 操作失败标志值。 */
#define INVALIDFLAG 0

//EEPROM ADDRESS
/* Page_ADC_Zero：旧 AD 零点校准 EEPROM 页号。 */
#define Page_ADC_Zero 0x00 //AD校准零漂与校准系数
/* Page_ADC_Ratio：原 Page_ADC 采样量的传感器/工程量换算比例。 */
#define Page_ADC_Ratio 0x01 //AD各通道变比与基准值
/* Page_CAL_Para：旧计算参数 EEPROM 页号。 */
#define Page_CAL_Para 0x02 //算法中各种参数
/* Page_CTL_Set：旧控制配置 EEPROM 页号。 */
#define Page_CTL_Set 0x03 //装置的各种设定
/* Page_LOG_A：旧记录 A 的 EEPROM 页号。 */
#define Page_LOG_A 0x04 //事件记录
/* Page_LOG_B：旧记录 B 的 EEPROM 页号。 */
#define Page_LOG_B 0x05
/* Page_LOG_C：旧记录 C 的 EEPROM 页号。 */
#define Page_LOG_C 0x06
/* Page_LOG_D：旧记录 D 的 EEPROM 页号。 */
#define Page_LOG_D 0x07

/* Addr_Udc_ref：旧 EEPROM 直流电压给定字节偏移。 */
#define Addr_Udc_ref 0x20
/* Addr_Q_ref：旧 EEPROM 无功功率给定字节偏移。 */
#define Addr_Q_ref 0x22
/* Addr_IQ_ref：旧 EEPROM q 轴电流给定字节偏移。 */
#define Addr_IQ_ref 0x24
/* Addr_Adj_Phi：旧 EEPROM 相位 Phi 调整参数字节偏移。 */
#define Addr_Adj_Phi 0x38
/* Addr_Adj_Theta：旧 EEPROM 相位 Theta 调整参数字节偏移。 */
#define Addr_Adj_Theta 0x3A
/* Addr_Modulation：旧 EEPROM 调制比参数字节偏移。 */
#define Addr_Modulation 0x3C
/* Addr_Debug_Rec：旧 EEPROM 调试记录参数字节偏移。 */
#define Addr_Debug_Rec 0x3E

    /****************************************************************************/
    /*                                                                          */
    /*              外部调用函数声明                                                                                                                                        */
    /*                                                                          */
    /****************************************************************************/

    /*==================================================================
 * 函数功能: 通过软件 I²C 写入 EEPROM 单字节，保留最多三次重试
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8Offset：EEPROM 页内字节偏移
 *           u8Data：待写入字节，或调用者提供的段写数据缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_byte_write(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 u8Data);
    /*==================================================================
 * 函数功能: 通过软件 I²C 写入指定页内的一段字节数据
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8Offset：EEPROM 页内字节偏移
 *           length：有效字节数；维护对象按其接口约定使用 16 位字数
 *           u8Data：待写入字节，或调用者提供的段写数据缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_section_write(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 length, Uint8 *u8Data);
    /*==================================================================
 * 函数功能: 通过软件 I²C 读取 EEPROM 单字节
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8Offset：EEPROM 页内字节偏移
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 读取的字节；失败沿用原函数的返回约定
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_byte_read(Uint8 u8PageNum, Uint8 pu8Offset);
    /*==================================================================
 * 函数功能: 通过软件 I²C 读取指定页内的一段数据
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8Offset：EEPROM 页内字节偏移
 *           length：有效字节数；维护对象按其接口约定使用 16 位字数
 *           u8Data：待写入字节，或调用者提供的段写数据缓存
 * 输出参数: u8Data 指向的 length 个接收字节。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_section_read(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 length, Uint8 *u8Data);
    /*==================================================================
 * 函数功能: 读取 EEPROM 指定页的 64 个字节
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8RecByte：输出页读取缓存，容量至少 64 字节
 * 输出参数: pu8RecByte 指向的 64 字节页缓存。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_page_read(Uint8 u8PageNum, Uint8 *pu8RecByte);
    /*==================================================================
 * 函数功能: 写入 EEPROM/RTC 的控制寄存器
 * 输入参数: u8Data：待写入字节，或调用者提供的段写数据缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_control_write(Uint8 u8Data);
    /*==================================================================
 * 函数功能: 写 EEPROM 控制寄存器以恢复写保护
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_protect(void);
    /*==================================================================
 * 函数功能: 写 EEPROM 控制寄存器以允许写入
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_eeprom_unprotect(void);
    /*==================================================================
 * 函数功能: 通过软件 I²C 读取 RTC 时间寄存器到调用者缓存
 * 输入参数: time：输出 RTC 时间寄存器字节缓存
 * 输出参数: time 指向的原 RTC 时间寄存器字段缓存。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_time_read(Uint8 *time);
    /*==================================================================
 * 函数功能: 通过软件 I²C 写入调用者提供的 RTC 时间字段
 * 输入参数: rec_time：输入 RTC 时间寄存器字节缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 r_time_write(Uint8 *rec_time);
    /*==================================================================
 * 函数功能: 把两位压缩 BCD 转为十进制数值
 * 输入参数: val：待转换的两位 BCD/十进制数值
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 转换后的二进制整数数值
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 BCD2HEX(Uint8 val);
    /*==================================================================
 * 函数功能: 把十进制数值转为两位压缩 BCD
 * 输入参数: val：待转换的两位 BCD/十进制数值
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 高半字节为十位、低半字节为个位
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
    extern Uint8 HEX2BCD(Uint8 val);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif // end of EEPROM_H definition

//===========================================================================
// End of file.
//===========================================================================
