from __future__ import annotations

import re
from pathlib import Path

from .check import check_all
from .manifest import load_manifest
from .paths import LAYOUTS_DIR
from .policies import load_policy_index, registry_pending
from .spec import load_layout
from .validate import exit_code, format_issues


def generate_report(*, markdown: bool = True) -> str:
    manifest = load_manifest()
    policies = load_policy_index()
    issues = check_all(strict=False)
    errors = sum(1 for i in issues if i.level == "error")
    warns = sum(1 for i in issues if i.level == "warn")
    pending = registry_pending()

    lines = [
        "# Figma Layout Toolchain Report",
        "",
        f"- **Specs:** {len(list(LAYOUTS_DIR.glob('*.layout.json')))}",
        f"- **Registry:** {len(manifest.get('registry') or [])}",
        f"- **Errors:** {errors}",
        f"- **Warnings:** {warns}",
        f"- **Pending:** {len(pending)}",
        "",
        "## Exported frames",
        "",
        "| Canonical | Node | Tier | Layout JSON |",
        "|-----------|------|------|-------------|",
    ]

    layout_by_node = {e.get("nodeId"): e for e in manifest.get("layouts") or [] if isinstance(e, dict)}
    for reg in manifest.get("registry") or []:
        if not isinstance(reg, dict):
            continue
        canonical = reg.get("canonicalName", "")
        node = reg.get("nodeId", "")
        tier = reg.get("tier") or policies.get(canonical, {}).get("tier") or reg.get("status", "")
        layout = layout_by_node.get(node, {})
        file_name = layout.get("file", "—")
        lines.append(f"| {canonical} | `{node}` | {tier} | `{file_name}` |")

    if pending:
        lines.extend(["", "## Pending registry", ""])
        for reg in pending:
            lines.append(f"- `{reg.get('canonicalName')}` ({reg.get('nodeId')})")

    if issues:
        lines.extend(["", "## Issues", "", "```"])
        lines.append(format_issues(issues))
        lines.append("```")

    lines.extend(["", "## Audit delta rows (auto)", ""])
    for path in sorted(LAYOUTS_DIR.glob("*.layout.json")):
        spec = load_layout(path)
        canonical = spec.get("canonicalName") or spec.get("frameName")
        node = spec.get("nodeId")
        size = spec.get("size") or {}
        lines.append(
            f"- **{canonical}** (`{node}`): {size.get('width')}×{size.get('height')} — "
            f"spec `{path.name}`"
        )

    lines.append("")
    return "\n".join(lines)


def write_report(output: Path) -> None:
    output.write_text(generate_report(), encoding="utf-8")
