from __future__ import annotations

from typing import Any

from .spec import ValidationIssue, is_play_mode_constant


def _dimension_for_constant(node: dict[str, Any], const: str) -> int | None:
    if const.endswith("Width") or "Width" in const:
        if node.get("width") is not None:
            return int(node["width"])
    if const.endswith("Height") or "Height" in const:
        if node.get("height") is not None:
            return int(node["height"])
    if node.get("height") is not None:
        return int(node["height"])
    if node.get("width") is not None:
        return int(node["width"])
    return None


def figma_dimension(node: dict[str, Any], const: str | None = None) -> int | None:
    """Primary bound used for constant binding."""
    if const:
        return _dimension_for_constant(node, const)
    return _dimension_for_constant(node, "")


def drift_cpp_value(node: dict[str, Any]) -> int | None:
    drift = node.get("drift")
    if isinstance(drift, dict) and drift.get("cpp") is not None:
        return int(drift["cpp"])
    return None


def validate_drift_bindings(
    spec: dict[str, Any],
    cpp: dict[str, int | list[int]],
    source: str,
) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []

    def walk(node: dict[str, Any], path: str) -> None:
        const = node.get("constant")
        figma_val = figma_dimension(node, const if isinstance(const, str) else None)
        if isinstance(const, str) and is_play_mode_constant(const):
            cpp_val = cpp.get(const)
            if cpp_val is None or isinstance(cpp_val, list):
                return
            drift = node.get("drift")
            if isinstance(drift, dict):
                doc_cpp = drift.get("cpp")
                if doc_cpp is not None and int(doc_cpp) != int(cpp_val):
                    issues.append(
                        ValidationIssue(
                            "error",
                            source,
                            f"{path}: drift.cpp={doc_cpp} != PlayModeLayout {const}={cpp_val}",
                        )
                    )
                if figma_val is not None and int(cpp_val) != figma_val:
                    if doc_cpp is None or int(doc_cpp) != int(cpp_val):
                        issues.append(
                            ValidationIssue(
                                "error",
                                source,
                                f"{path}: {const} figma={figma_val} cpp={cpp_val} — add drift{{cpp, reason}}",
                            )
                        )
            elif figma_val is not None and int(cpp_val) != figma_val:
                issues.append(
                    ValidationIssue(
                        "error",
                        source,
                        f"{path}: {const} figma={figma_val} != PlayModeLayout={cpp_val} (undocumented drift)",
                    )
                )
        for i, child in enumerate(node.get("children") or []):
            if isinstance(child, dict):
                name = child.get("name") or f"child[{i}]"
                walk(child, f"{path}.{name}")

    for i, child in enumerate(spec.get("children") or []):
        if isinstance(child, dict):
            walk(child, child.get("name") or f"children[{i}]")

    # Legacy free-text notes about drift → warn to migrate
    def walk_notes(node: dict[str, Any], path: str) -> None:
        note = node.get("note")
        if isinstance(note, str) and ("vs k" in note.lower() or "vs " in note.lower()):
            if "drift" not in node:
                issues.append(
                    ValidationIssue(
                        "warn",
                        source,
                        f"{path}: migrate note to structured drift object",
                    )
                )
        for child in node.get("children") or []:
            if isinstance(child, dict):
                walk_notes(child, f"{path}.{child.get('name', '?')}")

    for child in spec.get("children") or []:
        if isinstance(child, dict):
            walk_notes(child, child.get("name") or "root")

    return issues
