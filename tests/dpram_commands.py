"""Check actual DPRAM command adapter ownership and storage/stop interlocks."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from host_modules import prepare_headers


def main():
    project = Path(__file__).resolve().parents[1]
    compiler = shutil.which("gcc")
    if not compiler:
        raise SystemExit("Host GCC required")
    app = project / "Source/04_App"
    source = (app / "04_Control/SstCommand.c").read_text(encoding="utf-8-sig")
    source = re.sub(r'^\s*#\s*include\s*"[^"\n]+"\s*$', "", source, flags=re.MULTILINE)
    template = Path(__file__).with_suffix(".c").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory(prefix="sst_dpram_commands_") as directory:
        work = Path(directory)
        prepare_headers(project, work)
        unit = work / "commands_test.c"
        unit.write_text(template.replace("/* INSERT_COMMAND_IMPLEMENTATION */", source), encoding="utf-8")
        for name, defines in (
            ("simulator", ["SST_SIMULATOR_MODE=1", "SST_ENABLE_DPRAM=0"]),
            ("serial_owner", ["SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=0"]),
            ("simulator_with_dpram", ["SST_SIMULATOR_MODE=1", "SST_ENABLE_DPRAM=1"]),
            ("dpram_owner", ["SST_SIMULATOR_MODE=0", "SST_ENABLE_DPRAM=1"]),
        ):
            binary = work / (name + ".exe")
            command = [compiler, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                       *["-D" + value for value in defines], str(unit), "-o", str(binary)]
            subprocess.run(command, check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            if result.returncode:
                raise SystemExit(name + " failed:\n" + result.stdout + result.stderr)
            print(name + ": " + result.stdout.strip())


if __name__ == "__main__":
    main()
