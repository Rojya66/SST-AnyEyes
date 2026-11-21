"""Run the real software-I2C driver against an edge-level BL24C512A model."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    compiler = shutil.which("g++")
    assert compiler, "Host g++ required"
    driver = ROOT / "Source/02_Peripheral/02_Storage"
    with tempfile.TemporaryDirectory(prefix="sst_bl24c512a_") as tmp:
        work = Path(tmp)
        for name in ("EEProm.H", "EepromConfig.h"):
            shutil.copy2(driver / name, work / name)
        (work / "SstTypes.h").write_text("/* GPIO-only fixture; no DSP typedefs required. */\n")
        (work / "SstBoardConfig.h").write_text("#define SYSCLK_1_FREQ 456000000U\n")
        (work / "soc_C6748.h").write_text("#define SOC_GPIO_0_REGS 0U\n")
        (work / "gpio.h").write_text(
            "#pragma once\n#define GPIO_DIR_INPUT 1U\n#define GPIO_DIR_OUTPUT 0U\n"
            "void GPIOPinWrite(unsigned,unsigned,unsigned);\n"
            "void GPIODirModeSet(unsigned,unsigned,unsigned);\nint GPIOPinRead(unsigned,unsigned);\n")
        template = Path(__file__).with_suffix(".cpp").read_text(encoding="utf-8")
        unit = work / "driver_test.cpp"
        unit.write_text(template.replace("/* DRIVER */", (driver / "EEProm.C").read_text(encoding="utf-8-sig")), encoding="utf-8")
        binary = work / "driver_test.exe"
        subprocess.run([compiler,"-std=c++11","-O2","-Wall","-Wextra",str(unit),"-o",str(binary)], check=True)
        run = subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
        assert run.returncode == 0, run.stdout + run.stderr
        print(run.stdout,end="")


if __name__ == "__main__":
    main()
