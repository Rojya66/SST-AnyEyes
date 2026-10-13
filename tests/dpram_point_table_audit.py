"""Audit published parallel points against real firmware tables and TI layout.

This test never regenerates its inputs. The checked-in CSV, source mapping and
fixed pre-extension Git layout are independent evidence, preventing a common
generator mistake from validating its own output.
"""
from pathlib import Path
import argparse
import csv
from decimal import Decimal
import hashlib
import io
import json
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "6415bad0169c5d3fd7132c5d4e713cd129b77e51"
EXTENSIONS = {
    "SstRequest": (0x66002C00, 128, "W"),
    "SstResponse": (0x66002D00, 128, "R"),
    "SstTelemetry": (0x66002E00, 128, "R"),
    "SstHistory": (0x66002F00, 512, "R"),
}


def read_csv(path):
    with path.open(encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def source_entries(source, table_name):
    body = re.search(r"static const SST_PARAMETER\s+" + table_name +
                     r"\[\]\s*=\s*\{(.*?)\};", source, re.S)
    assert body, "Missing real firmware table: " + table_name
    pattern = (r"\{\s*\(volatile void \*\)\s*&g_chb_dab_(command|params)\.(\w+),"
               r"\s*([\d.]+)F,\s*(-?\d+)L,\s*(-?\d+)L,\s*([01])U,\s*([01])U\s*\}")
    entries = re.findall(pattern, body.group(1))
    assert len(entries) == body.group(1).count("(volatile void *)"), "Unparsed mapping row"
    return [dict(field=field, scale=Decimal(scale), minimum=int(low), maximum=int(high),
                 wire_type="int16" if signed == "1" else "uint16")
            for _, field, scale, low, high, signed, _ in entries]


def audit_points(points, addresses):
    source = (ROOT / "Source/04_App/01_FuncCode/SstParameterTable.c").read_text(encoding="utf-8-sig")
    commands = source_entries(source, "commands")
    parameters = source_entries(source, "parameters")
    assert len(commands) == 8 and len(parameters) == 37
    enable = dict(field="g_chb_dab_protection_enable", scale=Decimal(1),
                  minimum=0, maximum=1, wire_type="uint16")
    legacy_names = ["OC_level_SET", "OC_time_SET", "OC_level_SET_L2", "OC_time_SET_L2",
                    "OV_level_SET", "OV_time_SET", "HA_L_Voltage_Precharge", "HA_H_Voltage"]
    legacy_source = (ROOT / "Source/04_App/01_FuncCode/SstLegacyParameters.c").read_text(encoding="utf-8-sig")
    for index, name in enumerate(legacy_names[:6]):
        assert re.search(r"case\s+" + str(index) + r"\s*:\s*return\s+" + name + r"\s*;", legacy_source)
    for index, name in enumerate(legacy_names[6:], 6):
        assert re.search(r"case\s+" + str(index) + r"\s*:\s*scaled\s*=\s*" + name + r"\s*\*\s*10\.0F", legacy_source)
        assert re.search(name + r"\s*=\s*\(float\)snapshot\[" + str(index) + r"\]\s*/\s*10\.0F", legacy_source)
    for index in (1, 3, 5):
        assert re.search(r"snapshot\[" + str(index) + r"\]\s*==\s*65535U", legacy_source)
    assert "snapshot[6] >= snapshot[7]" in legacy_source
    legacy = [dict(field=name, scale=Decimal(10 if index >= 6 else 1),
                   minimum=0 if index in (1, 3, 5, 6) else 1,
                   maximum=65534 if index in (1, 3, 5, 6) else 65535,
                   wire_type="uint16") for index, name in enumerate(legacy_names)]
    expected = commands + [enable] + parameters + legacy
    assert len(points) == len(expected) == 54
    by_address = {int(row["dsp_byte_address"], 16): row for row in addresses}
    for index, (row, real) in enumerate(zip(points, expected)):
        prefix = "point " + str(index) + ": "
        assert int(row["point"]) == int(row["write_point_value"]) == index, prefix + "index"
        assert row["field"] == real["field"], prefix + "member"
        assert Decimal(row["physical_scale"]) == real["scale"], prefix + "scale"
        assert (int(row["raw_min"]), int(row["raw_max"])) == (real["minimum"], real["maximum"]), prefix + "range"
        assert row["wire_type"] == real["wire_type"], prefix + "signedness"
        read_address = 0x66002D40 + 2 * index
        assert int(row["read_address"], 16) == read_address, prefix + "read address"
        assert int(row["write_index_address"], 16) == 0x66002C08, prefix + "index register"
        assert int(row["write_value_address"], 16) == 0x66002C0A, prefix + "value register"
        published = by_address[read_address]
        assert published["region"] == "SstResponse" and published["field"] == row["field"]
        assert published["wire_type"] == row["wire_type"] and published["host_permission"] == "R"
        assert row["host_permission"] == ("R" if index == 9 else "R / request-W"), prefix + "permission"
        assert row["persistent"] == ("no" if index in (0, 1, 3, 8, 9) else "v2"), prefix + "persistence"
        if index < 9:
            assert (int(row["anyeyes_group"]), int(row["anyeyes_index"])) == (0, index)
        elif index < 46:
            n = index - 9
            assert (int(row["anyeyes_group"]), int(row["anyeyes_index"])) == (1 if n < 34 else 2, n if n < 34 else n - 34)
        else:
            assert not row["anyeyes_group"] and not row["anyeyes_index"]
    return len(expected)


def audit_addresses(addresses):
    assert len(addresses) == 3038, "Expected 2142 original and 896 extension words"
    keys = [int(row["dsp_byte_address"], 16) for row in addresses]
    assert len(keys) == len(set(keys)), "Duplicate DSP address"
    for row, address in zip(addresses, keys):
        assert 0x66000000 <= address < 0x66004000 and address % 2 == 0
        assert int(row["dpram_byte_offset"], 16) == address - 0x66000000
        assert int(row["dpram_word_address"], 16) == (address - 0x66000000) // 2
    raw = subprocess.check_output(["git", "show", BASELINE + ":docs/DPRAM_ADDRESSES.csv"], cwd=ROOT)
    baseline = list(csv.DictReader(io.StringIO(raw.decode("utf-8-sig"))))
    current = [row for row in addresses if row["region"] not in EXTENSIONS]
    assert len(current) == len(baseline) == 2142
    # Functional descriptions may change; field ordering, width and address cannot.
    columns = ("region", "word_index", "field", "dsp_byte_address", "dpram_byte_offset", "dpram_word_address", "wire_type")
    assert [tuple(row[key] for key in columns) for row in current] == [tuple(row[key] for key in columns) for row in baseline], "Original layout changed"
    linker = (ROOT / "C6748.cmd").read_text(encoding="utf-8-sig")
    for region, (base, count, permission) in EXTENSIONS.items():
        rows = [row for row in addresses if row["region"] == region]
        assert len(rows) == count
        for index, row in enumerate(rows):
            assert int(row["word_index"]) == index
            assert int(row["dsp_byte_address"], 16) == base + 2 * index
            assert row["host_permission"] == permission
        memory = "CS5_SST_" + region.removeprefix("Sst").upper()
        match = re.search(memory + r"\s+o\s*=\s*(0x[\da-fA-F]+)\s+l\s*=\s*(0x[\da-fA-F]+)", linker)
        assert match and (int(match[1], 16), int(match[2], 16)) == (base, count * 2)
        assert re.search(r"\.DPRAM_" + region + r"\s*>\s*" + memory + r",\s*type\s*=\s*NOINIT", linker)
    return len(baseline)


def compile_layout(tool_root, output):
    compiler = tool_root / "bin/cl6x.exe"
    assert compiler.is_file(), "TI C6000 compiler not found: " + str(compiler)
    header = ROOT / "Source/02_Peripheral/01_DPRAM"
    with tempfile.TemporaryDirectory(prefix="sst_parallel_layout_") as directory:
        work = Path(directory)
        source = work / "dpram_layout.c"
        source.write_text('#include "SstDpramExtension.h"\n'
                          'typedef char word_width[(sizeof(uint16_t)==2)?1:-1];\n'
                          'typedef char request_size[(sizeof(DPRAM_SstRequest)==256)?1:-1];\n'
                          'typedef char response_size[(sizeof(DPRAM_SstResponse)==256)?1:-1];\n'
                          'typedef char telemetry_size[(sizeof(DPRAM_SstTelemetry)==256)?1:-1];\n'
                          'typedef char history_size[(sizeof(DPRAM_SstHistory)==1024)?1:-1];\n'
                          'typedef char request_count[(SST_DPRAM_REQUEST_WORDS==16)?1:-1];\n'
                          'typedef char point_count[(SST_DPRAM_POINT_COUNT==54)?1:-1];\n'
                          'typedef char mailbox_block[(SST_DPRAM_MAINTENANCE_BLOCK==21)?1:-1];\n', encoding="utf-8")
        run = subprocess.run([str(compiler), "-mv6740", "--compile_only", "--display_error_number",
                              "--include_path=" + str(tool_root / "include"),
                              "--include_path=" + str(header),
                              "--output_file=" + str(work / "layout.obj"), str(source)],
                             cwd=ROOT, capture_output=True, text=True, timeout=30)
        (output / "ti_layout.log").write_text(run.stdout + run.stderr, encoding="utf-8")
        assert run.returncode == 0, "TI sizeof/layout compilation failed: " + run.stdout + run.stderr
        return run.returncode


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--tool-root", type=Path, default=Path(os.environ.get("SST_CGT_ROOT", "E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12")))
    args = parser.parse_args()
    output = ROOT / "build/dpram_point_table_audit"
    output.mkdir(parents=True, exist_ok=True)
    addresses = read_csv(ROOT / "docs/DPRAM_ADDRESSES.csv")
    points = read_csv(ROOT / "docs/DPRAM_PARAMETERS.csv")
    legacy_count = audit_addresses(addresses)
    point_count = audit_points(points, addresses)
    status = compile_layout(args.tool_root, output)
    checked_paths = ["docs/DPRAM_ADDRESSES.csv", "docs/DPRAM_PARAMETERS.csv", "C6748.cmd",
                     "Source/04_App/01_FuncCode/SstParameterTable.c", "Source/04_App/01_FuncCode/SstLegacyParameters.c",
                     "Source/02_Peripheral/01_DPRAM/SstDpramExtension.h"]
    result = dict(status="pass", address_rows=len(addresses), legacy_rows_unchanged=legacy_count,
                  baseline=BASELINE, points_matched_to_source=point_count, ti_layout_compile=status,
                  sha256={path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in checked_paths})
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
