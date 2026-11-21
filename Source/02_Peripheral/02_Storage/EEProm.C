//###########################################################################
// FILE: EEProm.c
// FUNCTION: BL24C512A软件I2C；SDA/SCL只拉低或释放，不输出推挽高电平。
//           所有调用均在初始化或主循环，不关全局中断、不进入10kHz FPGA中断。
//###########################################################################
#include "EEProm.H"
#include "SstBoardConfig.h"
#include "gpio.h"
#include "soc_C6748.h"
#if defined(__TI_COMPILER_VERSION__)
#include <c6x.h>
#endif

/* 最近一次I2C访问的错误、地址及阶段，供CCS排查；不在ISR中更新。 */
volatile SST_EEPROM_STATUS g_sst_eeprom_status = {0U, 0U, 0U, SST_EEPROM_DEVICE_WRITE};

/*==================================================================
 * 函数功能: 等待I2C建立/保持时间，允许10kHz中断正常抢占
 * 输入参数: microseconds：最短等待微秒数
 * 返回值: 无。TI目标使用456MHz CPU周期计数器，主机测试使用有限循环。
 *================================================================*/
static void Eeprom_DelayUs(unsigned short microseconds)
{
#if defined(__TI_COMPILER_VERSION__)
    unsigned int start = TSCL; /* 本次等待开始时的周期计数，允许32位自然回绕。 */
    unsigned int cycles = (SYSCLK_1_FREQ / 1000000U) * microseconds; /* 最短等待对应的CPU周期数。 */
    while ((unsigned int)(TSCL - start) < cycles)
    {
    }
#else
    volatile unsigned int cycles; /* 主机验证不模拟真实墙钟，只保留有限等待。 */
    for (cycles = 0U; cycles < microseconds; ++cycles)
    {
    }
#endif
}

/*==================================================================
 * 函数功能: 释放GPIO为输入，由外部上拉产生总线高电平
 * 输入参数: pin：SDA或SCL的GPIO库编号
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static void Eeprom_ReleasePin(unsigned int pin)
{
    GPIODirModeSet(SOC_GPIO_0_REGS, pin, GPIO_DIR_INPUT);
}

/*==================================================================
 * 函数功能: 先清输出锁存器，再将GPIO切为输出以拉低总线
 * 输入参数: pin：SDA或SCL的GPIO库编号
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static void Eeprom_PullPinLow(unsigned int pin)
{
    GPIOPinWrite(SOC_GPIO_0_REGS, pin, 0U);
    GPIODirModeSet(SOC_GPIO_0_REGS, pin, GPIO_DIR_OUTPUT);
}

/*==================================================================
 * 函数功能: 释放SCL并等待最短高电平时间；DSP独占总线
 * 输入参数: 无
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static void Eeprom_ClockHigh(void)
{
    Eeprom_ReleasePin(SST_EEPROM_SCL_PIN);
    Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
}

/*==================================================================
 * 函数功能: 产生I2C STOP并释放两根线；正常及错误退出共用
 * 输入参数: 无
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static void Eeprom_Stop(void)
{
    Eeprom_PullPinLow(SST_EEPROM_SCL_PIN);
    Eeprom_PullPinLow(SST_EEPROM_SDA_PIN);
    Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
    Eeprom_ClockHigh();
    Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
    Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
}

/*==================================================================
 * 函数功能: 产生START或重复START，先检查两根总线是否已释放
 * 输入参数: 无
 * 返回值: VALIDFLAG成功；INVALIDFLAG总线被拉低，错误记录到全局状态
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static Uint8 Eeprom_Start(void)
{
    Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
    /* 重复START前可能刚结束ACK的SCL下降沿，仍须满足最短低电平时间。 */
    Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
    Eeprom_ClockHigh();
    if ((GPIOPinRead(SOC_GPIO_0_REGS, SST_EEPROM_SDA_PIN) == 0U) ||
        (GPIOPinRead(SOC_GPIO_0_REGS, SST_EEPROM_SCL_PIN) == 0U))
    {
        g_sst_eeprom_status.error = EEPROM_ERROR_BUS_LOW;
        return INVALIDFLAG;
    }
    Eeprom_PullPinLow(SST_EEPROM_SDA_PIN);
    Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
    Eeprom_PullPinLow(SST_EEPROM_SCL_PIN);
    return VALIDFLAG;
}

/*==================================================================
 * 函数功能: 按最高位先发送8位，并读取第9拍从机ACK
 * 输入参数: value：本次发送的数据或寻址字节
 * 返回值: VALIDFLAG收到ACK；INVALIDFLAG无应答，上层随后产生STOP
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static Uint8 Eeprom_SendByte(Uint8 value)
{
    unsigned short bit; /* 本字节位序，按MSB先发送。 */
    Uint8 acknowledged; /* 第9拍SDA=0才是从机ACK。 */
    for (bit = 0U; bit < 8U; ++bit)
    {
        if ((value & 0x80U) != 0U)
            Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
        else
            Eeprom_PullPinLow(SST_EEPROM_SDA_PIN);
        Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
        Eeprom_ClockHigh();
        Eeprom_PullPinLow(SST_EEPROM_SCL_PIN);
        value = (Uint8)(value << 1U);
    }
    Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
    Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
    Eeprom_ClockHigh();
    acknowledged = (GPIOPinRead(SOC_GPIO_0_REGS, SST_EEPROM_SDA_PIN) == 0U) ? VALIDFLAG : INVALIDFLAG;
    Eeprom_PullPinLow(SST_EEPROM_SCL_PIN);
    if (acknowledged == INVALIDFLAG)
        g_sst_eeprom_status.error = EEPROM_ERROR_NO_ACK;
    return acknowledged;
}

/*==================================================================
 * 函数功能: 接收8位并在第9拍发送ACK或末字节NACK
 * 输入参数: more：非0继续读并发送ACK；0发送NACK结束读取
 * 返回值: 接收的8位数据
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static Uint8 Eeprom_ReceiveByte(Uint8 more)
{
    unsigned short bit; /* 接收位序。 */
    Uint8 value = 0U; /* 按MSB到LSB拼成的返回字节。 */
    Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
    for (bit = 0U; bit < 8U; ++bit)
    {
        Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
        Eeprom_ClockHigh();
        value = (Uint8)((value << 1U) | (GPIOPinRead(SOC_GPIO_0_REGS, SST_EEPROM_SDA_PIN) & 1U));
        Eeprom_PullPinLow(SST_EEPROM_SCL_PIN);
    }
    if (more != 0U)
        Eeprom_PullPinLow(SST_EEPROM_SDA_PIN);
    else
        Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
    Eeprom_DelayUs(SST_EEPROM_HALF_CLOCK_US);
    Eeprom_ClockHigh();
    Eeprom_PullPinLow(SST_EEPROM_SCL_PIN);
    Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
    return value;
}

/*==================================================================
 * 函数功能: 发送写器件地址及16位存储地址，每一步检查ACK
 * 输入参数: address：EEPROM绝对字节地址，范围0000h..FFFFh
 * 返回值: VALIDFLAG全部被应答；INVALIDFLAG失败，上层负责STOP
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static Uint8 Eeprom_SendAddress(unsigned short address)
{
    g_sst_eeprom_status.address = address;
    g_sst_eeprom_status.error = EEPROM_ERROR_NONE;
    g_sst_eeprom_status.device_byte = SST_EEPROM_DEVICE_WRITE;
    g_sst_eeprom_status.stage = 1U;
    if (Eeprom_Start() == INVALIDFLAG)
        return INVALIDFLAG;
    if (Eeprom_SendByte((Uint8)SST_EEPROM_DEVICE_WRITE) == INVALIDFLAG)
        return INVALIDFLAG;
    g_sst_eeprom_status.stage = 2U;
    if (Eeprom_SendByte((Uint8)(address >> 8U)) == INVALIDFLAG)
        return INVALIDFLAG;
    g_sst_eeprom_status.stage = 3U;
    return Eeprom_SendByte((Uint8)address);
}

/*==================================================================
 * 函数功能: 有限ACK轮询等待内部写周期结束，最多64次
 * 输入参数: 无
 * 返回值: VALIDFLAG器件就绪；INVALIDFLAG总线错误或写忙超时
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
static Uint8 Eeprom_WaitReady(void)
{
    unsigned short poll; /* 写忙轮询次数，达到上限后退出而不死等。 */
    g_sst_eeprom_status.device_byte = SST_EEPROM_DEVICE_WRITE;
    for (poll = 0U; poll < SST_EEPROM_READY_POLLS; ++poll)
    {
        if (Eeprom_Start() == INVALIDFLAG)
        {
            Eeprom_Stop();
            return INVALIDFLAG;
        }
        g_sst_eeprom_status.stage = 1U;
        if (Eeprom_SendByte((Uint8)SST_EEPROM_DEVICE_WRITE) == VALIDFLAG)
        {
            Eeprom_Stop();
            g_sst_eeprom_status.error = EEPROM_ERROR_NONE;
            return VALIDFLAG;
        }
        Eeprom_Stop();
    }
    g_sst_eeprom_status.error = EEPROM_ERROR_WRITE_TIMEOUT;
    return INVALIDFLAG;
}

/*==================================================================
 * 函数功能: 释放软件I2C总线，并在上电一次启动CPU周期计数延时时基
 * 输入参数: 无
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
void Eeprom_Initialize(void)
{
#if defined(__TI_COMPILER_VERSION__)
    TSCL = 0U; /* 上电一次启动CPU周期计数器；后续读写不复位计数器。 */
#endif
    Eeprom_ReleasePin(SST_EEPROM_SDA_PIN);
    Eeprom_ReleasePin(SST_EEPROM_SCL_PIN);
    g_sst_eeprom_status.error = EEPROM_ERROR_NONE;
    g_sst_eeprom_status.stage = 0U;
}

/*==================================================================
 * 函数功能: 按16位字节地址连续读取；重复START切读方向，最后字节NACK
 * 输入参数: address：EEPROM绝对起始字节地址
 *           data：接收缓存，至少length字节
 *           length：读取字节数，非0且不超过器件末尾
 * 返回值: VALIDFLAG读取完成，此时输出缓存有效；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 Eeprom_Read(unsigned short address, Uint8 *data, unsigned short length)
{
    unsigned short index; /* 当前连续读取字节序号。 */
    if ((data == 0) || (length == 0U) || ((unsigned long)address + length > SST_EEPROM_CAPACITY_BYTES))
    {
        g_sst_eeprom_status.error = EEPROM_ERROR_ADDRESS;
        return INVALIDFLAG;
    }
    if (Eeprom_SendAddress(address) == INVALIDFLAG)
    {
        Eeprom_Stop();
        return INVALIDFLAG;
    }
    g_sst_eeprom_status.stage = 5U;
    g_sst_eeprom_status.device_byte = SST_EEPROM_DEVICE_WRITE | 1U;
    if ((Eeprom_Start() == INVALIDFLAG) ||
        (Eeprom_SendByte((Uint8)(SST_EEPROM_DEVICE_WRITE | 1U)) == INVALIDFLAG))
    {
        Eeprom_Stop();
        return INVALIDFLAG;
    }
    for (index = 0U; index < length; ++index)
        data[index] = Eeprom_ReceiveByte((index + 1U < length) ? 1U : 0U);
    Eeprom_Stop();
    g_sst_eeprom_status.stage = 0U;
    return VALIDFLAG;
}

/*==================================================================
 * 函数功能: 发起一字节写入，不等待最长3ms内部写周期
 * 输入参数: address：EEPROM绝对字节地址
 *           value：待写的单字节值
 * 返回值: VALIDFLAG器件已接收；INVALIDFLAG无应答或总线错误；写完成由调用者等待/校验
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 Eeprom_StartByteWrite(unsigned short address, Uint8 value)
{
    Uint8 result; /* 数据字节是否被器件ACK。 */
    if (Eeprom_SendAddress(address) == INVALIDFLAG)
    {
        Eeprom_Stop();
        return INVALIDFLAG;
    }
    g_sst_eeprom_status.stage = 4U;
    result = Eeprom_SendByte(value);
    Eeprom_Stop();
    if (result == VALIDFLAG)
        g_sst_eeprom_status.stage = 0U;
    return result;
}

/*==================================================================
 * 函数功能: 连续写入，按128字节物理页边界分段并有限轮询写完成
 * 输入参数: address：EEPROM绝对起始字节地址
 *           data：待写字节缓存，至少length字节
 *           length：写入字节数，非0且不超过器件末尾
 * 返回值: VALIDFLAG所有段写完成；INVALIDFLAG范围、总线或应答错误
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 Eeprom_Write(unsigned short address, const Uint8 *data, unsigned short length)
{
    unsigned short written = 0U; /* 已写完并确认内部写周期结束的字节数。 */
    unsigned short chunk; /* 本次不跨128字节物理页的分段长度。 */
    unsigned short index; /* 分段内字节序号。 */
    if ((data == 0) || (length == 0U) || ((unsigned long)address + length > SST_EEPROM_CAPACITY_BYTES))
    {
        g_sst_eeprom_status.error = EEPROM_ERROR_ADDRESS;
        return INVALIDFLAG;
    }
    while (written < length)
    {
        chunk = SST_EEPROM_PHYSICAL_PAGE_BYTES - (address % SST_EEPROM_PHYSICAL_PAGE_BYTES);
        if (chunk > length - written)
            chunk = length - written;
        if ((Eeprom_WaitReady() == INVALIDFLAG) || (Eeprom_SendAddress(address) == INVALIDFLAG))
        {
            Eeprom_Stop();
            return INVALIDFLAG;
        }
        g_sst_eeprom_status.stage = 4U;
        for (index = 0U; index < chunk; ++index)
        {
            if (Eeprom_SendByte(data[written + index]) == INVALIDFLAG)
            {
                Eeprom_Stop();
                return INVALIDFLAG;
            }
        }
        Eeprom_Stop();
        if (Eeprom_WaitReady() == INVALIDFLAG)
            return INVALIDFLAG;
        written += chunk;
        address = (unsigned short)(address + chunk);
    }
    g_sst_eeprom_status.stage = 0U;
    return VALIDFLAG;
}

/*==================================================================
 * 函数功能: 按原64字节逻辑页发起单字节写，供每10ms异步保存使用
 * 输入参数: page：逻辑页号，绝对地址的页部分为page乘64
 *           offset：该逻辑页内字节偏移，调用者按0..63组织
 *           value：待写字节
 * 返回值: 底层VALIDFLAG或INVALIDFLAG；不等待内部写周期
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 r_eeprom_byte_write(Uint8 page, Uint8 offset, Uint8 value)
{
    return Eeprom_StartByteWrite((unsigned short)(page * SST_EEPROM_LOGICAL_PAGE_BYTES + offset), value);
}

/*==================================================================
 * 函数功能: 原逻辑页接口的连续段写；不执行X1226解锁或RTC寻址
 * 输入参数: page：原64字节逻辑页号
 *           offset：起始字节偏移
 *           length：连续写入字节数
 *           data：至少length字节的待写缓存
 * 返回值: VALIDFLAG全部写完成；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 r_eeprom_section_write(Uint8 page, Uint8 offset, Uint8 length, Uint8 *data)
{
    return Eeprom_Write((unsigned short)(page * SST_EEPROM_LOGICAL_PAGE_BYTES + offset), data, length);
}

/*==================================================================
 * 函数功能: 原逻辑页接口的连续段读，显式返回I2C成功状态
 * 输入参数: page：原64字节逻辑页号
 *           offset：起始字节偏移
 *           length：连续读取字节数，可跨逻辑页
 *           data：至少length字节的接收缓存
 * 返回值: VALIDFLAG读取完成，此时缓存有效；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 r_eeprom_section_read(Uint8 page, Uint8 offset, Uint8 length, Uint8 *data)
{
    return Eeprom_Read((unsigned short)(page * SST_EEPROM_LOGICAL_PAGE_BYTES + offset), data, length);
}

/*==================================================================
 * 函数功能: 读取完整的64字节逻辑页；不把逻辑页改为128字节
 * 输入参数: page：原64字节逻辑页号
 *           data：至少64字节的接收缓存
 * 返回值: VALIDFLAG读取完成；INVALIDFLAG失败
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 r_eeprom_page_read(Uint8 page, Uint8 *data)
{
    return r_eeprom_section_read(page, 0U, SST_EEPROM_LOGICAL_PAGE_BYTES, data);
}

/*==================================================================
 * 函数功能: 兼容旧调用的单字节读，失败返回FF；新镜像使用带状态的段读
 * 输入参数: page：原64字节逻辑页号
 *           offset：页内字节偏移
 * 返回值: 读取的数据，失败FF；此接口不能区分有效FF和无应答
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
Uint8 r_eeprom_byte_read(Uint8 page, Uint8 offset)
{
    Uint8 value = 0xFFU; /* 旧接口用FF表示失败；新参数镜像不能靠该字节判断应答。 */
    (void)r_eeprom_section_read(page, offset, 1U, &value);
    return value;
}
