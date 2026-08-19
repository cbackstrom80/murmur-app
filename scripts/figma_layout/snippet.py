from __future__ import annotations

from pathlib import Path

from .policies import policy_for_spec
from .spec import _dimension_for_constant, load_layout


def generate_snippet(layout_path: Path) -> str:
    spec = load_layout(layout_path)
    policy = policy_for_spec(spec)
    node_id = spec.get("nodeId", "?")
    frame = spec.get("frameName") or layout_path.stem
    canonical = spec.get("canonicalName") or frame
    size = spec.get("size") or {}
    w, h = size.get("width", "?"), size.get("height", "?")

    lines = [
        f"// Generated from {layout_path.name} — do not edit by hand.",
        f"// Figma node {node_id} @ {w}×{h} ({canonical})",
        "// Paste into plugin/src/ui/PlayModeLayout.h if values drift.",
        "",
    ]

    gap_key = policy.get("sectionGap")
    if gap_key and spec.get("sectionGap") is not None:
        lines.append(f"inline constexpr int {gap_key} = {spec['sectionGap']};  // sectionGap")

    inset_map = policy.get("insets") or {}
    insets = spec.get("insets") or {}
    for side, key in inset_map.items():
        if side in insets:
            lines.append(f"inline constexpr int {key} = {insets[side]};  // insets.{side}")

    frame_size = policy.get("frameSize")
    if frame_size and isinstance(frame_size, list):
        w_key = frame_size[0] if len(frame_size) > 0 else None
        h_key = frame_size[1] if len(frame_size) > 1 else None
        if w_key and w != "?":
            lines.append(f"inline constexpr int {w_key} = {w};  // frame width")
        if h_key and h != "?" and not policy.get("frameWidthOnly"):
            lines.append(f"inline constexpr int {h_key} = {h};  // frame height")
    elif policy.get("frameWidthOnly") and w != "?":
        lines.append(f"inline constexpr int kDefaultWidth = {w};  // frame width (height artboard-only)")

    for entry in spec.get("scaleLadder") or []:
        const = entry.get("constant")
        px = entry.get("figmaPx")
        ctx = entry.get("context", "")
        if const and px is not None:
            lines.append(f"// {ctx}: figma {px}px → {const}")

    def walk(node: dict, depth: int = 0) -> None:
        const = node.get("constant")
        drift = node.get("drift")
        if const:
            val = _dimension_for_constant(node, const)
            name = node.get("name", "")
            nid = node.get("nodeId", "")
            comment = f"  // {name} ({nid})"
            if isinstance(drift, dict) and drift.get("cpp") is not None:
                comment += f" drift cpp={drift['cpp']} figma={val}"
            if val is not None:
                lines.append(f"inline constexpr int {const} = {val};{comment}")
        for child in node.get("children") or []:
            if isinstance(child, dict):
                walk(child, depth + 1)

    for child in spec.get("children") or []:
        if isinstance(child, dict):
            walk(child)

    lines.append("")
    return "\n".join(lines)
