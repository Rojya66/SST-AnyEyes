/* Real maintenance and parameter modules; hardware and transport substitutes. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstFault.h"
#include "SstParams.h"
#include "SstCommand.h"
#include "SstParameterTable.h"
#include "SstParameterStore.h"
#include "MaintenanceProtocol.H"
#include "SstDebug.h"
#define VALIDFLAG 1U
#define INVALIDFLAG 0U
/* LEGACY_GLOBALS */
/* DEFAULTS */
volatile CHB_DAB_STATUS g_chb_dab_status;
volatile unsigned short g_chb_dab_protection_enable;
static uint8_t eeprom[512], reply[256];
static uint16_t reply_length;
static unsigned writes, regulate_calls, legacy_writes, new_writes;

uint32_t SstDebug_GetTick10ms(void) { return (uint32_t)g_chb_dab_status.tick_10ms; }
uint16_t SstSerial_TxFreeFrames(void) { return 8; }
int SstSerial_SendFrame(const uint8_t *bytes,uint16_t count)
{ assert(count <= sizeof(reply)); memcpy(reply,bytes,count); reply_length=count; return 1; }
static uint8_t r_eeprom_byte_read(uint8_t page,uint8_t offset)
{ assert(page>=4 && page<=7 && offset<64); return eeprom[page*64+offset]; }
static uint8_t r_eeprom_byte_write(uint8_t page,uint8_t offset,uint8_t value)
{ assert(page>=4 && page<=7 && offset<64); eeprom[page*64+offset]=value;
  ++writes; if(page<6)++legacy_writes;else ++new_writes; return VALIDFLAG; }
static uint8_t r_eeprom_section_read(uint8_t page,uint8_t offset,uint8_t count,uint8_t *data)
{ assert(page>=4 && page<=7 && offset+count<=64); memcpy(data,eeprom+page*64+offset,count); return VALIDFLAG; }
void RemoteRegulate_Dispose_2(unsigned char unused) { (void)unused; ++regulate_calls; }
unsigned short SstFault_GetCode(unsigned short i) { (void)i;return 0; }
unsigned short SstFault_GetActiveCount(void) { return 0; }
unsigned short SstFault_GetHistoryCount(void) { return 0; }
unsigned short SstFault_GetHistoryLevel(void) { return 4; }
unsigned short SstFault_GetFirstCode(void) { return 0; }
unsigned short SstFault_GetLevel(void) { return 4; }
unsigned short SstFault_GetLegacyHistoryWord(unsigned short i) { (void)i; return 0; }
/* REMOTE_APPLY */
/* IMPLEMENTATION */

static void stopped(void)
{
    g_chb_dab_status.state=CHB_STATE_STOPPED;
    g_chb_dab_status.ac_pwm_enabled=0; g_chb_dab_status.dab_pwm_enabled=0;
    g_chb_dab_command.run_enable=0;g_chb_dab_command.dab_enable=0;
    g_chb_dab_command.fault_reset=0;
    Port_RunStatus=PortStatusNum_Standby;
}
static void request(uint8_t op,const uint8_t *data,uint16_t count)
{
    uint8_t frame[256];uint16_t i,crc;
    assert(count+10<=sizeof(frame));
    frame[0]=MAINT_SOF0;frame[1]=MAINT_SOF1;frame[2]=MAINT_VERSION;
    frame[3]=0;frame[4]=0x37;frame[5]=op;MaintWrite16(frame+6,count);
    if(count)memcpy(frame+8,data,count);
    crc=MaintCRC16(frame,count+8);MaintWrite16(frame+count+8,crc);
    reply_length=0;
    for(i=0;i<count+10;++i) MaintenanceProtocol_RxByte(frame[i]);
    for(i=0;i<5;++i)MaintenanceProtocol_Process();
}
static uint8_t exec(uint8_t control,uint16_t value)
{
    uint8_t data[3]={0};data[0]=control;MaintWrite16(data+1,value);
    request(MAINT_CMD_EXEC_CONTROL,data,3);assert(reply_length>=11);
    assert(reply[5]==MAINT_CMD_EXEC_CONTROL && reply[4]==0x37);
    return reply[8];
}
static uint8_t control_block(uint16_t start,uint16_t stop,uint16_t on,uint16_t off)
{
    XXG_DPRAM_RemControl block;uint8_t data[38];unsigned i;
    memset(&block,0,sizeof(block));block.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16=start;
    block.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16=stop;
    block.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16=on;
    block.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16=off;
    data[0]=MAINT_OBJ_CONTROL;data[1]=0;MaintWrite16(data+2,0);MaintWrite16(data+4,16);
    for(i=0;i<16;++i)MaintWrite16(data+6+i*2,block.DPRAM_Block1Buf[i]);
    request(MAINT_CMD_WRITE_OBJECT,data,sizeof(data));assert(reply_length>=11);
    return reply[8];
}
static void finish_saves(void)
{
    unsigned i,before;
    for(i=0;i<200 && SstParams_IsStorageBusy();++i)
    {
        ++g_chb_dab_status.tick_10ms;before=writes;
        MaintenanceProtocol_Process();SstParams_Service();
        assert(writes-before<=1); /* shared busy must prevent two writers */
    }
    assert(!SstParams_IsStorageBusy());
}
int main(void)
{
    uint16_t value;unsigned before;uint8_t data[8];
    stopped();memset(eeprom,0xff,sizeof(eeprom));MaintenanceProtocol_Init();
#if !SST_ALLOW_SERIAL_COMMANDS
    assert(exec(MAINT_CTL_START,0)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_HF_ON,0)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_LF_ON,0)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_FAULT_CLEAR,0)==MAINT_ERR_NOT_ALLOWED);
    assert(control_block(1,0,3,0)==MAINT_ERR_NOT_ALLOWED);
    assert(SstParams_RunCommand(1)==SST_PARAM_DENIED);
    assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
    g_chb_dab_command.run_enable=1;g_chb_dab_command.dab_enable=1;
    assert(exec(MAINT_CTL_STOP,0)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_HF_OFF,0)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_LF_OFF,0)==MAINT_ERR_NOT_ALLOWED);
    assert(control_block(0,1,0,3)==MAINT_ERR_NOT_ALLOWED);
    assert(g_chb_dab_command.run_enable&&g_chb_dab_command.dab_enable);
    SstParams_EmergencyStop();
    assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
    assert(exec(MAINT_CTL_SWITCH_ON,1)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_SWITCH_EN,1)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_MODE,1)==MAINT_ERR_NOT_ALLOWED);
#if SST_ALLOW_REMOTE_COMMANDS && SST_ENABLE_DPRAM
    SstCommand_ApplyDpram(1,0,0,0,0);
    assert(g_chb_dab_command.run_enable&&g_chb_dab_command.dab_enable);
    SstCommand_ApplyDpram(1,1,3,0,0);
    assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
#else
    SstCommand_ApplyDpram(1,0,3,0,1);
    assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
#endif
    /* Maintenance remains a read/configuration service for these owners. */
    assert(SstParams_Write(29,0,0xA55A)==SST_PARAM_OK);
    request(MAINT_CMD_SAVE,0,0);assert(reply[8]==MAINT_ERR_BUSY);
    finish_saves();
    request(MAINT_CMD_SAVE,0,0);assert(MaintenanceProtocol_IsSaving());
    assert(SstParams_Write(29,0,0xA55A)==SST_PARAM_BUSY);
    finish_saves();assert(reply[8]==MAINT_ERR_OK);
    assert(!regulate_calls && legacy_writes==93 && new_writes==93);
    puts("PASS: serial commands denied by simulator/DPRAM owner; unconditional emergency stop remains available");
    puts("PASS: correct DPRAM ownership and maintenance/new-storage mutual exclusion");
    return 0;
#endif
    assert(exec(MAINT_CTL_START,0)==MAINT_ERR_OK);
    assert(g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
    assert(exec(MAINT_CTL_STOP,0)==MAINT_ERR_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(control_block(1,1,3,0)==MAINT_ERR_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(control_block(1,0,3,3)==MAINT_ERR_OK);
    assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
    assert(exec(MAINT_CTL_HF_ON,0)==0 && g_chb_dab_command.dab_enable);
    assert(exec(MAINT_CTL_HF_OFF,0)==0 && !g_chb_dab_command.dab_enable);
    assert(exec(MAINT_CTL_LF_ON,0)==0 && g_chb_dab_command.run_enable);
    assert(exec(MAINT_CTL_LF_OFF,0)==0 && !g_chb_dab_command.run_enable);
    assert(exec(MAINT_CTL_SWITCH_ON,1)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_SWITCH_OFF,1)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_SWITCH_EN,1)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_SWITCH_DIS,1)==MAINT_ERR_NOT_ALLOWED);
    assert(exec(MAINT_CTL_MODE,1)==MAINT_ERR_NOT_ALLOWED);
    assert(!regulate_calls);
    puts("PASS: real framed maintenance commands, STOP/OFF priority, manual switching denied");

    assert(SstParams_Write(29,0,0xA55A)==SST_PARAM_OK);
    assert(SstParams_IsStorageBusy());before=legacy_writes;
    assert(exec(MAINT_CTL_START,0)==MAINT_ERR_BUSY);
    assert(exec(MAINT_CTL_HF_ON,0)==MAINT_ERR_BUSY);
    assert(exec(MAINT_CTL_LF_ON,0)==MAINT_ERR_BUSY);
    assert(exec(MAINT_CTL_FAULT_CLEAR,0)==MAINT_ERR_BUSY);
    assert(control_block(1,0,3,0)==MAINT_ERR_OK);
    assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
    request(MAINT_CMD_SAVE,0,0);assert(reply[8]==MAINT_ERR_BUSY);
    assert(!MaintenanceProtocol_IsSaving());
    g_chb_dab_command.run_enable=1;g_chb_dab_command.dab_enable=1;
    assert(exec(MAINT_CTL_STOP,0)==MAINT_ERR_OK);
    assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
    g_chb_dab_command.run_enable=1;g_chb_dab_command.dab_enable=1;
    assert(control_block(1,1,3,0)==MAINT_ERR_OK);
    assert(!g_chb_dab_command.run_enable&&!g_chb_dab_command.dab_enable);
    finish_saves();assert(legacy_writes==before && new_writes==93);
    assert(SstParams_GetStorageResult()==SST_PARAM_OK);
    puts("PASS: new save blocks maintenance START/ON/SAVE/reset; STOP remains effective");

    request(MAINT_CMD_SAVE,0,0);assert(MaintenanceProtocol_IsSaving());
    assert(SstParams_IsStorageBusy());before=new_writes;
    assert(SstParams_Write(29,0,0xA55A)==SST_PARAM_BUSY);
    assert(SstParams_Write(29,1,0xA55A)==SST_PARAM_BUSY);
    assert(SstParams_Write(1,24,100)==SST_PARAM_BUSY);
    assert(SstParams_RunCommand(1)==SST_PARAM_BUSY);
    assert(exec(MAINT_CTL_START,0)==MAINT_ERR_BUSY);
    assert(exec(MAINT_CTL_STOP,0)==MAINT_ERR_OK);
    assert(exec(MAINT_CTL_HF_OFF,0)==MAINT_ERR_OK);
    assert(exec(MAINT_CTL_LF_OFF,0)==MAINT_ERR_OK);
    assert(SstParams_RunCommand(0)==SST_PARAM_OK);
    data[0]=MAINT_OBJ_PARAMETER;data[1]=6;MaintWrite16(data+2,0);
    MaintWrite16(data+4,1);MaintWrite16(data+6,120);
    request(MAINT_CMD_WRITE_OBJECT,data,sizeof(data));assert(reply[8]==MAINT_ERR_NOT_ALLOWED);
    finish_saves();assert(new_writes==before && legacy_writes==93);
    assert(reply[5]==MAINT_CMD_SAVE && reply[4]==0x37 && reply[8]==MAINT_ERR_OK);
    assert(eeprom[256]==MAINT_PERSIST_MAGIC0 && eeprom[384]==0x53);
    assert(SstParams_Read(29,1,&value)==SST_PARAM_OK && !value);
    puts("PASS: maintenance save owns shared busy; parameter/save/load/start interlock and async completion");
    return 0;
}
