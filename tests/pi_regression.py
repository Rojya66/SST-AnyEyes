"""编译并执行 SstPi 公共 PI 模块的主机回归用例。"""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


TEST_SOURCE = r'''#include <math.h>
#include <stdio.h>
#include <float.h>
#include "SstPi.h"

static int check(int condition, const char *name)
{
    if (!condition) { printf("FAIL: %s\n", name); return 1; }
    return 0;
}

static int near(float a, float b)
{
    return fabsf(a - b) < 0.0001F;
}

int main(void)
{
    SST_PI_CONTROLLER pi, neg, d, q, no_aw_d, no_aw_q;
    int failures = 0;
    float nan_value = NAN;

    SstPi_Init(&pi);
    failures += check(SstPi_Configure(&pi, 2.0F, 10.0F, 0.1F,
                                      -1.0F, 1.0F, -10.0F, 10.0F, 1U), "configure");
    failures += check(SstPi_Run(&pi, 1.0F, 0.0F, 0.0F) == SST_PI_SUCCESS, "positive run");
    failures += check(near(pi.output, 1.0F), "positive output saturation");
    failures += check(near(pi.integral, 0.0F), "positive anti windup");
    failures += check((pi.flags & SST_PI_FLAG_INTEGRAL_HELD) != 0U, "positive held flag");
    failures += check(SstPi_Run(&pi, -0.1F, 0.0F, 0.0F) == SST_PI_SUCCESS, "recovery run");
    failures += check(pi.output < 0.0F, "saturation recovery direction");

    SstPi_Reset(&pi);
    failures += check(SstPi_Run(&pi, 0.1F, 0.0F, 0.0F) == SST_PI_SUCCESS, "unsaturated run");
    failures += check(near(pi.integral, 0.1F) && near(pi.output, 0.3F), "same sample integration");
    failures += check(SstPi_Configure(&pi, 2.0F, 10.0F, 0.1F,
                                      -1.0F, 1.0F, -10.0F, 10.0F, 1U), "reconfigure");
    failures += check(near(pi.integral, 0.1F), "reconfigure preserves integral");

    SstPi_Init(&neg);
    failures += check(SstPi_Configure(&neg, 0.0F, -1.0F, 1.0F,
                                      -2.0F, 2.0F, -10.0F, 10.0F, 1U), "negative Ki configure");
    failures += check(SstPi_Run(&neg, 1.0F, 0.0F, 0.0F) == SST_PI_SUCCESS, "negative Ki run");
    failures += check(near(neg.output, -1.0F), "negative Ki sign");
    failures += check(SstPi_Run(&neg, 3.0F, 0.0F, 0.0F) == SST_PI_SUCCESS, "negative Ki saturation");
    failures += check(near(neg.integral, -1.0F), "negative Ki outward held");
    failures += check(SstPi_Run(&neg, -0.5F, 0.0F, 0.0F) == SST_PI_SUCCESS, "negative Ki recovery");
    failures += check(near(neg.integral, -0.5F), "negative Ki inward accepted");

    pi.feedforward = 0.2F;
    pi.proportional = 0.1F;
    failures += check(SstPi_Track(&pi, 0.7F) == SST_PI_SUCCESS, "tracking");
    failures += check(near(pi.integral, 0.4F) && (pi.flags & SST_PI_FLAG_TRACKING), "tracking state");

    SstPi_Init(&d);
    SstPi_Init(&q);
    failures += check(SstPi_Configure(&d, 1.0F, 0.0F, 0.0001F,
                                      -10.0F, 10.0F, -10.0F, 10.0F, 1U), "d configure");
    failures += check(SstPi_Configure(&q, 1.0F, 0.0F, 0.0001F,
                                      -10.0F, 10.0F, -10.0F, 10.0F, 1U), "q configure");
    failures += check(SstPi_RunDq(&d, &q, 3.0F, 0.0F, 0.0F,
                                  4.0F, 0.0F, 0.0F, 1.0F) == SST_PI_SUCCESS, "vector run");
    failures += check(near(d.output, 0.6F) && near(q.output, 0.8F), "vector circle");
    failures += check((d.flags & SST_PI_FLAG_VECTOR_LIMIT) != 0U, "vector flag");
    failures += check(SstPi_RunDq(&d, &q, 3.0F, 0.0F, 0.0F,
                                  4.0F, 0.0F, 0.0F, 0.0F) == SST_PI_SUCCESS, "zero vector limit");
    failures += check(near(d.output, 0.0F) && near(q.output, 0.0F), "zero vector output");
    failures += check((d.flags & SST_PI_FLAG_INTEGRAL_HELD) != 0U, "zero vector held");

    /* 禁用可选抗饱和时，零动态边界仍必须冻结积分，避免上层禁止输出期间积累。 */
    SstPi_Init(&no_aw_d);
    SstPi_Init(&no_aw_q);
    failures += check(SstPi_Configure(&no_aw_d, 0.0F, 1.0F, 1.0F,
                                      -10.0F, 10.0F, -10.0F, 10.0F, 0U), "zero limit d configure");
    failures += check(SstPi_Configure(&no_aw_q, 0.0F, 1.0F, 1.0F,
                                      -10.0F, 10.0F, -10.0F, 10.0F, 0U), "zero limit q configure");
    failures += check(SstPi_RunDq(&no_aw_d, &no_aw_q, 1.0F, 0.0F, 0.0F,
                                  2.0F, 0.0F, 0.0F, 10.0F) == SST_PI_SUCCESS, "zero limit preload");
    failures += check(near(no_aw_d.integral, 1.0F) && near(no_aw_q.integral, 2.0F), "zero limit preload integral");
    failures += check(SstPi_RunDq(&no_aw_d, &no_aw_q, 3.0F, 0.0F, 0.0F,
                                  4.0F, 0.0F, 0.0F, 0.0F) == SST_PI_SUCCESS, "zero limit disabled anti windup");
    failures += check(near(no_aw_d.integral, 1.0F) && near(no_aw_q.integral, 2.0F), "zero limit always held");

    failures += check(SstPi_Configure(&d, 1.0F, 1.0F, 0.1F,
                                      -1.0F, 1.0F, -10.0F, 10.0F, 1U), "rectangle d configure");
    failures += check(SstPi_Configure(&q, 1.0F, 1.0F, 0.1F,
                                      -1.0F, 1.0F, -10.0F, 10.0F, 1U), "rectangle q configure");
    failures += check(SstPi_RunDq(&d, &q, 3.0F, 0.0F, 0.0F,
                                  0.0F, 0.0F, 0.0F, 10.0F) == SST_PI_SUCCESS, "rectangle run");
    failures += check(near(d.integral, 0.0F), "rectangle anti windup");
    d.integral = 0.4F;
    q.integral = -0.3F;
    failures += check(SstPi_RunDq(&d, &q, 3.0F, 0.0F, 0.0F,
                                  2.0F, 0.0F, 0.0F, 0.0F) == SST_PI_SUCCESS, "zero dynamic run");
    failures += check(near(d.integral, 0.4F) && near(q.integral, -0.3F), "zero dynamic integral hold");

    failures += check(SstPi_Configure(&pi, FLT_MAX, 1.0F, 0.1F,
                                      -1.0F, 1.0F, -10.0F, 10.0F, 1U), "overflow configure");
    failures += check(SstPi_Run(&pi, FLT_MAX, 0.0F, 0.0F) == SST_PI_FAILURE, "finite input overflow");
    failures += check((pi.flags & SST_PI_FLAG_NUMERIC_ERROR) != 0U && near(pi.output, 0.0F), "overflow fail closed");

    failures += check(SstPi_Configure(&d, FLT_MAX, 1.0F, 0.1F,
                                      -1.0F, 1.0F, -10.0F, 10.0F, 1U), "vector overflow configure");
    failures += check(SstPi_RunDq(&d, &q, FLT_MAX, 0.0F, 0.0F,
                                  0.0F, 0.0F, 0.0F, 10.0F) == SST_PI_FAILURE, "vector overflow");
    failures += check((d.flags & SST_PI_FLAG_NUMERIC_ERROR) != 0U && (q.flags & SST_PI_FLAG_NUMERIC_ERROR) != 0U,
                     "vector shared invalidation");

    failures += check(SstPi_Run(&pi, nan_value, 0.0F, 0.0F) == SST_PI_FAILURE, "NaN input");
    failures += check((pi.flags & SST_PI_FLAG_INVALID_INPUT) != 0U, "NaN flag");
    failures += check(SstPi_Configure(&pi, 1.0F, 1.0F, 0.0F,
                                      -1.0F, 1.0F, -1.0F, 1.0F, 1U) == SST_PI_FAILURE,
                     "invalid Ts");
    failures += check((pi.flags & SST_PI_FLAG_INVALID_PARAMETER) != 0U, "invalid parameter flag");

    if (failures != 0) { return 1; }
    puts("SstPi regression: PASS");
    return 0;
}
'''


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--cc", default=shutil.which("gcc"))
    args = parser.parse_args()
    if not args.cc:
        raise SystemExit("GCC not found; pass --cc with --cc")
    pi_dir = args.project / "Source/01_ComLib/04_Pi"
    with tempfile.TemporaryDirectory(prefix="sst_pi_") as folder:
        folder_path = Path(folder)
        source = folder_path / "pi_regression.c"
        binary = folder_path / "pi_regression.exe"
        source.write_text(TEST_SOURCE, encoding="utf-8")
        command = [args.cc, "-std=c99", "-O2", "-Wall", "-Wextra",
                   "-Werror", "-I", str(pi_dir), str(source), str(pi_dir / "SstPi.c"),
                   "-lm", "-o", str(binary)]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
