from __future__ import annotations

import json
from pathlib import Path

from .cpp_constants import parse_play_mode_layout
from .drift import validate_drift_bindings
from .paths import FIGMA_CONNECT_DIR, LAYOUTS_DIR, MANIFEST_PATH, PLAY_MODE_LAYOUT_H
from .policies import policy_for_spec, policy_tier, registry_pending
from .spec import ValidationIssue, is_play_mode_constant, iter_constant_bindings, load_layout, validate_spec_shape


def validate_all(*, strict: bool = False) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []

    if not PLAY_MODE_LAYOUT_H.is_file():
        issues.append(ValidationIssue("error", str(PLAY_MODE_LAYOUT_H), "PlayModeLayout.h missing"))
        return issues

    cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
    layout_files = sorted(LAYOUTS_DIR.glob("*.layout.json"))
    if not layout_files:
        issues.append(ValidationIssue("error", str(LAYOUTS_DIR), "no *.layout.json files found"))
        return issues

    manifest_entries: dict[str, dict] = {}
    if MANIFEST_PATH.is_file():
        try:
            manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
            for entry in manifest.get("layouts") or []:
                if isinstance(entry, dict) and entry.get("file"):
                    manifest_entries[entry["file"]] = entry
        except json.JSONDecodeError as exc:
            issues.append(ValidationIssue("error", str(MANIFEST_PATH), f"invalid manifest JSON: {exc}"))

    from .code_connect import scan_figma_ts

    ts_refs = scan_figma_ts(FIGMA_CONNECT_DIR)

    for layout_path in layout_files:
        rel = layout_path.name
        source = str(layout_path.relative_to(LAYOUTS_DIR.parents[2]))
        try:
            spec = load_layout(layout_path)
        except json.JSONDecodeError as exc:
            issues.append(ValidationIssue("error", source, f"invalid JSON: {exc}"))
            continue

        issues.extend(validate_spec_shape(spec, source))
        issues.extend(_validate_policy_fields(spec, cpp, source))
        issues.extend(validate_drift_bindings(spec, cpp, source))

        for const, val, ctx in iter_constant_bindings(spec):
            if const.startswith("__frame_") or val is None:
                continue
            if not is_play_mode_constant(const):
                continue
            cpp_val = cpp.get(const)
            if cpp_val is None:
                if strict:
                    issues.append(
                        ValidationIssue("warn", source, f"constant '{const}' ({ctx}) not found in PlayModeLayout.h")
                    )
                continue
            if isinstance(cpp_val, list):
                if val not in cpp_val:
                    issues.append(
                        ValidationIssue(
                            "error",
                            source,
                            f"{const} ladder {cpp_val} does not include Figma value {val} ({ctx})",
                        )
                    )
            elif cpp_val != val:
                # Drift module handles documented mismatches; skip duplicate errors when drift present.
                if not _has_documented_drift(spec, const, val, cpp_val):
                    issues.append(
                        ValidationIssue(
                            "error",
                            source,
                            f"{const}={cpp_val} in PlayModeLayout.h but Figma has {val} ({ctx})",
                        )
                    )

        if rel not in ts_refs and rel not in manifest_entries:
            issues.append(
                ValidationIssue(
                    "warn",
                    source,
                    "layout not referenced by any *.figma.ts or manifest.json",
                )
            )

    for file_name in manifest_entries:
        path = LAYOUTS_DIR / file_name
        if not path.is_file():
            issues.append(ValidationIssue("error", str(MANIFEST_PATH), f"manifest entry missing file: {file_name}"))

    if strict:
        for reg in registry_pending():
            level = "error" if reg.get("required") or reg.get("tier") == "required" else "warn"
            issues.append(
                ValidationIssue(
                    level,
                    str(MANIFEST_PATH),
                    f"registry frame {reg.get('canonicalName')} ({reg.get('nodeId')}) pending export",
                )
            )

    return issues


def _has_documented_drift(spec: dict, const: str, figma_val: int, cpp_val: int) -> bool:
    from .drift import drift_cpp_value, figma_dimension

    def walk(node: dict) -> bool:
        if node.get("constant") == const:
            c = node.get("constant")
            if drift_cpp_value(node) == cpp_val and figma_dimension(node, str(c)) == figma_val:
                return True
        for child in node.get("children") or []:
            if isinstance(child, dict) and walk(child):
                return True
        return False

    for child in spec.get("children") or []:
        if isinstance(child, dict) and walk(child):
            return True
    return False


def _validate_policy_fields(spec: dict, cpp: dict[str, int | list[int]], source: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    policy = policy_for_spec(spec)
    canonical = spec.get("canonicalName") or spec.get("frameName") or ""
    size = spec.get("size") or {}

    frame_size = policy.get("frameSize")
    if frame_size and isinstance(frame_size, list) and len(frame_size) >= 2:
        w_key, h_key = frame_size[0], frame_size[1]
        if policy.get("frameWidthOnly"):
            if w_key in cpp and size.get("width") is not None and size["width"] != cpp[w_key]:
                issues.append(
                    ValidationIssue(
                        "error",
                        source,
                        f"{w_key}={cpp[w_key]} but Figma size.width={size['width']}",
                    )
                )
        else:
            for key, dim in ((w_key, "width"), (h_key, "height")):
                if key in cpp and isinstance(cpp[key], int):
                    actual = size.get(dim)
                    if actual is not None and actual != cpp[key]:
                        issues.append(
                            ValidationIssue(
                                "error",
                                source,
                                f"{key}={cpp[key]} in PlayModeLayout.h but Figma size.{dim}={actual}",
                            )
                        )
    elif policy.get("frameWidthOnly") and "kDefaultWidth" in cpp:
        if size.get("width") is not None and size["width"] != cpp["kDefaultWidth"]:
            issues.append(
                ValidationIssue(
                    "error",
                    source,
                    f"kDefaultWidth={cpp['kDefaultWidth']} but Figma size.width={size['width']}",
                )
            )

    gap_key = policy.get("sectionGap")
    if gap_key and spec.get("sectionGap") is not None and gap_key in cpp:
        if spec["sectionGap"] != cpp[gap_key]:
            issues.append(
                ValidationIssue(
                    "error",
                    source,
                    f"{gap_key}={cpp[gap_key]} but sectionGap={spec['sectionGap']}",
                )
            )

    inset_map = policy.get("insets") or {}
    insets = spec.get("insets") or {}
    for side, key in inset_map.items():
        if side in insets and key in cpp and insets[side] != cpp[key]:
            issues.append(
                ValidationIssue(
                    "error",
                    source,
                    f"{key}={cpp[key]} but insets.{side}={insets[side]}",
                )
            )

    if not policy and canonical:
        issues.append(
            ValidationIssue("warn", source, f"no layoutPolicy for canonical '{canonical}'"),
        )

    return issues


def format_issues(issues: list[ValidationIssue]) -> str:
    lines: list[str] = []
    for issue in issues:
        prefix = "ERROR" if issue.level == "error" else "WARN"
        lines.append(f"{prefix} [{issue.path}] {issue.message}")
    return "\n".join(lines)


def exit_code(issues: list[ValidationIssue]) -> int:
    return 1 if any(i.level == "error" for i in issues) else 0
