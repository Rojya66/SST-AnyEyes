"""Execute real startup/scheduler/ISR C units with fake peripheral primitives.

Production sources and declarations are compiled as separate translation units.
Only HWREG/HWREGH and hardware/service callees are replaced. This proves tested
call order and branch effects; it does not emulate C6748 latency or bitfield ABI.
"""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    "Source/03_System/01_OS/SystemInit.c",
    "Source/03_System/01_OS/TaskSlice.c",
    "Source/03_System/01_OS/CpuTask.c",
    "Source/03_System/02_ISR/Redef_Interrupt.c",
    "Source/03_System/01_OS/SoftTimeData.c",
    "Source/04_App/04_Control/ControlData.c",
    "Source/04_App/04_Control/PortStatusData.c",
    "Source/04_App/02_Measurement/MeasurementData.c",
    "Source/04_App/08_Fault/ProtectionData.c",
    "Source/04_App/10_Host/DpramHostData.c",
    "Source/02_Peripheral/00_FPGA/FpgaRegister.c",
    "Source/02_Peripheral/00_FPGA/Ft3RecorderData.c",
]


def main():
    compiler = shutil.which("gcc")
    if not compiler:
        raise SystemExit("gcc required for firmware execution trace checks")
    output = ROOT / "build/firmware_trace"
    output.mkdir(parents=True, exist_ok=True)
    # Shadow only the absolute MMIO dereference header. Peripheral constants,
    # custom layouts, declarations, conditions and complete bodies stay real.
    (output / "hw_types.h").write_text(
        "#ifndef SST_TRACE_HW_TYPES_H\n#define SST_TRACE_HW_TYPES_H\n"
        "#include <stdint.h>\n"
        "volatile uint32_t *TraceRegister32(uintptr_t address);\n"
        "volatile uint16_t *TraceRegister16(uintptr_t address);\n"
        "#define HWREG(address) (*TraceRegister32((uintptr_t)(address)))\n"
        "#define HWREGH(address) (*TraceRegister16((uintptr_t)(address)))\n"
        "#endif\n", encoding="utf-8")
    includes = [output, ROOT] + [path for path in sorted((ROOT / "Source").rglob("*")) if path.is_dir()]
    # soc_C6748's directory comes before conflicting platform headers.
    includes.insert(1, ROOT / "Source/00_Device/include/c674x/c6748")
    template = Path(__file__).with_suffix(".c")
    variants = [("default", []),
                ("simulator", ["SST_ENABLE_DPRAM=0", "SST_SIMULATOR_MODE=1"]),
                ("serial", ["SST_ENABLE_DPRAM=0", "SST_SIMULATOR_MODE=0"]),
                ("eeprom", ["CHB_LOAD_EEPROM_PARAMETERS=1"])]
    rows = []
    for name, defines in variants:
        defines = ["SST_BENCH_INPUT_SIM=0", *defines]
        # "default.exe" triggers a local Windows executable-name compatibility
        # path before main. Prefix the fixture binary; compiled bytes unchanged.
        binary = output / ("sst_trace_" + name + ".exe")
        command = [compiler, "-std=c99", "-O0", "-g", "-Wno-unknown-pragmas",
                   "-Dc6748", *["-D" + define for define in defines],
                   *["-I" + str(path) for path in includes],
                   *[str(ROOT / source) for source in SOURCES], str(template), "-o", str(binary)]
        built = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
        (output / (name + "_compile.log")).write_text(built.stdout + built.stderr, encoding="utf-8")
        if built.returncode:
            raise SystemExit(name + " trace host compilation failed:\n" + built.stdout + built.stderr)
        run = subprocess.run([str(binary)], capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=15)
        (output / (name + ".log")).write_text(run.stdout + run.stderr, encoding="utf-8")
        if run.returncode:
            raise SystemExit(name + " execution trace failed (exit=" + str(run.returncode) + "):\n" + run.stdout + run.stderr)
        rows.append(dict(variant=name, defines=defines, status="pass", output=run.stdout.strip()))
        print(name + ": " + run.stdout.strip())
    report = dict(status="pass", variants=rows,
                  input_sha256={source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest() for source in SOURCES},
                  fixture_sha256=hashlib.sha256(template.read_bytes()).hexdigest(),
                  method="gcc separate real production startup/scheduler/ISR/data translation units, fake hardware/service callees",
                  limitations=["Fake hardware checks branch/state/call order, not real-time latency.",
                               "Host ABI is not used to validate C6748 wire layouts; TI map/layout checks cover that separately."])
    (output / "result.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
