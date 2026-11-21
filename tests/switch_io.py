"""Run production switch output/control functions with host MMIO and DIN substitutes.

The real register union is extracted from FpgaRegister.h. Covers the drawing
channel assignment, output-off, feedback-off and both phase configurations.
No board electrical timing or C6748 WCET is measured.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from single_phase_io import extract_function
from common_control_safety import strip_includes

ROOT = Path(__file__).resolve().parents[1]


def read(relative):
    return (ROOT / relative).read_text(encoding='utf-8-sig')


def main():
    # TI ABI offsets determine actual DSP byte addresses; host unsigned/bitfield ABI is not evidence.
    from bench_isolation import compile_unit
    probe = ROOT / 'build/switch_register_probe.c'
    probe.parent.mkdir(parents=True, exist_ok=True)
    probe.write_text('#include <stddef.h>\n#include "FpgaRegister.h"\n'
                     'typedef char check_union[(sizeof(XXG_SwitchINOUT)==2)?1:-1];\n'
                     'typedef char check_block[(sizeof(FPGA_InterfaceBLOCK)==0x200)?1:-1];\n'
                     'typedef char check_dout[(offsetof(FPGA_InterfaceBLOCK,DOUT)==0x40)?1:-1];\n'
                     'typedef char check_din[(offsetof(FPGA_InterfaceBLOCK,DIN)==0x1C0)?1:-1];\n'
                     'typedef char check_latch[(offsetof(FPGA_InterfaceBLOCK,DIN_LATCH)==0x1C2)?1:-1];\n',
                     encoding='utf-8')
    import os
    cgt = Path(os.environ.get('SST_CGT_ROOT',
        'E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12'))
    result = compile_unit(ROOT, probe, cgt, {}, ROOT / 'build/switch_register_probe')
    assert result.returncode == 0, 'TI switch-register ABI probe failed'
    board = read('Source/03_System/00_Board/SstBoardConfig.h')
    io_plugin = int(re.search(r'#define IO_PluginUnit_2\s+(\d+)', board).group(1))
    base = 0x60000000 + (io_plugin - 2) * 0x200
    print(f'PASS TI ABI and actual plugin {io_plugin}: DOUT {base+0x40:08X}; '
          f'DIN {base+0x1C0:08X}; DIN_LATCH {base+0x1C2:08X}')
    compiler = shutil.which('gcc')
    assert compiler, 'GCC required'
    registers = read('Source/02_Peripheral/00_FPGA/FpgaRegister.h')
    union = re.search(r'typedef union _XXG_SwitchINOUT\b.*?} XXG_SwitchINOUT;', registers, re.S).group()
    prefix = read('tests/control_regression.c').split('/* INSERT_CONTROL_IMPLEMENTATION */')[0]
    prefix = prefix.replace('typedef struct {\n    unsigned short Reference,',
                            'typedef unsigned short ushort;\n' + union +
                            '\ntypedef struct {\n    XXG_SwitchINOUT DOUT;\n    unsigned short Reference,')
    prefix, count = re.subn(r'static struct \{ struct \{ unsigned short Qf1ClosePulse, Qf1OpenPulse,\s*'
                           r'Kt1Hold, AcRunning, AcFault; } Dout; } Port_DOUT;',
                           'static XXG_SwitchINOUT Port_DOUT;', prefix)
    assert count == 1
    prefix, count = re.subn(r'static void Port_IO_Out\(void\) \{.*?\}', '', prefix, flags=re.S)
    assert count == 1
    output = extract_function(read('Source/02_Peripheral/00_FPGA/FpgaIo.c'), 'Port_IO_Out')
    support = '''
#define IO_PluginUnit_2 IO_PLUGIN_NUMBER
/* Host substitute for the fault word read by the production maintenance interlock. */
static struct { unsigned short FaultTypeFLAG16; } FaultTypeFLAG;
static unsigned short fake_din;
static unsigned int din_reads;
static unsigned short Port_DINRead(unsigned char latch)
{
    assert(latch == 0U);
    ++din_reads;
    assert(fake_interrupt_enabled != 0U);
    return fake_din;
}
'''
    support = support.replace('IO_PLUGIN_NUMBER', str(io_plugin))
    safety = read('tests/common_control_safety.c').replace('int main(void)', 'int unused_common_main(void)')
    pieces = [prefix, support, output, '#include "SstPi.h"\n#include "SstSinglePhase.h"']
    for relative in ('Source/01_ComLib/04_Pi/SstPi.c',
                     'Source/04_App/04_Control/SstSinglePhase.c',
                     'Source/04_App/04_Control/ChbDabControl.c'):
        pieces.append(strip_includes(read(relative)))
    pieces += [safety, read('tests/switch_io.c')]
    with tempfile.TemporaryDirectory(prefix='sst_switch_io_') as folder:
        work = Path(folder)
        for relative in ('Source/01_ComLib/04_Pi/SstPi.h',
                         'Source/04_App/04_Control/ChbDabControl.h',
                         'Source/04_App/04_Control/SstSinglePhase.h',
                         'Source/04_App/00_Config/ChbBuildConfig.h'):
            shutil.copy2(ROOT / relative, work / Path(relative).name)
        unit = work / 'switch_io.c'
        unit.write_text('\n'.join(pieces), encoding='utf-8')
        for phase in (0, 1):
            for output_on, feedback in ((0, 0), (1, 0), (1, 1)):
                defines = dict(SST_PHASE_A_ONLY_TEST=phase, SST_BENCH_INPUT_SIM=0,
                               SST_ENABLE_SWITCH_OUTPUT=output_on, SST_ENABLE_SWITCH_FEEDBACK=feedback)
                binary = work / 'switch_io.exe'
                command = [compiler, '-std=c99', '-O2', '-Wall', '-Wextra',
                           *[f'-D{k}={v}' for k, v in defines.items()], str(unit), '-lm', '-o', str(binary)]
                result = subprocess.run(command, capture_output=True, text=True)
                assert result.returncode == 0, result.stdout + result.stderr
                result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=15)
                assert result.returncode == 0, str(defines) + '\n' + result.stdout + result.stderr
                print(f'PASS switch_io phase={phase} output={output_on} feedback={feedback}: '
                      f'{result.stdout.strip()}')



if __name__ == '__main__':
    main()
