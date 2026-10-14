"""Audit temporary bench input isolation with the real TI C6000 compiler.

The saved baseline and working source use identical explicit build switches.
This performs preprocessing and individual translation-unit compilation only;
it does not link firmware, prove executable byte identity, or measure timing.
"""
from __future__ import annotations

import argparse
import difflib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parents[1]
BASELINE = "6675a727fc5424b49e860b72acdf2c49e9181b9b"
FILES = {
    "Port_AD_Sample": "Source/04_App/02_Measurement/ADC_Sample.C",
    "SVG_Udc_Average": "Source/04_App/04_Control/HVAC_Strategy.C",
    "Udc_check": "Source/04_App/08_Fault/ModuleVoltageCheck.c",
    "SystemInit_Runtime": "Source/03_System/01_OS/SystemInit.c",
}
ISR = "Source/03_System/02_ISR/Redef_Interrupt.c"
CONTROL = "Source/04_App/04_Control/ChbDabControl.c"
BENCH = "Source/04_App/02_Measurement/SstBenchInputs.c"
ISR_NAMES = ("FPGAIsr", "FAULTIsr", "Timer2Isr", "DPRAMIsr")
LITERAL_OR_COMMENT = re.compile(
    r'''("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|/\*[\s\S]*?\*/|//[^\n]*''')
TOKEN = re.compile(
    r'''"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|'''
    r'''(?:\d+(?:\.\d*)?|\.\d+)(?:[EePp][+-]?\d+)?[A-Za-z_\d]*|'''
    r'''>>=|<<=|->|\+\+|--|&&|\|\||<=|>=|==|!=|\*=|/=|%=|\+=|-=|<<|>>|&=|\^=|\|=|[^\s]''')


def read(path: Path) -> str:
    """Read existing source and compiler output without changing its encoding."""
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "gb18030"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            pass
    return data.decode("latin1")


def tokens(text: str) -> list[str]:
    """Keep literal contents and operators significant; ignore only comments/space."""
    clean = LITERAL_OR_COMMENT.sub(lambda match: match[1] or " ", text)
    return TOKEN.findall(clean)


def balanced_end(items: list[str], start: int, opening: str, closing: str) -> int:
    """Find a closing token while respecting nested parentheses or braces."""
    level = 0
    for index in range(start, len(items)):
        level += (items[index] == opening) - (items[index] == closing)
        if level == 0:
            return index
    raise ValueError("Unbalanced " + opening)


def function(items: list[str], name: str) -> list[str]:
    """Select a definition body, excluding prototypes and ordinary call sites."""
    for index in range(len(items) - 1):
        if items[index:index + 2] != [name, "("]:
            continue
        end = balanced_end(items, index + 1, "(", ")")
        if end + 1 < len(items) and items[end + 1] == "{":
            closing = balanced_end(items, end + 1, "{", "}")
            return items[end + 1:closing + 1]
    raise ValueError("Definition missing: " + name)


def digest(items: list[str]) -> str:
    return hashlib.sha256("\0".join(items).encode("utf-8")).hexdigest()


def file_hash(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_hash(project: Path) -> str:
    """Match the repository's firmware input fingerprint, without extra imports."""
    result = hashlib.sha256()
    paths = [path for path in (project / "Source").rglob("*") if path.is_file()]
    paths += [project / path for path in (".project", ".cproject", ".ccsproject", "C6748.cmd")]
    for path in sorted(paths):
        result.update(path.relative_to(project).as_posix().encode())
        result.update(b"\0")
        result.update(path.read_bytes())
        result.update(b"\0")
    return result.hexdigest()


def run(command: list[str | Path], cwd: Path, log: Path | None = None,
        require_success: bool = True) -> subprocess.CompletedProcess:
    result = subprocess.run([str(value) for value in command], cwd=cwd,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=120)
    if log:
        log.write_bytes(result.stdout)
    if require_success and result.returncode:
        raise RuntimeError(result.stdout.decode(errors="replace")[-8000:])
    return result


def switches(bench: int = 0, simulator: int = 1, dpram: int = 0,
             eeprom: int = 0) -> dict[str, int]:
    return {"SST_BENCH_INPUT_SIM": bench, "SST_SIMULATOR_MODE": simulator,
            "SST_ENABLE_DPRAM": dpram, "CHB_LOAD_EEPROM_PARAMETERS": eeprom}


def flags(project: Path, cgt: Path, defines: dict[str, int]) -> list[str]:
    includes = [project, cgt / "include"]
    includes += [path for path in sorted((project / "Source").rglob("*")) if path.is_dir()]
    return (["-mv6740", "-g", "--define=c6748", "--diag_warning=225",
             "--diag_wrap=off", "--display_error_number"] +
            ["--define=" + name + "=" + str(value) for name, value in defines.items()] +
            ["--include_path=" + str(path) for path in includes])


def preprocess(project: Path, relative: str, cgt: Path, defines: dict[str, int],
               folder: Path) -> list[str]:
    folder.mkdir(parents=True, exist_ok=True)
    run([cgt / "bin/cl6x.exe"] + flags(project, cgt, defines) +
        ["--preproc_only", "--pp_directory=" + str(folder), project / relative],
        project, folder / "preprocess.log")
    output = folder / (Path(relative).stem + ".pp")
    if not output.is_file():
        raise RuntimeError("TI preprocessing did not create " + str(output))
    return tokens(read(output))


def compile_unit(project: Path, source: Path, cgt: Path, defines: dict[str, int],
                 folder: Path, keep_asm: bool = False) -> subprocess.CompletedProcess:
    folder.mkdir(parents=True, exist_ok=True)
    options = ["--compile_only", "--output_file=" + str(folder / "unit.obj")]
    if keep_asm:
        options += ["--keep_asm", "--asm_directory=" + str(folder)]
    return run([cgt / "bin/cl6x.exe"] + flags(project, cgt, defines) + options +
               [source], project, folder / "compile.log", require_success=False)


def subsequence(items: list[str], sequence: list[str]) -> list[int]:
    return [index for index in range(len(items) - len(sequence) + 1)
            if items[index:index + len(sequence)] == sequence]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=ROOT)
    parser.add_argument("--baseline", default=BASELINE)
    parser.add_argument("--tool-root", type=Path, default=Path(os.environ.get(
        "SST_CGT_ROOT", "E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12")))
    args = parser.parse_args()
    project, cgt = args.project.resolve(), args.tool_root.resolve()
    destination = project / "build/bench_isolation"
    destination.mkdir(parents=True, exist_ok=True)
    report = {
        "baseline": args.baseline, "compiler": str(cgt / "bin/cl6x.exe"),
        "compiler_flags": ["-mv6740", "-g", "--define=c6748"],
        "baseline_comparison_defines": switches(), "results": [],
        "method": "TI --preproc_only lexical comparison and individual TI --keep_asm compilations; no firmware link",
        "limitations": "No executable byte-identity, hardware timing, interrupt latency, power-circuit model or physical fault-path claim.",
    }

    def check(condition: bool, name: str, **details) -> None:
        report["results"].append(dict(name=name, passed=bool(condition), **details))
        print(("PASS " if condition else "FAIL ") + name, flush=True)

    def equivalent(before: list[str], after: list[str], name: str) -> None:
        same = before == after
        check(same, name, baseline_token_count=len(before), current_token_count=len(after),
              baseline_sha256=digest(before), current_sha256=digest(after))
        if not same:
            diff = difflib.unified_diff(before, after, fromfile="baseline", tofile="current")
            (destination / (re.sub(r"[^\w.-]", "_", name) + ".diff")).write_text(
                "\n".join(diff), encoding="utf-8")

    try:
        if not (cgt / "bin/cl6x.exe").is_file():
            raise FileNotFoundError("TI C6000 compiler missing: " + str(cgt))
        start_hash = source_hash(project)
        report["firmware_input_sha256"] = start_hash
        report["compiler_sha256"] = file_hash(cgt / "bin/cl6x.exe")
        relevant = list(FILES.values()) + [ISR, CONTROL, BENCH,
                    "Source/04_App/02_Measurement/SstBenchInputs.h",
                    "Source/04_App/00_Config/ChbBuildConfig.h"]
        report["source_file_sha256"] = {path: file_hash(project / path) for path in relevant}
        fixtures = [Path(__file__).resolve(), project / "tests/isr_binary_audit.py"]
        report["fixture_sha256"] = {path.name: file_hash(path) for path in fixtures}
        revision = run(["git", "rev-parse", args.baseline + "^{commit}"], project).stdout.decode().strip()
        report["baseline_resolved_commit"] = revision
        archive = run(["git", "archive", "--format=zip", revision], project).stdout
        with tempfile.TemporaryDirectory(prefix="sst_bench_baseline_") as temporary:
            baseline = Path(temporary)
            with zipfile.ZipFile(io.BytesIO(archive)) as saved:
                saved.extractall(baseline)
            report["baseline_firmware_input_sha256"] = source_hash(baseline)
            old, new = {}, {}
            for relative in list(FILES.values()) + [ISR, CONTROL]:
                label = Path(relative).stem
                old[relative] = preprocess(baseline, relative, cgt, switches(),
                                           destination / "baseline" / label)
                new[relative] = preprocess(project, relative, cgt, switches(),
                                           destination / "current_off" / label)
            for name, relative in FILES.items():
                equivalent(function(old[relative], name), function(new[relative], name),
                           "macro-off body " + name)
            equivalent(old[ISR], new[ISR], "macro-off entire ISR translation unit")
            equivalent(old[CONTROL], new[CONTROL], "macro-off entire CHB control translation unit")

            # Reuse only the established normalization helper, never its build main.
            spec = importlib.util.spec_from_file_location("bench_isr_audit", project / "tests/isr_binary_audit.py")
            helper = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(helper)
            assembly = {}
            for label, tree in (("baseline", baseline), ("current", project)):
                folder = destination / (label + "_assembly")
                result = compile_unit(tree, tree / ISR, cgt, switches(), folder, keep_asm=True)
                if result.returncode:
                    raise RuntimeError(result.stdout.decode(errors="replace")[-8000:])
                check(not re.search(rb"warning\s+#225", result.stdout),
                      label + " ISR has no implicit function declaration")
                assembly[label] = read(folder / (Path(ISR).stem + ".asm"))
            for name in ISR_NAMES:
                before = helper.routine(assembly["baseline"], name)
                after = helper.routine(assembly["current"], name)
                exact = before["instructions"] == after["instructions"]
                calls_equal = before["direct_calls"] == after["direct_calls"]
                stack_equal = (before["stack_usage"] is not None and
                               before["stack_usage"] == after["stack_usage"])
                check(exact and calls_equal and stack_equal, "TI ISR assembly " + name,
                      normalized_instructions_equal=exact, direct_calls_equal=calls_equal,
                      direct_calls=after["direct_calls"], stack_equal=stack_equal,
                      baseline_stack=before["stack_usage"], current_stack=after["stack_usage"],
                      baseline_instruction_sha256=digest(before["instructions"]),
                      current_instruction_sha256=digest(after["instructions"]))
                if not exact:
                    diff = difflib.unified_diff(before["instructions"], after["instructions"],
                                               fromfile="baseline", tofile="current")
                    (destination / (name + ".assembly.diff")).write_text("\n".join(diff), encoding="utf-8")

            enabled = {relative: preprocess(project, relative, cgt, switches(bench=1),
                        destination / "current_on" / Path(relative).stem) for relative in FILES.values()}
            original = function(old[FILES["Port_AD_Sample"]], "Port_AD_Sample")
            sampled = function(enabled[FILES["Port_AD_Sample"]], "Port_AD_Sample")
            hook = tokens("if(latch == 0U) { SstBenchInputs_FastStep(); return; }")
            positions = subsequence(sampled, hook)
            check(len(positions) == 1, "macro-on routine sample calls generator and returns only for latch 0")
            if len(positions) == 1:
                offset = positions[0]
                remainder = sampled[:offset] + sampled[offset + len(hook):]
                equivalent(original, remainder, "macro-on latch nonzero retains entire original sampling body")
                prefix = sampled[1:offset]
                declarations = " ".join(prefix).split(";")
                declaration_only = all(not part.strip() or re.fullmatch(
                    r"\s*(?:unsigned\s+char|float)\s+[\w\s,\[\]]+", part)
                    for part in declarations)
                check(declaration_only, "macro-on injection precedes every original sampling statement")
            for name in ("SVG_Udc_Average", "Udc_check"):
                body = function(enabled[FILES[name]], name)
                check(body == ["{", "return", ";", "}"],
                      "macro-on " + name + " returns without FPGA/module reads", body_sha256=digest(body))
            startup = function(enabled[FILES["SystemInit_Runtime"]], "SystemInit_Runtime")
            initialization = tokens("SstBenchInputs_Init();")
            offsets = subsequence(startup, initialization)
            check(len(offsets) == 1 and startup.index("ChbDabControl_Init") <
                  startup.index("SstBenchInputs_Init") < startup.index("GPIOBankPinInterruptInit"),
                  "macro-on bench initialization follows core init and precedes IRQ enable")
            if len(offsets) == 1:
                offset = offsets[0]
                equivalent(function(old[FILES["SystemInit_Runtime"]], "SystemInit_Runtime"),
                           startup[:offset] + startup[offset + len(initialization):],
                           "macro-on startup has only the bench initialization addition")

            off_module = preprocess(project, BENCH, cgt, switches(), destination / "bench_module_off")
            for name in ("SstBenchInputs_Init", "SstBenchInputs_FastStep"):
                check(function(off_module, name) == ["{", "}"], "macro-off API empty " + name)
            forbidden = sorted({item for item in off_module if item.startswith("g_sst_bench_") or
                                item in ("s_grid_angle_rad", "BenchRangeValid", "cos", "cosf", "sin", "sinf")})
            check(not forbidden, "macro-off module has no bench data or waveform math", forbidden_tokens=forbidden)
            for bench in (0, 1):
                result = compile_unit(project, project / BENCH, cgt, switches(bench=bench),
                                      destination / ("bench_module_compile_" + str(bench)))
                check(result.returncode == 0, "TI compiles real bench module macro " + str(bench),
                      returncode=result.returncode)

            fixture = destination / "config_fixture.c"
            fixture.write_text('#include "ChbBuildConfig.h"\nint main(void) { return 0; }\n', encoding="utf-8")
            report["fixture_sha256"]["config_fixture.c"] = file_hash(fixture)
            cases = [(f"off_sim{sim}_dpram{dpram}_eeprom{eeprom}", switches(0, sim, dpram, eeprom), True, "")
                     for sim in (0, 1) for dpram in (0, 1) for eeprom in (0, 1)]
            message = "Bench input simulation requires simulator=1, DPRAM=0 and EEPROM boot load=0"
            cases += [("on_valid", switches(1), True, ""),
                      ("on_invalid_simulator", switches(1, simulator=0), False, message),
                      ("on_invalid_dpram", switches(1, dpram=1), False, message),
                      ("on_invalid_eeprom", switches(1, eeprom=1), False, message),
                      ("invalid_bench_2", switches(2), False, "SST_BENCH_INPUT_SIM must be 0 or 1")]
            for label, defines, expected, diagnostic in cases:
                result = compile_unit(project, fixture, cgt, defines, destination / "config" / label)
                output = result.stdout.decode(errors="replace")
                passed = (result.returncode == 0) if expected else (result.returncode != 0 and diagnostic in output)
                check(passed, "TI configuration " + label, defines=defines,
                      expected_compilation_success=expected, returncode=result.returncode,
                      required_diagnostic=diagnostic)
            final_hash = source_hash(project)
            report["firmware_input_sha256_after"] = final_hash
            check(start_hash == final_hash, "firmware input unchanged during audit")
    except Exception as error:
        check(False, "audit completed", error=type(error).__name__ + ": " + str(error))
    report["passed"] = bool(report["results"]) and all(row["passed"] for row in report["results"])
    (destination / "result.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print("REPORT " + str(destination / "result.json"), flush=True)
    raise SystemExit(0 if report["passed"] else 1)


if __name__ == "__main__":
    main()
