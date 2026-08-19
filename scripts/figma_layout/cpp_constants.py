from __future__ import annotations

import re

from .paths import PLAY_MODE_LAYOUT_H

_INT_RE = re.compile(r"inline\s+constexpr\s+int\s+(\w+)\s*=\s*(-?\d+)\s*;")
_ARRAY_RE = re.compile(r"inline\s+constexpr\s+std::array<int,\s*\d+>\s+(\w+)\s*=\s*\{([^}]+)\}\s*;")


def parse_play_mode_layout(path=PLAY_MODE_LAYOUT_H) -> dict[str, int | list[int]]:
    text = path.read_text(encoding="utf-8")
    constants: dict[str, int | list[int]] = {}

    for match in _INT_RE.finditer(text):
        constants[match.group(1)] = int(match.group(2))

    for match in _ARRAY_RE.finditer(text):
        name = match.group(1)
        nums = [int(x.strip()) for x in match.group(2).split(",") if x.strip()]
        constants[name] = nums

    return constants
