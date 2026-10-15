#include "SstTypes.h"
// TI File $Revision: /main/1 $
// Checkin $Date: August 18, 2006   13:46:25 $
//###########################################################################
//
// FILE:	EEProm.c
//
// TITLE:	EEPROM & Time read/write
// AUTHOR: Li Kai
//
//###########################################################################
// $TI Release: DSP2833x Header Files V1.10 $
// $Release Date: February 15, 2008 $
//###########################################################################

#include "EEProm.h"
#include "gpio.h"
#include "soc_C6748.h"
/****************************************************************************/
/*                                                                          */
/*              宏定义                                                                                                                                                          */
/*                                                                          */
/****************************************************************************/
/* SDAO：把软件 I²C SDA 配置为输出。 */
#define SDAO GPIODirModeSet(SOC_GPIO_0_REGS, 9, GPIO_DIR_OUTPUT)
/* SDAI：把软件 I²C SDA 配置为输入。 */
#define SDAI GPIODirModeSet(SOC_GPIO_0_REGS, 9, GPIO_DIR_INPUT)
/* SDAH：软件 I²C SDA 输出高电平。 */
#define SDAH GPIOPinWrite(SOC_GPIO_0_REGS, 9, 1)
/* SDAL：软件 I²C SDA 输出低电平。 */
#define SDAL GPIOPinWrite(SOC_GPIO_0_REGS, 9, 0)
/* SCLH：软件 I²C SCL 输出高电平。 */
#define SCLH GPIOPinWrite(SOC_GPIO_0_REGS, 10, 1)
/* SCLL：软件 I²C SCL 输出低电平。 */
#define SCLL GPIOPinWrite(SOC_GPIO_0_REGS, 10, 0)
/* ACK：读取软件 I²C SDA 的应答电平。 */
#define ACK GPIOPinRead(SOC_GPIO_0_REGS, 9) & 0x01
/* SDA：读取软件 I²C SDA 当前电平。 */
#define SDA GPIOPinRead(SOC_GPIO_0_REGS, 9) & 0x01

/****************************************************************************/
/*                                                                          */
/*              函数声明                                                                                                                                                      */
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
Uint8 r_eeprom_byte_write(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 u8Data);
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
Uint8 r_eeprom_section_write(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 length, Uint8 *u8Data);
/*==================================================================
 * 函数功能: 通过软件 I²C 读取 EEPROM 单字节
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8Offset：EEPROM 页内字节偏移
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 读取的字节；失败沿用原函数的返回约定
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_byte_read(Uint8 u8PageNum, Uint8 pu8Offset);
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
Uint8 r_eeprom_section_read(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 length, Uint8 *u8Data);
/*==================================================================
 * 函数功能: 读取 EEPROM 指定页的 64 个字节
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8RecByte：输出页读取缓存，容量至少 64 字节
 * 输出参数: pu8RecByte 指向的 64 字节页缓存。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_page_read(Uint8 u8PageNum, Uint8 *pu8RecByte);
/*==================================================================
 * 函数功能: 写入 EEPROM/RTC 的控制寄存器
 * 输入参数: u8Data：待写入字节，或调用者提供的段写数据缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_control_write(Uint8 u8Data);
/*==================================================================
 * 函数功能: 写 EEPROM 控制寄存器以恢复写保护
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_protect(void);
/*==================================================================
 * 函数功能: 写 EEPROM 控制寄存器以允许写入
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_unprotect(void);

/*==================================================================
 * 函数功能: 按 SCL 时序采集 SDA 的八位数据
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 接收的一个字节
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_iic_rec_byte(void);
/*==================================================================
 * 函数功能: 向从器件发送低电平 ACK 应答
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_send_ack(void);
/*==================================================================
 * 函数功能: 按高位优先在 SDA/SCL 上发送一个字节
 * 输入参数: u8Byte：软件 I²C 待发送字节，高位先发
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_send_byte(Uint8 u8Byte);
/*==================================================================
 * 函数功能: 向从器件发送高电平 NACK 应答
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_send_nack(void);
/*==================================================================
 * 函数功能: 按 GPIO 时序产生软件 I²C 起始条件
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_start(void);
/*==================================================================
 * 函数功能: 按 GPIO 时序产生软件 I²C 停止条件
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_stop(void);
/*==================================================================
 * 函数功能: 读取 SDA 以检查从器件的应答
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示应答有效，INVALIDFLAG 表示无应答
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_iic_wait_ack();
/*==================================================================
 * 函数功能: 通过软件 I²C 读取 RTC 时间寄存器到调用者缓存
 * 输入参数: time：输出 RTC 时间寄存器字节缓存
 * 输出参数: time 指向的原 RTC 时间寄存器字段缓存。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_time_read(Uint8 *time);
/*==================================================================
 * 函数功能: 通过软件 I²C 写入调用者提供的 RTC 时间字段
 * 输入参数: rec_time：输入 RTC 时间寄存器字节缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_time_write(Uint8 *rec_time);
/*==================================================================
 * 函数功能: 执行原 EEPROM 驱动的循环延时，实际时长依赖时钟及编译优化
 * 输入参数: n：本次有效数据数量或循环次数，按调用接口的长度单位解释
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void DELAY_US(unsigned int n);
/*==================================================================
 * 函数功能: 把两位压缩 BCD 转为十进制数值
 * 输入参数: val：待转换的两位 BCD/十进制数值
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 转换后的二进制整数数值
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 BCD2HEX(Uint8 val);
/*==================================================================
 * 函数功能: 把十进制数值转为两位压缩 BCD
 * 输入参数: val：待转换的两位 BCD/十进制数值
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 高半字节为十位、低半字节为个位
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 HEX2BCD(Uint8 val);

/*************************************************************************
Function Name   :r_eeprom_protect()
Description     :eeprom write protect
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          : VALIDFLAG  -- success
                  INVALIDFLAG -- failed
Revision        :
Others          :
**************************************************************************/
/*==================================================================
 * 函数功能: 写 EEPROM 控制寄存器以恢复写保护
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_protect(void) //保护，写使能
{
    Uint8 u8Result; /* u8Result：软件 I²C 控制写入的累计成功/失败标志。 */

    u8Result = VALIDFLAG;
#ifdef X4643
    u8Result &= r_eeprom_control_write(0x02);
#endif
    u8Result &= r_eeprom_control_write(0x00);

    return u8Result;
}

/*************************************************************************
Function Name   :r_eeprom_unprotect()
Description     :unprotect the eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          :
Revision        :
Others          :
**************************************************************************/
/*==================================================================
 * 函数功能: 写 EEPROM 控制寄存器以允许写入
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_unprotect(void)
{
    Uint8 u8Result; /* u8Result：软件 I²C 控制写入的累计成功/失败标志。 */

    u8Result = VALIDFLAG;
    u8Result &= r_eeprom_control_write(0x02);
    u8Result &= r_eeprom_control_write(0x06);
#ifdef X4643
    u8Result &= r_eeprom_control_write(0x62);
#endif

    return u8Result;
}

/*************************************************************************
Function Name   :r_eeprom_byte_write()
Description     :write one byte to eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :u8PageNum     -- page number in eeprom
                 *pu8Offset     -- offset address in the page
                 u8Data        -- data writed to eeprom
Output          :
Return          :VALIDFLAG  --success
                 INVALIDFLAG --failed
Revision        :
Others          :
**************************************************************************/
/*==================================================================
 * 函数功能: 通过软件 I²C 写入 EEPROM 单字节，保留最多三次重试
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8Offset：EEPROM 页内字节偏移
 *           u8Data：待写入字节，或调用者提供的段写数据缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_byte_write(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 u8Data)
{
    Uint16 u16ByteAdd; /* u16ByteAdd：EEPROM 绝对字节地址，由页号乘 64 加页内偏移得到。 */
    Uint8 u8HighAdd; /* u8HighAdd：EEPROM 绝对字节地址的高 8 位。 */
    Uint8 u8LowAdd; /* u8LowAdd：EEPROM 绝对字节地址的低 8 位。 */
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */

    u16ByteAdd = (Uint16)u8PageNum;
    u16ByteAdd = (u16ByteAdd << 6) + pu8Offset; /* the byte address */
    u8LowAdd = (Uint8)u16ByteAdd;
    u8HighAdd = (Uint8)(u16ByteAdd >> 8);

    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        r_iic_start(); /* start the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS); /* send the eeprom address to eeprom */

        if (r_iic_wait_ack() == INVALIDFLAG) /* check ACK received from eeprom */
        {
            continue;
        }

        r_iic_send_byte(u8HighAdd); /* send the high address to eeprom*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8LowAdd); /* send the low address to eeprom*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8Data); /* write the data to eeprom */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_stop(); /* stop the iic operation */
        DELAY_US(1); /* wait for the eeprom write cycle */ //原来是12000，现在改为了12；

        return VALIDFLAG;
    }

    return INVALIDFLAG;
}

/*************************************************************************
Function Name   :r_eeprom_section_write()
Description     :write one section to eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :u8PageNum     -- page number in eeprom
                 pu8Offset     -- offset address in the page
                 length        -- length of the section
                 *u8Data        --data writed to eeprom
Output          :
Return          :VALIDFLAG  --success
                 INVALIDFLAG --failed
Revision        :
Others          :
**************************************************************************/
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
Uint8 r_eeprom_section_write(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 length, Uint8 *u8Data)
{
    Uint16 u16ByteAdd; /* u16ByteAdd：EEPROM 绝对字节地址，由页号乘 64 加页内偏移得到。 */
    Uint8 u8HighAdd; /* u8HighAdd：EEPROM 绝对字节地址的高 8 位。 */
    Uint8 u8LowAdd; /* u8LowAdd：EEPROM 绝对字节地址的低 8 位。 */
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */
    Uint8 Temp_error; /* Temp_error：EEPROM 段写入的应答失败累计状态。 */
    Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */

    u16ByteAdd = (Uint16)u8PageNum;
    u16ByteAdd = (u16ByteAdd << 6) + pu8Offset; /* the byte address */
    u8LowAdd = (Uint8)u16ByteAdd;
    u8HighAdd = (Uint8)(u16ByteAdd >> 8);

    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        Temp_error = 0;
        r_iic_start(); /* start the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS); /* send the eeprom address to eeprom */

        if (r_iic_wait_ack() == INVALIDFLAG) /* check ACK received from eeprom */
        {
            continue;
        }

        r_iic_send_byte(u8HighAdd); /* send the high address to eeprom*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8LowAdd); /* send the low address to eeprom*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }
        for (i = 0; i < length; i++)
        {
            r_iic_send_byte(*(u8Data + i)); /* write the data to eeprom */

            if (r_iic_wait_ack() == INVALIDFLAG)
            {
                Temp_error = 1;
                break;
            }
        }
        if (Temp_error == 1)
        {
            continue;
        }
        r_iic_stop(); /* stop the iic operation */
        DELAY_US(12000); /* wait for the eeprom write cycle */ //原来是12000，现在改为了12；

        return VALIDFLAG;
    }

    return INVALIDFLAG;
}

/*************************************************************************
Function Name   :r_eeprom_byte_read()
Description     :read one byte from eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :u8PageNum     -- page number in eeprom
                 *pu8Offset     -- offset in the page
Output          :
Return          :u8RecByte     -- byte read back from eeprom
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 通过软件 I²C 读取 EEPROM 单字节
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8Offset：EEPROM 页内字节偏移
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 读取的字节；失败沿用原函数的返回约定
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_byte_read(Uint8 u8PageNum, Uint8 pu8Offset)
{
    Uint8 u8RecByte; /* u8RecByte：EEPROM 读取的一个原始字节。 */
    Uint16 u16ByteAdd; /* u16ByteAdd：EEPROM 绝对字节地址，由页号乘 64 加页内偏移得到。 */
    Uint8 u8HighAdd; /* u8HighAdd：EEPROM 绝对字节地址的高 8 位。 */
    Uint8 u8LowAdd; /* u8LowAdd：EEPROM 绝对字节地址的低 8 位。 */
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */

    u16ByteAdd = (Uint16)u8PageNum;
    u16ByteAdd = (u16ByteAdd << 6) + pu8Offset;
    u8LowAdd = (Uint8)u16ByteAdd;
    u8HighAdd = (Uint8)(u16ByteAdd >> 8);

    u8RecByte = 0;

    /*read the data form eeprom*/

    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        r_iic_start(); /*start the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS); /* send the eeprom address byte */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8HighAdd); /* send high order word address */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8LowAdd); /* send high order word address */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_start(); /* restart the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS + 1); /* send the eeprom address byte
                                                and set the read mode*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        u8RecByte = r_iic_rec_byte(); /* receive one byte */
        r_iic_send_nack(); /* send nack to eeprom */
        r_iic_stop(); /* stop the iic operation */
        break;
    }

    return (u8RecByte);
}

/*************************************************************************
Function Name   :r_eeprom_section_read()
Description     :read one section from eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :u8PageNum     -- page number in eeprom
                 pu8Offset     -- offset address in the page
                 length        -- length of the section
                 *u8Data        --data read from eeprom
Output          :
Return          :VALIDFLAG  --success
                 INVALIDFLAG --failed
Revision        :
Others          :
**************************************************************************/

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
Uint8 r_eeprom_section_read(Uint8 u8PageNum, Uint8 pu8Offset, Uint8 length, Uint8 *u8Data)
{
    Uint16 u16ByteAdd; /* u16ByteAdd：EEPROM 绝对字节地址，由页号乘 64 加页内偏移得到。 */
    Uint8 u8HighAdd; /* u8HighAdd：EEPROM 绝对字节地址的高 8 位。 */
    Uint8 u8LowAdd; /* u8LowAdd：EEPROM 绝对字节地址的低 8 位。 */
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */
    Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */

    u16ByteAdd = (Uint16)u8PageNum;
    u16ByteAdd = (u16ByteAdd << 6) + pu8Offset;
    u8LowAdd = (Uint8)u16ByteAdd;
    u8HighAdd = (Uint8)(u16ByteAdd >> 8);

    /*read the data form eeprom*/

    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        r_iic_start(); /*start the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS); /* send the eeprom address byte */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8HighAdd); /* send high order word address */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8LowAdd); /* send high order word address */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_start(); /* restart the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS + 1); /* send the eeprom address byte and set the read mode*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }
        for (i = 0; i < length; i++)
        {
            *(u8Data + i) = r_iic_rec_byte(); /* receive one byte */
            if (i < (length - 1))
                r_iic_send_ack(); /* send ack to eeprom */
            else
                r_iic_send_nack(); /* send no-ack to eeprom */
        }
        r_iic_stop(); /* stop the iic operation */
        return VALIDFLAG;
    }

    return INVALIDFLAG;
}

/*************************************************************************
Function Name   :r_eeprom_page_read()
Description     :read 64 byte from eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :u8PageNum     -- page number in eeprom
Output          :*pu8RecByte   -- byte read back from eeprom
Return          :
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 读取 EEPROM 指定页的 64 个字节
 * 输入参数: u8PageNum：EEPROM 页号，每页 64 字节
 *           pu8RecByte：输出页读取缓存，容量至少 64 字节
 * 输出参数: pu8RecByte 指向的 64 字节页缓存。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_page_read(Uint8 u8PageNum, Uint8 *pu8RecByte)
{
    Uint16 u16ByteAdd; /* u16ByteAdd：EEPROM 绝对字节地址，由页号乘 64 加页内偏移得到。 */
    Uint8 u8HighAdd; /* u8HighAdd：EEPROM 绝对字节地址的高 8 位。 */
    Uint8 u8LowAdd; /* u8LowAdd：EEPROM 绝对字节地址的低 8 位。 */
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */
    Uint16 i; /* i：本次数组、参数或模块遍历的索引。 */

    u16ByteAdd = (Uint16)u8PageNum;
    u16ByteAdd = (u16ByteAdd << 6);
    u8LowAdd = (Uint8)u16ByteAdd;
    u8HighAdd = (Uint8)(u16ByteAdd >> 8);

    /*read the data form eeprom*/

    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        r_iic_start(); /*start the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS); /* send the eeprom address byte */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8HighAdd); /* send high order word address */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8LowAdd); /* send high order word address */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_start(); /* restart the iic operation */
        r_iic_send_byte(EEPROM_ADDRESS + 1); /* send the eeprom address byte
                                                and set the read mode*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }
        for (i = 0; i < 64; i++)
        {
            *(pu8RecByte + i) = r_iic_rec_byte(); /* receive one byte */
            if (i < 63)
                r_iic_send_ack(); /* send ack to eeprom */
            else
                r_iic_send_nack(); /* send no-ack to eeprom */
        }
        r_iic_stop(); /* stop the iic operation */
        return VALIDFLAG;
    }
    return INVALIDFLAG;
}

/*************************************************************************
Function Name   :r_iic_start()
Description     :start the iic operation
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          :
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 按 GPIO 时序产生软件 I²C 起始条件
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_start(void)
{
    SDAO; /* output port */
    SCLL; /* SCL low level */
    SDAH; /* SDA high level */
    DELAY_US(4);
    SCLH; /* SCL high level */
    DELAY_US(4);
    SDAL; /* SDA low level */
    DELAY_US(4);
    SCLL; /* SCL low level */
}

/*************************************************************************
Function Name   :r_iic_stop()
Description     :stop the iic operation
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          :
Revision        :
Others          :
**************************************************************************/
/*==================================================================
 * 函数功能: 按 GPIO 时序产生软件 I²C 停止条件
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_stop(void)
{
    SDAO; /* output port */
    SCLL; /* SCL low level */
    SDAL; /* SDA low level */
    DELAY_US(4);
    SCLH; /* SCL high level */
    DELAY_US(4);
    SDAH; /* SDA high level */
    DELAY_US(4);
    SCLL; /* SCL low level */
}

/*************************************************************************
Function Name   :r_iic_send_byte()
Description     :send 8 bits to eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :u8Byte   --8 bit data to be sended to eeprom
Output          :
Return          :
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 按高位优先在 SDA/SCL 上发送一个字节
 * 输入参数: u8Byte：软件 I²C 待发送字节，高位先发
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_send_byte(Uint8 u8Byte)
{
    Uint8 i; /* i：本次字节/位循环序号。 */

    SDAO; /* SDA is out port */

    /* send one byte to eeprom */

    for (i = 0; i < 8; i++)
    {
        SCLL; /* SCL low level */

        if (u8Byte & 0x80) /* check the msb is 1 or 0 */
        {
            SDAH; /* SDA high level */
        }
        else
        {
            SDAL; /* SDA low level */
        }

        DELAY_US(4);
        SCLH; /* SCL high level */
        DELAY_US(4);
        u8Byte = u8Byte << 1;
    }
    SCLL; /* SCL must be set low level before
                                           the SDA changing direction*/
}

/*************************************************************************
Function Name   :r_iic_wait_ack()
Description     :wait for the ack from eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          :VALIDFLAG  --success
                 INVALIDFLAG --failed
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 读取 SDA 以检查从器件的应答
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示应答有效，INVALIDFLAG 表示无应答
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_iic_wait_ack(void)
{
    SDAI; /* SDA is in port */
    SCLL; /* SCL low level */
    DELAY_US(4);
    SCLH; /* SCL high level */

    if (ACK == 1) /* check ACK is high level */
    {
        r_iic_stop(); /* error */
        return INVALIDFLAG;
    }

    DELAY_US(4);
    SCLL;

    return VALIDFLAG;
}

/*************************************************************************
Function Name   :r_iic_rec_byte()
Description     :receive 8 bits from eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          :u8RcvByte  -- received from eeprom
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 按 SCL 时序采集 SDA 的八位数据
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 接收的一个字节
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_iic_rec_byte(void)
{
    Uint8 i; /* i：本次字节/位循环序号。 */
    Uint8 u8RcvByte; /* u8RcvByte：软件 I²C 逐位组装的接收字节。 */

    u8RcvByte = 0;
    SDAI;

    /* receive one byte from eeprom, msb is in head */ // msb是 最高位

    for (i = 0; i < 8; i++)
    {
        u8RcvByte = u8RcvByte << 1;
        SCLL; /* SCL low level */
        DELAY_US(4);
        SCLH;

        if (SDA == 1) /* check SDA is high level */
        {
            u8RcvByte += 1;
        }

        DELAY_US(4);
    }

    SCLL;

    return (u8RcvByte);
}

/*************************************************************************
Function Name   :r_iic_send_ack()
Description     :send ack to eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          :
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 向从器件发送低电平 ACK 应答
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_send_ack(void)
{
    SDAO;
    SCLL;
    SDAL;
    DELAY_US(4);
    SCLH;
    DELAY_US(4);
    SCLL;
}

/*************************************************************************
Function Name   :r_iic_send_nack()    不需要等待应答；？？
Description     :send nack to eeprom
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :
Output          :
Return          :
Revision        :
Others          :
**************************************************************************/

/*==================================================================
 * 函数功能: 向从器件发送高电平 NACK 应答
 * 输入参数: 无
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void r_iic_send_nack(void)
{
    SDAO;
    SCLL;
    SDAH;
    DELAY_US(4);
    SCLH;
    DELAY_US(4);
    SCLL;
}

/*************************************************************************
Function Name   :r_eeprom_control_write()
Description     :write control register
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :u8Data     --the byte to be writed to control register
Output          :
Return          :VALIDFLAG   --success
                 INVALIDFLAG  --failed
Revision        :
Others          :
**************************************************************************/
/*==================================================================
 * 函数功能: 写入 EEPROM/RTC 的控制寄存器
 * 输入参数: u8Data：待写入字节，或调用者提供的段写数据缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_eeprom_control_write(Uint8 u8Data) //写一个字节；
{
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */

    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        r_iic_start(); /* start the iic operation */
        r_iic_send_byte(CONTROL_ADDRESS); /* send the CCR address to eeprom */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(CONTROL_ADDRESS_HIGH); /* send the high address to eeprom*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(CONTROL_ADDRESS_LOW); /* send the low address to eeprom*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(u8Data); /* write the byte to eeprom */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_stop(); /* stop the iic operation */
        DELAY_US(12000);

        return VALIDFLAG;
    }

    return INVALIDFLAG;
}

/*************************************************************************
Function Name   :r_time_read()
Description     :read real time
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :void
Output          :time reg
Return          :VALIDFLAG   --success
                 INVALIDFLAG  --failed
Revision        :
Others          :
**************************************************************************/
/*==================================================================
 * 函数功能: 通过软件 I²C 读取 RTC 时间寄存器到调用者缓存
 * 输入参数: time：输出 RTC 时间寄存器字节缓存
 * 输出参数: time 指向的原 RTC 时间寄存器字段缓存。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_time_read(Uint8 *time)
{
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */

    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        r_iic_start(); /* start the iic operation */
        r_iic_send_byte(TIME_ADDRESS); /* send the RTC address to 1226 */

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(TIME_ADDRESS_HIGH); /* send the high address to RTC*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_send_byte(TIME_ADDRESS_LOW); /* send the low address to RTC*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        r_iic_start(); /* restart the iic operation */
        r_iic_send_byte(TIME_ADDRESS + 1); /* send the RTC address byte
                                                        and set the read mode*/

        if (r_iic_wait_ack() == INVALIDFLAG)
        {
            continue;
        }

        *time = r_iic_rec_byte();
        r_iic_send_ack();
        *(time + 1) = r_iic_rec_byte();
        r_iic_send_ack();
        *(time + 2) = 0x3F & (r_iic_rec_byte());
        r_iic_send_ack();
        *(time + 3) = r_iic_rec_byte();
        r_iic_send_ack();
        *(time + 4) = r_iic_rec_byte();
        r_iic_send_ack();
        *(time + 5) = r_iic_rec_byte();
        r_iic_send_ack();
        r_iic_stop(); /* stop the iic operation */

        return VALIDFLAG;
    }

    return INVALIDFLAG;
}

/*************************************************************************
Function Name   :r_time_write()
Description     :write real time
Algorithm       :
Calls           :
Called By       :
Table Accessed  :
Table Updated   :
Input           :received 6 byte from UART
Output          :
Return          :VALIDFLAG   --success
                 INVALIDFLAG  --failed
Revision        :
Others          :
**************************************************************************/
/*==================================================================
 * 函数功能: 通过软件 I²C 写入调用者提供的 RTC 时间字段
 * 输入参数: rec_time：输入 RTC 时间寄存器字节缓存
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: VALIDFLAG 表示成功，INVALIDFLAG 表示失败
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 r_time_write(Uint8 *rec_time)
{
    Uint8 u8Retry; /* u8Retry：软件 I²C 访问重试序号，最多三次。 */
    Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */
    for (u8Retry = 0; u8Retry < 3; u8Retry++)
    {
        for (i = 0; i < 6; i++)
        {
            r_iic_start(); /* start the iic operation */
            r_iic_send_byte(TIME_ADDRESS); /* send the CCR address to 1226 */

            if (r_iic_wait_ack() == INVALIDFLAG)
            {
                continue;
            }

            r_iic_send_byte(TIME_ADDRESS_HIGH); /* send the high address to RTC*/

            if (r_iic_wait_ack() == INVALIDFLAG)
            {
                continue;
            }

            r_iic_send_byte(TIME_ADDRESS_LOW + i); /* send the low address to RTC*/

            if (r_iic_wait_ack() == INVALIDFLAG)
            {
                continue;
            }

            if (i == 2) // Set MIL = 1, 24 Hour//这里可进行调整；
                r_iic_send_byte(0x80 | (*(rec_time + i)));
            else
                r_iic_send_byte(*(rec_time + i)); /* write the byte to RTC */

            if (r_iic_wait_ack() == INVALIDFLAG)
            {
                continue;
            }
            r_iic_stop(); /* stop the iic operation */
        }
        return VALIDFLAG;
    }

    return INVALIDFLAG;
}

/****************************************************************************/
/*                                                                          */
/*              延时1us                                                      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 执行原 EEPROM 驱动的循环延时，实际时长依赖时钟及编译优化
 * 输入参数: n：本次有效数据数量或循环次数，按调用接口的长度单位解释
 * 输出参数: 读操作写入接收缓存；其余操作改变 SDA/SCL 或 EEPROM/RTC 内容。
 * 返回值: 无
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
void DELAY_US(unsigned int n)
{
    long long i; /* i：本次数组、参数或模块遍历的索引。 */

    for (i = n * 90; i > 0; i--) //C6748的主频是456MHz,此处假设执行1次i--需要5个机器指令，此处设置1us内可执行90次i--;
    {
    }
}

/****************************************************************************/
/*                                                                          */
/*              BCD和HEX转换                                                     */
/*                                                                          */
/****************************************************************************/

/*==================================================================
 * 函数功能: 把两位压缩 BCD 转为十进制数值
 * 输入参数: val：待转换的两位 BCD/十进制数值
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 转换后的二进制整数数值
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 BCD2HEX(Uint8 val)
{
    Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */
    i = val & 0x0F;
    val >>= 4;
    val &= 0x0f;
    val *= 10;
    i += val;
    return i;
}

/*==================================================================
 * 函数功能: 把十进制数值转为两位压缩 BCD
 * 输入参数: val：待转换的两位 BCD/十进制数值
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 高半字节为十位、低半字节为个位
 * 执行说明: 板级自定义软件 I²C 驱动；SDA/SCL 位于 GPIO0，运行时存储任务在主循环调用。
 *================================================================*/
Uint8 HEX2BCD(Uint8 val)
{
    Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */
    i = ((val / 10) << 4) & 0xF0;
    i = i | (val % 10);
    return i;
}

//===========================================================================
// End of file.
//===========================================================================
