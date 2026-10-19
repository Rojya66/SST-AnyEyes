"""Compile the real background VOFA path and check its 12-channel wire map."""
from __future__ import annotations
import argparse
from pathlib import Path
import json
import re
import shutil
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[1]
# When staged under ignored build/, --project points to the real source checkout.
sys.path.insert(0, str(ROOT / "tests"))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=ROOT)
    parser.add_argument("--overlay", type=Path,
                        help="Read staged SstDebug.c/vofa.c without changing the checkout")
    args = parser.parse_args()
    sys.path.insert(0, str(args.project / "tests"))
    from host_modules import protocol_modules, prepare_headers, compile_modules
    cc = shutil.which("g++")
    if not cc:
        raise SystemExit("g++ is required")
    control = (args.project / "Source/04_App/04_Control/ChbDabControl.c").read_text(encoding="utf-8-sig")
    defaults = "\n".join(re.search(r"volatile " + kind + r" " + name + r"\s*=\s*\{.*?\};",
                                     control, re.S).group(0)
                         for kind, name in [("CHB_DAB_COMMAND", "g_chb_dab_command"),
                                            ("CHB_DAB_PARAMETERS", "g_chb_dab_params")])
    # Reuse only the fake hardware/globals and existing helpers; this entry point
    # does not assert obsolete high-voltage parameter defaults.
    scaffold = (args.project / "tests/protocol_regression.cpp").read_text(encoding="utf-8-sig")
    scaffold = scaffold.split("int main(void)", 1)[0]
    test = Path(__file__).with_suffix(".cpp").read_text(encoding="utf-8-sig")
    sources = protocol_modules(args.project)
    if args.overlay:
        sources = [args.overlay / p.relative_to(args.project)
                   if (args.overlay / p.relative_to(args.project)).exists() else p
                   for p in sources]
    defines = ["SST_DEBUG_PROTOCOL=2", "SST_SIMULATOR_MODE=1", "SST_ENABLE_DPRAM=0",
               "SST_BENCH_INPUT_SIM=0"]
    with tempfile.TemporaryDirectory(prefix="sst_vofa_telemetry_") as directory:
        work = Path(directory)
        prepare_headers(args.project, work)
        unit = work / "vofa_telemetry.cpp"
        unit.write_text(scaffold.replace("/* DEFAULTS */", defaults)
                        .replace("/* IMPLEMENTATION */", "") + test, encoding="utf-8")
        objects = compile_modules(cc, sources, work, defines)
        binary = work / "vofa_telemetry.exe"
        command = [cc, "-std=c++11", "-O2", "-static", "-Wall", "-Wextra",
                   "-Wno-unused-function", "-Wno-misleading-indentation",
                   *["-D" + x for x in defines], "-include", str(work / "host_protocol_io.h"),
                   str(unit), *[str(obj) for obj in objects], "-o", str(binary)]
        subprocess.run(command, check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if result.returncode:
            raise SystemExit(result.stdout + result.stderr)
        print(result.stdout.strip())
    evidence = args.project / "build/vofa_telemetry"
    evidence.mkdir(parents=True, exist_ok=True)
    (evidence / "result.json").write_text(json.dumps({"status": "pass",
        "output": result.stdout.strip(), "defines": defines,
        "limitations": "Fake UART checks bytes and scheduling, not RS485 electrical timing or same-TAD coherence."},
        indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
