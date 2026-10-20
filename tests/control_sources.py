"""Helpers for host tests that inline the split CHB/DAB control sources."""
from pathlib import Path
import re

CONTROL_FILES = (
    "ChbDabControl.c",
    "ChbDabControlFast.c",
    "ChbDabControlService.c",
)


def strip_local_includes(source: str) -> str:
    """Remove project-local includes; host scaffolds provide their fake ABI."""
    return re.sub(r"^[ \t]*#[ \t]*include[ \t]*\"[^\"\n]+\"[ \t]*$", "", source,
                  flags=re.MULTILINE)


def control_paths(project: Path) -> list[Path]:
    root = project / "Source/04_App/04_Control"
    return [root / name for name in CONTROL_FILES]


def control_source(project: Path) -> str:
    """Return all split control implementations in link-order-independent form."""
    return '#include "ChbDabControlInternal.h"\n' + "\n".join(
        strip_local_includes(path.read_text(encoding="utf-8-sig"))
        for path in control_paths(project))
