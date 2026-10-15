"""Compare allocated ELF32 sections while ignoring source/debug metadata.

Use the same C6000 compiler, flags, libraries and macro definitions on both sides.
Equality covers linked instructions, constants, initialization data, RAM layout
and fixed-address allocations. It is not a hardware timing or WCET measurement.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct


def sections(path):
    data = path.read_bytes()
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError('Expected little-endian ELF32: ' + str(path))
    header = struct.unpack_from('<HHIIIIIHHHHHH', data, 16)
    offset, entry_size, count, names_index = header[5], header[10], header[11], header[12]
    entries = [struct.unpack_from('<IIIIIIIIII', data, offset + i * entry_size) for i in range(count)]
    names = entries[names_index]
    strings = data[names[4]:names[4] + names[5]]
    result = {}
    for section in entries:
        name_offset, kind, flags, address, file_offset, size, _, _, alignment, _ = section
        if not flags & 2:  # SHF_ALLOC; omit symbols, debug records and source paths.
            continue
        end = strings.find(b'\0', name_offset)
        name = strings[name_offset:end].decode('utf-8')
        if name in result:
            raise ValueError('Duplicate allocated section: ' + name)
        result[name] = dict(type=kind, flags=flags, address=address, size=size, alignment=alignment,
                            sha256=None if kind == 8 else  # SHT_NOBITS has no file payload.
                            hashlib.sha256(data[file_offset:file_offset + size]).hexdigest())
    return dict(machine=header[1], entry_point=header[3], sections=result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=Path)
    parser.add_argument('after', type=Path)
    parser.add_argument('--report', type=Path, default=Path('build/style_audit/firmware_sections.json'))
    args = parser.parse_args()
    before, after = sections(args.before), sections(args.after)
    names = sorted(set(before['sections']) | set(after['sections']))
    rows = [dict(section=name, unchanged=before['sections'].get(name) == after['sections'].get(name),
                 before=before['sections'].get(name), after=after['sections'].get(name)) for name in names]
    equal = before == after
    report = dict(before=str(args.before.resolve()), after=str(args.after.resolve()),
                  allocated_sections_equal=equal, section_count=len(rows),
                  entry_point_equal=before['entry_point'] == after['entry_point'],
                  sections=rows,
                  limit='Excludes debug metadata; no hardware timing or WCET measurement')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('Allocated ELF sections:', len(rows), 'UNCHANGED' if equal else 'DIFFERENT')
    for row in rows:
        if not row['unchanged']:
            print('DIFFERENT:', row['section'])
    print('Report:', args.report)
    raise SystemExit(0 if equal else 1)


if __name__ == '__main__':
    main()
