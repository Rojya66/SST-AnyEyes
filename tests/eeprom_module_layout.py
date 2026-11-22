"""Validate actual PM/NUM reconstruction, not a duplicate popcount implementation."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ["Source/02_Peripheral/00_FPGA/FpgaInit.c",
           "Source/02_Peripheral/00_FPGA/FpgaIo.c",
           "Source/04_App/04_Control/05_ControlOutput/PwmGate.c",
           "Source/02_Peripheral/00_FPGA/FpgaRegister.c",
           "Source/04_App/04_Control/ControlData.c",
           "Source/04_App/04_Control/PortStatusData.c",
           "Source/04_App/08_Fault/ProtectionData.c",
           "Source/04_App/10_Host/DpramHostData.c",
           "Source/04_App/10_Host/DpramDefaults.c",
           "Source/04_App/01_FuncCode/SstHardwareParameters.c"]


def main():
    compiler = shutil.which("gcc")
    assert compiler, "Host GCC required"
    includes = [ROOT / "Source/00_Device/include/c674x/c6748", ROOT]
    includes += sorted(p for p in (ROOT / "Source").rglob("*") if p.is_dir())
    with tempfile.TemporaryDirectory(prefix="sst_eeprom_layout_") as tmp:
        binary = Path(tmp) / "layout.exe"
        command = [compiler,"-std=c99","-O2","-ffunction-sections","-fdata-sections",
                   "-Wno-unknown-pragmas","-Wl,--gc-sections","-Dc6748",
                   *["-I"+str(p) for p in includes],
                   *[str(ROOT / p) for p in SOURCES],str(Path(__file__).with_suffix(".c")),"-o",str(binary)]
        built = subprocess.run(command,capture_output=True,text=True)
        assert built.returncode == 0, built.stdout + built.stderr
        run = subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
        assert run.returncode == 0, run.stdout + run.stderr
        print(run.stdout,end="")


if __name__ == "__main__":
    main()
