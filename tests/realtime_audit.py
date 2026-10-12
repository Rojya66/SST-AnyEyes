"""Static migration invariants, not a schedulability or hardware timing proof."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import re


def read(path: Path) -> str:
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "gb18030"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            pass
    return data.decode("latin1")


def uncomment(text: str) -> str:
    pattern = r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|/\*[\s\S]*?\*/|//[^\n]*'
    return re.sub(pattern, lambda match: match[1] or " ", text)


def tokens(text: str) -> list[str]:
    return re.findall(r'\w+|[^\s\w]', uncomment(text))


def exclude_literal_if_zero(text: str) -> str:
    """Drop literal #if 0 branches (legacy disabled code may have bad braces)."""
    result = []
    disabled_depth = 0
    for line in text.splitlines(True):
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b(.*)', line)
        if not disabled_depth:
            if directive and directive[1] == "if" and directive[2].strip() == "0":
                disabled_depth = 1
            else:
                result.append(line)
        elif directive:
            if directive[1] in ("if", "ifdef", "ifndef"):
                disabled_depth += 1
            elif directive[1] == "endif":
                disabled_depth -= 1
            elif directive[1] == "else" and disabled_depth == 1:
                disabled_depth = 0
    return "".join(result)


def function(text: str, name: str) -> str:
    clean = uncomment(text)
    match = re.search(r'\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', clean)
    if not match:
        raise ValueError(f"Function {name} is missing")
    start = clean.index("{", match.start())
    nesting = 1
    for position in range(start + 1, len(clean)):
        nesting += (clean[position] == "{") - (clean[position] == "}")
        if nesting == 0:
            return clean[start:position + 1]
    raise ValueError(f"Unbalanced function {name}")


def definitions(text: str) -> dict[str, str]:
    clean = exclude_literal_if_zero(uncomment(text))
    names = re.findall(r'^\s*(?:extern\s+"C"\s+)?(?:(?:static|inline|extern)\s+)*'
                       r'(?:[A-Za-z_]\w*\s+)+\**\s*([A-Za-z_]\w*(?:::[A-Za-z_]\w*)?)'
                       r'\s*\([^;{}]*\)\s*\{', clean, re.MULTILINE)
    result: dict[str, str] = {}
    for name in names:
        if name in ("if", "for", "while", "switch", "catch"):
            continue
        key = name.split("::")[-1]
        result[key] = result.get(key, "") + function(clean, name)
    return result


def select_dpram_branches(text: str, enabled: bool) -> str:
    """Select DPRAM gates and the fixed HVAC build; retain unrelated branches."""
    result = []
    stack: list[tuple[bool, bool | None]] = []
    active = True
    for line in uncomment(text).splitlines(True):
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b(.*)', line)
        if not directive:
            if active:
                result.append(line)
            continue
        kind, expression = directive[1], directive[2].strip()
        if kind in ("if", "ifdef", "ifndef"):
            known = None
            if kind in ("ifdef", "ifndef") and expression in (
                    "HVAC_online", "HVDC_online", "LVDC_online", "LVAC_online"):
                known = expression == "HVAC_online"
                if kind == "ifndef":
                    known = not known
            if kind == "if":
                if expression == "SST_ENABLE_DPRAM":
                    known = enabled
                elif expression == "!SST_ENABLE_DPRAM":
                    known = not enabled
                elif expression in ("0", "1"):
                    known = expression == "1"
                elif not enabled and re.fullmatch(r'\w+\s*&&\s*SST_ENABLE_DPRAM', expression):
                    known = False
            stack.append((active, known))
            active = active and known is not False
        elif kind == "else":
            parent, known = stack[-1]
            active = parent and known is not True
        elif kind == "elif":
            # Unrecognized alternate conditions are conservatively retained.
            active = stack[-1][0]
        elif kind == "endif":
            active = stack.pop()[0]
    if stack:
        raise ValueError("Unbalanced conditional directives")
    return "".join(result)


def closure(name: str, functions: dict[str, str]) -> set[str]:
    seen: set[str] = set()
    pending = [name]
    while pending:
        current = pending.pop()
        if current in seen:
            continue
        seen.add(current)
        pending.extend(re.findall(r'\b([A-Za-z_]\w*)\s*\(', functions.get(current, "")))
    return seen


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path,
                        default=Path(__file__).resolve().parents[2] / "Chb_DabPrim_vofa")
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    source = args.project / "Source"
    failures: list[str] = []
    checked = 0

    def check(condition: bool, label: str) -> None:
        nonlocal checked
        checked += 1
        print(("PASS " if condition else "FAIL ") + label)
        if not condition:
            failures.append(label)

    unchanged = {
        "ChbDabControl.c": "04_App/04_Control/ChbDabControl.c",
        "ChbDabControl.h": "04_App/04_Control/ChbDabControl.h",
        "ADC_Sample.C": "04_App/02_Measurement/ADC_Sample.C",
        "HVAC_Strategy.C": "04_App/04_Control/HVAC_Strategy.C",
        "FaultCheck.C": "04_App/08_Fault/FaultCheck.C",
        "main.H": "04_App/00_Config/main.H",
    }
    for name, relative in unchanged.items():
        original = read(args.baseline / name)
        migrated = read(source / relative)
        check(tokens(original) == tokens(migrated), f"identical executable tokens (comments excluded): {name} "
              f"sha256={hashlib.sha256(migrated.encode('utf-8')).hexdigest()[:16]}")
    baseline_linker = uncomment(read(args.baseline / "C6748.cmd"))
    migrated_linker = uncomment(read(args.project / "C6748.cmd"))
    noinit_line = r'(?m)^(\s*\.DPRAM_\w+\s*>\s*CS5_DPRAMBLOCK_\w+)\s*,\s*type\s*=\s*NOINIT\s*$'
    canonical_linker, noinit_count = re.subn(noinit_line, r'\1', migrated_linker)
    check(noinit_count == 19 and tokens(baseline_linker) == tokens(canonical_linker),
          "linker preserves all source layout tokens except 19 DPRAM NOINIT attributes")
    original_plugin = select_dpram_branches(read(args.baseline / "PluginUnit.C"), True)
    migrated_plugin = read(source / "02_Peripheral/00_FPGA/PluginUnit.C")
    migrated_plugin = re.sub(r'^\s*#\s*include\s*"ChbBuildConfig.h"\s*$', "",
                             migrated_plugin, flags=re.MULTILINE)
    check(tokens(original_plugin) == tokens(select_dpram_branches(migrated_plugin, True)),
          "DPRAM-enabled HVAC FPGA operations preserve every original token")
    fault_source = "DPRAM_Block3_remMeaSig.DPRAM_RemMeaSig.PORT_FAULT.PORT_FAULT16"
    check(original_plugin.count(fault_source) == 1 and
          tokens(original_plugin.replace(fault_source, "Port_RealFault.PORT_FAULT16")) ==
          tokens(select_dpram_branches(migrated_plugin, False)),
          "DPRAM-off FPGA operations differ only in the recorder local fault-word source")

    all_functions: dict[str, str] = {}
    no_dpram_functions: dict[str, str] = {}
    for path in source.rglob("*"):
        if path.suffix.lower() not in (".c", ".cpp"):
            continue
        # Device libraries contain unsupported peripheral examples, not app roots.
        if "00_Device" in path.parts or "01_Drive" in path.parts:
            continue
        for name, body in definitions(read(path)).items():
            # Conditional definitions are conservatively combined for call checks.
            all_functions[name] = all_functions.get(name, "") + body
        for name, body in definitions(select_dpram_branches(read(path), False)).items():
            no_dpram_functions[name] = no_dpram_functions.get(name, "") + body

    migrated_main = read(source / "03_System/01_OS/main_485test.c")
    mapped = set(re.findall(r'#\s*pragma\s+DATA_SECTION\(\s*(DPRAM_\w+)\s*,',
                            uncomment(migrated_main)))
    noinit_sections = set(re.findall(r'(?m)^\s*\.(DPRAM_\w+)\s*>\s*CS5_DPRAMBLOCK_\w+\s*,\s*type\s*=\s*NOINIT\s*$', migrated_linker))
    check(len(mapped) == 19 and noinit_sections == mapped,
          "all 19 mapped DPRAM objects have explicit linker NOINIT storage")
    selected_ports = re.findall(r'^\s*#\s*define\s+(\w+_online)\b',
                                uncomment(read(source / "04_App/00_Config/main.H")), re.MULTILINE)
    check(selected_ports == ["HVAC_online"], "DPRAM reachability analysis uses the configured HVAC port")
    no_dpram_reached = set().union(*(closure(name, no_dpram_functions)
                                  for name in ("main", "FPGAIsr", "FAULTIsr", "Timer2Isr")))
    dpram_access = re.compile(r'\b(?:DPRAM_Block[1-4]_\w+|DPRAM_Maibox_\w+|0x66[0-9A-Fa-f]{6})\b')
    live_accesses = sorted(name for name in no_dpram_reached
                          if dpram_access.search(no_dpram_functions.get(name, "")))
    check(not live_accesses,
          f"DPRAM-off startup/runtime/enabled ISRs cannot reach mapped memory: {live_accesses}")
    check("DPRAMIsr" not in no_dpram_functions.get("GPIOBankPinInterruptInit", ""),
          "DPRAM-off does not register its interrupt handler")

    baseline_main = read(args.baseline / "main_485test.c")
    check(tokens(function(baseline_main, "Initial_Variable")) ==
          tokens(all_functions.get("Initial_Variable", "")),
          "initial FPGA/module/TAD configuration unchanged")
    baseline_config = uncomment(read(args.baseline / "main.H"))
    migrated_config = uncomment(read(source / "04_App/00_Config/main.H"))
    for macro in ("ControlTAD", "TAD_PLL"):
        expression = rf'^\s*#\s*define\s+{macro}\s+([^\n]+)'
        old_match = re.search(expression, baseline_config, re.MULTILINE)
        new_match = re.search(expression, migrated_config, re.MULTILINE)
        check(bool(old_match and new_match) and
              tokens(old_match[1]) == tokens(new_match[1]),
              f"source timing macro {macro} preserved")
    original_tad = function(baseline_main, "FPGAIsr")
    original_tad = re.sub(
        r'static\s+unsigned\s+short\s+vofa_counter\s*=\s*0\s*;\s*'
        r'vofa_counter\+\+\s*;\s*if\s*\(\s*vofa_counter\s*>=\s*10\s*\)\s*'
        r'\{\s*vofa_counter\s*=\s*0\s*;\s*Flag_VofaSend\s*=\s*1\s*;\s*\}',
        "", original_tad)
    migrated_tad = all_functions.get("FPGAIsr", "")
    check(tokens(original_tad) == tokens(migrated_tad),
          "TAD preserves every source token except VOFA scheduling flag")
    check(tokens(function(baseline_main, "FAULTIsr")) ==
          tokens(all_functions.get("FAULTIsr", "")), "hardware fault ISR unchanged")
    check("ChbDabControl_FastStep" in migrated_tad,
          "fast control remains in FPGA ISR")
    fast_callers = [name for name, body in all_functions.items()
                    if re.search(r'\bChbDabControl_FastStep\s*\(', body)]
    check(fast_callers == ["FPGAIsr"], f"only FPGA ISR calls FastStep: {fast_callers}")

    platform = uncomment(read(source / "03_System/00_Board/ChbPlatform.c"))
    patterns = [
        (r'GPIOIntTypeSet\(\s*SOC_GPIO_0_REGS\s*,\s*69\s*,\s*GPIO_INT_TYPE_RISEDGE\s*\)', "GPIO69 TAD rising edge"),
        (r'IntRegister\(\s*C674X_MASK_INT5\s*,\s*FPGAIsr\s*\)', "INT5 TAD vector"),
        (r'IntEventMap\(\s*C674X_MASK_INT5\s*,\s*SYS_INT_GPIO_B4INT\s*\)', "GPIO bank4 -> INT5"),
        (r'IntRegister\(\s*C674X_MASK_INT4\s*,\s*FAULTIsr\s*\)', "INT4 hardware fault vector"),
        (r'IntEventMap\(\s*C674X_MASK_INT4\s*,\s*SYS_INT_GPIO_B8INT\s*\)', "GPIO bank8 -> INT4"),
    ]
    for pattern, label in patterns:
        check(re.search(pattern, platform) is not None, label)
    check("UARTInterruptInit" not in closure("main", all_functions),
          "startup/runtime cannot reach legacy UART2 ISR registration")
    init_runtime = all_functions.get("ChbApplication_InitRuntime", "")
    names = ["ChbDabControl_Init", "SstFault_Init", "SstDebug_Init",
             "GPIOBankPinInterruptInit"]
    positions = [init_runtime.find(name + "(") for name in names]
    check(all(position >= 0 for position in positions) and positions == sorted(positions),
          "control/fault/debug state initialized before TAD/FAULT IRQ enable")

    forbidden_realtime = re.compile(r'^(SstDebug_|SstAnyEyes_|SstSerial_|SstParams_|SstFault_|'
                                    r'VOFA_|MaintenanceProtocol_|UARTChar|UARTReceive)')
    for isr in ("FPGAIsr", "FAULTIsr", "Timer2Isr", "DPRAMIsr"):
        reached = closure(isr, all_functions)
        forbidden = sorted(name for name in reached if forbidden_realtime.search(name))
        check(not forbidden, f"{isr} cannot reach added communication/reporting: {forbidden}")
    runtime_roots = [name for name in all_functions
                     if re.match(r'^(?:Sst.*|ChbApplication)_Service\w*$', name)]
    check(bool(runtime_roots), "background service entry points found")
    reached = set().union(*(closure(name, all_functions) for name in runtime_roots))
    forbidden_masking = {"IntGlobalDisable", "_disable_interrupts", "IntGlobalRestore",
                         "_restore_interrupts", "IntDisable", "IntEnable", "IntEventMap",
                         "IntEventCombineAdd", "IntEventCombineRemove"}
    blocked = sorted(reached & forbidden_masking)
    blocked += sorted(name for name in reached if re.match(r'^(?:Cache|CACHE)', name))
    check(not blocked, f"background has no global interrupt masking call path: {blocked}")
    direct_masking = [name for name, body in all_functions.items()
                      if name.startswith(("Sst", "ChbApplication_Service")) and
                      re.search(r'\b(?:IntGlobalDisable|_disable_interrupts|CSR)\b', body)]
    check(not direct_masking, f"no direct global masking in migration services: {direct_masking}")

    print(f"\n{checked - len(failures)}/{checked} static invariants passed")
    print("LIMIT: call graph is conservative source analysis, not preprocessed binary analysis.")
    print("LIMIT: target ISR WCET, latency, cache/EMIF stalls and missed edges require hardware measurement.")
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
