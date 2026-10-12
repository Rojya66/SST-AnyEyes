"""Verify linked DPRAM placement and the absence of runtime initialization images."""
from pathlib import Path
import argparse
import hashlib
import json
import re


def inspect(path):
    data = path.read_bytes()
    text = data.decode("utf-8", errors="replace")
    expected = {
        "DPRAM_Block1_remControl": (0x660001E0, 0x20),
        "DPRAM_Block2_remRegulate": (0x660003FE, 0x56),
        "DPRAM_Block3_remMeaSig": (0x66000A00, 0x42),
    }
    expected.update({f"DPRAM_Block4_PluginUnit{i}":
                     (0x66000D00 + (i - 2) * 0x200, 0x100) for i in range(2, 18)})
    errors = []
    if re.search(r'\.cinit[^\r\n]*DPRAM_|\.binit[^\r\n]*DPRAM_', text):
        errors.append("DPRAM startup initialization image present")
    sections = {}
    pattern = r'^\.(DPRAM_\w+)\s*\r?\n\*?\s+\d+\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})\s+UNINITIALIZED'
    for name, origin, size in re.findall(pattern, text, re.MULTILINE):
        location = (int(origin, 16), int(size, 16))
        sections[name] = location
        if expected.get(name) != location:
            errors.append(f"Unexpected layout for {name}: {location}")
    required = set(expected) - {"DPRAM_Block4_PluginUnit17"}
    if not required.issubset(sections):
        errors.append("Missing retained DPRAM sections: " + str(sorted(required - sections.keys())))
    hardware_expected = {
        "FPGA_InterfaceFile": (0x60000000, 0x1E00),
        "FT3_PTCT_Register": (0x60001E00, 0x40),
        "FT3_FaultRecorder_Register": (0x60001E40, 0x40),
        "FT3_EER2_Register": (0x60001E80, 0x40),
        "FT3_Backup_Register": (0x60001EC0, 0x40),
    }
    hardware_sections = {}
    hardware_pattern = r'^\.(FPGA_InterfaceFile|FT3_\w+)\s*\r?\n\*?\s+\d+\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})\s+UNINITIALIZED'
    for name, origin, size in re.findall(hardware_pattern, text, re.MULTILINE):
        location = (int(origin, 16), int(size, 16))
        hardware_sections[name] = location
        if hardware_expected.get(name) != location:
            errors.append(f"Unexpected FPGA/FT3 layout for {name}: {location}")
    # Unused FT3 plugins are removed by the linker in the selected HVAC build.
    for name in ("FPGA_InterfaceFile", "FT3_FaultRecorder_Register"):
        if name not in hardware_sections:
            errors.append("Missing retained FPGA/FT3 section: " + name)
    # TI map segment table: load-address, run-address, memory-size, file-size.
    for address, file_size in re.findall(
            r'^\s*(66[0-9a-fA-F]{6})\s+66[0-9a-fA-F]{6}\s+[0-9a-fA-F]{8}\s+([0-9a-fA-F]{8})\s+rw-',
            text, re.MULTILINE):
        if int(file_size, 16):
            errors.append(f"DPRAM segment {address} has loadable file bytes")
    return {"path": str(path.resolve()), "sha256": hashlib.sha256(data).hexdigest(),
            "retained_sections": len(sections), "fpga_ft3_sections": hardware_sections,
            "errors": errors, "passed": not errors}


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("maps", type=Path, nargs="*")
    parser.add_argument("--delivery", action="store_true", help="audit the 11 matrix and 4 default/CCS maps")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.delivery:
        variants = json.loads((root / "build/matrix_result.json").read_text())
        paths = [Path(item["executable"]).with_suffix(".map") for item in variants]
        paths += [root / item for item in ("build/Debug/default/SST_AnyEyes.map",
                  "build/Release/default/SST_AnyEyes.map", "Debug/SST_AnyEyes.map", "Release/SST_AnyEyes.map")]
    else:
        paths = args.maps or [root / "build/Debug/default/SST_AnyEyes.map"]
    results = [inspect(path) for path in paths]
    for result in results:
        print(("PASS" if result["passed"] else "FAIL") + f": {result['path']}: "
              f"{result['retained_sections']} DPRAM sections, " +
              ("fixed addresses, zero load bytes, no startup images" if result["passed"] else
               "; ".join(result["errors"])))
    if args.report:
        args.report.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    if not all(result["passed"] for result in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
