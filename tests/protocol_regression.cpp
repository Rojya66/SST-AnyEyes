#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#undef assert
#define assert(condition) do { if(!(condition)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition); exit(1); } } while(0)
#include <vector>
#include <deque>
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstFault.h"
#include "SstDebug.h"
#include "SstParams.h"
#include "SstBoard.h"
#define SOC_UART_2_REGS 0U
#define SOC_GPIO_0_REGS 0U
#define UART_LSR 0U
#define UART_LSR_TEMT 0x40U
#define UART_INT_RXDATA_CTI 1U
#define UART_INT_TX_EMPTY 2U
#define UART_WORDL_8BITS 3U
#define UART_OVER_SAMP_RATE_16 4U
#define UART_RX_TRIG_LEVEL_1 1U
#define VALIDFLAG 1U
static unsigned hwLsr=UART_LSR_TEMT;
#define HWREG(x) hwLsr
static std::deque<uint8_t> hwRx;
static std::vector<uint8_t> hwTx;
static unsigned direction,txBudget=65535,rxReads,eepromWrites;
static uint8_t eeprom[512];
static void UARTIntDisable(unsigned,unsigned) {}
static void UARTConfigSetExpClk(unsigned,unsigned,unsigned,unsigned,unsigned) {}
static void UARTFIFOEnable(unsigned) {}
static void UARTFIFOLevelSet(unsigned,unsigned) {}
static void UARTEnable(unsigned) {}
static void GPIOPinWrite(unsigned,unsigned,unsigned value) {direction=value;}
static unsigned UARTCharsAvail(unsigned) {return !hwRx.empty();}
static unsigned UARTCharGetNonBlocking(unsigned) {uint8_t b=hwRx.front();hwRx.pop_front();++rxReads;return b;}
static unsigned UARTSpaceAvail(unsigned) {return txBudget!=0;}
static void UARTCharPutNonBlocking(unsigned,unsigned b) {assert(direction);--txBudget;hwTx.push_back((uint8_t)b);}
static uint8_t r_eeprom_byte_read(uint8_t page,uint8_t offset) {return eeprom[page*64+offset];}
static uint8_t r_eeprom_byte_write(uint8_t page,uint8_t offset,uint8_t value)
{assert(page>=6&&page<=7);eeprom[page*64+offset]=value;++eepromWrites;return 1;}
volatile CHB_DAB_STATUS g_chb_dab_status;
volatile unsigned short g_chb_dab_protection_enable=1;
float HA_Port_Volt_A=1.25F,HA_Port_Volt_B=2,HA_Port_Volt_C=3;
float HA_Port_Current_A=-4,HA_Port_Current_B=5,HA_Port_Current_C=6;
struct { float PORT_V_A; } PORT_RealTime_DATA = { 1.25F };
unsigned short Port_State=7;
uint32_t SstBoard_GetTick1ms(void) {return (uint32_t)g_chb_dab_status.tick_10ms*10;}
unsigned short SstFault_GetCode(unsigned short i) {return i==0?29103:0;}
unsigned short SstFault_GetActiveCount(void) {return 1;}
unsigned short SstFault_GetHistoryCount(void) {return 1;}
unsigned short SstFault_GetHistoryCode(unsigned short i) {return i==0?29103:0;}
unsigned short SstFault_GetHistoryLevel(void) {return 1;}
unsigned short SstFault_GetFirstCode(void) {return 29103;}
unsigned short SstFault_GetLevel(void) {return 1;}
unsigned short SstFault_GetLegacyHistoryWord(unsigned short i) {return i==0?3:i==2?29103:0;}
unsigned short SstFault_GetHistory(unsigned short i,SST_FAULT_RECORD *r)
{if(i)return 0;memset(r,0,sizeof(*r));r->code=29103;r->sequence=1;return 1;}
/* DEFAULTS */
/* IMPLEMENTATION */
static uint8_t RefCrc(const std::vector<uint8_t>&v)
{
 unsigned c=0;
 for(size_t i=0;i<v.size();++i){c^=v[i];for(int k=0;k<8;++k)c=(c<<1)^((c&128)?7:0);c&=255;}
 return (uint8_t)(c^255);
}
static std::vector<uint8_t> Frame(unsigned op,std::vector<uint8_t> payload)
{
 std::vector<uint8_t> v;v.push_back(0x6B);v.push_back((uint8_t)(payload.size()+1));v.push_back((uint8_t)op);
 v.insert(v.end(),payload.begin(),payload.end());v.push_back(RefCrc(v));return v;
}
static void Send(unsigned op,std::vector<uint8_t> payload)
{std::vector<uint8_t> f=Frame(op,payload);for(size_t i=0;i<f.size();++i)hwRx.push_back(f[i]);}
static void Drain(void)
{for(int i=0;i<200;++i){txBudget=65535;SstDebug_Service();}assert(SstSerial_TxFreeFrames()==8);}
static void Reset(void)
{
 hwRx.clear();hwTx.clear();txBudget=65535;hwLsr=UART_LSR_TEMT;rxReads=0;
 g_chb_dab_command.run_enable=0;g_chb_dab_command.dab_enable=0;
 g_chb_dab_status.state=CHB_STATE_STOPPED;g_chb_dab_status.ac_pwm_enabled=0;g_chb_dab_status.dab_pwm_enabled=0;
 g_chb_dab_status.tick_10ms=0;SstDebug_Init();
}
static void Params(void)
{
 uint16_t value;
 Reset();
 assert(SstParams_Read(1,24,&value)==0&& (int16_t)value==-1249);
 assert(SstParams_Write(1,24,(uint16_t)(int16_t)-1500)==0);
 assert(g_chb_dab_params.current_kp==-1.5F);
 assert(SstParams_Write(1,0,1000)==SST_PARAM_READ_ONLY);
 assert(SstParams_Write(1,27,0)==SST_PARAM_RANGE);
 assert(SstParams_Read(30,0,&value)==SST_PARAM_BAD_ADDRESS);
 assert(SstParams_Read(2,33,&value)==0&&value==0);
 assert(SstParams_Write(2,33,1)==SST_PARAM_READ_ONLY);
 g_chb_dab_status.state=CHB_STATE_RUNNING;
 assert(SstParams_Write(1,24,1)==SST_PARAM_BUSY);
 g_chb_dab_status.state=CHB_STATE_STOPPED;
 assert(SstParams_RunCommand(1)==(SST_ALLOW_SERIAL_COMMANDS?0:SST_PARAM_DENIED));
 assert(g_chb_dab_command.run_enable==(SST_ALLOW_SERIAL_COMMANDS?1:0));
 SstParams_EmergencyStop();assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
 SstDisplay_Refresh();
 assert(SstDisplay_Read(79)==1&&SstDisplay_Read(80)==1&&SstDisplay_Read(81)==29103);
 assert(SstParams_Read(19,0,&value)==0&&value==3);
 assert(SstParams_Read(19,1,&value)==0&&value==0);
 assert(SstParams_Read(19,2,&value)==0&&value==29103);
 memset(eeprom,0xFF,sizeof(eeprom));eepromWrites=0;
 assert(SstParams_Write(29,1,0xA55A)==SST_PARAM_STORAGE);
 assert(SstParams_Write(29,0,0x0000)==SST_PARAM_RANGE);
 assert(!SstParams_IsStorageBusy()&&eepromWrites==0);
 const uint16_t commandIndices[5]={2,4,5,6,7};
 const uint16_t savedCommands[5]={1,3456,6012,7123,(uint16_t)(int16_t)-1234};
 for(int i=0;i<5;++i) assert(SstParams_Write(0,commandIndices[i],savedCommands[i])==0);
 assert(SstParams_Write(29,0,0xA55A)==0);
 assert(SstParams_Write(0,4,1)==SST_PARAM_BUSY);
 assert(SstParams_Write(29,1,0xA55A)==SST_PARAM_BUSY);
 for(int i=0;i<200;++i) {
  unsigned old=eepromWrites;++g_chb_dab_status.tick_10ms;SstParams_Service();
  assert(eepromWrites-old<=1);
 }
 assert(SstParams_GetStorageResult()==0&&eepromWrites==93);
 const uint16_t differentCommands[5]={0,1000,5000,6800,100};
 for(int i=0;i<5;++i) assert(SstParams_Write(0,commandIndices[i],differentCommands[i])==0);
 assert(SstParams_Write(1,24,0)==0);
 assert(SstParams_Write(29,1,0xA55A)==0&&g_chb_dab_params.current_kp==-1.5F);
 for(int i=0;i<5;++i) {
  assert(SstParams_Read(0,commandIndices[i],&value)==0);
  assert(value==savedCommands[i]);
 }
 eeprom[6*64+20]^=1;
 assert(SstParams_Write(29,1,0xA55A)==SST_PARAM_STORAGE);
 assert(SstParams_Read(0,7,&value)==0&&value==savedCommands[4]);
}
static void Transport(void)
{
 Reset();uint8_t a[3]={1,2,3},b[2]={4,5};
 assert(SstSerial_SendFrame(a,3));assert(SstSerial_SendFrame(b,2));
 txBudget=1;hwLsr=0;SstDebug_Service();assert(hwTx.size()==1&&direction==1);
 txBudget=99;SstDebug_Service();assert(hwTx.size()==3&&direction==1);
 for(int i=0;i<10;++i)SstDebug_Service();assert(hwTx.size()==3);
 hwLsr=UART_LSR_TEMT;Drain();assert(hwTx==std::vector<uint8_t>({1,2,3,4,5})&&direction==0);
 Reset();for(int i=0;i<1000;++i)hwRx.push_back(0x11);
 SstDebug_Service();assert(rxReads==64);
}
static void Protocol(void)
{
#if SST_DEBUG_PROTOCOL == SST_DEBUG_PROTOCOL_ANYEYES
 Reset();
 Send(0xF0,{1,24});Drain();assert(hwTx.empty());
 Send(0xFF,{255,255,255,255});Drain();
 assert(hwTx==Frame(0x7F,{0x55,255,43,1,0,1,0}));hwTx.clear();
 Send(0xF1,{1,24,0xFA,0x24});Drain();assert(hwTx==Frame(0x71,{0}));hwTx.clear();
 Send(0xF0,{1,24});Drain();assert(hwTx==Frame(0x70,{1,24,0xFA,0x24,0}));hwTx.clear();
 Send(0xF2,{1,200});Drain();++g_chb_dab_status.tick_10ms;Drain();assert(hwTx.empty());
 Send(0xF2,{1,0,81});Drain();++g_chb_dab_status.tick_10ms;Drain();
 assert(hwTx==Frame(0x72,{0,0,0x71,0xAF}));hwTx.clear();
 Send(0xF3,{});Drain();++g_chb_dab_status.tick_10ms;Drain();assert(hwTx.empty());
 Send(0xF4,{});Drain();++g_chb_dab_status.tick_10ms;Drain();
 assert(hwTx==Frame(0x74,{0,0,0}));hwTx.clear();
 Send(0xF5,{});Drain();
 auto bad=Frame(0xF1,{1,24,0,1});bad.back()^=1;
 for(size_t i=0;i<bad.size();++i)hwRx.push_back(bad[i]);Drain();assert(hwTx.empty());
 hwRx.push_back(0x6B);hwRx.push_back(0xFF);Drain();
 Send(0xF0,{1,24});Drain();assert(hwTx==Frame(0x70,{1,24,0xFA,0x24,0}));hwTx.clear();
 Send(0xF8,{0x55});Drain();assert(g_chb_dab_command.run_enable==(SST_ALLOW_SERIAL_COMMANDS?1:0));
 Send(0xFC,{0x55,0xAA,0xCC});Drain();assert(!g_chb_dab_command.run_enable);
 g_chb_dab_status.tick_10ms=26;Drain();
 assert(hwTx==Frame(0x7E,{0xFF,0xAA,0xAA}));hwTx.clear();
 g_chb_dab_status.ac_pwm_enabled=1;
 g_chb_dab_status.tick_10ms=52;Drain();
 assert(hwTx==Frame(0x7E,{0xFF,0x55,0xAA}));hwTx.clear();
 g_chb_dab_status.ac_pwm_enabled=0;
 g_chb_dab_status.tick_10ms+=3001;Drain();hwTx.clear();
 Send(0xF0,{1,24});Drain();assert(hwTx.empty());
#endif
}
static void Vofa(void)
{
 Reset();VOFA_Send4Float(1,-2,0,3.5F);Drain();
 const uint8_t expected[]={0,0,128,63,0,0,0,192,0,0,0,0,0,0,96,64,0,0,128,127};
 assert(hwTx==std::vector<uint8_t>(expected,expected+sizeof(expected)));
}
int main(void)
{
 fprintf(stderr,"params\n");Params();fprintf(stderr,"transport\n");Transport();fprintf(stderr,"protocol\n");Protocol();fprintf(stderr,"vofa\n");Vofa();
 puts("PASS: mapping/permissions/history/save CRC, framing/CRC/recovery/stream/timeout, bounded RX/nonblocking TX, VOFA");
}

