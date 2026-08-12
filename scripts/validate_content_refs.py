#!/usr/bin/env python3
"""Validate that every wavetableId in content/presets resolves after install layout."""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PRESETS_DIR = REPO_ROOT / "content" / "presets"
WAVETABLES_SRC = REPO_ROOT / "content" / "wavetables"
INSTALLED_WT_DIR = Path("/Library/Application Support/Patchwork Eight/Wavetables")


def resolve_installed(wavetable_id: str) -> Path | None:
    if not wavetable_id:
        return None
    p = Path(wavetable_id)
    if p.is_absolute() and p.is_file():
        return p
    base = p.name
    candidates = [
        REPO_ROOT / wavetable_id,
        WAVETABLES_SRC / base,
        INSTALLED_WT_DIR / base,
    ]
    for c in candidates:
        if c.is_file():
            return c
    return None


def main() -> int:
    missing: list[tuple[str, str]] = []
    refs = 0
    for pw8 in sorted(PRESETS_DIR.rglob("*.pw8")):
        data = json.loads(pw8.read_text())
        for op in data.get("layerA", {}).get("operators", []):
            wt = op.get("wavetableId") or ""
            if not wt:
                continue
            refs += 1
            if resolve_installed(wt) is None:
                missing.append((str(pw8.relative_to(REPO_ROOT)), wt))

    if missing:
        print(f"FAIL: {len(missing)}/{refs} wavetable refs missing (repo or installed layout):", file=sys.stderr)
        for path, wt in missing[:20]:
            print(f"  {path}: {wt}", file=sys.stderr)
        if len(missing) > 20:
            print(f"  ... and {len(missing) - 20} more", file=sys.stderr)
        return 1

    print(f"OK: all {refs} wavetableId references resolve (repo + {INSTALLED_WT_DIR})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
