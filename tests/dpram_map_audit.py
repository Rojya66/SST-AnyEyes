"""Verify linked DPRAM placement and the absence of runtime initialization images."""
from pathlib import Path
import argparse
import hashlib
import json
import re


def inspect(path, dpram_enabled=None):
    data = path.read_bytes()
    text = data.decode("utf-8", errors="replace")
    expected = {
        "DPRAM_Block1_remControl": (0x660001E0, 0x20),
        "DPRAM_Block2_remRegulate": (0x660003FE, 0x56),
        "DPRAM_Block3_remMeaSig": (0x66000A00, 0x42),
    }
    expected.update({f"DPRAM_Block4_PluginUnit{i}":
                     (0x66000D00 + (i - 2) * 0x200, 0x100) for i in range(2, 18)})
    extensions = {
        "DPRAM_SstRequest": (0x66002C00, 0x100), "DPRAM_SstResponse": (0x66002D00, 0x100),
        "DPRAM_SstTelemetry": (0x66002E00, 0x100), "DPRAM_SstHistory": (0x66002F00, 0x400),
    }
    expected.update(extensions)
    # Reproducible builds record exact definitions. Disabled extension objects
    # may be pruned, but every retained section must still match its fixed range.
    build_report = path.with_name("build_result.json")
    if dpram_enabled is None and build_report.exists():
        definitions = json.loads(build_report.read_text(encoding="utf-8"))["defines"]
        setting = next((definition.split("=", 1)[1] for definition in definitions
                        if definition.startswith("SST_ENABLE_DPRAM=")), None)
        if setting is None:
            project = Path(__file__).resolve().parents[1]
            config = (project / "Source/04_App/00_Config/ChbBuildConfig.h").read_text(encoding="utf-8")
            setting = re.search(r'^\s*#define\s+SST_ENABLE_DPRAM\s+([01])U?\s*$', config, re.MULTILINE)[1]
        dpram_enabled = setting.rstrip("Uu") == "1"
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
    required = set(expected) - set(extensions) - {"DPRAM_Block4_PluginUnit17"}
    if dpram_enabled is None:
        dpram_enabled = bool(set(extensions) & sections.keys())
    if dpram_enabled:
        required.update(extensions)
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
            "retained_sections": len(sections), "dpram_enabled": dpram_enabled,
            "retained_extension_sections": sorted(set(extensions) & sections.keys()),
            "fpga_ft3_sections": hardware_sections,
            "errors": errors, "passed": not errors}


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("maps", type=Path, nargs="*")
    parser.add_argument("--delivery", action="store_true", help="audit all current matrix and default/CCS maps")
    parser.add_argument("--dpram", choices=("auto", "enabled", "disabled"), default="auto",
                        help="require the four extension sections when enabled; auto reads adjacent build metadata")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.delivery:
        variants = json.loads((root / "build/matrix_result.json").read_text())
        paths = [Path(item["executable"]).with_suffix(".map") for item in variants]
        paths += [root / item for item in ("build/Debug/default/SST_AnyEyes.map",
                  "build/Release/default/SST_AnyEyes.map", "Debug/SST_AnyEyes.map", "Release/SST_AnyEyes.map")]
    else:
        paths = args.maps or [root / "build/Debug/default/SST_AnyEyes.map"]
    configured = None if args.dpram == "auto" else args.dpram == "enabled"
    results = [inspect(path, configured) for path in paths]
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
