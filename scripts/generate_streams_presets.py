#!/usr/bin/env python3
"""Generate Streams (master dynamics) factory demo presets (Track C).

Each preset enables masterDynamics with a distinct mode and sensible defaults
for listening / A-B checks. Output: content/presets/factory/Streams/*.pw8

Run from repo root:
    python3 scripts/generate_streams_presets.py
"""
from __future__ import annotations

import importlib.util
import json
import pathlib

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "content" / "presets" / "factory" / "Streams"
AUTHOR = "MURMUR Sound Design"
BATCH_TAG = "streams"
TYPE_TAG = "Streams"
COUNT = 10

PRESET_CATALOG = [
    ("Gate Swell", "envelope", {"attackMs": 8.0, "releaseMs": 600.0, "mix": 1.0},
     "Hold chord — gate triggers master envelope swell."),
    ("Opto Tail", "vactrol", {"attackMs": 3.0, "vactrolSlewMs": 120.0, "mix": 1.0},
     "Slow vactrol-style release after note-off."),
    ("Sidechain Duck", "follower", {"attackMs": 2.0, "releaseMs": 180.0, "sidechainGain": 1.2, "mix": 0.85},
     "Follower ducks program on sidechain peaks."),
    ("Bus Glue", "compressor", {"thresholdDb": -14.0, "ratio": 3.5, "attackMs": 12.0, "releaseMs": 120.0,
                                "makeupDb": 2.0, "mix": 0.7},
     "Soft-knee compressor with makeup — stack chords."),
    ("Punch Comp", "compressor", {"thresholdDb": -20.0, "ratio": 6.0, "attackMs": 1.5, "releaseMs": 80.0,
                                  "makeupDb": 4.0, "mix": 1.0},
     "Fast attack compressor for transient punch."),
    ("Deep Duck", "follower", {"attackMs": 1.0, "releaseMs": 400.0, "sidechainGain": 1.6, "mix": 1.0},
     "Heavy follower depth — mod wheel on mix if routed."),
    ("Slow Opto", "vactrol", {"attackMs": 6.0, "vactrolSlewMs": 280.0, "mix": 0.9},
     "Long opto tail for pad releases."),
    ("Rhythmic Gate", "envelope", {"attackMs": 1.0, "releaseMs": 120.0, "mix": 0.75},
     "Short AR envelope for rhythmic pumping."),
    ("Parallel Comp", "compressor", {"thresholdDb": -10.0, "ratio": 2.5, "attackMs": 20.0, "releaseMs": 200.0,
                                     "makeupDb": 1.0, "mix": 0.45},
     "Parallel mix — blend dry/wet on OUTPUT deck."),
    ("Streams Demo", "follower", {"attackMs": 4.0, "releaseMs": 90.0, "sidechainGain": 0.9, "mix": 0.8},
     "Factory showcase — toggle modes on OUTPUT deck."),
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


def master_dynamics_block(mode: str, overrides: dict) -> dict:
    base = {
        "enabled": True,
        "mode": mode,
        "thresholdDb": -12.0,
        "ratio": 4.0,
        "attackMs": 5.0,
        "releaseMs": 80.0,
        "sidechainGain": 1.0,
        "vactrolSlewMs": 40.0,
        "makeupDb": 0.0,
        "mix": 1.0,
    }
    base.update(overrides)
    return base


def build_streams_patch(f, ist, slot, display_name, mode, dyn_overrides, tip, rng):
    v = slot % 8
    result = ist.gen_stack(v, f, rng)
    if len(result) == 9:
        ops, amp_env, _filter1, lfo1, lfo2, mod_routes, insert, master, macro_names = result
    elif len(result) == 10:
        ops, amp_env, _filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, _arp = result
    else:
        ops, amp_env, _filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, _arp, _extras = result

    seed = hash((BATCH_TAG, mode, slot, display_name)) & 0xFFFFFFFF
    desc = f"Streams preset #{slot:03d} — {display_name}. Master dynamics ({mode}). {tip}"

    patch = f.build_patch(
        name=display_name.upper(),
        description=desc,
        category="pad",
        moods=["dynamic", "evolving", "factory"],
        tags=[BATCH_TAG, "MasterDynamics", TYPE_TAG, "factory", mode],
        seed=seed,
        operators=ops,
        ampEnv=amp_env,
        filter1=_filter1,
        lfo1=lfo1,
        lfo2=lfo2,
        modRoutes=mod_routes,
        insertEffects=insert,
        masterEffects=master,
        macroNames=macro_names,
        polyphony=12,
    )

    patch["schemaVersion"] = 3
    patch["metadata"]["schemaVersion"] = 3
    patch["metadata"]["category"] = TYPE_TAG
    patch["metadata"]["author"] = AUTHOR
    patch["metadata"]["id"] = f"pw8-{BATCH_TAG}-{slot:03d}-{seed}"
    patch["metadata"]["type"] = TYPE_TAG
    patch["metadata"]["genres"] = ["electronic", "sound-design", "factory"]
    patch["metadata"]["createdAt"] = "2026-08-17T00:00:00Z"
    patch["masterDynamics"] = master_dynamics_block(mode, dyn_overrides)
    f.normalize_preset_standards(patch)
    return patch


def main():
    f = load_factory()
    ist = load_interstellar()
    import random
    rng = random.Random(0x57EA0000)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    assert len(PRESET_CATALOG) == COUNT

    for slot, (name, mode, overrides, tip) in enumerate(PRESET_CATALOG, start=1):
        patch = build_streams_patch(f, ist, slot, name, mode, overrides, tip, rng)
        out_path = OUT_DIR / f"{slot:03d}-{slugify(name)}.murmur"
        out_path.write_text(json.dumps(patch, indent=2) + "\n")
        print(f"Wrote {out_path.name}")

    readme = f"""# Streams Factory Presets

{COUNT} master-bus dynamics demos (**Track C** — Mutable Instruments Streams parity).

| Tag | Meaning |
|-----|---------|
| `Streams` | This bank |
| `MasterDynamics` | `masterDynamics.enabled` true |
| Mode strings | `envelope`, `vactrol`, `follower`, `compressor` |

Regenerate: `python3 scripts/generate_streams_presets.py`
"""
    (OUT_DIR / "README.md").write_text(readme)
    print(f"Wrote README.md ({COUNT} presets)")


if __name__ == "__main__":
    main()
