"""Compile original and migrated control implementations with identical fake I/O.

Only local include directives are replaced in temporary translation units; the
control function bodies and constants are compiled unmodified. Every observable
status field, FPGA write value and fake output action contributes to each trace.
The historical trace projects the four original reference/DAB FPGA fields;
the repaired DC_Log writer is verified independently on every TAD instead of
comparing it with the original isolated source's omitted writer. No target
files are modified. Host results do not prove embedded ISR timing.
"""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from git_baseline import materialize
from control_sources import control_source


def _support_translation_unit(project: Path | None) -> tuple[str, list[Path]]:
    """Return the host PI support source and headers used by the migrated unit.

    The original host harness inlined only ``ChbDabControl.c``.  The PI
    refactor deliberately moves state and execution into ``SstPi.c``; copying
    that small translation unit here keeps the regression test coupled to the
    same implementation without teaching the fake target headers about CCS.
    """
    if project is None:
        return "", []
    source_path = project / "Source/01_ComLib/04_Pi/SstPi.c"
    header_path = project / "Source/01_ComLib/04_Pi/SstPi.h"
    if not source_path.is_file():
        return "", []
    support = source_path.read_text(encoding="utf-8-sig")
    support = re.sub(r'^\s*#\s*include\s*"[^"\n]+"\s*$', "", support,
                     flags=re.MULTILINE)
    config = project / "Source/04_App/08_Fault/ProtectionConfig.h"
    return '#include "ProtectionConfig.h"\n#include "SstPi.h"\n' + support, [header_path, config]


def run(source: Path, header: Path, config: Path, compiler: str, template: str,
        support_project: Path | None = None) -> str:
    with tempfile.TemporaryDirectory(prefix="sst_control_regression_") as folder:
        work = Path(folder)
        shutil.copy2(header, work / "ChbDabControl.h")
        if support_project is not None:
            shutil.copy2(support_project / "Source/04_App/04_Control/ChbDabControlInternal.h",
                         work / "ChbDabControlInternal.h")
        shutil.copy2(config, work / "ChbBuildConfig.h")
        support_source, support_headers = _support_translation_unit(support_project)
        for support_header in support_headers:
            shutil.copy2(support_header, work / support_header.name)
        if support_project is not None:
            implementation = control_source(support_project)
        else:
            implementation = re.sub(r'^\s*#\s*include\s*"[^"\n]+"\s*$', "",
                                    source.read_text(encoding="utf-8-sig"), flags=re.MULTILINE)
        # SstPi.c must precede the control source because the latter references
        # the public PI instances and step functions from its header.
        implementation = support_source + "\n" + implementation
        code = template.replace("/* INSERT_CONTROL_IMPLEMENTATION */", implementation)
        unit = work / "control_test.c"
        unit.write_text(code, encoding="utf-8")
        binary = work / "control_test.exe"
        command = [compiler, "-std=c99", "-O2", "-Wall", "-Wextra",
                   "-DSST_SIMULATOR_MODE=1", "-DSST_ENABLE_DPRAM=0",
                   "-DSST_BENCH_INPUT_SIM=0", "-DSST_PHASE_A_ONLY_TEST=0",
                   "-DSST_LOW_VOLTAGE_100V_TEST=0", str(unit),
                   "-lm", "-o", str(binary)]
        if support_project is not None:
            command.insert(1, "-DSST_HOST_EXPECT_DC_LOG_UPDATES=1")
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
    saved_baseline = None
    if not (args.baseline / 'ChbDabControl.c').is_file():
        saved_baseline = tempfile.TemporaryDirectory(prefix='sst_git_control_baseline_')
        args.baseline = Path(saved_baseline.name)
        revision = materialize(args.project,args.baseline)
        print('BASELINE: immutable pre-framework Git commit '+revision+'; external original SST directory unavailable')
    else:
        print('BASELINE: external original SST source '+str(args.baseline))
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
            support_project = args.project if source == args.project / "Source/04_App/04_Control/ChbDabControl.c" else None
            results.append(run(source, header, config, args.cc, template, support_project))
    except subprocess.CalledProcessError as error:
        raise SystemExit(f"Regression compile/run failed:\n{error.stdout}\n{error.stderr}")
    # The first two scenarios exercise only open-loop and independent DAB gate
    # behavior. Their per-step observed state/reference/duty/output snapshots
    # remain a strict compatibility contract. The event-inclusive hash remains
    # diagnostic: the repaired atomic gate transaction deliberately replaces
    # legacy PWMPluse calls with bounded Module_* commits. The C harness checks
    # gate effects, inverse legacy flags and critical-region use independently.
    # Closed
    # loop scenarios intentionally change with the shared anti-windup PI,
    # measured-bus limit, and low-voltage startup defaults; they retain their
    # assertions in control_regression.c and are reported for review below.
    scenario_names = ("open_loop_and_stop_priority", "dab_only_and_independent_gates")
    def scenario_lines(output: str) -> dict[str, str]:
        lines = {}
        for line in output.splitlines():
            name = line.split(" ", 1)[0] if line else ""
            if name:
                lines[name] = line
        return lines

    before = scenario_lines(results[0])
    after = scenario_lines(results[1])
    def compatibility_projection(line: str | None) -> dict[str, str]:
        if not line:
            return {}
        fields = dict(part.split("=", 1) for part in line.split()[1:] if "=" in part)
        return {field: fields.get(field, "missing") for field in ("steps", "snapshot", "state", "fault")}

    mismatches = [name for name in scenario_names
                  if compatibility_projection(before.get(name)) != compatibility_projection(after.get(name))]
    if mismatches:
        import difflib
        print("".join(difflib.unified_diff(results[0].splitlines(True),
              results[1].splitlines(True), fromfile="baseline", tofile="migrated")))
        raise SystemExit("FAIL: unchanged open-loop/DAB observation snapshots differ: " + ", ".join(mismatches))
    sorting_name = "dc_log_faststep"
    sorting_expected = ("dc_log_faststep phases=3 cards=6 combinations=4 cycles=2 "
                        "states=stopped/running reserved=preserved")
    if after.get(sorting_name) != sorting_expected:
        raise SystemExit("FAIL: migrated three-phase FastStep DC_Log repair assertions did not run")
    changed = [name for name in after if name not in scenario_names and name != sorting_name]
    print("INTENTIONAL CONTROL TRACE CHANGES: " + (", ".join(changed) if changed else "none"))
    print(results[1], end="")
    print(f"PASS: {len(scenario_names)} open-loop/DAB per-step observation snapshots match exactly; "
          f"{len(changed)} closed-loop/protection scenarios pass assertions; "
          "six-card DC_Log repair passes independent per-TAD assertions; "
          f"SHA256={hashlib.sha256(results[1].encode()).hexdigest()}")


if __name__ == "__main__":
    main()
