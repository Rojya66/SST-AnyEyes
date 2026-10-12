"""Record final firmware hashes and successful build/check evidence."""
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
host = json.loads((ROOT / 'build/host_validation/result.json').read_text())
matrix = json.loads((ROOT / 'build/matrix_result.json').read_text())
ccs = json.loads((ROOT / 'build/ccs_validation/result.json').read_text())
defaults = [json.loads((ROOT / 'build' / mode / 'default/build_result.json').read_text())
            for mode in ('Debug', 'Release')]
maps = json.loads((ROOT / 'build/dpram_map_validation.json').read_text())
comments = json.loads((ROOT / 'build/comment_inventory/verification.json').read_text(encoding='utf-8'))
assert not comments['failures']
assert all(item['executable_tokens_unchanged'] for item in comments['files'])
for item in comments['files']:
    assert hashlib.sha256((ROOT / item['file']).read_bytes()).hexdigest() == item['annotated_sha256']
assert len(maps) == 15 and all(r['passed'] for r in maps)
for item in maps:
    assert hashlib.sha256(Path(item['path']).read_bytes()).hexdigest() == item['sha256']
assert host['firmware_unchanged_during_checks']
assert all(r['exit_code'] == 0 for r in host['checks'])
assert len(matrix) == 11 and all(r['exit_code'] == 0 for r in matrix)
assert len(ccs['results']) == 3 and all(r['exit_code'] == 0 for r in ccs['results'])
assert all(r['link_status'] == 0 and not r['compile_failures'] for r in defaults)

digest = hashlib.sha256()
inputs = [p for p in (ROOT / 'Source').rglob('*') if p.is_file()]
inputs += [ROOT / p for p in ('.project', '.cproject', '.ccsproject', 'C6748.cmd')]
for path in sorted(inputs):
    digest.update(path.relative_to(ROOT).as_posix().encode())
    digest.update(b'\0')
    digest.update(path.read_bytes())
    digest.update(b'\0')
assert digest.hexdigest() == host['firmware_input_sha256'], 'Firmware changed after host validation'

paths = [ROOT / 'Debug/SST_AnyEyes.out', ROOT / 'Release/SST_AnyEyes.out']
paths += [Path(r['executable']) for r in defaults + matrix]
artifacts = []
for path in paths:
    assert path.is_file()
    artifacts.append(dict(path=path.relative_to(ROOT).as_posix(), bytes=path.stat().st_size,
                          sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
report = dict(created_utc=datetime.now(timezone.utc).isoformat(),
              project='AnyEyesVersion', firmware_input_sha256=digest.hexdigest(),
              ccs_results=ccs['results'], macro_variants=len(matrix),
              host_checks=host['checks'], dpram_map_checks=maps, firmware_artifacts=artifacts,
              custom_comment_verification=dict(files=comments['custom_source_files'],
                  symbol_counts=comments['symbol_counts'], failures=comments['failures'],
                  report='build/comment_inventory/verification.json'),
              physical_board_validation_performed=False)
target = ROOT / 'build/delivery_manifest.json'
target.write_text(json.dumps(report, indent=2), encoding='utf-8')
print('Delivery verified:', len(artifacts), 'firmware files,', len(host['checks']), 'host checks,', len(matrix), 'macro variants')
print('Firmware inputs SHA256:', digest.hexdigest())
print('Manifest:', target)
