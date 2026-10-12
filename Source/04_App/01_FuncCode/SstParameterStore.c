//###########################################################################
// FILE: SstParameterStore.c
// FUNCTION: EEPROM页6/7的92字节镜像及分步保存/整体校验加载事务。
//           参数表归SstParameterTable；停机判断归SstCommand。
//###########################################################################
#include <string.h>
#include "SstParameterStore.h"
#include "SstParameterTable.h"
#include "SstCommand.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "EEProm.H"
#if SST_ENABLE_MAINTENANCE
#include "MaintenanceProtocol.H"
#endif
static uint16_t storageResult,storagePending,storagePosition; /* storageResult：最近一次参数保存/加载的 SST_PARAM_* 结果。 */
/* storagePending：异步参数保存正在执行的标志。 */
/* storagePosition：参数镜像写入/提交/读回校验所处的步序。 */
static uint32_t storageTick; /* storageTick：上次推进参数存储任务的 10 ms 节拍。 */
/* SST_STORE_LENGTH：页 6/7 参数镜像总长度，92 字节。 */
#define SST_STORE_LENGTH 92U
static uint8_t storageImage[SST_STORE_LENGTH]; /* storageImage：页 6/7 的 92 字节参数镜像，包含魔数、版本及 CRC。 */
/*==================================================================
 * 函数功能: 查询 AnyEyes 与可选维护协议共享的存储忙状态
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 1 表示存储正在进行，0 表示空闲
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParameterStore_IsBusy(void)
{
#if SST_ENABLE_MAINTENANCE
 return (uint16_t)(storagePending || MaintenanceProtocol_IsSaving());
#else
 return storagePending;
#endif
}

/*==================================================================
 * 函数功能: 读取本参数镜像事务正在保存的标志
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 1表示本事务进行中；不包括维护协议事务
 * 执行说明: 主循环调用；不在 FPGA 10 kHz ISR 内运行。
 *================================================================*/
uint16_t SstParameterStore_IsPending(void)
{
 return storagePending;
}

/*==================================================================
 * 函数功能: 计算参数存储镜像的 CRC16，初值 FFFF、多项式 A001
 * 输入参数: data：本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定
 *           count：有效字节数；维护对象按其接口约定使用 16 位字数
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 16 位 CRC 校验值
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreCrc(const uint8_t *data,uint16_t count)
{
 uint16_t crc=0xFFFF,i,b; /* crc：逐字节、逐位累加的 CRC 校验状态。 */
 /* i：本次字节/位循环序号。 */
 /* b：CRC 位循环序号，或三量比较的第二个输入。 */
 for(i=0;i<count;++i) { crc^=data[i];for(b=0;b<8;++b) crc=(crc&1)?(crc>>1)^0xA001:crc>>1; }
 return crc;
}

/*==================================================================
 * 函数功能: 按小端顺序把一个 16 位字写入 storageImage
 * 输入参数: offset：对象中的字偏移或镜像字节偏移，按接口说明解释
 *           value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: storageImage 中 offset、offset+1 两字节。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static void StorePut(uint16_t offset,uint16_t value)
{
 storageImage[offset]=(uint8_t)value;storageImage[offset+1]=(uint8_t)(value>>8);
}

/*==================================================================
 * 函数功能: 按小端顺序从 storageImage 读取一个 16 位字
 * 输入参数: offset：对象中的字偏移或镜像字节偏移，按接口说明解释
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 镜像中的 16 位字
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreGet(uint16_t offset)
{
 return (uint16_t)storageImage[offset]|((uint16_t)storageImage[offset+1]<<8);
}

/*==================================================================
 * 函数功能: 停机时读取页 6/7 的 92 字节镜像，整体校验后恢复可持久化参数
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
static uint16_t StoreLoad(void)
{
 uint16_t i,result; /* i：镜像读取及快照解码的索引。 */
 /* result：整体42字快照校验结果。 */
 uint16_t snapshot[42]; /* snapshot：页6/7镜像解码后的37参数和5命令给定。 */
 if(!SstCommand_IsStopped()) return SST_PARAM_BUSY;
 for(i=0;i<SST_STORE_LENGTH;++i)
  storageImage[i]=r_eeprom_byte_read((uint8_t)(6+i/64),(uint8_t)(i%64));
 if(storageImage[0]!=0x53 || storageImage[1]!=0x54 || storageImage[2]!=1 ||
    storageImage[3]!=SST_STORE_LENGTH || StoreGet(90)!=StoreCrc(storageImage,90))
  return SST_PARAM_STORAGE;
 /* 先解码并整体校验42字；全部通过后才恢复任一活动成员。 */
 for(i=0;i<42;++i) snapshot[i]=StoreGet((uint16_t)(6+2*i));
 result=SstParameterTable_SnapshotValidate(snapshot);
 if(result!=SST_PARAM_OK) return result;
 SstParameterTable_SnapshotApply(snapshot);
 return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 在停机且存储空闲时执行加载，或建立异步保存镜像
 * 输入参数: load：0 请求保存，非零请求加载
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: SST_PARAM_* 操作结果
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParameterStore_Request(uint16_t load)
{
 uint16_t i; /* i：本次数组、参数或模块遍历的索引。 */
 if(SstParameterStore_IsBusy() || !SstCommand_IsStopped()) return SST_PARAM_BUSY;
 if(load) { storageResult=StoreLoad();return storageResult; }
 memset(storageImage,0,sizeof(storageImage));
 storageImage[0]=0x53;storageImage[1]=0x54;storageImage[2]=1;storageImage[3]=SST_STORE_LENGTH;
 for(i=0;i<42;++i) StorePut((uint16_t)(6+2*i),SstParameterTable_SnapshotRead(i));
 StorePut(90,StoreCrc(storageImage,90));
 storagePending=1;storagePosition=0;storageResult=SST_PARAM_BUSY;
 storageTick=(uint32_t)g_chb_dab_status.tick_10ms-1;
 return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 每 10 ms 至多推进一次单字节写入或读回校验，最后提交镜像有效标记
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParameterStore_Service(void)
{
 uint32_t now=(uint32_t)g_chb_dab_status.tick_10ms; /* now：本轮读取的 10 ms 控制节拍，用于后台超时或逐拍服务。 */uint16_t offset; /* offset：本拍写入或读回的镜像字节偏移。 */uint8_t value,ok; /* value：本拍写入 EEPROM 的单字节值。 */
     /* ok：EEPROM 单字节写入的 VALIDFLAG/INVALIDFLAG 结果。 */
 if(!storagePending || now==storageTick) return;
 storageTick=now;
 if(!SstCommand_IsStopped()) { storagePending=0;storageResult=SST_PARAM_BUSY;return; }
 /* Invalidate magic first, commit it last. At most one byte per 10ms tick;
    no global interrupt mask and no EEPROM write-cycle wait in this service. */
 if(storagePosition==0) { offset=0;value=0; }
 else if(storagePosition<SST_STORE_LENGTH) { offset=storagePosition;value=storageImage[offset]; }
 else if(storagePosition==SST_STORE_LENGTH) { offset=0;value=storageImage[0]; }
 else {
  offset=(uint16_t)(storagePosition-SST_STORE_LENGTH-1);
  if(r_eeprom_byte_read((uint8_t)(6+offset/64),(uint8_t)(offset%64))!=storageImage[offset]) {
   storagePending=0;storageResult=SST_PARAM_STORAGE;return;
  }
  ++storagePosition;
  if(offset==SST_STORE_LENGTH-1) { storagePending=0;storageResult=SST_PARAM_OK; }
  return;
 }
 ok=r_eeprom_byte_write((uint8_t)(6+offset/64),(uint8_t)(offset%64),value);
 if(ok!=VALIDFLAG) { storagePending=0;storageResult=SST_PARAM_STORAGE;return; }
 ++storagePosition;
}

/*==================================================================
 * 函数功能: 读取最近一次参数保存/加载的结果
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: SST_PARAM_* 结果码
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
uint16_t SstParameterStore_GetResult(void) { return storageResult; }

/*==================================================================
 * 函数功能: 在初始化阶段尝试恢复参数并记录存储结果
 * 输入参数: 无
 * 输出参数: 读接口写入输出指针；写/存储接口更新受控参数、命令或存储状态。
 * 返回值: 无
 * 执行说明: 后台参数与遥测服务；不从 10 kHz ISR 调用，也不任意访问上位机给出的 RAM 地址。
 *================================================================*/
void SstParameterStore_LoadPersistent(void) { storageResult=StoreLoad(); }

