"""Compile and run host scenarios for the optional A-only control path.

The test translation unit contains the production SOGI and CHB/DAB sources
verbatim.  Only quoted target-header includes are removed because the small
fake-I/O ABI is declared in ``control_regression.c``.  This verifies behavior
without claiming TI ABI, FPGA timing, or physical gate timing.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def strip_local_includes(source: str) -> str:
    return re.sub(r'^[ \t]*#[ \t]*include[ \t]*"[^"\n]+"[ \t]*$', "", source,
                  flags=re.MULTILINE)


def run(project: Path, compiler: str) -> str:
    tests = project / "tests"
    source_dir = project / "Source/04_App"
    with tempfile.TemporaryDirectory(prefix="sst_single_phase_") as folder:
        work = Path(folder)
        for name, path in (
            ("ChbBuildConfig.h", source_dir / "00_Config/ChbBuildConfig.h"),
            ("ChbDabControl.h", source_dir / "04_Control/ChbDabControl.h"),
            ("SstSinglePhase.h", source_dir / "04_Control/SstSinglePhase.h"),
        ):
            shutil.copy2(path, work / name)
        prefix = (tests / "control_regression.c").read_text(encoding="utf-8-sig")
        prefix = prefix.split("/* INSERT_CONTROL_IMPLEMENTATION */", 1)[0]
        sogi = strip_local_includes(
            (source_dir / "04_Control/SstSinglePhase.c").read_text(encoding="utf-8-sig")
        )
        control = strip_local_includes(
            (source_dir / "04_Control/ChbDabControl.c").read_text(encoding="utf-8-sig")
        )
        body = (tests / "single_phase_control.c").read_text(encoding="utf-8-sig")
        unit = work / "single_phase_control_test.c"
        # Keep assertion/compile diagnostics attached to their real source
        # files even though the host runner uses one translation unit.
        pieces = []
        for path, source in (
            (tests / "control_regression.c", prefix),
            (source_dir / "04_Control/SstSinglePhase.c", sogi),
            (source_dir / "04_Control/ChbDabControl.c", control),
            (tests / "single_phase_control.c", body),
        ):
            if path.name == "SstSinglePhase.c":
                pieces.append('#include "SstSinglePhase.h"\n')
            pieces.append(f'#line 1 "{path.as_posix()}"\n{source}\n')
        unit.write_text("".join(pieces), encoding="utf-8")
        binary = work / "single_phase_control_test.exe"
        command = [
            compiler,
            "-std=c99",
            "-O2",
            "-Wall",
            "-Wextra",
            "-DSST_SIMULATOR_MODE=1",
            "-DSST_ENABLE_DPRAM=0",
            "-DSST_BENCH_INPUT_SIM=0",
            "-DSST_PHASE_A_ONLY_TEST=1",
            str(unit),
            "-lm",
            "-o",
            str(binary),
        ]
        subprocess.run(command, check=True, capture_output=True, text=True)
        result = subprocess.run(
            [str(binary)], check=True, capture_output=True, text=True, timeout=30
        )
        return result.stdout


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--cc", default=shutil.which("gcc"))
    args = parser.parse_args()
    if not args.cc:
        raise SystemExit("GCC not found; pass --cc with a host GCC executable")
    try:
        output = run(args.project, args.cc)
    except subprocess.CalledProcessError as error:
        raise SystemExit(f"single-phase compile/run failed:\n{error.stdout}\n{error.stderr}")
    print(output, end="")


if __name__ == "__main__":
    main()
