"""Reproduce a direct-child CCS import conflict and validate matching project names."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ECLIPSE = Path('E:/Applications/ti/ccs1250/ccs/eclipse/eclipsec.exe')
LOGDIR = ROOT / 'build/workspace_import_validation'
LOGDIR.mkdir(parents=True, exist_ok=True)
workspace = Path(tempfile.mkdtemp(prefix='SST_direct_child_'))
project = workspace / 'AnyEyesVersion'
project.mkdir()
for name in ('Source', 'targetConfigs'):
    shutil.copytree(ROOT / name, project / name)
for name in ('.project', '.cproject', '.ccsproject', 'C6748.cmd'):
    shutil.copy2(ROOT / name, project / name)

results = []
def run(label, extra):
    result = subprocess.run([str(ECLIPSE), '-noSplash', '-data', str(workspace)] + extra,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = result.stdout.decode('utf-8', errors='replace')
    (LOGDIR / (label + '.log')).write_text(output, encoding='utf-8')
    results.append(dict(action=label, exit_code=result.returncode))
    print(label, 'exit', result.returncode, flush=True)
    return result.returncode, output

path = project / '.project'
path.write_text(path.read_text(encoding='utf-8').replace('<name>AnyEyesVersion</name>', '<name>SST_AnyEyes</name>'), encoding='utf-8')
command = ['-application', 'com.ti.ccstudio.apps.projectImport', '-ccs.location', str(project)]
code, log = run('before', command)
assert code != 0 and 'overlaps the location of another project' in log, log
path = project / '.project'
path.write_text(path.read_text(encoding='utf-8').replace('<name>SST_AnyEyes</name>',
                                                      '<name>AnyEyesVersion</name>'), encoding='utf-8')
path = project / '.cproject'
path.write_text(path.read_text(encoding='utf-8').replace('${ProjName}', 'SST_AnyEyes'), encoding='utf-8')
code, log = run('after', command)
assert code == 0, log
for mode in ('Debug', 'Release'):
    code, log = run(mode, ['-application', 'com.ti.ccstudio.apps.projectBuild',
                          '-ccs.projects', 'AnyEyesVersion', '-ccs.configuration', mode])
    assert code == 0, log
    assert (project / mode / 'SST_AnyEyes.out').is_file()
report = dict(workspace=str(workspace), project=str(project), results=results)
(LOGDIR / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
