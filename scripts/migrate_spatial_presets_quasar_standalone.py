#!/usr/bin/env python3
"""Migrate Interstellar Spatial MURMUR presets for standalone QUASAR plugin.

- Extract master slot type 11 (legacy BinauralSpace) params to companion .quasar files
- Replace slot with Bypass (type 0)
- Add metadata spatial=use-quasar-plugin + companionQuasar filename
- Strip morph paramOverrides referencing masterEffects quasar fields
- Remove mod routes targeting master FX slot 2 (former Quasar slot)

Run from repo root:
    python3 scripts/migrate_spatial_presets_quasar_standalone.py
"""
from __future__ import annotations

import json
import pathlib
import re

REPO = pathlib.Path(__file__).resolve().parent.parent
SPATIAL_DIR = REPO / "content" / "presets" / "factory" / "Interstellar" / "Spatial"
QUASAR_DIR = REPO / "content" / "presets" / "quasar" / "interstellar"

QUASAR_PARAM_KEYS = (
    "mix",
    "qsr1Level",
    "qsr2Level",
    "cntrLevel",
    "inputSplitHpfHz",
    "cntrHpfHz",
    "qsr1Height",
    "qsr1Angle",
    "qsr1Distance",
    "qsr2Height",
    "qsr2Angle",
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
)

QUASAR_SLOT = 2
LEGACY_BINAURAL_TYPE = 11
MASTER_FX_DESTINATIONS = {13, 14, 15, 16, 17, 18, 19}  # MasterFxMix + MasterReverb*

QUASAR_OVERRIDE_RE = re.compile(
    r"^masterEffects\[\d+\]\.(qsr|cntr|quasar|inputSplit|mix)"
)


def default_quasar_params() -> dict:
    return {
        "mix": 0.72,
        "qsr1Level": 0.65,
        "qsr2Level": 0.55,
        "cntrLevel": 0.75,
        "inputSplitHpfHz": 120.0,
        "cntrHpfHz": 80.0,
        "qsr1Height": 0.0,
        "qsr1Angle": 45.0,
        "qsr1Distance": 0.35,
        "qsr2Height": 0.0,
        "qsr2Angle": 315.0,
        "qsr2Distance": 0.35,
        "qsr1RoomAmount": 0.4,
        "qsr1RoomSize": 1.0,
        "qsr1RoomDamping": 0.45,
        "qsr2RoomAmount": 0.36,
        "qsr2RoomSize": 1.05,
        "qsr2RoomDamping": 0.42,
        "quasarDelayTimeMs": 440.0,
        "quasarDelayFeedback": 0.32,
        "quasarDelayVolume": 0.22,
        "quasarOutputMode": 0,
        "quasarCrossfeed": 0.0,
        "quasarDelaySync": 0,
        "quasarDelaySyncDivision": 2,
    }


def extract_quasar_params(slot: dict) -> dict:
    params = default_quasar_params()
    for key in QUASAR_PARAM_KEYS:
        if key in slot:
            params[key] = slot[key]
    return params


def strip_quasar_keys(slot: dict) -> None:
    for key in list(slot.keys()):
        if key.startswith(("qsr", "cntr", "quasar", "inputSplit")):
            del slot[key]


def clean_morph(morph: dict) -> None:
    for frame in morph.get("keyframes", []):
        overrides = frame.get("paramOverrides")
        if not isinstance(overrides, dict):
            continue
        frame["paramOverrides"] = {
            k: v for k, v in overrides.items() if not QUASAR_OVERRIDE_RE.match(k)
        }
        if not frame["paramOverrides"]:
            del frame["paramOverrides"]


def clean_mod_routes(layer: dict) -> None:
    """Mod routes to slot 2 are retained — slot 2 is now Reverb (was Quasar)."""
    _ = layer


def update_descriptions(doc: dict, preset_stem: str) -> None:
    meta = doc.setdefault("metadata", {})
    meta["spatial"] = "use-quasar-plugin"
    meta["companionQuasar"] = f"interstellar/{preset_stem}.quasar"

    desc = meta.get("description", "")
    if "Quasar binaural scene on master M3" in desc:
        meta["description"] = desc.replace(
            "Headphone-first Quasar binaural scene on master M3.",
            "Headphone-first spatial pad — M3 is algorithmic reverb; load companion QUASAR "
            f"preset `{preset_stem}.quasar` on the master bus for binaural spatial.",
        )

    morph = doc.get("morphKoin")
    if isinstance(morph, dict) and "Quasar" in morph.get("description", ""):
        morph["description"] = (
            "INTIMATE in-head ↔ VOID wash — use QUASAR plugin + morph position."
        )

    for macro in doc.get("macros", []):
        d = macro.get("description", "")
        if "Quasar" in d or "M3" in d and "spatial" in d.lower():
            macro["description"] = (
                "Spatial macro — pair with standalone QUASAR plugin on the master bus."
            )


def migrate_preset(path: pathlib.Path) -> None:
    doc = json.loads(path.read_text(encoding="utf-8"))
    master = doc.get("masterEffects", [])
    if len(master) <= QUASAR_SLOT:
        return

    slot = master[QUASAR_SLOT]
    if slot.get("type") != LEGACY_BINAURAL_TYPE:
        return

    stem = path.stem
    quasar_params = extract_quasar_params(slot)

    QUASAR_DIR.mkdir(parents=True, exist_ok=True)
    quasar_path = QUASAR_DIR / f"{stem}.quasar"
    preset_name = doc.get("metadata", {}).get("name", stem)
    quasar_doc = {
        "schemaVersion": 1,
        "metadata": {
            "name": preset_name.upper(),
            "author": doc.get("metadata", {}).get("author", "MURMUR Sound Design"),
            "description": f"Companion QUASAR preset for Interstellar Spatial MURMUR patch {stem}.",
            "tags": ["spatial", "quasar", "interstellar", "companion"],
            "murmurPreset": f"Interstellar/Spatial/{path.name}",
        },
        "params": quasar_params,
    }
    quasar_path.write_text(json.dumps(quasar_doc, indent=2) + "\n", encoding="utf-8")

    # Replace legacy Quasar slot with algorithmic Reverb so MURMUR-only playback and
    # SPACE macro KOINS (MasterReverb* on slot 2) still behave without the plugin.
    reverb_mix = min(1.0, float(quasar_params.get("mix", 0.72)) * 0.55)
    reverb_size = float(quasar_params.get("qsr1RoomSize", 1.0))
    reverb_decay = 2.5 + float(quasar_params.get("qsr1RoomAmount", 0.4)) * 6.0
    reverb_predelay = float(quasar_params.get("quasarDelayTimeMs", 40.0)) * 0.08
    slot.clear()
    slot.update(
        {
            "type": 7,
            "mix": reverb_mix,
            "reverbSizeParam": reverb_size,
            "reverbDecaySeconds": reverb_decay,
            "reverbPreDelayMs": reverb_predelay,
            "reverbDiffusion": 0.72,
            "reverbModDepth": 0.18,
        }
    )

    for s in master:
        strip_quasar_keys(s)

    if isinstance(doc.get("morphKoin"), dict):
        clean_morph(doc["morphKoin"])

    for layer_key in ("layerA", "layerB"):
        layer = doc.get(layer_key)
        if isinstance(layer, dict):
            clean_mod_routes(layer)

    update_descriptions(doc, stem)
    path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    paths = sorted(SPATIAL_DIR.glob("*.pw8"))
    migrated = 0
    for path in paths:
        before = path.read_text(encoding="utf-8")
        migrate_preset(path)
        after = path.read_text(encoding="utf-8")
        if before != after:
            migrated += 1
    print(f"Migrated {migrated} / {len(paths)} spatial presets")
    print(f"Companion QUASAR presets: {QUASAR_DIR}")


if __name__ == "__main__":
    main()
