"""Export compact, relocatable software evidence after delivery validation.

Run delivery_manifest.py first. Large local logs and generated firmware remain
under ignored build directories; this summary records their hashes and results.
"""
import hashlib
import json
import re
from pathlib import Path

from firmware_inputs import fingerprint

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'docs/evidence'


def read(relative):
    return json.loads((ROOT / relative).read_text(encoding='utf-8'))


def sha(relative):
    return hashlib.sha256((ROOT / relative).read_bytes()).hexdigest()


def local_path(value):
    return Path(value).relative_to(ROOT).as_posix()


manifest = read('build/delivery_manifest.json')
assert manifest['firmware_input_sha256'] == fingerprint(ROOT)
refactor = read('build/refactor_equivalence.json')
comments = read('build/comment_inventory/verification.json')
structure = read('build/framework_structure/result.json')
trace = read('build/firmware_trace/result.json')
binary = read('build/isr_binary_audit/result.json')
initialization = read('build/control_init_equivalence.json')
defaults = [read(f'build/{mode}/default/build_result.json') for mode in ('Debug', 'Release')]
memory = []
for mode in ('Debug', 'Release'):
    text = (ROOT / f'build/{mode}/default/SST_AnyEyes.map').read_text(encoding='utf-8')
    match = re.search(r'^\s+SHDSPL2RAM\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)', text, re.M | re.I)
    assert match
    memory.append(dict(configuration=mode, origin='0x' + match[1],
                       length='0x' + match[2], used='0x' + match[3], remaining='0x' + match[4]))

report_paths = ['build/delivery_manifest.json', 'build/host_validation/result.json',
                'build/matrix_result.json', 'build/ccs_validation/result.json',
                'build/workspace_import_validation/result.json', 'build/dpram_map_validation.json',
                'build/refactor_equivalence.json', 'build/control_init_equivalence.json',
                'build/comment_inventory/verification.json', 'build/framework_structure/result.json',
                'build/firmware_trace/result.json', 'build/isr_binary_audit/result.json']
report = dict(
    created_utc=manifest['created_utc'], project='AnyEyesVersion',
    baseline_reference='baseline-pre-framework-2026-10-12',
    baseline_commit=refactor['baseline_commit'],
    refactored_branch='refactor/f28377s-framework',
    refactored_tag='framework-refactored-2026-10-12',
    firmware_input_sha256=manifest['firmware_input_sha256'],
    target_builds=[dict(configuration=d['configuration'], source_count=d['source_count'],
                        warnings=d['warnings'], link_status=d['link_status'],
                        compile_failures=d['compile_failures']) for d in defaults],
    macro_builds=[dict(variant=d['variant'], defines=d['defines'], warnings=d['warnings'],
                       exit_code=d['exit_code']) for d in read('build/matrix_result.json')],
    ccs_results=manifest['ccs_results'], workspace_import_results=manifest['workspace_import_results'],
    host_checks=[dict(check=c['check'], exit_code=c['exit_code'], log=local_path(c['log']),
                      log_sha256=hashlib.sha256(Path(c['log']).read_bytes()).hexdigest())
                 for c in manifest['host_checks']],
    refactor_equivalence=dict(original_entities=refactor['original_entities'],
                              current_entities=refactor['current_entities'],
                              entity_groups=refactor['entity_groups'], failures=refactor['failures']),
    initialization_equivalence=initialization,
    comments=dict(custom_source_files=comments['custom_source_files'],
                  symbol_counts=comments['symbol_counts'], failures=comments['failures']),
    structure=dict(assertions=structure['assertions'], header_compile_units=len(structure['headers']),
                   failures=structure['failures']),
    firmware_traces=dict(variants=trace['variants'], method=trace['method'], limitations=trace['limitations']),
    isr_binary_comparison=dict(results=binary['results'], passed=binary['passed'], method=binary['method'],
                               limitations=binary['limitations'], firmware_sha256=binary['firmware_sha256']),
    map_audits=[dict(path=local_path(m['path']), passed=m['passed'], sha256=m['sha256'])
                for m in manifest['dpram_map_checks']],
    memory_usage=memory, firmware_artifacts=manifest['firmware_artifacts'],
    report_sha256={path: sha(path) for path in report_paths},
    physical_board_validation_performed=False,
)
DEST.mkdir(parents=True, exist_ok=True)
(DEST / 'software_validation.json').write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
print('Exported:', DEST / 'software_validation.json')
