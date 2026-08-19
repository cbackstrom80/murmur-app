from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import Any


@dataclass
class MetadataNode:
    node_id: str
    name: str
    x: float
    y: float
    width: float
    height: float
    children: list["MetadataNode"] = field(default_factory=list)


_ATTR_FLOAT = ("x", "y", "width", "height")


def _parse_attrs(elem: ET.Element) -> dict[str, Any]:
    out: dict[str, Any] = {}
    for key in ("id", "name", *_ATTR_FLOAT):
        if key in elem.attrib:
            val = elem.attrib[key]
            if key in _ATTR_FLOAT:
                out[key] = float(val)
            else:
                out[key] = val
    return out


def _elem_to_node(elem: ET.Element) -> MetadataNode | None:
    attrs = _parse_attrs(elem)
    node_id = attrs.get("id") or elem.attrib.get("node-id") or elem.attrib.get("nodeId")
    if not node_id:
        return None
    name = str(attrs.get("name") or elem.tag)
    return MetadataNode(
        node_id=str(node_id),
        name=name,
        x=float(attrs.get("x", 0)),
        y=float(attrs.get("y", 0)),
        width=float(attrs.get("width", 0)),
        height=float(attrs.get("height", 0)),
        children=[c for child in elem if (c := _elem_to_node(child)) is not None],
    )


def parse_metadata_xml(text: str) -> MetadataNode:
    text = text.strip()
    if not text:
        raise ValueError("empty metadata input")

    # MCP dumps may prefix prose before the root element.
    if not text.startswith("<"):
        start = text.find("<frame")
        if start == -1:
            start = text.find("<")
        if start == -1:
            raise ValueError("metadata must be XML from Figma get_metadata")
        text = text[start:]

    # MCP responses may append prose after the closing tag — keep root element only.
    end = text.rfind("</frame>")
    if end != -1:
        text = text[: end + len("</frame>")]

    root = ET.fromstring(text)
    node = _elem_to_node(root)
    if node is None:
        raise ValueError("could not parse root metadata node (missing id)")
    return node


def find_node_by_id(root: MetadataNode, node_id: str) -> MetadataNode | None:
    if root.node_id == node_id:
        return root
    for child in root.children:
        if found := find_node_by_id(child, node_id):
            return found
    return None


def _slug(name: str) -> str:
    slug = re.sub(r"[^a-zA-Z0-9]+", "-", name.strip().lower()).strip("-")
    return slug or "frame"


def metadata_to_layout_spec(
    root: MetadataNode,
    *,
    figma_file_key: str,
    node_id: str,
    frame_name: str | None = None,
    export_source: str = "figma_layout export",
) -> dict[str, Any]:
    frame = find_node_by_id(root, node_id) or root
    name = frame_name or frame.name

    def child_dict(n: MetadataNode) -> dict[str, Any]:
        d: dict[str, Any] = {
            "nodeId": n.node_id,
            "name": n.name,
            "x": int(round(n.x)),
            "y": int(round(n.y)),
            "width": int(round(n.width)),
            "height": int(round(n.height)),
        }
        if n.children:
            d["children"] = [child_dict(c) for c in n.children]
        return d

    return {
        "$schema": "./figma-layout-spec.schema.json",
        "figmaFileKey": figma_file_key,
        "nodeId": node_id,
        "frameName": _slug(name),
        "canonicalName": _slug(name),
        "exportSource": export_source,
        "size": {
            "width": int(round(frame.width)),
            "height": int(round(frame.height)),
        },
        "children": [child_dict(c) for c in frame.children],
    }
