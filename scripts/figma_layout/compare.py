from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any

from .metadata_parser import metadata_to_layout_spec, parse_metadata_xml
from .spec import load_layout


@dataclass
class LayoutDiff:
    path: str
    committed: Any
    exported: Any

    def __str__(self) -> str:
        return f"{self.path}: committed={self.committed!r} exported={self.exported!r}"


def _bounds_map(spec: dict, *, top_level_only: bool = False) -> dict[str, dict[str, int]]:
    out: dict[str, dict[str, int]] = {}

    def walk(node: dict) -> None:
        nid = node.get("nodeId")
        if nid:
            out[str(nid)] = {
                k: int(node[k])
                for k in ("x", "y", "width", "height")
                if k in node and node[k] is not None
            }
        if top_level_only:
            return
        for child in node.get("children") or []:
            if isinstance(child, dict):
                walk(child)

    if top_level_only:
        for child in spec.get("children") or []:
            if isinstance(child, dict):
                walk(child)
    else:
        walk(spec)

    size = spec.get("size") or {}
    root_id = spec.get("nodeId")
    if root_id:
        out[str(root_id)] = {
            "width": int(size.get("width", 0)),
            "height": int(size.get("height", 0)),
        }
    return out


def compare_metadata_to_layout(
    metadata_xml: str,
    layout_spec: dict,
    *,
    figma_file_key: str | None = None,
    mode: str = "bounds",
    tolerance: int = 0,
    strict: bool = False,
) -> list[LayoutDiff]:
    root = parse_metadata_xml(metadata_xml)
    node_id = layout_spec.get("nodeId", "")
    exported = metadata_to_layout_spec(
        root,
        figma_file_key=figma_file_key or layout_spec.get("figmaFileKey", ""),
        node_id=node_id,
        frame_name=layout_spec.get("frameName"),
        export_source="compare",
    )

    top_level = mode in ("top-level", "top_level")
    committed_bounds = _bounds_map(layout_spec, top_level_only=top_level)
    exported_bounds = _bounds_map(exported, top_level_only=top_level)

    diffs: list[LayoutDiff] = []

    if strict:
        for nid in sorted(set(exported_bounds) - set(committed_bounds)):
            diffs.append(LayoutDiff(f"node {nid}", None, exported_bounds[nid]))

    all_ids = sorted(set(committed_bounds) & set(exported_bounds))
    for nid in all_ids:
        c = committed_bounds[nid]
        e = exported_bounds[nid]
        for key in ("x", "y", "width", "height"):
            cv = c.get(key)
            ev = e.get(key)
            if cv is None:
                continue
            if ev is None:
                if strict:
                    diffs.append(LayoutDiff(f"node {nid}.{key}", cv, ev))
                continue
            if abs(cv - ev) > tolerance:
                diffs.append(LayoutDiff(f"node {nid}.{key}", cv, ev))

    return diffs


def compare_metadata_file_to_layout(
    metadata_path: str,
    layout_path: str,
    **kwargs: Any,
) -> list[LayoutDiff]:
    from pathlib import Path

    metadata_xml = Path(metadata_path).read_text(encoding="utf-8")
    spec = load_layout(Path(layout_path))
    return compare_metadata_to_layout(metadata_xml, spec, **kwargs)
