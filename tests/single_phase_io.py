"""Exercise the real single-phase I/O branches with small host-side stubs.

The test extracts the selected C functions from the current sources, supplies
only the FPGA/measurement objects they touch, and compiles both macro choices
where the function has an unchanged three-phase branch.  It is deliberately a
host test: it checks conversion and write semantics, not C6748 timing.
The old H2/H6 filters are identity stubs; their response is outside this test.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def extract_function(source: str, name: str, occurrence: int = 0) -> str:
    """Return one complete void definition, ignoring prototypes and conditional copies."""
    matches = list(re.finditer(r"\bvoid\s+" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", source))
    if len(matches) <= occurrence:
        raise ValueError(f"missing definition {name}[{occurrence}] ({len(matches)} matches)")
    match = matches[occurrence]
    opening = source.find("{", match.start())
    depth = 0
    in_string = None
    escaped = False
    in_line_comment = False
    in_block_comment = False
    index = opening
    while index < len(source):
        char = source[index]
        next_char = source[index + 1] if index + 1 < len(source) else ""
        if in_line_comment:
            if char == "\n":
                in_line_comment = False
        elif in_block_comment:
            if char == "*" and next_char == "/":
                in_block_comment = False
                index += 1
        elif in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == in_string:
                in_string = None
        elif char in ("\"", "'"):
            in_string = char
        elif char == "/" and next_char == "/":
            in_line_comment = True
            index += 1
        elif char == "/" and next_char == "*":
            in_block_comment = True
            index += 1
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[match.start(): index + 1]
        index += 1
    raise ValueError(f"unterminated definition {name}")


PREFIX = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef struct { short AD[16]; short AD_LATCH[16]; short Avg_IG; short Avg_DC; struct { unsigned short PM_CTL16; } PM_CTL; } FPGA_SLOT;
static FPGA_SLOT FPGA_Interface[16];
static short Port_AD_Buf[16];
static unsigned short Analog_PluginUnit_3 = 2U;
static unsigned short Com_A1_PluginUnit_5 = 5U, Com_A2_PluginUnit_6 = 6U;
static unsigned short Com_B1_PluginUnit_9 = 9U, Com_B2_PluginUnit_10 = 10U;
static unsigned short Com_C1_PluginUnit_13 = 13U, Com_C2_PluginUnit_14 = 14U;
static float HA_CT_Ratio = 1.0F, HA_PT_Ratio = 1.0F;
static float HA_Port_Volt_A, HA_Port_Volt_B, HA_Port_Volt_C;
static float HA_Port_Volt_AB, HA_Port_Volt_BC, HA_Port_Volt_CA;
static float HA_Port_Volt_AB_LATCH, HA_Port_Volt_BC_LATCH, HA_Port_Volt_CA_LATCH;
static float HA_Port_Current_A, HA_Port_Current_B, HA_Port_Current_C;
static float HA_Grid_Current_A, HA_Grid_Current_B, HA_Grid_Current_C;
static float HA_Grid_Current_A_LATCH, HA_Grid_Current_B_LATCH, HA_Grid_Current_C_LATCH;
static float LPF_5KHZ_X_Volt_A_1, LPF_5KHZ_Y_Volt_A_1;
static float LPF_5KHZ_X_Volt_B_1, LPF_5KHZ_Y_Volt_B_1;
static unsigned short HA_PM_A1 = 1U, HA_PM_A2 = 0U, HA_PM_B1 = 0U, HA_PM_B2 = 0U, HA_PM_C1 = 0U, HA_PM_C2 = 0U;
static unsigned short HA_IG_Log_A, HA_IG_Log_B, HA_IG_Log_C;
static unsigned short HA_DC_Log_A, HA_DC_Log_B, HA_DC_Log_C;
static unsigned char HVAC_RealParameter_N;
static float HVAC_RealVoltage_Sum[3], HVAC_RealCurrent_Sum[3], HVAC_RealGridCurrent_Sum[3];
static float HA_PortP_LF_1, HA_PortP_LF_2, HA_PortQ_LF_1, HA_PortQ_LF_2;
static float Sample_UP_20kHz = 1.0F, Sample_DOWN_20kHz = 0.0F;
typedef struct { float PORT_V_A, PORT_V_B, PORT_V_C, PORT_I_A, PORT_I_B, PORT_I_C, I_A_2, I_B_2, I_C_2, PORT_P, PORT_Q, PORT_PF; } MEASURE;
static MEASURE PORT_RealTime_DATA;
typedef struct { float voltage_alpha_v, voltage_beta_v, current_alpha_a, current_beta_a; unsigned short ready; } SINGLE;
static SINGLE g_sst_single_phase_status;
static struct { struct { unsigned short Port_UdcOK; } Bbits; } PortRunFLAG;
static float SVG_Udc_A_Ave, SVG_Udc_B_Ave, SVG_Udc_C_Ave, SVG_Udc_Ave;
static float SVG_Udc_Ave_temp, SVG_Udc_Ave_temp_last;
static unsigned short Samplenoise_udc;
static struct { float H_Filter_BUF[8]; } Va_Ave_H2_100_Filter, H2_100_Filter, Vb_Ave_H2_100_Filter, Vc_Ave_H2_100_Filter;
static struct { float H_Filter_BUF[8]; } Ave_H6_100_Filter, H6_100_Filter;
static void H_Filter_Memcopy(float *out, const float *in) { memcpy(out, in, sizeof(float) * 8U); }
static float BandH2_100_Filter(float x) { return x; }
static float BandH6_100_Filter(float x) { return x; }
static float UstOK_HVAC_Port = 680.0F, Ust_add = 20.0F;
static float Ust_HVAC_Port = 80.0F, VoltLevel_1 = 50.0F, VoltLevel_2 = 70.0F;
static unsigned short Port_Outer_Elec, Port_Abnormal, Port_LV2;
__PM_CTL_TYPE__
static XXG_PM_CTL Module_PM_CTL;
'''


def c_source(project: Path, phase_a_only: bool) -> str:
    adc = (project / "Source/04_App/02_Measurement/ADC_Sample.C").read_text(encoding="utf-8-sig")
    fpga = (project / "Source/02_Peripheral/00_FPGA/FpgaIo.c").read_text(encoding="utf-8-sig")
    hvac = (project / "Source/04_App/04_Control/HVAC_Strategy.C").read_text(encoding="utf-8-sig")
    registers = (project / "Source/02_Peripheral/00_FPGA/FpgaRegister.h").read_text(encoding="utf-8-sig")
    pm_type = re.search(r"typedef union _XXG_PM_CTL\b.*?}\s*XXG_PM_CTL\s*;", registers, re.S)
    if not pm_type:
        raise ValueError("missing real XXG_PM_CTL register layout")
    selected = "1" if phase_a_only else "0"
    parts = [PREFIX.replace("__PM_CTL_TYPE__", pm_type.group(0)),
             f"#define HVAC_online 1\n#define SST_BENCH_INPUT_SIM 0\n#define SST_PHASE_A_ONLY_TEST {selected}\n"]
    parts.append(extract_function(adc, "HVAC_RealTimeParameter"))
    parts.append(extract_function(adc, "Port_AD_Sample"))
    measurement = (project / "Source/04_App/02_Measurement/MeasurementService.c").read_text(encoding="utf-8-sig")
    parts.append(extract_function(measurement, "Port_RealTimeParameter"))
    parts.append(extract_function(fpga, "Module_Control_Write"))
    parts.append(extract_function(hvac, "SVG_Udc_Average"))
    parts.append(r'''
static void check_sample(void) {
    const short raw_a[] = {10000, 10000, -20000, 3000, -1000, 15000};
    const short raw_b[] = {-3000, 23000, -10000, 7000, 1000, -20000};
    float previous_a = 0.0F, previous_b = 0.0F;
    float output_a = 0.0F, output_b = 0.0F;
    memset(FPGA_Interface, 0, sizeof(FPGA_Interface));
    memset(Port_AD_Buf, 0, sizeof(Port_AD_Buf));
    HA_PT_Ratio = 1.7F;
    HA_CT_Ratio = 2.3F;
    LPF_5KHZ_X_Volt_A_1 = LPF_5KHZ_Y_Volt_A_1 = 0.0F;
    LPF_5KHZ_X_Volt_B_1 = LPF_5KHZ_Y_Volt_B_1 = 0.0F;
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].Avg_IG = 25;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].Avg_IG = -37;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].Avg_IG = 99;
    for (unsigned i = 0U; i < 6U; ++i) {
        const unsigned char latch = (unsigned char)(i % 2U);
        short *ad = latch ? FPGA_Interface[0].AD_LATCH : FPGA_Interface[0].AD;
        ad[3] = (short)(1000 - (short)(500 * i));
        ad[4] = 77;
        ad[6] = raw_a[i];
        ad[7] = raw_b[i];
        /* Cover A1 priority, A2 selection, and the unchanged no-mask fallback. */
        HA_PM_A1 = i % 3U == 1U ? 0U : 1U;
        HA_PM_A2 = i % 3U == 2U ? 0U : 1U;
        if (i == 5U) HA_PM_A1 = 0U;
        output_a = previous_a * 0.9567F + output_a * 0.04328F;
        output_b = previous_b * 0.9567F + output_b * 0.04328F;
        previous_a = (0.0079785F * (float)raw_a[i] + 0.15957F) * HA_PT_Ratio;
        previous_b = (0.0079678F * (float)raw_b[i] + 0.545795F) * HA_PT_Ratio;
        Port_AD_Sample(latch);
        assert(fabsf(HA_Port_Current_A - ((HA_PM_A1 == 0U && HA_PM_A2 != 0U) ? -3.7F : 2.5F)) < 1e-5F);
        if (latch) {
            assert(fabsf(HA_Grid_Current_A_LATCH - (0.0121963F * (float)ad[3] + 0.09757F) * HA_CT_Ratio) < 1e-4F);
            assert(fabsf(HA_Port_Volt_AB_LATCH - output_a) < 1e-4F);
        } else {
            assert(fabsf(HA_Grid_Current_A - (0.0121963F * (float)ad[3] + 0.09757F) * HA_CT_Ratio) < 1e-4F);
            assert(fabsf(HA_Port_Volt_AB - output_a) < 1e-4F);
        }
#if SST_PHASE_A_ONLY_TEST
        assert(fabsf(HA_Port_Current_B) < 1e-6F && fabsf(HA_Port_Current_C) < 1e-6F);
        assert(fabsf(HA_Port_Volt_B) < 1e-6F && fabsf(HA_Port_Volt_C) < 1e-6F);
        assert(fabsf(HA_Port_Volt_A - output_a) < 1e-4F);
        if (latch) {
            assert(fabsf(HA_Grid_Current_B_LATCH) < 1e-6F && fabsf(HA_Grid_Current_C_LATCH) < 1e-6F);
            assert(fabsf(HA_Port_Volt_BC_LATCH) < 1e-6F && fabsf(HA_Port_Volt_CA_LATCH) < 1e-6F);
        } else {
            assert(fabsf(HA_Grid_Current_B) < 1e-6F && fabsf(HA_Grid_Current_C) < 1e-6F);
            assert(fabsf(HA_Port_Volt_BC) < 1e-6F && fabsf(HA_Port_Volt_CA) < 1e-6F);
        }
#else
        assert(fabsf(HA_Port_Current_B - 9.9F) < 1e-5F);
        assert(fabsf(HA_Port_Volt_A - (2.0F * output_a + output_b) / 3.0F) < 1e-4F);
        assert(fabsf(HA_Port_Volt_B - (output_b - output_a) / 3.0F) < 1e-4F);
        assert(fabsf(HA_Port_Volt_C + (2.0F * output_b + output_a) / 3.0F) < 1e-4F);
#endif
    }
}
static void check_telemetry(float lag) {
    float previous_power = 0.0F, power_sum = 0.0F;
    float expected_q = 0.5F * 100.0F * 10.0F * sinf(lag) / 1000.0F;
    memset(&PORT_RealTime_DATA, 0, sizeof(PORT_RealTime_DATA));
    memset(HVAC_RealVoltage_Sum, 0, sizeof(HVAC_RealVoltage_Sum));
    memset(HVAC_RealCurrent_Sum, 0, sizeof(HVAC_RealCurrent_Sum));
    memset(HVAC_RealGridCurrent_Sum, 0, sizeof(HVAC_RealGridCurrent_Sum));
    HVAC_RealParameter_N = 0U;
    HA_PortP_LF_1 = HA_PortP_LF_2 = HA_PortQ_LF_1 = HA_PortQ_LF_2 = 0.0F;
    HA_Port_Volt_B = HA_Port_Volt_C = 0.0F;
    HA_Port_Current_B = HA_Port_Current_C = 0.0F;
    HA_Grid_Current_B = HA_Grid_Current_C = 0.0F;
    g_sst_single_phase_status.ready = 1U;
    for (unsigned i = 0U; i < 200U; ++i) {
        const float angle = 6.28318530718F * (float)i / 200.0F;
        HA_Port_Volt_A = 100.0F * cosf(angle);
        HA_Port_Current_A = 10.0F * cosf(angle - lag);
        HA_Grid_Current_A = 20.0F * cosf(angle - lag);
        /* Deliberately different from AN: single-phase RMS must ignore AB/BC/CA. */
        HA_Port_Volt_AB = 300.0F * cosf(angle);
        HA_Port_Volt_BC = 400.0F * cosf(angle);
        HA_Port_Volt_CA = 500.0F * cosf(angle);
        g_sst_single_phase_status.voltage_alpha_v = HA_Port_Volt_A;
        g_sst_single_phase_status.voltage_beta_v = 100.0F * sinf(angle);
        g_sst_single_phase_status.current_alpha_a = HA_Port_Current_A;
        g_sst_single_phase_status.current_beta_a = 10.0F * sinf(angle - lag);
        HVAC_RealTimeParameter();
        assert(fabsf(PORT_RealTime_DATA.PORT_P - previous_power / 1000.0F) < 1e-5F);
        previous_power = HA_Port_Volt_A * HA_Port_Current_A;
        if (i != 0U) power_sum += PORT_RealTime_DATA.PORT_P;
#if SST_PHASE_A_ONLY_TEST
        if (i != 0U) assert(fabsf(PORT_RealTime_DATA.PORT_Q - expected_q) < 1e-5F);
#endif
    }
    /* Existing P/Q filters use the previous input; flush one sample before checking units. */
    HVAC_RealTimeParameter();
    power_sum += PORT_RealTime_DATA.PORT_P;
    assert(fabsf(power_sum / 200.0F - 0.5F * cosf(lag)) < 1e-5F);
    Port_RealTimeParameter();
    assert(fabsf(PORT_RealTime_DATA.PORT_I_A - 10.0F / sqrtf(2.0F)) < 1e-4F);
    assert(fabsf(PORT_RealTime_DATA.I_A_2 - 20.0F / sqrtf(2.0F)) < 1e-4F);
#if SST_PHASE_A_ONLY_TEST
    assert(fabsf(PORT_RealTime_DATA.PORT_V_A - 100.0F / sqrtf(2.0F)) < 1e-4F);
    assert(fabsf(PORT_RealTime_DATA.PORT_V_B) < 1e-5F && fabsf(PORT_RealTime_DATA.PORT_V_C) < 1e-5F);
    assert(fabsf(PORT_RealTime_DATA.PORT_I_B) < 1e-5F && fabsf(PORT_RealTime_DATA.PORT_I_C) < 1e-5F);
    assert(fabsf(PORT_RealTime_DATA.PORT_Q - expected_q) < 1e-5F);
    g_sst_single_phase_status.ready = 0U;
    HVAC_RealTimeParameter();
    HVAC_RealTimeParameter();
    assert(fabsf(PORT_RealTime_DATA.PORT_Q) < 1e-5F);
#else
    assert(fabsf(PORT_RealTime_DATA.PORT_V_A - 300.0F / sqrtf(2.0F)) < 1e-4F);
    assert(fabsf(PORT_RealTime_DATA.PORT_V_B - 400.0F / sqrtf(2.0F)) < 1e-4F);
    assert(fabsf(PORT_RealTime_DATA.PORT_V_C - 500.0F / sqrtf(2.0F)) < 1e-4F);
#endif
}
static void check_control_and_udc(void) {
    const unsigned short commands[] = {0U, 1U, 16U, 17U, 0xFFFFU};
    const unsigned bcs[] = {7U, 8U, 11U, 12U};
    HA_PM_A1 = HA_PM_A2 = 1U;
    HA_PM_B1 = HA_PM_B2 = HA_PM_C1 = HA_PM_C2 = 0U;
    for (unsigned i = 0U; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        Module_PM_CTL.PM_CTL16 = commands[i];
        for (unsigned n = 0U; n < 4U; ++n) FPGA_Interface[bcs[n]].PM_CTL.PM_CTL16 = 0xFFFFU;
        Module_Control_Write();
        assert(FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM_CTL.PM_CTL16 == commands[i]);
        assert(FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM_CTL.PM_CTL16 == commands[i]);
        for (unsigned n = 0U; n < 4U; ++n) {
#if SST_PHASE_A_ONLY_TEST
            assert(FPGA_Interface[bcs[n]].PM_CTL.PM_CTL16 == (commands[i] & 0xFFEEU));
#else
            assert(FPGA_Interface[bcs[n]].PM_CTL.PM_CTL16 == 0xFFFFU);
#endif
        }
    }
    HA_PM_B1 = HA_PM_B2 = HA_PM_C1 = HA_PM_C2 = 1U;
    Module_PM_CTL.PM_CTL16 = 0xFFFFU;
    Module_Control_Write();
    for (unsigned n = 0U; n < 4U; ++n) {
#if SST_PHASE_A_ONLY_TEST
        assert(FPGA_Interface[bcs[n]].PM_CTL.PM_CTL16 == 0xFFEEU);
#else
        assert(FPGA_Interface[bcs[n]].PM_CTL.PM_CTL16 == 0xFFFFU);
#endif
    }
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].Avg_DC = 240;
    HA_PM_A1 = 1U; HA_PM_A2 = HA_PM_B1 = HA_PM_B2 = HA_PM_C1 = HA_PM_C2 = 0U;
    SVG_Udc_Average();
#if SST_PHASE_A_ONLY_TEST
    assert(fabsf(SVG_Udc_Ave - 240.0F) < 1e-5F);
    assert(fabsf(SVG_Udc_B_Ave) < 1e-5F && fabsf(SVG_Udc_C_Ave) < 1e-5F);
    assert(PortRunFLAG.Bbits.Port_UdcOK == 0U);
    HA_PM_A2 = 1U;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].Avg_DC = 300;
    SVG_Udc_Average();
    assert(fabsf(SVG_Udc_Ave - 270.0F) < 1e-5F);
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].Avg_DC = 680;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].Avg_DC = 680;
    SVG_Udc_Average();
    assert(PortRunFLAG.Bbits.Port_UdcOK == 1U);
    assert(fabsf(SVG_Udc_Ave - 680.0F) < 1e-5F);
    HA_PM_A1 = HA_PM_A2 = 0U;
    SVG_Udc_Average();
    assert(fabsf(SVG_Udc_Ave) < 1e-5F);
    assert(PortRunFLAG.Bbits.Port_UdcOK == 0U);
#else
    /* Original no-module B/C path remains finite and keeps its old 500 V clamp. */
    assert(fabsf(SVG_Udc_Ave - 500.0F) < 1e-5F);
    assert(PortRunFLAG.Bbits.Port_UdcOK == 0U);
#endif
}
int main(void) {
    check_sample();
    check_telemetry(0.5235987756F);
    check_telemetry(-0.5235987756F);
    check_control_and_udc();
    puts("PASS: single-phase real I/O branches");
    return 0;
}
''')
    return "\n".join(parts)


def run(project: Path, compiler: str, phase_a_only: bool) -> None:
    with tempfile.TemporaryDirectory(prefix="sst_single_phase_io_") as folder:
        root = Path(folder)
        source = root / "single_phase_io.c"
        binary = root / "single_phase_io.exe"
        source.write_text(c_source(project, phase_a_only), encoding="utf-8")
        command = [compiler, "-std=c99", "-O0", "-Wall", "-Wextra", "-Werror",
                   "-Wno-unused-variable", "-Wno-unused-function",
                   str(source), "-lm", "-o", str(binary)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(f"single-phase macro={int(phase_a_only)} compile failed:\n{result.stdout}{result.stderr}")
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        if result.returncode:
            raise SystemExit(f"single-phase macro={int(phase_a_only)} test failed:\n{result.stdout}{result.stderr}")
        print(f"macro={int(phase_a_only)}: {result.stdout.strip()}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=ROOT)
    parser.add_argument("--cc", default=shutil.which("gcc"))
    args = parser.parse_args()
    if not args.cc:
        raise SystemExit("GCC not found; pass --cc")
    run(args.project, args.cc, True)
    run(args.project, args.cc, False)
    print("PASS: AN conversion, A current, single-phase telemetry, B/C PWM isolation, and A-only Udc")


if __name__ == "__main__":
    main()
