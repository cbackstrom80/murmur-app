#!/usr/bin/env python3
"""Embed companion QUASAR presets into Interstellar Spatial MURMUR master FX slots.

Reverses the standalone-plugin migration: reads each `.quasar` companion file and
writes an inline `BinauralSpace` (type 13) slot on master M3 (index 2).

Run from repo root:
    python3 scripts/embed_spatial_quasar_slot.py [--dry-run]
"""
from __future__ import annotations

import argparse
import json
import pathlib

REPO = pathlib.Path(__file__).resolve().parent.parent
SPATIAL_DIR = REPO / "content" / "presets" / "factory" / "Interstellar" / "Spatial"
QUASAR_DIR = REPO / "content" / "presets" / "quasar" / "interstellar"

QUASAR_SLOT = 2
BINAURAL_TYPE = 13

ANGLE_ALIASES = ("qsr1AngleDeg", "qsr1Angle")
ANGLE2_ALIASES = ("qsr2AngleDeg", "qsr2Angle")

QUASAR_PARAM_KEYS = (
    "mix",
    "qsr1Level",
    "qsr2Level",
    "cntrLevel",
    "inputSplitHpfHz",
    "cntrHpfHz",
    "qsr1Height",
    "qsr1Distance",
    "qsr2Height",
    "qsr2Distance",
    "qsr1RoomAmount",
    "qsr1RoomSize",
    "qsr1RoomDamping",
    "qsr2RoomAmount",
    "qsr2RoomSize",
    "qsr2RoomDamping",
    "quasarDelayTimeMs",
    "quasarDelayFeedback",
    "quasarDelayVolume",
    "quasarOutputMode",
    "quasarCrossfeed",
    "quasarDelaySync",
    "quasarDelaySyncDivision",
    "quasarDelaySyncDivisionIndex",
)


def pick(params: dict, *keys, default=None):
    for key in keys:
        if key in params:
            return params[key]
    return default


def quasar_slot_from_companion(params: dict) -> dict:
    slot: dict = {"type": BINAURAL_TYPE, "mix": float(params.get("mix", 0.72))}
    for key in QUASAR_PARAM_KEYS:
        if key in params:
            slot[key] = params[key]
    slot["qsr1AngleDeg"] = float(pick(params, *ANGLE_ALIASES, default=30.0))
    slot["qsr2AngleDeg"] = float(pick(params, *ANGLE2_ALIASES, default=330.0))
    if "quasarDelaySync" in slot:
        slot["quasarDelaySync"] = bool(slot["quasarDelaySync"])
    if "quasarDelaySyncDivision" in params and "quasarDelaySyncDivisionIndex" not in slot:
        slot["quasarDelaySyncDivisionIndex"] = params["quasarDelaySyncDivision"]
    return slot


RETARGET_REVERB_TO_QUASAR = {
    14: 13,  # MasterReverbMix -> MasterFxMix
    15: 32,  # size -> room amount
    16: 38,  # decay -> delay feedback
    17: 37,  # pre-delay -> delay time
    18: 32,  # diffusion -> room amount
    19: 33,  # mod depth -> crossfeed
}


def retarget_mod_routes(doc: dict) -> int:
    routes = doc.get("layerA", {}).get("modRoutes", [])
    changed = 0
    for route in routes:
        if route.get("targetIndex") != QUASAR_SLOT:
            continue
        new_dest = RETARGET_REVERB_TO_QUASAR.get(route.get("destination"))
        if new_dest is not None:
            route["destination"] = new_dest
            changed += 1
    return changed


def update_metadata(doc: dict, stem: str) -> None:
    meta = doc.setdefault("metadata", {})
    meta["spatial"] = "inline-quasar"
    meta.pop("companionQuasar", None)

    desc = meta.get("description", "")
    if "standalone QUASAR" in desc or "companion QUASAR" in desc.lower():
        meta["description"] = (
            f"Interstellar spatial pad — {meta.get('name', stem)}. "
            "Headphone-first Quasar binaural scene on master M3 (embedded). "
            "Hold C3–G4 chords; sweep SPACE and DRIFT — BLOOM freezes per voice."
        )
    elif "algorithmic reverb" in desc.lower():
        meta["description"] = desc.replace(
            "M3 is algorithmic reverb; load companion QUASAR preset "
            f"`{stem}.quasar` on the master bus for binaural spatial.",
            "M3 is inline QUASAR binaural spatial (embedded in patch)",
        )

    for macro in doc.get("macros", []):
        if macro.get("name") == "SPACE":
            macro["description"] = "Spatial macro — room, delay, and mix on master Quasar (M3)."
        elif macro.get("name") == "DRIFT":
            if "reverb" in macro.get("description", "").lower():
                macro["description"] = "Orbit Quasar room, delay pre-time, and diffusion on M3."


def embed_preset(path: pathlib.Path, dry_run: bool) -> bool:
    doc = json.loads(path.read_text(encoding="utf-8"))
    stem = path.stem
    quasar_path = QUASAR_DIR / f"{stem}.quasar"
    if not quasar_path.exists():
        return False

    quasar_doc = json.loads(quasar_path.read_text(encoding="utf-8"))
    params = quasar_doc.get("params", quasar_doc)
    master = doc.setdefault("masterEffects", [])
    while len(master) <= QUASAR_SLOT:
        master.append({"type": 0, "mix": 1.0})

    master[QUASAR_SLOT] = quasar_slot_from_companion(params)
    update_metadata(doc, stem)
    retarget_mod_routes(doc)

    if not dry_run:
        path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    return True


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true", help="Report without writing files")
    args = parser.parse_args()

    paths = sorted(SPATIAL_DIR.glob("*.pw8"))
    embedded = 0
    missing = 0
    for path in paths:
        quasar_path = QUASAR_DIR / f"{path.stem}.quasar"
        if not quasar_path.exists():
            missing += 1
            continue
        if embed_preset(path, args.dry_run):
            embedded += 1
    mode = "Would embed" if args.dry_run else "Embedded"
    print(f"{mode} {embedded} / {len(paths)} spatial presets ({missing} missing companion .quasar)")


if __name__ == "__main__":
    main()
