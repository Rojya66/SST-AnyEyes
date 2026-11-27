"""Verify the V2 point layout, CCS identity and preservation of existing control logic."""
from pathlib import Path
import csv
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PROJECT = "SST_CHB_V1.0"
sys.path.insert(0, str(ROOT / 'tools'))
from source_equivalence import inventory


def baseline(path):
    return subprocess.check_output(['git', 'show', 'fd727eb:' + path], cwd=ROOT)


def source_changes(path):
    old = {(x.kind, x.name): x.sha256 for x in inventory(baseline(path).decode('utf-8-sig'), path)}
    new = {(x.kind, x.name): x.sha256 for x in inventory((ROOT / path).read_text(encoding='utf-8-sig'), path)}
    assert old.keys() <= new.keys(), old.keys() - new.keys()
    changed = {key for key in old if old[key] != new[key]}
    return len(old) - len(changed), changed


def main():
    unchanged = ['Source/04_App/02_Measurement/ADC_Sample.C',
                 'Source/04_App/08_Fault/ProtectionConfig.h']
    for path in unchanged:
        assert (ROOT / path).read_bytes() == baseline(path), path
    _, isr_changes = source_changes('Source/03_System/02_ISR/Redef_Interrupt.c')
    assert isr_changes <= {('function', 'Timer2Isr')}, isr_changes
    control_preserved, changed = source_changes('Source/04_App/04_Control/ChbDabControl.c')
    allowed_control_changes = {
        ('object', 'g_chb_dab_params'), ('object', 'g_chb_dab_command'),
        ('function', 'Chb_ReadSwitchFeedback'), ('function', 'Chb_ReadUnitLinks'),
        ('function', 'Chb_EnterState'), ('function', 'ChbDabControl_Init'),
        ('function', 'ChbDabControl_Service'),
    }
    assert changed <= allowed_control_changes, changed - allowed_control_changes

    assert ET.parse(ROOT / '.project').findtext('name') == PROJECT
    cproject = ET.parse(ROOT / '.cproject')
    assert {x.get('artifactName') for x in cproject.findall('.//configuration')
            if x.get('artifactName')} == {PROJECT}
    launch = ET.parse(ROOT / f'.launches/{PROJECT}.launch')
    assert any(attr.get('value') == '${build_artifact:' + PROJECT + '}'
               for attr in launch.findall('stringAttribute'))
    linker = (ROOT / 'C6748.cmd').read_text(encoding='utf-8-sig')
    assert re.search(r'CS5_DPRAM_V2\s+o\s*=\s*0x66000000\s+l\s*=\s*0x00003FFC', linker)
    assert re.search(r'CS5_DPRAM_MAILBOX\s+o\s*=\s*0x66003FFC\s+l\s*=\s*0x00000004', linker)
    assert re.findall(r'^\s*\.DPRAM_(\w+)\s*>', linker, re.MULTILINE) == ['V2']
    assert re.search(r'\.DPRAM_V2\s*>\s*CS5_DPRAM_V2,\s*type\s*=\s*NOINIT', linker)
    mailbox = (ROOT / 'Source/02_Peripheral/01_DPRAM/DPRAM.H').read_text(encoding='utf-8-sig')
    assert re.search(r'^#define\s+DPRAM_Maibox_Left\s+0x66003FFC\b', mailbox, re.MULTILINE)
    assert re.search(r'^#define\s+DPRAM_Maibox_Right\s+0x66003FFE\b', mailbox, re.MULTILINE)

    # Keep the old sheet as a historical record while checking the active V2 plan.
    path = 'docs/DPRAM_Screen_V11/DPRAM_MODBUS_POINTS.csv'
    assert (ROOT / path).read_bytes() == baseline(path)
    with (ROOT / 'docs/DPRAM_Screen_V2/DPRAM_MODBUS_POINTS.csv').open(encoding='utf-8-sig') as file:
        rows = list(csv.DictReader(file))
    used_words = set()
    symbols = {}
    for row in rows:
        address = int(row['字地址 HEX'], 16)
        width = int(row['占字'])
        symbol = row['符号']
        assert width == (2 if row['类型'] in ('U32', 'F32') else 1), symbol
        words = set(range(address, address + width))
        assert 0 <= address and address + width <= 0x2000 and not used_words & words, symbol
        assert symbol not in symbols, symbol
        used_words.update(words)
        symbols[symbol] = address
        assert int(row['PDU 十进制']) == address
        assert int(row['40001 号制']) == 40001 + address
        assert int(row['ARM 字节地址'], 16) == 0x60000000 + 2 * address
        assert int(row['DSP 字节地址'], 16) == 0x66000000 + 2 * address
    ranges = [(0x0100, 0x0103), (0x01FF, 0x023E), (0x0500, 0x050F),
              (0x0510, 0x0525), (0x0540, 0x0570), (0x0580, 0x05B3),
              (0x0600, 0x06A7), (0x0700, 0x0709), (0x0720, 0x073F),
              (0x0800, 0x0803), (0x0810, 0x0A4F), (0x1FFE, 0x1FFF)]
    assert used_words == {address for first, last in ranges for address in range(first, last + 1)}
    assert symbols['MB_LEFT'] == 0x1FFE and symbols['MB_RIGHT'] == 0x1FFF
    assert symbols['FAN_OFF_COUNT'] == 0x023D and symbols['FAN_ON_COUNT'] == 0x023E
    for phase_index, phase in enumerate('ABC'):
        assert symbols[f'FAN_CONFIG_CONFIRMED_{phase}'] == 0x05AD + phase_index
        assert symbols[f'FAN_CONFIG_REJECTED_{phase}'] == 0x05B0 + phase_index
    for phase_index, phase in enumerate('ABC'):
        for module in range(1, 15):
            base = 0x0600 + 56 * phase_index + 4 * (module - 1)
            for offset, name in enumerate(('UDC', 'IR', 'IG', 'TEMP')):
                assert symbols[f'{phase}{module:02}_{name}'] == base + offset
    for slot in range(32):
        base = 0x0810 + 18 * slot
        prefix = f'hist_{slot:02}'
        assert symbols[f'{prefix}_observation_tick'] == base + 11
        assert symbols[f'{prefix}_sequence'] == base + 13
        for offset, phase in enumerate('abc'):
            assert symbols[f'{prefix}_lost_{phase}'] == base + 15 + offset
    print(f'PASS: {control_preserved} control entities preserved; TAD/FAULT ISR, ADC and protection unchanged; '
          f'CCS identity, NOINIT, {len(rows)} V2 points/{len(used_words)} words and physical mailboxes')


if __name__ == '__main__':
    main()
