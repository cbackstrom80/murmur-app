from __future__ import annotations

import json
import os
import time
import urllib.error
import urllib.request
import xml.sax.saxutils as saxutils
from typing import Any

from .compare import compare_metadata_to_layout
from .metadata_parser import metadata_to_layout_spec, parse_metadata_xml
from .spec import load_layout


def _figma_node_id_for_api(node_id: str) -> str:
    return node_id.replace(":", "%3A")


def fetch_figma_node_metadata_xml(file_key: str, node_id: str, *, token: str | None = None) -> str:
    """Fetch node tree from Figma REST API and emit get_metadata-compatible XML."""
    token = token or os.environ.get("FIGMA_ACCESS_TOKEN") or os.environ.get("FIGMA_TOKEN")
    if not token:
        raise RuntimeError("FIGMA_ACCESS_TOKEN (or FIGMA_TOKEN) required for fetch")

    url = f"https://api.figma.com/v1/files/{file_key}/nodes?ids={_figma_node_id_for_api(node_id)}"
    req = urllib.request.Request(url, headers={"X-Figma-Token": token})
    payload = None
    last_error: Exception | None = None
    for attempt in range(5):
        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                payload = json.loads(resp.read().decode("utf-8"))
            break
        except urllib.error.HTTPError as exc:
            body = exc.read().decode("utf-8", errors="replace")
            last_error = RuntimeError(f"Figma API HTTP {exc.code}: {body}")
            if exc.code == 429 and attempt < 4:
                time.sleep(2 ** attempt)
                continue
            raise last_error from exc
        except (urllib.error.URLError, TimeoutError) as exc:
            last_error = RuntimeError(f"Figma API network error: {exc}")
            if attempt < 4:
                time.sleep(2 ** attempt)
                continue
            raise last_error from exc
    if payload is None:
        raise last_error or RuntimeError("Figma API fetch failed after retries")

    nodes = payload.get("nodes") or {}
    entry = nodes.get(node_id) or nodes.get(node_id.replace("-", ":"))
    if not entry:
        # API keys may use encoded ids
        for key, val in nodes.items():
            if key.replace("-", ":") == node_id or key.replace("%3A", ":") == node_id:
                entry = val
                break
    if not entry or not entry.get("document"):
        raise RuntimeError(f"node {node_id} not found in Figma API response")

    doc = entry["document"]
    root_bbox = doc.get("absoluteBoundingBox") or {}
    return _document_to_metadata_xml(
        doc,
        parent_abs_x=float(root_bbox.get("x", 0)),
        parent_abs_y=float(root_bbox.get("y", 0)),
        is_root=True,
    )


def _round_px(value: float) -> int:
    return int(round(value))


def _document_to_metadata_xml(
    doc: dict[str, Any],
    *,
    parent_abs_x: float,
    parent_abs_y: float,
    is_root: bool,
    depth: int = 0,
) -> str:
    bbox = doc.get("absoluteBoundingBox") or {}
    abs_x = float(bbox.get("x", 0))
    abs_y = float(bbox.get("y", 0))
    w = float(bbox.get("width", 0))
    h = float(bbox.get("height", 0))

    if is_root:
        rel_x, rel_y = 0, 0
    else:
        rel_x = abs_x - parent_abs_x
        rel_y = abs_y - parent_abs_y

    node_id = doc.get("id", "")
    name = saxutils.escape(str(doc.get("name", "frame")), {'"': "&quot;"})
    indent = "  " * depth
    lines = [
        f'{indent}<frame id="{node_id}" name="{name}" '
        f'x="{_round_px(rel_x)}" y="{_round_px(rel_y)}" '
        f'width="{_round_px(w)}" height="{_round_px(h)}">',
    ]
    for child in doc.get("children") or []:
        if isinstance(child, dict):
            lines.append(
                _document_to_metadata_xml(
                    child,
                    parent_abs_x=abs_x,
                    parent_abs_y=abs_y,
                    is_root=False,
                    depth=depth + 1,
                )
            )
    lines.append(f"{indent}</frame>")
    return "\n".join(lines)


def fetch_and_compare(
    layout_path: str,
    *,
    file_key: str | None = None,
    node_id: str | None = None,
    mode: str = "top-level",
    tolerance: int = 0,
    strict: bool = False,
) -> tuple[str, list]:
    from pathlib import Path

    from .paths import LAYOUTS_DIR

    path = Path(layout_path)
    if not path.is_file():
        path = LAYOUTS_DIR / layout_path
    spec = load_layout(path)
    fk = file_key or spec.get("figmaFileKey", "")
    nid = node_id or spec.get("nodeId", "")
    xml = fetch_figma_node_metadata_xml(fk, nid)
    diffs = compare_metadata_to_layout(xml, spec, mode=mode, tolerance=tolerance, strict=strict)
    return xml, diffs
