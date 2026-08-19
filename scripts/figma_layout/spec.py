from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class ValidationIssue:
    level: str  # error | warn
    path: str
    message: str


def load_layout(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def save_layout(path: Path, spec: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(spec, indent=2) + "\n", encoding="utf-8")


def validate_spec_shape(spec: dict[str, Any], source: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []

    def req(key: str, typ: type) -> None:
        if key not in spec:
            issues.append(ValidationIssue("error", source, f"missing required field '{key}'"))
        elif not isinstance(spec[key], typ):
            issues.append(ValidationIssue("error", source, f"'{key}' must be {typ.__name__}"))

    req("figmaFileKey", str)
    req("nodeId", str)
    req("frameName", str)
    req("size", dict)

    if isinstance(spec.get("size"), dict):
        for dim in ("width", "height"):
            if dim not in spec["size"]:
                issues.append(ValidationIssue("error", source, f"size.{dim} required"))
            elif not isinstance(spec["size"][dim], int) or spec["size"][dim] < 1:
                issues.append(ValidationIssue("error", source, f"size.{dim} must be positive int"))

    if "children" not in spec and "variants" not in spec and "scaleLadder" not in spec:
        issues.append(
            ValidationIssue(
                "warn",
                source,
                "spec has no children, variants, or scaleLadder (knob-only specs OK)",
            )
        )

    node_id = spec.get("nodeId", "")
    if isinstance(node_id, str) and node_id and ":" not in node_id:
        issues.append(ValidationIssue("error", source, f"nodeId '{node_id}' must look like '4:1134'"))

    return issues


def is_play_mode_constant(name: str) -> bool:
    """True for identifiers like kCompactWidth (not Figma layer slugs)."""
    return bool(re.match(r"^k[A-Z]", name))


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


def iter_constant_bindings(spec: dict[str, Any]) -> list[tuple[str, int | None, str]]:
    """Yield (constantName, numericValue, contextPath) from a layout spec."""
    out: list[tuple[str, int | None, str]] = []

    size = spec.get("size") or {}
    if canonical := spec.get("canonicalName"):
        out.append((f"__frame_width__:{canonical}", size.get("width"), "size.width"))
        out.append((f"__frame_height__:{canonical}", size.get("height"), "size.height"))

    def walk(node: dict[str, Any], prefix: str) -> None:
        const = node.get("constant")
        if isinstance(const, str):
            val = _dimension_for_constant(node, const)
            if val is None and isinstance(node.get("size"), (int, float)):
                val = int(node["size"])
            out.append((const, val, prefix))
        for i, child in enumerate(node.get("children") or []):
            if isinstance(child, dict):
                name = child.get("name", f"child[{i}]")
                walk(child, f"{prefix}.{name}")

    for i, child in enumerate(spec.get("children") or []):
        if isinstance(child, dict):
            walk(child, child.get("name") or f"children[{i}]")

    for i, variant in enumerate(spec.get("variants") or []):
        if not isinstance(variant, dict):
            continue
        const = variant.get("codeMap", "")
        if "constant" in variant:
            pass
        size = variant.get("size")
        if size is not None and isinstance(variant.get("name"), str):
            for entry in spec.get("scaleLadder") or []:
                if entry.get("context") == variant["name"] and entry.get("constant"):
                    out.append((entry["constant"], int(size), f"variants.{variant['name']}.size"))

    for entry in spec.get("scaleLadder") or []:
        if not isinstance(entry, dict):
            continue
        const = entry.get("constant")
        px = entry.get("figmaPx")
        if const and px is not None:
            out.append((const, int(px), f"scaleLadder.{entry.get('context', const)}"))

    return out


def layout_filename(frame_name: str, node_id: str) -> str:
    safe_name = frame_name.strip().lower().replace(" ", "-")
    safe_id = node_id.replace(":", "-")
    return f"{safe_name}.{safe_id}.layout.json"
