"""Verify module boundaries, startup/background order and standalone TI headers."""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from source_equivalence import source_paths, source_inventory, decode, tokens
from realtime_audit import read, function, uncomment


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool-root", type=Path, default=Path(os.environ.get("SST_CGT_ROOT",
                        "E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12")))
    args = parser.parse_args()
    compiler = args.tool_root / "bin/cl6x.exe"
    if not compiler.exists():
        raise SystemExit("TI compiler unavailable: " + str(compiler))
    paths = source_paths(ROOT)
    current = {path.relative_to(ROOT).as_posix(): read(path) for path in paths}
    failures, assertions = [], []
    def check(condition, label):
        assertions.append(dict(assertion=label, passed=bool(condition)))
        print(("PASS " if condition else "FAIL ") + label)
        if not condition:
            failures.append(label)

    forbidden = []
    for path, text in current.items():
        # The compatibility headers may list domain headers; no module includes
        # the old umbrella API after the migration.
        for target in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"', uncomment(text), re.MULTILINE):
            if target.lower() in ("main.h", "pluginunit.h", "stateflow.h"):
                forbidden.append(path + " -> " + target)
    check(not forbidden, "activity source has no legacy umbrella-header includes: " + repr(forbidden))
    entities = source_inventory(ROOT)
    functions = defaultdict(list)
    for entity in entities:
        if entity.kind == "function":
            functions[entity.name].append(entity)
    def body(name):
        matches = functions[name]
        if len(matches) != 1:
            raise ValueError(name + " must have one unique definition")
        return function(matches[0].text, name)
    def calls(contents):
        return re.findall(r'\b([A-Za-z_]\w*)\s*\(', uncomment(contents))
    entry = body("main")
    boot = ["CpuTask_InitBootFlags", "SystemInit_Hardware", "SystemInit_WaitPowerOnDelay", "SystemInit_Runtime"]
    positions = [entry.find(name + "(") for name in boot]
    check(all(position >= 0 for position in positions) and positions == sorted(positions),
          "Main performs boot flags, hardware, original 20s wait, runtime initialization in order")
    check(re.search(r'#\s*if\s+CHB_OPEN_LOOP_PULSE_TEST', entry) is not None and
          calls(entry).count("TaskSlice_Service") == 2,
          "Main retains pulse-test and normal main-loop branches with the same background service")
    main_entities = [entity for entity in entities if entity.file.endswith("/Main.c")]
    check(all(entity.kind in ("function", "prototype") for entity in main_entities) and
          {entity.name for entity in main_entities if entity.kind == "function"} == {"main"},
          "Main owns no domain data or interrupt implementations")
    task = body("TaskSlice_Service")
    expected = ["CpuTask_ServiceHeartbeat", "ChbDabControl_Service", "SstFault_Service",
                "SstDpramHost_ServiceRemoteControl", "SstDpramHost_ServiceTelemetry", "SstDpramMaintenance_Service",
                "SstDebug_GetTick10ms", "SstSerial_TxService", "SstDebug_ReceiveService",
                "SstDebug_ProtocolService", "SstParameterStore_Service", "SstSerial_TxService",
                "Port_RealTimeParameter"]
    actual = [name for name in calls(task) if name not in ("if", "for", "while")]
    check(actual == expected, "TaskSlice explicit heartbeat/control/fault/DPRAM/parallel maintenance, capture-before-TX, TX/RX/protocol/store/TX/measurement order")
    runtime = body("SystemInit_Runtime")
    gate = runtime.find("GPIOBankPinInterruptInit(")
    required = ["Initial_Variable", "Variable_RST", "FaultLatch_Clear", "ChbDabControl_Init", "SstFault_Init",
                "SstDpramMaintenance_Init", "SstDebug_Init"]
    check(gate > 0 and all(0 <= runtime.find(name + "(") < gate for name in required),
          "SystemInit initializes safe control/fault/debug objects before enabling external interrupts")
    check(bool(re.search(r'while\s*\(\s*!Flag_20s\s*\)', body("SystemInit_WaitPowerOnDelay"))),
          "SystemInit retains Timer2 Flag_20s power-on wait")

    # Multiple conditional definitions in one translation unit are legitimate;
    # any definition in more than one .c/.cpp is a build ownership failure.
    duplicates = []
    for kind in ("function", "object", "section"):
        owners = defaultdict(set)
        for entity in entities:
            if entity.kind == kind and Path(entity.file).suffix.lower() in (".c", ".cpp"):
                if kind != "section" and tokens(entity.text)[:1] == ("static",):
                    continue
                owners[entity.name].add(entity.file)
        duplicates += [kind + ":" + name + " " + repr(sorted(files))
                       for name, files in owners.items() if len(files) > 1]
    check(not duplicates, "each external function/object/MMIO assignment has one owning translation unit: " + repr(duplicates))

    headers = [path for path in paths if path.suffix.lower() in (".h", ".hpp")]
    by_name = {path.name.lower(): path for path in headers}
    dependencies = {path: [by_name[target.lower()] for target in re.findall(
        r'^\s*#\s*include\s*"([^"\n]+)"', uncomment(read(path)), re.MULTILINE)
        if target.lower() in by_name] for path in headers}
    cycles = set()
    def visit(path, stack):
        if path in stack:
            cycles.add(" -> ".join(item.name for item in stack[stack.index(path):] + [path]))
            return
        for child in dependencies[path]:
            visit(child, stack + [path])
    for path in headers:
        visit(path, [])
    check(not cycles, "custom public headers have no include dependency cycle: " + repr(sorted(cycles)))
    includes = [ROOT, args.tool_root / "include"] + [path for path in (ROOT / "Source").rglob("*") if path.is_dir()]
    includes = ["--include_path=" + str(path) for path in includes]
    report_folder = ROOT / "build/framework_structure"
    report_folder.mkdir(parents=True, exist_ok=True)
    header_results = []
    with tempfile.TemporaryDirectory(prefix="sst_header_interfaces_") as folder:
        work = Path(folder)
        def compile_header(item):
            header, mode = item
            unit = work / (header.name + "." + mode)
            unit.write_text('#include "' + header.name + '"\n', encoding="utf-8")
            obj = work / (header.name + "." + mode + ".obj")
            result = subprocess.run([str(compiler), "-mv6740", "--define=c6748", "--compile_only",
                                     "--diag_wrap=off", "--display_error_number", *includes,
                                     "--output_file=" + str(obj), str(unit)], cwd=work,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            return dict(header=header.relative_to(ROOT).as_posix(), language=mode, exit_code=result.returncode,
                        output=result.stdout.decode("utf-8", errors="replace"))
        items = [(header, mode) for header in headers for mode in
                 (("cpp",) if header.suffix.lower() == ".hpp" else ("c", "cpp"))]
        with ThreadPoolExecutor(max_workers=4) as pool:
            header_results = list(pool.map(compile_header, items))
    failed_headers = [result["header"] + " [" + result["language"] + "]" for result in header_results if result["exit_code"]]
    check(not failed_headers, f"standalone TI C/C++ header compilation ({len(header_results)} units): " + repr(failed_headers))
    active_build = ROOT / 'build/Debug/default/build.log'
    if active_build.exists():
        implicit = re.findall(r'^.*warning\s+#225[^\n]*', active_build.read_text(encoding='utf-8'), re.MULTILINE)
        check(not implicit, "TI Debug build has no implicit function-declaration diagnostics: " + repr(implicit))
    report = dict(assertions=assertions, failures=failures, headers=header_results,
                  input_sha256={path.relative_to(ROOT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths},
                  limitations="Structural and standalone compilation checks do not prove ISR worst-case execution time.")
    (report_folder / "result.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
