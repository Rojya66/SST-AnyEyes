"""Exercise the production manual-switch API and its existing core output helpers with fake I/O."""
from pathlib import Path
import shutil
import subprocess
import tempfile
from single_phase_io import extract_function

ROOT=Path(__file__).resolve().parents[1]
PREFIX=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ChbBuildConfig.h"
#include "ChbDabControl.h"
#undef assert
#define assert(x) ((x)?(void)0:(fprintf(stderr,"ASSERT %d: %s\n",__LINE__,#x),exit(2)))
volatile CHB_DAB_COMMAND g_chb_dab_command;
volatile CHB_DAB_STATUS g_chb_dab_status;
volatile CHB_DAB_PARAMETERS g_chb_dab_params;
static unsigned short s_active_ac_mode=CHB_AC_MODE_CLOSED_LOOP;
static unsigned long s_breaker_pulse_end_tick;
static unsigned short interrupts=1, disable_count, restore_count, outputs;
static union { uint16_t PORT_FAULT16; struct {unsigned unused:15;unsigned PORT_FAULT_bit15:1;} Bbits;} Port_RealFault;
static struct {uint16_t FaultTypeFLAG16;} FaultTypeFLAG;
static struct {struct {unsigned short Breaker_InnerOn,Breaker_InnerOff,Contactor_Inner;}Bbits;}Port_DOUT;
static unsigned IntGlobalDisable(void){unsigned old=interrupts;interrupts=0;++disable_count;return old;}
static void IntGlobalRestore(unsigned old){interrupts=old;++restore_count;}
static void Port_IO_Out(void){assert(!interrupts);++outputs;}
'''
TEST=r'''
int main(void){
 unsigned long deadline;
 g_chb_dab_command.ac_mode=CHB_AC_MODE_CLOSED_LOOP;
 g_chb_dab_status.state=CHB_STATE_STOPPED;
 g_chb_dab_params.breaker_pulse_10ms=50;
#if SST_ENABLE_SWITCH_OUTPUT
 assert(ChbDabControl_MaintenanceSwitch(1,0)==CHB_SWITCH_DENIED);
 assert(ChbDabControl_MaintenanceSwitch(2,0)==CHB_SWITCH_OK);
 assert(g_chb_dab_status.ac_input_closed && Port_DOUT.Bbits.Breaker_InnerOn);
 deadline=s_breaker_pulse_end_tick;g_chb_dab_status.tick_10ms=10;
 assert(ChbDabControl_MaintenanceSwitch(2,0)==CHB_SWITCH_OK);
 assert(s_breaker_pulse_end_tick==deadline);
#if SST_ENABLE_SWITCH_FEEDBACK
 assert(ChbDabControl_MaintenanceSwitch(1,0)==CHB_SWITCH_DENIED);
 g_chb_dab_status.ac_input_feedback=1;
#endif
 assert(ChbDabControl_MaintenanceSwitch(1,0)==CHB_SWITCH_OK);
 assert(g_chb_dab_status.precharge_bypassed && Port_DOUT.Bbits.Contactor_Inner);
 assert(ChbDabControl_MaintenanceSwitch(3,3)==CHB_SWITCH_OK);
 assert(!g_chb_dab_status.precharge_bypassed && !g_chb_dab_status.ac_input_closed);
 assert(Port_DOUT.Bbits.Breaker_InnerOff && !Port_DOUT.Bbits.Contactor_Inner);
 FaultTypeFLAG.FaultTypeFLAG16=2;
 assert(ChbDabControl_MaintenanceSwitch(2,0)==CHB_SWITCH_DENIED);
 FaultTypeFLAG.FaultTypeFLAG16=0;g_chb_dab_status.state=CHB_STATE_FAULT;
 assert(ChbDabControl_MaintenanceSwitch(2,0)==CHB_SWITCH_DENIED);
 assert(ChbDabControl_MaintenanceSwitch(0,3)==CHB_SWITCH_OK);
 g_chb_dab_status.state=CHB_STATE_RUNNING;g_chb_dab_command.run_enable=1;
 assert(ChbDabControl_MaintenanceSwitch(0,3)==CHB_SWITCH_BUSY);
 assert(!g_chb_dab_status.ac_pwm_enabled && !g_chb_dab_status.dab_pwm_enabled);
 assert(disable_count==restore_count && interrupts);
#else
 (void)deadline;
 assert(ChbDabControl_MaintenanceSwitch(2,0)==CHB_SWITCH_DENIED);
 assert(!outputs && !disable_count && !g_chb_dab_status.ac_input_closed);
#endif
 puts("PASS manual switch: stopped/fault interlocks, real feedback, OFF priority, no pulse retrigger and no PWM writes");return 0;
}
'''


def main():
    source=(ROOT/'Source/04_App/04_Control/ChbDabControl.c').read_text(encoding='utf-8-sig')
    code=PREFIX+extract_function(source,'Chb_SetAcInput')+'\n'+extract_function(source,'Chb_SetPrechargeBypass')+'\n'
    code+=source[source.index('unsigned short ChbDabControl_MaintenanceSwitch('):]+TEST
    with tempfile.TemporaryDirectory(prefix='sst_manual_switch_') as folder:
        work=Path(folder)
        for path in ['00_Config/ChbBuildConfig.h','04_Control/ChbDabControl.h']:
            shutil.copy2(ROOT/'Source/04_App'/path,work/Path(path).name)
        (work/'switch.c').write_text(code,encoding='utf-8')
        for phase,output,feedback in [(p,o,f) for p in (0,1) for o in (0,1) for f in (0,1)]:
            binary=work/'sst_switch.exe'
            args=[shutil.which('gcc'),'-std=c99','-O2','-I'+str(work),f'-DSST_PHASE_A_ONLY_TEST={phase}',
                  f'-DSST_LOW_VOLTAGE_100V_TEST={phase}',f'-DSST_ENABLE_SWITCH_OUTPUT={output}',
                  f'-DSST_ENABLE_SWITCH_FEEDBACK={feedback}',str(work/'switch.c'),'-o',str(binary)]
            result=subprocess.run(args,capture_output=True,text=True)
            if feedback and not output:
                assert result.returncode!=0 and 'requires switch output enabled' in result.stderr
                print(f'phase={phase}: PASS rejected feedback=1/output=0 at compile time')
                continue
            assert result.returncode==0,result.stderr
            result=subprocess.run([str(binary)],capture_output=True,text=True);assert result.returncode==0,result.stderr
            print(f'phase={phase} output={output} feedback={feedback}: '+result.stdout.strip())


if __name__=='__main__':main()
