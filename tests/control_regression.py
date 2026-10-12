"""Compile original and migrated control implementations with identical fake I/O.

Only local include directives are replaced in temporary translation units; the
control function bodies and constants are compiled unmodified. Every observable
status field, FPGA write value and fake output action contributes to each trace.
No target files are modified. Host results do not prove embedded ISR timing.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def run(source: Path, header: Path, config: Path, compiler: str, template: str) -> str:
    with tempfile.TemporaryDirectory(prefix="sst_control_regression_") as folder:
        work = Path(folder)
        shutil.copy2(header, work / "ChbDabControl.h")
        shutil.copy2(config, work / "ChbBuildConfig.h")
        implementation = source.read_text(encoding="utf-8-sig")
        implementation = re.sub(r'^\s*#\s*include\s*"[^"\n]+"\s*$', "", implementation,
                                flags=re.MULTILINE)
        code = template.replace("/* INSERT_CONTROL_IMPLEMENTATION */", implementation)
        unit = work / "control_test.c"
        unit.write_text(code, encoding="utf-8")
        binary = work / "control_test.exe"
        command = [compiler, "-std=c99", "-O2", "-Wall", "-Wextra", str(unit),
                   "-lm", "-o", str(binary)]
        subprocess.run(command, check=True, capture_output=True, text=True)
        return subprocess.run([str(binary)], check=True, capture_output=True, text=True,
                              timeout=20).stdout


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path,
                        default=Path(__file__).resolve().parents[2] / "Chb_DabPrim_vofa")
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--cc", default=shutil.which("gcc"))
    args = parser.parse_args()
    if not args.cc:
        raise SystemExit("GCC not found; pass --cc with a host GCC executable")
    template = Path(__file__).with_suffix(".c").read_text(encoding="utf-8")
    paths = [
        (args.baseline / "ChbDabControl.c", args.baseline / "ChbDabControl.h",
         args.baseline / "ChbBuildConfig.h"),
        (args.project / "Source/04_App/04_Control/ChbDabControl.c",
         args.project / "Source/04_App/04_Control/ChbDabControl.h",
         args.project / "Source/04_App/00_Config/ChbBuildConfig.h"),
    ]
    results = []
    try:
        for source, header, config in paths:
            results.append(run(source, header, config, args.cc, template))
    except subprocess.CalledProcessError as error:
        raise SystemExit(f"Regression compile/run failed:\n{error.stdout}\n{error.stderr}")
    if results[0] != results[1]:
        import difflib
        print("".join(difflib.unified_diff(results[0].splitlines(True),
              results[1].splitlines(True), fromfile="baseline", tofile="migrated")))
        raise SystemExit("FAIL: original/migrated behavioral traces differ")
    print(results[1], end="")
    print(f"PASS: {len(results[1].splitlines())} scenarios match original trace exactly; "
          f"SHA256={hashlib.sha256(results[1].encode()).hexdigest()}")


if __name__ == "__main__":
    main()
