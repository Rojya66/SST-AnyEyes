"""Compile real maintenance + parameter + command sources against fake hardware.

The real legacy main.H supplies DPRAM layout; hardware register includes are
removed in temporary copies only. The test uses complete CRC16 wire frames.
"""
from pathlib import Path
import json
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "Source/04_App"


def read(path):
    return path.read_text(encoding="utf-8-sig")


def main():
    cc = shutil.which("gcc")
    if not cc:
        raise SystemExit("GCC required")
    sources = [APP / "01_FuncCode/SstParams.c",
               APP / "03_PcDebug/MaintenanceProtocol.C",
               APP / "04_Control/SstCommand.c"]
    code = "\n".join(re.sub(r'^\s*#include\s*"[^"\n]+"\s*$', "", read(p),
                            flags=re.M) for p in sources)
    legacy = read(APP / "00_Config/main.H")
    # Keep actual types and structures. Define its data externs as test storage.
    declarations = re.findall(r'^extern\s+([^;\n]+);', legacy, flags=re.M)
    definitions = "\n".join(x + ";" for x in declarations if "(" not in x)
    legacy = re.sub(r'^#include "soc_C6748.h".*$', "", legacy, flags=re.M)
    control = read(APP / "04_Control/ChbDabControl.c")
    defaults = "\n".join(re.search(r"volatile " + kind + r" " + name +
            r"\s*=\s*\{.*?\};", control, re.S).group(0) for kind, name in
            [("CHB_DAB_COMMAND", "g_chb_dab_command"),
             ("CHB_DAB_PARAMETERS", "g_chb_dab_params")])
    remote = re.search(r'void ChbDabControl_ApplyRemoteCommand\(.*?\n}', control,
                       re.S).group(0)
    template = read(Path(__file__).with_suffix(".c"))
    with tempfile.TemporaryDirectory(prefix="sst_maintenance_") as directory:
        work = Path(directory)
        for folder in ("00_Config", "01_FuncCode", "03_PcDebug", "04_Control", "08_Fault"):
            for p in (APP / folder).glob("*"):
                if p.suffix.lower() in (".h", ".hpp"):
                    shutil.copy2(p, work / p.name)
        (work / "main.h").write_text(legacy, encoding="utf-8")
        unit = work / "maintenance.c"
        unit.write_text(template.replace("/* LEGACY_GLOBALS */", definitions)
                        .replace("/* DEFAULTS */", defaults)
                        .replace("/* REMOTE_APPLY */", remote)
                        .replace("/* IMPLEMENTATION */", code), encoding="utf-8")
        reports = []
        evidence = ROOT / "build"
        evidence.mkdir(exist_ok=True)
        variants = [("remote_anyeyes", ["SST_SIMULATOR_MODE=0"]),
                    ("remote_vofa", ["SST_SIMULATOR_MODE=0", "SST_DEBUG_PROTOCOL=2"]),
                    ("sim_anyeyes", []),
                    ("dpram_anyeyes", ["SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=1"])]
        for name, defines in variants:
            binary = work / (name + ".exe")
            command = [cc, "-std=c99", "-O2", "-Wall", "-Wextra",
                       "-DSST_ENABLE_MAINTENANCE=1", *["-D"+d for d in defines],
                       str(unit), "-lm", "-o", str(binary)]
            result = subprocess.run(command, capture_output=True, text=True)
            report = dict(variant=name, compile_status=result.returncode,
                          compile_output=result.stdout + result.stderr)
            reports.append(report)
            if result.returncode:
                (evidence / "maintenance_result.json").write_text(
                    json.dumps(reports, indent=2), encoding="utf-8")
                raise SystemExit(result.stdout + result.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            report.update(run_status=result.returncode, output=result.stdout+result.stderr)
            (evidence / "maintenance_result.json").write_text(
                json.dumps(reports, indent=2), encoding="utf-8")
            if result.returncode:
                raise SystemExit(name+": "+result.stdout+result.stderr)
            print(name+":\n"+result.stdout, end="")


if __name__ == "__main__":
    main()
