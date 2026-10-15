"""Export checked, current evidence for the 2026-10-15 source-style delivery."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import sys

from firmware_inputs import fingerprint

ROOT = Path(__file__).resolve().parents[1]
BASELINE = '1330a040ba95b9e6da1c5e87f96a5a73b0277638'


def read(relative):
    return json.loads((ROOT / relative).read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def artifact(relative):
    path = ROOT / relative
    return dict(path=path.relative_to(ROOT).as_posix(), sha256=sha(path))


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    current = fingerprint(ROOT)
    style = read('build/style_audit/final_style.json')
    comments = read('build/comment_inventory/verification.json')
    host = read('build/host_validation/result.json')
    binary = read('build/style_audit/firmware_sections.json')
    require(not style['failures'] and style['custom_source_files'] == 132, 'Style/preservation failed')
    require(not comments['failures'] and comments['custom_source_files'] == 132, 'Comment coverage failed')
    for row in style['files']:
        require(row.get('tokens_unchanged') and row.get('directives_unchanged'), row['file'])
        require(row['sha256'] == sha(ROOT / row['file']), 'Stale style inventory: ' + row['file'])
    for row in comments['files']:
        require(row['annotated_sha256'] == sha(ROOT / row['file']), 'Stale comments: ' + row['file'])
    require(host['firmware_input_sha256'] == current and host['firmware_unchanged_during_checks'],
            'Host checks do not match current firmware inputs')
    require(len(host['checks']) == 14 and all(row['exit_code'] == 0 for row in host['checks']),
            'Host checks failed/incomplete')
    require(binary['allocated_sections_equal'] and binary['section_count'] == 34, 'Binary sections differ')
    require(all(row['unchanged'] for row in binary['sections']), 'Linked section changed')
    vendor_diff = subprocess.check_output(['git', 'diff', BASELINE, '--name-only', '--',
                                          'Source/00_Device', 'Source/01_Drive'], cwd=ROOT)
    require(not vendor_diff.strip(), 'Vendor files were changed')
    checks = []
    for configuration, variant in [('Debug', 'style_after'), ('Release', 'style_after'),
                                    ('Debug', 'style_vofa_dpram')]:
        directory = ROOT / 'build' / configuration / variant
        build = json.loads((directory / 'build_result.json').read_text(encoding='utf-8'))
        require(not build['compile_failures'] and build['link_status'] == 0, 'Build failed')
        require(build['firmware_input_sha256'] == current and build['firmware_unchanged_during_build'],
                'Stale build: ' + variant)
        checks.append(dict(configuration=configuration, variant=variant, defines=build['defines'],
                           source_count=build['source_count'], warnings=build['warnings'],
                           executable=artifact(Path(build['executable']).relative_to(ROOT)),
                           report=artifact(directory.relative_to(ROOT) / 'build_result.json')))
    self_check = subprocess.run([sys.executable, str(ROOT / 'tests/comment_coverage.py'), '--self-test'],
                                cwd=ROOT, capture_output=True, text=True)
    require(self_check.returncode == 0, self_check.stdout + self_check.stderr)
    shutil.copyfile(ROOT / 'build/style_audit/final_style.json', ROOT / 'build/style_audit/source_preservation.json')
    summary = dict(date='2026-10-15', baseline_commit=BASELINE, firmware_input_sha256=current,
                   scope=style['scope'], custom_source_files=132, formatter=style['formatter'],
                   vendor_sources_unchanged=True, executable_tokens_and_directives_unchanged=True,
                   screening_metrics=dict(before=style['baseline'], after=style['after']),
                   comment_symbol_counts=comments['symbol_counts'], missing_documentation=0,
                   comment_audit_self_checks=self_check.stdout.strip(),
                   allocated_elf_section_count=34, allocated_elf_sections_unchanged=True,
                   sections=binary['sections'], builds=checks,
                   host_checks=[dict(check=row['check'], exit_code=row['exit_code'],
                                     log=artifact(Path(row['log']).relative_to(ROOT))) for row in host['checks']],
                   source_files=[dict(file=row['file'], sha256=row['sha256']) for row in style['files']],
                   tools=[artifact(path) for path in ['.clang-format', 'tools/format_source.py',
                          'tools/compare_firmware_sections.py', 'tools/source_equivalence.py',
                          'tools/comment_inventory.py', 'tests/comment_coverage.py']],
                   limit='No hardware timing/WCET measurements; .out debug metadata can differ')
    target = ROOT / 'docs/evidence/source_style_validation.json'
    target.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('PASS:', target)
    print('132 source files; 3 target builds; 14 host checks; 34 unchanged allocated ELF sections')


if __name__ == '__main__':
    main()
