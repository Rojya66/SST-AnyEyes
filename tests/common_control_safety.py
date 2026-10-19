"""Exercise common control safety with the production source in both phase modes.

Fake I/O permits precise ISR/background race injection; it does not model the
power stage, target ABI or execution time. Each profile is a separate binary.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def strip_includes(text):
    return re.sub(r'^\s*#\s*include\s*"[^"\n]+"\s*$', '', text,
                  flags=re.MULTILINE)


def main():
    compiler = shutil.which('gcc')
    if not compiler:
        raise SystemExit('GCC required')
    prefix = (ROOT / 'tests/control_regression.c').read_text(encoding='utf-8-sig')
    prefix = prefix.split('/* INSERT_CONTROL_IMPLEMENTATION */', 1)[0]
    # Instrument only the host MMIO substitute.  The real protection routine
    # clears controllers after committing shutdown, so the first gate write
    # captures whether PI changed on the fault sample before that reset.
    fake_signature = 'static void Module_PWMEN(unsigned short enable) {'
    assert prefix.count(fake_signature) == 1
    prefix = prefix.replace(fake_signature,
                            'static void fake_Module_PWMEN(unsigned short enable) {', 1)
    gate_probe = '''
static void common_capture_shutdown(void);
static void Module_PWMEN(unsigned short enable)
{
    if (enable == 0U)
    {
        common_capture_shutdown();
    }
    fake_Module_PWMEN(enable);
}
'''
    sources = [ROOT / p for p in (
        'Source/01_ComLib/04_Pi/SstPi.c',
        'Source/04_App/04_Control/SstSinglePhase.c',
        'Source/04_App/04_Control/ChbDabControl.c',
        'tests/common_control_safety.c')]
    headers = [ROOT / p for p in (
        'Source/01_ComLib/04_Pi/SstPi.h',
        'Source/04_App/00_Config/ChbBuildConfig.h',
        'Source/04_App/04_Control/ChbDabControl.h',
        'Source/04_App/04_Control/SstSinglePhase.h')]
    with tempfile.TemporaryDirectory(prefix='sst_common_safety_') as temp:
        work = Path(temp)
        for path in headers:
            shutil.copy2(path, work / path.name)
        pieces = [prefix, gate_probe, '#include "SstPi.h"\n#include "SstSinglePhase.h"\n']
        for path in sources:
            pieces.append(f'#line 1 "{path.as_posix()}"\n' +
                          strip_includes(path.read_text(encoding='utf-8-sig')) + '\n')
        unit = work / 'safety.c'
        unit.write_text('\n'.join(pieces), encoding='utf-8')
        for phase, low in ((0, 0), (1, 0), (1, 1), (0, None), (1, None)):
            expected_low = 0 if low is None else low  # Current checkout explicitly selects the high-voltage profile.
            binary = work / f'safety_{phase}_{low}.exe'
            command = [compiler, '-std=c99', '-O2', '-Wall', '-Wextra',
                       '-DSST_SIMULATOR_MODE=1', '-DSST_ENABLE_DPRAM=0',
                       '-DSST_BENCH_INPUT_SIM=0', f'-DSST_PHASE_A_ONLY_TEST={phase}',
                       f'-DSST_HOST_EXPECT_LOW_PROFILE={expected_low}']
            if low is not None:
                command.append(f'-DSST_LOW_VOLTAGE_100V_TEST={low}')
            command.extend([str(unit), '-lm', '-o', str(binary)])
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode:
                raise SystemExit(result.stdout + result.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
            profile_label = str(low) if low is not None else f'auto({expected_low})'
            print(f'PROFILE phase_a={phase} low_voltage={profile_label}\n{result.stdout}', end='')
            if result.returncode:
                raise SystemExit(result.stderr or result.returncode)
        # The low-voltage settings describe one physical A-phase fixture.
        invalid_command = [part for part in command[:command.index(str(unit))]
                           if not part.startswith(('-DSST_PHASE_A_ONLY_TEST=',
                                                   '-DSST_LOW_VOLTAGE_100V_TEST='))]
        invalid = subprocess.run(invalid_command + ['-DSST_PHASE_A_ONLY_TEST=0',
                                 '-DSST_LOW_VOLTAGE_100V_TEST=1', '-fsyntax-only', str(unit)],
                                 capture_output=True, text=True)
        assert invalid.returncode != 0 and 'requires SST_PHASE_A_ONLY_TEST' in invalid.stderr
        print('PASS unsupported low-voltage three-phase combination rejected')


if __name__ == '__main__':
    main()
