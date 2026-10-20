"""Compile real bench input and control code against host hardware substitutes.

Only include directives are removed from temporary copies of implementation
files. The input generator, PLL, control state machine, PI, and software fault
function bodies are compiled unmodified. These tests verify input and control
behavior; they do not measure target ISR timing or model a power circuit.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from control_sources import control_source


def source_body(path: Path) -> str:
    """Read implementation while replacing project-local include boundaries."""
    source = path.read_text(encoding="utf-8-sig")
    return re.sub(r'^\s*#\s*include\s*"[^"\n]+"\s*$', "", source,
                  flags=re.MULTILINE)


def run(project: Path, compiler: str, enabled: bool, template: str) -> str:
    """Build one macro selection with the current, real implementation files."""
    with tempfile.TemporaryDirectory(prefix="sst_bench_inputs_") as folder:
        work = Path(folder)
        for relative in (
            "Source/04_App/00_Config/ChbBuildConfig.h",
            "Source/04_App/04_Control/ChbDabControl.h",
            "Source/04_App/04_Control/ChbDabControlInternal.h",
            "Source/04_App/02_Measurement/SstBenchInputs.h",
            "Source/01_ComLib/04_Pi/SstPi.h",
        ):
            shutil.copy2(project / relative, work / Path(relative).name)
        code = template.replace("/* INSERT_BENCH_IMPLEMENTATION */", source_body(
            project / "Source/04_App/02_Measurement/SstBenchInputs.c"))
        control = ("#include \"SstPi.h\"\n" + source_body(
            project / "Source/01_ComLib/04_Pi/SstPi.c") + "\n" + control_source(project)) if enabled else ""
        code = code.replace("/* INSERT_CONTROL_IMPLEMENTATION */", control)
        unit = work / "bench_test.c"
        unit.write_text(code, encoding="utf-8")
        binary = work / "bench_test.exe"
        command = [compiler, "-std=c99", "-O2", "-Wall", "-Wextra",
                   f"-DSST_BENCH_INPUT_SIM={int(enabled)}",
                   "-DSST_SIMULATOR_MODE=1", "-DSST_ENABLE_DPRAM=0",
                   "-DSST_PHASE_A_ONLY_TEST=0", "-DSST_LOW_VOLTAGE_100V_TEST=0",
                   str(unit), "-lm", "-o", str(binary)]
        subprocess.run(command, check=True, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
        return subprocess.run([str(binary)], check=True, capture_output=True,
                              text=True, timeout=20).stdout


def main() -> None:
    """Run enabled integration cases and disabled input preservation case."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--cc", default=shutil.which("gcc"))
    args = parser.parse_args()
    if not args.cc:
        raise SystemExit("GCC not found; pass --cc with a host GCC executable")
    template = Path(__file__).with_suffix(".c").read_text(encoding="utf-8")
    try:
        for enabled in (False, True):
            print(run(args.project, args.cc, enabled, template), end="")
    except subprocess.CalledProcessError as error:
        raise SystemExit(f"Bench compile/run failed:\n{error.stdout}\n{error.stderr}")
    print("PASS: real bench generator and control integration; macro-off inputs unchanged")


if __name__ == "__main__":
    main()
