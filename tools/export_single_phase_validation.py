"""Export reproducible evidence for the optional phase-A SOGI change."""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path

from firmware_inputs import fingerprint

ROOT = Path(__file__).resolve().parents[1]
BUILDS = [
    ("Debug", "phase_a_bench"),
    ("Debug", "phase_a_real"),
    ("Release", "phase_a_real"),
    ("Debug", "phase_a_vofa_dpram"),
    ("Debug", "phase_abc_real"),
    ("Debug", "phase_abc_bench"),
]


def read(relative):
    return json.loads((ROOT / relative).read_text(encoding="utf-8"))


def evidence(relative):
    path = ROOT / relative
    return {"path": path.relative_to(ROOT).as_posix(),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    source_hash = fingerprint(ROOT)
    builds = []
    for configuration, variant in BUILDS:
        folder = Path("build") / configuration / variant
        report = read(folder / "build_result.json")
        assert report["firmware_input_sha256"] == source_hash, folder
        assert report["firmware_unchanged_during_build"], folder
        assert not report["compile_failures"] and report["link_status"] == 0, folder
        builds.append({"configuration": configuration, "variant": variant,
                       "result": report,
                       "artifacts": [evidence(folder / name) for name in
                                     ("SST_AnyEyes.out", "SST_AnyEyes.map", "build.log")]})
    host = read("build/host_validation/result.json")
    assert host["firmware_input_sha256"] == source_hash
    assert host["firmware_unchanged_during_checks"]
    assert len(host["checks"]) == 16
    assert all(check["exit_code"] == 0 for check in host["checks"])
    for check in host["checks"]:
        path = Path(check.pop("log"))
        check["log"] = path.relative_to(ROOT).as_posix()
        check["log_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
        check["output"] = path.read_text(encoding="utf-8")
    style = read("build/style_audit/result.json")
    assert not style["failures"]
    assert style["after"]["multiple_statement_lines"] == 0
    assert style["after"]["code_lines_over_120"] == 0
    for row in style["files"]:
        assert evidence(row["file"])["sha256"] == row["sha256"]
    report = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "implementation_baseline": "bcd8d1c",
        "firmware_input_sha256": source_hash,
        "builds": builds,
        "host_checks": host,
        "style": {key: style[key] for key in
                  ("formatter", "custom_source_files", "after", "failures")},
        "dpram_maps": read("build/single_phase_validation/dpram_maps.json"),
        "limitations": [
            "No power-stage model, board WCET measurement or gate-output measurement.",
            "Existing PI values and phase compensation retained; single-phase physical stability not established.",
            "SOGI beta lags alpha by 90 degrees; physical feedback/modulation polarity still needs low-voltage verification.",
        ],
    }
    isolation = read("build/single_phase_isolation/result.json")
    assert isolation["passed"]
    assert isolation["firmware_input_sha256"] == source_hash
    assert all(row["passed"] for row in isolation["results"])
    report["ti_macro_isolation"] = isolation
    report["isolation_report"] = evidence("build/single_phase_isolation/result.json")
    target = ROOT / "docs/evidence/single_phase_validation.json"
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(target)
    print("PASS: 6 current builds, 16 host checks, 134-file formatting, and DPRAM maps")


if __name__ == "__main__":
    main()
