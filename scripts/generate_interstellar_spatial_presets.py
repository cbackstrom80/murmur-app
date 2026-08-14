#!/usr/bin/env python3
"""Generate 75 headphone-first Interstellar spatial pad factory presets.

Every preset: master FX M3 = BinauralSpace (QUASAR), 1–3 expressive macro KOINS
with multi-route bundles, slow LFO orbit on global Quasar params, macro dissemination.

Output: content/presets/factory/Interstellar/Spatial/001-*.pw8 … 075-*.pw8 + README.md

Run from repo root:
    python3 scripts/generate_interstellar_spatial_presets.py
"""
from __future__ import annotations

import importlib.util
import json
import pathlib
import random

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "content" / "presets" / "factory" / "Interstellar" / "Spatial"
MANIFEST_PATH = REPO_ROOT / "content" / "presets" / "factory" / "MANIFEST.json"
AUTHOR = "MURMUR Sound Design"
BATCH_TAG = "interstellar-spatial"
TYPE_TAG = "Interstellar"
COUNT = 75
QUASAR_SLOT = 2  # master M3

# ModSource / ModDestination ordinals — ModMatrixTypes.hpp
SRC_LFO1, SRC_LFO2 = 1, 2
SRC_MACRO1 = 21
SRC_MOD_WHEEL = 29
SRC_EXPRESSION = 30
SRC_CHANNEL_PRESSURE = 18

DST_FILTER_CUTOFF = 1
DST_FILTER_RES = 2
DST_OP_LEVEL = 5
DST_PAN = 6
DST_WT_POS = 7
DST_WT_BEND = 8
DST_WT_ASYM = 9
DST_WT_FORMANT = 11

DST_MASTER_FX_MIX = 13
DST_MASTER_REVERB_MIX = 14
DST_MASTER_REVERB_SIZE = 15
DST_MASTER_REVERB_DECAY = 16
DST_MASTER_REVERB_PREDELAY = 17
DST_MASTER_REVERB_DIFFUSION = 18

SCOPE_VOICE = 0
SCOPE_GLOBAL = 2

ANGLE_PAIRS = [
    (30.0, 330.0), (45.0, 315.0), (90.0, 270.0), (60.0, 300.0),
    (120.0, 240.0), (35.0, 325.0), (70.0, 290.0), (150.0, 210.0),
]

# (display_name, archetype_key, variant_index)
PRESET_CATALOG = [
    # 25 wide nebula pads
    ("Nebula Drift", "wide_nebula", 0),
    ("Void Cathedral", "wide_nebula", 1),
    ("Orbital Hymn", "wide_nebula", 2),
    ("Stellar Mist", "wide_nebula", 3),
    ("Cosmic Veil", "wide_nebula", 4),
    ("Pillars Drift", "wide_nebula", 5),
    ("Amber Nebula", "wide_nebula", 6),
    ("Deep Field Orbit", "wide_nebula", 7),
    ("Solar Wind Pad", "wide_nebula", 8),
    ("Event Horizon Veil", "wide_nebula", 9),
    ("Gravity Well Haze", "wide_nebula", 10),
    ("Interstellar Bloom", "wide_nebula", 11),
    ("Pale Nebula", "wide_nebula", 12),
    ("Dust Lane Drift", "wide_nebula", 13),
    ("Tesseract Veil", "wide_nebula", 14),
    ("Cathedral Mist", "wide_nebula", 15),
    ("Wormhole Drift", "wide_nebula", 16),
    ("Stellar Nursery Wash", "wide_nebula", 17),
    ("Cosmic Lullaby Space", "wide_nebula", 18),
    ("Heliosphere Pad", "wide_nebula", 19),
    ("Magnetosphere Veil", "wide_nebula", 20),
    ("Oort Drift", "wide_nebula", 21),
    ("Redshift Nebula", "wide_nebula", 22),
    ("Quasar Mist", "wide_nebula", 23),
    ("Dark Flow Pad", "wide_nebula", 24),
    # 25 orbital drift
    ("Orbital Drift", "orbital_drift", 0),
    ("Lagrange Point", "orbital_drift", 1),
    ("Apogee Wash", "orbital_drift", 2),
    ("Perigee Veil", "orbital_drift", 3),
    ("Synodic Orbit", "orbital_drift", 4),
    ("Equatorial Drift", "orbital_drift", 5),
    ("Polar Orbit Pad", "orbital_drift", 6),
    ("Kepler Drift", "orbital_drift", 7),
    ("Tidal Orbit", "orbital_drift", 8),
    ("Roche Drift", "orbital_drift", 9),
    ("Eclipse Orbit", "orbital_drift", 10),
    ("Zenith Drift", "orbital_drift", 11),
    ("Nadir Wash", "orbital_drift", 12),
    ("Ascension Orbit", "orbital_drift", 13),
    ("Descent Veil", "orbital_drift", 14),
    ("Circumlunar Pad", "orbital_drift", 15),
    ("Heliocentric Drift", "orbital_drift", 16),
    ("Galactic Orbit", "orbital_drift", 17),
    ("Spiral Arm Drift", "orbital_drift", 18),
    ("Cosmic Orbit", "orbital_drift", 19),
    ("Parallax Drift", "orbital_drift", 20),
    ("Precession Pad", "orbital_drift", 21),
    ("Libration Wash", "orbital_drift", 22),
    ("Aphelion Veil", "orbital_drift", 23),
    ("Perihelion Drift", "orbital_drift", 24),
    # 15 void cathedral
    ("Void Sanctuary", "void_cathedral", 0),
    ("Cathedral Orbit", "void_cathedral", 1),
    ("Organ Of Stars", "void_cathedral", 2),
    ("Hymn Of Void", "void_cathedral", 3),
    ("Sacred Nebula", "void_cathedral", 4),
    ("Monolith Pad", "void_cathedral", 5),
    ("Obsidian Cathedral", "void_cathedral", 6),
    ("Penumbra Hymn", "void_cathedral", 7),
    ("Umbra Drift", "void_cathedral", 8),
    ("Abyssal Orbit", "void_cathedral", 9),
    ("Chancel Wash", "void_cathedral", 10),
    ("Nave Of Void", "void_cathedral", 11),
    ("Transept Drift", "void_cathedral", 12),
    ("Apse Nebula", "void_cathedral", 13),
    ("Cloister Pad", "void_cathedral", 14),
    # 10 echo chamber style stacks
    ("Echo Chamber Drift", "echo_chamber", 0),
    ("Chamber Orbit", "echo_chamber", 1),
    ("Resonant Void", "echo_chamber", 2),
    ("Hall Of Stars", "echo_chamber", 3),
    ("Vault Nebula", "echo_chamber", 4),
    ("Cavern Orbit", "echo_chamber", 5),
    ("Grotto Drift", "echo_chamber", 6),
    ("Amphitheater Pad", "echo_chamber", 7),
    ("Rotunda Wash", "echo_chamber", 8),
    ("Atrium Space", "echo_chamber", 9),
]

assert len(PRESET_CATALOG) == COUNT

ARCHETYPE_TAGS = {
    "wide_nebula": ["spatial", "quasar", "binaural", "nebula-pad", "headphone-first"],
    "orbital_drift": ["spatial", "quasar", "binaural", "orbital-drift", "evolving-space"],
    "void_cathedral": ["spatial", "quasar", "binaural", "void-cathedral", "organ-pad"],
    "echo_chamber": ["spatial", "quasar", "binaural", "echo-chamber", "dual-layer"],
}

PLAYING_TIPS = {
    "wide_nebula": "Hold C3–G4 chords; headphones required. Sweep SPACE and DRIFT — BLOOM freezes per voice.",
    "orbital_drift": "Single held notes 8+ s; ORBIT and SPACE macros move the Quasar scene; slow LFO orbit is always on.",
    "void_cathedral": "Organ-weight mid register; VOID opens room and distance; CNTR holds the hymn body.",
    "echo_chamber": "Mid-register chords; dual layers + Quasar delay wash; SPACE opens the chamber.",
}

THEME_GROUPS = [
    ("Wide Nebula Pads", "Expansive Quasar washes with BLOOM/SPACE/DRIFT macro bundles.", range(1, 26)),
    ("Orbital Drift", "Slow-evolving orbit via LFO → global Quasar room/distance/delay.", range(26, 51)),
    ("Void Cathedrals", "Organ/additive hymn pads with deep void spatial scenes.", range(51, 66)),
    ("Echo Chambers", "Dual-layer stack showcases with Quasar delay feedback.", range(66, 76)),
]

MACRO_LAYOUTS = {
    "wide_nebula": {
        "names": ["BLOOM", "VOID", "DRIFT", "SPACE", "HAZE", "MORPH", "DEPTH", "AIR"],
        "koins": [
            (0, "BLOOM", "Per-note filter + WT spread; hold chord then re-sweep."),
            (3, "SPACE", "Opens Quasar mix, room size, distance, and delay tail on M3."),
            (2, "DRIFT", "Orbit room size, pan, WT bend, and delay diffusion."),
        ],
    },
    "orbital_drift": {
        "names": ["ORBIT", "VOID", "SPACE", "DRIFT", "BLOOM", "HAZE", "DEPTH", "AIR"],
        "koins": [
            (0, "ORBIT", "Quasar distance, room, mix, and layer pan orbit."),
            (2, "SPACE", "Master spatial wash — room size, decay, pre-delay, diffusion."),
            (1, "VOID", "Pulls CNTR back and opens void distance + filter darkness."),
        ],
    },
    "void_cathedral": {
        "names": ["VOID", "HYMN", "SPACE", "BLOOM", "DEPTH", "BODY", "AIR", "SHIMMER"],
        "koins": [
            (0, "VOID", "Cathedral void — room, distance, mix, filter darkness."),
            (1, "HYMN", "Organ vowel morph + level + filter warmth."),
            (2, "SPACE", "Quasar scene depth — mix, size, tail, delay."),
        ],
    },
    "echo_chamber": {
        "names": ["SPACE", "ECHO", "BLOOM", "MORPH", "DEPTH", "VOID", "BODY", "STACK"],
        "koins": [
            (0, "SPACE", "Quasar chamber — mix, room, delay feedback, distance."),
            (1, "ECHO", "Delay wash, diffusion, WT position, layer crossfade."),
            (2, "BLOOM", "Filter bloom + WT morph on held chord spread."),
        ],
    },
}


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


def global_route(source: int, dest: int, amount: float, slot: int = QUASAR_SLOT) -> dict:
    return {
        "source": source,
        "destination": dest,
        "targetIndex": slot,
        "amount": amount,
        "scope": SCOPE_GLOBAL,
    }


def binaural_space(variant: int, rng: random.Random) -> dict:
    a1, a2 = ANGLE_PAIRS[variant % len(ANGLE_PAIRS)]
    dist1 = 0.28 + (variant % 7) * 0.04 + rng.uniform(-0.03, 0.03)
    dist2 = dist1 + 0.04 + rng.uniform(0.0, 0.06)
    room1 = 0.95 + (variant % 5) * 0.08
    room2 = room1 + 0.05 + rng.uniform(-0.02, 0.04)
    delay_ms = 420.0 + variant * 18.0 + rng.uniform(-40, 80)
    return {
        "type": 11,
        "mix": 0.68 + (variant % 4) * 0.04,
        "qsr1Level": 0.62 + rng.uniform(-0.05, 0.08),
        "qsr2Level": 0.52 + rng.uniform(-0.05, 0.08),
        "cntrLevel": 0.82 + rng.uniform(-0.06, 0.06),
        "qsr1Height": -0.05 + (variant % 3) * 0.08,
        "qsr1Angle": a1 + rng.uniform(-4, 4),
        "qsr1Distance": min(0.72, dist1),
        "qsr1RoomAmount": 0.36 + (variant % 4) * 0.04,
        "qsr1RoomSize": room1,
        "qsr1RoomDamping": 0.48 + rng.uniform(-0.06, 0.06),
        "qsr2Height": 0.05 - (variant % 3) * 0.07,
        "qsr2Angle": a2 + rng.uniform(-4, 4),
        "qsr2Distance": min(0.78, dist2),
        "qsr2RoomAmount": 0.32 + (variant % 4) * 0.04,
        "qsr2RoomSize": room2,
        "qsr2RoomDamping": 0.44 + rng.uniform(-0.06, 0.06),
        "quasarDelayTimeMs": delay_ms,
        "quasarDelayFeedback": 0.28 + (variant % 5) * 0.04,
        "quasarDelayVolume": 0.18 + (variant % 3) * 0.04,
    }


def master_spatial_chain(f, variant: int, rng: random.Random) -> list:
    return [
        f.eq(lowDb=0.5, midDb=-1.0, highDb=-0.6),
        f.chorus(mix=0.18 + (variant % 3) * 0.03, rate=0.12, depth=4.0, base=14.0),
        binaural_space(variant, rng),
        f.comp(thresh=-16.0, ratio=2.2, atk=22.0, rel=220.0),
        f.limiter(),
    ]


def space_macro_routes(macro_idx: int = 3) -> list:
    src = SRC_MACRO1 + macro_idx
    return [
        global_route(src, DST_MASTER_FX_MIX, 0.35),
        global_route(src, DST_MASTER_REVERB_SIZE, 0.52),
        global_route(src, DST_MASTER_REVERB_DECAY, 1.15),
        global_route(src, DST_MASTER_REVERB_PREDELAY, 16.0),
        global_route(src, DST_MASTER_REVERB_DIFFUSION, 0.26),
    ]


def bloom_macro_routes(macro_idx: int, wt_op: int = 1) -> list:
    src = SRC_MACRO1 + macro_idx
    return [
        {"source": src, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 14.0, "scope": SCOPE_VOICE},
        {"source": src, "destination": DST_WT_POS, "targetIndex": wt_op, "amount": 0.42, "scope": 1},
        {"source": src, "destination": DST_FILTER_RES, "targetIndex": 0, "amount": 0.24, "scope": SCOPE_VOICE},
        {"source": src, "destination": DST_OP_LEVEL, "targetIndex": wt_op, "amount": 0.2, "scope": 1},
        {"source": src, "destination": DST_WT_FORMANT, "targetIndex": wt_op, "amount": 0.32, "scope": 1},
    ]


def drift_macro_routes(macro_idx: int, wt_op: int = 1) -> list:
    src = SRC_MACRO1 + macro_idx
    return [
        global_route(src, DST_MASTER_REVERB_SIZE, 0.38),
        global_route(src, DST_MASTER_REVERB_PREDELAY, 20.0),
        global_route(src, DST_MASTER_REVERB_DIFFUSION, 0.22),
        {"source": src, "destination": DST_PAN, "targetIndex": 0, "amount": 0.42, "scope": SCOPE_VOICE},
        {"source": src, "destination": DST_WT_BEND, "targetIndex": wt_op, "amount": 0.35, "scope": 1},
    ]


def void_macro_routes(macro_idx: int) -> list:
    src = SRC_MACRO1 + macro_idx
    return [
        global_route(src, DST_MASTER_FX_MIX, 0.32),
        global_route(src, DST_MASTER_REVERB_DECAY, 1.0),
        global_route(src, DST_MASTER_REVERB_PREDELAY, 24.0),
        {"source": src, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": -12.0, "scope": SCOPE_VOICE},
        global_route(src, DST_MASTER_REVERB_SIZE, 0.45),
    ]


def orbit_macro_routes(macro_idx: int, wt_op: int = 1) -> list:
    src = SRC_MACRO1 + macro_idx
    return [
        global_route(src, DST_MASTER_REVERB_PREDELAY, 28.0),
        global_route(src, DST_MASTER_REVERB_SIZE, 0.42),
        global_route(src, DST_MASTER_FX_MIX, 0.28),
        global_route(src, DST_MASTER_REVERB_DIFFUSION, 0.3),
        {"source": src, "destination": DST_PAN, "targetIndex": 0, "amount": 0.55, "scope": SCOPE_VOICE},
    ]


def hymn_macro_routes(macro_idx: int, wt_op: int = 1) -> list:
    src = SRC_MACRO1 + macro_idx
    return [
        {"source": src, "destination": DST_WT_FORMANT, "targetIndex": wt_op, "amount": 0.48, "scope": 1},
        {"source": src, "destination": DST_OP_LEVEL, "targetIndex": wt_op, "amount": 0.28, "scope": 1},
        {"source": src, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 10.0, "scope": SCOPE_VOICE},
        {"source": src, "destination": DST_WT_POS, "targetIndex": wt_op, "amount": 0.3, "scope": 1},
        {"source": src, "destination": DST_FILTER_RES, "targetIndex": 0, "amount": 0.18, "scope": SCOPE_VOICE},
    ]


def echo_macro_routes(macro_idx: int, wt_op: int = 1) -> list:
    src = SRC_MACRO1 + macro_idx
    return [
        global_route(src, DST_MASTER_REVERB_DIFFUSION, 0.38),
        global_route(src, DST_MASTER_REVERB_DECAY, 0.85),
        global_route(src, DST_MASTER_REVERB_PREDELAY, 14.0),
        {"source": src, "destination": DST_WT_POS, "targetIndex": wt_op, "amount": 0.36, "scope": 1},
        global_route(src, DST_MASTER_REVERB_SIZE, 0.28),
    ]


def spatial_lfo_routes(variant: int) -> list:
    """Slow global Quasar orbit — room size, distance, room amount, delay diffusion."""
    size_amt = 0.14 + (variant % 6) * 0.015
    predelay_amt = 10.0 + (variant % 5) * 2.5
    decay_amt = 0.55 + (variant % 4) * 0.08
    diff_amt = 0.1 + (variant % 3) * 0.025
    return [
        global_route(SRC_LFO1, DST_MASTER_REVERB_SIZE, size_amt),
        global_route(SRC_LFO1, DST_MASTER_REVERB_PREDELAY, predelay_amt),
        global_route(SRC_LFO2, DST_MASTER_REVERB_DECAY, decay_amt),
        global_route(SRC_LFO2, DST_MASTER_REVERB_DIFFUSION, diff_amt),
    ]


def slow_spatial_lfos(variant: int) -> tuple[dict, dict]:
    rate1 = 0.028 + (variant % 8) * 0.012  # 0.028–0.112 Hz
    rate2 = 0.04 + (variant % 6) * 0.018   # 0.04–0.13 Hz
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": min(rate1, 0.15), "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": min(rate2, 0.15), "syncDivisionIndex": 4,
            "phaseOffset": 0.25 + (variant % 5) * 0.12}
    return lfo1, lfo2


def append_unique_routes(base: list, extra: list) -> list:
    out = list(base)
    seen = {(r["source"], r["destination"], r.get("targetIndex", 0), r.get("scope", 0)) for r in out}
    for r in extra:
        key = (r["source"], r["destination"], r.get("targetIndex", 0), r.get("scope", 0))
        if key not in seen and len(out) < 64:
            out.append(r)
            seen.add(key)
    return out


def gen_wide_nebula(v, f, ist, rng):
    ops, amp, filt, lfo1, lfo2, routes, insert, _master, names, arp, extras = ist.gen_cosmic_pad(v, f, rng)
    lfo1, lfo2 = slow_spatial_lfos(v)
    routes = append_unique_routes(routes, spatial_lfo_routes(v))
    routes = append_unique_routes(routes, bloom_macro_routes(0, wt_op=1))
    routes = append_unique_routes(routes, drift_macro_routes(2, wt_op=1))
    routes = append_unique_routes(routes, space_macro_routes(3))
    routes = append_unique_routes(routes, void_macro_routes(1))
    master = master_spatial_chain(f, v, rng)
    macro_names = MACRO_LAYOUTS["wide_nebula"]["names"]
    return ops, amp, filt, lfo1, lfo2, routes, insert, master, macro_names, arp, extras


def gen_orbital_drift(v, f, ist, rng):
    ops, amp, filt, lfo1, lfo2, routes, insert, _master, names, arp, extras = ist.gen_texture(v, f, rng)
    lfo1, lfo2 = slow_spatial_lfos(v + 3)
    routes = append_unique_routes(routes, spatial_lfo_routes(v + 3))
    routes = append_unique_routes(routes, orbit_macro_routes(0, wt_op=1))
    routes = append_unique_routes(routes, space_macro_routes(2))
    routes = append_unique_routes(routes, void_macro_routes(1))
    routes = append_unique_routes(routes, bloom_macro_routes(4, wt_op=1))
    master = master_spatial_chain(f, v + 5, rng)
    macro_names = MACRO_LAYOUTS["orbital_drift"]["names"]
    return ops, amp, filt, lfo1, lfo2, routes, insert, master, macro_names, arp, extras


def gen_void_cathedral(v, f, ist, rng):
    ops, amp, filt, lfo1, lfo2, routes, insert, _master, names, arp, extras = ist.gen_cosmic_pad(v, f, rng)
    # Force organ/additive weight on even variants
    if v % 2 == 0 and len(ops) > 1:
        ops[1] = {
            "engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.84, "pan": 0.0,
            "additivePartialCount": 52, "additiveTilt": -0.22, "additiveOddEven": 0.78, "additiveStretch": 0.01,
        }
    lfo1, lfo2 = slow_spatial_lfos(v + 7)
    routes = append_unique_routes(routes, spatial_lfo_routes(v + 7))
    routes = append_unique_routes(routes, void_macro_routes(0))
    routes = append_unique_routes(routes, hymn_macro_routes(1, wt_op=1))
    routes = append_unique_routes(routes, space_macro_routes(2))
    routes = append_unique_routes(routes, bloom_macro_routes(3, wt_op=1))
    master = master_spatial_chain(f, v + 10, rng)
    # Larger rooms for cathedral
    quasar = master[2]
    quasar["qsr1RoomSize"] = 1.25 + v * 0.03
    quasar["qsr2RoomSize"] = 1.3 + v * 0.03
    quasar["qsr1RoomAmount"] = 0.48 + v * 0.015
    quasar["qsr2RoomAmount"] = 0.44 + v * 0.015
    macro_names = MACRO_LAYOUTS["void_cathedral"]["names"]
    extras = extras or {}
    extras["unison"] = {"mode": 1, "voices": 4}
    return ops, amp, filt, lfo1, lfo2, routes, insert, master, macro_names, arp, extras


def gen_echo_chamber(v, f, ist, rng):
    ops, amp, filt, lfo1, lfo2, routes, insert, _master, names, arp, extras = ist.gen_stack(v, f, rng)
    lfo1, lfo2 = slow_spatial_lfos(v + 11)
    routes = append_unique_routes(routes, spatial_lfo_routes(v + 11))
    routes = append_unique_routes(routes, space_macro_routes(0))
    routes = append_unique_routes(routes, echo_macro_routes(1, wt_op=1))
    routes = append_unique_routes(routes, bloom_macro_routes(2, wt_op=1))
    master = master_spatial_chain(f, v + 12, rng)
    quasar = master[2]
    quasar["quasarDelayFeedback"] = 0.38 + v * 0.04
    quasar["quasarDelayTimeMs"] = 580.0 + v * 45.0
    quasar["quasarDelayVolume"] = 0.26 + v * 0.02
    macro_names = MACRO_LAYOUTS["echo_chamber"]["names"]
    return ops, amp, filt, lfo1, lfo2, routes, insert, master, macro_names, arp, extras


GENERATORS = {
    "wide_nebula": gen_wide_nebula,
    "orbital_drift": gen_orbital_drift,
    "void_cathedral": gen_void_cathedral,
    "echo_chamber": gen_echo_chamber,
}


def apply_macro_descriptions(patch: dict, archetype: str) -> None:
    layout = MACRO_LAYOUTS[archetype]
    name_to_desc = {label: desc for _idx, label, desc in layout["koins"]}
    for m in patch.get("macros", []):
        label = m.get("name", "")
        if label in name_to_desc:
            m["description"] = name_to_desc[label]
        elif label == "SPACE":
            m["description"] = "Opens Quasar spatial mix, room size, distance, and delay on M3."


def apply_ui_focus(patch: dict, archetype: str) -> None:
    koins = MACRO_LAYOUTS[archetype]["koins"]
    patch["uiFocus"] = {
        "maxKnobs": len(koins),
        "knobs": [{"kind": "macro", "index": idx, "label": label} for idx, label, _desc in koins],
    }


def build_spatial_patch(f, ist, slot, display_name, archetype, variant, rng):
    gen = GENERATORS[archetype]
    result = gen(variant, f, ist, rng)
    arp = None
    extras = {}
    if len(result) == 9:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names = result
    elif len(result) == 10:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, arp = result
    else:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, arp, extras = result

    polyphony = max(extras.pop("polyphony", 16), 24)
    seed = hash((BATCH_TAG, archetype, slot, display_name)) & 0xFFFFFFFF
    engines_used = sorted({o["engine"] for o in ops})
    engine_names = ["Classic", "Wavetable", "FM/PM", "Additive", "PhaseShape",
                    "Granular", "NoiseChaos", "Resonator"]
    tip = PLAYING_TIPS[archetype]
    desc = (f"Interstellar spatial pad #{slot:03d} — {display_name}. "
            f"Headphone-first Quasar binaural scene on master M3. {tip} "
            f"Engines: {', '.join(engine_names[e] for e in engines_used)}.")

    tags = list(dict.fromkeys(
        ["interstellar", "spatial", "quasar", "pad", "binaural", "factory"] + ARCHETYPE_TAGS[archetype]
    ))
    moods = ["cinematic", "cosmic", "spatial", "vast", "headphone"]

    patch = f.build_patch(
        name=display_name.upper(),
        description=desc,
        category="pad",
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
    patch["metadata"]["category"] = "interstellar"
    patch["metadata"]["type"] = TYPE_TAG
    patch["metadata"]["author"] = AUTHOR
    patch["metadata"]["id"] = f"pw8-{BATCH_TAG}-{slot:03d}-{seed}"
    patch["metadata"]["masterFx"] = "quasar"
    patch["metadata"]["genres"] = ["cinematic", "score", "ambient", "spatial", "factory"]
    patch["metadata"]["createdAt"] = "2026-08-14T00:00:00Z"
    patch["metadata"]["performanceHints"] = [
        "headphone-first binaural wash",
        "CNTR holds bass — sweep SPACE for orbit",
        "slow LFO orbit on Quasar room/distance",
    ]

    depth = 0.22 + (variant % 14) * 0.01
    patch["voiceSettings"]["macroDissemination"] = True
    patch["voiceSettings"]["disseminationDepth"] = round(min(0.35, depth), 2)

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
    patch["voiceSettings"]["macroDissemination"] = True
    patch["voiceSettings"]["disseminationDepth"] = round(min(0.35, depth), 2)
    apply_macro_descriptions(patch, archetype)
    apply_ui_focus(patch, archetype)
    return patch


def write_readme():
    rows = []
    for slot, (name, arch, _) in enumerate(PRESET_CATALOG, start=1):
        koins = ", ".join(label for _i, label, _d in MACRO_LAYOUTS[arch]["koins"])
        rows.append(f"| {slot:03d} | `{slugify(name)}.pw8` | {arch.replace('_', ' ')} | {koins} | {name.upper()} |")

    theme_sections = []
    for title, blurb, slots in THEME_GROUPS:
        theme_sections.append(f"### {title}\n\n{blurb}\n")
        sub = [r for i, r in enumerate(rows, start=1) if i in slots]
        theme_sections.append("| # | File | Archetype | Feature KOINS | Name |\n|---:|------|-----------|---------------|------|\n")
        theme_sections.extend(line + "\n" for line in sub)
        theme_sections.append("\n")

    readme = f"""# Interstellar Spatial Pad Bank ({COUNT} presets)

Headphone-first **Quasar** (`BinauralSpace`) pads — slow-evolving binaural orbit, expressive macro KOINS,
and macro dissemination on held chords. Master FX slot **M3** is always QUASAR.

**Showcase references:** `Interstellar/001-cathedral-nebula`, `004-tesseract-bloom`, `092-echo-chamber`.

## Playing

1. Use **headphones** — ITD/ILD spatial mix is the point.
2. Hold C3–G4 chords (or single notes for orbital drift archetypes).
3. Sweep **SPACE**, **BLOOM**, **DRIFT**, **ORBIT**, or **VOID** — each bundles 3–5 mod routes.
4. Slow LFOs continuously orbit Quasar room size, distance, and delay diffusion.

## Archetypes

| Archetype | Count |
|-----------|------:|
| Wide nebula pads | 25 |
| Orbital drift | 25 |
| Void cathedral | 15 |
| Echo chamber (stack) | 10 |

## Theme groups

{''.join(theme_sections)}

## Regenerate

```bash
python3 scripts/generate_interstellar_spatial_presets.py
```

Authored by {AUTHOR}. Batch tag: `{BATCH_TAG}`.
"""
    (OUT_DIR / "README.md").write_text(readme)


def update_manifest(new_entries: list[dict]) -> None:
    manifest = []
    if MANIFEST_PATH.exists():
        manifest = json.loads(MANIFEST_PATH.read_text())
    manifest = [e for e in manifest if "factory/Interstellar/Spatial/" not in e.get("file", "")]
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
        patch = build_spatial_patch(f, ist, slot, display_name, archetype, variant, seed_rng)
        fname = f"{slot:03d}-{slugify(display_name)}.pw8"
        path = OUT_DIR / fname
        path.write_text(json.dumps(patch, indent=2) + "\n")
        manifest_entries.append({
            "category": "Interstellar",
            "file": f"factory/Interstellar/Spatial/{fname}",
            "name": display_name.upper(),
        })

    write_readme()
    update_manifest(manifest_entries)
    print(f"Generated {len(manifest_entries)} Interstellar spatial presets in {OUT_DIR}")


if __name__ == "__main__":
    main()
