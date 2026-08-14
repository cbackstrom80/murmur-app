#!/usr/bin/env python3
"""Generate 100 PoliMATHS macro-dissemination factory presets.

Every preset has voiceSettings.macroDissemination: true, 1–3 feature macro KOINS
with multi-destination modRoutes, and metadata tags for the Dissemination bank.

Output: content/presets/factory/Dissemination/001-*.pw8 … 100-*.pw8 + README.md

Run from repo root:
    python3 scripts/generate_dissemination_presets.py
"""
from __future__ import annotations

import importlib.util
import json
import pathlib
import random
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "content" / "presets" / "factory" / "Dissemination"
MANIFEST_PATH = REPO_ROOT / "content" / "presets" / "factory" / "MANIFEST.json"
AUTHOR = "MURMUR Sound Design"
BATCH_TAG = "dissemination"
TYPE_TAG = "Dissemination"
COUNT = 100

# (display_name, archetype_key, variant_index)
PRESET_CATALOG = [
    # 25 spread pads — held chords, macro freeze bloom
    ("Strata Chord", "spread_pad", 0),
    ("Frozen Bloom", "spread_pad", 1),
    ("Voice Shadow", "spread_pad", 2),
    ("Poly Veil", "spread_pad", 3),
    ("Layered Hymn", "spread_pad", 4),
    ("Chord Mist", "spread_pad", 5),
    ("Velvet Spread", "spread_pad", 6),
    ("Cathedral Layers", "spread_pad", 7),
    ("Amber Strata", "spread_pad", 8),
    ("Halo Wash", "spread_pad", 9),
    ("Drift Ensemble", "spread_pad", 10),
    ("Morphic Pad", "spread_pad", 11),
    ("Glass Strata", "spread_pad", 12),
    ("Warm Bloom", "spread_pad", 13),
    ("Echo Chord", "spread_pad", 14),
    ("Swell Matrix", "spread_pad", 15),
    ("Cloud Stack", "spread_pad", 16),
    ("Prism Hold", "spread_pad", 17),
    ("Luminous Veil", "spread_pad", 18),
    ("Tidal Pad", "spread_pad", 19),
    ("Horizon Bloom", "spread_pad", 20),
    ("Silk Spread", "spread_pad", 21),
    ("Dusk Layers", "spread_pad", 22),
    ("Feather Chord", "spread_pad", 23),
    ("Nova Hold", "spread_pad", 24),
    # 25 ambient drones
    ("Frozen Drift", "drone", 0),
    ("Granular Mist", "drone", 1),
    ("Resonant Cave", "drone", 2),
    ("Void Bloom", "drone", 3),
    ("Shimmer Field", "drone", 4),
    ("Tape Haze", "drone", 5),
    ("Sub Strata", "drone", 6),
    ("Dust Cloud", "drone", 7),
    ("Polar Drone", "drone", 8),
    ("Ember Wash", "drone", 9),
    ("Glacier Hold", "drone", 10),
    ("Basin Expanse", "drone", 11),
    ("Ochre Field", "drone", 12),
    ("Hollow Echo", "drone", 13),
    ("Nebula Wash", "drone", 14),
    ("Tundra Bloom", "drone", 15),
    ("Fathomless Drift", "drone", 16),
    ("Ember Hymn", "drone", 17),
    ("Abyssal Swell", "drone", 18),
    ("Midnight Veil", "drone", 19),
    ("Pale Expanse", "drone", 20),
    ("Quartz Drone", "drone", 21),
    ("Spectral Hold", "drone", 22),
    ("Ion Mist", "drone", 23),
    ("Deep Basin", "drone", 24),
    # 20 cinematic textures
    ("Score Bloom", "cinematic", 0),
    ("Film Strata", "cinematic", 1),
    ("Tension Hold", "cinematic", 2),
    ("Reveal Chord", "cinematic", 3),
    ("Memory Layer", "cinematic", 4),
    ("Grief Pad", "cinematic", 5),
    ("Hope Spread", "cinematic", 6),
    ("Epic Wash", "cinematic", 7),
    ("Intimate Hold", "cinematic", 8),
    ("Vast Choir", "cinematic", 9),
    ("Solstice Bloom", "cinematic", 10),
    ("Equinox Veil", "cinematic", 11),
    ("Twilight Strata", "cinematic", 12),
    ("Aurora Hold", "cinematic", 13),
    ("Monolith Pad", "cinematic", 14),
    ("Passage Bloom", "cinematic", 15),
    ("Threshold Chord", "cinematic", 16),
    ("Afterglow Spread", "cinematic", 17),
    ("Meridian Wash", "cinematic", 18),
    ("Obsidian Layer", "cinematic", 19),
    # 15 evolving stacks (dual layer / unison)
    ("Dual Spread", "stack", 0),
    ("Parallel Voices", "stack", 1),
    ("Morphic Stack", "stack", 2),
    ("Binary Bloom", "stack", 3),
    ("Twin Strata", "stack", 4),
    ("Layered Cosmos", "stack", 5),
    ("Echo Stack", "stack", 6),
    ("Nebula Layers", "stack", 7),
    ("Organ Spread", "stack", 8),
    ("Stack Of Voices", "stack", 9),
    ("Multiverse Hold", "stack", 10),
    ("Folded Chord", "stack", 11),
    ("Phase Stack", "stack", 12),
    ("Reso Layers", "stack", 13),
    ("Gran Stack", "stack", 14),
    # 15 chord-freeze showcases (granular / formant / WT morph)
    ("Formant Freeze", "freeze", 0),
    ("WT Strata", "freeze", 1),
    ("Morph Hold", "freeze", 2),
    ("Spread Formant", "freeze", 3),
    ("Bend Bloom", "freeze", 4),
    ("Sync Layers", "freeze", 5),
    ("Vowel Stack", "freeze", 6),
    ("Frame Drift", "freeze", 7),
    ("Position Hold", "freeze", 8),
    ("Asym Spread", "freeze", 9),
    ("Gran Freeze", "freeze", 10),
    ("Cloud Morph", "freeze", 11),
    ("Shimmer Hold", "freeze", 12),
    ("Phase Bloom", "freeze", 13),
    ("Reso Freeze", "freeze", 14),
]

assert len(PRESET_CATALOG) == COUNT

ARCHETYPE_INTERNAL_CAT = {
    "spread_pad": "pad",
    "drone": "ambient",
    "cinematic": "pad",
    "stack": "pad",
    "freeze": "ambient",
}

ARCHETYPE_TAGS = {
    "spread_pad": ["dissemination", "poly-spread", "held-chord", "macro-freeze"],
    "drone": ["dissemination", "evolving-drone", "gran-cloud", "macro-freeze"],
    "cinematic": ["dissemination", "cinematic", "score-pad", "macro-freeze"],
    "stack": ["dissemination", "dual-layer", "stack-showcase", "macro-freeze"],
    "freeze": ["dissemination", "wt-evolve", "formant-choir", "macro-freeze"],
}

PLAYING_TIPS = {
    "spread_pad": "Hold C3–G4 chords; sweep macro 1–3 — each voice keeps its note-on snapshot.",
    "drone": "Single or two-note holds 8+ seconds; macros morph frozen voices independently.",
    "cinematic": "Slow macro sweeps over held triads; mod wheel adds global bloom on new notes.",
    "stack": "Mid-register chords; MORPH crossfades layers while held voices stay frozen.",
    "freeze": "WT/formant macros — retrigger notes at different macro positions for spread.",
}

THEME_GROUPS = [
    ("Spread Pads", "Polyphonic held-chord pads with per-voice macro capture.", range(1, 26)),
    ("Ambient Drones", "Long-evolving granular and resonant drones.", range(26, 51)),
    ("Cinematic Textures", "Score-ready washes and emotional swells.", range(51, 71)),
    ("Layer Stacks", "Dual-layer and unison stack showcases.", range(71, 86)),
    ("Freeze Showcases", "Wavetable/formant macro freeze demos.", range(86, 101)),
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


def gen_spread_pad(v, f, ist, rng):
    """Reuse cosmic pad generator — dissemination's sweet spot."""
    return ist.gen_cosmic_pad(v, f, rng)


def gen_drone(v, f, ist, rng):
    return ist.gen_texture(v, f, rng)


def gen_cinematic(v, f, ist, rng):
    if v % 3 == 0:
        return ist.gen_cosmic_pad(v, f, rng)
    return ist.gen_stack(v % 8, f, rng)


def gen_stack(v, f, ist, rng):
    return ist.gen_stack(v, f, rng)


def gen_freeze(v, f, ist, rng):
    if v % 2 == 0:
        return ist.gen_cosmic_pad(v, f, rng)
    return ist.gen_texture(v, f, rng)


GENERATORS = {
    "spread_pad": gen_spread_pad,
    "drone": gen_drone,
    "cinematic": gen_cinematic,
    "stack": gen_stack,
    "freeze": gen_freeze,
}


def build_dissemination_patch(f, ist, slot, display_name, archetype, variant, rng):
    gen = GENERATORS[archetype]
    internal_cat = ARCHETYPE_INTERNAL_CAT[archetype]
    result = gen(variant, f, ist, rng)
    arp = None
    extras = {}
    if len(result) == 9:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names = result
    elif len(result) == 10:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, arp = result
    else:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, arp, extras = result

    # Dissemination presets need polyphonic headroom for chord spread demos.
    polyphony = max(extras.pop("polyphony", 16), 16)
    if archetype in ("spread_pad", "cinematic", "stack"):
        polyphony = max(polyphony, 24)

    seed = hash((BATCH_TAG, archetype, slot, display_name)) & 0xFFFFFFFF
    engines_used = sorted({o["engine"] for o in ops})
    engine_names = ["Classic", "Wavetable", "FM/PM", "Additive", "PhaseShape",
                    "Granular", "NoiseChaos", "Resonator"]
    tip = PLAYING_TIPS[archetype]
    desc = (f"Dissemination preset #{slot:03d} — {display_name}. "
            f"PoliMATHS per-note macro capture. {tip} "
            f"Engines: {', '.join(engine_names[e] for e in engines_used)}.")

    tags = [BATCH_TAG, TYPE_TAG.lower(), "PoliMATHS", "factory"] + ARCHETYPE_TAGS[archetype]
    moods = ["spread", "evolving", "cinematic", "held"]

    patch = f.build_patch(
        name=display_name.upper(),
        description=desc,
        category=internal_cat,
        moods=moods,
        tags=tags,
        seed=seed,
        operators=ops,
        ampEnv=amp_env,
        filter1=filter1,
        lfo1=lfo1,
        lfo2=lfo2,
        modRoutes=mod_routes,
        insertEffects=insert,
        masterEffects=master,
        macroNames=macro_names,
        arpeggiator=arp,
        polyphony=polyphony,
    )

    patch["schemaVersion"] = 3
    patch["metadata"]["schemaVersion"] = 3
    patch["metadata"]["category"] = TYPE_TAG
    patch["metadata"]["type"] = TYPE_TAG
    patch["metadata"]["author"] = AUTHOR
    patch["metadata"]["id"] = f"pw8-{BATCH_TAG}-{slot:03d}-{seed}"
    patch["metadata"]["genres"] = ["ambient", "cinematic", "factory"]
    patch["metadata"]["createdAt"] = "2026-08-14T00:00:00Z"

    patch["voiceSettings"]["macroDissemination"] = True

    if "filter2" in extras:
        patch["layerA"]["filter2"] = extras["filter2"]
    if extras.get("layer_mode"):
        patch["layerMode"] = extras["layer_mode"]
    if extras.get("layer_b"):
        patch["layerB"] = extras["layer_b"]
    unison = extras.pop("unison", None)
    if unison:
        patch["layerA"]["unison"] = unison

    f.normalize_preset_standards(patch)
    # Re-assert after normalize (normalize doesn't strip dissemination).
    patch["voiceSettings"]["macroDissemination"] = True
    return patch


def write_readme():
    rows = []
    for slot, (name, arch, _) in enumerate(PRESET_CATALOG, start=1):
        rows.append(f"| {slot:03d} | `{slugify(name)}.pw8` | {arch.replace('_', ' ')} | {name.upper()} |")

    theme_sections = []
    for title, blurb, slots in THEME_GROUPS:
        theme_sections.append(f"### {title}\n\n{blurb}\n")
        sub = [r for i, r in enumerate(rows, start=1) if i in slots]
        theme_sections.append("| # | File | Archetype | Name |\n|---:|------|-----------|------|\n")
        theme_sections.extend(line + "\n" for line in sub)
        theme_sections.append("\n")

    readme = f"""# Dissemination Factory Bank ({COUNT} presets)

PoliMATHS-inspired **macro dissemination** — per-note macro capture at note-on.
Every preset has `voiceSettings.macroDissemination: true` and 1–3 feature macro KOINS.

**Showcase reference:** Interstellar `CATHEDRAL NEBULA` (001).

## Playing

1. Hold a chord (C3–G4 recommended).
2. Sweep Macro 1–3 — voices triggered earlier keep their note-on macro values.
3. Add new notes while moving macros — each voice gets its own frozen snapshot.

## Theme groups

{''.join(theme_sections)}

## Regenerate

```bash
python3 scripts/generate_dissemination_presets.py
python3 scripts/audit_dissemination_candidates.py
```

Authored by {AUTHOR}. Batch tag: `{BATCH_TAG}`.
"""
    (OUT_DIR / "README.md").write_text(readme)


def update_manifest(new_entries: list[dict]) -> None:
    manifest = []
    if MANIFEST_PATH.exists():
        manifest = json.loads(MANIFEST_PATH.read_text())
    manifest = [e for e in manifest if "factory/Dissemination/" not in e.get("file", "")]
    manifest.extend(new_entries)
    manifest.sort(key=lambda e: (e["category"], e["file"]))
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"MANIFEST.json now lists {len(manifest)} total factory presets.")


def main():
    f = load_factory()
    ist = load_interstellar()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for stale in OUT_DIR.glob("*.pw8"):
        stale.unlink()

    manifest_entries = []
    for slot, (display_name, archetype, variant) in enumerate(PRESET_CATALOG, start=1):
        seed_rng = random.Random(hash((BATCH_TAG, slot, display_name)) & 0xFFFFFFFF)
        patch = build_dissemination_patch(f, ist, slot, display_name, archetype, variant, seed_rng)
        fname = f"{slot:03d}-{slugify(display_name)}.pw8"
        path = OUT_DIR / fname
        path.write_text(json.dumps(patch, indent=2) + "\n")
        manifest_entries.append({
            "category": TYPE_TAG,
            "file": f"factory/Dissemination/{fname}",
            "name": display_name.upper(),
        })

    write_readme()
    update_manifest(manifest_entries)
    print(f"Generated {len(manifest_entries)} Dissemination presets in {OUT_DIR}")


if __name__ == "__main__":
    main()
