"""Location-independent lexical inventories for a reviewed framework refactor.

This is a source preservation check, not a preprocessor, linker or timing proof.
All conditional branches are inventoried. Include/guard changes are structural;
function signatures/bodies, individual object declarations, typedefs, macros,
and DATA_SECTION mappings retain their executable tokens and branch context.
The historical comment baseline is never read as an output or changed here.
"""
from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
import hashlib
import re
from pathlib import Path
from tree_sitter import Language, Parser
import tree_sitter_c
import tree_sitter_cpp


def decode(data: bytes) -> str:
    for encoding in ("utf-8-sig", "gb18030"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            pass
    raise ValueError("Unknown source encoding")


def tokens(text: str) -> tuple[str, ...]:
    # Strings and character literals are single tokens so spaces inside them
    # remain significant; comments, external whitespace and CRLF do not.
    pattern = r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|/\*[\s\S]*?\*/|//[^\n]*'
    clean = re.sub(pattern, lambda match: match[1] or " ", text)
    return tuple(re.findall(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s\w]', clean))


def lexical_hash(text: str) -> str:
    return hashlib.sha256("\0".join(tokens(text)).encode("utf-8")).hexdigest()


@dataclass(frozen=True)
class Entity:
    kind: str
    name: str
    file: str
    line: int
    text: str
    context: tuple[str, ...] = ()

    @property
    def key(self) -> str:
        return self.kind + ":" + self.name

    @property
    def sha256(self) -> str:
        # HVAC is the fixed project port (audited independently), and C linkage
        # wrappers in public headers are an interface organization change. Keep
        # these original/new conditions in the report without pretending a move
        # changed executable declaration tokens. All other contexts stay hashed.
        effective = [condition for condition in self.context
                     if condition not in ("#ifdef HVAC_online", "#ifdef __cplusplus")]
        contents = re.sub(r'^\s*extern\s+(?!"C")', "", self.text) if self.kind == "prototype" else self.text
        return lexical_hash("\n".join(effective) + "\n" + contents)

    def location(self) -> dict:
        return dict(file=self.file, line=self.line, sha256=self.sha256,
                    conditions=list(self.context))


def identifier(node):
    if node is None:
        return None
    if node.type in ("identifier", "field_identifier", "qualified_identifier", "type_identifier"):
        return node
    return identifier(node.child_by_field_name("declarator"))


def descendants(node):
    yield node
    for child in node.named_children:
        yield from descendants(child)


def inventory(text: str, file: str) -> list[Entity]:
    data = text.encode("utf-8")
    language = tree_sitter_cpp if Path(file).suffix.lower() in (".cpp", ".hpp") else tree_sitter_c
    root = Parser(Language(language.language())).parse(data).root_node
    result: list[Entity] = []
    conditional_lines: dict[int, tuple[str, ...]] = {}
    stack: list[str] = []
    comment_pattern = r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|/\*[\s\S]*?\*/|//[^\n]*'
    directive_text = re.sub(comment_pattern, lambda match: match[1] or "\n" * match[0].count("\n"), text)
    guards = set(re.findall(r'^[ \t]*#\s*ifndef[ \t]+(\w+)[ \t]*\r?\n[ \t]*#\s*define[ \t]+\1[ \t]*(?:\r?\n|$)',
                            directive_text, re.MULTILINE))
    for line_number, line in enumerate(directive_text.splitlines(), 1):
        conditional_lines[line_number] = tuple(condition for condition in stack if condition)
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)', line)
        if not directive:
            continue
        kind, expression = directive[1], directive[2].strip()
        if kind in ("if", "ifdef", "ifndef"):
            stack.append("" if kind == "ifndef" and expression in guards else
                         "#" + kind + " " + " ".join(tokens(expression)))
        elif kind == "endif" and stack:
            stack.pop()
        elif kind in ("else", "elif") and stack:
            stack[-1] += " / #" + kind + " " + " ".join(tokens(expression))

    def txt(node):
        return data[node.start_byte:node.end_byte].decode() if node is not None else ""

    def add(kind, name, node, contents=None):
        line = data.count(b"\n", 0, node.start_byte) + 1
        result.append(Entity(kind, name, file, line,
                             txt(node) if contents is None else contents,
                             conditional_lines.get(line, ())))

    def scan(node):
        if node.type == "function_definition":
            name = identifier(node.child_by_field_name("declarator"))
            if name is not None:
                add("function", txt(name), node)
            # Locals are included in the whole function; do not count them as
            # module-owned objects, prototypes or typedefs a second time.
            return
        if node.type == "type_definition":
            for declarator in node.children_by_field_name("declarator"):
                name = identifier(declarator)
                if name is not None:
                    add("type", txt(name), node)
            return
        if node.type == "declaration":
            declarators = node.children_by_field_name("declarator")
            if declarators:
                prefix = data[node.start_byte:declarators[0].start_byte].decode()
                for declarator in declarators:
                    name = identifier(declarator)
                    if name is None:
                        continue
                    is_function = any(child.type == "function_declarator"
                                      for child in descendants(declarator))
                    kind = "prototype" if is_function else "extern" if re.search(r'\bextern\b', prefix) else "object"
                    # Treat a,b as two declarations, allowing their definitions
                    # to move independently without losing type/initial-value checks.
                    add(kind, txt(name), node, prefix + txt(declarator) + ";")
                return
        if node.type in ("preproc_def", "preproc_function_def"):
            name = node.child_by_field_name("name")
            value = node.child_by_field_name("value")
            spelling = txt(name)
            empty = value is None or not txt(value).strip()
            if empty and (spelling.endswith(("_H_", "_H", "_HPP_", "_HPP")) or
                          spelling.startswith("_") and "H" in spelling):
                return
            add("macro", spelling, node)
            return
        if node.type == "preproc_call":
            contents = txt(node)
            match = re.match(r'\s*#\s*pragma\s+DATA_SECTION\s*\(\s*(\w+)\s*,', contents)
            if match:
                add("section", match[1], node)
            return
        for child in node.named_children:
            scan(child)

    scan(root)
    return result


def source_paths(root: Path) -> list[Path]:
    return sorted(path for path in (root / "Source").rglob("*")
                  if path.is_file() and path.suffix.lower() in (".c", ".cpp", ".h", ".hpp")
                  and not any(part in ("00_Device", "01_Drive") for part in path.relative_to(root).parts))


def file_inventory(path: Path, root: Path | None = None) -> list[Entity]:
    return inventory(decode(path.read_bytes()), path.relative_to(root).as_posix() if root else str(path))


def source_inventory(root: Path) -> list[Entity]:
    return [entity for path in source_paths(root) for entity in file_inventory(path, root)]


def missing_entities(original: list[Entity], current: list[Entity]) -> list[Entity]:
    """Preserve every original semantic entity regardless of its new location."""
    available = Counter((entity.key, entity.sha256) for entity in current)
    missing = []
    declarations = set()
    for entity in original:
        pair = (entity.key, entity.sha256)
        if entity.kind in ("prototype", "extern") and pair in declarations:
            continue
        if entity.kind in ("prototype", "extern"):
            declarations.add(pair)
        if available[pair]:
            available[pair] -= 1
        else:
            missing.append(entity)
    return missing
