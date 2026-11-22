"""Exercise real FPGA accessors, fault classification and legacy telemetry against fake registers."""
from pathlib import Path
import csv
import re
import shutil
import subprocess
import tempfile
from host_modules import prepare_headers

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    """Extract one unchanged production function body; replace only peripheral memory."""
    match = re.search(r'^(?:static )?(?:void|ushort)\s+' + name + r'\s*\([^;{}]*\)\s*\{.*?^\}',
                      source, re.M | re.S)
    assert match, name
    return match.group(0)


def main():
    compiler = shutil.which('gcc')
    assert compiler, 'Host GCC required'
    app = ROOT / 'Source/04_App'
    io = (ROOT / 'Source/02_Peripheral/00_FPGA/FpgaIo.c').read_text(encoding='utf-8-sig')
    fault = (app / '08_Fault/FaultCheck.C').read_text(encoding='utf-8-sig')
    telemetry = (app / '10_Host/SstDpramTelemetry.c').read_text(encoding='utf-8-sig')
    telemetry = re.sub(r'^\s*#\s*include[^\n]*$', '', telemetry, flags=re.M)
    accessors = 'static uint8_t s_module_fo_phases = 7U;\n' + '\n'.join(function(io, name) for name in [
        'PhaseModuleMask', 'Port_EffectiveFOEnable', 'Port_EffectiveFBEnable',
        'Port_FO_EN_Set', 'Port_FB_EN_Set', 'Port_FOStatusRead', 'Port_FBStatusRead',
        'Port_AD_ProtectStatusRead'])
    body = Path(__file__).with_suffix('.c').read_text(encoding='utf-8')
    body = body.replace('/* PRODUCTION_ACCESSORS */', accessors)
    body = body.replace('/* PRODUCTION_FAULT_STEP */', function(fault, 'FaultCheckAndDispose_StepOne'))
    body = body.replace('/* PRODUCTION_TELEMETRY */', telemetry)
    # Use the real slot numbers, including the board's intentional AD/IO name swap.
    board = (ROOT / 'Source/03_System/00_Board/SstBoardConfig.h').read_text(encoding='utf-8-sig')
    slots = '\n'.join(re.findall(r'^#define\s+\w+_PluginUnit_\d+\s+\d+\s*$', board, re.M))
    body = body.replace('/* REAL_SLOT_NUMBERS */', slots)
    with tempfile.TemporaryDirectory(prefix='sst_fo_fault_') as directory:
        work = Path(directory)
        prepare_headers(ROOT, work)
        unit = work / 'fo_fault.c'
        unit.write_text(body, encoding='utf-8')
        for phase in [1, 0]:
            binary = work / f'fo_fault_{phase}.exe'
            run = subprocess.run([compiler, '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                                  '-Wno-unused-parameter', '-DHVAC_online',
                                  f'-DSST_PHASE_A_ONLY_TEST={phase}', '-I', str(work),
                                  str(unit), '-lm', '-o', str(binary)], capture_output=True, text=True)
            assert run.returncode == 0, run.stdout + run.stderr
            run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            assert run.returncode == 0, run.stdout + run.stderr
            print(f'phase={phase}: {run.stdout.strip()}')
    rows = list(csv.DictReader((ROOT/'docs/DPRAM_Screen_V11/DPRAM_MODBUS_POINTS.csv').open(encoding='utf-8-sig')))
    assert len(rows) == 2400
    assert len({row['modbus_hex'] for row in rows}) == len(rows)
    for address in [0x051E, 0x051F, 0x0520]:
        row = next(row for row in rows if row['modbus_decimal'] == str(address))
        assert row['host_access'] == 'R' and row['encoding'] == 'uint16 bitmap'
        assert int(row['dsp_byte'], 16) == 0x66000000 + 2 * address
        assert int(row['arm_byte'], 16) == 0x60000000 + 2 * address
    print('PASS: three read-only uint16 fault bitmaps retain the 33-word layout and exact ARM/DSP addresses')


if __name__ == '__main__':
    main()
