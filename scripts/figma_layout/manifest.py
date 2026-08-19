from __future__ import annotations

import json
from pathlib import Path

from .code_connect import scan_figma_ts
from .paths import FIGMA_CONNECT_DIR, LAYOUTS_DIR, MANIFEST_PATH
from .spec import ValidationIssue


def load_manifest() -> dict:
    if not MANIFEST_PATH.is_file():
        return {"layouts": [], "registry": []}
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def validate_manifest(*, strict: bool = False) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    manifest = load_manifest()
    ts_refs = scan_figma_ts(FIGMA_CONNECT_DIR)

    for entry in manifest.get("layouts") or []:
        if not isinstance(entry, dict):
            continue
        file_name = entry.get("file")
        if not file_name:
            issues.append(ValidationIssue("error", str(MANIFEST_PATH), "layout entry missing file"))
            continue

        layout_path = LAYOUTS_DIR / file_name
        if not layout_path.is_file():
            issues.append(ValidationIssue("error", str(MANIFEST_PATH), f"missing layout file: {file_name}"))
            continue

        layout_text = layout_path.read_text(encoding="utf-8")
        for ts_name in entry.get("codeConnect") or []:
            ts_path = FIGMA_CONNECT_DIR / ts_name
            if not ts_path.is_file():
                issues.append(
                    ValidationIssue("error", str(MANIFEST_PATH), f"codeConnect file missing: {ts_name}")
                )
                continue
            ts_text = ts_path.read_text(encoding="utf-8")
            if file_name not in ts_text and file_name.replace(".layout.json", "") not in ts_text:
                issues.append(
                    ValidationIssue(
                        "error",
                        ts_name,
                        f"does not reference layout {file_name}",
                    )
                )

        node_id = entry.get("nodeId")
        spec = json.loads(layout_text)
        if node_id and spec.get("nodeId") != node_id:
            issues.append(
                ValidationIssue(
                    "error",
                    file_name,
                    f"manifest nodeId {node_id} != spec nodeId {spec.get('nodeId')}",
                )
            )

        if file_name not in ts_refs:
            issues.append(
                ValidationIssue(
                    "warn",
                    file_name,
                    "not found in *.figma.ts scan (manifest codeConnect may use comment-only refs)",
                )
            )

    exported_nodes = {e.get("nodeId") for e in manifest.get("layouts") or [] if isinstance(e, dict)}
    for reg in manifest.get("registry") or []:
        if not isinstance(reg, dict):
            continue
        node_id = reg.get("nodeId")
        tier = reg.get("tier") or reg.get("status")
        required = reg.get("required", False) or tier == "required"
        if required and node_id not in exported_nodes:
            level = "error" if strict else "warn"
            issues.append(
                ValidationIssue(
                    level,
                    str(MANIFEST_PATH),
                    f"registry frame {reg.get('canonicalName')} ({node_id}) has no committed layout.json",
                )
            )
        if tier == "pending" and strict:
            issues.append(
                ValidationIssue(
                    "warn",
                    str(MANIFEST_PATH),
                    f"registry frame {reg.get('canonicalName')} ({node_id}) tier=pending",
                )
            )

    return issues
