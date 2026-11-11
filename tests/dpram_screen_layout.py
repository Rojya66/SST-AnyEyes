"""Pin old addresses, the new NOINIT tail, CCS identity and the exact accepted bench ISR/control baseline."""
from pathlib import Path
import csv
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from source_equivalence import inventory


def baseline(path):
    return subprocess.check_output(['git','show','766fad0:'+path],cwd=ROOT)


def main():
    unchanged=['Source/03_System/02_ISR/Redef_Interrupt.c',
               'Source/04_App/02_Measurement/ADC_Sample.C',
               'Source/04_App/08_Fault/ProtectionConfig.h']
    for path in unchanged:
        assert (ROOT/path).read_bytes()==baseline(path),path
    path='Source/04_App/04_Control/ChbDabControl.c'
    old={x.kind+':'+x.name:x for x in inventory(baseline(path).decode('utf-8-sig'),path)}
    new={x.kind+':'+x.name:x for x in inventory((ROOT/path).read_text(encoding='utf-8-sig'),path)}
    for name,entry in old.items():
        assert name in new and entry.sha256==new[name].sha256, name
    assert set(new)-set(old)=={'function:ChbDabControl_MaintenanceSwitch'}
    name=ET.parse(ROOT/'.project').findtext('name')
    assert name==ROOT.name=='AnyEyesVersion_DpramCompat'
    cproject=ET.parse(ROOT/'.cproject')
    assert {x.get('artifactName') for x in cproject.findall('.//configuration') if x.get('artifactName')}=={'SST_DpramCompat'}
    linker=(ROOT/'C6748.cmd').read_text(encoding='utf-8-sig')
    assert re.search(r'CS5_SST_SCREEN\s+o\s*=\s*0x66003300\s+l\s*=\s*0x00000320',linker)
    assert re.search(r'\.DPRAM_SstScreen\s*>\s*CS5_SST_SCREEN,\s*type\s*=\s*NOINIT',linker)
    rows=list(csv.DictReader((ROOT/'docs/DPRAM_Screen_V11/DPRAM_MODBUS_POINTS.csv').open(encoding='utf-8-sig')))
    addresses=[int(row['modbus_hex'],16) for row in rows]
    assert len(addresses)==len(set(addresses))
    assert set(range(0xF0,0x100))<=set(addresses)
    assert set(range(0x1FF,0x22A))<=set(addresses)
    assert set(range(0x500,0x521))<=set(addresses)
    for row in rows:
        address=int(row['modbus_hex'],16)
        assert int(row['arm_byte'],16)==0x60000000+2*address
        assert int(row['dsp_byte'],16)==0x66000000+2*address
        assert int(row['display_40001'])==40001+address
    assert len(rows)==2400
    print(f'PASS: {len(old)} existing control entities unchanged; ISR/ADC/protection byte-identical; unique CCS name, NOINIT and 2400 address mappings')


if __name__=='__main__':main()
