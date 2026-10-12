/* Fake primitive boundary for real SystemInit/TaskSlice/CPU/ISR source units. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ChbBuildConfig.h"
#include "SstControlConfig.h"
#include "SstBoardConfig.h"
#include "SystemInit.h"
#include "TaskSlice.h"
#include "CpuTask.h"
#include "Redef_Interrupt.h"
#include "SstBoard.h"
#include "ChbDabControl.h"
#include "ControlData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SoftTimeData.h"
#include "DpramHostData.h"
#include "Ft3RecorderData.h"
#include "FpgaRegister.h"
#include "interrupt.h"
#include "gpio.h"
#include "timer.h"

volatile CHB_DAB_COMMAND g_chb_dab_command;
volatile CHB_DAB_PARAMETERS g_chb_dab_params;
volatile CHB_DAB_STATUS g_chb_dab_status;
volatile unsigned short g_chb_dab_protection_enable;

static const char *events[4096];
static unsigned int event_count, scenarios;
static int record_events = 1, core_ready, fault_ready, debug_ready;
static unsigned int pending_pin = 69, timer_counter;
static volatile uint32_t registers32[8];
static volatile uint16_t register16;
static void (*handlers[16])(void);

static void event(const char *name) { if(record_events) { if(event_count>=4096) abort(); events[event_count++]=name; } }
static void fail(const char *message) { fprintf(stderr,"FAIL %s\n",message); exit(1); }
static void check(int condition,const char *message) { if(!condition) fail(message); }
static void reset_events(void) { event_count=0; }
static int index_of(const char *name) { unsigned int i; for(i=0;i<event_count;i++) if(!strcmp(events[i],name)) return (int)i; return -1; }
static unsigned int count_of(const char *name) { unsigned int i,count=0; for(i=0;i<event_count;i++) if(!strcmp(events[i],name)) count++; return count; }
static void before(const char *first,const char *second) { check(index_of(first)>=0 && index_of(second)>index_of(first),"required call order"); }
static void exact(const char *const *expected,unsigned int count) { unsigned int i; check(event_count==count,"unexpected call count"); for(i=0;i<count;i++) { if(strcmp(events[i],expected[i])) { fprintf(stderr,"at %u expected %s got %s\n",i,expected[i],events[i]); fail("unexpected call trace"); } } scenarios++; }

volatile uint32_t *TraceRegister32(uintptr_t address) { event("HWREG32"); return &registers32[(address>>2)&7]; }
volatile uint16_t *TraceRegister16(uintptr_t address) { (void)address; event("mailbox"); return &register16; }
void GPIOPinWrite(unsigned int base,unsigned int pin,unsigned int value) { (void)base; (void)value; if(pin==43) event(value?"pin43on":"pin43off"); else if(pin==45) event(value?"pin45on":"pin45off"); else if(pin==144) event("dpram_ack"); }
void GPIOBankIntDisable(unsigned int base,unsigned int bank) { (void)base; event(bank==4?"bank4off":bank==8?"bank8off":"bank6off"); }
void GPIOBankIntEnable(unsigned int base,unsigned int bank) { (void)base; event(bank==4?"bank4on":bank==8?"bank8on":"bank6on"); }
unsigned int GPIOPinIntStatus(unsigned int base,unsigned int pin) { (void)base; event("pending"); return pin==pending_pin?GPIO_INT_PEND:0; }
void GPIOPinIntClear(unsigned int base,unsigned int pin) { (void)base; (void)pin; event("gpio_clear"); }
void GPIOIntTypeSet(unsigned int base,unsigned int pin,unsigned int type) { (void)base; (void)type; event(pin==69?"tad_edge":pin==142?"fault_edge":"dpram_edge"); }
void IntRegister(unsigned int vector,void (*handler)(void)) { handlers[vector]=handler; event(vector==C674X_MASK_INT5?"register_tad":vector==C674X_MASK_INT4?"register_fault":vector==C674X_MASK_INT7?"register_dpram":"register_timer"); }
void IntEventMap(unsigned int vector,unsigned int source) { (void)vector; (void)source; event("map"); }
void IntEnable(unsigned int vector) { if(vector==C674X_MASK_INT5) { check(core_ready && fault_ready && debug_ready,"TAD enabled before runtime initialization"); event("enable_tad"); } else event(vector==C674X_MASK_INT7?"enable_dpram":vector==C674X_MASK_INT4?"enable_fault":"enable_timer"); }
void IntEventClear(unsigned int source) { (void)source; event("event_clear"); }
void TimerIntDisable(unsigned int base,unsigned int flags) { (void)base; (void)flags; event("timer_off"); }
void TimerIntEnable(unsigned int base,unsigned int flags) { (void)base; (void)flags; event("timer_on"); }
unsigned int TimerIntStatusClear(unsigned int base,unsigned int flags) { (void)base; (void)flags; event("timer_clear"); return 0; }
unsigned int TimerCounterGet(unsigned int base,unsigned int timer) { (void)base; (void)timer; return timer_counter; }

#define STUB(name,label) void name(void) { event(label); }
STUB(PSCInit,"psc")
STUB(EMIFASetup,"emifa")
STUB(GPIOBankPinMuxSet,"pinmux")
STUB(GPIOBankPinInit,"gpio_init")
void IntDSPINTCInit(void) { event("interrupt_init"); }
void IntGlobalEnable(void) { event("global_interrupts"); }
STUB(TimerInit,"timer_init")
STUB(Initial_Variable,"variable_init")
STUB(Variable_RST,"variable_reset")
STUB(MaintenanceProtocol_LoadPersistent,"maintenance_load")
STUB(SstParameters_LoadPersistent,"sst_load")
void ChbDabControl_Init(void) { event("core_init"); core_ready=1; }
void SstFault_Init(void) { event("fault_init"); fault_ready=1; }
void SstDebug_Init(void) { event("debug_init"); debug_ready=1; }
void FaultLatch_Clear(unsigned char run,unsigned char reset) { (void)reset; event(run?"latch_runtime_clear":"latch_boot_clear"); }
void ChbDabControl_Tick10ms(void) { event("core_tick"); g_chb_dab_status.tick_10ms++; }
STUB(ChbDabControl_Service,"core_service")
STUB(SstFault_Service,"fault_service")
STUB(SstDpramHost_ServiceRemoteControl,"dpram_control")
STUB(SstDpramHost_ServiceTelemetry,"dpram_telemetry")
uint32_t SstDebug_GetTick10ms(void) { event("tick_capture"); return (uint32_t)g_chb_dab_status.tick_10ms; }
STUB(SstSerial_TxService,"tx")
STUB(SstDebug_ReceiveService,"rx")
void SstDebug_ProtocolService(uint32_t now) { check(now==(uint32_t)g_chb_dab_status.tick_10ms,"captured scheduler time"); event("protocol"); }
STUB(SstParameterStore_Service,"store")
STUB(Port_RealTimeParameter,"realtime_background")
STUB(Port_StateUpdate_Read,"state_read")
void Port_AD_Sample(unsigned char latch) { check(latch==0,"live AD sample"); event("sample"); }
STUB(HVAC_RealTimeParameter,"rms")
STUB(SVG_Udc_Average,"udc_average")
STUB(ChbDabControl_FastStep,"fast_step")
STUB(Udc_check,"udc_check")
STUB(FaultDispose,"fault_dispose")
void ChbDabControl_ForceFault(unsigned short code) { check(code==CHB_FAULT_HARDWARE,"hardware fault code"); event("force_fault"); }
STUB(OC_OV_LV_FaultDispose,"software_dispose")
STUB(OC_OV_LV_faultcheck,"software_check")
void PortController_Communication(unsigned char mode) { check(mode==1,"stop FPGA communication"); event("stop_communication"); }
STUB(Port_AllStopFlag_RST,"stop_reset")
STUB(Port_StateUpdate_Write,"state_write")
void FT3_PluginUint_Write(unsigned short index,unsigned short count) { check(index==2 && count==dataPackNum,"recorder pack"); event("ft3_write"); }
void FT3_PluginUint_Read(unsigned short index) { check(index==2,"recorder index"); event("ft3_read"); }
STUB(DAC_Output,"dac")
STUB(FaultCheckAndDispose_StepOne,"fault_step1")

static void reset_fast(void) {
    memset((void *)&FaultTypeFLAG,0,sizeof(FaultTypeFLAG));
    memset((void *)&PortRunFLAG,0,sizeof(PortRunFLAG));
    memset((void *)&Port_RealFault,0,sizeof(Port_RealFault));
    ModuleFault_DisposeTimes=0; PortLatch_TAD=0; HVAC_OC_Fault=HVAC_OV_Fault=0;
    OC_OV_LV_PortLatch_TAD=0; g_chb_dab_protection_enable=1; dataPackNum=1;
    pending_pin=69; reset_events();
}

int main(void) {
    unsigned int i;
    static const char *open_trace[]={"bank4off","event_clear","pending","gpio_clear","pin43on","pin45on","state_read","sample","rms","udc_average","fast_step","state_write","ft3_write","ft3_read","dac","pin43off","pin45off","bank4on"};
    static const char *timer_trace[]={"timer_off","event_clear","timer_clear","timer_on","core_tick"};
    static const char *fault_trace[]={"bank8off","event_clear","pending","gpio_clear","fault_step1","fault_dispose","force_fault","bank8on"};
    static const char *dpram_trace[]={"bank6off","event_clear","pending","gpio_clear","bank6on"};
    static const char *background[]={"core_service","fault_service",
#if SST_ENABLE_DPRAM
      "dpram_control","dpram_telemetry",
#endif
      "tick_capture","tx","rx","protocol","store","tx","realtime_background"};
    CpuTask_InitBootFlags(); SystemInit_Hardware();
    before("psc","emifa"); before("emifa","pinmux"); before("gpio_init","interrupt_init"); before("timer_init","register_timer");
    check(handlers[C674X_MASK_INT8]==Timer2Isr && !handlers[C674X_MASK_INT5],"only timer registered during boot"); scenarios++;
    record_events=0; for(i=0;i<2000;i++) Timer2Isr(); record_events=1;
    check(Flag_20s==1 && g_chb_dab_status.tick_10ms==2000,"real timer 20 second flag and core ticks");
    SystemInit_WaitPowerOnDelay(); check(!Flag_20s && !Flag_WDI,"startup wait consumes elapsed flag"); scenarios++;
    reset_events(); SystemInit_Runtime();
    before("variable_init","variable_reset"); before("variable_reset","latch_boot_clear");
    before("latch_boot_clear","core_init"); before("core_init","fault_init"); before("fault_init","debug_init"); before("debug_init","enable_tad");
#if CHB_LOAD_EEPROM_PARAMETERS && SST_ENABLE_MAINTENANCE
    before("variable_reset","maintenance_load"); before("maintenance_load","latch_boot_clear");
#endif
#if CHB_LOAD_EEPROM_PARAMETERS
    before("debug_init","sst_load"); before("sst_load","enable_tad");
#endif
    check(handlers[C674X_MASK_INT5]==FPGAIsr && handlers[C674X_MASK_INT4]==FAULTIsr,"TAD and fault handlers bound");
#if SST_ENABLE_DPRAM
    check(handlers[C674X_MASK_INT7]==DPRAMIsr && count_of("enable_dpram")==1 && count_of("mailbox")==1,"DPRAM compile switch enables notification");
#else
    check(!handlers[C674X_MASK_INT7] && !count_of("mailbox"),"disabled DPRAM never registered or read");
#endif
    scenarios++;
    Flag_1s=0; reset_events(); TaskSlice_Service(); exact(background,sizeof(background)/sizeof(background[0]));
    Flag_1s=1; reset_events(); CpuTask_ServiceHeartbeat(); check(!Flag_1s && Flag_temp && Flag_WDI && FPGA_Interface[MainControl_PluginUnit_4-2].LED.Bbits.led0,"real heartbeat consumes timer flag"); scenarios++;
    reset_fast(); g_chb_dab_command.ac_mode=CHB_AC_MODE_OPEN_LOOP; FPGAIsr(); exact(open_trace,sizeof(open_trace)/sizeof(open_trace[0]));
    reset_fast(); g_chb_dab_command.ac_mode=CHB_AC_MODE_CLOSED_LOOP; FPGAIsr(); before("fast_step","udc_check"); before("udc_check","software_check"); before("software_check","state_write"); check(count_of("software_check")==1,"closed loop checks software protection"); scenarios++;
    reset_fast(); g_chb_dab_command.ac_mode=CHB_AC_MODE_CLOSED_LOOP; HVAC_OC_Fault=1; FPGAIsr(); before("udc_check","software_dispose"); before("software_dispose","force_fault"); before("force_fault","state_write"); check(!count_of("software_check"),"latched software fault uses dispose branch"); scenarios++;
    reset_fast(); g_chb_dab_command.ac_mode=CHB_AC_MODE_OPEN_LOOP; FaultTypeFLAG.Bbits.Module_Fault=1; ModuleFault_DisposeTimes=1; FPGAIsr(); before("fast_step","fault_dispose"); before("fault_dispose","force_fault"); check(count_of("force_fault")==1 && !count_of("software_check"),"hardware protection remains in open loop"); scenarios++;
    reset_fast(); g_chb_dab_command.ac_mode=CHB_AC_MODE_OPEN_LOOP; PortRunFLAG.Bbits.PWMState_50Hz=PortRunFLAG.Bbits.PWMState_Dab=PortRunFLAG.Bbits.Stop_AllPort=1; FPGAIsr(); before("fast_step","stop_communication"); before("stop_communication","stop_reset"); before("stop_reset","state_write"); scenarios++;
    reset_fast(); pending_pin=0; FPGAIsr(); check(event_count==4 && !count_of("fast_step"),"spurious TAD bank interrupt skips control"); scenarios++;
    reset_fast(); pending_pin=142; PortLatch_TAD=1; FAULTIsr(); exact(fault_trace,sizeof(fault_trace)/sizeof(fault_trace[0]));
    reset_fast(); pending_pin=142; FAULTIsr(); check(!count_of("fault_dispose") && !count_of("force_fault"),"no latched fault avoids forced stop"); scenarios++;
    reset_fast(); pending_pin=101; DPRAMIsr(); exact(dpram_trace,sizeof(dpram_trace)/sizeof(dpram_trace[0])); check(Flag_RemoteControlRegulate==1,"DPRAM ISR only signals background");
    reset_fast(); Flag_Clear_0=Flag_Clear_1=0; cout_Wait=2; reset_events(); Timer2Isr(); exact(timer_trace,sizeof(timer_trace)/sizeof(timer_trace[0]));
    Flag_Clear_0=1; Timer2Isr(); check(!Flag_Clear_0 && Flag_Clear_1,"timer clear first stage"); reset_events(); Timer2Isr(); before("core_tick","latch_runtime_clear"); check(!Flag_Clear_1,"timer clear second stage"); scenarios++;
    g_chb_dab_status.tick_10ms=10; timer_counter=(SYSCLK_2_FREQ/1000U)*4U; check(SstBoard_GetTick1ms()==104,"real foreground fractional clock"); timer_counter=0; check(SstBoard_GetTick1ms()==104,"foreground clock monotonic guard"); scenarios++;
    printf("PASS %u startup/scheduler/ISR scenarios; real production units, fake hardware boundaries\n",scenarios);
    return 0;
}
