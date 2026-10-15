/*======================================================================================*
 * 文件名称 : Comm2PcDebug.cpp
 * 功能说明 : AnyEyes 上位机调试协议的接收、握手、参数访问及周期上报。
 *            沿用 F28377S_Plat 的命令码和参数地址，通过 SstSerial 收发完整帧，
 *            通过 SST 参数表、显示表和版本模块访问本机数据。
 * 调用关系 : 由主循环调试服务调用；不在 FPGA 10 kHz 中断内解析或发送协议帧。
 *=====================================================================================*/

/*======================================== 文件包含 =====================================*/
#include <string.h>
#include "Comm2PcDebug.hpp"
#include "SstDebug.h"
#include "ChbBuildConfig.h"
#include "SoftVersion.h"

/*======================================== 私有类型 =====================================*/
/* AnyEyes 请求/响应命令码；保留项仅保持协议编号，不表示已实现对应操作。 */
enum E_PC_INSTRUCT
{
    E_ReadFuncCode_Inst = 0xF0, /* 读取单个参数。 */
    E_ReadFuncCode_Rply = 0x70, /* 单参数读取响应。 */

    E_WriteFuncCode_Inst = 0xF1, /* 写入单个参数。 */
    E_WriteFuncCode_Rply = 0x71, /* 单参数写入响应。 */

    E_ScopeRun_Inst = 0xF2, /* 设置波形通道并开始上报。 */
    E_ScopeRun_Rply = 0x72, /* 波形数据帧。 */

    E_ScopeStop_Inst = 0xF3, /* 停止波形上报。 */
    E_ScopeStop_Rply = 0x73, /* 波形停止响应编号，当前未发送。 */

    E_StateRun_Inst = 0xF4, /* 开始轮询上报状态面板。 */
    E_StateRun_Rply = 0x74, /* 单个状态面板数据帧。 */

    E_StateStop_Inst = 0xF5, /* 停止状态面板上报。 */
    E_StateStop_Rply = 0x75, /* 状态面板停止响应编号，当前未发送。 */

    E_ReadAllFuncCode_Inst = 0xF6, /* 批量读取保留请求，仅置标志。 */
    E_ReadAllFuncCode_Rply = 0x76, /* 批量读取保留响应编号。 */

    E_WriteAllFuncCode_Inst = 0xF7, /* 批量写入保留请求，仅置标志。 */
    E_WriteAllFuncCode_Rply = 0x77, /* 批量写入保留响应编号。 */

    E_RunStopCmd_Inst = 0xF8, /* 运行/停止命令。 */
    E_RunStopCmd_Rply = 0x78, /* 运行/停止响应编号，当前未发送。 */

    E_ReadErr_Inst = 0xF9, /* 故障查询保留请求，仅置标志。 */
    E_ReadErr_Rply = 0x79, /* 故障查询保留响应编号。 */

    E_ReadVersion_Inst = 0xFA, /* 查询六段软件版本。 */
    E_ReadVersion_Rply = 0x7A, /* 六段软件版本响应。 */

    E_ReqReBoot_Inst = 0xFB, /* 复位保留请求，仅校验解锁字节。 */
    E_ReqReBoot_Rply = 0x7B, /* 复位保留响应编号。 */

    E_ReqEmrgStop_Inst = 0xFC, /* 校验解锁字节后提交紧急停止。 */
    E_ReqEmrgStop_Rply = 0x7C, /* 紧急停止响应编号，当前未发送。 */

    E_PC_LIFE_Inst = 0xFE, /* PC 心跳，接受后刷新连接存活时间。 */
    E_MCU_LIFE_Rply = 0x7E, /* MCU 心跳及运行/故障摘要。 */

    E_PC_LINK_Inst = 0xFF, /* 按四项电子标签请求连接。 */
    E_PC_LINK_Rply = 0x7F /* 握手结果及六段软件版本。 */
};

/*======================================== 私有变量 =====================================*/
static C_COMM2PC sPc; /* 主循环使用的唯一 AnyEyes 会话对象。 */

/*======================================== CRC 校验 =====================================*/
/*==================================================================
 * 函数功能: 计算 AnyEyes 帧使用的 CRC8，与参考 Calcu_CRC8 保持一致。
 * 输入参数: data：待校验字节数组；count：有效字节数，不包含帧尾 CRC。
 * 返回值  : 8 位 CRC 校验值。
 * 算法说明: 初值 0，按每字节最高位先行处理；多项式为 0x07
 *           （x^8+x^2+x+1），输入/输出均不反射，最终按位取反（xorout=0xFF）。
 *================================================================*/
extern "C" uint8_t SstAnyEyes_Crc8(const uint8_t *data, uint16_t count)
{
    uint16_t i; /* 当前参与校验的字节索引。 */
    uint8_t crc = 0, /* 每处理一位后截取低 8 位的校验状态。 */
        bit; /* 当前字节内的位循环次数，范围 0..7。 */

    for (i = 0; i < count; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (uint8_t)((crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1);
    }

    return (uint8_t)~crc;
}

/*======================================== 会话初始化 ===================================*/
/*==================================================================
 * 函数功能: 初始化连接、接收组帧状态及各项上报的毫秒时基。
 * 输入参数: 无。
 * 返回值  : 无。
 *================================================================*/
void C_COMM2PC::Init()
{
    ResetSession();
    rxCount = rxExpected = 0;
    lastRxTick = SstDebug_GetTick1ms();
}

/*==================================================================
 * 函数功能: 清除握手、待处理参数请求及波形/面板订阅，复位周期发送时基。
 * 输入参数: 无。
 * 返回值  : 无。
 * 调用说明: 初始化、新握手和连接超时使用；不清空接收缓存或 UART 已排队帧，
 *           也不改写控制器的运行命令。
 *================================================================*/
void C_COMM2PC::ResetSession()
{
    memset(&stWorkFlag, 0, sizeof(stWorkFlag));
    lastServiceTick = lastScopeTick = lastPanelTick = lastLifeTick = SstDebug_GetTick1ms();
    stWorkFlag.stScope.ScopeRunSendInvertTickUp = 1;
}

/*======================================== 接收与命令解析 ===============================*/
/*==================================================================
 * 函数功能: 接收一个 UART 字节，完成帧头、长度、字节间超时及 CRC 校验。
 * 输入参数: byte：主循环接收服务取出的一个字节。
 * 返回值  : 无。
 * 帧格式  : 0x6B、长度、命令、载荷、CRC；长度包含命令及载荷，不含其余三项。
 * 调用说明: 合法完整帧转为每元素低 8 位有效的 u16 数组后解析；有效指令刷新
 *           连接存活时间，应答只入共享发送队列，不等待 UART 硬件发送完成。
 *================================================================*/
void C_COMM2PC::RxByte(uint8_t byte)
{
    u16 data[32], /* 适配参考解析接口的逐字节 u16 接收帧。 */
        length, /* 当前完整接收帧的总字节数，包含 CRC。 */
        i; /* 接收帧字节复制索引。 */
    uint32_t now = SstDebug_GetTick1ms(); /* 当前毫秒节拍，用于字节间超时及存活时间。 */

    if (rxCount && (uint32_t)(now - lastRxTick) > 100U)
        rxCount = rxExpected = 0;
    lastRxTick = now;

    if (!rxCount)
    {
        if (byte == 0x6B)
            rx[rxCount++] = byte;
        return;
    }

    if (rxCount == 1)
    {
        if (byte < 1 || byte > 29)
        {
            rxCount = rxExpected = 0;
            if (byte == 0x6B)
                rx[rxCount++] = byte;
            return;
        }
        rxExpected = (u16)byte + 3;
    }

    rx[rxCount++] = byte;
    if (rxCount != rxExpected)
        return;

    length = rxExpected;
    rxCount = rxExpected = 0;
    if (SstAnyEyes_Crc8(rx, length - 1) != rx[length - 1])
        return;

    for (i = 0; i < length; ++i)
        data[i] = rx[i];
    if (ParseInstruct(data, &length) == 1)
        stWorkFlag.stPcLink.LostPcLifeTick = now;

    ExecuteInstruct();
}

/*==================================================================
 * 函数功能: 检查命令载荷，设置待处理请求或提交运行/停止、紧急停止命令。
 * 输入参数: DataList：完整帧，每个 u16 元素仅低 8 位有效。
 *           ValidLenth：完整帧字节数指针，沿用参考接口的参数拼写。
 * 返回值  : 1：已接受指令；-1：未连接、载荷非法、请求忙或命令不支持。
 * 调用说明: 握手前仅接受连接请求及保留复位请求；F6/F7/F9 仅置保留标志，
 *           FB 仅校验固定字节，不执行 C6748 硬件复位。
 *================================================================*/
s16 C_COMM2PC::ParseInstruct(u16 *DataList, u16 *ValidLenth)
{
    u16 Instct = DataList[2], /* 完整帧偏移 2 处的命令字节。 */
        n = (u16)(*ValidLenth - 4), /* 扣除帧头、长度、命令及 CRC 后的载荷字节数。 */
        i; /* 通道选择或复位解锁字节的校验索引。 */

    if (!stWorkFlag.stPcLink.PcLinkFlag && Instct != E_PC_LINK_Inst && Instct != E_ReqReBoot_Inst)
        return -1;

    switch (Instct)
    {
        case E_ReadFuncCode_Inst:
            if (n != 2 || stWorkFlag.stReadFunc.ReadFuncCodeFlag)
                return -1;
            stWorkFlag.stReadFunc.ReadFuncCodeFlag = true;
            stWorkFlag.stReadFunc.ReadAdsH = DataList[3];
            stWorkFlag.stReadFunc.ReadAdsL = DataList[4];
            break;

        case E_WriteFuncCode_Inst:
            if (n != 4 || stWorkFlag.stWriteFunc.WriteFuncCodeFalg)
                return -1;
            stWorkFlag.stWriteFunc.WriteFuncCodeFalg = true;
            stWorkFlag.stWriteFunc.WriteAdsH = DataList[3];
            stWorkFlag.stWriteFunc.WriteAdsL = DataList[4];
            stWorkFlag.stWriteFunc.WriteData = (u16)((DataList[5] << 8) | DataList[6]);
            break;

        case E_ScopeRun_Inst:
            if (n < 2 || n > 17)
                return -1;
            for (i = 1; i < n; ++i)
                if (DataList[i + 3] >= SST_DISPLAY_COUNT)
                    return -1;

            stWorkFlag.stScope.ScopeRunFalg = true;
            stWorkFlag.stScope.WorkChNum = n - 1;
            stWorkFlag.stScope.ScopeRunSendInvertTickUp = DataList[3] ? DataList[3] : 1;
            {
                /* 每帧 4+2×通道数个字节，按每字节 10 位计算发送时间并向上取整到 ms。 */
                uint32_t minimum = ((4U + 2U * (n - 1)) * 10000UL + SST_UART_BAUD - 1) / SST_UART_BAUD;
                if (stWorkFlag.stScope.ScopeRunSendInvertTickUp < minimum)
                    stWorkFlag.stScope.ScopeRunSendInvertTickUp = (u16)minimum;
            }
            for (i = 0; i < n - 1; ++i)
                stWorkFlag.stScope.ChannelSel[i] = DataList[i + 4];
            break;

        case E_ScopeStop_Inst:
            if (n)
                return -1;
            stWorkFlag.stScope.ScopeRunFalg = false;
            break;

        case E_StateRun_Inst:
            if (n)
                return -1;
            stWorkFlag.stStatePanel.StatePanelRunFlag = true;
            break;

        case E_StateStop_Inst:
            if (n)
                return -1;
            stWorkFlag.stStatePanel.StatePanelRunFlag = false;
            break;

        case E_ReadAllFuncCode_Inst:
            if (n)
                return -1;
            stWorkFlag.ReadAllFuncCodeFlag = true;
            break;

        case E_WriteAllFuncCode_Inst:
            if (n)
                return -1;
            stWorkFlag.WriteAllFuncCodeFlag = true;
            break;

        case E_RunStopCmd_Inst:
            if (n != 1)
                return -1;
            if (DataList[3] == 0x55)
                SstParams_RunCommand(1);
            else if (DataList[3] == 0xAA)
                SstParams_RunCommand(0);
            else
                return -1;
            break;

        case E_ReadErr_Inst:
            if (n)
                return -1;
            stWorkFlag.QueryErrInfoFalg = true;
            break;

        case E_ReadVersion_Inst:
            if (n)
                return -1;
            stWorkFlag.QuerySoftInfoFalg = true;
            break;

        case E_ReqReBoot_Inst:
            /* 保留参考 FB 请求：只检查 55/AA 交替序列，不调用硬件复位。 */
            if (n != 8)
                return -1;
            for (i = 0; i < 8; ++i)
                if (DataList[3 + i] != (i & 1 ? 0xAA : 0x55))
                    return -1;
            break;

        case E_ReqEmrgStop_Inst:
            if (n != 3 || DataList[3] != 0x55 || DataList[4] != 0xAA || DataList[5] != 0xCC)
                return -1;
            SstParams_EmergencyStop();
            break;

        case E_PC_LINK_Inst:
            if (n != SST_TAG_FIELD_COUNT)
                return -1;
            ResetSession();
            stWorkFlag.stPcLink.NeedAckPcLinkResulFlag = true;
            stWorkFlag.stPcLink.PcLinkFlag = (SstVersion_MatchElectronicTag(&DataList[3]) != 0U);
            break;

        case E_PC_LIFE_Inst:
            break; /* 心跳无需额外动作，由 RxByte 刷新存活时间。 */

        default:
            return -1;
    }

    return 1;
}

/*======================================== 发送与周期上报 ===============================*/
/*==================================================================
 * 函数功能: 将命令、载荷及 CRC 打包为 AnyEyes 完整帧，提交非阻塞发送队列。
 * 输入参数: instruction：响应命令码，低 8 位有效。
 *           data：载荷数组，每个 u16 元素仅低 8 位发送；count：载荷字节数。
 * 返回值  : true：完整帧已入队；false：载荷超过 36 字节或发送队列拒绝入队。
 * 调用说明: 总帧长为 count+4 字节；本函数不等待 UART FIFO。
 *================================================================*/
bool C_COMM2PC::Pack(u16 instruction, const u16 *data, u16 count)
{
    uint8_t frame[40]; /* 帧头、长度、命令、最多 36 字节载荷及 CRC。 */
    u16 i; /* u16 载荷到线上字节的复制索引。 */

    if (count > 36)
        return false;

    frame[0] = 0x6B;
    frame[1] = (uint8_t)(count + 1);
    frame[2] = (uint8_t)instruction;
    for (i = 0; i < count; ++i)
        frame[3 + i] = (uint8_t)data[i];
    frame[3 + count] = SstAnyEyes_Crc8(frame, (u16)(3 + count));

    return SstSerial_SendFrame(frame, (u16)(count + 4)) != 0;
}

/*==================================================================
 * 函数功能: 执行待处理握手、版本及单参数请求，按毫秒时基上报心跳、波形和面板。
 * 输入参数: 无。
 * 返回值  : 0：握手响应尚未入队或未连接；1：已连接，完成本轮服务。
 * 调用说明: 请求成功入队后才清除待处理标志；周期上报要求发送队列至少留出
 *           一个空帧位置。周期部分在同一毫秒内最多执行一次。
 *================================================================*/
s16 C_COMM2PC::ExecuteInstruct()
{
    u16 data[32], /* 握手、参数应答或最多 16 路波形的逐字节载荷。 */
        i, /* 波形通道遍历索引。 */
        word; /* 参数或显示表读出的 16 位编码值。 */
    s16 result; /* 参数访问结果：1 为成功，负数为错误码。 */
    uint32_t now = SstDebug_GetTick1ms(); /* 本轮服务的毫秒节拍快照。 */

    /* 握手应答优先：首字节为结果，随后返回六段软件版本。 */
    if (stWorkFlag.stPcLink.NeedAckPcLinkResulFlag)
    {
        data[0] = stWorkFlag.stPcLink.PcLinkFlag ? 0x55 : 0xAA;
        SstVersion_CopyBytes(&data[1]);
        if (!SstSerial_TxFreeFrames() || !Pack(E_PC_LINK_Rply, data, (u16)(SST_VERSION_FIELD_COUNT + 1U)))
            return 0;
        stWorkFlag.stPcLink.NeedAckPcLinkResulFlag = false;
    }

    if (!stWorkFlag.stPcLink.PcLinkFlag)
        return 0;

    if (stWorkFlag.QuerySoftInfoFalg && SstSerial_TxFreeFrames())
    {
        SstVersion_CopyBytes(data);
        if (Pack(E_ReadVersion_Rply, data, SST_VERSION_FIELD_COUNT))
            stWorkFlag.QuerySoftInfoFalg = false;
    }

    /* 单参数读取响应：组号、组内索引、数据高/低字节、结果字节。 */
    if (stWorkFlag.stReadFunc.ReadFuncCodeFlag && SstSerial_TxFreeFrames())
    {
        word = 0;
        result = FuncCode.GetOneValueFromRealMem(stWorkFlag.stReadFunc.ReadAdsH, stWorkFlag.stReadFunc.ReadAdsL, &word);
        data[0] = stWorkFlag.stReadFunc.ReadAdsH;
        data[1] = stWorkFlag.stReadFunc.ReadAdsL;
        data[2] = (word >> 8) & 255;
        data[3] = word & 255;
        data[4] = (result == 1) ? 0 : 1;
        if (Pack(E_ReadFuncCode_Rply, data, 5))
            stWorkFlag.stReadFunc.ReadFuncCodeFlag = false;
    }

    /* 单参数写入仍经 FuncCode 入口检查权限和范围，不直接改写控制变量。 */
    if (stWorkFlag.stWriteFunc.WriteFuncCodeFalg && SstSerial_TxFreeFrames())
    {
        result = FuncCode.SetOneValue2BothMem(
            stWorkFlag.stWriteFunc.WriteAdsH, stWorkFlag.stWriteFunc.WriteAdsL, stWorkFlag.stWriteFunc.WriteData);
        data[0] = (result == 1) ? 0 : (u16)-result;
        if (Pack(E_WriteFuncCode_Rply, data, 1))
            stWorkFlag.stWriteFunc.WriteFuncCodeFalg = false;
    }

    /* 主循环可重复调用，但显示表刷新及周期发送每个毫秒节拍只执行一次。 */
    if (now == lastServiceTick)
        return 1;
    lastServiceTick = now;
    SstDisplay_Refresh();

    if ((uint32_t)(now - lastLifeTick) >= 256U && SstSerial_TxFreeFrames() > 1)
    {
        /* 心跳编码：FF、AC/DAB PWM 使能摘要、故障摘要；发送周期至少 256 ms。 */
        data[0] = 0xFF;
        data[1] = (SstDisplay_Read(8) || SstDisplay_Read(9)) ? 0x55 : 0xAA;
        data[2] = SstDisplay_Read(2) ? 0xAA : 0x55;
        if (Pack(E_MCU_LIFE_Rply, data, 3))
            lastLifeTick = now;
    }

    if (stWorkFlag.stScope.ScopeRunFalg &&
        (uint32_t)(now - lastScopeTick) >= stWorkFlag.stScope.ScopeRunSendInvertTickUp && SstSerial_TxFreeFrames() > 1)
    {
        for (i = 0; i < stWorkFlag.stScope.WorkChNum; ++i)
        {
            word = SstDisplay_Read(stWorkFlag.stScope.ChannelSel[i]);
            stWorkFlag.stScope.ChannelData[i] = word;
            data[2 * i] = (word >> 8) & 255;
            data[2 * i + 1] = word & 255;
        }
        if (Pack(E_ScopeRun_Rply, data, (u16)(2 * stWorkFlag.stScope.WorkChNum)))
            lastScopeTick = now;
    }

    /* 状态面板每次发送一个显示表项，成功入队后递增索引并循环。 */
    if (stWorkFlag.stStatePanel.StatePanelRunFlag && (uint32_t)(now - lastPanelTick) >= 10U &&
        SstSerial_TxFreeFrames() > 1)
    {
        data[0] = stWorkFlag.stStatePanel.StatePanelPos;
        word = SstDisplay_Read(data[0]);
        data[1] = (word >> 8) & 255;
        data[2] = word & 255;
        if (Pack(E_StateRun_Rply, data, 3))
        {
            stWorkFlag.stStatePanel.StatePanelPos = (u16)((data[0] + 1) % SST_DISPLAY_COUNT);
            lastPanelTick = now;
        }
    }

    return 1;
}

/*==================================================================
 * 函数功能: 处理 100 ms 字节间超时及 30 s 连接超时，推进响应和周期上报。
 * 输入参数: 无。
 * 返回值  : 无。
 * 调用说明: 主循环调用；连接超时只重置通信会话，不自动下发控制启停命令。
 *================================================================*/
void C_COMM2PC::MainFSM()
{
    uint32_t now = SstDebug_GetTick1ms(); /* 本轮接收及连接超时判断使用的毫秒节拍。 */

    if (rxCount && (uint32_t)(now - lastRxTick) > 100U)
        rxCount = rxExpected = 0;
    if (stWorkFlag.stPcLink.PcLinkFlag && (uint32_t)(now - stWorkFlag.stPcLink.LostPcLifeTick) > 30000U)
    {
        ResetSession();
    }

    ExecuteInstruct();
}

/*======================================== C 语言调用接口 ===============================*/
/*==================================================================
 * 函数功能: 初始化本文件唯一的 AnyEyes 会话对象。
 * 输入参数: 无。
 * 返回值  : 无。
 *================================================================*/
extern "C" void SstAnyEyes_Init(void)
{
    sPc.Init();
}

/*==================================================================
 * 函数功能: 将主循环 UART 接收服务取出的单个字节交给 AnyEyes 组帧器。
 * 输入参数: byte：UART 接收字节。
 * 返回值  : 无。
 *================================================================*/
extern "C" void SstAnyEyes_RxByte(uint8_t byte)
{
    sPc.RxByte(byte);
}

/*==================================================================
 * 函数功能: 在主循环推进 AnyEyes 会话超时检查、请求应答及周期上报。
 * 输入参数: 无。
 * 返回值  : 无。
 *================================================================*/
extern "C" void SstAnyEyes_Service(void)
{
    sPc.MainFSM();
}
