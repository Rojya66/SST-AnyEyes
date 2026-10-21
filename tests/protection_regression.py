"""Compile the actual HVAC RMS detector and unified fast protection bodies.

Only hardware actions and measurement storage are substituted. This checks
threshold ownership and fault confirmation, not the electrical power stage.
"""
from pathlib import Path
import argparse
import shutil
import subprocess
import tempfile

from realtime_audit import function, read

ROOT = Path(__file__).resolve().parents[1]

PREFIX = r'''
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "ChbDabControl.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define PortStatusNum_Run 0x20U
static struct { struct { unsigned short SVG_Start; } Bbits; } ControlFLAG;
static struct { float PORT_I_A, PORT_I_B, PORT_I_C; } PORT_RealTime_DATA;
static unsigned short Port_RunStatus, HVAC_OC_Fault, HVAC_OV_Fault, OC_OV_LV_PortLatch_TAD;
static unsigned short counter_OC_flag, counter_OC_flag1, counter_OC_time, counter_OC_time1;
/* These legacy OV fields deliberately have hostile values. They must not
 * provide a second active trip path in the HVAC detector. */
static unsigned short counter_OV_flag, counter_OV_time, OV_level_SET = 10U, OV_time_SET;
static unsigned short OC_level_SET = 40U, OC_time_SET = 100U;
static unsigned short OC_level_SET_L2 = 45U, OC_time_SET_L2 = 5U;
static float SVG_Udc_Ave = 1000.0F;
static float SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave;
static float HA_Port_Current_A, HA_Port_Current_B, HA_Port_Current_C;
static unsigned short s_ac_overcurrent_count, s_dc_overvoltage_count;
volatile CHB_DAB_PARAMETERS g_chb_dab_params;
volatile unsigned short g_chb_dab_protection_enable = 1U;
static unsigned short fault, dispose_calls;
static float Chb_Abs(float x) { return fabsf(x); }
#if !SST_PHASE_A_ONLY_TEST
static float Chb_Max3(float a, float b, float c) { return fmaxf(a, fmaxf(b, c)); }
#endif
void ChbDabControl_ForceFault(unsigned short code) { if (fault == 0U) fault = code; }
static void OC_OV_LV_FaultDispose(void) { ++dispose_calls; OC_OV_LV_PortLatch_TAD = 1U; }
'''

SCENARIOS = r'''
static void reset(void)
{
    OV_level_SET = 10U;
    OV_time_SET = 0U;
    SVG_Udc_Ave = 1000.0F;
    fault = HVAC_OC_Fault = HVAC_OV_Fault = OC_OV_LV_PortLatch_TAD = dispose_calls = 0U;
    counter_OC_flag = counter_OC_flag1 = counter_OC_time = counter_OC_time1 = 0U;
    counter_OV_flag = counter_OV_time = 0U;
    s_ac_overcurrent_count = s_dc_overvoltage_count = 0U;
    PORT_RealTime_DATA.PORT_I_A = PORT_RealTime_DATA.PORT_I_B = PORT_RealTime_DATA.PORT_I_C = 0.0F;
    HA_Port_Current_A = HA_Port_Current_B = HA_Port_Current_C = 0.0F;
    SVG_Udc_A_Ave = SVG_Udc_B_Ave = SVG_Udc_C_Ave = 100.0F;
    Port_RunStatus = PortStatusNum_Run;
    g_chb_dab_protection_enable = 1U;
    g_chb_dab_params.ac_overcurrent_trip_a = 60.0F;
    g_chb_dab_params.dc_overvoltage_trip_v = 150.0F;
    g_chb_dab_params.protection_confirm_samples = 5U;
}

int main(void)
{
    unsigned int sample;
    reset();
    for (sample = 0U; sample < 1000U; ++sample)
    {
        if (counter_OV_flag != 0U) ++counter_OV_time;
        Chb_CheckSoftwareProtection();
    }
    CHECK(fault == 0U && HVAC_OV_Fault == 0U && counter_OV_flag == 0U);

    SVG_Udc_A_Ave = 150.0F;
    for (sample = 0U; sample < 4U; ++sample) Chb_CheckSoftwareProtection();
    CHECK(fault == 0U);
    SVG_Udc_A_Ave = 149.0F;
    Chb_CheckSoftwareProtection();
    CHECK(s_dc_overvoltage_count == 0U);
    SVG_Udc_A_Ave = 150.0F;
    for (sample = 0U; sample < 4U; ++sample) Chb_CheckSoftwareProtection();
    CHECK(fault == 0U);
    Chb_CheckSoftwareProtection();
    CHECK(fault == CHB_FAULT_DC_OVERVOLTAGE);

    reset();
    SVG_Udc_B_Ave = 200.0F;
    for (sample = 0U; sample < 5U; ++sample) Chb_CheckSoftwareProtection();
#if SST_PHASE_A_ONLY_TEST
    CHECK(fault == 0U);
#else
    CHECK(fault == CHB_FAULT_DC_OVERVOLTAGE);
#endif

    reset();
    HA_Port_Current_A = -60.0F;
    for (sample = 0U; sample < 4U; ++sample) Chb_CheckSoftwareProtection();
    CHECK(fault == 0U);
    Chb_CheckSoftwareProtection();
    CHECK(fault == CHB_FAULT_AC_OVERCURRENT);

    reset();
    PORT_RealTime_DATA.PORT_I_A = 41.0F;
    OC_OV_LV_faultcheck();
    counter_OC_time = 101U;
    OC_OV_LV_faultcheck();
    CHECK(HVAC_OC_Fault == 1U);
    PORT_RealTime_DATA.PORT_I_A = 0.0F;
    Chb_CheckSoftwareProtection();
    CHECK(HVAC_OC_Fault == 1U && fault == CHB_FAULT_HARDWARE && dispose_calls == 1U);

    reset();
    PORT_RealTime_DATA.PORT_I_A = 46.0F;
    OC_OV_LV_faultcheck();
    counter_OC_time1 = 6U;
    OC_OV_LV_faultcheck();
    CHECK(HVAC_OC_Fault == 1U);

    reset();
    g_chb_dab_protection_enable = 0U;
    HA_Port_Current_A = 1000.0F;
    SVG_Udc_A_Ave = 1000.0F;
    PORT_RealTime_DATA.PORT_I_A = 1000.0F;
    for (sample = 0U; sample < 100U; ++sample) Chb_CheckSoftwareProtection();
    CHECK(fault == 0U && counter_OC_flag == 0U && s_dc_overvoltage_count == 0U);
    puts("PASS unified DC threshold/confirmation, phase selection, instantaneous OC and both latched RMS OC levels");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default=shutil.which("gcc"))
    args = parser.parse_args()
    if not args.cc:
        raise SystemExit("Host C compiler required; pass --cc")
    legacy = function(read(ROOT / "Source/04_App/08_Fault/FaultCheck.C"), "OC_OV_LV_faultcheck")
    fast = function(read(ROOT / "Source/04_App/04_Control/ChbDabControlFast.c"), "Chb_CheckSoftwareProtection")
    assert "Udc_check" not in fast and "HVAC_OV_Fault" not in fast
    with tempfile.TemporaryDirectory(prefix="sst_protection_") as folder:
        work = Path(folder)
        for path in ("Source/04_App/04_Control/ChbDabControl.h", "Source/04_App/00_Config/ChbBuildConfig.h"):
            shutil.copy2(ROOT / path, work / Path(path).name)
        unit = work / "protection.c"
        unit.write_text(PREFIX + "\nstatic void OC_OV_LV_faultcheck(void)" + legacy +
                        "\nstatic void Chb_CheckSoftwareProtection(void)" + fast + SCENARIOS, encoding="utf-8")
        for phase in (0, 1):
            binary = work / f"protection_{phase}.exe"
            subprocess.run([args.cc, "-std=c99", "-O2", "-Wall", "-Wextra",
                            f"-DSST_PHASE_A_ONLY_TEST={phase}", "-DSST_LOW_VOLTAGE_100V_TEST=0",
                            str(unit), "-lm", "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
