"""Inventory application-owned C/C++ symbols; vendor device/driver trees excluded."""
from pathlib import Path
import json
from tree_sitter import Language, Parser
import tree_sitter_c
import tree_sitter_cpp

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = ('Source/00_Device/', 'Source/01_Drive/')

def source_files():
    return sorted(p for p in (ROOT / 'Source').rglob('*')
                  if p.suffix.lower() in ('.c', '.cpp', '.h', '.hpp')
                  and not any(p.relative_to(ROOT).as_posix().startswith(x) for x in EXCLUDED))

def decode(data):
    for encoding in ('utf-8-sig', 'gb18030'):
        try:
            return data.decode(encoding), encoding
        except UnicodeDecodeError:
            pass
    raise ValueError('Unknown source encoding')

def walk(node):
    yield node
    for child in node.named_children:
        yield from walk(child)

def identifier(node):
    if node is None:
        return None
    if node.type in ('identifier', 'field_identifier', 'qualified_identifier', 'type_identifier'):
        return node
    child = node.child_by_field_name('declarator')
    if child is None and node.type == 'parenthesized_declarator':
        child = next((n for n in node.named_children if n.type != 'comment'), None)
    return identifier(child) if child is not None else None

def parse_source(raw, suffix):
    """Parse without changing source files or the immutable comment baseline.

    Existing start/end fields continue to describe whole declaration nodes.
    Exact declarator and owner spans bind documentation to an individual item
    of a comma-separated declaration without breaking older inventory users.
    """
    data = raw.encode('utf-8')
    parser = Parser(Language(tree_sitter_cpp.language() if suffix.lower() in ('.cpp', '.hpp') else tree_sitter_c.language()))
    root = parser.parse(data).root_node
    symbols = []
    def txt(n):
        return data[n.start_byte:n.end_byte].decode() if n else ''
    for n in walk(root):
        if n.type == 'function_definition':
            d = n.child_by_field_name('declarator')
            name = identifier(d)
            if name:
                params = [x for x in walk(d) if x.type == 'parameter_declaration']
                symbols.append(dict(kind='function', name=txt(name), line=data.count(b'\n', 0, n.start_byte) + 1,
                                    start=n.start_byte, end=n.end_byte, declaration=txt(n).split('{')[0].strip(),
                                    parameters=[txt(x) for x in params],
                                    return_type=txt(n.child_by_field_name('type')),
                                    declarator_start=d.start_byte, declarator_end=d.end_byte))
        elif n.type in ('declaration', 'field_declaration', 'parameter_declaration'):
            if n.type == 'parameter_declaration':
                declarator = n.parent
                while declarator and declarator.type != 'function_declarator':
                    declarator = declarator.parent
                # Conditional preprocessor branches can make a recovery parser
                # interpret an "else if" expression as a function parameter.
                candidate = identifier(declarator)
                if candidate and txt(candidate) in ('if', 'for', 'while', 'switch', 'catch'):
                    continue
            scope = n.parent
            while scope and scope.type not in ('function_definition', 'struct_specifier', 'class_specifier', 'translation_unit'):
                scope = scope.parent
            owner = identifier(scope.child_by_field_name('declarator')) if scope and scope.type == 'function_definition' else scope.child_by_field_name('name') if scope else None
            declarators = n.children_by_field_name('declarator')
            for index, d in enumerate(declarators):
                name = identifier(d)
                if name:
                    isfunc = any(x.type == 'function_declarator' for x in walk(d))
                    function_owner = n.parent
                    while function_owner and function_owner.type not in ('function_definition', 'declaration', 'field_declaration', 'type_definition'):
                        function_owner = function_owner.parent
                    function_name = identifier(function_owner.child_by_field_name('declarator')) if function_owner else None
                    symbols.append(dict(kind='prototype' if isfunc else 'parameter' if n.type == 'parameter_declaration' else 'field' if n.type == 'field_declaration' else 'variable',
                                        name=txt(name), owner=txt(owner), line=data.count(b'\n', 0, d.start_byte) + 1,
                                        start=n.start_byte, end=n.end_byte, declaration=txt(n),
                                        return_type=txt(n.child_by_field_name('type')) if isfunc else None,
                                        declarator_start=d.start_byte, declarator_end=d.end_byte,
                                        declarator_index=index, declarator_count=len(declarators),
                                        previous_declarator_end=declarators[index - 1].end_byte if index else n.start_byte,
                                        next_declarator_start=declarators[index + 1].start_byte if index + 1 < len(declarators) else n.end_byte,
                                        function_start=function_owner.start_byte if function_owner else None,
                                        function_name=txt(function_name)))
        elif n.type == 'enumerator':
            name = n.child_by_field_name('name')
            symbols.append(dict(kind='enum', name=txt(name), line=data.count(b'\n', 0, n.start_byte) + 1, start=n.start_byte, end=n.end_byte, declaration=txt(n)))
    return data, root, symbols


def collect(path):
    raw, encoding = decode(path.read_bytes())
    data, root, symbols = parse_source(raw, path.suffix)
    return raw, encoding, data, root, symbols

if __name__ == '__main__':
    out = ROOT / 'build/comment_inventory'
    out.mkdir(parents=True, exist_ok=True)
    result = {}
    for path in source_files():
        rel = path.relative_to(ROOT).as_posix()
        _, _, _, _, syms = collect(path)
        result[rel] = syms
        backup = ROOT / 'build/comment_baseline' / rel
        backup.parent.mkdir(parents=True, exist_ok=True)
        if not backup.exists():
            backup.write_bytes(path.read_bytes())
    (out / 'symbols.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    for rel, syms in result.items():
        funcs = [s['name'] for s in syms if s['kind'] == 'function']
        print(rel, len(syms), '\n  ' + ', '.join(funcs))
