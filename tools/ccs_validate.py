"""Import/build the saved CCS project in an isolated temporary workspace."""
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ECLIPSE = Path('E:/Applications/ti/ccs1250/ccs/eclipse/eclipsec.exe')
LOGDIR = ROOT / 'build/ccs_validation'
LOGDIR.mkdir(parents=True, exist_ok=True)
workspace = Path(tempfile.mkdtemp(prefix='SST_AnyEyes_ccs_'))
results = []
for action, extra in [
    ('import', ['-application', 'com.ti.ccstudio.apps.projectImport', '-ccs.location', str(ROOT)]),
    ('Debug', ['-application', 'com.ti.ccstudio.apps.projectBuild', '-ccs.projects', 'AnyEyesVersion',
               '-ccs.configuration', 'Debug']),
    ('Release', ['-application', 'com.ti.ccstudio.apps.projectBuild', '-ccs.projects', 'AnyEyesVersion',
                 '-ccs.configuration', 'Release'])]:
    run = subprocess.run([str(ECLIPSE), '-noSplash', '-data', str(workspace)] + extra,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log = run.stdout.decode('utf-8', errors='replace')
    (LOGDIR / (action + '.log')).write_text(log, encoding='utf-8')
    results.append(dict(action=action, exit_code=run.returncode))
    print(action, 'exit', run.returncode, flush=True)
    if run.returncode:
        print(log[-7000:])
        break
report = dict(workspace=str(workspace), results=results)
(LOGDIR / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
raise SystemExit(0 if len(results) == 3 and all(r['exit_code'] == 0 for r in results) else 1)
