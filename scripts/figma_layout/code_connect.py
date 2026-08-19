from __future__ import annotations

import re
from pathlib import Path

LAYOUT_JSON_RE = re.compile(r"layout(?:Json|Spec)\s*[=:]\s*['\"]?([^'\"\n]+)", re.I)
FIGMA_NODE_RE = re.compile(r"figmaNodeId['\"]?\s*[:=]\s*['\"](\d+:\d+)['\"]")
LAYOUT_SPEC_META_RE = re.compile(r"layoutSpec:\s*['\"]([^'\"]+)['\"]")


def scan_figma_ts(connect_dir: Path) -> dict[str, list[str]]:
    """Map layout json basename → list of .figma.ts files referencing it."""
    refs: dict[str, list[str]] = {}
    for path in sorted(connect_dir.glob("*.figma.ts")):
        text = path.read_text(encoding="utf-8")
        candidates: set[str] = set()
        for m in LAYOUT_JSON_RE.finditer(text):
            candidates.add(Path(m.group(1)).name)
        for m in LAYOUT_SPEC_META_RE.finditer(text):
            candidates.add(Path(m.group(1)).name)
        if "layouts/" in text:
            for part in re.findall(r"layouts/([a-zA-Z0-9._-]+\.layout\.json)", text):
                candidates.add(part)
        for name in candidates:
            refs.setdefault(name, []).append(path.name)
    return refs
