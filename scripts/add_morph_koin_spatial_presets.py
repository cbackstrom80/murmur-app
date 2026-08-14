#!/usr/bin/env python3
"""Add morphKoin (INTIMATE ↔ VOID) to Interstellar Spatial showcase presets."""

from __future__ import annotations

import json
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SPATIAL = REPO / "content/presets/factory/Interstellar/Spatial"

# First 20 numbered showcase pads get morph KOIN metadata.
TARGETS = [
    "001-nebula-drift.pw8",
    "002-void-cathedral.pw8",
    "003-orbital-hymn.pw8",
    "004-stellar-mist.pw8",
    "005-cosmic-veil.pw8",
    "006-pillars-drift.pw8",
    "007-amber-nebula.pw8",
    "008-deep-field-orbit.pw8",
    "009-solar-wind-pad.pw8",
    "010-event-horizon-veil.pw8",
    "011-gravity-well-haze.pw8",
    "012-interstellar-bloom.pw8",
    "013-pale-nebula.pw8",
    "014-dust-lane-drift.pw8",
    "015-tesseract-veil.pw8",
    "016-cathedral-mist.pw8",
    "017-wormhole-drift.pw8",
    "018-stellar-nursery-wash.pw8",
    "019-cosmic-lullaby-space.pw8",
    "020-heliosphere-pad.pw8",
]

MORPH = {
    "label": "SCENE",
    "description": "INTIMATE in-head ↔ VOID binaural wash — Quasar distance and room.",
    "defaultPosition": 0.35,
    "position": 0.35,
    "curve": "smooth",
    "wrap": False,
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
                "masterEffects[2].mix": 0.15,
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
                "masterEffects[2].mix": 0.55,
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
                "masterEffects[2].quasarDelayFeedback": 0.55,
                "masterEffects[2].mix": 0.8,
            },
        },
    ],
}


def main() -> None:
    updated = 0
    for name in TARGETS:
        path = SPATIAL / name
        if not path.exists():
            print(f"skip missing {name}")
            continue
        patch = json.loads(path.read_text())
        patch["morphKoin"] = MORPH
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
        print(f"updated {name}")
    print(f"Done — morphKoin on {updated} presets.")


if __name__ == "__main__":
    main()
