"""Audit Chinese documentation against exact Tree-sitter declaration spans.

Documentation-only dependencies: tree-sitter, tree-sitter-c, tree-sitter-cpp.
The immutable historical baseline and its informational token-equality fields
are retained. Function banners may omit meaningless void outputs/returns and
execution boilerplate; every actual parameter and non-void return still needs
specific documentation. Comments must attach to the actual declarator, enum or
macro rather than happen to occur later in the file.
"""
from bisect import bisect_left, bisect_right
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from comment_inventory import source_files, collect, decode, walk, parse_source
from annotate_source import token_stream

CHINESE = re.compile(r'[\u4e00-\u9fff]')
LABEL = re.compile(r'^\s*\*?\s*(函数名称|函数功能|输入参数|输出参数|返回值|执行说明|调用说明|算法说明|帧格式)\s*[:：]\s*(.*)$')
CONDITIONAL = re.compile(r'^\s*#\s*(?:if|ifdef|ifndef|elif|else|endif)\b[^\n]*(?:\n|$)', re.M)


def sections(comment):
    """Read banner labels with either colon and optional alignment spaces."""
    comment = re.sub(r'^\s*/\*', '', comment)
    comment = re.sub(r'\*/\s*$', '', comment)
    result = {}
    current = None
    for line in comment.splitlines():
        line = re.sub(r'^\s*/\*\s?', '', line)
        match = LABEL.match(line)
        if match:
            current = match[1]
            result[current] = match[2].strip()
        elif current and not re.match(r'^\s*\*?\s*[=*/-]{3,}', line):
            result[current] += '\n' + re.sub(r'^\s*\*?\s?', '', line).strip()
    return result


def descriptive(comment):
    # A section/file banner does not document the variable following the banner.
    return bool(CHINESE.search(comment)) and not re.search(r'={4,}', comment)


def names_parameter(text, name):
    # Support individually named inputs and a/b or a,b grouped inputs followed
    # by a concrete Chinese explanation; mentioning the name elsewhere is not enough.
    pattern = (r'(?<![\w])' + re.escape(name) +
               r'(?:\s*[/、,，]\s*[A-Za-z_]\w*)*\s*[:：]\s*([^\n]+)')
    def concrete(description):
        return bool(CHINESE.search(description) or re.search(r'\d+\s*(?:\.\.|[～~])\s*\d+', description))
    if any(concrete(match[1]) for match in re.finditer(pattern, text)):
        return True
    # A numbered channel range (f1～f12) describes every argument in that
    # explicit range without requiring twelve duplicate sentences.
    numbered = re.fullmatch(r'([A-Za-z_]+)(\d+)', name)
    if numbered:
        ranges = re.finditer(r'(?<![\w])([A-Za-z_]+)(\d+)\s*[～~]\s*\1(\d+)\s*[:：]\s*([^\n]+)', text)
        return any(match[1] == numbered[1] and int(match[2]) <= int(numbered[2]) <= int(match[3])
                   and concrete(match[4]) for match in ranges)
    return False


def explicitly_named(comment, name):
    return bool(re.search(r'(?<![\w])' + re.escape(name) + r'\s*[:：]', comment))


class AttachedComments:
    """Only return adjacent comments with no intervening executable syntax."""

    def __init__(self, data, root):
        self.data = data
        self.items = sorted((n for n in walk(root) if n.type == 'comment'),
                            key=lambda n: n.start_byte)
        self.starts = [n.start_byte for n in self.items]
        self.ends = [n.end_byte for n in self.items]

    def text(self, comment):
        return self.data[comment.start_byte:comment.end_byte].decode()

    def leading(self, offset, named=None):
        index = bisect_right(self.ends, offset) - 1
        if index < 0:
            return None
        comment = self.items[index]
        gap = self.data[comment.end_byte:offset].decode()
        gap = CONDITIONAL.sub('', gap)
        if not re.fullmatch(r'\s*(?:extern\s+"C"\s*\{?\s*)?', gap):
            return None
        line_start = self.data.rfind(b'\n', 0, comment.start_byte) + 1
        prefix = self.data[line_start:comment.start_byte].strip()
        # An earlier declaration's trailing comment cannot document the next item.
        if prefix and not (named and explicitly_named(self.text(comment), named)):
            return None
        return comment

    def inline_after(self, offset, limit=None):
        index = bisect_left(self.starts, offset)
        if index >= len(self.items):
            return None
        comment = self.items[index]
        if limit is not None and comment.end_byte > limit:
            return None
        gap = self.data[offset:comment.start_byte]
        return comment if re.fullmatch(rb'[ \t,;]*', gap) else None

    def named_after_declaration(self, offset, name):
        # Legacy layout: one named block per member after the semicolon. The
        # comment run must remain contiguous and stop before any new C token.
        for comment in self.items[bisect_left(self.starts, offset):]:
            gap = self.data[offset:comment.start_byte]
            if gap.strip() or gap.count(b'\n') > 2:
                break
            text = self.text(comment)
            if descriptive(text) and explicitly_named(text, name):
                return comment
            offset = comment.end_byte
        return None

    def declaration(self, symbol):
        start = symbol['declarator_start']
        end = symbol['declarator_end']
        limit = symbol['next_declarator_start'] if symbol['declarator_index'] + 1 < symbol['declarator_count'] else None
        comment = self.inline_after(end, limit)
        if comment and descriptive(self.text(comment)):
            return comment
        comment = self.leading(start, symbol['name'])
        if comment and descriptive(self.text(comment)):
            return comment
        comment = self.leading(symbol['start'], symbol['name'])
        if comment and descriptive(self.text(comment)):
            if symbol['declarator_count'] == 1 or explicitly_named(self.text(comment), symbol['name']):
                return comment
        return self.named_after_declaration(symbol['end'], symbol['name'])


def is_void_function(symbol):
    if not symbol.get('return_type'):
        return True                     # C++ constructor/destructor has no return.
    prefix = symbol['declaration'].split(symbol['name'], 1)[0]
    return symbol['return_type'] == 'void' and '*' not in prefix and '&' not in prefix


def audit(data, root, symbols):
    """Return issues and per-symbol comment-span evidence without mutating source."""
    issues = []
    evidence = []
    attached = AttachedComments(data, root)
    function_docs = {}

    def issue(symbol, message):
        issues.append(dict(kind=symbol['kind'], name=symbol['name'],
                           line=symbol['line'], message=message))

    def record(symbol, comment):
        evidence.append(dict(kind=symbol['kind'], name=symbol['name'], line=symbol['line'],
                             comment_start=comment.start_byte, comment_end=comment.end_byte,
                             comment_line=data.count(b'\n', 0, comment.start_byte) + 1))

    for symbol in symbols:
        if symbol['kind'] not in ('function', 'prototype'):
            continue
        comment = attached.leading(symbol['start'])
        doc = sections(attached.text(comment)) if comment else {}
        function_docs[symbol['start']] = (doc, comment)
        if not CHINESE.search(doc.get('函数功能', '')) or not doc.get('输入参数', '').strip():
            issue(symbol, 'undocumented function')
        if not is_void_function(symbol):
            value = doc.get('返回值', '').strip().rstrip('。.')
            if not value or value in ('无', '无返回', '无返回值'):
                issue(symbol, 'non-void function missing return description')
        if comment and doc:
            record(symbol, comment)

    for symbol in symbols:
        if symbol['kind'] in ('variable', 'field'):
            comment = attached.declaration(symbol)
            if not comment:
                issue(symbol, 'undocumented declaration')
            else:
                record(symbol, comment)
        elif symbol['kind'] == 'parameter':
            doc, comment = function_docs.get(symbol.get('function_start'), ({}, None))
            if not doc and symbol.get('function_start') is not None:
                # Function-pointer typedefs also own parameter contracts, even
                # though the type itself is not an executable function symbol.
                comment = attached.leading(symbol['function_start'])
                doc = sections(attached.text(comment)) if comment else {}
            if not names_parameter(doc.get('输入参数', ''), symbol['name']):
                issue(symbol, 'undocumented parameter')
            elif comment:
                record(symbol, comment)
        elif symbol['kind'] == 'enum':
            comment = attached.inline_after(symbol['end']) or attached.leading(symbol['start'])
            if not comment or not descriptive(attached.text(comment)):
                issue(symbol, 'undocumented enum constant')
            else:
                record(symbol, comment)

    for node in walk(root):
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
        symbol = dict(kind='macro', name=name, line=data.count(b'\n', 0, node.start_byte) + 1)
        comment = attached.leading(node.start_byte, name)
        if not comment or not descriptive(attached.text(comment)):
            # A macro's trailing comment is inside the preprocessor AST node.
            comment = next((c for c in attached.items
                            if nn.end_byte <= c.start_byte < node.end_byte
                            and data.count(b'\n', nn.end_byte, c.start_byte) == 0
                            and descriptive(attached.text(c))), None)
        if not comment or not descriptive(attached.text(comment)):
            issue(symbol, 'undocumented compile macro')
        else:
            record(symbol, comment)
    return issues, evidence


def self_test():
    """Regression fixtures prove new layouts pass and missing comments still fail."""
    banner = ('/*==================================================================\n'
              ' * 函数功能: 计算测试值。\n * 输入参数: x：输入测试值。\n'
              ' * 返回值  : 输入值。\n *================================================================*/\n')
    fixtures = [
        ('comma trailing', 'int a, /* 第一通道计数。 */\n b; /* 第二通道计数。 */', 0),
        ('legacy named', 'int a,b; /* a：第一通道计数。 */\n/* b：第二通道计数。 */', 0),
        ('preceding', '/* 单个运行标志。 */\nint running;', 0),
        ('enum trailing', 'enum E { A=1, /* 停止状态。 */\n B=2 /* 运行状态。 */ };', 0),
        ('macro trailing', '#define COUNT 4U /* 波形通道数。 */\n', 0),
        ('numbered parameter range', banner.replace('x：输入测试值。', 'f1～f2：依次为两路输入。') + 'int f(int f1,int f2) {return f1;}', 0),
        ('numeric parameter range', banner.replace('x：输入测试值。', 'x：0..53') + 'int f(int x) {return x;}', 0),
        ('callback typedef', banner.replace('x：输入测试值。', 'value：收到的单字节。') + 'typedef void (*CALLBACK)(int value);', 0),
        ('function compact labels', banner + 'int f(int x) {return x;}', 0),
        ('void no return', '/* 函数功能: 清空测试状态。\n输入参数: 无。 */\nvoid f(void) {}', 0),
        ('missing first declarator', 'int a,b; /* 第二通道计数。 */', 1),
        ('remote comment rejected', 'int a;\nint b; /* b：第二通道计数。 */', 1),
        ('previous trailing rejected', 'int a; /* 第一通道计数。 */\nint b;', 1),
        ('missing parameter', banner.replace('x：输入测试值。', '无。') + 'int f(int x) {return x;}', 1),
        ('missing return', banner.replace(' * 返回值  : 输入值。\n', '') + 'int f(int x) {return x;}', 1),
        ('wrong no return', banner.replace('返回值  : 输入值。', '返回值 : 无。') + 'int f(int x) {return x;}', 1),
        ('inline no return rejected', '/* 函数功能: 获取测试值。\n输入参数: 无。\n返回值: 无。 */\nint f(void) {return 0;}', 1),
        ('pointer return needs contract', '/* 函数功能: 获取数据地址。\n输入参数: 无。 */\nvoid *f(void) {return 0;}', 1),
        ('named preceding group', '/* a：第一通道计数。 */\nint a,b;', 1),
        ('beyond numbered range rejected', banner.replace('x：输入测试值。', 'f1～f2：依次为两路输入。') + 'int f(int f3) {return f3;}', 1),
        ('missing enum', 'enum E { A=1, /* 停止状态。 */\n B=2 };', 1),
        ('section not a declaration doc', '/*=========== 私有变量 ===========*/\nint a;', 1),
    ]
    for name, text, expected in fixtures:
        data, root, symbols = parse_source(text, '.cpp')
        issues, _ = audit(data, root, symbols)
        assert len(issues) == expected, (name, expected, issues)
    conditional = (banner.replace('x：输入测试值。', '无。') +
                   'void first(void) {\n#if TEST\n if (left)\n#else\n if (right)\n#endif\n'
                   ' { int count = 0; /* 中文计数变量。 */ }\n}\n' + banner +
                   'int last(int x) { int local = x; /* 中文局部值。 */ return local; }')
    for suffix in ('.c', '.cpp'):
        data, root, symbols = parse_source(conditional, suffix)
        assert [s['name'] for s in symbols if s['kind'] == 'function'] == ['first', 'last']
        last = next(s for s in symbols if s['kind'] == 'function' and s['name'] == 'last')
        local = next(s for s in symbols if s['kind'] == 'variable' and s['name'] == 'local')
        assert last['start'] == data.index(b'int last(')
        assert local['function_start'] == last['start'] and local['owner'] == 'last'
        assert data[local['declarator_start']:local['declarator_end']] == b'local = x'
        issues, _ = audit(data, root, symbols)
        assert not issues, (suffix, issues)
    print('Comment audit self-tests: PASS;', len(fixtures),
          'positive/negative fixtures; Chinese conditional-recovery byte spans in C/C++')


def main():
    if '--self-test' in sys.argv:
        self_test()
        return 0
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
        issues, evidence = audit(data, root, symbols)
        failures.extend(f"{rel}:{item['line']}: {item['message']} {item['name']}" for item in issues)
        counts.update(symbol['kind'] for symbol in symbols)
        results.append(dict(file=rel, executable_tokens_unchanged=unchanged,
                            baseline_sha256=baseline_hash,
                            annotated_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                            symbol_counts=dict(Counter(symbol['kind'] for symbol in symbols)),
                            documentation_evidence=evidence))
    report = dict(custom_source_files=len(results), symbol_counts=dict(counts),
                  failures=failures, files=results,
                  scope='Source except vendor 00_Device and 01_Drive trees; includes legacy conditional branches',
                  method='Tree-sitter exact declarators + adjacent Chinese documentation; named parameter and non-void return contracts; historical per-file tokens are informational only',
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
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
