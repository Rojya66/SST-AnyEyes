"""Expand domain initialization calls and verify every legacy statement/directive.

A relocation may introduce new scopes and local declarations, but the executable
statement token stream, preprocessor directives and step order must remain exact.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
from tree_sitter import Language, Parser
import tree_sitter_c

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from annotate_source import token_stream

PARSER = Parser(Language(tree_sitter_c.language()))

def walk(node):
    yield node
    for child in node.named_children:
        yield from walk(child)

def identifier(node):
    if node is None:
        return None
    if node.type == 'identifier':
        return node
    return identifier(node.child_by_field_name('declarator'))

def get_function(data, name):
    matches = []
    for node in walk(PARSER.parse(data).root_node):
        if node.type != 'function_definition':
            continue
        named = identifier(node.child_by_field_name('declarator'))
        if named and data[named.start_byte:named.end_byte].decode() == name:
            matches.append(node)
    if len(matches) != 1:
        raise AssertionError(f'{name}: expected one definition, found {len(matches)}')
    return matches[0]

def contexts(data, node):
    result = []
    parent = node.parent
    while parent:
        if parent.type.startswith('preproc_'):
            directive = data[parent.start_byte:data.index(b'\n', parent.start_byte)].decode().strip()
            result.append(directive)
        parent = parent.parent
    return list(reversed(result))

def inner_and_locals(data, function):
    body = function.child_by_field_name('body')
    leading = []
    for child in body.named_children:
        if child.type == 'comment':
            continue
        if child.type != 'declaration':
            break
        leading.append(child)
    declarations = {}
    for node in leading:
        if b'=' in data[node.start_byte:node.end_byte]:
            raise AssertionError('Relocated local variables may not introduce initializers')
        declared = identifier(node.child_by_field_name('declarator'))
        declarations[data[declared.start_byte:declared.end_byte].decode()] = token_stream(
            data[node.start_byte:node.end_byte].decode())
    inner = data[body.start_byte + 1:body.end_byte - 1]
    for node in reversed(leading):
        start, end = node.start_byte - body.start_byte - 1, node.end_byte - body.start_byte - 1
        inner = inner[:start] + inner[end:]
    return inner, declarations

def verify():
    contract = json.loads((ROOT / 'tools/control_init_segments.json').read_text('utf-8'))
    original = subprocess.check_output(['git', 'show', contract['baseline'] + ':' + contract['original_file']], cwd=ROOT)
    coordinator = (ROOT / contract['coordinator']).read_bytes()
    results = []
    for name, key in [('Initial_Variable', 'initial'), ('Variable_RST', 'reset')]:
        old_fn = get_function(original, name)
        current_fn = get_function(coordinator, name)
        assert contexts(original, old_fn) == contexts(coordinator, current_fn), name + ': outer condition changed'
        original_inner, old_locals = inner_and_locals(original, old_fn)
        current_inner, current_locals = inner_and_locals(coordinator, current_fn)
        assert not current_locals, name + ': coordinator must not own domain locals'
        calls = []
        for statement in current_fn.child_by_field_name('body').named_children:
            if statement.type == 'comment':
                continue
            assert statement.type == 'expression_statement', name + ': unexpected coordinator statement'
            expression = statement.named_children[0]
            assert expression.type == 'call_expression', name + ': unexpected coordinator expression'
            target = expression.child_by_field_name('function')
            arguments = expression.child_by_field_name('arguments')
            assert not arguments.named_children, name + ': helper arguments would need explicit equivalence'
            calls.append(coordinator[target.start_byte:target.end_byte].decode())
        assert calls == [step['name'] for step in contract[key]], name + ': step order changed'
        expanded = b''
        for step in contract[key]:
            data = (ROOT / step['file']).read_bytes()
            helper = get_function(data, step['name'])
            assert contexts(data, helper) == contexts(original, old_fn), step['name'] + ': helper outer condition changed'
            inner, relocated = inner_and_locals(data, helper)
            for local, declaration in relocated.items():
                assert local in old_locals and declaration == old_locals[local], step['name'] + ': altered local declaration'
            expanded += inner
        before, after = token_stream(original_inner.decode()), token_stream(expanded.decode())
        if before != after:
            comparison = 0 if before[0] != after[0] else 1
            left, right = before[comparison], after[comparison]
            mismatch = next((i for i, pair in enumerate(zip(left, right)) if pair[0] != pair[1]), min(len(left), len(right)))
            raise AssertionError(f'{name}: executable token/directive mismatch at {mismatch}: {left[mismatch:mismatch+6]} != {right[mismatch:mismatch+6]}')
        results.append(dict(name=name, steps=len(calls), executable_tokens=len(before[0]), preprocessor_directives=len(before[1]),
                            ordered_tokens_and_directives_equal=True,
                            token_sha256=hashlib.sha256(json.dumps(before, ensure_ascii=False).encode()).hexdigest()))
    return dict(baseline=contract['baseline'], method='Expand ordered no-argument helpers; check relocated local types; compare full original executable token and preprocessor stream', results=results)

if __name__ == '__main__':
    report = verify()
    target = ROOT / 'build/control_init_equivalence.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    for result in report['results']:
        print('PASS:', result['name'], result['steps'], 'steps;', result['executable_tokens'], 'original executable tokens/directives preserved')
    print('Report:', target)
