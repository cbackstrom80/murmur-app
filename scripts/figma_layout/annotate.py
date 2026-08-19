from __future__ import annotations

import copy
from dataclasses import dataclass
from typing import Any

from .cpp_constants import parse_play_mode_layout
from .paths import PLAY_MODE_LAYOUT_H
from .policies import CODE_MAP_HINTS, CONSTANT_PREFIX, policy_for_spec


@dataclass
class AnnotationSuggestion:
    path: str
    field: str
    value: Any
    reason: str


def _walk_nodes(node: dict, prefix: str) -> list[tuple[dict, str]]:
    out = [(node, prefix)]
    for i, child in enumerate(node.get("children") or []):
        if isinstance(child, dict):
            name = child.get("name") or f"child[{i}]"
            out.extend(_walk_nodes(child, f"{prefix}.{name}"))
    return out


def _match_constant(
    value: int,
    cpp: dict[str, int | list[int]],
    prefix: str,
    *,
    tolerance: int = 2,
) -> tuple[str | None, int | None]:
    exact: list[tuple[str, int]] = []
    near: list[tuple[str, int]] = []
    for name, cpp_val in cpp.items():
        if not name.startswith(prefix):
            continue
        if isinstance(cpp_val, list):
            continue
        if cpp_val == value:
            exact.append((name, cpp_val))
        elif abs(cpp_val - value) <= tolerance:
            near.append((name, cpp_val))
    if exact:
        return exact[0]
    if near:
        return near[0]
    return None, None


def suggest_annotations(spec: dict[str, Any], *, tolerance: int = 2) -> list[AnnotationSuggestion]:
    suggestions: list[AnnotationSuggestion] = []
    cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
    policy = policy_for_spec(spec)
    canonical = spec.get("canonicalName") or spec.get("frameName") or ""
    prefix = CONSTANT_PREFIX.get(canonical, "k")

    if spec.get("sectionGap") is None:
        gap_key = policy.get("sectionGap")
        if gap_key and gap_key in cpp:
            suggestions.append(
                AnnotationSuggestion("root", "sectionGap", cpp[gap_key], f"from policy {gap_key}")
            )

    inset_map = policy.get("insets") or {}
    if inset_map and not spec.get("insets"):
        insets = {side: cpp[key] for side, key in inset_map.items() if key in cpp and isinstance(cpp[key], int)}
        if insets:
            suggestions.append(AnnotationSuggestion("root", "insets", insets, "from policy insets"))

    if not spec.get("codeOwners") and policy.get("cppOwners"):
        suggestions.append(
            AnnotationSuggestion(
                "root",
                "codeOwners",
                {"component": policy["cppOwners"][0].replace(".cpp", ""), "constants": "PlayModeLayout.h"},
                "from policy cppOwners",
            )
        )

    for node, path in _walk_nodes(spec, "root"):
        name = node.get("name") or ""
        if not node.get("codeMap") and name in CODE_MAP_HINTS:
            suggestions.append(
                AnnotationSuggestion(path, "codeMap", CODE_MAP_HINTS[name], f"name hint '{name}'")
            )

        dim = node.get("height") or node.get("width")
        if dim is None:
            continue
        dim = int(dim)

        if not node.get("constant"):
            match, cpp_val = _match_constant(dim, cpp, prefix, tolerance=tolerance)
            if match:
                if cpp_val == dim:
                    suggestions.append(
                        AnnotationSuggestion(path, "constant", match, f"exact match {match}={dim}")
                    )
                else:
                    suggestions.append(
                        AnnotationSuggestion(
                            path,
                            "constant",
                            match,
                            f"near match {match}={cpp_val} figma={dim}",
                        )
                    )
                    suggestions.append(
                        AnnotationSuggestion(
                            path,
                            "drift",
                            {"cpp": cpp_val, "reason": f"Figma {dim}px vs {match} {cpp_val}px"},
                            "documented drift",
                        )
                    )

    return suggestions


def apply_annotations(spec: dict[str, Any], suggestions: list[AnnotationSuggestion]) -> dict[str, Any]:
    out = copy.deepcopy(spec)

    def set_field(path: str, field: str, value: Any) -> None:
        if path == "root":
            out[field] = value
            return
        parts = path.split(".")[1:]  # drop 'root'
        node = out
        for part in parts:
            if part.startswith("children["):
                continue
            children = node.get("children") or []
            found = None
            for child in children:
                if isinstance(child, dict) and child.get("name") == part:
                    found = child
                    break
            if found is None:
                return
            node = found
        node[field] = value

    for s in suggestions:
        set_field(s.path, s.field, s.value)
    return out


def format_suggestions(suggestions: list[AnnotationSuggestion]) -> str:
    lines = ["# Annotation suggestions", ""]
    for s in suggestions:
        lines.append(f"- `{s.path}` → `{s.field}` = {s.value!r}  ({s.reason})")
    lines.append("")
    return "\n".join(lines)
