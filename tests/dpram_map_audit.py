"""Verify V2 DPRAM placement, reserved mailboxes and NOINIT in TI linker maps."""
from pathlib import Path
import argparse
import hashlib
import json
import re


PROJECT = "SST_CHB_V1.0"
DPRAM_BASE = 0x66000000
DPRAM_END = 0x66004000
DPRAM_DATA_SIZE = 0x3FFC


def inspect(path, dpram_enabled=None):
    data = path.read_bytes()
    text = data.decode("utf-8", errors="replace")
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
    # TI prints short section names on the same line and longer names on the
    # preceding line. Include other sections in case one overlaps the MMIO range.
    section_map = text.partition("SECTION ALLOCATION MAP")[2].partition("GLOBAL SYMBOLS")[0]
    section_pattern = (
        r'^\.([\w.$]+)[ \t]*(?:\r?\n)?[ \t]*\*?[ \t]*\d+'
        r'[ \t]+([0-9a-fA-F]{8})[ \t]+([0-9a-fA-F]{8})([^\r\n]*)'
    )
    sections = {name: (int(origin, 16), int(size, 16), attributes.strip())
                for name, origin, size, attributes in
                re.findall(section_pattern, section_map, re.MULTILINE)}
    dpram_sections = {}
    for name, (origin, size, attributes) in sections.items():
        if name.startswith("DPRAM_") or (origin < DPRAM_END and origin + size > DPRAM_BASE):
            dpram_sections[name] = (origin, size)
            if name != "DPRAM_V2":
                errors.append(f"Unexpected DPRAM section {name}: 0x{origin:08X}, 0x{size:X} bytes")
            elif (origin, size, attributes) != (DPRAM_BASE, DPRAM_DATA_SIZE, "UNINITIALIZED"):
                errors.append("DPRAM_V2 must be NOINIT at 0x66000000 for 0x3FFC bytes")
    if dpram_enabled is None:
        dpram_enabled = "DPRAM_V2" in dpram_sections
    if dpram_enabled != ("DPRAM_V2" in dpram_sections):
        errors.append("DPRAM_V2 retention does not match the build configuration")

    hardware_expected = {
        "FPGA_InterfaceFile": (0x60000000, 0x1E00),
        "FT3_PTCT_Register": (0x60001E00, 0x40),
        "FT3_FaultRecorder_Register": (0x60001E40, 0x40),
        "FT3_EER2_Register": (0x60001E80, 0x40),
        "FT3_Backup_Register": (0x60001EC0, 0x40),
    }
    hardware_sections = {}
    for name, (origin, size, attributes) in sections.items():
        if name == "FPGA_InterfaceFile" or name.startswith("FT3_"):
            location = (origin, size)
            hardware_sections[name] = location
            if hardware_expected.get(name) != location or attributes != "UNINITIALIZED":
                errors.append(f"Unexpected FPGA/FT3 layout for {name}: {location}")
    if "FPGA_InterfaceFile" not in hardware_sections:
        errors.append("Missing retained FPGA_InterfaceFile")

    segments = text.partition("SEGMENT ALLOCATION MAP")[2].partition("SECTION ALLOCATION MAP")[0]
    for run, load, length, init in re.findall(
            r'^[ \t]*([0-9a-fA-F]{8})[ \t]+([0-9a-fA-F]{8})[ \t]+'
            r'([0-9a-fA-F]{8})[ \t]+([0-9a-fA-F]{8})[ \t]+[rwx-]{3}(?=[ \t]|$)',
            segments, re.MULTILINE):
        run, load, length, init = (int(value, 16) for value in (run, load, length, init))
        in_dpram = (run < DPRAM_END and run + length > DPRAM_BASE or
                    load < DPRAM_END and load + init > DPRAM_BASE)
        if in_dpram and init:
            errors.append(f"DPRAM segment 0x{run:08X} has loadable file bytes")
    return {"path": str(path.resolve()), "sha256": hashlib.sha256(data).hexdigest(),
            "retained_sections": len(dpram_sections), "dpram_enabled": dpram_enabled,
            "dpram_sections": dpram_sections,
            "fpga_ft3_sections": hardware_sections,
            "errors": errors, "passed": not errors}


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("maps", type=Path, nargs="*")
    parser.add_argument("--delivery", action="store_true", help="audit all current matrix and default/CCS maps")
    parser.add_argument("--dpram", choices=("auto", "enabled", "disabled"), default="auto",
                        help="require V2 NOINIT when enabled; auto reads adjacent build metadata")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.delivery:
        variants = json.loads((root / "build/matrix_result.json").read_text())
        paths = [Path(item["executable"]).with_suffix(".map") for item in variants]
        paths += [root / folder / f"{PROJECT}.map" for folder in
                  ("build/Debug/default", "build/Release/default", "Debug", "Release")]
    else:
        paths = args.maps or [root / "build/Debug/default" / f"{PROJECT}.map"]
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
