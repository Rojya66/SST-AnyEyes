"""Validate current AnyEyes identity delivery reports without rebuilding firmware.

The six CLI builds, CCS import and both builds, eight map audits, fourteen host
checks, protocol evidence, isolation evidence and comment hashes must all match
current inputs. Historical evidence files are preserved. The planned delivery
tag avoids embedding a self-referential final commit hash in its own commit.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
from io import BytesIO
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from zipfile import ZipFile

from firmware_inputs import fingerprint


ROOT = Path(__file__).resolve().parents[1]
PRECHANGE_COMMIT = "fc3c917c35d2832b8c34d5516858d7cf1df30cb2"
ISOLATION_BASELINE = "6675a727fc5424b49e860b72acdf2c49e9181b9b"
DELIVERY_TAG = "anyeyes-identity-2026-10-14"
VERSION_DEFINES = ["SST_PRODUCT_SERIES=201", "SST_PRODUCT_PART=202",
                   "SST_HARDWARE_PLATFORM=203", "SST_VERSION_BRANCH=204",
                   "SST_VERSION_RELEASE=205", "SST_VERSION_BETA=206"]
VERSION_NAMES = ["SST_PRODUCT_SERIES", "SST_PRODUCT_PART", "SST_HARDWARE_PLATFORM",
                 "SST_VERSION_BRANCH", "SST_VERSION_RELEASE", "SST_VERSION_BETA"]
BUILD_SELECTIONS = [
    ("Debug", "anyeyes_identity", []),
    ("Release", "anyeyes_identity", []),
    ("Debug", "anyeyes_identity_normal", ["SST_BENCH_INPUT_SIM=0"]),
    ("Debug", "anyeyes_identity_dpram", ["SST_BENCH_INPUT_SIM=0",
        "SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=1"]),
    ("Debug", "anyeyes_identity_vofa", ["SST_DEBUG_PROTOCOL=2"]),
    ("Debug", "anyeyes_identity_custom", VERSION_DEFINES),
]
HOST_CHECKS = {
    "realtime_audit", "control_regression", "fault_reporting", "dpram_commands",
    "parameter_precision", "protocol_regression", "refactor_equivalence",
    "comment_coverage", "framework_structure", "firmware_trace",
    "serial_protocol_scope", "parameter_store_migration", "dpram_maintenance",
    "bench_inputs",
}
PROTOCOL_VARIANTS = {
    "sim_anyeyes": ["SST_BENCH_INPUT_SIM=0", "SST_SIMULATOR_MODE=1", "SST_ENABLE_DPRAM=0"],
    "remote_anyeyes": ["SST_BENCH_INPUT_SIM=0", "SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=0"],
    "dpram_anyeyes": ["SST_BENCH_INPUT_SIM=0", "SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=1"],
    "sim_vofa": ["SST_BENCH_INPUT_SIM=0", "SST_DEBUG_PROTOCOL=2",
                 "SST_SIMULATOR_MODE=1", "SST_ENABLE_DPRAM=0"],
    "custom_tag_anyeyes": ["SST_BENCH_INPUT_SIM=0", "SST_SIMULATOR_MODE=1",
                          "SST_ENABLE_DPRAM=0", *VERSION_DEFINES],
}
HISTORICAL_FILENAMES = {
    "software_validation.json", "dpram_maintenance_validation.json",
    "dpram_mapping_audit.json", "protection_comment_correction.json",
    "bench_input_validation.json",
}


def require(condition: bool, message: str) -> None:
    """Reject missing, failed, stale or inconsistent evidence explicitly."""
    if not condition:
        raise ValueError(message)


def local_path(value: str | Path) -> Path:
    """Resolve a present evidence file and keep it inside the project."""
    path = Path(value)
    path = (path if path.is_absolute() else ROOT / path).resolve()
    require(path.is_relative_to(ROOT), "Evidence outside project: " + str(path))
    require(path.is_file(), "Missing evidence: " + str(path))
    return path


def relative(path: Path) -> str:
    """Return a portable project-relative slash-separated path."""
    return path.relative_to(ROOT).as_posix()


def sha(path: Path) -> str:
    """Fingerprint raw source, report, log, tool or output bytes."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def artifact(value: str | Path) -> dict:
    """Record a nonempty artifact's path, size and SHA256."""
    path = local_path(value)
    size = path.stat().st_size
    require(size > 0, "Empty evidence: " + str(path))
    return {"path": relative(path), "size_bytes": size, "sha256": sha(path)}


def read(value: str | Path) -> dict | list:
    """Load a UTF-8 JSON report without changing it."""
    return json.loads(local_path(value).read_text(encoding="utf-8-sig"))


def verify_hashes(hashes: dict, description: str) -> None:
    """Require execution-time input hashes to equal current raw file hashes."""
    require(bool(hashes), description + " hash inventory is empty")
    for path, expected in hashes.items():
        require(sha(local_path(path)) == expected, "Stale " + description + ": " + path)


def git_revision(value: str) -> str:
    """Resolve the commit underlying a revision or annotated tag."""
    return subprocess.check_output(["git", "rev-parse", value + "^{commit}"],
                                   cwd=ROOT, text=True).strip()


def saved_fingerprint(revision: str) -> str:
    """Apply the firmware-input fingerprint algorithm to an immutable Git snapshot."""
    archive = subprocess.check_output(["git", "archive", "--format=zip", revision], cwd=ROOT)
    digest = hashlib.sha256()
    metadata = {".project", ".cproject", ".ccsproject", "C6748.cmd"}
    with ZipFile(BytesIO(archive)) as snapshot:
        paths = sorted(name for name in snapshot.namelist() if not name.endswith("/") and
                       (name.startswith("Source/") or name in metadata))
        require(metadata.issubset(paths), "Saved snapshot is missing project metadata")
        for name in paths:
            digest.update(name.encode())
            digest.update(b"\0")
            digest.update(snapshot.read(name))
            digest.update(b"\0")
    return digest.hexdigest()


def verify_macros(compiler: str, defines: list[str], variant: str) -> dict:
    """Assert current build selection and all six identity values by preprocessing."""
    expected = {"SST_BENCH_INPUT_SIM": int(variant not in
        {"anyeyes_identity_normal", "anyeyes_identity_dpram"}),
        "SST_SIMULATOR_MODE": int(variant != "anyeyes_identity_dpram"),
        "SST_ENABLE_DPRAM": int(variant == "anyeyes_identity_dpram"),
        "CHB_LOAD_EEPROM_PARAMETERS": 0,
        "SST_DEBUG_PROTOCOL": 2 if variant == "anyeyes_identity_vofa" else 1}
    values = list(range(201, 207)) if variant == "anyeyes_identity_custom" else [255, 43, 1, 0, 1, 0]
    expected.update(zip(VERSION_NAMES, values))
    source = "".join('#include "' + (ROOT / path).as_posix() + '"\n' for path in
                     ("Source/04_App/00_Config/ChbBuildConfig.h", "Source/04_App/00_Version/SoftVersion.h"))
    for name, value in expected.items():
        source += f'#if {name} != {value}\n#error "Unexpected {name}"\n#endif\n'
    with tempfile.TemporaryDirectory(prefix="sst_anyeyes_identity_export_") as directory:
        fixture = Path(directory) / "effective_macros.c"
        fixture.write_text(source, encoding="utf-8")
        result = subprocess.run([compiler, "-E", "-x", "c", *["-D" + value for value in defines],
                                 str(fixture)], cwd=ROOT, capture_output=True, text=True)
    require(result.returncode == 0, "Effective macro verification failed: " + result.stderr)
    return {"effective_values": expected, "exit_code": result.returncode,
            "method": "GCC preprocessing-only #if assertions on current configuration and version headers",
            "fixture_sha256": hashlib.sha256(source.encode("utf-8")).hexdigest()}


def main() -> None:
    """Cross-check complete current reports before writing one delivery summary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="anyeyes_identity_validation.json",
                        help="JSON filename under docs/evidence; historical names cannot be overwritten")
    parser.add_argument("--cc", default=shutil.which("gcc"), help="GCC executable for preprocessing only")
    args = parser.parse_args()
    require(Path(args.output).name == args.output and args.output.endswith(".json"),
            "Output must be a JSON filename below docs/evidence")
    require(args.output not in HISTORICAL_FILENAMES, "Historical evidence cannot be overwritten")
    require(bool(args.cc), "GCC unavailable; supply --cc for preprocessing")
    current_fingerprint = fingerprint(ROOT)
    require(git_revision(PRECHANGE_COMMIT) == PRECHANGE_COMMIT, "Wrong immutable prechange commit")
    ancestor = subprocess.run(["git", "merge-base", "--is-ancestor", PRECHANGE_COMMIT, "HEAD"], cwd=ROOT)
    require(ancestor.returncode == 0, "Current HEAD does not descend from the saved prechange commit")
    prechange_fingerprint = saved_fingerprint(PRECHANGE_COMMIT)

    builds, report_paths, target_compilers, map_paths = [], [], set(), set()
    for configuration, variant, defines in BUILD_SELECTIONS:
        folder = f"build/{configuration}/{variant}"
        report_path = folder + "/build_result.json"
        report = read(report_path)
        require(report["configuration"] == configuration and report["variant"] == variant,
                "Wrong CLI build identity: " + report_path)
        require(sorted(report["defines"]) == sorted(defines), "Wrong CLI defines: " + report_path)
        require(report["source_count"] == 89, "Unexpected source count: " + report_path)
        require(report["compile_failures"] == [] and report["link_status"] == 0,
                "CLI compilation or link failed: " + report_path)
        require(report["firmware_unchanged_during_build"] is True and
                report["firmware_input_sha256"] == current_fingerprint, "Changed or stale CLI build: " + report_path)
        executable = local_path(report["executable"])
        require(relative(executable) == folder + "/SST_AnyEyes.out", "Wrong CLI output: " + report_path)
        map_paths.add(folder + "/SST_AnyEyes.map")
        target_compilers.add(report["compiler"])
        log = local_path(folder + "/build.log")
        require("error #" not in log.read_text(encoding="utf-8-sig"), "CLI log contains an error: " + str(log))
        builds.append({key: report[key] for key in ("configuration", "variant", "defines", "source_count",
            "compile_failures", "link_status", "warnings", "firmware_input_sha256", "firmware_unchanged_during_build")})
        builds[-1].update(executable=artifact(executable), map=artifact(folder + "/SST_AnyEyes.map"),
            build_report=artifact(report_path), log=artifact(log),
            macro_verification=verify_macros(args.cc, defines, variant))
        report_paths.append(report_path)

    ccs = read("build/ccs_validation/result.json")
    require(ccs["firmware_input_sha256"] == current_fingerprint and
            ccs["firmware_unchanged_during_build"] is True, "CCS report is stale or inputs changed")
    require(len(ccs["results"]) == 3 and [item["action"] for item in ccs["results"]] ==
            ["import", "Debug", "Release"], "CCS report must include import, Debug and Release in order")
    ccs_results = []
    for item in ccs["results"]:
        action = item["action"]
        require(item["exit_code"] == 0, "CCS step failed: " + action)
        log_path = f"build/ccs_validation/{action}.log"
        log_text = local_path(log_path).read_text(encoding="utf-8-sig")
        entry = {"action": action, "exit_code": 0, "log": artifact(log_path)}
        if action == "import":
            require("Importing project" in log_text and "Done!" in log_text, "CCS import confirmation missing")
        else:
            require(item["clean_exit_code"] == 0, "CCS clean failed: " + action)
            require("0 out of 1 projects have errors" in log_text and
                    "Finished building target:" in log_text, "CCS build completion missing: " + action)
            require("SoftVersion.obj" in log_text and "SstBenchInputs.obj" in log_text,
                    "CCS log omits current identity/bench objects: " + action)
            require("error #" not in log_text, "CCS log contains compiler error: " + action)
            map_paths.add(action + "/SST_AnyEyes.map")
            entry.update(clean_exit_code=0, executable=artifact(action + "/SST_AnyEyes.out"),
                         map=artifact(action + "/SST_AnyEyes.map"))
        ccs_results.append(entry)
    report_paths.append("build/ccs_validation/result.json")

    host = read("build/host_validation/result.json")
    require(host["firmware_input_sha256"] == current_fingerprint and
            host["firmware_unchanged_during_checks"] is True, "Host report is stale or inputs changed")
    require(len(host["checks"]) == 14 and {item["check"] for item in host["checks"]} == HOST_CHECKS,
            "Host report must contain exactly the fourteen required checks")
    host_checks = []
    for item in host["checks"]:
        require(item["exit_code"] == 0, "Host check failed: " + item["check"])
        log = local_path(item["log"])
        require(relative(log) == "build/host_validation/" + item["check"] + ".log", "Wrong host log location")
        require("PASS" in log.read_text(encoding="utf-8-sig"), "Host PASS missing: " + item["check"])
        host_checks.append({"check": item["check"], "exit_code": 0, "passed": True, "log": artifact(log)})
    report_paths.append("build/host_validation/result.json")

    protocol = read("build/protocol_host/result.json")
    require(protocol["status"] == "pass" and len(protocol["variants"]) == 5 and
            {item["variant"] for item in protocol["variants"]} == set(PROTOCOL_VARIANTS),
            "Protocol evidence must contain five passing identity/ownership variants")
    protocol_results = []
    for item in protocol["variants"]:
        name = item["variant"]
        require(item["status"] == "pass" and "PASS" in item["output"], "Protocol variant failed: " + name)
        require(sorted(item["defines"]) == sorted(PROTOCOL_VARIANTS[name]), "Wrong protocol defines: " + name)
        binary = local_path("build/protocol_host/" + name + ".exe")
        require(sha(binary) == item["binary_sha256"], "Stale protocol binary: " + name)
        protocol_results.append(dict(item, binary=artifact(binary)))
    expected_rejections = {(name, value) for name in VERSION_NAMES for value in (-1, 256)}
    rejection_checks = protocol["configuration_checks"]
    require(len(rejection_checks) == 12 and {(item["macro"], item["value"]) for item in rejection_checks}
            == expected_rejections and all(item["rejected"] is True for item in rejection_checks),
            "Protocol evidence must reject all twelve out-of-range macro configurations")
    verify_hashes(protocol["input_sha256"], "protocol input")
    required_protocol_inputs = {"Source/04_App/00_Version/SoftVersion.c", "Source/04_App/00_Version/SoftVersion.h",
        "Source/04_App/03_PcDebug/Comm2PcDebug.cpp", "tests/protocol_regression.py", "tests/protocol_regression.cpp"}
    require(required_protocol_inputs.issubset(protocol["input_sha256"]), "Identity protocol input inventory incomplete")
    report_paths.append("build/protocol_host/result.json")

    isolation = read("build/bench_isolation/result.json")
    require(isolation["baseline"] == ISOLATION_BASELINE and
            isolation["baseline_resolved_commit"] == ISOLATION_BASELINE, "Wrong fixed bench isolation baseline")
    require(isolation["firmware_input_sha256"] == current_fingerprint and
            isolation["firmware_input_sha256_after"] == current_fingerprint, "Isolation report is stale or inputs changed")
    require(isolation["passed"] is True and len(isolation["results"]) == 38 and
            all(item["passed"] is True for item in isolation["results"]), "Isolation must pass all thirty-eight checks")
    verify_hashes(isolation["source_file_sha256"], "isolation source")
    fixture_paths = {"bench_isolation.py": "tests/bench_isolation.py",
                     "isr_binary_audit.py": "tests/isr_binary_audit.py",
                     "config_fixture.c": "build/bench_isolation/config_fixture.c"}
    require(set(isolation["fixture_sha256"]) == set(fixture_paths), "Wrong isolation fixture inventory")
    for name, expected in isolation["fixture_sha256"].items():
        require(sha(local_path(fixture_paths[name])) == expected, "Stale isolation fixture: " + name)
    require(len(target_compilers) == 1 and isolation["compiler"] in target_compilers,
            "Isolation and CLI target compiler paths differ")
    compiler_path = Path(isolation["compiler"])
    require(compiler_path.is_file() and sha(compiler_path) == isolation["compiler_sha256"], "Target compiler hash changed")
    report_paths.append("build/bench_isolation/result.json")

    audits = read("build/anyeyes_identity/dpram_maps.json")
    require(len(audits) == 8 and {relative(local_path(item["path"])) for item in audits} == map_paths,
            "Map audit must cover all six CLI and both CCS outputs")
    map_results = []
    for item in audits:
        path = local_path(item["path"])
        require(item["passed"] is True and item["errors"] == [], "Map audit failed: " + str(path))
        require(sha(path) == item["sha256"], "Stale map audit: " + str(path))
        require(item["dpram_enabled"] == ("anyeyes_identity_dpram" in path.parts),
                "Wrong DPRAM extension selection: " + str(path))
        map_results.append(dict(item, path=relative(path)))
    report_paths.append("build/anyeyes_identity/dpram_maps.json")

    comments = read("build/comment_inventory/verification.json")
    require(comments["custom_source_files"] == 132 and len(comments["files"]) == 132 and
            comments["failures"] == [], "Comment coverage must verify all 132 custom source files")
    comment_hashes = {item["file"]: item["annotated_sha256"] for item in comments["files"]}
    require(len(comment_hashes) == 132, "Comment source file inventory contains duplicate paths")
    verify_hashes(comment_hashes, "comment source")
    report_paths.append("build/comment_inventory/verification.json")

    source_paths = set(isolation["source_file_sha256"]) | required_protocol_inputs - {
        "tests/protocol_regression.py", "tests/protocol_regression.cpp"}
    source_paths |= {"Source/04_App/03_PcDebug/Comm2PcDebug.hpp", "C6748.cmd", ".project", ".cproject", ".ccsproject"}
    source_hashes = {path: sha(local_path(path)) for path in sorted(source_paths)}
    tool_hashes = {}
    for folder in ("tests", "tools"):
        for path in sorted((ROOT / folder).rglob("*")):
            if path.is_file() and path.suffix.lower() in (".py", ".c", ".cpp", ".h", ".json"):
                tool_hashes[relative(path)] = sha(path)
    require(fingerprint(ROOT) == current_fingerprint, "Firmware inputs changed during evidence export")
    report = {
        "created_utc": datetime.now(timezone.utc).isoformat(), "project": "AnyEyesVersion",
        "prechange_commit": PRECHANGE_COMMIT, "prechange_firmware_input_sha256": prechange_fingerprint,
        "delivery_tag": DELIVERY_TAG, "firmware_input_sha256": current_fingerprint,
        "scope": "Centralized four-field electronic tag and six-field software identity, unchanged FF/7F layout, "
                 "added FA/7A version response, fresh-session task clearing and bounded queued replies",
        "default_electronic_tag_decimal": [255, 43, 1, 0], "default_software_version_decimal": [255, 43, 1, 0, 1, 0],
        "cli_build_count": 6, "cli_builds": builds,
        "ccs_validation": {"passed": True, "action_count": 3, "build_count": 2,
            "firmware_unchanged_during_build": True, "results": ccs_results,
            "report": artifact("build/ccs_validation/result.json")},
        "target_compiler": {"path": str(compiler_path), "sha256": sha(compiler_path)},
        "macro_verification_compiler": {"path": str(Path(args.cc).resolve()), "sha256": sha(Path(args.cc).resolve())},
        "host_validation": {"passed": True, "check_count": 14, "checks": host_checks,
            "firmware_unchanged_during_checks": True},
        "protocol_validation": {"passed": True, "variant_count": 5, "variants": protocol_results,
            "invalid_macro_rejection_count": 12, "configuration_checks": rejection_checks,
            "input_sha256": protocol["input_sha256"], "method": protocol["method"], "limitations": protocol["limitations"]},
        "bench_isolation": {"passed": True, "check_count": 38,
            "baseline_commit": ISOLATION_BASELINE,
            "baseline_firmware_input_sha256": isolation["baseline_firmware_input_sha256"],
            "method": isolation["method"], "limitations": isolation["limitations"],
            "results": isolation["results"], "source_file_sha256": isolation["source_file_sha256"],
            "fixture_sha256": isolation["fixture_sha256"]},
        "dpram_map_audit_count": 8, "dpram_map_audits": map_results,
        "comment_coverage": {"custom_source_files": 132, "failures": [], "symbol_counts": comments["symbol_counts"],
            "source_file_sha256": comment_hashes, "report": artifact("build/comment_inventory/verification.json")},
        "key_source_file_sha256": source_hashes, "current_test_tool_sha256": tool_hashes,
        "test_tool_hash_scope": "Protocol and isolation execution reports provide hashes checked against current inputs. "
            "The full host runner records firmware fingerprint and logs; current_test_tool_sha256 captures other tools at export time.",
        "report_sha256": {path: sha(local_path(path)) for path in report_paths},
        "limitations": [
            "No physical UNIT board, real RS485 electrical/turnaround timing or installed AnyEyes user-interface acceptance test.",
            "Electronic tags select matching product configuration; full-FF universal connection is retained and is not authentication.",
            "FA/7A is a completed SST response using the reference reserved command IDs; host support for it still requires UI verification.",
            "Queued transport frames are preserved at reconnect; ResetSession cancels unqueued tasks and subscriptions, not control run commands.",
            "The saved user configuration enables bench inputs by default; disabling SST_BENCH_INPUT_SIM restores hardware sampling. "
            "Bench input generation is not a power-circuit dynamic model.",
            "Source and normalized ISR assembly isolation do not prove hardware interrupt latency or the 100 us deadline. "
            "The bench waveform calculation still requires physical timing and missed-interrupt measurements.",
            "This export validates existing reports and hashes; it does not rebuild or rerun behavioral suites.",
            "The planned delivery tag and firmware fingerprint identify the final delivery without a self-referential final commit hash.",
        ],
        "physical_board_validation_performed": False, "real_anyeyes_ui_validation_performed": False,
    }
    destination = ROOT / "docs/evidence" / args.output
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("Exported:", destination)
    print("PASS: 6 CLI builds, CCS import/2 builds, 14 host checks, 5 protocol variants/12 rejects, "
          "38 isolation checks, 8 maps, 132 comment source hashes")
    print("Firmware SHA256:", current_fingerprint)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
        raise SystemExit("AnyEyes identity evidence export failed: " + str(error))
