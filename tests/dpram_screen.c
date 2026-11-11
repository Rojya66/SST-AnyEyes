#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#include "SstDpramProtocol.h"
#include "SstParameterTable.h"
#include "SstLegacyParameters.h"
#include "SstCommand.h"
#include "SstParameterStore.h"

#define VALIDFLAG 0x55U
static uint8_t eeprom[1024];
static uint16_t fail_write;
static uint16_t hardware_writes, manual_calls;
static uint16_t OC_level_SET=100, OC_time_SET=20, OC_level_SET_L2=120, OC_time_SET_L2=5;
static uint16_t OV_level_SET=300, OV_time_SET=5, HVAC_OC_Fault, HVAC_OV_Fault;
static float HA_L_Voltage_Precharge=10.0F, HA_H_Voltage=400.0F;
static uint16_t Port_AD_PORT_EN=0x7000, Port_FO_1_EN=0x007F, Port_FO_2_EN, Port_FO_3_EN, Port_FB_EN;
static struct { struct { unsigned Port_RunOrTest, Port_AutoOrManual; } Bbits; } PortRunFLAG;
static struct { uint16_t PORT_FAULT16; } Port_RealFault;
static struct { uint16_t FaultTypeFLAG16; } FaultTypeFLAG;
static uint16_t Port_Outer_Elec=1, NHA_module_A=6, NHA_module_B=6, NHA_module_C=6;
static float SVG_Udc_A_Ave=60;
static struct {float PORT_P,PORT_Q,PORT_V_A,PORT_V_B,PORT_V_C,PORT_I_A,PORT_I_B,PORT_I_C,PORT_PF;} PORT_RealTime_DATA;
static void Port_ADProtect_EN(void) {++hardware_writes;}
static void Port_FO_EN_Set(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e) {(void)a;(void)b;(void)c;(void)d;(void)e;++hardware_writes;}
static void Port_FB_EN_Set(void) {++hardware_writes;}
static uint8_t r_eeprom_byte_read(uint8_t p,uint8_t i) {return eeprom[p*64U+i];}
static uint8_t r_eeprom_byte_write(uint8_t p,uint8_t i,uint8_t v) {if(fail_write)return 0;eeprom[p*64U+i]=v;return VALIDFLAG;}
uint16_t SstParams_IsStopped(void) {return SstCommand_IsStopped();}
uint16_t SstParams_IsStorageBusy(void) {return SstParameterStore_IsBusy();}
void ChbDabControl_ApplyRemoteCommand(unsigned short start,unsigned short stop) {
 if(stop&1)g_chb_dab_command.run_enable=g_chb_dab_command.dab_enable=0;
 else if(start&1)g_chb_dab_command.run_enable=g_chb_dab_command.dab_enable=1;
}
unsigned short ChbDabControl_MaintenanceSwitch(unsigned short a,unsigned short b) {(void)a;(void)b;++manual_calls;return SST_PARAM_DENIED;}
unsigned short SstFault_GetFirstCode(void) {return 29103;}
unsigned short SstFault_GetLevel(void) {return 2;}
unsigned short SstFault_GetActiveCount(void) {return 1;}
unsigned short SstFault_GetHistoryCount(void) {return 1;}
unsigned short SstFault_GetCode(unsigned short i) {return i?0:29103;}
/* DEFAULTS */
/* IMPLEMENTATIONS */

#if SST_ENABLE_DPRAM
static void finish_publication(void) {unsigned i;for(i=0;i<20;++i)SstDpramScreen_Service();}
static void transaction(uint16_t seq,uint16_t op) {
 DPRAM_SstScreen[384]=seq;DPRAM_SstScreen[385]=op;SstDpramScreen_Service();
}
#endif
#undef assert
#define assert(x) ((x)?(void)0:(fprintf(stderr,"ASSERT line %d: %s\n",__LINE__,#x),exit(2)))
int main(void) {
 uint16_t w[57], old[43], block[16]={0}, telemetry[33];
 float kp,ki; unsigned i;
 /* Exhaustive signed encoding: both signs, zero and int16 endpoints. */
 for(i=0;i<65536U;++i) {
  assert(SstParameterTable_WriteParameter(24,(uint16_t)i)==0);
  assert(SstParameterTable_ReadParameter(24)==(uint16_t)i);
 }
 g_chb_dab_params.current_kp=-1.249166667F;
 g_chb_dab_status.state=CHB_STATE_STOPPED;
 g_chb_dab_command.run_enable=g_chb_dab_command.dab_enable=0;
 SstDpramParameter_Read(w);
 assert(SstParameterTable_SnapshotValidate(w)==0);
 kp=g_chb_dab_params.current_kp;ki=g_chb_dab_params.current_ki;
#if SST_ENABLE_DPRAM && SST_ALLOW_REMOTE_COMMANDS
 /* A single Udc write must not quantize the untouched current PI. */
 SstDpramParameter_ReadLegacy(old);old[8]=60;
 assert(SstDpramParameter_ApplyLegacy(old)==0);
 assert(g_chb_dab_command.dc_voltage_ref_v==60);
 assert(g_chb_dab_params.current_kp==kp && g_chb_dab_params.current_ki==ki);
 old[29]=(uint16_t)(int16_t)-1250;old[30]=(uint16_t)(int16_t)-16000;
 assert(SstDpramParameter_ApplyLegacy(old)==0);
 assert(g_chb_dab_params.current_kp==-1.25F && g_chb_dab_params.current_ki==-16.0F);
 SstDpramParameter_ReadLegacy(old);assert(old[29]==0xFB1E && old[30]==0xC180);
 /* Reject an unsupported PI group as a whole; Udc must stay 60. */
 old[8]=65;old[31]=10;assert(SstDpramParameter_ApplyLegacy(old)==SST_SCREEN_UNSUPPORTED);
 assert(g_chb_dab_command.dc_voltage_ref_v==60);
 SstDpramParameter_Read(w);w[40]=650;w[48]=w[49];
 assert(SstDpramParameter_Apply(w)==SST_PARAM_RANGE);assert(g_chb_dab_command.dc_voltage_ref_v==60);
 SstDpramParameter_Read(w);w[33]=2500;w[34]=100;
 assert(SstDpramParameter_Apply(w)==SST_PARAM_RANGE);
 SstDpramParameter_Read(w);w[52]=0xF001;assert(SstDpramParameter_Apply(w)==SST_PARAM_RANGE);
 g_chb_dab_command.run_enable=1;assert(SstDpramParameter_Apply(w)==SST_PARAM_BUSY);
 block[2]=block[3]=1;assert(SstDpramCommand_Apply(block)==0);assert(!g_chb_dab_command.run_enable);
 memset(block,0,sizeof(block));block[2]=1;assert(SstDpramCommand_Apply(block)==0);
 assert(g_chb_dab_command.run_enable && g_chb_dab_command.dab_enable);
 block[2]=0;block[3]=1;SstDpramCommand_Apply(block);
 g_chb_dab_status.state=CHB_STATE_FAULT;g_chb_dab_status.fault_code=3;
 memset(block,0,sizeof(block));block[2]=1;assert(SstDpramCommand_Apply(block)==SST_PARAM_DENIED);
 g_chb_dab_status.state=0;g_chb_dab_status.fault_code=0;
 memset(block,0,sizeof(block));block[4]=2;assert(SstDpramCommand_Apply(block)==SST_SCREEN_UNSUPPORTED);
 SstDpramScreen_Init();finish_publication();assert(!(DPRAM_SstScreen[0]&1));
 /* Stage edits remain inactive until op is written last. */
 DPRAM_SstScreen[128+24]=(uint16_t)(int16_t)-1500;finish_publication();
 assert(g_chb_dab_params.current_kp==-1.25F);
 transaction(1,SST_SCREEN_APPLY);assert(DPRAM_SstScreen[386]==1 && DPRAM_SstScreen[387]==0);
 assert(g_chb_dab_params.current_kp==-1.5F);
 /* Exactly-once retry: same sequence/op must not reapply edited stage. */
 DPRAM_SstScreen[128+24]=(uint16_t)(int16_t)-1700;transaction(1,SST_SCREEN_APPLY);
 assert(g_chb_dab_params.current_kp==-1.5F);
 transaction(1,SST_SCREEN_LOAD);assert(DPRAM_SstScreen[387]==SST_SCREEN_CONFLICT);
 transaction(2,SST_SCREEN_REFRESH);assert(DPRAM_SstScreen[128+24]==(uint16_t)(int16_t)-1500);
 transaction(3,SST_SCREEN_SAVE);assert(DPRAM_SstScreen[388]==1 && DPRAM_SstScreen[386]!=3);
 memset(block,0,sizeof(block));block[2]=1;assert(SstDpramCommand_Apply(block)==SST_PARAM_BUSY);
 for(i=0;i<230;++i) {++g_chb_dab_status.tick_10ms;SstParameterStore_Service();SstDpramScreen_Service();}
 assert(DPRAM_SstScreen[386]==3 && DPRAM_SstScreen[387]==0 && !DPRAM_SstScreen[388]);
 g_chb_dab_params.current_kp=-2;transaction(4,SST_SCREEN_LOAD);assert(g_chb_dab_params.current_kp==-1.5F);
 fail_write=1;transaction(5,SST_SCREEN_SAVE);++g_chb_dab_status.tick_10ms;SstParameterStore_Service();SstDpramScreen_Service();
 assert(DPRAM_SstScreen[386]==5 && DPRAM_SstScreen[387]==SST_PARAM_STORAGE);
#else
 assert(SstDpramParameter_Apply(w)==SST_PARAM_DENIED);
 assert(SstDpramCommand_Apply(block)==SST_PARAM_DENIED);
 assert(g_chb_dab_params.current_kp==kp && g_chb_dab_params.current_ki==ki);
 (void)old;(void)i;
#endif
 /* Internal software OV/OC must never appear as COMM/SYNC. */
 Port_RealFault.PORT_FAULT16=0x2000;assert(SstDpramTelemetry_Fault()==0x9000);
 Port_RealFault.PORT_FAULT16=0x4000;assert(SstDpramTelemetry_Fault()==0x8400);
 Port_RealFault.PORT_FAULT16=0;FaultTypeFLAG.FaultTypeFLAG16=0x1800;
 assert(SstDpramTelemetry_Fault()==0xE000);
 FaultTypeFLAG.FaultTypeFLAG16=0;g_chb_dab_status.fault_code=CHB_FAULT_BAD_CONFIGURATION;
 assert(SstDpramTelemetry_Fault()==0x8000);
 g_chb_dab_status.fault_code=0;PortRunFLAG.Bbits.Port_RunOrTest=0;PortRunFLAG.Bbits.Port_AutoOrManual=0;
 g_chb_dab_status.ac_pwm_enabled=1;g_chb_dab_status.dab_pwm_enabled=0;
 SstDpramTelemetry_Build(telemetry);
 assert((telemetry[1]&0xE000)==0x6000);assert((telemetry[1]&0x0300)==0x0200);
 PORT_RealTime_DATA.PORT_I_A=100000;SstDpramTelemetry_Build(telemetry);assert(telemetry[8]==32767);
 puts("PASS: signed PI, untouched precision, transactional rejection, modes, stop/fault priority, EEPROM ACK and Word bit packing");
 return 0;
}
