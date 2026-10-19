"""Run host regressions and retain output with a fingerprint of firmware inputs."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
import sys

# GCC diagnostics can contain replacement characters or source comments that
# the inherited Windows console code page cannot encode. Keep full UTF-8 logs
# on disk and make console rendering lossy instead of aborting the test report.
for stream in (sys.stdout, sys.stderr):
    if hasattr(stream, 'reconfigure'):
        stream.reconfigure(errors='replace')

ROOT = Path(__file__).resolve().parents[1]
LOGDIR = ROOT / 'build/host_validation'
LOGDIR.mkdir(parents=True, exist_ok=True)
CHECKS = ['realtime_audit', 'control_regression', 'pi_regression', 'fault_reporting',
          'dpram_commands', 'parameter_precision', 'protocol_regression',
          'refactor_equivalence', 'comment_coverage', 'framework_structure', 'firmware_trace',
          'serial_protocol_scope', 'parameter_store_migration', 'dpram_maintenance', 'bench_inputs',
          'single_phase_control', 'single_phase_io', 'single_phase_isolation', 'vofa_telemetry',
          'common_control_safety', 'switch_io']


def fingerprint():
    digest = hashlib.sha256()
    inputs = [p for p in (ROOT / 'Source').rglob('*') if p.is_file()]
    inputs += [ROOT / p for p in ('.project', '.cproject', '.ccsproject', 'C6748.cmd')]
    for path in sorted(inputs):
        digest.update(path.relative_to(ROOT).as_posix().encode())
        digest.update(b'\0')
        digest.update(path.read_bytes())
        digest.update(b'\0')
    return digest.hexdigest()


before = fingerprint()


def run(name):
    result = subprocess.run([sys.executable, str(ROOT / 'tests' / (name + '.py'))],
                            cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log = result.stdout.decode('utf-8', errors='replace')
    (LOGDIR / (name + '.log')).write_text(log, encoding='utf-8')
    print(name, 'PASS' if result.returncode == 0 else 'FAIL', flush=True)
    if result.returncode:
        print(log[-6000:], flush=True)
    return dict(check=name, exit_code=result.returncode, log=str(LOGDIR / (name + '.log')))


with ThreadPoolExecutor(max_workers=3) as pool:
    results = list(pool.map(run, CHECKS))
after = fingerprint()
report = dict(firmware_input_sha256=before, firmware_unchanged_during_checks=before == after,
              checks=results)
(LOGDIR / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
raise SystemExit(0 if before == after and all(r['exit_code'] == 0 for r in results) else 1)
