#ifndef SST_COMM2PCDEBUG_HPP
#define SST_COMM2PCDEBUG_HPP
#include <stdint.h>
#include "FuncCode.hpp"
/* C6748 port of F28377S_Plat C_COMM2PC. Wire octets and parameter words differ. */
typedef struct
{
    bool ReadFuncCodeFlag; /* ReadFuncCodeFlag：待执行单参数读取标志，入队响应后清零。 */
    u16 ReadAdsH,ReadAdsL,ReadData; /* ReadAdsH：读参数组号，对应参考地址高字节。 */
    /* ReadAdsL：读参数组内索引，对应参考地址低字节。 */
    /* ReadData：保留参考读参数结果字段。 */
} ST_READ_FUNC;
typedef struct
{
    bool WriteFuncCodeFalg; /* WriteFuncCodeFalg：待执行单参数写入标志，沿用参考字段拼写。 */
    u16 WriteAdsH,WriteAdsL,WriteData; /* WriteAdsH：写参数组号，对应参考地址高字节。 */
    /* WriteAdsL：写参数组内索引，对应参考地址低字节。 */
    /* WriteData：待写入的 16 位参数编码。 */
} ST_WRITE_FUNC;
typedef struct {
    bool ScopeRunFalg; /* ScopeRunFalg：参考波形连续上报使能，沿用参考字段拼写。 */
    u16 WorkChNum,ScopeRunSendInvertTick,ScopeRunSendInvertTickUp; /* WorkChNum：实际波形通道数，范围 1..16。 */
    /* ScopeRunSendInvertTick：保留参考波形发送间隔计数。 */
    /* ScopeRunSendInvertTickUp：波形发送间隔，单位 ms，并按 UART 带宽设下限。 */
    u16 ChannelSel[16],ChannelData[16]; /* ChannelSel：16 个波形通道的显示表索引，索引范围 0..199。 */
    /* ChannelData：参考波形通道值缓存。 */
} ST_SCOPE;
typedef struct
{
    bool StatePanelRunFlag; /* StatePanelRunFlag：状态面板连续上报使能。 */
    u16 StatePanelPos,StateValueSendInvertTick; /* StatePanelPos：当前状态面板发送位置。 */
    /* StateValueSendInvertTick：保留参考状态面板间隔计数。 */
} ST_STATE_PANEL;
typedef struct
{
    bool NeedAckPcLinkResulFlag,PcLinkFlag; /* NeedAckPcLinkResulFlag：连接握手结果等待发送的标志。 */
    /* PcLinkFlag：PC 连接握手成功标志。 */
    uint32_t LostPcLifeTick; /* LostPcLifeTick：最近有效 PC 指令的毫秒节拍，用于连接超时判断。 */
    u16 LifeTick; /* LifeTick：保留参考 MCU 心跳计数字段。 */
} ST_PC_LINK;
typedef struct {
    ST_READ_FUNC stReadFunc; /* stReadFunc：参考单参数读取工作状态。 */
    ST_WRITE_FUNC stWriteFunc; /* stWriteFunc：参考单参数写入工作状态。 */
    ST_SCOPE stScope; /* stScope：参考波形选择及发送工作状态。 */
    ST_STATE_PANEL stStatePanel; /* stStatePanel：参考状态面板工作状态。 */
    ST_PC_LINK stPcLink; /* stPcLink：参考握手及心跳工作状态。 */
    bool ReadAllFuncCodeFlag,WriteAllFuncCodeFlag,QueryErrInfoFalg,QuerySoftInfoFalg; /* ReadAllFuncCodeFlag：保留参考 F6 请求标志，参考实现未执行批量读取。 */
    /* WriteAllFuncCodeFlag：保留参考 F7 请求标志，参考实现未执行批量写入。 */
    /* QueryErrInfoFalg：保留参考 F9 查询标志，故障通过显示表/组 19 上报。 */
    /* QuerySoftInfoFalg：FA版本查询待应答标志，7A成功入队或会话重置后清除。 */
} ST_WORK_FLAG;
class C_COMM2PC {
    ST_WORK_FLAG stWorkFlag; /* stWorkFlag：PcDebug 参数、波形、状态和连接任务的聚合状态。 */
    uint8_t rx[32]; /* rx：32 字节 AnyEyes 接收帧缓存。 */
    u16 rxCount,rxExpected; /* rxCount：当前 AnyEyes 已收字节数。 */
    /* rxExpected：当前 AnyEyes 帧的预期总字节数。 */
    uint32_t lastRxTick,lastServiceTick,lastScopeTick,lastPanelTick,lastLifeTick; /* lastRxTick：最近接收字节时刻，单位 ms。 */
    /* lastServiceTick：最近周期服务时刻，单位 ms，用于抑制同拍重复上报。 */
    /* lastScopeTick：最近波形上报时刻，单位 ms。 */
    /* lastPanelTick：最近状态面板上报时刻，单位 ms。 */
    /* lastLifeTick：最近 MCU 心跳上报时刻，单位 ms。 */
    C_FUNC_CODE FuncCode; /* FuncCode：参考参数访问接口，目标绑定到 SST 固定参数表。 */
    /*==================================================================
     * 函数功能: 清除连接及待处理任务，避免重连恢复旧写请求或波形订阅
     * 输入参数: 无
     * 输出参数: PcDebug工作标志及周期发送时基复位，已入UART队列的帧保留
     * 返回值: 无
     * 执行说明: 初始化、新握手和失联时调用；不改变控制启停命令或接收帧缓存。
     *================================================================*/
    void ResetSession();
    /*==================================================================
     * 函数功能: 按参考命令码校验载荷、设置参数读写/波形/面板标志并转接启停命令
     * 输入参数: dataList：输入参数/帧数据缓存，按该接口的固定字格式解释
     *           validLength：参考接口的有效数据长度
     * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
     * 返回值: 有效命令返回 1，非法或未连接命令返回 -1
     * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
     *================================================================*/
    s16 ParseInstruct(u16 *dataList,u16 *validLength);
    /*==================================================================
     * 函数功能: 执行待处理参数读写及应答，并按毫秒节拍发送波形、状态和心跳
     * 输入参数: 无
     * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
     * 返回值: 未连接或握手应答待入队返回0，已连接正常服务返回1
     * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
     *================================================================*/
    s16 ExecuteInstruct();
    /*==================================================================
     * 函数功能: 按参考帧格式打包字节载荷及 CRC，并将整帧提交非阻塞发送队列
     * 输入参数: instruction：参考 AnyEyes 响应命令码，只使用低 8 位
     *           data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
     *           count：参考线上载荷字节数，每个 16 位元素只承载低 8 位
     * 输出参数: 共享整帧 TX 队列；函数不等待 UART FIFO。
     * 返回值: true 表示入队成功，false 表示长度非法或队列满
     * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
     *================================================================*/
    bool Pack(u16 instruction,const u16 *data,u16 count);
public:
    /*==================================================================
     * 函数功能: 清空 PcDebug 工作标志及接收状态，初始化后台上报时基
     * 输入参数: 无
     * 输出参数: PcDebug 工作标志、接收计数及各个毫秒时基。
     * 返回值: 无
     * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
     *================================================================*/
    void Init();
    /*==================================================================
     * 函数功能: 收集单字节并检查帧头、长度、超时与 CRC，完整帧交给命令解析
     * 输入参数: byte：后台收到的单个 UART 字节
     * 输出参数: 接收帧状态；完整有效帧更新参考工作标志并执行命令。
     * 返回值: 无
     * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
     *================================================================*/
    void RxByte(uint8_t byte);
    /*==================================================================
     * 函数功能: 检查 PC 心跳超时并推进参考 PcDebug 后台任务
     * 输入参数: 无
     * 输出参数: 连接及接收超时状态，并调用 ExecuteInstruct 推进响应/上报。
     * 返回值: 无
     * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
     *================================================================*/
    void MainFSM();
};
#ifdef __cplusplus
extern "C" {
#endif
/*==================================================================
 * 函数功能: 计算参考 PcDebug 的 CRC8：初值 0、多项式 07、结果取反
 * 输入参数: data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           count：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 帧尾 CRC8 字节
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
uint8_t SstAnyEyes_Crc8(const uint8_t *data,uint16_t count);
#ifdef __cplusplus
}
#endif
#endif

