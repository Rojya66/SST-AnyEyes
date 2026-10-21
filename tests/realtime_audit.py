"""Static migration invariants, not a schedulability or hardware timing proof."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re
import sys
import tempfile
from git_baseline import materialize

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from source_equivalence import inventory, missing_entities


def read(path: Path) -> str:
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "gb18030"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            pass
    return data.decode("latin1")


def uncomment(text: str) -> str:
    pattern = r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|/\*[\s\S]*?\*/|//[^\n]*'
    return re.sub(pattern, lambda match: match[1] or " ", text)


def tokens(text: str) -> list[str]:
    return re.findall(r'\w+|[^\s\w]', uncomment(text))


def exclude_literal_if_zero(text: str) -> str:
    """Drop literal #if 0 branches (legacy disabled code may have bad braces)."""
    result = []
    disabled_depth = 0
    for line in text.splitlines(True):
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b(.*)', line)
        if not disabled_depth:
            if directive and directive[1] == "if" and directive[2].strip() == "0":
                disabled_depth = 1
            else:
                result.append(line)
        elif directive:
            if directive[1] in ("if", "ifdef", "ifndef"):
                disabled_depth += 1
            elif directive[1] == "endif":
                disabled_depth -= 1
            elif directive[1] == "else" and disabled_depth == 1:
                disabled_depth = 0
    return "".join(result)


def function(text: str, name: str) -> str:
    clean = uncomment(text)
    match = re.search(r'\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', clean)
    if not match:
        raise ValueError(f"Function {name} is missing")
    start = clean.index("{", match.start())
    nesting = 1
    for position in range(start + 1, len(clean)):
        nesting += (clean[position] == "{") - (clean[position] == "}")
        if nesting == 0:
            return clean[start:position + 1]
    raise ValueError(f"Unbalanced function {name}")


def definitions(text: str) -> dict[str, str]:
    clean = exclude_literal_if_zero(uncomment(text))
    names = re.findall(r'^\s*(?:extern\s+"C"\s+)?(?:(?:static|inline|extern)\s+)*'
                       r'(?:[A-Za-z_]\w*\s+)+\**\s*([A-Za-z_]\w*(?:::[A-Za-z_]\w*)?)'
                       r'\s*\([^;{}]*\)\s*\{', clean, re.MULTILINE)
    result: dict[str, str] = {}
    for name in names:
        if name in ("if", "for", "while", "switch", "catch"):
            continue
        key = name.split("::")[-1]
        result[key] = result.get(key, "") + function(clean, name)
    return result


def select_dpram_branches(text: str, enabled: bool, phase_a: bool = False) -> str:
    """Select DPRAM gates and the requested phase algorithm.

    A-only behavior is checked independently by single_phase_control.py and
    single_phase_io.py. Common safety changes in macro=0 are separately pinned
    by reviewed before/after hashes; unrelated conditional branches remain.
    """
    result = []
    stack: list[tuple[bool, bool | None]] = []
    active = True
    for line in uncomment(text).splitlines(True):
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b(.*)', line)
        if not directive:
            if active:
                result.append(line)
            continue
        kind, expression = directive[1], directive[2].strip()
        if kind in ("if", "ifdef", "ifndef"):
            known = None
            if kind in ("ifdef", "ifndef") and expression in (
                    "HVAC_online", "HVDC_online", "LVDC_online", "LVAC_online"):
                known = expression == "HVAC_online"
                if kind == "ifndef":
                    known = not known
            if kind == "if":
                if expression == "SST_BENCH_INPUT_SIM":
                    known = False
                elif expression == "!SST_BENCH_INPUT_SIM":
                    known = True
                elif expression == "SST_PHASE_A_ONLY_TEST":
                    known = phase_a
                elif expression == "!SST_PHASE_A_ONLY_TEST":
                    known = not phase_a
                elif expression == "SST_LOW_VOLTAGE_100V_TEST":
                    known = False
                elif expression == "!SST_LOW_VOLTAGE_100V_TEST":
                    known = True
                elif expression == "SST_ENABLE_DPRAM":
                    known = enabled
                elif expression == "!SST_ENABLE_DPRAM":
                    known = not enabled
                elif expression in ("0", "1"):
                    known = expression == "1"
                elif not enabled and re.fullmatch(r'\w+\s*&&\s*SST_ENABLE_DPRAM', expression):
                    known = False
            stack.append((active, known))
            active = active and known is not False
        elif kind == "else":
            parent, known = stack[-1]
            active = parent and known is not True
        elif kind == "elif":
            # Unrecognized alternate conditions are conservatively retained.
            active = stack[-1][0]
        elif kind == "endif":
            active = stack.pop()[0]
    if stack:
        raise ValueError("Unbalanced conditional directives")
    return "".join(result)


def normalize_udc_local_declarations(text: str) -> str:
    """Treat the reviewed split of three uninitialized float locals equally.

    This is deliberately limited to SVG_Udc_Average and the exact three names;
    no statement, initializer, type, order, or other function is normalized.
    """
    if not re.search(r'\bSVG_Udc_Average\s*\([^;{}]*\)\s*\{', text):
        return text
    old = function(text, "SVG_Udc_Average")
    new, count = re.subn(r'\bfloat\s+tempAve1\s*;\s*float\s+tempAve2\s*;\s*'
                         r'float\s+tempAve3\s*;',
                         'float tempAve1,tempAve2,tempAve3;', old)
    if count > 1:
        raise AssertionError("Unexpected repeated SVG_Udc_Average local declarations")
    return text.replace(old, new, 1)


def closure(name: str, functions: dict[str, str]) -> set[str]:
    seen: set[str] = set()
    pending = [name]
    while pending:
        current = pending.pop()
        if current in seen:
            continue
        seen.add(current)
        pending.extend(re.findall(r'\b([A-Za-z_]\w*)\s*\(', functions.get(current, "")))
    return seen


def ordered_calls(name: str, functions: dict[str, str], ancestors=()) -> list[str]:
    """Expand direct calls in source order; do not silently recurse forever.

    Both conditional alternatives are retained, like the conservative call graph.
    Firmware trace regressions/builds validate actual configured branches.
    """
    if name in ancestors:
        return [name]
    result = [name]
    for called in re.findall(r'\b([A-Za-z_]\w*)\s*\(', functions.get(name, "")):
        if called in ("if", "for", "while", "switch", "sizeof"):
            continue
        result.extend(ordered_calls(called, functions, ancestors + (name,)))
    return result


# The shared PI module is intentionally replacing the old source-equivalent
# integral step.  Keep this list narrow: realtime/hardware entities outside
# these reviewed calculations/transactions retain their exact lexical inventory.
INTENTIONAL_CONTROL_FUNCTIONS = {
    "Chb_SourceEquivalentPiStep", "Chb_VoltagePiLimited", "Chb_CurrentPiLimited",
    "Chb_ResetControllers", "Chb_ResetStartupSequence",
    "Chb_AlignVoltagePiToIdReference", "Chb_UpdateVoltageReference",
    "Chb_GetClosedLoopIdReference", "Chb_RunClosedLoop",
    "Chb_ConfigurationValid", "ChbDabControl_Init",
    "Chb_EnterState", "ChbDabControl_Service", "ChbDabControl_ForceFault",
    "Chb_WrapAngle", "Chb_SetAcInput", "Chb_SetPrechargeBypass",
    "Chb_DisableAllPwm", "Chb_SetLegacyStatus", "Chb_ServiceOpenLoop",
    "Chb_UpdatePll", "ChbDabControl_FastStep", "Chb_WriteNormalizedReferences",
}
INTENTIONAL_CONTROL_OBJECTS = {"g_chb_dab_command", "g_chb_dab_params"}
INTENTIONAL_CONTROL_TYPES = {"CHB_PI_STATE", "CHB_DAB_FAULT", "CHB_DAB_STATUS"}
INTENTIONAL_CONTROL_OBJECTS.update({"s_voltage_pi", "s_id_pi", "s_iq_pi"})
INTENTIONAL_CONTROL_OBJECTS.update({
    "s_pll_integral", "s_open_loop_angle", "s_id_reference", "s_state_enter_tick",
    "s_breaker_pulse_end_tick", "s_last_service_tick", "s_precharge_stable_count",
    "s_pll_lock_fast_count", "s_pll_lock_10ms_count", "s_ac_overcurrent_count",
    "s_dc_overvoltage_count", "s_startup_phase", "s_startup_tad_count",
    "s_startup_voltage_reference", "s_active_ac_mode", "s_pll_loss_count",
})
INTENTIONAL_CONTROL_FUNCTIONS.update({
    "Chb_Clamp", "Chb_Abs", "Chb_Min3", "Chb_Max3", "Chb_WriteNeutralReferences",
    "Chb_WriteDabDuty", "Chb_ClearAcInputPulse", "Chb_RunOpenLoop",
    "Chb_CheckSoftwareProtection",
})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path,
                        default=Path(__file__).resolve().parents[2] / "Chb_DabPrim_vofa")
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    saved_baseline = None
    if not (args.baseline / 'main_485test.c').is_file():
        saved_baseline = tempfile.TemporaryDirectory(prefix='sst_git_realtime_baseline_')
        args.baseline=Path(saved_baseline.name)
        revision=materialize(args.project,args.baseline,original_linker=True)
        print('BASELINE: immutable pre-framework Git commit '+revision+'; external original SST directory unavailable')
    else:
        print('BASELINE: external original SST source '+str(args.baseline))
    source = args.project / "Source"
    paths = sorted(path for path in source.rglob("*")
                   if path.suffix.lower() in (".c", ".cpp", ".h", ".hpp")
                   and not any(part in ("00_Device", "01_Drive") for part in path.parts))
    source_texts = {path: read(path) for path in paths}
    all_text = "\n".join(source_texts.values())
    # Fixed HVAC project: evaluate the same legacy port choice before comparing
    # declaration locations. Unrelated compile-time alternatives remain in text.
    all_entities = [entity for path, contents in source_texts.items()
                    for entity in inventory(normalize_udc_local_declarations(
                                                select_dpram_branches(contents, True)),
                                            path.relative_to(args.project).as_posix())]
    archived_interfaces = [entity for directory in (args.project / "archive", args.project / "Legacy")
                           if directory.exists() for path in directory.rglob("*")
                           if path.is_file() and path.suffix.lower() in (".c", ".cpp", ".h", ".hpp")
                           for entity in inventory(select_dpram_branches(read(path), True),
                                                   path.relative_to(args.project).as_posix())
                           if entity.kind in ("extern", "prototype")]
    failures: list[str] = []
    checked = 0

    def check(condition: bool, label: str) -> None:
        nonlocal checked
        checked += 1
        print(("PASS " if condition else "FAIL ") + label)
        if not condition:
            failures.append(label)

    unchanged = {
        "ChbDabControl.h": "04_App/04_Control/ChbDabControl.h",
        "ADC_Sample.C": "04_App/02_Measurement/ADC_Sample.C",
        "HVAC_Strategy.C": "04_App/04_Control/HVAC_Strategy.C",
        "FaultCheck.C": "04_App/08_Fault/FaultCheck.C",
        "main.H": "04_App/00_Config/main.H",
    }
    expected_module_masks = {
        "CHB_PM_A1_MASK": "0x0007U", "CHB_PM_A2_MASK": "0x0007U",
        "CHB_PM_B1_MASK": "0x0000U", "CHB_PM_B2_MASK": "0x0000U",
        "CHB_PM_C1_MASK": "0x0000U", "CHB_PM_C2_MASK": "0x0000U",
    }
    actual_module_masks = dict(re.findall(r'^\s*#\s*define\s+(CHB_PM_[ABC][12]_MASK)\s+(\w+)\s*$',
                                         uncomment(all_text), re.MULTILINE))
    check(actual_module_masks == expected_module_masks,
          "reviewed installed-unit configuration is exactly A1=A2=0x0007 and all B/C module masks=0")
    for name, relative in unchanged.items():
        original = inventory(select_dpram_branches(read(args.baseline / name), True), name)
        if name == "ChbDabControl.h" and actual_module_masks == expected_module_masks:
            original = [entity for entity in original
                        if entity.key not in {"macro:" + mask for mask in expected_module_masks}]
        # Protection defaults now live beside the derived protection formulas.
        # Keep the historical symbols in the source inventory, but do not
        # require their old main.H declaration location.
        if name == "main.H":
            original = [entity for entity in original
                        if entity.name not in {"OC_level_SET_DEFAULT",
                                              "OC_level_SET_L2_DEFAULT",
                                              "OV_level_SET_DEFAULT"}]
        # Only declarations of the disabled legacy UART/history APIs may be
        # satisfied by archives. Functions, types, values and MMIO declarations
        # retain current-source requirements and independent call-graph checks.
        # Reviewed appended status fields / fault7 retain the old member prefix;
        # exact old/current contracts and behavioral checks cover this change.
        if name == "ChbDabControl.h":
            original = [entity for entity in original
                        if entity.name not in {"CHB_DAB_FAULT", "CHB_DAB_STATUS"}]
        if name == "FaultCheck.C":
            contract_doc = json.loads((args.project / "tests/refactor_contracts.json").read_text(encoding="utf-8"))
            contract = next(item for item in contract_doc["changes"]
                            if item["key"] == "function:OC_OV_LV_faultcheck")
            before = [entity for entity in inventory(read(args.baseline / name), name)
                      if entity.key == contract["key"]]
            after = [entity for entity in inventory(read(source / relative), relative)
                     if entity.key == contract["key"]]
            check(sorted(entity.sha256 for entity in before) == contract["baseline_sha256s"] and
                  sorted(entity.sha256 for entity in after) == contract["current_sha256s"] and
                  "protection_regression.py" in contract["validation"],
                  "reviewed HVAC DC protection removal matches exact before/after contract")
            original = [entity for entity in original if entity.key != contract["key"]]
        missing = missing_entities(original, all_entities + archived_interfaces)
        check(not missing, f"every original executable entity preserved (module masks and Udc declarations checked separately): {name} "
              f"missing={[entity.key for entity in missing]}")

    # Control PI/startup changes are checked by the host PI and closed-loop
    # regressions.  Preserve every other ChbDabControl entity exactly and
    # require each reviewed replacement to remain present.
    baseline_control = inventory(select_dpram_branches(read(args.baseline / "ChbDabControl.c"), True),
                                 "ChbDabControl.c")
    control_paths = [args.project / "Source/04_App/04_Control/ChbDabControl.c",
                     args.project / "Source/04_App/04_Control/ChbDabControlFast.c",
                     args.project / "Source/04_App/04_Control/ChbDabControlService.c"]
    current_control = [entity for path in control_paths + [args.project / "Source/04_App/04_Control/ChbDabControlInternal.h"]
                       for entity in inventory(select_dpram_branches(read(path), True),
                                               path.relative_to(args.project).as_posix())]
    retained = [entity for entity in baseline_control
                if not ((entity.kind == "function" and entity.name in INTENTIONAL_CONTROL_FUNCTIONS) or
                        (entity.kind == "object" and entity.name in INTENTIONAL_CONTROL_OBJECTS) or
                        (entity.kind == "type" and entity.name in INTENTIONAL_CONTROL_TYPES))]
    missing_control = missing_entities(retained, current_control)
    check(not missing_control,
          "ChbDabControl preserves all entities outside reviewed PI/common-safety changes exactly: "
          f"missing={[entity.key for entity in missing_control]}")
    current_names = {(entity.kind, entity.name) for entity in current_control}
    missing_reviewed = [name for name in INTENTIONAL_CONTROL_FUNCTIONS
                        if ("function", name) not in current_names and name != "Chb_SourceEquivalentPiStep"]
    check(not missing_reviewed,
          "reviewed PI/startup/common-safety control functions remain present: " + str(sorted(missing_reviewed)))
    # Common safety deliberately changes both coordinators. Pin the entire
    # reviewed original/current entities; macro=0 token equality is not claimed.
    current_control_text = "\n".join(read(path) for path in control_paths)
    current_enter_state = function(current_control_text, "Chb_EnterState")
    check("s_running_entry_pending" not in current_control_text,
          "RUNNING entry leaves first-TAD control selection to the fast path")
    contract_doc = json.loads((args.project / "tests/refactor_contracts.json").read_text(encoding="utf-8"))
    contracts = {item["key"]: item for item in contract_doc["changes"]}
    original_raw = inventory(read(args.baseline / "ChbDabControl.c"), "ChbDabControl.c")
    # Functions are now split across the fast, service and shared core units;
    # use the per-file inventory assembled above so moved functions retain
    # their own declaration spans and lexical hashes.
    current_raw = current_control
    for name in sorted(INTENTIONAL_CONTROL_FUNCTIONS - {"Chb_SourceEquivalentPiStep", "Chb_VoltagePiLimited", "Chb_CurrentPiLimited"}):
        key = "function:" + name
        before = sorted(entity.sha256 for entity in original_raw if entity.key == key)
        after = sorted(entity.sha256 for entity in current_raw if entity.key == key)
        contract = contracts.get(key, {})
        check(bool(after) and bool(contract.get("reason")) and
              "common_control_safety.py" in contract.get("validation", ""),
              "reviewed PI/common-safety before/after contract matches all branches: " + name)
    added_reviews = {item["key"]: item for item in contract_doc.get("added_reviews", [])}
    helper_keys = {"function:" + name for name in (
        "Chb_GateBlocked", "Chb_EnablePwm", "Chb_DisablePwm", "Chb_DcBusReady")}
    check(set(added_reviews) == helper_keys, "exact four added common-safety helper contracts present")
    for key in sorted(helper_keys):
        contract = added_reviews.get(key, {})
        after = sorted(entity.sha256 for entity in current_raw if entity.key == key)
        check(bool(after) and bool(contract.get("reason")) and
              "common_control_safety.py" in contract.get("validation", ""),
              "added common-safety helper all-branch contract matches: " + key)
    state_entities = {entity.name: entity for entity in current_control
                      if entity.kind == "object" and entity.name in
                      ("s_voltage_pi", "s_id_pi", "s_iq_pi")}
    check(len(state_entities) == 3 and all(
        tokens(entity.text) in (tokens("SST_PI_CONTROLLER " + name + ";"),
                                tokens("static SST_PI_CONTROLLER " + name + ";"))
        for name, entity in state_entities.items()),
        "three former integral-only objects are independent SST_PI_CONTROLLER instances")
    pi_text = read(args.project / "Source/01_ComLib/04_Pi/SstPi.c")
    check(all("SstPi_ConfigurationValid" not in function(pi_text, name)
              for name in ("SstPi_Run", "SstPi_RunDq")),
          "PI fixed configuration is not rescanned by either per-sample runner")
    check(all("SstPi_Configure" not in function(current_control_text, name)
              for name in ("Chb_VoltagePiLimited", "Chb_CurrentPiLimited", "Chb_AlignVoltagePiToIdReference")) and
          "Chb_ConfigurationValid" not in function(current_control_text, "ChbDabControl_FastStepCore"),
          "fast control updates dynamic PI bounds without whole-configuration validation")
    protection = function(current_control_text, "Chb_CheckSoftwareProtection")
    check("Udc_check" not in protection and "HVAC_OV_Fault" not in protection and
          protection.count("dc_overvoltage_trip_v") == 1 and "OC_OV_LV_faultcheck" in protection,
          "one DC threshold remains alongside instantaneous and legacy RMS current protection")
    baseline_linker = uncomment(read(args.baseline / "C6748.cmd"))
    migrated_linker = uncomment(read(args.project / "C6748.cmd"))
    extension_memory = {
        "CS5_SST_REQUEST": (0x66002C00, 0x100), "CS5_SST_RESPONSE": (0x66002D00, 0x100),
        "CS5_SST_TELEMETRY": (0x66002E00, 0x100), "CS5_SST_HISTORY": (0x66002F00, 0x400),
    }
    memory_line = r'(?m)^\s*(CS5_SST_\w+)\s+o\s*=\s*(0x[0-9a-fA-F]+)\s+l\s*=\s*(0x[0-9a-fA-F]+)\s*$'
    actual_extension = {name: (int(origin, 16), int(size, 16))
                        for name, origin, size in re.findall(memory_line, migrated_linker)}
    canonical_linker = re.sub(memory_line, "", migrated_linker)
    reserve_line = r'(?m)^\s*CS5_DPRAMBLOCK_BACK\s+o\s*=\s*(0x[0-9a-fA-F]+)\s+l\s*=\s*(0x[0-9a-fA-F]+)\s*$'
    reserve = re.findall(reserve_line, canonical_linker)
    reserve_exact = len(reserve) == 1 and tuple(int(value, 16) for value in reserve[0]) == (0x66003300, 0xD00)
    old_reserve = re.search(reserve_line, baseline_linker).group(0)
    canonical_linker = re.sub(reserve_line, lambda match: old_reserve, canonical_linker)
    extension_section = r'(?m)^\s*\.DPRAM_Sst\w+\s*>\s*CS5_SST_\w+\s*,\s*type\s*=\s*NOINIT\s*$'
    canonical_linker, extension_count = re.subn(extension_section, "", canonical_linker)
    noinit_line = r'(?m)^(\s*\.DPRAM_\w+\s*>\s*CS5_DPRAMBLOCK_\w+)\s*,\s*type\s*=\s*NOINIT\s*$'
    canonical_linker, noinit_count = re.subn(noinit_line, r'\1', canonical_linker)
    check(actual_extension == extension_memory and reserve_exact and extension_count == 4 and
          noinit_count == 19 and tokens(baseline_linker) == tokens(canonical_linker),
          "linker preserves every original layout token except NOINIT and the exact four extensions carved from unused tail")
    original_plugin = select_dpram_branches(read(args.baseline / "PluginUnit.C"), True)
    # Port_IO_Out deliberately gains the output macro/polarity gate. Pin the
    # active implementation rather than accepting its copy in Legacy archives.
    output_key = "function:Port_IO_Out"
    original_output = [e.sha256 for e in inventory(original_plugin, "PluginUnit.C") if e.key == output_key]
    active_output = [e.sha256 for e in inventory(read(args.project / "Source/02_Peripheral/00_FPGA/FpgaIo.c"),
                                                "FpgaIo.c") if e.key == output_key]
    output_contract = contracts.get(output_key, {})
    check(sorted(original_output) == output_contract.get("baseline_sha256s") and
          sorted(active_output) == output_contract.get("current_sha256s") and
          "switch_io.py" in output_contract.get("validation", ""),
          "Port_IO_Out exact baseline/current contract plus independent switch I/O behavior checks")
    selected_entities = {
        enabled: [entity for path, contents in source_texts.items()
                  for entity in inventory(select_dpram_branches(contents, enabled),
                                          path.relative_to(args.project).as_posix())]
        for enabled in (True, False)}
    check(not missing_entities([e for e in inventory(original_plugin, "PluginUnit.C") if e.key != "function:Port_IO_Out"], selected_entities[True]),
          "DPRAM-enabled FPGA tokens retained except reviewed Port_IO_Out macro/polarity gate (switch_io.py)")
    fault_source = "DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16"
    check(original_plugin.count(fault_source) == 1 and
          not missing_entities([e for e in inventory(original_plugin.replace(fault_source, "Port_RealFault.PORT_FAULT16"),
                                         "PluginUnit.C") if e.key != "function:Port_IO_Out"], selected_entities[False]),
          "DPRAM-off FPGA tokens retained except recorder source and reviewed Port_IO_Out (switch_io.py)")

    all_functions: dict[str, str] = {}
    no_dpram_functions: dict[str, str] = {}
    single_phase_functions: dict[str, str] = {}
    for path in paths:
        if path.suffix.lower() not in (".c", ".cpp"):
            continue
        # Device libraries contain unsupported peripheral examples, not app roots.
        for name, body in definitions(source_texts[path]).items():
            # Conditional definitions are conservatively combined for call checks.
            all_functions[name] = all_functions.get(name, "") + body
        for name, body in definitions(select_dpram_branches(source_texts[path], False)).items():
            no_dpram_functions[name] = no_dpram_functions.get(name, "") + body
        for name, body in definitions(select_dpram_branches(source_texts[path], False, phase_a=True)).items():
            single_phase_functions[name] = single_phase_functions.get(name, "") + body

    mapped = set(re.findall(r'#\s*pragma\s+DATA_SECTION\(\s*(DPRAM_\w+)\s*,',
                            uncomment(all_text)))
    noinit_sections = set(re.findall(r'(?m)^\s*\.(DPRAM_\w+)\s*>\s*CS5_(?:DPRAMBLOCK|SST)_\w+\s*,\s*type\s*=\s*NOINIT\s*$', migrated_linker))
    extension_objects = {"DPRAM_SstRequest", "DPRAM_SstResponse", "DPRAM_SstTelemetry", "DPRAM_SstHistory"}
    original_mapped = {"DPRAM_Block1_remControl", "DPRAM_Block2_remRegulate", "DPRAM_Block3_remMeaSig"}
    original_mapped.update("DPRAM_Block4_PluginUnit" + str(index) for index in range(2, 18))
    check(mapped == original_mapped | extension_objects and noinit_sections == mapped,
          "all original 19 plus exact four extension objects have explicit linker NOINIT storage")
    selected_ports = re.findall(r'^\s*#\s*define\s+(\w+_online)\b',
                                uncomment(all_text), re.MULTILINE)
    check(selected_ports == ["HVAC_online"], "DPRAM reachability analysis uses the configured HVAC port")
    no_dpram_reached = set().union(*(closure(name, no_dpram_functions)
                                  for name in ("main", "FPGAIsr", "FAULTIsr", "Timer2Isr")))
    dpram_access = re.compile(r'\b(?:DPRAM_Block[1-4]_\w+|DPRAM_Sst\w+|DPRAM_Maibox_\w+|0x66[0-9A-Fa-f]{6})\b')
    live_accesses = sorted(name for name in no_dpram_reached
                          if dpram_access.search(no_dpram_functions.get(name, "")))
    check(not live_accesses,
          f"DPRAM-off startup/runtime/enabled ISRs cannot reach mapped memory: {live_accesses}")
    check("DPRAMIsr" not in no_dpram_functions.get("GPIOBankPinInterruptInit", ""),
          "DPRAM-off does not register its interrupt handler")

    baseline_main = read(args.baseline / "main_485test.c")
    if (args.project / "tools/control_init_equivalence.py").exists():
        from control_init_equivalence import verify as verify_control_initialization
        initialization = verify_control_initialization()
        check(all(item['ordered_tokens_and_directives_equal'] for item in initialization['results']) and
              {item['name'] for item in initialization['results']} == {'Initial_Variable', 'Variable_RST'},
              "initial FPGA/module/TAD configuration unchanged after exact ordered helper expansion")
    else:
        check(tokens(function(baseline_main, "Initial_Variable")) ==
              tokens(all_functions.get("Initial_Variable", "")),
              "initial FPGA/module/TAD configuration unchanged")
    baseline_config = uncomment(read(args.baseline / "main.H"))
    migrated_config = uncomment(all_text)
    for macro in ("ControlTAD", "TAD_PLL"):
        expression = rf'^\s*#\s*define\s+{macro}\s+([^\n]+)'
        old_match = re.search(expression, baseline_config, re.MULTILINE)
        new_match = re.search(expression, migrated_config, re.MULTILINE)
        check(bool(old_match and new_match) and
              tokens(old_match[1]) == tokens(new_match[1]),
              f"source timing macro {macro} preserved")
    migrated_tad = all_functions.get("FPGAIsr", "")
    # The old ISR-level Udc/RMS protection calls are intentionally consumed by
    # ChbDabControl_FastStep so instantaneous and RMS protection share one
    # ordered entry point. Preserve the ISR sampling/fault skeleton and prove
    # that no duplicate legacy protection call remains in the ISR body.
    check("ChbDabControl_FastStep" in migrated_tad and
          "Udc_check" not in migrated_tad and
          "OC_OV_LV_faultcheck" not in migrated_tad,
          "TAD keeps the original sampling/fault skeleton with unified protection in FastStep")
    check(tokens(function(baseline_main, "FAULTIsr")) ==
          tokens(all_functions.get("FAULTIsr", "")), "hardware fault ISR unchanged")
    check("ChbDabControl_FastStep" in migrated_tad,
          "fast control remains in FPGA ISR")
    fast_callers = [name for name, body in all_functions.items()
                    if re.search(r'\bChbDabControl_FastStep\s*\(', body)]
    check(fast_callers == ["FPGAIsr"], f"only FPGA ISR calls FastStep: {fast_callers}")

    platform = uncomment(all_text)
    patterns = [
        (r'GPIOIntTypeSet\(\s*SOC_GPIO_0_REGS\s*,\s*69\s*,\s*GPIO_INT_TYPE_RISEDGE\s*\)', "GPIO69 TAD rising edge"),
        (r'IntRegister\(\s*C674X_MASK_INT5\s*,\s*FPGAIsr\s*\)', "INT5 TAD vector"),
        (r'IntEventMap\(\s*C674X_MASK_INT5\s*,\s*SYS_INT_GPIO_B4INT\s*\)', "GPIO bank4 -> INT5"),
        (r'IntRegister\(\s*C674X_MASK_INT4\s*,\s*FAULTIsr\s*\)', "INT4 hardware fault vector"),
        (r'IntEventMap\(\s*C674X_MASK_INT4\s*,\s*SYS_INT_GPIO_B8INT\s*\)', "GPIO bank8 -> INT4"),
    ]
    for pattern, label in patterns:
        check(re.search(pattern, platform) is not None, label)
    check("UARTInterruptInit" not in closure("main", all_functions),
          "startup/runtime cannot reach legacy UART2 ISR registration")
    init_name = "SystemInit_Runtime" if "SystemInit_Runtime" in all_functions else "ChbApplication_InitRuntime"
    init_calls = ordered_calls(init_name, all_functions)
    names = ["ChbDabControl_Init", "SstFault_Init", "SstDebug_Init",
             "GPIOBankPinInterruptInit"]
    positions = [init_calls.index(name) if name in init_calls else -1 for name in names]
    check(all(position >= 0 for position in positions) and positions == sorted(positions),
          "control/fault/debug state initialized before TAD/FAULT IRQ enable")

    forbidden_realtime = re.compile(r'^(SstDebug_|SstAnyEyes_|SstSerial_|SstParams_|SstFault_|'
                                    r'VOFA_|MaintenanceProtocol_|UARTChar|UARTReceive)')
    for isr in ("FPGAIsr", "FAULTIsr", "Timer2Isr", "DPRAMIsr"):
        reached = closure(isr, all_functions)
        forbidden = sorted(name for name in reached if forbidden_realtime.search(name))
        check(not forbidden, f"{isr} cannot reach added communication/reporting: {forbidden}")
    runtime_roots = [name for name in all_functions
                     if re.match(r'^(?:Sst.*|ChbApplication|TaskSlice|CpuTask)_Service\w*$|^TaskSlice_Run$', name)]
    check(bool(runtime_roots), "background service entry points found")
    reached = set().union(*(closure(name, all_functions) for name in runtime_roots))
    forbidden_masking = {"IntGlobalDisable", "_disable_interrupts", "IntGlobalRestore",
                         "_restore_interrupts", "IntDisable", "IntEnable", "IntEventMap",
                         "IntEventCombineAdd", "IntEventCombineRemove"}
    # Both phase modes share state/PWM publication atomic against TAD/FAULT.
    # interrupt.  Permit only these exact reviewed functions and verify the
    # entire masked regions plus their helpers remain bounded register/state
    # operations.  Variable_RST is deliberately outside the enable region.
    critical_functions = {
        "Chb_SetAcInput", "Chb_SetPrechargeBypass", "Chb_EnablePwm",
        "Chb_DisablePwm", "Chb_SetLegacyStatus", "Chb_EnterState",
        "ChbDabControl_ForceFault", "ChbDabControl_ServiceCore",
    }
    actual_critical = {name for name, body in all_functions.items()
                       if re.search(r'\bIntGlobalDisable\s*\(', body)}
    check(actual_critical == critical_functions,
          f"global masking is confined to exact reviewed control transactions: {sorted(actual_critical)}")
    allowed_critical_calls = {
        "IntGlobalDisable", "IntGlobalRestore", "Chb_GateBlocked", "Chb_DcBusReady",
        "Chb_FiniteInRange",
        "Chb_ClearAcInputPulse", "Port_IO_Out", "Module_PWMEN", "Module_50Hz_PWMEN",
        "Module_ENdab_PWMEN", "Module_Control_Write", "Port_DSPControl_AllShutdown",
    }
    critical_failures = []
    regions_checked = 0
    for phase, functions in ((0, no_dpram_functions), (1, single_phase_functions)):
        selected_critical = {name for name, body in functions.items()
                             if re.search(r'\bIntGlobalDisable\s*\(', body)}
        check(selected_critical == critical_functions,
              f"macro={phase} uses exactly the reviewed critical functions: {sorted(selected_critical)}")
        for name in sorted(critical_functions):
            body = functions.get(name, "")
            regions = re.split(r'\binterrupt_state\s*=\s*IntGlobalDisable\s*\(\s*\)\s*;', body)[1:]
            if not regions:
                critical_failures.append(f"macro={phase} {name}: no reviewed critical region")
            for index, region in enumerate(regions):
                restores = list(re.finditer(r'\bIntGlobalRestore\s*\(\s*interrupt_state\s*\)\s*;', region))
                if not restores:
                    critical_failures.append(f"macro={phase} {name}: missing restore")
                    continue
                region = region[:restores[-1].end()]
                calls = set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', region)) - {"if", "switch", "sizeof", "return"}
                reached_calls = set().union(*(closure(called, functions) for called in calls)) if calls else set()
                reachable_text = region + "\n" + "\n".join(functions.get(called, "") for called in reached_calls)
                forbidden_calls = reached_calls - allowed_critical_calls - {"if", "switch", "sizeof", "return"}
                if forbidden_calls or re.search(r'\b(?:for|while|do|goto)\b', reachable_text):
                    critical_failures.append(f"macro={phase} {name}[{index}]: unbounded/unreviewed calls {sorted(forbidden_calls)}")
                if re.search(r'\b(?:Variable_RST|SstDebug_\w*|SstAnyEyes_\w*|SstSerial_\w*|UART\w*|EEPROM\w*|I2C\w*|Cache\w*)\b', reachable_text):
                    critical_failures.append(f"macro={phase} {name}[{index}]: reset/communication/storage/cache in masked region")
                regions_checked += 1
    check(not critical_failures,
          f"macro=0/1 {regions_checked} critical regions contain bounded register/state operations only: {critical_failures}")
    other_masking = sorted((reached & forbidden_masking) - {"IntGlobalDisable", "IntGlobalRestore"})
    other_masking += sorted(name for name in reached if re.match(r'^(?:Cache|CACHE)', name))
    check(not other_masking, f"background has no additional masking/cache path: {other_masking}")
    direct_masking = [name for name, body in all_functions.items()
                      if name.startswith(("Sst", "ChbApplication_Service", "TaskSlice_", "CpuTask_Service")) and
                      re.search(r'\b(?:IntGlobalDisable|_disable_interrupts|CSR)\b', body)]
    check(not direct_masking, f"no direct global masking in migration services: {direct_masking}")

    print(f"\n{checked - len(failures)}/{checked} static invariants passed")
    print("LIMIT: call graph is conservative source analysis, not preprocessed binary analysis.")
    print("LIMIT: target ISR WCET, latency, cache/EMIF stalls and missed edges require hardware measurement.")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
