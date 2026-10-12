"""Import/build the saved CCS project in an isolated temporary workspace."""
import json
from pathlib import Path
import subprocess
import tempfile
from firmware_inputs import fingerprint

ROOT = Path(__file__).resolve().parents[1]
ECLIPSE = Path('E:/Applications/ti/ccs1250/ccs/eclipse/eclipsec.exe')
LOGDIR = ROOT / 'build/ccs_validation'
LOGDIR.mkdir(parents=True, exist_ok=True)
workspace = Path(tempfile.mkdtemp(prefix='SST_AnyEyes_ccs_'))
results = []
input_sha256 = fingerprint(ROOT)
for action, extra in [
    ('import', ['-application', 'com.ti.ccstudio.apps.projectImport', '-ccs.location', str(ROOT)]),
    ('Debug', ['-application', 'com.ti.ccstudio.apps.projectBuild', '-ccs.projects', 'AnyEyesVersion',
               '-ccs.configuration', 'Debug']),
    ('Release', ['-application', 'com.ti.ccstudio.apps.projectBuild', '-ccs.projects', 'AnyEyesVersion',
                 '-ccs.configuration', 'Release'])]:
    clean_log = ''
    if action != 'import':
        cleaned = subprocess.run([str(ECLIPSE), '-noSplash', '-data', str(workspace)] + extra + ['-ccs.clean'],
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        clean_log = cleaned.stdout.decode('utf-8', errors='replace')
        if cleaned.returncode:
            (LOGDIR / (action + '.log')).write_text(clean_log, encoding='utf-8')
            results.append(dict(action=action, exit_code=cleaned.returncode, clean_exit_code=cleaned.returncode))
            print(action, 'clean failed', cleaned.returncode, flush=True)
            break
    run = subprocess.run([str(ECLIPSE), '-noSplash', '-data', str(workspace)] + extra,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log = clean_log + '\n' + run.stdout.decode('utf-8', errors='replace')
    (LOGDIR / (action + '.log')).write_text(log, encoding='utf-8')
    artifact = ROOT / action / 'SST_AnyEyes.out' if action != 'import' else None
    success = run.returncode == 0 and (artifact is None or artifact.is_file())
    results.append(dict(action=action, exit_code=run.returncode if success else 1,
                        clean_exit_code=0 if action != 'import' else None))
    print(action, 'exit', run.returncode, flush=True)
    if not success:
        print(log[-7000:])
        break
report = dict(workspace=str(workspace), results=results, firmware_input_sha256=input_sha256,
              firmware_unchanged_during_build=input_sha256 == fingerprint(ROOT))
(LOGDIR / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
raise SystemExit(0 if len(results) == 3 and all(r['exit_code'] == 0 for r in results) else 1)
