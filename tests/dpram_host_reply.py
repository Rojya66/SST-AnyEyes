"""Run the production V2 Host on a fake DPRAM address space."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    compiler = shutil.which("gcc")
    assert compiler, "Host GCC required"
    source = ROOT / "Source"
    include_dirs = sorted(p for p in source.rglob("*") if p.is_dir())
    fixture = Path(__file__).with_suffix(".c")
    control = source / "04_App/04_Control/SstCommand.c"
    with tempfile.TemporaryDirectory(prefix="sst_host_reply_") as folder:
        temp = Path(folder)
        binary = temp / "host_reply.exe"
        # The TI project uses both cases for this header on Windows.
        shutil.copyfile(source / "02_Peripheral/01_DPRAM/DPRAM.H", temp / "DPRAM.h")
        for enabled, simulator, phase in ((1, 0, 1), (1, 0, 0), (1, 1, 1), (0, 0, 1)):
            subprocess.run([compiler, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unknown-pragmas", "-Dc6748", f"-DSST_ENABLE_DPRAM={enabled}",
                            f"-DSST_SIMULATOR_MODE={simulator}", f"-DSST_PHASE_A_ONLY_TEST={phase}",
                            "-I", str(temp), *[flag for path in include_dirs for flag in ("-I", str(path))],
                            str(fixture), *([str(control)] if enabled else []), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    main()
