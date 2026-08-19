from __future__ import annotations

import re
from pathlib import Path

from .cpp_constants import parse_play_mode_layout
from .paths import FIGMA_KNOB_TOKENS_H, PLAY_MODE_LAYOUT_H
from .spec import ValidationIssue, is_play_mode_constant


_FLOAT_RATIO_RE = re.compile(
    r"static\s+constexpr\s+float\s+(\w+)\s*=\s*([\d.]+)f\s*/\s*([\d.]+)f\s*;"
)


def parse_hero_ratios(path: Path) -> dict[str, dict[str, float]]:
    text = path.read_text(encoding="utf-8")
    ratios: dict[str, dict[str, float]] = {}
    current_struct: str | None = None
    for line in text.splitlines():
        m = re.search(r"struct\s+(\w+Ratios)", line)
        if m:
            current_struct = m.group(1)
            ratios[current_struct] = {}
            continue
        if current_struct:
            fm = _FLOAT_RATIO_RE.search(line)
            if fm:
                ratios[current_struct][fm.group(1)] = float(fm.group(2)) / float(fm.group(3))
    return ratios


def validate_scale_ladder(spec: dict, source: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    if not spec.get("scaleLadder"):
        return issues

    cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
    for entry in spec.get("scaleLadder") or []:
        if not isinstance(entry, dict):
            continue
        const = entry.get("constant")
        px = entry.get("figmaPx")
        if not const or px is None:
            continue
        if not is_play_mode_constant(str(const)):
            continue
        if const not in cpp:
            issues.append(ValidationIssue("warn", source, f"scaleLadder {const} not in PlayModeLayout.h"))
            continue
        if cpp[const] != int(px):
            issues.append(
                ValidationIssue(
                    "error",
                    source,
                    f"scaleLadder {const}={cpp[const]} != figmaPx {px}",
                )
            )
    return issues


def validate_hero_ratios(spec: dict, source: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    if not FIGMA_KNOB_TOKENS_H.is_file():
        return issues

    cpp_ratios = parse_hero_ratios(FIGMA_KNOB_TOKENS_H)
    for variant in spec.get("variants") or []:
        if not isinstance(variant, dict):
            continue
        ratios = variant.get("diameterRatios")
        if not ratios:
            continue
        name = variant.get("name", "")
        struct_key = "DualRingRatios" if "dual" in name else "TripleRingRatios" if "triple" in name else None
        if not struct_key or struct_key not in cpp_ratios:
            continue
        cpp_map = cpp_ratios[struct_key]
        for key, figma_ratio in ratios.items():
            cpp_val = cpp_map.get(key)
            if cpp_val is None:
                continue
            if abs(float(figma_ratio) - cpp_val) > 0.08:
                issues.append(
                    ValidationIssue(
                        "warn",
                        source,
                        f"variant {name} ratio {key} figma={figma_ratio} vs {struct_key}::{key}={cpp_val:.3f}",
                    )
                )
    return issues
