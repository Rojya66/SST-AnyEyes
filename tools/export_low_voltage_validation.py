"""Verify existing final reports and export portable 100 Vac delivery evidence.

This command does not build firmware or rerun tests. It refuses stale source
fingerprints, missing checks, changed report inputs, and mismatched executables.
"""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys

from firmware_inputs import fingerprint


ROOT = Path(__file__).resolve().parents[1]
DELIVERY_TAG = "low-voltage-100vac-20261019"
HOST_CHECKS = {
    "realtime_audit", "control_regression", "pi_regression", "fault_reporting",
    "dpram_commands", "parameter_precision", "protocol_regression",
    "refactor_equivalence", "comment_coverage", "framework_structure",
    "firmware_trace", "serial_protocol_scope", "parameter_store_migration",
    "dpram_maintenance", "bench_inputs", "single_phase_control",
    "single_phase_io", "single_phase_isolation", "vofa_telemetry",
    "common_control_safety",
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def local_path(value):
    path = Path(str(value).replace("\\", "/"))
    path = path if path.is_absolute() else ROOT / path
    path = path.resolve()
    require(path.is_relative_to(ROOT), "Evidence path escapes project: " + str(value))
    return path


def relative(value):
    return local_path(value).relative_to(ROOT).as_posix()


def read(value):
    return json.loads(local_path(value).read_text(encoding="utf-8-sig"))


def sha(value):
    return hashlib.sha256(local_path(value).read_bytes()).hexdigest()


def artifact(value):
    path = local_path(value)
    return {"path": relative(path), "size_bytes": path.stat().st_size,
            "sha256": sha(path)}


def validate_inputs(report, key):
    """Validate exactly the per-file hashes actually recorded by a report."""
    recorded = report[key]
    require(isinstance(recorded, dict) and bool(recorded), "Missing input hashes: " + key)
    for name, digest in recorded.items():
        require(sha(name) == digest, "Stale report input: " + name)
    return {relative(name): digest for name, digest in recorded.items()}


def collect_cli(source_hash):
    name = "build/control_compatibility_matrix.json"
    matrix = read(name)
    require(matrix["case_count"] == matrix["passed"] == 13 and matrix["failed"] == 0,
            "CLI matrix must contain thirteen passing builds")
    require(matrix["requested_matrix_count"] == 12 and
            matrix["additional_default_case_count"] == 1,
            "CLI matrix must cover twelve combinations and the phase-only override")
    require(matrix["fingerprints"] == [source_hash] and
            matrix["all_same_firmware_fingerprint"] is True and
            matrix["all_sources_unchanged_during_build"] is True,
            "CLI matrix is stale or firmware changed during builds")
    expected = {(configuration, "audit_" + profile + "_" + protocol)
                for configuration in ("Debug", "Release")
                for profile in ("three_phase_high", "single_phase_high", "single_phase_low")
                for protocol in ("anyeyes", "vofa")}
    expected.add(("Debug", "audit_three_phase_default"))
    rows = matrix["results"]
    require(len(rows) == 13 and
            {(row["configuration"], row["variant"]) for row in rows} == expected,
            "Wrong or duplicate CLI build combinations")
    builds = []
    for row in rows:
        result_path = row["result_path"]
        result = read(result_path)
        require(result == row["result"], "Matrix/per-build report mismatch: " + result_path)
        require(row["exit_code"] == 0 and result["compile_failures"] == [] and
                result["link_status"] == 0, "Failed CLI build: " + result_path)
        require(result["firmware_input_sha256"] == source_hash and
                result["firmware_unchanged_during_build"] is True,
                "Stale or modified CLI build: " + result_path)
        require(result["configuration"] == row["configuration"] and
                result["variant"] == row["variant"] and
                result["defines"] == row["defines"], "CLI metadata mismatch: " + result_path)
        folder = "build/" + row["configuration"] + "/" + row["variant"]
        require(relative(result["executable"]) == folder + "/SST_AnyEyes.out",
                "Unexpected CLI executable: " + result_path)
        executable = artifact(result["executable"])
        require(executable["sha256"] == row["output_sha256"] and
                executable["size_bytes"] == row["output_size_bytes"],
                "CLI executable differs from collected matrix hash: " + folder)
        require(bool(row["output_hash_collected_utc"]), "CLI hash collection time missing")
        log = local_path(folder + "/build.log").read_text(encoding="utf-8-sig")
        require("error #" not in log, "Compiler error in CLI log: " + folder)
        builds.append({
            "configuration": row["configuration"], "variant": row["variant"],
            "profile": row["profile"], "protocol": row["protocol"],
            "defines": row["defines"], "source_count": result["source_count"],
            "warnings": result["warnings"], "warning_codes": row["warning_codes"],
            "firmware_input_sha256": source_hash, "firmware_unchanged_during_build": True,
            "executable": executable, "map": artifact(folder + "/SST_AnyEyes.map"),
            "report": artifact(result_path), "log": artifact(folder + "/build.log"),
            "output_hash_collected_utc": row["output_hash_collected_utc"],
            "output_hash_scope": "Existing artifact hashed after the successful build; "
                                 "checked again during this export.",
        })
    return {"passed": True, "build_count": len(builds), "builds": builds,
            "report": artifact(name)}


def collect_ccs(source_hash):
    name = "build/ccs_validation/result.json"
    result = read(name)
    summary_name = "build/ccs_validation/summary.json"
    summary = read(summary_name)
    for report in (result, summary):
        require(report["firmware_input_sha256"] == source_hash and
                report["firmware_unchanged_during_build"] is True,
                "CCS report is stale or firmware changed during build")
    require(result["results"] == summary["results"], "CCS result/summary mismatch")
    require([row["action"] for row in result["results"]] == ["import", "Debug", "Release"],
            "CCS must import and clean-build both configurations")
    actions = []
    for row in result["results"]:
        action = row["action"]
        require(row["exit_code"] == 0, "CCS action failed: " + action)
        log_name = "build/ccs_validation/" + action + ".log"
        log = local_path(log_name).read_text(encoding="utf-8-sig")
        item = {"action": action, "exit_code": 0, "log": artifact(log_name)}
        if action == "import":
            require("Importing project" in log and "Done!" in log, "CCS import confirmation missing")
        else:
            require(row["clean_exit_code"] == 0, "CCS clean failed: " + action)
            recorded = summary["configurations"][action]
            executable = artifact(recorded["output"])
            require(executable["path"] == action + "/SST_AnyEyes.out" and
                    executable["sha256"] == recorded["output_sha256"] and
                    executable["size_bytes"] == recorded["output_size_bytes"],
                    "CCS executable differs from summary: " + action)
            require(recorded["error_diagnostics"] == [] and
                    recorded["headless_projects_with_errors"] == 0 and
                    "0 out of 1 projects have errors" in log and
                    "Finished building target:" in log and "error #" not in log,
                    "CCS build completion or error check failed: " + action)
            item.update(clean_exit_code=0, executable=executable,
                        map=artifact(action + "/SST_AnyEyes.map"),
                        warnings=recorded["warning_count"], warning_codes=recorded["warning_codes"])
        actions.append(item)
    return {"passed": True, "action_count": 3, "build_count": 2, "actions": actions,
            "report": artifact(name), "summary": artifact(summary_name)}


def collect_host(source_hash):
    name = "build/host_validation/result.json"
    result = read(name)
    require(result["firmware_input_sha256"] == source_hash and
            result["firmware_unchanged_during_checks"] is True,
            "Host checks are stale or firmware changed during checks")
    require(len(result["checks"]) == 20 and
            {row["check"] for row in result["checks"]} == HOST_CHECKS,
            "Host report must contain exactly twenty required checks")
    checks = []
    for row in result["checks"]:
        require(row["exit_code"] == 0, "Host check failed: " + row["check"])
        log = row["log"]
        require(relative(log) == "build/host_validation/" + row["check"] + ".log",
                "Unexpected host log path")
        content = local_path(log).read_text(encoding="utf-8-sig")
        require("PASS" in content, "Missing host PASS output: " + row["check"])
        checks.append({"check": row["check"], "exit_code": 0, "log": artifact(log),
                       "output": content})
    return {"passed": True, "check_count": len(checks),
            "firmware_unchanged_during_checks": True, "checks": checks,
            "report": artifact(name)}


def collect_supplementary(source_hash):
    reports = {}
    name = "build/single_phase_isolation/result.json"
    isolation = read(name)
    require(isolation["passed"] is True and
            isolation["firmware_input_sha256"] == source_hash and
            all(row["passed"] is True for row in isolation["results"]),
            "Stale or failed TI macro/ISR isolation")
    reports["ti_macro_and_isr_isolation"] = {
        "passed": True, "check_count": len(isolation["results"]),
        "baseline": isolation["baseline_resolved_commit"],
        "firmware_input_sha256": isolation["firmware_input_sha256"],
        "comparison_defines": isolation["comparison_defines"],
        "results": isolation["results"], "exceptions": isolation["exceptions"],
        "limitations": isolation["limitations"], "report": artifact(name),
        "unchanged_scope": "This report records a fingerprint; the enclosing host runner "
                           "separately confirms firmware unchanged during the full suite.",
    }
    name = "build/refactor_equivalence.json"
    result = read(name)
    require(result["failures"] == [], "Refactor inventory contains failures")
    inputs = validate_inputs(result, "current_files")
    reports["refactor_inventory"] = {
        "passed": True, "baseline_commit": result["baseline_commit"],
        "original_entities": result["original_entities"], "current_entities": result["current_entities"],
        "entity_groups": result["entity_groups"], "current_file_sha256": inputs,
        "method": result["method"], "limitations": result["limitations"], "report": artifact(name),
    }
    name = "build/comment_inventory/verification.json"
    result = read(name)
    require(result["failures"] == [], "Custom symbol comments are incomplete")
    require(len(result["files"]) == result["custom_source_files"], "Comment file count mismatch")
    for row in result["files"]:
        require(sha(row["file"]) == row["annotated_sha256"], "Stale comment audit: " + row["file"])
    reports["comment_coverage"] = {
        "passed": True, "custom_source_files": result["custom_source_files"],
        "symbol_counts": result["symbol_counts"], "scope": result["scope"],
        "method": result["method"], "report": artifact(name),
    }
    for label, name in (
        ("framework_structure", "build/framework_structure/result.json"),
        ("startup_and_isr_trace", "build/firmware_trace/result.json"),
        ("protocol", "build/protocol_host/result.json"),
    ):
        result = read(name)
        inputs = validate_inputs(result, "input_sha256")
        if label == "framework_structure":
            require(result["failures"] == [] and
                    all(row["passed"] for row in result["assertions"]) and
                    all(row["exit_code"] == 0 for row in result["headers"]),
                    "Framework structure/header check failed")
            counts = {"assertions": len(result["assertions"]), "headers": len(result["headers"])}
        else:
            require(result["status"] == "pass" and
                    all(row["status"] == "pass" for row in result["variants"]),
                    "Supplementary variant failed: " + label)
            counts = {"variants": len(result["variants"])}
            if label == "protocol":
                require(all(row["rejected"] for row in result["configuration_checks"]),
                        "Invalid identity macro unexpectedly accepted")
                counts["invalid_identity_rejections"] = len(result["configuration_checks"])
        reports[label] = {"passed": True, "counts": counts, "input_sha256": inputs,
                          "limitations": result["limitations"], "report": artifact(name)}
    for label, name in (("vofa", "build/vofa_telemetry/result.json"),
                        ("serial_protocol_scope", "build/serial_protocol_scope/result.json")):
        result = read(name)
        require(result["status"] == "pass", "Supplementary report failed: " + name)
        reports[label] = {"passed": True, "report": artifact(name), "recorded_result": result,
                          "input_hash_scope": "No independent firmware fingerprint in this report; "
                                              "covered by the twenty-check host runner fingerprint."}
    return reports


def main():
    source_hash = fingerprint(ROOT)
    cli = collect_cli(source_hash)
    ccs = collect_ccs(source_hash)
    host = collect_host(source_hash)
    supplementary = collect_supplementary(source_hash)
    tools = {}
    for folder in ("tests", "tools"):
        for path in sorted((ROOT / folder).rglob("*")):
            if path.is_file() and path.suffix.lower() in (".py", ".c", ".cpp", ".h", ".json"):
                tools[relative(path)] = sha(path)
    require(fingerprint(ROOT) == source_hash, "Firmware changed during evidence export")
    report = {
        "created_utc": datetime.now(timezone.utc).isoformat(), "delivery_tag": DELIVERY_TAG,
        "project": "AnyEyesVersion", "firmware_input_sha256": source_hash,
        "firmware_unchanged_during_export": True,
        "scope": "Shared PI and startup/protection repairs, three-phase/common DC_Log restoration, "
                 "100 Vac phase-A defaults, low-current sorting, twelve-channel VOFA diagnostics.",
        "cli_validation": cli, "ccs_validation": ccs, "host_validation": host,
        "supplementary_validation": supplementary,
        "current_test_tool_sha256": tools,
        "test_tool_hash_scope": "Tool hashes are collected at export time. The host runner records "
                                "firmware fingerprints and logs, not execution-time hashes of every test tool.",
        "physical_board_validation_performed": False,
        "limitations": [
            "No power-stage dynamics, physical UNIT gate outputs, real protection timing or closed-loop stability acceptance test.",
            "Unchanged ISR instructions/calls/own stack do not prove the complete 100 us control WCET, "
            "cache/EMIF stalls, missed interrupts or full stack high-water usage on hardware.",
            "100 Vac defaults are initial low-power commissioning values; voltage/current polarity, "
            "module topology, protection and current loop must be verified on the installed hardware.",
            "VOFA is a background 50 Hz status stream and not a simultaneous 10 kHz waveform capture.",
            "Reports without independent input hashes are attributed only to the enclosing host-run fingerprint.",
            "This export verifies existing evidence and output hashes; it does not rebuild or rerun tests.",
        ],
    }
    destination = ROOT / "docs/evidence/low_voltage_100vac_validation.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8", newline="\n")
    print("PASS: 13 CLI outputs, CCS import/2 clean outputs, 20 host checks and current supplementary inputs")
    print("Firmware SHA256:", source_hash)
    print("Evidence:", destination)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, TypeError) as error:
        print("Low-voltage evidence export failed:", error, file=sys.stderr)
        raise SystemExit(1)
