"""Audit phase boundaries with TI preprocessing/assembly and exact contracts.

All compared function tokens are exact except the three named, uninitialized
float locals in SVG_Udc_Average. Reviewed common PI/safety changes have exact
before/after contracts instead of claiming macro=0 equivalence. Additional stdint tokens from
the new disabled header are outside executable function bodies.  ISR assembly
is compared in both macro modes; this does not measure complete ISR WCET.
"""
from __future__ import annotations

import argparse
import difflib
import importlib.util
import io
import json
import os
from pathlib import Path
import re
import tempfile
import zipfile

from bench_isolation import (compile_unit, digest, function, preprocess, read,
                             run, source_hash, tokens)
from realtime_audit import INTENTIONAL_CONTROL_FUNCTIONS
from source_equivalence import inventory

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "bcd8d1c554a3faa3e7010149b868be59f993ffc8"
CONTROL = "Source/04_App/04_Control/ChbDabControl.c"
ISR = "Source/03_System/02_ISR/Redef_Interrupt.c"
CHANGED = {
    CONTROL: ["Chb_WriteNormalizedReferences", "Chb_ConfigurationValid", "ChbDabControl_Init",
              "Chb_Min3", "Chb_RunClosedLoop", "Chb_CheckSoftwareProtection", "ChbDabControl_FastStep",
              "Chb_EnterState", "ChbDabControl_Service", "ChbDabControl_ForceFault", "Chb_DisableAllPwm",
              "Chb_GetClosedLoopIdReference", "Chb_RunOpenLoop", "Chb_ServiceOpenLoop", "Chb_SetAcInput",
              "Chb_SetLegacyStatus", "Chb_SetPrechargeBypass", "Chb_UpdatePll",
              "Chb_WrapAngle"],
    "Source/04_App/02_Measurement/ADC_Sample.C": ["HVAC_RealTimeParameter", "Port_AD_Sample"],
    "Source/04_App/02_Measurement/MeasurementService.c": ["Port_RealTimeParameter"],
    "Source/04_App/04_Control/HVAC_Strategy.C": ["SVG_Udc_Average"],
    "Source/02_Peripheral/00_FPGA/FpgaIo.c": ["Module_Control_Write"],
}
ISR_NAMES = ("FPGAIsr", "FAULTIsr", "Timer2Isr", "DPRAMIsr")


def switches(phase_a: int) -> dict[str, int]:
    return {"SST_BENCH_INPUT_SIM": 0, "SST_SIMULATOR_MODE": 1,
            "SST_ENABLE_DPRAM": 0, "CHB_LOAD_EEPROM_PARAMETERS": 0,
            "SST_PHASE_A_ONLY_TEST": phase_a, "SST_LOW_VOLTAGE_100V_TEST": 0}


def normalize_named_declarations(items: list[str]) -> tuple[list[str], int]:
    before = tokens("float tempAve1; float tempAve2; float tempAve3;")
    after = tokens("float tempAve1,tempAve2,tempAve3;")
    offsets = [index for index in range(len(items)) if items[index:index + len(before)] == before]
    if len(offsets) != 1:
        return items, len(offsets)
    index = offsets[0]
    return items[:index] + after + items[index + len(before):], len(offsets)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=ROOT)
    parser.add_argument("--baseline", default=BASELINE)
    parser.add_argument("--tool-root", type=Path, default=Path(os.environ.get(
        "SST_CGT_ROOT", "E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12")))
    args = parser.parse_args()
    project, cgt = args.project.resolve(), args.tool_root.resolve()
    destination = project / "build/single_phase_isolation"
    destination.mkdir(parents=True, exist_ok=True)
    report = {"baseline": args.baseline, "results": [], "compiler": str(cgt / "bin/cl6x.exe"),
              "comparison_defines": switches(0),
              "exceptions": ["SVG_Udc_Average: exactly float tempAve1,tempAve2,tempAve3 becomes three uninitialized float declarations; type/order/statements retained",
                             "Reviewed common PI/safety functions: exact all-branch Git-baseline/current contract plus TI macro0 preprocessed hashes; control_regression/common_control_safety validate intended behavior",
                             "SstSinglePhase.h adds stdint typedef/pragma declarations outside executable function bodies when macro=0"],
              "limitations": "No complete fast-control WCET, board latency, cache/EMIF stall or physical protection timing measurement."}

    def check(condition: bool, label: str, **details) -> None:
        report["results"].append(dict(name=label, passed=bool(condition), **details))
        print(("PASS " if condition else "FAIL ") + label, flush=True)

    try:
        start_hash = source_hash(project)
        report["firmware_input_sha256"] = start_hash
        revision = run(["git", "rev-parse", args.baseline + "^{commit}"], project).stdout.decode().strip()
        report["baseline_resolved_commit"] = revision
        archive = run(["git", "archive", "--format=zip", revision], project).stdout
        with tempfile.TemporaryDirectory(prefix="sst_single_phase_baseline_") as temporary:
            baseline = Path(temporary)
            with zipfile.ZipFile(io.BytesIO(archive)) as saved:
                saved.extractall(baseline)
            for relative, names in CHANGED.items():
                label = Path(relative).stem
                old = preprocess(baseline, relative, cgt, switches(0), destination / "baseline" / label)
                new = preprocess(project, relative, cgt, switches(0), destination / "macro0" / label)
                raw_new = new
                if relative == CONTROL:
                    contracts = {item["key"]: item for item in json.loads(
                        (project / "tests/refactor_contracts.json").read_text(encoding="utf-8"))["changes"]}
                    contract_revision = "ddaeb56f21a81a70cdb76085728f91b67dc3c7a0"
                    original_control = run(["git", "show", contract_revision + ":" + CONTROL], project).stdout.decode("utf-8-sig")
                    original_entities = inventory(original_control, CONTROL)
                    current_entities = inventory(read(project / CONTROL), CONTROL)
                for name in names:
                    before, after = function(old, name), function(new, name)
                    raw_same = before == function(raw_new, name)
                    if name in INTENTIONAL_CONTROL_FUNCTIONS:
                        key = "function:" + name
                        contract = contracts.get(key, {})
                        before_hashes = sorted(entity.sha256 for entity in original_entities if entity.key == key)
                        after_hashes = sorted(entity.sha256 for entity in current_entities if entity.key == key)
                        check(bool(before_hashes and after_hashes) and
                              contract.get("baseline_sha256s") == before_hashes and
                              contract.get("current_sha256s") == after_hashes and
                              "common_control_safety.py" in contract.get("validation", ""),
                              "TI macro0 reviewed PI/common-safety exact contract " + name,
                              baseline_preprocessed_sha256=digest(before),
                              current_preprocessed_sha256=digest(after),
                              contract_baseline=contract_revision,
                              baseline_sha256s=before_hashes, current_sha256s=after_hashes,
                              reason=contract.get("reason"), validation=contract.get("validation"))
                        continue
                    if name == "SVG_Udc_Average":
                        after, count = normalize_named_declarations(after)
                        check(count == 1, "exact three-local declaration exception occurs once")
                    check(before == after, "TI macro0 body " + name, raw_tokens_equal=raw_same,
                          baseline_sha256=digest(before), current_normalized_sha256=digest(after))

            # Macro0 no longer implies the old switch-output write: the new
            # output macro defaults off. Check both TI-preprocessed branches,
            # while switch_io.py exercises values/coil polarity exhaustively.
            relative = "Source/02_Peripheral/00_FPGA/FpgaIo.c"
            current_io = inventory(read(project / relative), relative)
            contract = json.loads((project / "tests/refactor_contracts.json").read_text(encoding="utf-8"))
            output_contract = next(item for item in contract["changes"] if item["key"] == "function:Port_IO_Out")
            check(output_contract["current_sha256s"] == sorted(e.sha256 for e in current_io
                  if e.key == "function:Port_IO_Out") and "switch_io.py" in output_contract["validation"],
                  "Port_IO_Out exact active implementation contract (intentional switch-output change)")
            for enabled in (0, 1):
                defines = {**switches(0), "SST_ENABLE_SWITCH_OUTPUT": enabled}
                selected = preprocess(project, relative, cgt, defines, destination / f"switch_output_{enabled}")
                body = function(selected, "Port_IO_Out")
                check((body == tokens("{}")) if enabled == 0 else
                      ("physical_output" in body and "FPGA_Interface" in body),
                      f"TI switch output macro={enabled} {'empty body' if not enabled else 'physical writer retained'}")

            timing_header = "Source/04_App/00_Config/SstControlConfig.h"
            for name in ("ControlTAD", "TAD_PLL"):
                expression = r'^\s*#\s*define\s+' + name + r'\s+([^\n]+)'
                before = re.search(expression, read(baseline / timing_header), re.MULTILINE)
                after = re.search(expression, read(project / timing_header), re.MULTILINE)
                check(bool(before and after) and tokens(before[1]) == tokens(after[1]),
                      "source timing macro unchanged " + name)

            spec = importlib.util.spec_from_file_location("single_phase_isr_helper", project / "tests/isr_binary_audit.py")
            helper = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(helper)
            assembly = {}
            old_isr = preprocess(baseline, ISR, cgt, switches(0), destination / "baseline/isr")
            for label, tree, phase in (("baseline", baseline, 0), ("macro0", project, 0), ("macro1", project, 1)):
                folder = destination / (label + "_assembly")
                result = compile_unit(tree, tree / ISR, cgt, switches(phase), folder, keep_asm=True)
                check(result.returncode == 0 and not re.search(rb"warning\s+#225", result.stdout),
                      "TI ISR compile " + label)
                assembly[label] = read(folder / "Redef_Interrupt.asm")
                if label == "baseline":
                    continue
                new_isr = preprocess(tree, ISR, cgt, switches(phase), destination / label / "isr")
                for name in ISR_NAMES:
                    check(function(old_isr, name) == function(new_isr, name),
                          "TI " + label + " ISR body " + name)
                    before = helper.routine(assembly["baseline"], name)
                    after = helper.routine(assembly[label], name)
                    check(before == after, "TI " + label + " ISR instructions/calls/stack " + name,
                          instruction_sha256=digest(after["instructions"]),
                          direct_calls=after["direct_calls"], stack=after["stack_usage"])
        check(start_hash == source_hash(project), "firmware inputs unchanged during audit")
    except Exception as error:
        check(False, "audit completed", error=type(error).__name__ + ": " + str(error))
    report["passed"] = bool(report["results"]) and all(row["passed"] for row in report["results"])
    (destination / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("REPORT " + str(destination / "result.json"))
    raise SystemExit(0 if report["passed"] else 1)


if __name__ == "__main__":
    main()
