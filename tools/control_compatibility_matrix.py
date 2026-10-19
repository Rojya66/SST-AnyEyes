"""Build Debug/Release for three control profiles and both communication owners.

Retain per-build logs and one firmware fingerprint in build/control_compatibility_matrix.json.
The last case proves disabling only the phase-A macro selects the high-voltage profile.
"""
from pathlib import Path
import concurrent.futures
import datetime
import json
import re
import subprocess
import sys
import time


root = Path(__file__).resolve().parents[1]
started = datetime.datetime.now(datetime.timezone.utc).isoformat()
profiles = [('three_phase_high', 0, 0), ('single_phase_high', 1, 0), ('single_phase_low', 1, 1)]
protocols = [('anyeyes', 1, 1, 0), ('vofa', 2, 0, 1)]
cases = []
for configuration in ('Debug', 'Release'):
    for profile, phase, low in profiles:
        for protocol, protocol_id, dpram, simulator in protocols:
            variant = 'audit_' + profile + '_' + protocol
            defines = [f'SST_PHASE_A_ONLY_TEST={phase}', f'SST_LOW_VOLTAGE_100V_TEST={low}',
                       f'SST_DEBUG_PROTOCOL={protocol_id}', f'SST_ENABLE_DPRAM={dpram}',
                       f'SST_SIMULATOR_MODE={simulator}']
            cases.append(dict(configuration=configuration, variant=variant, profile=profile,
                              protocol=protocol, defines=defines))
cases.append(dict(configuration='Debug', variant='audit_three_phase_default',
                  profile='three_phase_default', protocol='default',
                  defines=['SST_PHASE_A_ONLY_TEST=0']))


def execute(case):
    command = [sys.executable, str(root / 'tools/build.py'), '--configuration', case['configuration'],
               '--variant', case['variant'], '--jobs', '3']
    for define in case['defines']:
        command += ['--define', define]
    start = time.time()
    proc = subprocess.run(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = root / 'build' / case['configuration'] / case['variant']
    out.mkdir(parents=True, exist_ok=True)
    (out / 'matrix_driver.log').write_bytes(proc.stdout)
    result_path = out / 'build_result.json'
    result = json.loads(result_path.read_text(encoding='utf-8')) if result_path.is_file() else {}
    build_log = ((out / 'build.log').read_text(encoding='utf-8', errors='replace')
                 if (out / 'build.log').is_file() else proc.stdout.decode('utf-8', errors='replace'))
    warnings = {}
    for warning in re.findall(r'warning #(\d+(?:-D)?)', build_log):
        warnings[warning] = warnings.get(warning, 0) + 1
    record = dict(case, exit_code=proc.returncode, elapsed_s=round(time.time() - start, 2),
                  result_path=str(result_path.relative_to(root)), result=result, warning_codes=warnings)
    print(f"{case['configuration']} {case['variant']}: {'PASS' if proc.returncode == 0 else 'FAIL'} "
          f"warnings={result.get('warnings')} fingerprint={result.get('firmware_input_sha256')} "
          f"unchanged={result.get('firmware_unchanged_during_build')}", flush=True)
    if proc.returncode:
        print(proc.stdout.decode('utf-8', errors='replace')[-10000:], flush=True)
    return record


with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(execute, cases))
fingerprints = sorted(set(r['result'].get('firmware_input_sha256', 'MISSING') for r in results))
summary = dict(started_utc=started,
               finished_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
               requested_matrix_count=12, additional_default_case_count=1, case_count=len(results),
               passed=sum(r['exit_code'] == 0 and not r['result'].get('compile_failures') and
                          r['result'].get('link_status') == 0 for r in results),
               failed=sum(r['exit_code'] != 0 for r in results), fingerprints=fingerprints,
               all_same_firmware_fingerprint=len(fingerprints) == 1,
               all_sources_unchanged_during_build=all(r['result'].get('firmware_unchanged_during_build')
                                                     for r in results),
               max_parallel_builds=2, jobs_per_build=3, results=results)
(root / 'build/control_compatibility_matrix.json').write_text(
    json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({k: v for k, v in summary.items() if k != 'results'}, ensure_ascii=False, indent=2), flush=True)
raise SystemExit(0 if summary['passed'] == summary['case_count'] and summary['failed'] == 0 and
                 summary['all_same_firmware_fingerprint'] and
                 summary['all_sources_unchanged_during_build'] else 1)
