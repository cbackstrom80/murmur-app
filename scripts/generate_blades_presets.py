#!/usr/bin/env python3
"""Generate 15 Blades dual-filter factory presets (Track B-M3).

Each preset enables Filter 1 + Filter 2, demonstrates routing morph / mode morph /
drive, and includes mod routes to Blades destinations where useful.

Output: content/presets/factory/Blades/001-*.pw8 … 015-*.pw8 + README.md

Run from repo root:
    python3 scripts/generate_blades_presets.py
"""
from __future__ import annotations

import importlib.util
import json
import pathlib

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "content" / "presets" / "factory" / "Blades"
AUTHOR = "MURMUR Sound Design"
BATCH_TAG = "blades"
TYPE_TAG = "Blades"
COUNT = 15

# ModDestination ordinals — must match ModMatrixTypes.hpp
DST_FILTER_CUTOFF = 1
DST_FILTER_RES = 2
DST_FILTER_MODE_MORPH = 25
DST_FILTER_ROUTING = 26
DST_FILTER_DRIVE = 27

SRC_LFO1 = 1
SRC_LFO2 = 2
SRC_ENV1 = 9
SRC_MOD_WHEEL = 29
SRC_MACRO1 = 21
SRC_VELOCITY = 17

PRESET_CATALOG = [
    ("Serial Sweep", "serial", 0.0, 0.0, 0.15, "Hold mid chords; sweep macro 1 for cutoff."),
    ("Parallel Bloom", "parallel", 0.5, 0.35, 0.25, "Routing at parallel — wide dual-filter stack."),
    ("Crossfade Morph", "crossfade", 1.0, 0.65, 0.35, "Crossfade routing with driven F2 character."),
    ("LP Morph Drift", "mode_morph", 0.25, 0.0, 0.1, "Mode morph LFO — LP toward BP sweep."),
    ("BP Focus", "mode_morph", 0.5, 0.5, 0.2, "Bandpass morph center with moderate drive."),
    ("HP Edge", "mode_morph", 0.75, 1.0, 0.4, "Bright HP morph with ladder grit."),
    ("Drive Stack", "drive", 0.0, 0.3, 0.55, "Serial stack — macro pushes F2 drive."),
    ("Offset Track", "offset", 0.0, 0.0, 0.2, "F2 tracks F1 +7 semis; mod wheel opens cutoff."),
    ("Routing LFO", "routing_lfo", 0.5, 0.4, 0.3, "LFO1 sweeps routing morph live."),
    ("Dual Reso", "resonance", 0.35, 0.45, 0.45, "Both filters resonant — expression on reso."),
    ("Sub Blade", "bass", 0.0, 0.0, 0.5, "Low register — serial filters for bass weight."),
    ("Pad Blade", "pad", 0.5, 0.25, 0.18, "Held pad with parallel routing wash."),
    ("Lead Cut", "lead", 0.15, 0.85, 0.28, "Lead line — HP morph + moderate drive."),
    ("Motion Route", "motion", 0.65, 0.55, 0.32, "Macro 1 crossfades routing; LFO on drive."),
    ("Blades Demo", "demo", 0.5, 0.5, 0.35, "All Blades controls active — factory showcase."),
]


def load_factory():
    path = REPO_ROOT / "scripts" / "generate_factory_presets.py"
    spec = importlib.util.spec_from_file_location("factory_presets", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def load_interstellar():
    path = REPO_ROOT / "scripts" / "generate_interstellar_presets.py"
    spec = importlib.util.spec_from_file_location("interstellar_presets", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def slugify(name: str) -> str:
    return name.lower().replace(" ", "-").replace("'", "")


def filter1_blades(mode=0, cutoff=4200.0, reso=0.35, mode_morph=0.0):
    return {
        "enabled": True,
        "mode": mode,
        "modeMorph": mode_morph,
        "cutoffHz": cutoff,
        "resonance": reso,
        "keyTrack": 0.35,
    }


def filter2_blades(cutoff=3200.0, reso=0.28, drive=0.2, offset_semis=0.0):
    return {
        "enabled": True,
        "cutoffHz": cutoff,
        "resonance": reso,
        "drive": drive,
        "cutoffOffsetSemis": offset_semis,
        "keyTrack": 0.2,
    }


def build_blades_patch(f, ist, slot, display_name, archetype, routing, mode_morph, drive, tip, rng):
    v = slot % 8
    if archetype == "bass":
        result = ist.gen_stack(v, f, rng)
    elif archetype == "pad":
        result = ist.gen_cosmic_pad(v, f, rng)
    elif archetype in ("lead", "resonance"):
        result = ist.gen_stack(v, f, rng)
    else:
        result = ist.gen_texture(v, f, rng)

    if len(result) == 9:
        ops, amp_env, _filter1, lfo1, lfo2, mod_routes, insert, master, macro_names = result
    elif len(result) == 10:
        ops, amp_env, _filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, _arp = result
    else:
        ops, amp_env, _filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, _arp, _extras = result

    mode = 0 if mode_morph < 0.34 else (2 if mode_morph < 0.67 else 1)
    f1 = filter1_blades(mode=mode, cutoff=3800 + slot * 120, reso=0.28 + slot * 0.01, mode_morph=mode_morph)
    f2 = filter2_blades(cutoff=2800 + slot * 80, drive=drive, offset_semis=7.0 if archetype == "offset" else 0.0)

    routes = list(mod_routes)
    routes.append({"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0,
                     "amount": 24.0, "scope": 0})
    if archetype == "routing_lfo":
        routes.append({"source": SRC_LFO1, "destination": DST_FILTER_ROUTING, "targetIndex": 0,
                         "amount": 0.45, "scope": 1})
    if archetype in ("mode_morph", "demo", "lead"):
        routes.append({"source": SRC_LFO2, "destination": DST_FILTER_MODE_MORPH, "targetIndex": 0,
                         "amount": 0.35, "scope": 1})
    if archetype in ("drive", "motion", "demo"):
        routes.append({"source": SRC_MACRO1, "destination": DST_FILTER_DRIVE, "targetIndex": 0,
                         "amount": 0.4, "scope": 0})
    if archetype == "motion":
        routes.append({"source": SRC_MACRO1, "destination": DST_FILTER_ROUTING, "targetIndex": 0,
                         "amount": 0.5, "scope": 0})
    if archetype == "resonance":
        routes.append({"source": SRC_ENV1, "destination": DST_FILTER_RES, "targetIndex": 0,
                         "amount": 0.35, "scope": 0})

    seed = hash((BATCH_TAG, archetype, slot, display_name)) & 0xFFFFFFFF
    desc = (f"Blades preset #{slot:03d} — {display_name}. Dual-filter routing morph showcase. {tip}")

    patch = f.build_patch(
        name=display_name.upper(),
        description=desc,
        category="bass" if archetype == "bass" else "pad",
        moods=["dark", "evolving", "gritty"],
        tags=[BATCH_TAG, "Filter2", "Blades", "factory", archetype],
        seed=seed,
        operators=ops,
        ampEnv=amp_env,
        filter1=f1,
        lfo1=lfo1,
        lfo2=lfo2,
        modRoutes=routes,
        insertEffects=insert,
        masterEffects=master,
        macroNames=macro_names,
        polyphony=16 if archetype != "bass" else 8,
    )

    patch["schemaVersion"] = 3
    patch["metadata"]["schemaVersion"] = 3
    patch["metadata"]["category"] = TYPE_TAG
    patch["metadata"]["author"] = AUTHOR
    patch["metadata"]["id"] = f"pw8-{BATCH_TAG}-{slot:03d}-{seed}"
    patch["metadata"]["type"] = TYPE_TAG
    patch["metadata"]["genres"] = ["electronic", "sound-design", "factory"]
    patch["metadata"]["createdAt"] = "2026-08-17T00:00:00Z"
    patch["layerA"]["filter2"] = f2
    patch["layerA"]["filterRouting"] = routing
    f.normalize_preset_standards(patch)
    return patch


def main():
    f = load_factory()
    ist = load_interstellar()
    import random
    rng = random.Random(0xB1ADE000)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    assert len(PRESET_CATALOG) == COUNT

    for slot, (name, archetype, routing, mode_morph, drive, tip) in enumerate(PRESET_CATALOG, start=1):
        patch = build_blades_patch(f, ist, slot, name, archetype, routing, mode_morph, drive, tip, rng)
        out_path = OUT_DIR / f"{slot:03d}-{slugify(name)}.murmur"
        out_path.write_text(json.dumps(patch, indent=2) + "\n")
        print(f"Wrote {out_path.name}")

    readme = f"""# Blades Factory Presets

{COUNT} dual-filter presets demonstrating **Filter 1 SVF + Filter 2 character** routing morph (Track B).

| Tag | Meaning |
|-----|---------|
| `Blades` | This bank |
| `Filter2` | Filter 2 enabled |
| `blades` | Routing/morph/drive showcase |

## Playing tips

- Open **PLAY → FILTER** tab — use **Blades** row (Route, Morph, Drive knobs).
- Sweep **Route** 0→1: serial F1→F2 → parallel → crossfade.
- Mod matrix destinations: Filter Mode Morph (25), Filter Routing (26), Filter 2 Drive (27).

Generated by `scripts/generate_blades_presets.py`.
"""
    (OUT_DIR / "README.md").write_text(readme)
    print(f"Done — {COUNT} presets in {OUT_DIR}")


if __name__ == "__main__":
    main()
