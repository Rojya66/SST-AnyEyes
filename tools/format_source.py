"""Format project-owned C/C++ sources and audit preservation against a Git ref.

Requires clang-format 20.1.8 and tree-sitter C/C++ grammars; see SOURCE_STYLE.md.
No formatter is run on TI device headers, vendor drivers, assembly, archives or
generated build outputs. Comments, layout and line endings may change; C tokens,
preprocessor directive boundaries and include order must remain unchanged.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
EXTENSIONS = {'.c', '.h', '.cpp', '.hpp'}
EXCLUDED = {'00_Device', '01_Drive'}
TOKEN = re.compile(
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|'
    r'[A-Za-z_]\w*|(?:\d|\.\d)[\w.]*(?:[eEpP][+-]?\d+)?|'
    r'>>=|<<=|\.\.\.|->\*|::|\.\*|\+\+|--|->|&&|\|\||'
    r'<=|>=|==|!=|\*=|/=|%=|\+=|-=|<<|>>|&=|\^=|\|=|##|[^\s]'
)
COMMENTS = re.compile(
    r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|/\*[\s\S]*?\*/|//[^\n]*'
)


def source_files():
    return sorted(p for p in (ROOT / 'Source').rglob('*')
                  if p.is_file() and p.suffix.lower() in EXTENSIONS
                  and p.relative_to(ROOT / 'Source').parts[0] not in EXCLUDED)


def decode(data):
    for encoding in ('utf-8-sig', 'gb18030'):
        try:
            text = data.decode(encoding)
            if encoding == 'utf-8-sig' and not data.startswith(b'\xef\xbb\xbf'):
                encoding = 'utf-8'
            return text, encoding
        except UnicodeDecodeError:
            pass
    raise ValueError('Unknown source encoding')


def uncomment(text, splice=True):
    if splice:
        text = text.replace('\\\r\n', '').replace('\\\n', '')
    return COMMENTS.sub(lambda m: m[1] if m[1] else ' ' + '\n' * m[0].count('\n'), text)


def token_stream(text):
    return tuple(TOKEN.findall(uncomment(text)))


def directives(text):
    return tuple(tuple(TOKEN.findall(line)) for line in uncomment(text).splitlines()
                 if line.lstrip().startswith('#'))


def expand_declaration_comments(text, suffix):
    """Put existing per-variable comments beside comma-separated declarators.

    Retain the original declaration, type, qualifiers, initializers and commas.
    Only move already present named comments; do not infer variable meanings or
    split anonymous aggregate types into distinct, potentially different types.
    """
    from tree_sitter import Language, Parser
    import tree_sitter_c
    import tree_sitter_cpp

    language = tree_sitter_cpp if suffix.lower() in ('.cpp', '.hpp') else tree_sitter_c
    data = text.encode('utf-8')
    tree = Parser(Language(language.language())).parse(data)
    edits = []

    def identifier(node):
        if node is None:
            return None
        if node.type in ('identifier', 'field_identifier', 'qualified_identifier'):
            return node
        return identifier(node.child_by_field_name('declarator'))

    def walk(node):
        if node.type in ('declaration', 'field_declaration'):
            declarations = node.children_by_field_name('declarator')
            names = [identifier(declarator) for declarator in declarations]
            contents = data[node.start_byte:node.end_byte].decode('utf-8')
            if len(declarations) > 1 and all(names) and '/*' not in contents and '//' not in contents:
                remaining = data[node.end_byte:].decode('utf-8')
                comments = []
                consumed = 0
                for name in names:
                    name_text = data[name.start_byte:name.end_byte].decode('utf-8')
                    match = re.match(r'\s*(/\* ' + re.escape(name_text) + r'：[\s\S]*?\*/)', remaining[consumed:])
                    if not match:
                        break
                    comments.append(match[1])
                    consumed += match.end()
                if len(comments) == len(names):
                    prefix = data[node.start_byte:declarations[0].start_byte].decode('utf-8').rstrip()
                    rows = []
                    for index, (declarator, comment) in enumerate(zip(declarations, comments)):
                        body = data[declarator.start_byte:declarator.end_byte].decode('utf-8')
                        punctuation = ';' if index == len(declarations) - 1 else ','
                        rows.append((prefix + ' ' if index == 0 else '') + body + punctuation + ' ' + comment)
                    edits.append((node.start_byte, node.end_byte + len(remaining[:consumed].encode('utf-8')),
                                  '\n'.join(rows).encode('utf-8')))
                    return
        for child in node.named_children:
            walk(child)

    walk(tree.root_node)
    for start, end, replacement in reversed(edits):
        data = data[:start] + replacement + data[end:]
    result = data.decode('utf-8')
    if token_stream(text) != token_stream(result) or directives(text) != directives(result):
        raise RuntimeError('Declaration comment layout changed C tokens')
    return result


def metrics(text):
    clean = uncomment(text, splice=False)
    rows = clean.splitlines()
    # Screening metric: discard for-header semicolons before counting statements.
    no_for = re.sub(r'\bfor\s*\([^;]*;[^;]*;[^)]*\)',
                    lambda m: m[0].replace(';', ' '), clean)
    multi = sum(line.count(';') > 1 for line in no_for.splitlines())
    return dict(lines=len(text.splitlines()), multiple_statement_lines=multi,
                code_lines_over_120=sum(len(row.expandtabs(4)) > 120 for row in rows if row.strip()))


def formatter_path(option):
    candidates = [option, shutil.which('clang-format'),
                  ROOT / 'build/style_tools/clang_format/data/bin/clang-format.exe']
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    raise SystemExit('Install clang-format==20.1.8, or pass --clang-format PATH')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Report formatting differences without editing')
    parser.add_argument('--write', action='store_true', help='Apply the checked-in style')
    parser.add_argument('--verify-ref', help='Compare C tokens and directives with this immutable Git ref')
    parser.add_argument('--clang-format')
    parser.add_argument('--report', default='build/style_audit/result.json')
    parser.add_argument('--exclude', action='append', default=[], help='Exact project-relative path to skip')
    args = parser.parse_args()
    if args.check and args.write:
        parser.error('--check and --write are mutually exclusive')
    if not (args.check or args.write or args.verify_ref):
        parser.error('Choose --check, --write or --verify-ref')
    formatter = formatter_path(args.clang_format) if args.check or args.write else None
    version = subprocess.check_output([formatter, '--version'], text=True).strip() if formatter else None
    if version and not re.search(r'\b20\.1\.8\b', version):
        raise SystemExit('Use clang-format 20.1.8 for reproducible layout: ' + version)
    rows, failures = [], []
    for path in source_files():
        rel = path.relative_to(ROOT).as_posix()
        if rel in args.exclude:
            continue
        original = path.read_bytes()
        text, encoding = decode(original)
        row = dict(file=rel, before=metrics(text))
        if formatter:
            prepared = expand_declaration_comments(text, path.suffix)
            result = subprocess.run([formatter, '--style=file', '--assume-filename=' + str(path)],
                                    input=prepared.encode('utf-8'), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if result.returncode:
                raise RuntimeError(rel + ': ' + result.stderr.decode(errors='replace'))
            formatted = result.stdout.decode('utf-8')
            formatted = '\n'.join(
                re.sub(r'^[ \t]+', lambda match: match[0].expandtabs(4), line).rstrip(' \t')
                for line in formatted.splitlines()) + '\n'
            if token_stream(text) != token_stream(formatted) or directives(text) != directives(formatted):
                raise RuntimeError('Formatter changed C tokens or directive boundaries: ' + rel)
            changed = formatted != text
            row['formatting_difference'] = changed
            if args.write and changed:
                path.write_bytes(formatted.encode(encoding))
            if args.check and changed:
                failures.append(rel + ': formatting difference')
            if args.write:
                text = formatted
        if args.verify_ref:
            previous = subprocess.check_output(['git', 'show', args.verify_ref + ':' + rel], cwd=ROOT)
            baseline, _ = decode(previous)
            row['baseline'] = metrics(baseline)
            row['tokens_unchanged'] = token_stream(baseline) == token_stream(text)
            row['directives_unchanged'] = directives(baseline) == directives(text)
            if not row['tokens_unchanged'] or not row['directives_unchanged']:
                failures.append(rel + ': source preservation mismatch')
            if re.search(r'\b__LINE__\b', uncomment(baseline)) and baseline != text:
                failures.append(rel + ': __LINE__ requires manual review')
        row['after'] = metrics(text)
        row['sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
        rows.append(row)
    report = dict(formatter=version, baseline_ref=args.verify_ref, custom_source_files=len(rows),
                  scope='Source C/C++ except TI 00_Device and 01_Drive libraries',
                  files=rows, failures=failures)
    for kind in ('baseline', 'before', 'after'):
        report[kind] = {metric: sum(row.get(kind, {}).get(metric, 0) for row in rows)
                        for metric in ('lines', 'multiple_statement_lines', 'code_lines_over_120')}
    target = ROOT / args.report
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({key: value for key, value in report.items() if key != 'files'}, ensure_ascii=False, indent=2))
    print('Report:', target)
    raise SystemExit(bool(failures))


if __name__ == '__main__':
    main()
