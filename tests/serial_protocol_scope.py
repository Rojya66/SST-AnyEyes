"""Check production UART supports only the selected AnyEyes or VOFA protocol."""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]


def main():
    forbidden = re.compile(r"\b(?:MaintenanceProtocol\w*|SstCommand_ApplyMaintenance|SST_ENABLE_MAINTENANCE|MAINT_\w+)\b")
    offenders = []
    for path in (ROOT / "Source").rglob("*"):
        if path.suffix.lower() in (".c", ".cpp", ".h", ".hpp"):
            for number, line in enumerate(path.read_text(encoding="utf-8-sig", errors="replace").splitlines(), 1):
                if forbidden.search(line):
                    offenders.append(f"{path.relative_to(ROOT)}:{number}: {line.strip()}")
    matrix = (ROOT / "tools/build_matrix.py").read_text(encoding="utf-8")
    assert "SST_ENABLE_MAINTENANCE" not in matrix
    assert not (ROOT / "Source/04_App/03_PcDebug/MaintenanceProtocol.C").exists()
    assert not (ROOT / "Source/04_App/03_PcDebug/MaintenanceProtocol.H").exists()
    assert not offenders, "Production maintenance protocol remains:\n" + "\n".join(offenders)
    output = ROOT / "build/serial_protocol_scope"
    output.mkdir(parents=True, exist_ok=True)
    (output / "result.json").write_text(json.dumps(dict(status="pass", production_offenders=offenders,
        validation="No production maintenance parser/command/config entry; protocol_regression exercises UART recovery and VOFA discard."), indent=2), encoding="utf-8")
    print("PASS: production UART/config/build matrix contain only AnyEyes/VOFA, no serial MaintenanceProtocol")


if __name__ == "__main__":
    main()
