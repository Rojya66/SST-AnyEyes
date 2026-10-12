"""Compile every primary macro combination and optional compatibility paths."""
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
variants = []
for protocol, name in [(1, 'anyeyes'), (2, 'vofa')]:
    for simulator in (1, 0):
        for dpram in (0, 1):
            variants.append((name + ('_sim' if simulator else '_remote') + ('_dpram' if dpram else ''),
                [f'SST_DEBUG_PROTOCOL={protocol}', f'SST_SIMULATOR_MODE={simulator}',
                 f'SST_ENABLE_DPRAM={dpram}']))
variants += [
    ('anyeyes_maintenance', ['SST_SIMULATOR_MODE=0', 'SST_ENABLE_MAINTENANCE=1']),
    ('vofa_maintenance', ['SST_SIMULATOR_MODE=0', 'SST_ENABLE_MAINTENANCE=1', 'SST_DEBUG_PROTOCOL=2']),
    ('anyeyes_persistent', ['CHB_LOAD_EEPROM_PARAMETERS=1']),
]

def build(item):
    name, definitions = item
    args = [sys.executable, str(ROOT / 'tools/build.py'), '--variant', name, '--jobs', '3']
    for define in definitions: args += ['--define', define]
    run = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    report = ROOT / 'build/Debug' / name / 'build_result.json'
    result = json.loads(report.read_text()) if report.exists() else dict(variant=name)
    result['exit_code'] = run.returncode
    print(name, 'PASS' if run.returncode == 0 else 'FAIL', flush=True)
    if run.returncode: print(run.stdout.decode(errors='replace')[-5000:], flush=True)
    return result

with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(build, variants))
(ROOT / 'build/matrix_result.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
print('Build variants:', len(results), 'passed:', sum(r['exit_code'] == 0 for r in results))
raise SystemExit(0 if all(r['exit_code'] == 0 for r in results) else 1)
