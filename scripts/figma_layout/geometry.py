from __future__ import annotations

from typing import Any

from .policies import policy_for_spec
from .spec import ValidationIssue, load_layout
from .paths import LAYOUTS_DIR


def validate_stack_geometry(spec: dict, source: str) -> list[ValidationIssue]:
    """Verify vertical budget: insets + top-level sections + gaps = frame height."""
    issues: list[ValidationIssue] = []
    policy = policy_for_spec(spec)
    geometry_modes = policy.get("geometry") or []
    if "vertical-stack" not in geometry_modes:
        return issues

    canonical = spec.get("canonicalName") or spec.get("frameName") or ""
    size = spec.get("size") or {}
    frame_h = size.get("height")
    frame_w = size.get("width")
    if frame_h is None or frame_w is None:
        return issues

    insets = spec.get("insets") or {}
    top = int(insets.get("top", 0))
    bottom = int(insets.get("bottom", 0))
    left = int(insets.get("left", insets.get("top", 0)))
    right = int(insets.get("right", insets.get("top", 0)))
    gap = int(spec.get("sectionGap", 0))

    children = [c for c in (spec.get("children") or []) if isinstance(c, dict)]
    if not children:
        issues.append(ValidationIssue("error", source, f"{canonical}: missing children for geometry check"))
        return issues

    k_width_slack = 8
    for child in children:
        w = child.get("width")
        x = child.get("x")
        if w is not None and x is not None:
            overflow = x + w + right - frame_w
            if overflow > k_width_slack:
                issues.append(
                    ValidationIssue(
                        "error",
                        source,
                        f"{child.get('name')}: x={x}+width={w}+right={right} exceeds frame width {frame_w} by {overflow}px",
                    )
                )

    sections = sorted(
        (c for c in children if c.get("height") is not None and c.get("y") is not None),
        key=lambda c: int(c["y"]),
    )

    if sections and int(sections[0]["y"]) != top:
        issues.append(
            ValidationIssue(
                "warn",
                source,
                f"first section y={sections[0]['y']} expected top inset {top}",
            )
        )

    prev_bottom: int | None = None
    for child in sections:
        y = int(child["y"])
        h = int(child["height"])
        if prev_bottom is not None:
            expected_y = prev_bottom + gap
            if y != expected_y:
                issues.append(
                    ValidationIssue(
                        "error",
                        source,
                        f"{child.get('name')}: y={y} expected {expected_y} (prev section + gap {gap})",
                    )
                )
        prev_bottom = y + h

    if prev_bottom is not None:
        budget = prev_bottom + bottom
        if budget != frame_h:
            issues.append(
                ValidationIssue(
                    "error",
                    source,
                    f"vertical budget {budget} (last y+h={prev_bottom} + bottom={bottom}) != frame height {frame_h}",
                )
            )

    return issues


def _find_child_by_name(nodes: list[dict], name: str) -> dict | None:
    for node in nodes:
        if node.get("name") == name:
            return node
        if children := node.get("children"):
            found = _find_child_by_name([c for c in children if isinstance(c, dict)], name)
            if found:
                return found
    return None


def validate_column_layouts(
    spec: dict,
    cpp: dict[str, int | list[int]],
    source: str,
) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    policy = policy_for_spec(spec)
    column_layouts = policy.get("columnLayouts") or spec.get("columnLayouts") or []
    if not column_layouts:
        return issues

    children = [c for c in (spec.get("children") or []) if isinstance(c, dict)]

    for layout in column_layouts:
        if not isinstance(layout, dict):
            continue
        parent_name = layout.get("parentName")
        parent = _find_child_by_name(children, str(parent_name))
        if parent is None:
            issues.append(ValidationIssue("error", source, f"column layout parent '{parent_name}' not found"))
            continue

        parent_w = parent.get("width")
        if parent_w is None:
            continue

        gap = layout.get("gap")
        gap_key = layout.get("gapConstant")
        if gap is None and gap_key and gap_key in cpp and isinstance(cpp[gap_key], int):
            gap = cpp[gap_key]
        gap = int(gap or 0)

        direction = layout.get("direction", "horizontal")
        if direction != "horizontal":
            continue

        child_names = layout.get("children") or []
        left = layout.get("leftChildren") or []
        right = layout.get("rightChildren") or []
        nested = [c for c in (parent.get("children") or []) if isinstance(c, dict)]

        if child_names:
            widths = []
            for name in child_names:
                node = _find_child_by_name(nested, name)
                if node and node.get("width") is not None:
                    widths.append(int(node["width"]))
            if widths:
                total = sum(widths) + gap * max(0, len(widths) - 1)
                slack = int(layout.get("gapTolerance") or policy.get("columnGapTolerance") or 1)
                if abs(total - int(parent_w)) > slack:
                    issues.append(
                        ValidationIssue(
                            "error",
                            source,
                            f"{parent_name} column widths {widths}+gaps={total} != parent width {parent_w}",
                        )
                    )
        elif left and right:
            left_nodes = [_find_child_by_name(nested, name) for name in left]
            right_nodes = [_find_child_by_name(nested, name) for name in right]
            left_w = max(
                (int(n["width"]) for n in left_nodes if n and n.get("width") is not None),
                default=0,
            )
            right_w = max(
                (int(n["width"]) for n in right_nodes if n and n.get("width") is not None),
                default=0,
            )
            total = left_w + gap + right_w
            if total != int(parent_w):
                issues.append(
                    ValidationIssue(
                        "error",
                        source,
                        f"{parent_name} columns left={left_w}+gap={gap}+right={right_w}={total} != {parent_w}",
                    )
                )

    return issues


def validate_parent_links(spec: dict, source: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    policy = policy_for_spec(spec)
    link = policy.get("parentLink") or {}
    parent_frame = spec.get("parentFrame") or link.get("parentFrame")
    embed_id = spec.get("nodeId") or link.get("embedNodeId")
    if not parent_frame or not embed_id:
        return issues

    parent_path = None
    for path in sorted(LAYOUTS_DIR.glob("*.layout.json")):
        parent_spec = load_layout(path)
        if parent_spec.get("nodeId") == parent_frame:
            parent_path = path
            break

    if parent_path is None:
        issues.append(
            ValidationIssue("warn", source, f"parent frame {parent_frame} layout spec not found")
        )
        return issues

    parent_spec = load_layout(parent_path)

    def find_embed(nodes: list[dict]) -> dict | None:
        for node in nodes:
            if node.get("nodeId") == embed_id:
                return node
            if kids := node.get("children"):
                if found := find_embed([c for c in kids if isinstance(c, dict)]):
                    return found
        return None

    embed = find_embed([c for c in (parent_spec.get("children") or []) if isinstance(c, dict)])
    if embed is None:
        issues.append(
            ValidationIssue("error", source, f"embed node {embed_id} not found in parent {parent_frame}")
        )
        return issues

    sub_size = spec.get("size") or {}
    for dim in ("width", "height"):
        sv = sub_size.get(dim)
        ev = embed.get(dim)
        if sv is not None and ev is not None and int(sv) != int(ev):
            issues.append(
                ValidationIssue(
                    "error",
                    source,
                    f"sub-panel size.{dim}={sv} != parent embed {embed_id}.{dim}={ev}",
                )
            )

    pad_key = policy.get("paddingConstant")
    padding = spec.get("padding")
    if pad_key and padding is not None:
        from .cpp_constants import parse_play_mode_layout
        from .paths import PLAY_MODE_LAYOUT_H

        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        if pad_key in cpp and int(cpp[pad_key]) != int(padding):
            issues.append(
                ValidationIssue(
                    "error",
                    source,
                    f"padding={padding} != {pad_key}={cpp[pad_key]}",
                )
            )

    return issues


# Back-compat alias used by tests
validate_compact_geometry = validate_stack_geometry
