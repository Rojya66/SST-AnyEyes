"""Run the 501/UNIT UART and EMIFA regression with Icarus Verilog on PATH."""

from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory


def main():
    test_dir = Path(__file__).resolve().parent
    board_501 = test_dir.parent
    unit = board_501.parent / "UnitBoard_DAB_Test_FanBalance"
    sources_501 = sorted((board_501 / "source").glob("*.v"))

    # The two bitstreams have different implementations with the same names.
    # Rename the UNIT copies for a simulation of both boards in one process.
    shared_names = {path.stem for path in sources_501}
    pattern = rb"\b(" + b"|".join(name.encode() for name in sorted(shared_names)) + rb")\b"

    with TemporaryDirectory(prefix="sst-fan-test-") as temp:
        temp_dir = Path(temp)
        unit_sources = []
        for source in sorted((unit / "source").glob("*.v")):
            renamed = temp_dir / source.name
            renamed.write_bytes(re.sub(pattern, rb"unit_\1", source.read_bytes()))
            unit_sources.append(renamed)

        simulation = temp_dir / "fan_thresholds.vvp"
        subprocess.run(
            [
                "iverilog", "-g2012", "-s", "tb_fan_thresholds", "-o", simulation,
                *sources_501, *unit_sources, test_dir / "tb_fan_thresholds.sv",
            ],
            check=True,
        )
        subprocess.run(["vvp", simulation, *sys.argv[1:]], check=True)


if __name__ == "__main__":
    main()
