"""Run the V2 publisher and fault history on an in-memory FPGA and DPRAM."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = ROOT / "Source"
    includes = sorted(p for p in source.rglob("*") if p.is_dir())
    units = [Path(__file__).with_suffix(".c"),
             source / "04_App/10_Host/SstDpramV2Telemetry.c",
             source / "04_App/08_Fault/SstFault.c"]
    compiler = shutil.which("gcc")
    assert compiler, "Host GCC required"
    with tempfile.TemporaryDirectory(prefix="sst_v2_telem_") as folder:
        binary = Path(folder) / "v2_telemetry.exe"
        for single_phase in (1, 0):
            subprocess.run([compiler, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                            "-Dc6748", f"-DSST_PHASE_A_ONLY_TEST={single_phase}",
                            *[item for p in includes for item in ("-I", str(p))],
                            *map(str, units), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    main()
