#include "main.h"
#include "MaintenanceProtocol.H"
#include "DPRAM.H"
#include "EEProm.H"
#include "FaultCheck.H"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "hw_types.h"
#include "uart.h"
#include "gpio.h"
#include "SstDebug.h"
#include "SstParams.h"

/* MAINT_FRAME_SIZE：维护完整帧最大字节数，含 8 字节头及 2 字节 CRC。 */
#define MAINT_FRAME_SIZE        (MAINT_MAX_PAYLOAD + 10)
/* MAINT_STATUS_WORDS：维护固定状态对象长度，16 个字。 */
#define MAINT_STATUS_WORDS      16
/* MAINT_PARAM_COUNT：旧维护参数对象索引数量，28 个。 */
#define MAINT_PARAM_COUNT       28
/* MAINT_ALL_PARAM_WORDS：旧维护全部参数编码字数，44 个字。 */
#define MAINT_ALL_PARAM_WORDS   44
/* MAINT_PLUGIN_COUNT：旧 FPGA 插件对象数量，15 个。 */
#define MAINT_PLUGIN_COUNT      15
/* MAINT_PLUGIN_SUMMARY_WORDS：插件摘要长度，每插件 5 个字。 */
#define MAINT_PLUGIN_SUMMARY_WORDS (MAINT_PLUGIN_COUNT * 5)
/* MAINT_PERSIST_MAGIC0：旧维护参数镜像魔数第一字节 4D。 */
#define MAINT_PERSIST_MAGIC0    0x4D
/* MAINT_PERSIST_MAGIC1：旧维护参数镜像魔数第二字节 50。 */
#define MAINT_PERSIST_MAGIC1    0x50

static volatile unsigned char MaintRxQueue[MAINT_RX_QUEUE_SIZE]; /* MaintRxQueue：维护接收字节环形缓存，后台从队尾解析。 */
static volatile unsigned short MaintRxHead; /* MaintRxHead：维护接收环下一写入位置。 */
static volatile unsigned short MaintRxTail; /* MaintRxTail：维护接收环下一读取位置。 */
static unsigned char MaintFrame[MAINT_FRAME_SIZE]; /* MaintFrame：当前维护帧的组装字节缓存。 */
static unsigned short MaintFrameIndex; /* MaintFrameIndex：当前维护帧已组装字节数。 */
static unsigned short MaintFrameExpected; /* MaintFrameExpected：当前维护帧预期总字节数。 */
static unsigned char MaintFrameState; /* MaintFrameState：维护帧头、长度及载荷解析状态。 */
/* 线上的维护帧分流状态。维护协议在主循环解析，分流状态只用于UART ISR。 */
static unsigned short MaintWireIndex; /* MaintWireIndex：UART 字节分流的维护帧位置。 */
static unsigned short MaintWireExpected; /* MaintWireExpected：UART 分流预期维护帧长度。 */
static unsigned char MaintWireState; /* MaintWireState：UART 字节分流的维护帧头识别状态。 */
static unsigned char MaintWireHeader[8]; /* MaintWireHeader：UART 分流使用的八字节维护帧头缓存。 */
static unsigned char MaintSaveImage[92]; /* MaintSaveImage：维护协议页 4/5 的 92 字节保存镜像。 */
static unsigned short MaintSavePosition; /* MaintSavePosition：维护镜像逐字节保存的当前步序。 */
static unsigned char MaintSavePending, MaintSaveSequence; /* MaintSavePending：维护镜像异步保存忙标志。 */
/* MaintSaveSequence：保存请求序号，用于后台完成响应。 */
static uint32_t MaintSaveTick; /* MaintSaveTick：维护保存上次推进的 10 ms 节拍。 */
/*==================================================================
 * 函数功能: 查询维护协议参数保存任务是否仍在执行
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 非零表示维护保存忙
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
unsigned short MaintenanceProtocol_IsSaving(void) { return MaintSavePending; }

/*==================================================================
 * 函数功能: 读取旧参数表的一个字或浮点数的指定半字
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 *           word：参数浮点值的半字序号/16 位数据，按对应函数约定
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 参数的 16 位编码字
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned short MaintReadParameter(unsigned char index, unsigned short word);
/*==================================================================
 * 函数功能: 校验旧参数长度和值并写入端口配置或浮点给定
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 *           data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           count：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 维护协议 MAINT_ERR_* 结果码
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintSetParameter(unsigned char index, const unsigned char *data,
                                       unsigned short count);
/*==================================================================
 * 函数功能: 检查编译端口是否支持指定旧参数索引
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 非零表示支持
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintParameterSupported(unsigned char index);
/*==================================================================
 * 函数功能: 检查旧参数索引是否允许写入
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 非零表示可写
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintParameterWritable(unsigned char index);

/*
 * 485 模式的启动/停止命令最终进入简化 CHB/DAB 状态机。
 *
 * 旧工程的维护协议仍然使用 DPRAM_RemControl 作为兼容缓存，并由
 * 把旧帧的启停/脉冲/故障复位字段映射到 g_chb_dab_command。
 * 禁止调用原先会直接开门极的 RemoteControl_Dispose()。
 */
/*==================================================================
 * 函数功能: 把旧遥控缓存的启停、脉冲和复位字段转为统一 CHB/DAB 命令
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintApplyChbCommand(void)
{
#if SST_ALLOW_SERIAL_COMMANDS
    unsigned short start, stop, on, off; /* start：旧遥控启动位图，停止/关脉冲优先。 */
    /* stop：旧遥控停止位图，停止优先于启动。 */
    /* on：旧脉冲使能位图，bit0 DAB、bit1 CHB。 */
    /* off：旧脉冲禁用位图，优先于同位使能。 */
    start = PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16;
    stop = PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16;
    on = PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16;
    off = PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16;
    if(SstParams_IsStorageBusy()) { start = 0; on = 0; }
    if(stop & 1U) { start = 0; on = 0; }
    ChbDabControl_ApplyRemoteCommand(start, stop);
    if(off & 1U) g_chb_dab_command.dab_enable = 0;
    else if(on & 1U) g_chb_dab_command.dab_enable = 1;
    if(off & 2U) g_chb_dab_command.run_enable = 0;
    else if(on & 2U) g_chb_dab_command.run_enable = 1;
    if((PORT_Control_DATA_Buf.DPRAM_RemControl.COM_FAULT_CLEAR.COM_FAULT_CLEAR16 & 1U) && SstParams_IsStopped() && !SstParams_IsStorageBusy())
        g_chb_dab_command.fault_reset = 1;
#endif
}
typedef union _MAINT_FLOAT_WORDS {
    float value; /* value：本次协议/存储处理的值，类型和编码保持原接口约定。 */
    unsigned short word[2]; /* word：本次协议/存储处理的值，类型和编码保持原接口约定。 */
} MAINT_FLOAT_WORDS;

/*==================================================================
 * 函数功能: 从字节缓存读取小端 16 位字
 * 输入参数: data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 小端解码后的字值
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned short MaintRead16(const unsigned char *data)
{
    return (unsigned short)data[0] | ((unsigned short)data[1] << 8);
}

/*==================================================================
 * 函数功能: 把 16 位字按小端顺序写入字节缓存
 * 输入参数: data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: data[0..1]，低字节在前。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintWrite16(unsigned char *data, unsigned short value)
{
    data[0] = (unsigned char)value;
    data[1] = (unsigned char)(value >> 8);
}

/*==================================================================
 * 函数功能: 计算维护帧的 Modbus CRC16，初值 FFFF、多项式 A001
 * 输入参数: data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           length：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 16 位 CRC 值
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned short MaintCRC16(const unsigned char *data, unsigned short length)
{
    unsigned short crc; /* crc：逐字节、逐位累加的 CRC 校验状态。 */
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */
    unsigned char bit; /* bit：CRC 或软件 I²C 的当前位序号，范围 0..7。 */

    crc = 0xFFFF;
    for(i = 0; i < length; i++)
    {
        crc ^= data[i];
        for(bit = 0; bit < 8; bit++)
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/*==================================================================
 * 函数功能: 复位维护帧解析位置、预期长度和状态
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintResetParser(void)
{
    MaintFrameIndex = 0;
    MaintFrameExpected = 0;
    MaintFrameState = 0;
}

/*==================================================================
 * 函数功能: 组装维护响应帧及 CRC，提交共享 UART 整帧队列
 * 输入参数: flags：维护帧响应/错误标志位图
 *           sequence：维护请求序号，响应原样回带
 *           command：维护协议命令编号
 *           payload：输入帧载荷缓存，不含帧头与 CRC
 *           length：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintSendFrame(unsigned char flags, unsigned char sequence,
                           unsigned char command, const unsigned char *payload,
                           unsigned short length)
{
    unsigned char frame[MAINT_FRAME_SIZE]; /* frame：完整发送帧的本地字节缓存，包含帧头及校验。 */
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */
    unsigned short crc; /* crc：逐字节、逐位累加的 CRC 校验状态。 */


    if(length > MAINT_MAX_PAYLOAD)
        return;

    frame[0] = MAINT_SOF0;
    frame[1] = MAINT_SOF1;
    frame[2] = MAINT_VERSION;
    frame[3] = flags;
    frame[4] = sequence;
    frame[5] = command;
    MaintWrite16(&frame[6], length);
    for(i = 0; i < length; i++)
        frame[8 + i] = payload[i];

    crc = MaintCRC16(frame, (unsigned short)(8 + length));
    MaintWrite16(&frame[8 + length], crc);

    SstSerial_SendFrame(frame, (uint16_t)(10 + length));
}

/*==================================================================
 * 函数功能: 组装维护协议错误载荷并发送错误响应
 * 输入参数: sequence：维护请求序号，响应原样回带
 *           command：维护协议命令编号
 *           error：维护协议 MAINT_ERR_* 错误码
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintSendError(unsigned char sequence, unsigned char command,
                           unsigned char error)
{
    unsigned char payload[2]; /* payload：帧载荷字节缓存/指针，不含帧头和 CRC。 */
    payload[0] = error;
    payload[1] = 0;
    MaintSendFrame(MAINT_FLAG_RESPONSE | MAINT_FLAG_ERROR, sequence,
                   command, payload, 2);
}

/*==================================================================
 * 函数功能: 查询对象及实例所支持的 16 位字数
 * 输入参数: object：维护协议对象类型编号
 *           index：所选表/对象的索引；范围按函数对应表容量检查
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 有效对象的字数；不支持返回 0
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned short MaintObjectLength(unsigned char object, unsigned char index)
{
    if(object == MAINT_OBJ_STATUS)
        return MAINT_STATUS_WORDS;
    if(object == MAINT_OBJ_TELEMETRY)
        return DPRAMBlock3_Length;
    if(object == MAINT_OBJ_CONTROL)
        return DPRAMBlock1_Length;
    if(object == MAINT_OBJ_REGULATE)
        return DPRAMBlock2_Length;
    if(object == MAINT_OBJ_PLUGIN)
    {
        if(index >= 15)
            return 0;
        return (unsigned short)(sizeof(FPGA_InterfaceBLOCK) / sizeof(unsigned short));
    }
    if(object == MAINT_OBJ_PARAMETER)
    {
        if(index >= MAINT_PARAM_COUNT || !MaintParameterSupported(index))
            return 0;
        return (index < 12) ? 1 : 2;
    }
    if(object == MAINT_OBJ_ALL_PARAMETERS)
        return (index == 0) ? MAINT_ALL_PARAM_WORDS : 0;
    if(object == MAINT_OBJ_PLUGIN_SUMMARY)
        return (index == 0) ? MAINT_PLUGIN_SUMMARY_WORDS : 0;
    return 0;
}

/*==================================================================
 * 函数功能: 按对象、实例和偏移读取兼容状态、遥测、插件或参数字
 * 输入参数: object：维护协议对象类型编号
 *           index：所选表/对象的索引；范围按函数对应表容量检查
 *           offset：对象中的字偏移或镜像字节偏移，按接口说明解释
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 对应的 16 位字
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned short MaintReadObjectWord(unsigned char object, unsigned char index,
                                          unsigned short offset)
{
    unsigned short status[MAINT_STATUS_WORDS]; /* status：维护协议固定状态字响应缓存。 */
    volatile unsigned short *source; /* source：读取对象的 16 位源地址。 */

    if(object == MAINT_OBJ_STATUS)
    {
        status[0] = PORT_Index;
        status[1] = Port_State;
        status[2] = Port_RunStatus;
        status[3] = Port_MainStatus.MainStatus16;
        status[4] = Port_RealStatus.PORT_STATUS16;
        status[5] = Port_RealFault.PORT_FAULT16;
        status[6] = PortRunFLAG.PortRunFLAG16;
        status[7] = ControlFLAG.ControlFLAG16;
        status[8] = FaultTypeFLAG.FaultTypeFLAG16;
        status[9] = FaultDisposeFLAG.FaultDisposeFLAG16;
        status[10] = SW.SwitchFLAG16;
        status[11] = Port_DSP_CTL.DSP_CTL16;
        status[12] = Port_DOUT.SwitchINOUT16;
        status[13] = PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16;
        status[14] = (unsigned short)Flag_Uart;
        status[15] = (unsigned short)MaintRxHead - MaintRxTail;
        return status[offset];
    }
    if(object == MAINT_OBJ_TELEMETRY)
        source = &PORT_RealTime_DATA_buf.DPRAM_Block3Buf[offset];
    else if(object == MAINT_OBJ_CONTROL)
        source = &PORT_Control_DATA_Buf.DPRAM_Block1Buf[offset];
    else if(object == MAINT_OBJ_REGULATE)
        source = &PORT_Regulate_DATA_Buf.DPRAM_Block2Buf[offset];
    else if(object == MAINT_OBJ_PARAMETER)
        return MaintReadParameter(index, offset);
    else if(object == MAINT_OBJ_ALL_PARAMETERS)
    {
        if(offset < 12)
            return MaintReadParameter((unsigned char)offset, 0);
        return MaintReadParameter((unsigned char)(12 + ((offset - 12) / 2)),
                                  (unsigned short)((offset - 12) & 1));
    }
    else if(object == MAINT_OBJ_PLUGIN_SUMMARY)
        return ((volatile unsigned short *)&FPGA_Interface[offset / 5])[offset % 5];
    else
        source = 0;

    if(object == MAINT_OBJ_PLUGIN)
        return ((volatile unsigned short *)&FPGA_Interface[index])[offset];
    return *source;
}

/*==================================================================
 * 函数功能: 读取旧参数表的一个字或浮点数的指定半字
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 *           word：参数浮点值的半字序号/16 位数据，按对应函数约定
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 参数的 16 位编码字
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned short MaintReadParameter(unsigned char index, unsigned short word)
{
    MAINT_FLOAT_WORDS value; /* value：本次协议/存储处理的值，类型和编码保持原接口约定。 */

#ifdef HVAC_online
    switch(index)
    {
        case 0: return HA_PM_A1;
        case 1: return HA_PM_A2;
        case 2: return HA_PM_B1;
        case 3: return HA_PM_B2;
        case 4: return HA_PM_C1;
        case 5: return HA_PM_C2;
        case 6: return (unsigned short)OC_level_SET;
        case 7: return (unsigned short)OC_time_SET;
        case 8: return (unsigned short)OC_level_SET_L2;
        case 9: return (unsigned short)OC_time_SET_L2;
        case 10: return (unsigned short)OV_level_SET;
        case 11: return (unsigned short)OV_time_SET;
        default: break;
    }

    switch(index)
    {
        case 12: value.value = PLL_PID_Num_1; break;
        case 13: value.value = PLL_PID_Num_0; break;
        case 14: value.value = Volt_PID_Num_1; break;
        case 15: value.value = Volt_PID_Num_0; break;
        case 16: value.value = PH_Volt_PID_Num_1; break;
        case 17: value.value = PH_Volt_PID_Num_0; break;
        case 18: value.value = Har_2_Controller_Num_1; break;
        case 19: value.value = Har_2_Controller_Num_0; break;
        case 20: value.value = Har_6_Controller_Num_1; break;
        case 21: value.value = Har_6_Controller_Num_0; break;
        case 22: value.value = Har_12_Controller_Num_1; break;
        case 23: value.value = Har_12_Controller_Num_0; break;
        case 24: value.value = Current_PID_Num_1; break;
        case 25: value.value = Current_PID_Num_0; break;
        case 26: value.value = Neg_Current_PID_Num_1; break;
        case 27: value.value = Neg_Current_PID_Num_0; break;
        default: return 0;
    }
#elif defined(HVDC_online)
    switch(index)
    {
        case 0: return HD_PM_A1;
        case 1: return HD_PM_A2;
        case 2: return NHD_module;
        default: break;
    }

    switch(index)
    {
        case 12: value.value = Volt_PID_Num_1; break;
        case 13: value.value = Volt_PID_Num_0; break;
        case 14: value.value = ModuleVolt_PID_Num_1; break;
        case 15: value.value = ModuleVolt_PID_Num_0; break;
        case 16: value.value = Current_PID_Num_1; break;
        case 17: value.value = Current_PID_Num_0; break;
        default: return 0;
    }
#else
    return 0;
#endif
    return value.word[word & 1];
}

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 根据 HVAC 各相模块位图重新统计在线模块数
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintRecountHVAC(void)
{
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */
    NHA_module_A = 0;
    NHA_module_B = 0;
    NHA_module_C = 0;
    for(i = 0; i < 8; i++)
    {
        if(HA_PM_A1 & (1 << i)) NHA_module_A++;
        if(HA_PM_A2 & (1 << i)) NHA_module_A++;
        if(HA_PM_B1 & (1 << i)) NHA_module_B++;
        if(HA_PM_B2 & (1 << i)) NHA_module_B++;
        if(HA_PM_C1 & (1 << i)) NHA_module_C++;
        if(HA_PM_C2 & (1 << i)) NHA_module_C++;
    }
    FPGA_Interface[3].PM = HA_PM_A1;
    FPGA_Interface[4].PM = HA_PM_A2;
    FPGA_Interface[7].PM = HA_PM_B1;
    FPGA_Interface[8].PM = HA_PM_B2;
    FPGA_Interface[11].PM = HA_PM_C1;
    FPGA_Interface[12].PM = HA_PM_C2;
    FPGA_Interface[3].NUM = NHA_module_A;
    FPGA_Interface[4].NUM = NHA_module_A;
    FPGA_Interface[7].NUM = NHA_module_B;
    FPGA_Interface[8].NUM = NHA_module_B;
    FPGA_Interface[11].NUM = NHA_module_C;
    FPGA_Interface[12].NUM = NHA_module_C;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[0] = HA_PM_A1;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[1] = HA_PM_A2;
}
#elif defined(HVDC_online)
/*==================================================================
 * 函数功能: 根据 HVDC 模块位图重新统计在线模块数
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintRecountHVDC(void)
{
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */
    NHD_module = 0;
    for(i = 0; i < 8; i++)
    {
        if(HD_PM_A1 & (1 << i)) NHD_module++;
        if(HD_PM_A2 & (1 << i)) NHD_module++;
    }
    FPGA_Interface[3].PM = HD_PM_A1;
    FPGA_Interface[4].PM = HD_PM_A2;
    FPGA_Interface[3].NUM = NHD_module;
    FPGA_Interface[4].NUM = NHD_module;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[0] = HD_PM_A1;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[1] = HD_PM_A2;
}
#endif

/*==================================================================
 * 函数功能: 检查编译端口是否支持指定旧参数索引
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 非零表示支持
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintParameterSupported(unsigned char index)
{
#ifdef HVAC_online
    return index < MAINT_PARAM_COUNT;
#elif defined(HVDC_online)
    return (index <= 2 || (index >= 12 && index <= 17));
#else
    return 0;
#endif
}

/*==================================================================
 * 函数功能: 检查旧参数索引是否允许写入
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 非零表示可写
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintParameterWritable(unsigned char index)
{
#ifdef HVAC_online
    return index < MAINT_PARAM_COUNT;
#elif defined(HVDC_online)
    return (index <= 1 || (index >= 12 && index <= 17));
#else
    return 0;
#endif
}

/*==================================================================
 * 函数功能: 校验旧参数长度和值并写入端口配置或浮点给定
 * 输入参数: index：所选表/对象的索引；范围按函数对应表容量检查
 *           data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           count：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 维护协议 MAINT_ERR_* 结果码
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintSetParameter(unsigned char index, const unsigned char *data,
                                       unsigned short count)
{
    MAINT_FLOAT_WORDS value; /* value：本次协议/存储处理的值，类型和编码保持原接口约定。 */
    unsigned short word; /* word：本次协议/存储处理的值，类型和编码保持原接口约定。 */

    if(!SstParams_IsStopped() || SstParams_IsStorageBusy() || Port_RunStatus == PortStatusNum_Run || Port_RunStatus == PortStatusNum_Start)
        return MAINT_ERR_NOT_ALLOWED;
    if(!MaintParameterWritable(index))
        return MAINT_ERR_NOT_ALLOWED;

#ifdef HVAC_online
    if(index < 6)
    {
        if(count != 1) return MAINT_ERR_BAD_LENGTH;
        word = MaintRead16(data);
        if(index == 0) HA_PM_A1 = word;
        if(index == 1) HA_PM_A2 = word;
        if(index == 2) HA_PM_B1 = word;
        if(index == 3) HA_PM_B2 = word;
        if(index == 4) HA_PM_C1 = word;
        if(index == 5) HA_PM_C2 = word;
        MaintRecountHVAC();
        return MAINT_ERR_OK;
    }
    if(index <= 11)
    {
        if(count != 1) return MAINT_ERR_BAD_LENGTH;
        word = MaintRead16(data);
        if(index == 6) OC_level_SET = word;
        if(index == 7) OC_time_SET = word;
        if(index == 8) OC_level_SET_L2 = word;
        if(index == 9) OC_time_SET_L2 = word;
        if(index == 10) OV_level_SET = word;
        if(index == 11) OV_time_SET = word;
        return MAINT_ERR_OK;
    }
    if(index >= MAINT_PARAM_COUNT || count != 2)
        return MAINT_ERR_BAD_LENGTH;
    value.word[0] = MaintRead16(data);
    value.word[1] = MaintRead16(data + 2);
    switch(index)
    {
        case 12: PLL_PID_Num_1 = value.value; break;
        case 13: PLL_PID_Num_0 = value.value; break;
        case 14: Volt_PID_Num_1 = value.value; break;
        case 15: Volt_PID_Num_0 = value.value; break;
        case 16: PH_Volt_PID_Num_1 = value.value; break;
        case 17: PH_Volt_PID_Num_0 = value.value; break;
        case 18: Har_2_Controller_Num_1 = value.value; break;
        case 19: Har_2_Controller_Num_0 = value.value; break;
        case 20: Har_6_Controller_Num_1 = value.value; break;
        case 21: Har_6_Controller_Num_0 = value.value; break;
        case 22: Har_12_Controller_Num_1 = value.value; break;
        case 23: Har_12_Controller_Num_0 = value.value; break;
        case 24: Current_PID_Num_1 = value.value; break;
        case 25: Current_PID_Num_0 = value.value; break;
        case 26: Neg_Current_PID_Num_1 = value.value; break;
        case 27: Neg_Current_PID_Num_0 = value.value; break;
        default: return MAINT_ERR_BAD_OBJECT;
    }
#elif defined(HVDC_online)
    if(index <= 1)
    {
        if(count != 1) return MAINT_ERR_BAD_LENGTH;
        word = MaintRead16(data);
        if(index == 0) HD_PM_A1 = word;
        if(index == 1) HD_PM_A2 = word;
        MaintRecountHVDC();
        return MAINT_ERR_OK;
    }
    if(count != 2)
        return MAINT_ERR_BAD_LENGTH;
    value.word[0] = MaintRead16(data);
    value.word[1] = MaintRead16(data + 2);
    switch(index)
    {
        case 12: Volt_PID_Num_1 = value.value; break;
        case 13: Volt_PID_Num_0 = value.value; break;
        case 14: ModuleVolt_PID_Num_1 = value.value; break;
        case 15: ModuleVolt_PID_Num_0 = value.value; break;
        case 16: Current_PID_Num_1 = value.value; break;
        case 17: Current_PID_Num_0 = value.value; break;
        default: return MAINT_ERR_BAD_OBJECT;
    }
#else
    return MAINT_ERR_BAD_OBJECT;
#endif
    return MAINT_ERR_OK;
}

/*==================================================================
 * 函数功能: 执行旧遥控缓存中的安全控制，受编译权限及停机互锁约束
 * 输入参数: command：维护协议命令编号
 *           value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 维护协议 MAINT_ERR_* 结果码，成功为 MAINT_ERR_OK
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintExecuteControl(unsigned char command, unsigned short value)
{
#if SST_ALLOW_SERIAL_COMMANDS
    unsigned short clearIndex; /* clearIndex：旧故障清除命令的兼容索引。 */
    if(SstParams_IsStorageBusy() && command != MAINT_CTL_STOP && command != MAINT_CTL_HF_OFF && command != MAINT_CTL_LF_OFF) return MAINT_ERR_BUSY;
    for(clearIndex = 0; clearIndex < DPRAMBlock1_Length; ++clearIndex)
        PORT_Control_DATA_Buf.DPRAM_Block1Buf[clearIndex] = 0;
    /* 维护命令是单次触发量，清掉上一次启停位，避免旧启动位压住新的停止位。 */
    PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16 = 0;
    PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16 = 0;
    if(command == MAINT_CTL_SWITCH_ON || command == MAINT_CTL_SWITCH_OFF ||
       command == MAINT_CTL_SWITCH_EN || command == MAINT_CTL_SWITCH_DIS || command == MAINT_CTL_MODE)
        return MAINT_ERR_NOT_ALLOWED;
    if(command == MAINT_CTL_START)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16 = value ? value : (1 << (PORT_Index - 1));
    else if(command == MAINT_CTL_STOP)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16 = value ? value : (1 << (PORT_Index - 1));
    else if(command == MAINT_CTL_MODE)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PORT_MODE.COM_PORT_MODE16 = value;
    else if(command == MAINT_CTL_SWITCH_ON)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_ON.COM_SWITCH16 = value ? value : 1;
    else if(command == MAINT_CTL_SWITCH_OFF)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_SWITCH_OFF.COM_SWITCH16 = value ? value : 1;
    else if(command == MAINT_CTL_HF_ON)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 = 1;
    else if(command == MAINT_CTL_LF_ON)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16 = 2;
    else if(command == MAINT_CTL_HF_OFF)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 = 1;
    else if(command == MAINT_CTL_LF_OFF)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16 = 2;
    else if(command == MAINT_CTL_FAULT_CLEAR)
        PORT_Control_DATA_Buf.DPRAM_RemControl.COM_FAULT_CLEAR.COM_FAULT_CLEAR16 = 1;
    else if(command == MAINT_CTL_SWITCH_EN)
        PORT_Control_DATA_Buf.DPRAM_RemControl.PORT_SWITCH_EN.PORT_SWITCH_EN16 = 1;
    else if(command == MAINT_CTL_SWITCH_DIS)
        PORT_Control_DATA_Buf.DPRAM_RemControl.PORT_SWITCH_EN.PORT_SWITCH_EN16 = 2;
    else
        return MAINT_ERR_BAD_COMMAND;

    /* 485 模式下立即同步简化状态机，不能等 DPRAM 中断。 */
    MaintApplyChbCommand();

    /* Commands are applied only through the CHB controller; never legacy direct gate writes. */
    for(clearIndex = 0; clearIndex < DPRAMBlock1_Length; ++clearIndex)
        PORT_Control_DATA_Buf.DPRAM_Block1Buf[clearIndex] = 0;
    return MAINT_ERR_OK;
#else
    (void)command;
    (void)value;
    return MAINT_ERR_NOT_ALLOWED;
#endif
}

/*==================================================================
 * 函数功能: 检查对象范围和写入权限后更新旧兼容缓存或参数
 * 输入参数: object：维护协议对象类型编号
 *           index：所选表/对象的索引；范围按函数对应表容量检查
 *           offset：对象中的字偏移或镜像字节偏移，按接口说明解释
 *           count：有效字节数；维护对象按其接口约定使用 16 位字数
 *           data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 维护协议 MAINT_ERR_* 结果码
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintWriteObject(unsigned char object, unsigned char index,
                                      unsigned short offset, unsigned short count,
                                      const unsigned char *data)
{
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */
    unsigned short length; /* length：有效数据数量；字节缓存按字节、16 位对象复制按字计数。 */
    volatile unsigned short *target; /* target：写对象的 16 位目标地址。 */

    if(object == MAINT_OBJ_CONTROL && !SST_ALLOW_SERIAL_COMMANDS) return MAINT_ERR_NOT_ALLOWED;
    length = MaintObjectLength(object, index);
    if(length == 0) return MAINT_ERR_BAD_OBJECT;
    if(count == 0 || offset >= length || count > (unsigned short)(length - offset))
        return MAINT_ERR_BAD_RANGE;

    if(object == MAINT_OBJ_CONTROL)
    {
        for(i = 0; i < DPRAMBlock1_Length; ++i)
            PORT_Control_DATA_Buf.DPRAM_Block1Buf[i] = 0;
        for(i = 0; i < count; ++i)
            PORT_Control_DATA_Buf.DPRAM_Block1Buf[offset + i] = MaintRead16(data + 2*i);
        /* Start/on are filtered while storage owns the stopped configuration;
           stop/off are still applied and cannot be held up by a save. */
        MaintApplyChbCommand();
        for(i = 0; i < DPRAMBlock1_Length; ++i)
            PORT_Control_DATA_Buf.DPRAM_Block1Buf[i] = 0;
        return MAINT_ERR_OK;
    }
    if(!SstParams_IsStopped() || SstParams_IsStorageBusy()) return MAINT_ERR_NOT_ALLOWED;

    if(object == MAINT_OBJ_PLUGIN)
    {
        if(offset > 4 || (offset + count) > 5)
            return MAINT_ERR_NOT_ALLOWED;
        target = (volatile unsigned short *)&FPGA_Interface[index];
    }
    else if(object == MAINT_OBJ_PARAMETER)
        return MaintSetParameter(index, data, count);
    else if(object == MAINT_OBJ_CONTROL)
        target = PORT_Control_DATA_Buf.DPRAM_Block1Buf;
    else if(object == MAINT_OBJ_REGULATE)
        target = PORT_Regulate_DATA_Buf.DPRAM_Block2Buf;
    else
        return MAINT_ERR_NOT_ALLOWED;

    if(object == MAINT_OBJ_REGULATE && (Port_RunStatus == PortStatusNum_Run || Port_RunStatus == PortStatusNum_Start))
        return MAINT_ERR_NOT_ALLOWED;
    for(i = 0; i < count; i++)
        target[offset + i] = MaintRead16(data + 2 * i);
    if(object == MAINT_OBJ_CONTROL)
    {
        /* 维护帧直接写控制对象时，也要走同一条 CHB/DAB 命令映射。 */
        MaintApplyChbCommand();
        /* Commands are applied only through the CHB controller; never legacy direct gate writes. */
        for(i = 0; i < DPRAMBlock1_Length; i++)
            PORT_Control_DATA_Buf.DPRAM_Block1Buf[i] = 0;
    }
    else if(object == MAINT_OBJ_REGULATE)
        RemoteRegulate_Dispose_2(0);
    return MAINT_ERR_OK;
}

/*==================================================================
 * 函数功能: 把旧维护参数组装为页 4/5 镜像并启动后台逐字节保存
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 维护协议 MAINT_ERR_* 结果码
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static unsigned char MaintSavePersistent(void)
{
    unsigned char data[104]; /* data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定。 */
    unsigned short pos; /* pos：持久化镜像参数打包/解包的位置。 */
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */
    unsigned short word; /* word：本次协议/存储处理的值，类型和编码保持原接口约定。 */

    if(!SstParams_IsStopped()) return MAINT_ERR_NOT_ALLOWED;
    for(i = 0; i < 104; i++) data[i] = 0;
    data[0] = MAINT_PERSIST_MAGIC0;
    data[1] = MAINT_PERSIST_MAGIC1;
    data[2] = MAINT_VERSION;
    /* Prevent parameters saved by one port type from loading into another. */
    data[3] = PORT_Index;
    pos = 4;
    for(i = 0; i < MAINT_PARAM_COUNT; i++)
    {
        word = MaintReadParameter((unsigned char)i, 0);
        data[pos++] = (unsigned char)word;
        data[pos++] = (unsigned char)(word >> 8);
        if(i >= 12)
        {
            word = MaintReadParameter((unsigned char)i, 1);
            data[pos++] = (unsigned char)word;
            data[pos++] = (unsigned char)(word >> 8);
        }
    }
    if(SstParams_IsStorageBusy()) return MAINT_ERR_BUSY;
    for(i = 0; i < 92; ++i) MaintSaveImage[i] = data[i];
    MaintSavePosition = 0;
    MaintSavePending = 1;
    MaintSaveTick = SstDebug_GetTick10ms() - 1;
    return MAINT_ERR_OK;
}

/*==================================================================
 * 函数功能: 处理校验通过的维护请求并构造对应读写或控制响应
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
static void MaintHandleFrame(void)
{
    unsigned char command; /* command：维护请求/响应命令码。 */
    unsigned char sequence; /* sequence：维护协议请求序号，或详细故障记录的递增事件序号。 */
    unsigned char object; /* object：维护协议对象类型编号。 */
    unsigned char index; /* index：当前维护对象/参数实例索引。 */
    unsigned char error; /* error：本次维护操作的 MAINT_ERR_* 结果码。 */
    unsigned short length; /* length：有效数据数量；字节缓存按字节、16 位对象复制按字计数。 */
    unsigned short offset; /* offset：当前对象字偏移或保存镜像字节偏移。 */
    unsigned short count; /* count：有效数据数量；字节缓存按字节、16 位对象复制按字计数。 */
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */
    unsigned short outLength; /* outLength：维护响应有效载荷长度，单位字节。 */
    unsigned char response[MAINT_MAX_PAYLOAD]; /* response：维护请求处理生成的响应载荷缓存。 */

    if(MaintFrame[2] != MAINT_VERSION)
    {
        MaintSendError(MaintFrame[4], MaintFrame[5], MAINT_ERR_BAD_VERSION);
        return;
    }
    sequence = MaintFrame[4];
    command = MaintFrame[5];
    length = MaintRead16(&MaintFrame[6]);
    object = (length > 0) ? MaintFrame[8] : 0;
    index = (length > 1) ? MaintFrame[9] : 0;

    if(command == MAINT_CMD_HELLO)
    {
        response[0] = MAINT_ERR_OK;
        response[1] = MAINT_VERSION;
        MaintWrite16(&response[2], PORT_Index);
        MaintWrite16(&response[4], 16);
        MaintWrite16(&response[6], 0x003F);
        /* The baud value is reported in units of 100 bit/s. */
        MaintWrite16(&response[8], MAINT_UART_BAUD / 100);
        MaintSendFrame(MAINT_FLAG_RESPONSE, sequence, command, response, 10);
        return;
    }
    if(command == MAINT_CMD_READ_OBJECT)
    {
        if(length != 6)
        {
            MaintSendError(sequence, command, MAINT_ERR_BAD_LENGTH);
            return;
        }
        offset = MaintRead16(&MaintFrame[10]);
        count = MaintRead16(&MaintFrame[12]);
        if(MaintObjectLength(object, index) == 0 || count == 0 || count > 96 ||
           offset >= MaintObjectLength(object, index) || count > (unsigned short)(MaintObjectLength(object, index) - offset))
        {
            MaintSendError(sequence, command, MAINT_ERR_BAD_RANGE);
            return;
        }
        response[0] = MAINT_ERR_OK;
        response[1] = object;
        response[2] = index;
        MaintWrite16(&response[3], offset);
        MaintWrite16(&response[5], count);
        for(i = 0; i < count; i++)
        {
            MaintWrite16(&response[7 + 2 * i], MaintReadObjectWord(object, index, offset + i));
        }
        outLength = (unsigned short)(7 + 2 * count);
        MaintSendFrame(MAINT_FLAG_RESPONSE, sequence, command, response, outLength);
        return;
    }
    if(command == MAINT_CMD_WRITE_OBJECT)
    {
        if(length < 6)
        {
            MaintSendError(sequence, command, MAINT_ERR_BAD_LENGTH);
            return;
        }
        offset = MaintRead16(&MaintFrame[10]);
        count = MaintRead16(&MaintFrame[12]);
        if(length != (unsigned short)(6 + 2 * count))
        {
            MaintSendError(sequence, command, MAINT_ERR_BAD_LENGTH);
            return;
        }
        error = MaintWriteObject(object, index, offset, count, &MaintFrame[14]);
        if(error != MAINT_ERR_OK)
        {
            MaintSendError(sequence, command, error);
            return;
        }
        response[0] = MAINT_ERR_OK;
        response[1] = object;
        response[2] = index;
        MaintWrite16(&response[3], offset);
        MaintWrite16(&response[5], count);
        MaintSendFrame(MAINT_FLAG_RESPONSE, sequence, command, response, 7);
        return;
    }
    if(command == MAINT_CMD_EXEC_CONTROL)
    {
        if(length != 3)
        {
            MaintSendError(sequence, command, MAINT_ERR_BAD_LENGTH);
            return;
        }
        error = MaintExecuteControl(object, MaintRead16(&MaintFrame[9]));
        if(error != MAINT_ERR_OK)
        {
            MaintSendError(sequence, command, error);
            return;
        }
        response[0] = MAINT_ERR_OK;
        response[1] = object;
        MaintWrite16(&response[2], Port_RunStatus);
        MaintSendFrame(MAINT_FLAG_RESPONSE, sequence, command, response, 4);
        return;
    }
    if(command == MAINT_CMD_SAVE)
    {
        error = MaintSavePersistent();
        if(error != MAINT_ERR_OK)
        {
            MaintSendError(sequence, command, error);
            return;
        }
        /* Completion reply is sent after the EEPROM write cycle finishes. */
        MaintSaveSequence = sequence;
        return;
    }
    MaintSendError(sequence, command, MAINT_ERR_BAD_COMMAND);
}

/*==================================================================
 * 函数功能: 清空维护接收队列、帧解析及持久化任务状态
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_Init(void)
{
    /* 维护协议使用环形接收队列，初始化时同时清空线缆帧和解析帧状态。 */
    MaintRxHead = 0;
    MaintRxTail = 0;
    MaintResetParser();
    MaintWireIndex = 0;
    MaintWireExpected = 0;
    MaintWireState = 0;
    MaintWireHeader[0] = 0;
    MaintSavePending = 0;
}

/*==================================================================
 * 函数功能: 将接收字节放入维护协议环形队列
 * 输入参数: value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_RxByte(unsigned char value)
{
    unsigned short next; /* next：环形接收队列下一写入下标。 */
    next = (unsigned short)((MaintRxHead + 1) % MAINT_RX_QUEUE_SIZE);
    if(next != MaintRxTail)
    {
        MaintRxQueue[MaintRxHead] = value;
        MaintRxHead = next;
    }
}

/*==================================================================
 * 函数功能: 检查维护帧头与长度以决定当前字节是否属于维护协议
 * 输入参数: value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 非零表示字节已被维护协议接收
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
unsigned char MaintenanceProtocol_RxByteForUart(unsigned char value)
{
    /*
     * 旧协议和维护协议共用UART2。只有确认收到A5 5A后，才把字节送入
     * 维护队列；否则旧协议继续按原来的地址帧处理。这样一个普通的
     * A5或异常帧不会在维护解析队列中留下“半个帧头”。
     */
    if(MaintWireState == 0)
    {
        if(value != MAINT_SOF0)
            return 0;
        MaintWireState = 1;
        MaintWireIndex = 1;
        MaintWireExpected = 0;
        MaintWireHeader[0] = value;
        return 1;
    }

    if(MaintWireState == 1)
    {
        if(value == MAINT_SOF0)
        {
            /* 连续两个A5时，后一个作为新的候选帧头。 */
            MaintWireIndex = 1;
            MaintWireHeader[0] = value;
            return 1;
        }
        if(value != MAINT_SOF1)
        {
            /* 当前字节可能是旧协议的地址头，交回旧解析器。 */
            MaintWireState = 0;
            MaintWireIndex = 0;
            MaintWireExpected = 0;
            return 0;
        }
        MaintWireState = 2;
        MaintWireIndex = 2;
        MaintWireHeader[1] = value;
        MaintenanceProtocol_RxByte(MaintWireHeader[0]);
        MaintenanceProtocol_RxByte(value);
        return 1;
    }

    if(MaintWireIndex < 8)
        MaintWireHeader[MaintWireIndex] = value;
    MaintenanceProtocol_RxByte(value);
    MaintWireIndex++;
    if(MaintWireIndex == 8)
    {
        unsigned short payloadLength; /* payloadLength：维护帧头声明的载荷长度，单位字节。 */
        payloadLength = (unsigned short)MaintWireHeader[6] |
                        ((unsigned short)MaintWireHeader[7] << 8);
        if(payloadLength > MAINT_MAX_PAYLOAD)
        {
            /* 长度非法时丢弃当前候选帧，避免把后续内容误交旧协议。 */
            MaintWireState = 0;
            MaintWireIndex = 0;
            MaintWireExpected = 0;
            return 1;
        }
        MaintWireExpected = (unsigned short)(10 + payloadLength);
    }
    if(MaintWireExpected != 0 && MaintWireIndex >= MaintWireExpected)
    {
        MaintWireState = 0;
        MaintWireIndex = 0;
        MaintWireExpected = 0;
    }
    else if(MaintWireIndex >= MAINT_FRAME_SIZE)
    {
        MaintWireState = 0;
        MaintWireIndex = 0;
        MaintWireExpected = 0;
    }
    return 1;
}

/*==================================================================
 * 函数功能: 在接收预算内推进维护帧解析及逐字节参数保存
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_Process(void)
{
    unsigned char value; /* value：本次协议/存储处理的值，类型和编码保持原接口约定。 */
    unsigned short budget = 64; /* budget：本次服务允许处理的最大字节数，避免后台单轮无界阻塞。 */
    uint32_t now = SstDebug_GetTick10ms(); /* now：本轮读取的 10 ms 控制节拍，用于后台超时或逐拍服务。 */

    if(MaintSavePending && now != MaintSaveTick)
    {
        unsigned short offset = MaintSavePosition; /* offset：当前对象字偏移或保存镜像字节偏移。 */
        unsigned char byteValue; /* byteValue：本次持久化单字节写入值。 */
        MaintSaveTick = now;
        if(!SstParams_IsStopped())
        {
            MaintSavePending = 0;
            MaintSendError(MaintSaveSequence, MAINT_CMD_SAVE, MAINT_ERR_NOT_ALLOWED);
        }
        else if(offset > 92)
        {
            offset = (unsigned short)(offset - 93);
            if(r_eeprom_byte_read((unsigned char)(4 + offset/64), (unsigned char)(offset%64)) != MaintSaveImage[offset])
            {
                MaintSendError(MaintSaveSequence, MAINT_CMD_SAVE, MAINT_ERR_EEPROM);
                MaintSavePending = 0;
            }
            else if(offset == 91)
            {
                value = MAINT_ERR_OK;
                MaintSendFrame(MAINT_FLAG_RESPONSE, MaintSaveSequence, MAINT_CMD_SAVE, &value, 1);
                MaintSavePending = 0;
            }
            else ++MaintSavePosition;
        }
        else
        {
            byteValue = offset == 0 ? 0 : (offset == 92 ? MaintSaveImage[0] : MaintSaveImage[offset]);
            if(offset == 92) offset = 0;
            if(r_eeprom_byte_write((unsigned char)(4 + offset/64), (unsigned char)(offset%64), byteValue) != VALIDFLAG)
            {
                MaintSavePending = 0;
                MaintSendError(MaintSaveSequence, MAINT_CMD_SAVE, MAINT_ERR_EEPROM);
            }
            else ++MaintSavePosition;
        }
    }

    /* 主循环消费UART ISR放入的字节；ISR只收字节，不在中断中做CRC和命令执行。 */
    while(budget-- && MaintRxTail != MaintRxHead && SstSerial_TxFreeFrames() > 1)
    {
        value = MaintRxQueue[MaintRxTail];
        MaintRxTail = (unsigned short)((MaintRxTail + 1) % MAINT_RX_QUEUE_SIZE);

        if(MaintFrameState == 0)
        {
            /* 状态0：等待维护帧头A5。 */
            if(value == MAINT_SOF0)
            {
                MaintFrame[0] = value;
                MaintFrameIndex = 1;
                MaintFrameState = 1;
            }
            continue;
        }
        if(MaintFrameState == 1)
        {
            /* 状态1：确认第二个帧头字节5A，失败则丢弃候选帧。 */
            if(value != MAINT_SOF1)
            {
                MaintResetParser();
                continue;
            }
            MaintFrame[MaintFrameIndex++] = value;
            MaintFrameState = 2;
            continue;
        }
        MaintFrame[MaintFrameIndex++] = value;
        if(MaintFrameIndex == 8)
        {
            /* 收齐固定头后检查版本和长度，再计算整帧应有的字节数。 */
            if(MaintFrame[2] != MAINT_VERSION || MaintRead16(&MaintFrame[6]) > MAINT_MAX_PAYLOAD)
            {
                MaintResetParser();
                continue;
            }
            MaintFrameExpected = (unsigned short)(10 + MaintRead16(&MaintFrame[6]));
        }
        if(MaintFrameExpected != 0 && MaintFrameIndex >= MaintFrameExpected)
        {
            /* 完整帧到达后校验CRC；合法帧才进入命令分发。 */
            if(MaintCRC16(MaintFrame, (unsigned short)(MaintFrameExpected - 2)) ==
               MaintRead16(&MaintFrame[MaintFrameExpected - 2]))
                MaintHandleFrame();
            else
                MaintSendError(MaintFrame[4], MaintFrame[5], MAINT_ERR_BAD_CRC);
            MaintResetParser();
        }
        if(MaintFrameIndex >= MAINT_FRAME_SIZE)
            MaintResetParser();
    }
}

/*==================================================================
 * 函数功能: 读取页 4/5 维护镜像并在校验通过时恢复旧参数
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_LoadPersistent(void)
{
    unsigned char data[64]; /* data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定。 */
    unsigned char data5[64]; /* data5：EEPROM 第 5 页读取缓存。 */
    unsigned char combined[92]; /* combined：页 4/5 合并后的维护参数镜像。 */
    MAINT_FLOAT_WORDS value; /* value：本次协议/存储处理的值，类型和编码保持原接口约定。 */
    unsigned short pos; /* pos：持久化镜像参数打包/解包的位置。 */
    unsigned short i; /* i：本次数组、参数或模块遍历的索引。 */

    if(r_eeprom_section_read(4, 0, 64, data) == INVALIDFLAG)
        return;
    if(data[0] != MAINT_PERSIST_MAGIC0 || data[1] != MAINT_PERSIST_MAGIC1 ||
       data[2] != MAINT_VERSION || data[3] != PORT_Index)
        return;
    if(r_eeprom_section_read(5, 0, 28, data5) == INVALIDFLAG)
        return;

    for(i = 0; i < 64; i++) combined[i] = data[i];
    for(i = 0; i < 28; i++) combined[64 + i] = data5[i];

    pos = 4;
    for(i = 0; i < MAINT_PARAM_COUNT; i++)
    {
        value.word[0] = combined[pos] | ((unsigned short)combined[pos + 1] << 8);
        pos += 2;
        if(i >= 12)
        {
            value.word[1] = combined[pos] | ((unsigned short)combined[pos + 1] << 8);
            pos += 2;
        }
        if(MaintParameterWritable((unsigned char)i))
            MaintSetParameter((unsigned char)i, (unsigned char *)&value.word[0],
                              (unsigned short)(i < 12 ? 1 : 2));
    }
#ifdef HVAC_online
    MaintRecountHVAC();
#elif defined(HVDC_online)
    MaintRecountHVDC();
#endif
}
