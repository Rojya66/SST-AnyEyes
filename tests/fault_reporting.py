"""Build the real background fault manager with scalar host substitutes."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    compiler = shutil.which("gcc")
    if compiler is None:
        raise SystemExit("GCC not found")
    source = project / "Source/04_App/08_Fault/SstFault.c"
    implementation = source.read_text(encoding="utf-8")
    implementation = re.sub(r'^\s*#\s*include\s*"[^"\n]+"\s*$', "",
                            implementation, flags=re.MULTILINE)
    template = Path(__file__).with_suffix(".c").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="sst_fault_reporting_") as folder:
        work = Path(folder)
        for header in (source.with_suffix(".h"),
                       project / "Source/04_App/04_Control/ChbDabControl.h"):
            shutil.copy2(header, work / header.name)
        unit = work / "fault_test.c"
        unit.write_text(template.replace("/* INSERT_FAULT_IMPLEMENTATION */",
                                         implementation), encoding="utf-8")
        binary = work / "fault_test.exe"
        subprocess.run([compiler, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                        str(unit), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == "__main__":
    main()
