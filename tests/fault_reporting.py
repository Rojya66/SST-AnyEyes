"""Build the background fault manager with host substitutes for hardware reads."""
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    source = Path(__file__).resolve().parents[1] / "Source"
    compiler = shutil.which("gcc")
    if compiler is None:
        raise SystemExit("GCC not found")
    includes = sorted(p for p in source.rglob("*") if p.is_dir())
    units = [Path(__file__).with_suffix(".c"), source / "04_App/08_Fault/SstFault.c"]
    with tempfile.TemporaryDirectory(prefix="sst_fault_reporting_") as folder:
        binary = Path(folder) / "fault_test.exe"
        subprocess.run([compiler, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-Dc6748", "-DSST_PHASE_A_ONLY_TEST=0",
                        *[item for p in includes for item in ("-I", str(p))],
                        *map(str, units), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    main()
