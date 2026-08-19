#!/usr/bin/env python3
"""Add morphKoin (INTIMATE ↔ VOID) to all Interstellar Spatial factory presets."""

from __future__ import annotations

import json
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SPATIAL = REPO / "content/presets/factory/Interstellar/Spatial"

BASE_MORPH = {
    "label": "SCENE",
    "description": "INTIMATE in-head ↔ VOID binaural wash — Quasar distance and room.",
    "defaultPosition": 0.35,
    "position": 0.35,
    "curve": "smooth",
    "wrap": False,
}


def morph_for_slot(slot: int) -> dict:
    """Slight per-preset variation so morph keyframes track each pad's Quasar mix."""
    t = (slot - 1) / 74.0
    intimate_mix = round(0.12 + 0.06 * t, 2)
    stage_mix = round(0.50 + 0.08 * t, 2)
    void_mix = round(0.72 + 0.12 * t, 2)
    void_fb = round(0.48 + 0.12 * t, 2)
    return {
        **BASE_MORPH,
        "keyframes": [
            {
                "name": "INTIMATE",
                "position": 0.0,
                "macroValues": [0.12, 0.05, 0, 0, 0, 0, 0, 0],
                "paramOverrides": {
                    "masterEffects[2].qsr1Distance": 0.1,
                    "masterEffects[2].qsr2Distance": 0.15,
                    "masterEffects[2].qsr1RoomAmount": 0.12,
                    "masterEffects[2].qsr2RoomAmount": 0.1,
                    "masterEffects[2].cntrLevel": 0.95,
                    "masterEffects[2].mix": intimate_mix,
                },
            },
            {
                "name": "STAGE",
                "position": 0.45,
                "macroValues": [0.45, 0.28, 0, 0, 0, 0, 0, 0],
                "paramOverrides": {
                    "masterEffects[2].qsr1Distance": 0.4,
                    "masterEffects[2].qsr2Distance": 0.45,
                    "masterEffects[2].qsr1RoomAmount": 0.35,
                    "masterEffects[2].qsr2RoomAmount": 0.32,
                    "masterEffects[2].cntrLevel": 0.75,
                    "masterEffects[2].mix": stage_mix,
                },
            },
            {
                "name": "VOID",
                "position": 1.0,
                "macroValues": [0.78, 0.62, 0, 0, 0, 0, 0, 0],
                "paramOverrides": {
                    "masterEffects[2].qsr1Distance": 0.85,
                    "masterEffects[2].qsr2Distance": 0.9,
                    "masterEffects[2].qsr1RoomAmount": 0.72,
                    "masterEffects[2].qsr2RoomAmount": 0.68,
                    "masterEffects[2].cntrLevel": 0.45,
                    "masterEffects[2].quasarDelayFeedback": void_fb,
                    "masterEffects[2].mix": void_mix,
                },
            },
        ],
    }


def slot_from_name(name: str) -> int:
    try:
        return int(name.split("-", 1)[0])
    except ValueError:
        return 1


def main() -> None:
    updated = 0
    skipped = 0
    for path in sorted(SPATIAL.glob("*.pw8")) + sorted(SPATIAL.glob("*.murmur")):
        patch = json.loads(path.read_text())
        slot = slot_from_name(path.stem)
        patch["morphKoin"] = morph_for_slot(slot)
        ui = patch.setdefault("uiFocus", {"maxKnobs": 3, "knobs": []})
        knobs = [k for k in ui.get("knobs", []) if k.get("kind") != "morph"]
        knobs.insert(0, {"kind": "morph", "label": "SCENE"})
        ui["knobs"] = knobs[:3]
        ui["maxKnobs"] = 3
        if patch.get("voiceSettings") is None:
            patch["voiceSettings"] = {}
        patch["voiceSettings"]["macroDissemination"] = True
        patch["voiceSettings"]["disseminationDepth"] = patch["voiceSettings"].get("disseminationDepth", 0.28)
        path.write_text(json.dumps(patch, indent=2) + "\n")
        updated += 1
        print(f"updated {path.name}")
    print(f"Done — morphKoin on {updated} presets ({skipped} skipped).")


if __name__ == "__main__":
    main()
