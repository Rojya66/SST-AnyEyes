/*
 * C6748 adaptation of F28377S_Plat/04_App/03_PcDebug.
 * Original protocol/class/command addresses retained. C28x SCI transport and
 * product-specific controller pointers are replaced by SstSerial/SstParams.
 * CRC8: polynomial 0x07, init 0, xorout 0xff (reference Calcu_CRC8).
 */
#include <string.h>
#include "Comm2PcDebug.hpp"
#include "SstDebug.h"
#include "ChbBuildConfig.h"

enum E_PC_INSTRUCT {
 /* 参考 单参数读取请求命令码。 */ E_ReadFuncCode_Inst=0xF0,/* 参考 单参数读取响应命令码。 */ E_ReadFuncCode_Rply=0x70,
 /* 参考 单参数写入请求命令码。 */ E_WriteFuncCode_Inst=0xF1,/* 参考 单参数写入响应命令码。 */ E_WriteFuncCode_Rply=0x71,
 /* 参考 波形开始请求命令码。 */ E_ScopeRun_Inst=0xF2,/* 参考 波形开始响应命令码。 */ E_ScopeRun_Rply=0x72,
 /* 参考 波形停止请求命令码。 */ E_ScopeStop_Inst=0xF3,/* 参考 波形停止响应命令码。 */ E_ScopeStop_Rply=0x73,
 /* 参考 状态面板开始请求命令码。 */ E_StateRun_Inst=0xF4,/* 参考 状态面板开始响应命令码。 */ E_StateRun_Rply=0x74,
 /* 参考 状态面板停止请求命令码。 */ E_StateStop_Inst=0xF5,/* 参考 状态面板停止响应命令码。 */ E_StateStop_Rply=0x75,
 /* 参考 全部参数读取保留请求命令码。 */ E_ReadAllFuncCode_Inst=0xF6,/* 参考 全部参数读取保留响应命令码。 */ E_ReadAllFuncCode_Rply=0x76,
 /* 参考 全部参数写入保留请求命令码。 */ E_WriteAllFuncCode_Inst=0xF7,/* 参考 全部参数写入保留响应命令码。 */ E_WriteAllFuncCode_Rply=0x77,
 /* 参考 运行/停止请求命令码。 */ E_RunStopCmd_Inst=0xF8,/* 参考 运行/停止响应命令码。 */ E_RunStopCmd_Rply=0x78,
 /* 参考 故障查询保留请求命令码。 */ E_ReadErr_Inst=0xF9,/* 参考 故障查询保留响应命令码。 */ E_ReadErr_Rply=0x79,
 /* 参考 版本查询保留请求命令码。 */ E_ReadVersion_Inst=0xFA,/* 参考 版本查询保留响应命令码。 */ E_ReadVersion_Rply=0x7A,
 /* 参考 复位保留请求命令码。 */ E_ReqReBoot_Inst=0xFB,/* 参考 复位保留响应命令码。 */ E_ReqReBoot_Rply=0x7B,
 /* 参考 紧急停止请求命令码。 */ E_ReqEmrgStop_Inst=0xFC,/* 参考 紧急停止响应命令码。 */ E_ReqEmrgStop_Rply=0x7C,
 /* 参考 PC 心跳请求命令码。 */ E_PC_LIFE_Inst=0xFE,/* 参考 MCU 心跳响应命令码。 */ E_MCU_LIFE_Rply=0x7E,
 /* 参考 连接握手请求命令码。 */ E_PC_LINK_Inst=0xFF,/* 参考 连接握手响应命令码。 */ E_PC_LINK_Rply=0x7F
};
static C_COMM2PC sPc; /* sPc：后台独占的参考 PcDebug 协议对象。 */
/*==================================================================
 * 函数功能: 计算参考 PcDebug 的 CRC8：初值 0、多项式 07、结果取反
 * 输入参数: data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           count：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 帧尾 CRC8 字节
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
extern "C" uint8_t SstAnyEyes_Crc8(const uint8_t *data,uint16_t count)
{
 uint16_t i; /* i：本次数组、参数或模块遍历的索引。 */
    uint8_t crc=0,bit; /* crc：逐字节、逐位累加的 CRC 校验状态。 */
     /* bit：CRC 或软件 I²C 的当前位序号，范围 0..7。 */
 for(i=0;i<count;++i) {
  crc^=data[i];
  for(bit=0;bit<8;++bit) crc=(uint8_t)((crc&0x80)?(crc<<1)^0x07:crc<<1);
 }
 return (uint8_t)~crc;
}
/*==================================================================
 * 函数功能: 清空 PcDebug 工作标志及接收状态，初始化后台上报时基
 * 输入参数: 无
 * 输出参数: PcDebug 工作标志、接收计数及各个毫秒时基。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
void C_COMM2PC::Init()
{
 memset(&stWorkFlag,0,sizeof(stWorkFlag));
 rxCount=rxExpected=0;
 lastRxTick=lastServiceTick=lastScopeTick=lastPanelTick=lastLifeTick=SstDebug_GetTick1ms();
 stWorkFlag.stScope.ScopeRunSendInvertTickUp=1;
}
/*==================================================================
 * 函数功能: 收集单字节并检查帧头、长度、超时与 CRC，完整帧交给命令解析
 * 输入参数: byte：后台收到的单个 UART 字节
 * 输出参数: 接收帧状态；完整有效帧更新参考工作标志并执行命令。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
void C_COMM2PC::RxByte(uint8_t byte)
{
 u16 data[32],length,i; /* data：把收到的字节帧扩展为参考解析接口的 16 位字数组。 */
 /* length：校验通过帧的完整字节数。 */
 /* i：本次数组、参数或模块遍历的索引。 */
 uint32_t now=SstDebug_GetTick1ms(); /* now：本轮读取的单调毫秒节拍，用于接收/连接超时或周期上报。 */
 if(rxCount && (uint32_t)(now-lastRxTick)>100U) rxCount=rxExpected=0;
 lastRxTick=now;
 if(!rxCount) { if(byte==0x6B) rx[rxCount++]=byte; return; }
 if(rxCount==1) {
  if(byte<1 || byte>29) { rxCount=rxExpected=0; if(byte==0x6B) rx[rxCount++]=byte; return; }
  rxExpected=(u16)byte+3;
 }
 rx[rxCount++]=byte;
 if(rxCount!=rxExpected) return;
 length=rxExpected;
 rxCount=rxExpected=0;
 if(SstAnyEyes_Crc8(rx,length-1)!=rx[length-1]) return;
 for(i=0;i<length;++i) data[i]=rx[i];
 if(ParseInstruct(data,&length)==1) stWorkFlag.stPcLink.LostPcLifeTick=now;
 /* Replies immediately enter the shared queue; never wait for UART hardware. */
 ExecuteInstruct();
}
/*==================================================================
 * 函数功能: 按参考命令码校验载荷、设置参数读写/波形/面板标志并转接启停命令
 * 输入参数: DataList：参考 PcDebug 的接收字节数组，每字只使用低 8 位
 *           ValidLenth：输入完整帧的有效字节数指针，沿用参考字段拼写
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 有效命令返回 1，非法或未连接命令返回 -1
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
s16 C_COMM2PC::ParseInstruct(u16 *DataList,u16 *ValidLenth)
{
 u16 Instct=DataList[2],n=(u16)(*ValidLenth-4),i; /* Instct：接收帧第 2 字节的参考命令码。 */
 /* n：去掉帧头、长度、命令和 CRC 后的载荷字节数。 */
 /* i：本次数组、参数或模块遍历的索引。 */
 if(!stWorkFlag.stPcLink.PcLinkFlag && Instct!=E_PC_LINK_Inst &&
    Instct!=E_ReqReBoot_Inst) return -1;
 switch(Instct) {
 case E_ReadFuncCode_Inst:
  if(n!=2 || stWorkFlag.stReadFunc.ReadFuncCodeFlag) return -1;
  stWorkFlag.stReadFunc.ReadFuncCodeFlag=true;
  stWorkFlag.stReadFunc.ReadAdsH=DataList[3]; stWorkFlag.stReadFunc.ReadAdsL=DataList[4]; break;
 case E_WriteFuncCode_Inst:
  if(n!=4 || stWorkFlag.stWriteFunc.WriteFuncCodeFalg) return -1;
  stWorkFlag.stWriteFunc.WriteFuncCodeFalg=true;
  stWorkFlag.stWriteFunc.WriteAdsH=DataList[3]; stWorkFlag.stWriteFunc.WriteAdsL=DataList[4];
  stWorkFlag.stWriteFunc.WriteData=(u16)((DataList[5]<<8)|DataList[6]); break;
 case E_ScopeRun_Inst:
  if(n<2 || n>17) return -1;
  for(i=1;i<n;++i) if(DataList[i+3]>=SST_DISPLAY_COUNT) return -1;
  stWorkFlag.stScope.ScopeRunFalg=true;
  stWorkFlag.stScope.WorkChNum=n-1;
  stWorkFlag.stScope.ScopeRunSendInvertTickUp=DataList[3]?DataList[3]:1;
  { uint32_t minimum=((4U+2U*(n-1))*10000UL+SST_UART_BAUD-1)/SST_UART_BAUD; /* minimum：按通道数和 UART 波特率计算的最小波形发送间隔，单位 ms。 */
    if(stWorkFlag.stScope.ScopeRunSendInvertTickUp<minimum)
     stWorkFlag.stScope.ScopeRunSendInvertTickUp=(u16)minimum; }
  for(i=0;i<n-1;++i) stWorkFlag.stScope.ChannelSel[i]=DataList[i+4];
  break;
 case E_ScopeStop_Inst:
  if(n) return -1; stWorkFlag.stScope.ScopeRunFalg=false; break;
 case E_StateRun_Inst:
  if(n) return -1; stWorkFlag.stStatePanel.StatePanelRunFlag=true; break;
 case E_StateStop_Inst:
  if(n) return -1; stWorkFlag.stStatePanel.StatePanelRunFlag=false; break;
 case E_ReadAllFuncCode_Inst:
  if(n) return -1; stWorkFlag.ReadAllFuncCodeFlag=true; break;
 case E_WriteAllFuncCode_Inst:
  if(n) return -1; stWorkFlag.WriteAllFuncCodeFlag=true; break;
 case E_RunStopCmd_Inst:
  if(n!=1) return -1;
  if(DataList[3]==0x55) SstParams_RunCommand(1);
  else if(DataList[3]==0xAA) SstParams_RunCommand(0);
  else return -1;
  break;
 case E_ReadErr_Inst:
  if(n) return -1; stWorkFlag.QueryErrInfoFalg=true; break;
 case E_ReadVersion_Inst:
  if(n) return -1; stWorkFlag.QuerySoftInfoFalg=true; break;
 case E_ReqReBoot_Inst:
  /* Reference FB is retained/reserved. No unverified C6748 hardware-reset path. */
  if(n!=8) return -1;
  for(i=0;i<8;++i) if(DataList[3+i]!=(i&1?0xAA:0x55)) return -1;
  break;
 case E_ReqEmrgStop_Inst:
  if(n!=3 || DataList[3]!=0x55 || DataList[4]!=0xAA || DataList[5]!=0xCC) return -1;
  SstParams_EmergencyStop(); break;
 case E_PC_LINK_Inst:
  if(n!=4) return -1;
  stWorkFlag.stPcLink.NeedAckPcLinkResulFlag=true;
  stWorkFlag.stPcLink.PcLinkFlag=
   ((DataList[3]==255 && DataList[4]==43 && DataList[5]==1 && DataList[6]==0) ||
    (DataList[3]==255 && DataList[4]==255 && DataList[5]==255 && DataList[6]==255));
  break;
 case E_PC_LIFE_Inst: break; /* reference accepts PC life as a valid no-op */
 default: return -1;
 }
 return 1;
}
/*==================================================================
 * 函数功能: 按参考帧格式打包字节载荷及 CRC，并将整帧提交非阻塞发送队列
 * 输入参数: instruction：参考 AnyEyes 响应命令码，只使用低 8 位
 *           data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           count：参考线上载荷字节数，每个 16 位元素只承载低 8 位
 * 输出参数: 共享整帧 TX 队列；函数不等待 UART FIFO。
 * 返回值: true 表示入队成功，false 表示长度非法或队列满
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
bool C_COMM2PC::Pack(u16 instruction,const u16 *data,u16 count)
{
 uint8_t frame[40]; /* frame：完整发送帧的本地字节缓存，包含帧头及校验。 */
    u16 i; /* i：本次数组、参数或模块遍历的索引。 */
 if(count>36) return false;
 frame[0]=0x6B; frame[1]=(uint8_t)(count+1); frame[2]=(uint8_t)instruction;
 for(i=0;i<count;++i) frame[3+i]=(uint8_t)data[i];
 frame[3+count]=SstAnyEyes_Crc8(frame,(u16)(3+count));
 return SstSerial_SendFrame(frame,(u16)(count+4))!=0;
}
/*==================================================================
 * 函数功能: 执行待处理参数读写及应答，并按毫秒节拍发送波形、状态和心跳
 * 输入参数: 无
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 未连接返回 0，已连接服务返回 1
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
s16 C_COMM2PC::ExecuteInstruct()
{
 u16 data[32],i,word; /* data：参考应答、波形、面板及心跳的临时字节载荷。 */
 /* i：本次数组、参数或模块遍历的索引。 */
 /* word：读取参数或显示表得到的 16 位字。 */
    s16 result; /* result：FuncCode 参数访问结果，成功为 1、失败为负结果码。 */
    uint32_t now=SstDebug_GetTick1ms(); /* now：本轮读取的单调毫秒节拍，用于接收/连接超时或周期上报。 */
 if(stWorkFlag.stPcLink.NeedAckPcLinkResulFlag) {
  data[0]=stWorkFlag.stPcLink.PcLinkFlag?0x55:0xAA;
  data[1]=255;data[2]=43;data[3]=1;data[4]=0;data[5]=1;data[6]=0;
  if(Pack(E_PC_LINK_Rply,data,7)) stWorkFlag.stPcLink.NeedAckPcLinkResulFlag=false;
 }
 if(!stWorkFlag.stPcLink.PcLinkFlag) return 0;
 if(stWorkFlag.stReadFunc.ReadFuncCodeFlag && SstSerial_TxFreeFrames()) {
  word=0; result=FuncCode.GetOneValueFromRealMem(stWorkFlag.stReadFunc.ReadAdsH,stWorkFlag.stReadFunc.ReadAdsL,&word);
  data[0]=stWorkFlag.stReadFunc.ReadAdsH;data[1]=stWorkFlag.stReadFunc.ReadAdsL;
  data[2]=(word>>8)&255;data[3]=word&255;data[4]=(result==1)?0:1;
  if(Pack(E_ReadFuncCode_Rply,data,5)) stWorkFlag.stReadFunc.ReadFuncCodeFlag=false;
 }
 if(stWorkFlag.stWriteFunc.WriteFuncCodeFalg && SstSerial_TxFreeFrames()) {
  result=FuncCode.SetOneValue2BothMem(stWorkFlag.stWriteFunc.WriteAdsH,stWorkFlag.stWriteFunc.WriteAdsL,stWorkFlag.stWriteFunc.WriteData);
  data[0]=(result==1)?0:(u16)-result;
  if(Pack(E_WriteFuncCode_Rply,data,1)) stWorkFlag.stWriteFunc.WriteFuncCodeFalg=false;
 }
 /* A 1ms background timebase retains reference task-slice units. No repeated output per main-loop iteration. */
 if(now==lastServiceTick) return 1;
 lastServiceTick=now;
 SstDisplay_Refresh();
 if((uint32_t)(now-lastLifeTick)>=256U && SstSerial_TxFreeFrames()>1) {
  /* Reference emits only when the 1ms LifeTick low byte is 0xff. */
  data[0]=0xFF;
  data[1]=(SstDisplay_Read(8)||SstDisplay_Read(9))?0x55:0xAA;
  data[2]=SstDisplay_Read(2)?0xAA:0x55;
  if(Pack(E_MCU_LIFE_Rply,data,3)) lastLifeTick=now;
 }
 if(stWorkFlag.stScope.ScopeRunFalg &&
    (uint32_t)(now-lastScopeTick)>=stWorkFlag.stScope.ScopeRunSendInvertTickUp &&
    SstSerial_TxFreeFrames()>1) {
  for(i=0;i<stWorkFlag.stScope.WorkChNum;++i) {
   word=SstDisplay_Read(stWorkFlag.stScope.ChannelSel[i]);
   stWorkFlag.stScope.ChannelData[i]=word;
   data[2*i]=(word>>8)&255;data[2*i+1]=word&255;
  }
  if(Pack(E_ScopeRun_Rply,data,(u16)(2*stWorkFlag.stScope.WorkChNum))) lastScopeTick=now;
 }
 if(stWorkFlag.stStatePanel.StatePanelRunFlag && (uint32_t)(now-lastPanelTick)>=10U &&
    SstSerial_TxFreeFrames()>1) {
  data[0]=stWorkFlag.stStatePanel.StatePanelPos;
  word=SstDisplay_Read(data[0]); data[1]=(word>>8)&255;data[2]=word&255;
  if(Pack(E_StateRun_Rply,data,3)) {
   stWorkFlag.stStatePanel.StatePanelPos=(u16)((data[0]+1)%SST_DISPLAY_COUNT);
   lastPanelTick=now;
  }
 }
 return 1;
}
/*==================================================================
 * 函数功能: 检查 PC 心跳超时并推进参考 PcDebug 后台任务
 * 输入参数: 无
 * 输出参数: 连接及接收超时状态，并调用 ExecuteInstruct 推进响应/上报。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
void C_COMM2PC::MainFSM()
{
 uint32_t now=SstDebug_GetTick1ms(); /* now：本轮读取的单调毫秒节拍，用于接收/连接超时或周期上报。 */
 if(rxCount && (uint32_t)(now-lastRxTick)>100U) rxCount=rxExpected=0;
 if(stWorkFlag.stPcLink.PcLinkFlag && (uint32_t)(now-stWorkFlag.stPcLink.LostPcLifeTick)>30000U) {
  stWorkFlag.stPcLink.PcLinkFlag=false;
  stWorkFlag.stScope.ScopeRunFalg=false;stWorkFlag.stStatePanel.StatePanelRunFlag=false;
  stWorkFlag.stReadFunc.ReadFuncCodeFlag=false;stWorkFlag.stWriteFunc.WriteFuncCodeFalg=false;
 }
 ExecuteInstruct();
}
/*==================================================================
 * 函数功能: 初始化全局 PcDebug 对象
 * 输入参数: 无
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
extern "C" void SstAnyEyes_Init(void) { sPc.Init(); }
/*==================================================================
 * 函数功能: 将后台 UART 收到的一个字节交给 PcDebug 解析器
 * 输入参数: byte：后台收到的单个 UART 字节
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
extern "C" void SstAnyEyes_RxByte(uint8_t byte) { sPc.RxByte(byte); }
/*==================================================================
 * 函数功能: 在主循环执行 PcDebug 状态机及周期上报
 * 输入参数: 无
 * 输出参数: 协议工作标志、接收缓存及共享整帧发送队列；控制命令通过受控参数入口提交。
 * 返回值: 无
 * 执行说明: 主循环通信路径；AnyEyes/VOFA 由宏二选一，UART 收发有固定预算，不在 FPGA ISR 内打包或发送。
 *================================================================*/
extern "C" void SstAnyEyes_Service(void) { sPc.MainFSM(); }

