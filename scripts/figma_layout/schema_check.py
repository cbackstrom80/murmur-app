from __future__ import annotations

import json
from typing import Any

from .paths import SCHEMA_PATH
from .spec import ValidationIssue


def _check_type(value: Any, expected: str) -> bool:
    if expected == "string":
        return isinstance(value, str)
    if expected == "integer":
        return isinstance(value, int) and not isinstance(value, bool)
    if expected == "object":
        return isinstance(value, dict)
    if expected == "array":
        return isinstance(value, list)
    return True


def validate_against_schema(spec: dict[str, Any], source: str) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []

    if not SCHEMA_PATH.is_file():
        return issues

    schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    required = schema.get("required") or []
    for key in required:
        if key not in spec:
            issues.append(ValidationIssue("error", source, f"schema: missing required '{key}'"))

    props = schema.get("properties") or {}
    for key, val in spec.items():
        if key.startswith("$"):
            continue
        prop = props.get(key)
        if prop is None:
            if schema.get("additionalProperties") is False:
                issues.append(ValidationIssue("error", source, f"schema: unknown property '{key}'"))
            continue
        expected = prop.get("type")
        if expected and not _check_type(val, expected):
            issues.append(ValidationIssue("error", source, f"schema: '{key}' must be {expected}"))

    size = spec.get("size")
    if isinstance(size, dict):
        for dim in ("width", "height"):
            v = size.get(dim)
            if v is not None and (not isinstance(v, int) or v < 1):
                issues.append(ValidationIssue("error", source, f"schema: size.{dim} must be positive int"))

    _validate_nodes(spec.get("children") or [], source, issues)
    return issues


def _validate_nodes(nodes: list, source: str, issues: list[ValidationIssue]) -> None:
    for i, node in enumerate(nodes):
        if not isinstance(node, dict):
            issues.append(ValidationIssue("error", source, f"children[{i}] must be object"))
            continue
        drift = node.get("drift")
        if drift is not None:
            if not isinstance(drift, dict):
                issues.append(ValidationIssue("error", source, f"{node.get('name')}: drift must be object"))
            elif "cpp" not in drift:
                issues.append(ValidationIssue("error", source, f"{node.get('name')}: drift.cpp required"))
        if node.get("children"):
            _validate_nodes(node["children"], source, issues)
