"""Materialize required original migration inputs from an immutable Git tag."""
from pathlib import Path
import re
import subprocess

REFERENCE = 'baseline-pre-framework-2026-10-12'
FILES = {
    'ChbDabControl.c': 'Source/04_App/04_Control/ChbDabControl.c',
    'ChbDabControl.h': 'Source/04_App/04_Control/ChbDabControl.h',
    'ChbBuildConfig.h': 'Source/04_App/00_Config/ChbBuildConfig.h',
    'ADC_Sample.C': 'Source/04_App/02_Measurement/ADC_Sample.C',
    'HVAC_Strategy.C': 'Source/04_App/04_Control/HVAC_Strategy.C',
    'FaultCheck.C': 'Source/04_App/08_Fault/FaultCheck.C',
    'main.H': 'Source/04_App/00_Config/main.H',
    'PluginUnit.C': 'Source/02_Peripheral/00_FPGA/PluginUnit.C',
    'main_485test.c': 'Source/03_System/01_OS/main_485test.c',
    'C6748.cmd': 'C6748.cmd',
}


def materialize(project: Path, destination: Path, *, original_linker=False):
    revision = subprocess.run(['git','rev-parse',REFERENCE+'^{commit}'],cwd=project,check=True,
                              stdout=subprocess.PIPE).stdout.decode().strip()
    for name, path in FILES.items():
        data = subprocess.run(['git','show',revision+':'+path],cwd=project,check=True,
                              stdout=subprocess.PIPE).stdout
        if name == 'C6748.cmd' and original_linker:
            # Migration added only NOINIT to the 19 DPRAM sections. Reconstruct
            # the old linker's exact executable token stream for that invariant;
            # the immutable tagged content is untouched.
            text = data.decode('utf-8-sig')
            text,count = re.subn(r'(?m)^(\s*\.DPRAM_\w+\s*>\s*CS5_DPRAMBLOCK_\w+)\s*,\s*type\s*=\s*NOINIT\s*$',
                                r'\1',text)
            if count != 19:
                raise AssertionError('Git baseline must preserve the known 19-section NOINIT migration')
            data=text.encode('utf-8')
        (destination/name).write_bytes(data)
    return revision
