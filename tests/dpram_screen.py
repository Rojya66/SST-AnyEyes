"""Execute production DPRAM screen adapters; replace only registers, EEPROM and external core services."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / 'Source/04_App'


def source(path):
    return re.sub(r'^\s*#\s*include[^\n]*$', '', path.read_text(encoding='utf-8-sig'), flags=re.M)


def main():
    compiler = shutil.which('gcc')
    assert compiler, 'Host GCC required'
    defaults = source(APP / '04_Control/ChbDabControl.c')
    defaults = defaults[defaults.index('volatile CHB_DAB_COMMAND g_chb_dab_command ='):]
    defaults = defaults[:defaults.index('static SST_PI_CONTROLLER')]
    modules = ['01_FuncCode/SstParameterTable.c', '01_FuncCode/SstLegacyParameters.c',
               '01_FuncCode/SstParameterStore.c', '04_Control/SstCommand.c',
               '10_Host/SstDpramParameter.c', '10_Host/SstDpramCommand.c',
               '10_Host/SstDpramTelemetry.c', '10_Host/SstDpramScreen.c']
    template = Path(__file__).with_suffix('.c').read_text(encoding='utf-8')
    body = template.replace('/* DEFAULTS */', defaults).replace(
        '/* IMPLEMENTATIONS */', '\n'.join(source(APP / name) for name in modules))
    with tempfile.TemporaryDirectory(prefix='sst_screen_') as tmp:
        work = Path(tmp)
        for folder in ['00_Config', '01_FuncCode', '04_Control', '10_Host']:
            for path in (APP / folder).glob('*.h'):
                shutil.copy2(path, work / path.name)
        unit = work / 'screen.c'
        unit.write_text(body, encoding='utf-8')
        for phase, simulator, dpram in [(1, 0, 1), (0, 0, 1), (1, 1, 1), (1, 0, 0)]:
            binary = work / f'screen_{phase}_{simulator}_{dpram}.exe'
            command = [compiler, '-std=c99', '-O2', '-Wall', '-Wextra',
                       '-Wno-unknown-pragmas', '-Wno-unused-variable', '-I', str(work),
                       '-DSST_PHASE_A_ONLY_TEST=' + str(phase),
                       '-DSST_LOW_VOLTAGE_100V_TEST=' + str(phase),
                       '-DSST_SIMULATOR_MODE=' + str(simulator), '-DSST_ENABLE_DPRAM=' + str(dpram),
                       str(unit), '-lm', '-o', str(binary)]
            run = subprocess.run(command, capture_output=True, text=True)
            assert run.returncode == 0, run.stdout + run.stderr
            run = subprocess.run([str(binary)], capture_output=True, text=True)
            assert run.returncode == 0, f'{phase}/{simulator}/{dpram}: ' + run.stdout + run.stderr
            print(f'phase={phase}, simulator={simulator}, DPRAM={dpram}: ' + run.stdout.strip())


if __name__ == '__main__':
    main()
