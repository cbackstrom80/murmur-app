from __future__ import annotations

import re
from pathlib import Path

from .paths import UI_SRC_DIR
from .policies import policy_for_spec
from .spec import ValidationIssue, load_layout


_LAYOUT_LITERAL_RE = re.compile(
    r"(?:removeFrom(?:Top|Bottom|Left|Right)|setBounds|setSize)\s*\(\s*(\d+)\s*(?:,\s*(\d+))?",
)
_MAGIC_NUMBER_RE = re.compile(r"\b(\d{2,4})\b")


def validate_reverse_audit(spec: dict, source: str) -> list[ValidationIssue]:
    """Flag layout-sized numeric literals in C++ owners that aren't named constants."""
    issues: list[ValidationIssue] = []
    policy = policy_for_spec(spec)
    owners = policy.get("cppOwners") or []
    if not owners:
        code_owners = spec.get("codeOwners") or {}
        for val in code_owners.values():
            if val.endswith(".h"):
                continue
            owners.append(f"{val}.cpp")

    from .cpp_constants import parse_play_mode_layout
    from .paths import PLAY_MODE_LAYOUT_H

    cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
    cpp_values = {v for v in cpp.values() if isinstance(v, int)}

    figma_dims: set[int] = set()

    def walk(node: dict) -> None:
        for key in ("width", "height", "x", "y"):
            if node.get(key) is not None:
                figma_dims.add(int(node[key]))
        for child in node.get("children") or []:
            if isinstance(child, dict):
                walk(child)

    for child in spec.get("children") or []:
        if isinstance(child, dict):
            walk(child)
    size = spec.get("size") or {}
    for key in ("width", "height"):
        if size.get(key) is not None:
            figma_dims.add(int(size[key]))

    interesting = {n for n in figma_dims if n >= 28}

    for owner in owners:
        cpp_path = _resolve_cpp(owner)
        if not cpp_path or not cpp_path.is_file():
            continue
        text = cpp_path.read_text(encoding="utf-8")
        for match in _LAYOUT_LITERAL_RE.finditer(text):
            for g in match.groups():
                if g is None:
                    continue
                val = int(g)
                if val in interesting and val not in cpp_values:
                    issues.append(
                        ValidationIssue(
                            "warn",
                            source,
                            f"{owner}: layout literal {val} may belong in PlayModeLayout.h",
                        )
                    )
    return issues


def _resolve_cpp(owner: str) -> Path | None:
    name = owner if owner.endswith(".cpp") else f"{owner}.cpp"
    candidates = list(UI_SRC_DIR.rglob(name))
    if not candidates:
        return None
    return candidates[0]
