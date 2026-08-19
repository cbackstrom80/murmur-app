#!/usr/bin/env python3
"""Migrate Spatial factory presets: add per-path morph paramOverrides with easing from global curve.

Spot-check: python scripts/migrate_spatial_morph_easing.py --dry-run content/presets/factory/Interstellar/Spatial/002-void-cathedral.pw8
Apply all:     python scripts/migrate_spatial_morph_easing.py
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SPATIAL_DIR = REPO_ROOT / "content/presets/factory/Interstellar/Spatial"

STANDARD_PATHS = (
    "layerA.filter1.cutoffHz",
    "layerA.filter1.resonance",
)


def macro_to_cutoff(macro0: float) -> float:
    return 200.0 + macro0 * 18000.0


def macro_to_resonance(macro1: float) -> float:
    return 0.05 + macro1 * 0.85


def migrate_keyframe(kf: dict, global_curve: str) -> bool:
    changed = False
    overrides = kf.setdefault("paramOverrides", {})
    macros = kf.get("macroValues") or []
    while len(macros) < 8:
        macros.append(0.0)

    defaults = {
        STANDARD_PATHS[0]: macro_to_cutoff(float(macros[0])),
        STANDARD_PATHS[1]: macro_to_resonance(float(macros[1] if len(macros) > 1 else 0.0)),
    }

    for path, value in defaults.items():
        existing = overrides.get(path)
        if isinstance(existing, dict):
            if not existing.get("easing"):
                existing["easing"] = global_curve
                changed = True
            continue
        if isinstance(existing, (int, float)):
            overrides[path] = {"value": float(existing), "easing": global_curve, "response": ""}
            changed = True
            continue
        overrides[path] = {"value": value, "easing": global_curve, "response": ""}
        changed = True

    return changed


def migrate_patch(path: Path, dry_run: bool) -> bool:
    data = json.loads(path.read_text(encoding="utf-8"))
    morph = data.get("morphKoin")
    if not morph or not morph.get("keyframes"):
        return False

    global_curve = morph.get("curve") or "linear"
    changed = False
    for kf in morph["keyframes"]:
        if migrate_keyframe(kf, global_curve):
            changed = True

    if changed and not dry_run:
        path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return changed


def main() -> None:
    parser = argparse.ArgumentParser(description="Add per-path morph easing to Spatial presets")
    parser.add_argument("paths", nargs="*", help="Specific .pw8 files (default: all Spatial presets)")
    parser.add_argument("--dry-run", action="store_true", help="Report changes without writing")
    args = parser.parse_args()

    if args.paths:
        files = [Path(p) if Path(p).is_absolute() else REPO_ROOT / p for p in args.paths]
    else:
        files = sorted(SPATIAL_DIR.glob("*.pw8"))

    updated = 0
    for path in files:
        if migrate_patch(path, args.dry_run):
            updated += 1
            print(("would update" if args.dry_run else "updated"), path.relative_to(REPO_ROOT))

    print(f"{'would migrate' if args.dry_run else 'migrated'} {updated}/{len(files)} preset(s)")


if __name__ == "__main__":
    main()
