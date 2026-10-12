"""Deterministic fingerprint shared by build and final delivery checks."""
from pathlib import Path
import hashlib

def fingerprint(root: Path) -> str:
    digest = hashlib.sha256()
    inputs = [p for p in (root / 'Source').rglob('*') if p.is_file()]
    inputs += [root / p for p in ('.project', '.cproject', '.ccsproject', 'C6748.cmd')]
    for path in sorted(inputs):
        digest.update(path.relative_to(root).as_posix().encode())
        digest.update(b'\0')
        digest.update(path.read_bytes())
        digest.update(b'\0')
    return digest.hexdigest()
