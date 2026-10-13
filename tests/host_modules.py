"""Temporary host sources: preserve module boundaries and replace hardware only."""
from pathlib import Path
import re
import shutil
import subprocess

HARDWARE_HEADERS = {"main.h", "soc_C6748.h", "hw_types.h", "hw_uart.h", "uart.h", "gpio.h", "EEProm.H",
                    "MeasurementData.h", "PortStatusData.h"}


def read(path):
    return path.read_text(encoding="utf-8-sig")


def host_source(path):
    return re.sub(r'^\s*#\s*include\s*"([^"\n]+)"[^\n]*$',
                  lambda match: "" if match[1] in HARDWARE_HEADERS else match[0],
                  read(path), flags=re.MULTILINE)


def prepare_headers(project, work):
    for path in (project / "Source").rglob("*"):
        if path.suffix.lower() in (".h", ".hpp") and not any(
                part in ("00_Device", "01_Drive") for part in path.parts):
            shutil.copy2(path, work / path.name)
    shutil.copy2(Path(__file__).with_name("host_protocol_io.h"), work / "host_protocol_io.h")


def protocol_modules(project):
    app = project / "Source/04_App"
    sources = [app / "01_FuncCode/SstParams.c", app / "03_PcDebug/SstDebug.c",
               app / "03_PcDebug/Comm2PcDebug.cpp", app / "03_PcDebug/vofa.c"]
    additions = [app / "01_FuncCode/SstParameterTable.c", app / "01_FuncCode/SstParameterStore.c",
                 app / "01_FuncCode/SstLegacyParameters.c",
                 app / "03_PcDebug/PcDebugDispData.c", app / "04_Control/SstCommand.c",
                 project / "Source/02_Peripheral/03_Serial/SstSerial.c"]
    return sources + [path for path in additions if path.exists()]


def compile_modules(compiler, paths, work, defines, *, exclude=()):
    objects = []
    for path in paths:
        if path.name in exclude:
            continue
        unit = work / (path.name + ".host.cpp")
        unit.write_text(host_source(path), encoding="utf-8")
        obj = work / (path.name + ".o")
        command = [compiler, "-std=c++11", "-O2", "-Wall", "-Wextra",
                   "-Wno-misleading-indentation", *["-D" + value for value in defines],
                   "-include", str(work / "host_protocol_io.h"), "-c", str(unit), "-o", str(obj)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(path.name + " host compilation failed:\n" + result.stdout + result.stderr)
        objects.append(obj)
    return objects
