"""Audit requested custom-source comments independently of file relocation.

Documentation-only dependency: pip install tree-sitter tree-sitter-c tree-sitter-cpp
Historical per-file lexical equality is reported when the old path exists. It is
not a refactor acceptance gate: tests/refactor_equivalence.py checks moved entities
against the immutable Git baseline. The old comment baseline is never changed.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from comment_inventory import source_files, collect, decode, walk
from annotate_source import token_stream

counts = Counter()
results = []
failures = []
for path in source_files():
    rel = path.relative_to(ROOT).as_posix()
    raw, _, data, root, symbols = collect(path)
    baseline = ROOT / 'build/comment_baseline' / rel
    unchanged = None
    baseline_hash = None
    if baseline.exists():
        old, _ = decode(baseline.read_bytes())
        unchanged = token_stream(raw) == token_stream(old)
        baseline_hash = hashlib.sha256(baseline.read_bytes()).hexdigest()
    functions = [s for s in symbols if s['kind'] in ('function', 'prototype')]
    parameters = [s for s in symbols if s['kind'] == 'parameter']
    for s in functions:
        left = data[:s['start']].decode()
        start = left.rfind('/*==================================================================')
        header = left[start:] if start >= 0 else ''
        # At most a C-linkage qualifier may occur after the immediately preceding banner.
        if not header or len(header) > 5000 or any(label not in header for label in (
                '函数功能:', '输入参数:', '输出参数:', '返回值:', '执行说明:')):
            failures.append(rel + ': undocumented function ' + s['name'])
        if s['kind']=='function' and 'void' not in s['declaration'].split('(')[0].split():
            if '返回值: 无\r\n' in header or '返回值: 无\n' in header:
                failures.append(rel + ': non-void function documented as no return: ' + s['name'])
    for s in symbols:
        if s['kind'] not in ('variable', 'field'):
            continue
        tail = data[s['end']:s['end'] + 6000].decode(errors='ignore')
        comments = re.match(r'(?:\s|/\*[\s\S]*?\*/)*', tail)[0]
        if '/* ' + s['name'] + '：' not in comments:
            failures.append(rel + ': undocumented declaration ' + s['name'])
    # Parameter descriptions live in the function banners, not inside signatures.
    for s in parameters:
        left = data[:s['start']].decode()
        start = left.rfind('/*==================================================================')
        if start < 0 or s['name'] + '：' not in left[start:]:
            failures.append(rel + ': undocumented parameter ' + s['name'])
    for node in walk(root):
        if node.type == 'enumerator':
            left = data[max(0, node.start_byte - 500):node.start_byte].decode(errors='ignore')
            if not re.search(r'/\*[^*]*[\u4e00-\u9fff][\s\S]*?\*/\s*$', left):
                failures.append(rel + ': undocumented enum constant')
        if node.type not in ('preproc_def', 'preproc_function_def'):
            continue
        nn = node.child_by_field_name('name')
        if not nn:
            continue
        name = data[nn.start_byte:nn.end_byte].decode()
        value = node.child_by_field_name('value')
        empty = value is None or not data[value.start_byte:value.end_byte].strip()
        if empty and (name.endswith(('_H_', '_H', '_HPP_', '_HPP')) or name.startswith('_') and 'H' in name):
            continue
        left = data[max(0,node.start_byte - 2000):node.start_byte].decode(errors='ignore')
        if '/* ' + name + '：' not in left[left.rfind('/*'):]:
            failures.append(rel + ': undocumented compile macro ' + name)
    counts.update(s['kind'] for s in symbols)
    results.append(dict(file=rel, executable_tokens_unchanged=unchanged,
                        baseline_sha256=baseline_hash,
                        annotated_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                        symbol_counts=dict(Counter(s['kind'] for s in symbols))))

report = dict(custom_source_files=len(results), symbol_counts=dict(counts),
              failures=failures, files=results,
              scope='Source except vendor 00_Device and 01_Drive trees; includes legacy conditional branches',
              method='Tree-sitter declarations + named Chinese comments; historical per-file tokens are informational only',
              equivalence_acceptance_report='build/refactor_equivalence.json')
target = ROOT / 'build/comment_inventory/verification.json'
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print('Custom comment coverage:', len(results), 'files;', dict(counts))
historical = [item for item in results if item['executable_tokens_unchanged'] is not None]
print('Historical same-path token equality (informational):', sum(item['executable_tokens_unchanged'] for item in historical), '/', len(historical))
for failure in failures:
    print('FAIL:', failure)
print('PASS' if not failures else 'FAIL', 'Report:', target)
raise SystemExit(bool(failures))
