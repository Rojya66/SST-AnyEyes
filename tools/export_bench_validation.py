"""Validate and export bench-input delivery evidence without rebuilding firmware.

Seven target build reports, host checks, isolation results, map audits and
comment-file hashes must match the current firmware inputs before this script
writes a new summary. Historical summaries and all input reports are preserved.
The delivery tag identifies the planned release without a circular commit hash.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

from firmware_inputs import fingerprint


ROOT = Path(__file__).resolve().parents[1]
BASELINE = "6675a727fc5424b49e860b72acdf2c49e9181b9b"
BASELINE_TAG = "bench-input-baseline-2026-10-14"
DELIVERY_TAG = "bench-input-simulation-2026-10-14"
HOST_CHECKS = {
    "realtime_audit", "control_regression", "fault_reporting", "dpram_commands",
    "parameter_precision", "protocol_regression", "refactor_equivalence",
    "comment_coverage", "framework_structure", "firmware_trace",
    "serial_protocol_scope", "parameter_store_migration", "dpram_maintenance",
    "bench_inputs",
}
BENCH_DEFINES = ["SST_BENCH_INPUT_SIM=1", "SST_SIMULATOR_MODE=1",
                 "SST_ENABLE_DPRAM=0", "CHB_LOAD_EEPROM_PARAMETERS=0"]
BUILD_SELECTIONS = [
    ("Debug", "default", []), ("Release", "default", []),
    ("Debug", "bench_inputs", BENCH_DEFINES),
    ("Release", "bench_inputs", BENCH_DEFINES),
    ("Debug", "bench_inputs_vofa", BENCH_DEFINES + ["SST_DEBUG_PROTOCOL=2"]),
    ("Release", "bench_inputs_vofa", BENCH_DEFINES + ["SST_DEBUG_PROTOCOL=2"]),
    ("Debug", "anyeyes_remote_dpram", ["SST_SIMULATOR_MODE=0",
        "SST_ENABLE_DPRAM=1", "SST_BENCH_INPUT_SIM=0"]),
]


def require(condition: bool, message: str) -> None:
    """Reject stale, failed, missing or inconsistent evidence explicitly."""
    if not condition:
        raise ValueError(message)


def local_path(value: str | Path) -> Path:
    """Resolve a reported file and ensure it belongs to this project."""
    path = Path(value)
    path = (path if path.is_absolute() else ROOT / path).resolve()
    require(path.is_relative_to(ROOT), "Evidence outside project: " + str(path))
    require(path.is_file(), "Missing evidence file: " + str(path))
    return path


def relative(path: Path) -> str:
    """Use project-relative slash-separated paths in the portable summary."""
    return path.relative_to(ROOT).as_posix()


def sha(path: Path) -> str:
    """Fingerprint one raw source, report, log or firmware artifact."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def artifact(value: str | Path) -> dict:
    """Record a present, nonempty file's path, size and SHA256."""
    path = local_path(value)
    size = path.stat().st_size
    require(size > 0, "Empty evidence file: " + str(path))
    return {"path": relative(path), "size_bytes": size, "sha256": sha(path)}


def read(value: str) -> dict | list:
    """Load an existing UTF-8 JSON report without modifying it."""
    return json.loads(local_path(value).read_text(encoding="utf-8-sig"))


def git_revision(value: str) -> str:
    """Resolve annotated tags through their commit rather than tag object IDs."""
    return subprocess.check_output(["git", "rev-parse", value + "^{commit}"],
                                   cwd=ROOT, text=True).strip()


def verify_macro_selection(compiler: str, defines: list[str], expected: dict) -> dict:
    """Check current effective config values with preprocessing-only assertions."""
    source = '#include "' + (ROOT / "Source/04_App/00_Config/ChbBuildConfig.h").as_posix() + '"\n'
    for name, value in expected.items():
        source += f'#if {name} != {value}\n#error "Unexpected {name}"\n#endif\n'
    with tempfile.TemporaryDirectory(prefix="sst_bench_export_macros_") as folder:
        fixture = Path(folder) / "macro_selection.c"
        fixture.write_text(source, encoding="utf-8")
        result = subprocess.run([compiler, "-E", "-x", "c"] +
            ["-D" + define for define in defines] + [str(fixture)],
            cwd=ROOT, capture_output=True, text=True)
    require(result.returncode == 0, "Effective macro check failed: " + result.stderr)
    return {"effective_values": expected,
            "method": "GCC preprocessing-only #if assertions on the current config header",
            "fixture_sha256": hashlib.sha256(source.encode("utf-8")).hexdigest(),
            "exit_code": result.returncode}


def main() -> None:
    """Cross-check all current delivery evidence, then write one new summary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="bench_input_validation.json",
                        help="Filename under docs/evidence; existing historical names stay untouched")
    parser.add_argument("--cc", default=shutil.which("gcc"),
                        help="Host GCC executable used only to verify effective config macros")
    args = parser.parse_args()
    require(Path(args.output).name == args.output and args.output.endswith(".json"),
            "Output must be a JSON filename below docs/evidence")
    require(bool(args.cc), "GCC missing; use --cc for preprocessing macro checks")
    require(args.output not in {"software_validation.json", "dpram_maintenance_validation.json",
                               "dpram_mapping_audit.json", "protection_comment_correction.json"},
            "Historical evidence filenames cannot be overwritten")
    initial_fingerprint = fingerprint(ROOT)
    require(git_revision(BASELINE_TAG) == BASELINE, "Baseline tag resolves to wrong commit")

    builds = []
    build_report_paths = []
    target_compilers = set()
    expected_map_paths = set()
    for configuration, variant, expected_defines in BUILD_SELECTIONS:
        folder = f"build/{configuration}/{variant}"
        report_path = folder + "/build_result.json"
        report = read(report_path)
        require(report["configuration"] == configuration and report["variant"] == variant,
                "Wrong target build identity: " + report_path)
        require(sorted(report["defines"]) == sorted(expected_defines),
                "Wrong target build defines: " + report_path)
        require(report["compile_failures"] == [] and report["link_status"] == 0,
                "Failed target build: " + report_path)
        require(report["firmware_unchanged_during_build"] is True,
                "Firmware changed during target build: " + report_path)
        require(report["firmware_input_sha256"] == initial_fingerprint,
                "Stale target build: " + report_path)
        require(report["source_count"] == 88, "Unexpected target source count: " + report_path)
        executable = local_path(report["executable"])
        require(relative(executable) == folder + "/SST_AnyEyes.out",
                "Wrong executable location: " + report_path)
        expected = {"SST_BENCH_INPUT_SIM": int(variant.startswith("bench_inputs")),
                    "SST_SIMULATOR_MODE": int(variant != "anyeyes_remote_dpram"),
                    "SST_ENABLE_DPRAM": int(variant == "anyeyes_remote_dpram"),
                    "CHB_LOAD_EEPROM_PARAMETERS": 0,
                    "SST_DEBUG_PROTOCOL": 2 if variant.endswith("vofa") else 1}
        expected_map_paths.add(folder + "/SST_AnyEyes.map")
        target_compilers.add(report["compiler"])
        builds.append({key: report[key] for key in (
            "configuration", "variant", "defines", "source_count", "compile_failures",
            "link_status", "warnings", "firmware_input_sha256", "firmware_unchanged_during_build")})
        builds[-1].update(executable=artifact(executable),
                          map=artifact(folder + "/SST_AnyEyes.map"),
                          build_report=artifact(report_path), log=artifact(folder + "/build.log"),
                          macro_verification=verify_macro_selection(args.cc, expected_defines, expected))
        build_report_paths.append(report_path)

    host = read("build/host_validation/result.json")
    require(host["firmware_input_sha256"] == initial_fingerprint,
            "Host validation belongs to different firmware inputs")
    require(host["firmware_unchanged_during_checks"] is True, "Firmware changed during host checks")
    require(len(host["checks"]) == 14 and {item["check"] for item in host["checks"]} == HOST_CHECKS,
            "Host validation must contain the exact 14 required checks")
    host_checks = []
    for item in host["checks"]:
        require(item["exit_code"] == 0, "Host check failed: " + item["check"])
        log = local_path(item["log"])
        require(relative(log) == "build/host_validation/" + item["check"] + ".log",
                "Wrong host log location: " + item["check"])
        require("PASS" in log.read_text(encoding="utf-8-sig"), "Missing host PASS: " + item["check"])
        host_checks.append({"check": item["check"], "exit_code": 0, "passed": True,
                            "log": artifact(log)})

    isolation = read("build/bench_isolation/result.json")
    require(isolation["baseline"] == BASELINE and isolation["baseline_resolved_commit"] == BASELINE,
            "Isolation baseline differs from saved bench baseline")
    require(isolation["firmware_input_sha256"] == initial_fingerprint and
            isolation["firmware_input_sha256_after"] == initial_fingerprint,
            "Isolation evidence belongs to changed or stale firmware")
    require(isolation["passed"] is True and len(isolation["results"]) == 38 and
            all(item["passed"] is True for item in isolation["results"]),
            "Isolation must contain 38 passing checks")
    for path, expected_sha in isolation["source_file_sha256"].items():
        require(sha(local_path(path)) == expected_sha, "Stale isolation source hash: " + path)
    fixture_paths = {"bench_isolation.py": "tests/bench_isolation.py",
                     "isr_binary_audit.py": "tests/isr_binary_audit.py",
                     "config_fixture.c": "build/bench_isolation/config_fixture.c"}
    require(set(isolation["fixture_sha256"]) == set(fixture_paths), "Wrong isolation fixture inventory")
    for name, expected_sha in isolation["fixture_sha256"].items():
        require(sha(local_path(fixture_paths[name])) == expected_sha, "Stale isolation fixture: " + name)
    require(len(target_compilers) == 1 and isolation["compiler"] in target_compilers,
            "Target compiler differs between full builds and isolation audit")
    compiler_path = Path(isolation["compiler"])
    require(compiler_path.is_file() and sha(compiler_path) == isolation["compiler_sha256"],
            "Target compiler binary differs from isolation audit")

    map_audits = read("build/bench_isolation/dpram_maps.json")
    require(len(map_audits) == 7 and {relative(local_path(item["path"])) for item in map_audits}
            == expected_map_paths, "Map audit must cover all seven target outputs")
    checked_maps = []
    for item in map_audits:
        path = local_path(item["path"])
        require(item["passed"] is True and item["errors"] == [], "Map audit failed: " + str(path))
        require(sha(path) == item["sha256"], "Stale map audit: " + str(path))
        require(item["dpram_enabled"] == ("anyeyes_remote_dpram" in path.parts),
                "Unexpected DPRAM section selection: " + str(path))
        checked_maps.append(dict(item, path=relative(path)))

    comments = read("build/comment_inventory/verification.json")
    require(comments["custom_source_files"] == 130 and len(comments["files"]) == 130 and
            comments["failures"] == [], "Comment coverage must contain 130 clean source files")
    comment_hashes = {}
    for item in comments["files"]:
        require(sha(local_path(item["file"])) == item["annotated_sha256"],
                "Stale comment source hash: " + item["file"])
        comment_hashes[item["file"]] = item["annotated_sha256"]

    key_sources = set(isolation["source_file_sha256"]) | {
        "Source/04_App/04_Control/ChbDabControl.h", "C6748.cmd",
        ".project", ".cproject", ".ccsproject"}
    source_hashes = {path: sha(local_path(path)) for path in sorted(key_sources)}
    fixture_snapshot = {}
    for folder in ("tests", "tools"):
        for path in sorted((ROOT / folder).rglob("*")):
            if path.is_file() and path.suffix.lower() in (".py", ".c", ".cpp", ".h", ".json"):
                fixture_snapshot[relative(path)] = sha(path)
    report_paths = build_report_paths + ["build/host_validation/result.json",
        "build/bench_isolation/result.json", "build/bench_isolation/dpram_maps.json",
        "build/comment_inventory/verification.json"]
    require(fingerprint(ROOT) == initial_fingerprint, "Firmware changed while exporting evidence")
    report = {
        "created_utc": datetime.now(timezone.utc).isoformat(), "project": "AnyEyesVersion",
        "baseline_commit": BASELINE, "baseline_tag": BASELINE_TAG,
        "baseline_firmware_input_sha256": isolation["baseline_firmware_input_sha256"],
        "delivery_tag": DELIVERY_TAG, "firmware_input_sha256": initial_fingerprint,
        "scope": "Default-off single-UNIT bench input generation for CCS closed-loop flow debugging; "
                 "independent balanced grid/current samples and editable phase DC inputs",
        "target_build_count": 7, "target_builds": builds,
        "target_compiler": {"path": str(compiler_path), "sha256": sha(compiler_path)},
        "macro_verification_compiler": {"path": str(Path(args.cc).resolve()),
                                         "sha256": sha(Path(args.cc).resolve())},
        "host_validation": {"passed": True, "check_count": 14,
            "firmware_unchanged_during_checks": True, "checks": host_checks},
        "bench_isolation": {"passed": True, "check_count": 38,
            "method": isolation["method"], "limitations": isolation["limitations"],
            "results": isolation["results"], "source_file_sha256": isolation["source_file_sha256"],
            "fixture_sha256": isolation["fixture_sha256"]},
        "dpram_map_audits": checked_maps,
        "comment_coverage": {"custom_source_files": 130, "failures": [],
            "symbol_counts": comments["symbol_counts"], "source_file_sha256": comment_hashes,
            "report": artifact("build/comment_inventory/verification.json")},
        "key_source_file_sha256": source_hashes, "current_test_tool_sha256": fixture_snapshot,
        "test_tool_hash_scope": "Isolation fixture hashes are checked against its execution report. "
            "The host runner records firmware fingerprint and logs but no execution-time fixture "
            "hashes; current_test_tool_sha256 records the exporter-time file snapshot.",
        "report_sha256": {path: sha(local_path(path)) for path in report_paths},
        "limitations": [
            "No physical board, power-circuit dynamics, real ADC/FO/FB signal or hardware fault-path validation.",
            "Host integration replaces only hardware boundaries; the virtual current is an independent "
            "waveform and does not respond to PWM or PI outputs.",
            "Macro-off lexical and normalized TI ISR assembly comparisons establish the checked "
            "source/instruction equivalence, not full executable byte identity or measured 100 us latency.",
            "Macro-on adds waveform calculation to the 10 kHz path; physical interrupt duration and "
            "missed-interrupt measurements remain outstanding.",
            "This export validates existing reports and current hashes; it does not rebuild firmware "
            "or rerun behavioral test suites.",
            "Delivery identity uses the release tag and firmware input fingerprint, avoiding a "
            "self-referential final commit hash. The tag can be applied after the summary is committed.",
        ],
        "physical_board_validation_performed": False,
    }
    destination = ROOT / "docs/evidence" / args.output
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("Exported:", destination)
    print("PASS: 7 builds, 14 host checks, 38 isolation checks, 7 maps, 130 comment source hashes")
    print("Firmware SHA256:", initial_fingerprint)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
        raise SystemExit("Bench evidence export failed: " + str(error))
