from __future__ import annotations

import json
from pathlib import Path

from .geometry import validate_column_layouts, validate_parent_links, validate_stack_geometry
from .manifest import load_manifest, validate_manifest
from .paths import LAYOUTS_DIR
from .policies import policy_for_spec, registry_pending
from .reverse_audit import validate_reverse_audit
from .schema_check import validate_against_schema
from .spec import ValidationIssue, load_layout, validate_spec_shape
from .token_ladder import validate_hero_ratios, validate_scale_ladder
from .validate import exit_code, format_issues, validate_all


def check_all(*, strict: bool = False) -> list[ValidationIssue]:
    issues = validate_all(strict=strict)
    issues.extend(validate_manifest(strict=strict))

    from .cpp_constants import parse_play_mode_layout
    from .paths import PLAY_MODE_LAYOUT_H

    cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H) if PLAY_MODE_LAYOUT_H.is_file() else {}

    for layout_path in sorted(LAYOUTS_DIR.glob("*.layout.json")):
        source = str(layout_path.relative_to(LAYOUTS_DIR.parents[2]))
        try:
            spec = load_layout(layout_path)
        except json.JSONDecodeError:
            continue
        issues.extend(validate_against_schema(spec, source))
        issues.extend(validate_spec_shape(spec, source))
        issues.extend(validate_stack_geometry(spec, source))
        issues.extend(validate_column_layouts(spec, cpp, source))
        issues.extend(validate_parent_links(spec, source))
        policy = policy_for_spec(spec)
        if policy.get("validateScaleLadder"):
            issues.extend(validate_scale_ladder(spec, source))
        if policy.get("validateHeroRatios"):
            issues.extend(validate_hero_ratios(spec, source))
        issues.extend(validate_reverse_audit(spec, source))

    return issues


def print_summary(issues: list[ValidationIssue]) -> None:
    manifest = load_manifest()
    layout_count = len(list(LAYOUTS_DIR.glob("*.layout.json")))
    registry_count = len(manifest.get("registry") or [])
    pending = registry_pending()
    required_missing = sum(1 for r in pending if r.get("required") or r.get("tier") == "required")
    errors = sum(1 for i in issues if i.level == "error")
    warns = sum(1 for i in issues if i.level == "warn")
    print(
        f"figma_layout check: {layout_count} specs, {registry_count} registry frames, "
        f"{errors} errors, {warns} warnings, {len(pending)} pending ({required_missing} required)"
    )


__all__ = ["check_all", "exit_code", "format_issues", "print_summary"]
