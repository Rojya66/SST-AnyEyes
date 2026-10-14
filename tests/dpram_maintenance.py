"""Exercise the real parallel maintenance, parameter, command and storage modules."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from host_modules import prepare_headers, host_source


def main():
    project = Path(__file__).resolve().parents[1]
    app = project / "Source/04_App"
    compiler = shutil.which("gcc")
    if not compiler:
        raise SystemExit("Host GCC required for DPRAM maintenance checks")
    sources = [app / "10_Host/SstDpramMaintenance.c",
               project / "Source/02_Peripheral/01_DPRAM/SstDpramExtension.c",
               app / "01_FuncCode/SstLegacyParameters.c",
               app / "01_FuncCode/SstParams.c",
               app / "01_FuncCode/SstParameterTable.c",
               app / "01_FuncCode/SstParameterStore.c",
               app / "04_Control/SstCommand.c"]
    control = (app / "04_Control/ChbDabControl.c").read_text(encoding="utf-8-sig")
    defaults = "\n".join(re.search(r"volatile " + kind + r" " + name + r"\s*=\s*\{.*?\};",
                                  control, re.S).group(0)
                         for kind, name in [("CHB_DAB_COMMAND", "g_chb_dab_command"),
                                            ("CHB_DAB_PARAMETERS", "g_chb_dab_params")])
    template = Path(__file__).with_suffix(".c").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="sst_dpram_maintenance_") as directory:
        work = Path(directory)
        prepare_headers(project, work)
        # Replace external hardware declarations, retaining each real module's boundaries.
        (work / "ProtectionData.h").write_text(
            '#include <stdint.h>\nextern uint16_t OC_level_SET, OC_time_SET, OC_level_SET_L2, '
            'OC_time_SET_L2, OV_level_SET, OV_time_SET;\n'
            'extern float HA_L_Voltage_Precharge, HA_H_Voltage;\n', encoding="utf-8")
        (work / "SstBoardConfig.h").write_text("/* Hardware macros are supplied by dpram_hardware.h. */\n", encoding="utf-8")
        hardware = work / "dpram_hardware.h"
        hardware.write_text(
            '#include <stdint.h>\n#define VALIDFLAG 1U\n'
            'volatile uint16_t *HostHalfword(uint32_t address);\n'
            '#define HWREGH(address) (*HostHalfword((uint32_t)(address)))\n'
            'uint8_t r_eeprom_byte_read(uint8_t page,uint8_t offset);\n'
            'uint8_t r_eeprom_byte_write(uint8_t page,uint8_t offset,uint8_t value);\n',
            encoding="utf-8")
        units = []
        for path in sources:
            unit = work / (path.name + ".host.c")
            unit.write_text(host_source(path), encoding="utf-8")
            units.append(unit)
        unit = work / "dpram_maintenance_test.c"
        unit.write_text(template.replace("/* DEFAULTS */", defaults), encoding="utf-8")
        for name, defines in (
            ("disabled", ["SST_ENABLE_DPRAM=0", "SST_SIMULATOR_MODE=1"]),
            ("serial_owner", ["SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=0"]),
            ("simulator_dpram", ["SST_ENABLE_DPRAM=1", "SST_SIMULATOR_MODE=1"]),
            ("parallel_owner", ["SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=1"]),
        ):
            binary = work / (name + ".exe")
            command = [compiler, "-std=c99", "-O2", "-Wall", "-Wextra",
                       "-Wno-unknown-pragmas", "-Wno-misleading-indentation",
                       "-DSST_BENCH_INPUT_SIM=0", *["-D" + value for value in defines], "-I", str(work),
                       "-include", str(hardware), str(unit),
                       *[str(path) for path in units], "-o", str(binary)]
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode:
                raise SystemExit(name + " compilation failed:\n" + result.stdout + result.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=15)
            if result.returncode:
                raise SystemExit(name + " failed:\n" + result.stdout + result.stderr)
            print(name + ": " + result.stdout.strip())


if __name__ == "__main__":
    main()
