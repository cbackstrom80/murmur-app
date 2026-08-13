#!/usr/bin/env python3
"""Generate 100 cinematic Interstellar-inspired factory presets.

Curated archetypes with musical variation — not random parameter soup.
Output: content/presets/factory/Interstellar/01-*.pw8 … 100-*.pw8 + README.md

Run from repo root:
    python3 scripts/generate_interstellar_presets.py
"""
from __future__ import annotations

import importlib.util
import json
import pathlib
import random

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "content" / "presets" / "factory" / "Interstellar"
MANIFEST_PATH = REPO_ROOT / "content" / "presets" / "factory" / "MANIFEST.json"
AUTHOR = "MURMUR Sound Design"
BATCH_TAG = "interstellar"
COUNT = 100

# ModDestination ordinals — must match ModMatrixTypes.hpp
DST_FILTER_CUTOFF = 1
DST_FILTER_RES = 2
DST_OP_LEVEL = 5
DST_PAN = 6
DST_WT_POS = 7
DST_WT_BEND = 8
DST_WT_ASYM = 9
DST_WT_SYNC = 10
DST_WT_FORMANT = 11

SRC_LFO1, SRC_LFO2 = 1, 2
SRC_ENV1 = 9
SRC_VELOCITY = 17
SRC_CHANNEL_PRESSURE = 18
SRC_MACRO1 = 21
SRC_MOD_WHEEL = 29
SRC_EXPRESSION = 30

HIGHLIGHT_TOP10 = [
    (1, "Cathedral Nebula", "Hold C3–C4; mod wheel blooms filter + WT morph. Best with long reverb tail."),
    (2, "Dust Lane Hymn", "Legato chords in low-mid register; pressure swells level and space."),
    (3, "Organ Of Void", "Pipe-organ weight — play octaves C2–C3; macro MORPH opens vowel formant."),
    (16, "Black Hole Gravity", "Sub register C1–C2; mod wheel adds harmonics without mud."),
    (38, "Cornfield Chase", "Lead lines G3–B4; portamento on legato; tape delay carries melody."),
    (28, "Ticking Eternity", "Enable arp latch; short staccato in mid register for clock tension."),
    (48, "Deep Space Drift", "Single held note anywhere — granular cloud evolves over 10s."),
    (58, "Wormhole Rise", "FX: sweep mod wheel up over 4 bars for riser; release for impact tail."),
    (68, "Exoplanet Keys", "Crystalline plucks C4–C5; velocity opens brightness."),
    (86, "Dual Cathedral", "Stack mode — macro MORPH balances organ + nebula layers."),
]

# (display_name, archetype_key, variant_index)
PRESET_CATALOG = [
    # 15 cosmic pads
    ("Cathedral Nebula", "cosmic_pad", 0),
    ("Dust Lane Hymn", "cosmic_pad", 1),
    ("Organ Of Void", "cosmic_pad", 2),
    ("Tesseract Bloom", "cosmic_pad", 3),
    ("Solar Wind Veil", "cosmic_pad", 4),
    ("Event Horizon Glow", "cosmic_pad", 5),
    ("Pillars Of Creation", "cosmic_pad", 6),
    ("Cosmic Lullaby", "cosmic_pad", 7),
    ("Wormhole Cathedral", "cosmic_pad", 8),
    ("Amber Expanse", "cosmic_pad", 9),
    ("Stellar Nursery", "cosmic_pad", 10),
    ("Deep Field Drift", "cosmic_pad", 11),
    ("Gravity Well Pad", "cosmic_pad", 12),
    ("Interstellar Hymn", "cosmic_pad", 13),
    ("Pale Blue Dot", "cosmic_pad", 14),
    # 12 massive bass
    ("Black Hole Gravity", "massive_bass", 0),
    ("Tectonic Sub", "massive_bass", 1),
    ("Singularity Pulse", "massive_bass", 2),
    ("Dark Matter Low", "massive_bass", 3),
    ("Orbital Decay", "massive_bass", 4),
    ("Collapsar Drone", "massive_bass", 5),
    ("Accretion Disk", "massive_bass", 6),
    ("Spaghettify", "massive_bass", 7),
    ("Penrose Bass", "massive_bass", 8),
    ("Void Anchor", "massive_bass", 9),
    ("Tidal Lock", "massive_bass", 10),
    ("Gravitational Mass", "massive_bass", 11),
    # 10 pulsar / clock
    ("Ticking Eternity", "pulsar", 0),
    ("Metronome Cosmos", "pulsar", 1),
    ("Pulsar Grid", "pulsar", 2),
    ("Clockwork Orbit", "pulsar", 3),
    ("Relativistic Tick", "pulsar", 4),
    ("Lighthouse Pulse", "pulsar", 5),
    ("Chronos Gate", "pulsar", 6),
    ("Time Dilation", "pulsar", 7),
    ("Beacon Rhythm", "pulsar", 8),
    ("Orbital Clock", "pulsar", 9),
    # 10 leads
    ("Cornfield Chase", "lead", 0),
    ("No Time For Caution", "lead", 1),
    ("First Step", "lead", 2),
    ("Docking Scene", "lead", 3),
    ("Ranger Theme", "lead", 4),
    ("Crystalline Signal", "lead", 5),
    ("Solar Sail", "lead", 6),
    ("Proxima Line", "lead", 7),
    ("Contact Tone", "lead", 8),
    ("Golden Record", "lead", 9),
    # 10 textures / drones
    ("Deep Space Drift", "texture", 0),
    ("Cosmic Microwave", "texture", 1),
    ("Void Whisper", "texture", 2),
    ("Stellar Wind", "texture", 3),
    ("Oort Cloud", "texture", 4),
    ("Redshift Drone", "texture", 5),
    ("Magnetosphere", "texture", 6),
    ("Heliopause", "texture", 7),
    ("Quasar Hum", "texture", 8),
    ("Dark Flow", "texture", 9),
    # 10 FX / transitions
    ("Wormhole Rise", "fx", 0),
    ("Hyperdrive Sweep", "fx", 1),
    ("Docking Impact", "fx", 2),
    ("Airlock Burst", "fx", 3),
    ("Reentry Flame", "fx", 4),
    ("Warp Jump", "fx", 5),
    ("Event Horizon Drop", "fx", 6),
    ("Supernova Hit", "fx", 7),
    ("Gravity Slingshot", "fx", 8),
    ("Countdown Zero", "fx", 9),
    # 10 keys / plucks
    ("Exoplanet Keys", "keys", 0),
    ("Cryo Pluck", "keys", 1),
    ("Ice Moon Arp", "keys", 2),
    ("Europa Glass", "keys", 3),
    ("Titan Atmosphere", "keys", 4),
    ("Enceladus Bell", "keys", 5),
    ("Mars Horizon", "keys", 6),
    ("Ceres Crystal", "keys", 7),
    ("Io Spark", "keys", 8),
    ("Ganymede Tide", "keys", 9),
    # 8 bells / mallets
    ("Orbital Chime", "bell", 0),
    ("Debris Strike", "bell", 1),
    ("Satellite Bell", "bell", 2),
    ("Comet Tail", "bell", 3),
    ("Ring Particle", "bell", 4),
    ("Meteor Shower", "bell", 5),
    ("Aurora Mallet", "bell", 6),
    ("Ionosphere Ring", "bell", 7),
    # 10 stack / layer showcases (Layer B + stack mode)
    ("Dual Cathedral", "stack", 0),
    ("Stack Of Stars", "stack", 1),
    ("Layered Cosmos", "stack", 2),
    ("Binary Sunset", "stack", 3),
    ("Twin Nebula", "stack", 4),
    ("Parallel Universe", "stack", 5),
    ("Echo Chamber", "stack", 6),
    ("Morphic Stack", "stack", 7),
    ("Cosmic Stack", "stack", 8),
    ("Nebula Layers", "stack", 9),
    # 5 wildcard experimental
    ("Quasar Glitch", "wildcard", 0),
    ("Dark Matter Chaos", "wildcard", 1),
    ("Quantum Foam", "wildcard", 2),
    ("Hawking Radiation", "wildcard", 4),
    ("Entropy Bloom", "wildcard", 6),
]

assert len(PRESET_CATALOG) == COUNT

# Ten research-themed groups (10 patches each) for README documentation.
THEME_CATEGORIES = [
    ("Nebulae & Cosmic Cathedrals", "Emission/reflection nebulae, organ-weight pads, ionized gas swells.", range(1, 11)),
    ("Deep Field & Gravity Wells", "Hubble deep-field drift, gravity-well pads, pale-blue perspective.", range(11, 21)),
    ("Black Holes & Accretion", "Event horizon bass, singularity pulses, Penrose process low end.", range(21, 31)),
    ("Pulsars & Time Dilation", "Millisecond pulsar grids, lighthouse beacons, relativistic clock rhythms.", range(31, 41)),
    ("First Contact Leads", "Cornfield chase energy, ranger themes, Contact/Golden Record melodic lines.", range(41, 51)),
    ("Interstellar Medium", "CMB hum, heliopause, magnetosphere, quasar drone textures.", range(51, 61)),
    ("Spacecraft & Transitions", "Wormhole risers, hyperdrive sweeps, docking/airlock FX.", range(61, 71)),
    ("Exoplanets & Ice Moons", "Cryo plucks, Europa glass, Titan atmosphere keys.", range(71, 81)),
    ("Orbital Bells & Rings", "Satellite chimes, comet tails, ring-particle mallets.", range(81, 91)),
    ("Spacetime & Quantum Edge", "Dual-layer stacks, multiverse folds, Hawking radiation wildcards.", range(91, 101)),
]

ARCHETYPE_INTERNAL_CAT = {
    "cosmic_pad": "pad",
    "massive_bass": "bass",
    "pulsar": "seq",
    "lead": "lead",
    "texture": "ambient",
    "fx": "ambient",
    "keys": "seq",
    "bell": "seq",
    "stack": "pad",
    "wildcard": "ambient",
}

ARCHETYPE_TAGS = {
    "cosmic_pad": ["cosmic-pad", "hymn-swell", "cinematic"],
    "massive_bass": ["sub-gravity", "fm-sub", "cinematic"],
    "pulsar": ["clock-tick", "pulsar", "cinematic"],
    "lead": ["singable-lead", "portamento", "cinematic"],
    "texture": ["gran-cloud", "evolving-drone", "cinematic"],
    "fx": ["fx-riser", "transition", "cinematic"],
    "keys": ["crystalline-keys", "pluck", "cinematic"],
    "bell": ["fm-bell", "orbital", "cinematic"],
    "stack": ["dual-layer", "stack-showcase", "cinematic"],
    "wildcard": ["experimental", "glitch", "cinematic"],
}

PLAYING_TIPS = {
    "cosmic_pad": "Hold mid register C3–C5; mod wheel blooms filter and wavetable; pressure adds shimmer.",
    "massive_bass": "Play C1–C2; mod wheel opens harmonics; keep velocity moderate for clean sub.",
    "pulsar": "Short notes or arp; mod wheel sharpens sync warp; works at 60–120 BPM.",
    "lead": "Melodic lines G3–B4; legato portamento; mod wheel adds bite and filter sweep.",
    "texture": "Hold any note 5+ seconds; mod wheel morphs granular density; minimal movement.",
    "fx": "Long notes or single triggers; sweep mod wheel for riser/drop; use as transition layer.",
    "keys": "Staccato C4–C5; velocity controls brightness; mod wheel adds space.",
    "bell": "Single strikes C5–C7; chorus widens; mod wheel shifts formant metallic tone.",
    "stack": "Chords in mid register; macro MORPH crossfades dual layers; mod wheel = bloom.",
    "wildcard": "Explore extremes; mod wheel is essential; still musical at moderate settings.",
}


def load_factory():
    path = REPO_ROOT / "scripts" / "generate_factory_presets.py"
    spec = importlib.util.spec_from_file_location("factory_presets", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def filter2_warm(cutoff=3200.0, resonance=0.22, drive=0.12):
    return {"enabled": True, "cutoffHz": cutoff, "resonance": resonance, "drive": drive, "keyTrack": 0.25}


def filter2_dark(cutoff=1800.0, resonance=0.35, drive=0.28):
    return {"enabled": True, "cutoffHz": cutoff, "resonance": resonance, "drive": drive, "keyTrack": 0.15}


# Algorithm graph topologies — 20 presets each (slots 1–100)
GRAPH_KINDS = ("parallel", "serial", "fm_pm", "feedback", "multi_out")


def padded_ops(ops):
    out = list(ops)
    while len(out) < 8:
        out.append({"engine": 0, "classicWaveform": 0, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.0, "pan": 0.0})
    return out[:8]


def algo_nodes_from_ops(ops):
    return [{"id": i, "engine": padded_ops(ops)[i]["engine"], "isOutput": False} for i in range(8)]


def build_algorithm_graph(kind: str, ops):
    nodes = algo_nodes_from_ops(ops)
    if kind == "parallel":
        for n in nodes:
            n["isOutput"] = True
        edges = []
    elif kind == "serial":
        for i, n in enumerate(nodes):
            n["isOutput"] = i == 0
        edges = [{"source": s, "destination": d, "type": 1, "amount": max(0.45, 0.92 - s * 0.05)}
                 for s, d in ((7, 6), (6, 5), (5, 4), (4, 3), (3, 2), (2, 1), (1, 0))]
    elif kind == "fm_pm":
        for i, n in enumerate(nodes):
            n["isOutput"] = i in (0, 1, 2, 6, 7)
        edges = [
            {"source": 3, "destination": 0, "type": 1, "amount": 0.68},
            {"source": 4, "destination": 1, "type": 1, "amount": 0.62},
            {"source": 5, "destination": 2, "type": 1, "amount": 0.58},
        ]
    elif kind == "feedback":
        for i, n in enumerate(nodes):
            n["isOutput"] = i == 0
        edges = [
            {"source": 2, "destination": 1, "type": 1, "amount": 0.78},
            {"source": 1, "destination": 1, "type": 6, "amount": 0.58},
            {"source": 1, "destination": 0, "type": 1, "amount": 1.0},
        ]
    else:
        for i, n in enumerate(nodes):
            n["isOutput"] = i in (0, 1, 2, 3)
        edges = [
            {"source": 4, "destination": 0, "type": 1, "amount": 0.52},
            {"source": 5, "destination": 1, "type": 1, "amount": 0.48},
            {"source": 6, "destination": 2, "type": 0, "amount": 0.38},
            {"source": 7, "destination": 3, "type": 1, "amount": 0.42},
        ]
    return {"nodes": nodes, "edges": edges}


def graph_kind_for_slot(slot: int) -> str:
    return GRAPH_KINDS[(slot - 1) // 20]


def mw_bloom_routes(wt_op=1, wt_amount=0.4, cutoff_amount=22.0):
    return [
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0,
         "amount": cutoff_amount, "scope": 0},
        {"source": SRC_MOD_WHEEL, "destination": DST_WT_POS, "targetIndex": wt_op,
         "amount": wt_amount, "scope": 1},
        {"source": SRC_MACRO1, "destination": DST_WT_POS, "targetIndex": wt_op,
         "amount": wt_amount * 0.85, "scope": 1},
    ]


def master_cinematic_pad(f):
    return [
        f.eq(lowDb=0.6, midDb=-1.2, highDb=-0.8),
        f.chorus(mix=0.22, rate=0.18, depth=4.5, base=14.0),
        f.reverb(mix=0.52, size=2.6, decay=9.5, predelay=42.0),
        f.comp(thresh=-16.0, ratio=2.4, atk=20.0, rel=200.0),
        f.limiter(),
    ]


def master_cinematic_lead(f):
    return [
        f.eq(highDb=1.2),
        f.reverb(mix=0.22, size=1.6, decay=3.2),
        f.comp(thresh=-14.0, ratio=3.0),
        f.limiter(),
    ]


def master_cinematic_bass(f):
    return [
        f.eq(lowDb=1.5, midDb=-1.0, highDb=-2.5),
        f.comp(thresh=-12.0, ratio=3.5, atk=8.0, rel=120.0),
        f.limiter(ceiling=-1.0),
    ]


def master_cinematic_ambient(f, long_reverb=True):
    rev = f.reverb(mix=0.48 if long_reverb else 0.32, size=2.8 if long_reverb else 1.8,
                   decay=11.0 if long_reverb else 5.0, predelay=55.0 if long_reverb else 20.0)
    return [f.eq(lowDb=0.3, midDb=-1.5, highDb=-1.0), rev, f.comp(thresh=-18.0, ratio=2.0), f.limiter()]


def active_layer_b(f, ops, amp_env, filter1=None, gain=0.82, insert=None):
    layer = f.layer_b()
    layer["operators"] = f.op_pad(ops)
    layer["envelopes"] = f.envelopes_from_amp(amp_env)
    layer["gain"] = gain
    layer["pan"] = 0.0
    layer["width"] = 1.0
    if filter1:
        layer["filter1"] = filter1
    if insert:
        layer["insertEffects"] = insert
    return layer


def gen_cosmic_pad(v, f, rng):
    profiles = [
        ("organ", "additive", "formant-vowel-aa.json", 0.15),
        ("hymn", "wt", "ambient-evolving-swell.json", 0.25),
        ("nebula", "wt", "formant-vowel-oo.json", 0.35, 0.25),
        ("bloom", "additive", "chord-fifth-stack.json", 0.08),
        ("veil", "wt", "ambient-dreamy-veil.json", 0.4),
        ("glow", "reso", None, 0.2),
        ("cathedral", "additive", "formant-vowel-morph-a-to-e.json", 0.05),
        ("lullaby", "wt", "classic-sine-to-triangle.json", 0.3),
        ("wormhole", "wt", "formant-vowel-morph-o-to-u.json", 0.45, 0.35),
        ("amber", "wt", "ambient-airy-drift.json", 0.22),
        ("nursery", "gran", "gran-ocean-swell.json", 0.18),
        ("deepfield", "wt", "texture-drone-evolve.json", 0.5),
        ("gravity", "additive", "chord-octave-stack.json", -0.15),
        ("hymn2", "wt", "formant-vowel-oh.json", 0.28, 0.18),
        ("pale", "wt", "ambient-evolving-swell.json", 0.12),
    ]
    prof = profiles[v % len(profiles)]
    name_key = prof[0]
    body = prof[1]
    wt_file = prof[2]
    wt_pos = prof[3] if len(prof) > 3 else 0.2
    formant = prof[4] if len(prof) > 4 else 0.0

    ops = [{"engine": 0, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.38, "pan": 0.0}]
    wt_op = 1

    if body == "additive":
        partials = 48 if name_key == "organ" else 36
        odd_even = 0.78 if name_key in ("organ", "cathedral") else 0.55
        ops.append({"engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.82, "pan": 0.0,
                    "additivePartialCount": partials, "additiveTilt": -0.25 + v * 0.01,
                    "additiveOddEven": odd_even, "additiveStretch": 0.012})
    elif body == "wt":
        op = {"engine": 1, "wavetableId": f"content/wavetables/{wt_file}",
              "wavetableFramePosition": wt_pos, "frequencyRatio": 1.0,
              "keyTrack": True, "level": 0.78, "pan": rng.uniform(-0.18, 0.18)}
        if formant > 0:
            op["wtFormantShift"] = formant
        if v in (8, 13):
            op["wtBend"] = 0.22 + v * 0.02
        ops.append(op)
    elif body == "reso":
        ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.68, "pan": 0.0,
                    "resonatorStructure": 0.42, "resonatorDecay": 0.78, "resonatorDamping": 0.38,
                    "resonatorBrightness": 0.52, "resonatorModeCount": 7})
    else:
        ops.append({"engine": 5, "wavetableId": f"content/wavetables/{wt_file}",
                    "wavetableFramePosition": 0.25, "frequencyRatio": 1.0, "keyTrack": True,
                    "level": 0.55, "pan": 0.2, "grainDensity": 10.0 + v, "grainSizeMs": 140.0,
                    "grainPositionJitter": 0.28, "grainPitchJitter": 0.06})

    if v % 2 == 0:
        ops.append({"engine": 4, "frequencyRatio": 1.002, "keyTrack": True, "level": 0.28,
                    "pan": 0.25, "phaseBend": 0.12, "phaseFold": 0.22, "phaseAsymmetry": 0.05,
                    "phaseShape": 0.45})
    if v % 3 == 1:
        ops.append({"engine": 2, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 2.0,
                    "keyTrack": True, "level": 0.18, "pan": -0.2,
                    "fmModulatorRatio": 3.0, "fmModulatorIndex": 0.28,
                    "fmModulatorFeedback": 0.06, "fmModulatorWaveform": f.WAVEFORM_SINE})

    cutoff = 1600.0 + v * 85.0
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": 0.1, "keyTrack": 0.22}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.045 + v * 0.002, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.028, "syncDivisionIndex": 4, "phaseOffset": 0.4}
    attack = 2.2 + v * 0.12
    amp_env = {"attackSeconds": attack, "decaySeconds": 1.6, "sustainLevel": 0.86,
               "releaseSeconds": min(4.0, 3.8 + v * 0.08), "curveShape": 2.15, "legato": True}
    mod_routes = mw_bloom_routes(wt_op=wt_op, wt_amount=0.38 + v * 0.015, cutoff_amount=20.0)
    mod_routes.extend([
        {"source": SRC_LFO1, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 6.5, "scope": 1},
        {"source": SRC_LFO2, "destination": DST_WT_POS, "targetIndex": wt_op, "amount": 0.32, "scope": 1},
        {"source": SRC_CHANNEL_PRESSURE, "destination": DST_OP_LEVEL, "targetIndex": 0, "amount": 0.22, "scope": 0},
    ])
    insert = [f.tape_delay(mix=0.18, ms=480.0, fb=0.28, driftDepth=8.0)]
    master = master_cinematic_pad(f)
    unison = {"mode": 1, "voices": 4} if v in (0, 6, 13) else {"mode": 1, "voices": 3}
    extras = {"filter2": filter2_warm(cutoff=2800 + v * 60), "unison": unison, "polyphony": 24}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["BLOOM", "WARMTH", "MORPH", "SPACE", "BODY", "SHIMMER", "AIR", "DEPTH"], None, extras


def gen_massive_bass(v, f, rng):
    styles = ["fm", "classic", "wt", "fm", "classic", "wt", "fm", "reso", "fm", "classic", "wt", "fm"]
    style = styles[v]
    ops = [{"engine": 0, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.55 + v * 0.02, "pan": 0.0}]
    if style == "fm":
        ops.append({"engine": 2, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.88, "pan": 0.0,
                    "fmModulatorRatio": [0.5, 1.0, 2.0, 0.5][v % 4],
                    "fmModulatorIndex": 0.35 + v * 0.04,
                    "fmModulatorFeedback": 0.12, "fmModulatorWaveform": f.WAVEFORM_SINE})
    elif style == "wt":
        op = {"engine": 1, "wavetableId": f.wt_path("bass", rng),
              "wavetableFramePosition": 0.15 + v * 0.04, "frequencyRatio": 1.0,
              "keyTrack": True, "level": 0.85, "pan": 0.0}
        if v % 2 == 0:
            op["wtSyncRatio"] = 1.5 + v * 0.1
            op["wtSyncAmount"] = 0.35
        ops.append(op)
    elif style == "reso":
        ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.82, "pan": 0.0,
                    "resonatorStructure": 0.25, "resonatorDecay": 0.55, "resonatorDamping": 0.55,
                    "resonatorBrightness": 0.35, "resonatorModeCount": 5})
    else:
        wf = f.WAVEFORM_SAW if v % 2 == 0 else f.WAVEFORM_SQR
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.78, "pan": 0.0})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.003,
                    "keyTrack": True, "level": 0.35, "pan": 0.12})

    cutoff = 120.0 + v * 35.0
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": 0.18 + v * 0.02, "keyTrack": 0.35}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.08 + v * 0.015, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.5, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    amp_env = {"attackSeconds": 0.004, "decaySeconds": 0.25, "sustainLevel": 0.82,
               "releaseSeconds": 0.35, "curveShape": 1.6}
    mod_routes = [
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 28.0, "scope": 0},
        {"source": SRC_VELOCITY, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 14.0, "scope": 0},
        {"source": SRC_MACRO1, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 14.0, "scope": 0},
    ]
    if style == "wt":
        mod_routes.append({"source": SRC_MOD_WHEEL, "destination": DST_WT_SYNC, "targetIndex": 1,
                           "amount": 2.5, "scope": 1})
    insert = [f.saturation(mix=0.28, drive=8.0 + v)]
    master = master_cinematic_bass(f)
    extras = {"filter2": filter2_dark(cutoff=900 + v * 40, drive=0.22 + v * 0.02), "polyphony": 8}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["GRAVITY", "PUNCH", "SUB", "GRIT", "MASS", "DRIVE", "DEPTH", "SYNC"], None, extras


def gen_pulsar(v, f, rng):
    ops = [{"engine": 0, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.35, "pan": 0.0}]
    op = {"engine": 1, "wavetableId": f.wt_path("seq", rng),
          "wavetableFramePosition": 0.35, "frequencyRatio": 1.0,
          "keyTrack": True, "level": 0.82, "pan": 0.0,
          "wtSyncRatio": 2.0 + v * 0.15, "wtSyncAmount": 0.42 + v * 0.03}
    ops.append(op)
    if v % 2 == 0:
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": 0.06,
                    "pan": 0.0, "noiseVariant": 6, "noiseRate": 80.0 + v * 12.0})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 2200.0 + v * 150, "resonance": 0.32, "keyTrack": 0.4}
    lfo1 = {"waveform": 3, "mode": 1, "rateHz": 2.0, "syncDivisionIndex": 5 + (v % 3), "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.2, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    amp_env = {"attackSeconds": 0.001, "decaySeconds": 0.12 + v * 0.01, "sustainLevel": 0.08,
               "releaseSeconds": 0.08, "curveShape": 1.4}
    mod_routes = [
        {"source": SRC_MOD_WHEEL, "destination": DST_WT_SYNC, "targetIndex": 1, "amount": 3.0, "scope": 1},
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 18.0, "scope": 0},
        {"source": SRC_LFO1, "destination": DST_OP_LEVEL, "targetIndex": 1, "amount": 0.55, "scope": 1},
    ]
    insert = [f.tape_delay(mix=0.25, ms=180.0, fb=0.35)]
    master = [f.eq(highDb=0.8), f.reverb(mix=0.18, size=1.2, decay=2.0), f.comp(ratio=3.5), f.limiter()]
    arp = {"enabled": True, "mode": v % 5, "rateMode": 1, "rateHz": 8.0,
           "syncDivisionIndex": [4, 5, 6, 5, 7][v % 5], "octaveRange": 1 + v % 2,
           "numSteps": [8, 12, 16, 8, 12][v % 5], "latch": v in (0, 5, 9)}
    extras = {"filter2": filter2_warm(cutoff=3500, drive=0.08), "polyphony": 12}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["TICK", "SYNC", "PULSE", "GATE", "SPACE", "RATE", "BITE", "CLOCK"], arp, extras


def gen_lead(v, f, rng):
    kinds = ["sync", "fm", "wt", "classic", "phase", "sync", "fm", "wt", "classic", "phase"]
    kind = kinds[v]
    ops = []
    if kind == "sync":
        op = {"engine": 1, "wavetableId": f.wt_path("lead", rng),
              "wavetableFramePosition": 0.4, "frequencyRatio": 1.0, "keyTrack": True,
              "level": 0.9, "pan": 0.0, "wtSyncRatio": 2.5, "wtSyncAmount": 0.55}
        ops.append(op)
    elif kind == "fm":
        ops.append({"engine": 2, "classicWaveform": f.WAVEFORM_SAW, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.88, "pan": 0.0,
                    "fmModulatorRatio": 2.0, "fmModulatorIndex": 0.55, "fmModulatorFeedback": 0.2,
                    "fmModulatorWaveform": f.WAVEFORM_SINE})
    elif kind == "wt":
        ops.append({"engine": 1, "wavetableId": f.wt_path("lead", rng),
                    "wavetableFramePosition": 0.55, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.88, "pan": 0.0, "wtBend": 0.25})
    elif kind == "phase":
        ops.append({"engine": 4, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.88, "pan": 0.0,
                    "phaseBend": 0.35, "phaseFold": 0.4, "phaseAsymmetry": 0.1, "phaseShape": 0.6})
    else:
        wf = f.WAVEFORM_SAW
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.85, "pan": 0.0})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.006, "keyTrack": True, "level": 0.5, "pan": 0.22})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 2800.0 + v * 200, "resonance": 0.25, "keyTrack": 0.5}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 4.8, "syncDivisionIndex": 6, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.15, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    amp_env = {"attackSeconds": 0.012 if v != 7 else 0.04, "decaySeconds": 0.28,
               "sustainLevel": 0.78, "releaseSeconds": 0.42, "curveShape": 1.85}
    mod_routes = [
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 30.0, "scope": 0},
        {"source": SRC_VELOCITY, "destination": DST_OP_LEVEL, "targetIndex": 0, "amount": 0.4, "scope": 0},
        {"source": SRC_EXPRESSION, "destination": DST_FILTER_RES, "targetIndex": 0, "amount": 0.3, "scope": 0},
    ]
    insert = [f.tape_delay(mix=0.22, ms=340.0, fb=0.32)]
    master = master_cinematic_lead(f)
    extras = {"filter2": filter2_warm(cutoff=4200, drive=0.15), "portamento": 0.08 + v * 0.008, "polyphony": 8}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["EDGE", "BLOOM", "SWEEP", "DELAY", "BITE", "AIR", "MOTION", "SOLO"], None, extras


def gen_texture(v, f, rng):
    ops = [{"engine": 0, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.32, "pan": 0.0}]
    ops.append({"engine": 5, "wavetableId": f.wt_path("gran", rng),
                "wavetableFramePosition": 0.2 + v * 0.04, "frequencyRatio": 1.0, "keyTrack": True,
                "level": 0.52, "pan": rng.uniform(-0.35, 0.35),
                "grainDensity": 8.0 + v * 1.2, "grainSizeMs": 110.0 + v * 8,
                "grainPositionJitter": 0.32, "grainPitchJitter": 0.05})
    ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.48, "pan": 0.0,
                "resonatorStructure": 0.38, "resonatorDecay": 0.82, "resonatorDamping": 0.42,
                "resonatorBrightness": 0.48, "resonatorModeCount": 6})
    if v % 2 == 0:
        ops.append({"engine": 1, "wavetableId": f.wt_path("ambient", rng),
                    "wavetableFramePosition": 0.45, "frequencyRatio": 0.5, "keyTrack": True,
                    "level": 0.38, "pan": -0.15, "wtBend": 0.18})
    if v % 3 == 0:
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": 0.05,
                    "pan": 0.0, "noiseVariant": 1, "noiseRate": 25.0})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1500.0 + v * 90, "resonance": 0.08, "keyTrack": 0.12}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.035, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.018, "syncDivisionIndex": 4, "phaseOffset": 0.55}
    amp_env = {"attackSeconds": 3.5 + v * 0.2, "decaySeconds": 2.0, "sustainLevel": 0.88,
               "releaseSeconds": 6.0 + v * 0.15, "curveShape": 2.3}
    mod_routes = mw_bloom_routes(wt_op=3 if v % 2 == 0 else 1, wt_amount=0.35, cutoff_amount=14.0)
    mod_routes.append({"source": SRC_LFO2, "destination": DST_WT_POS, "targetIndex": 1, "amount": 0.45, "scope": 1})
    insert = [f.tape_delay(mix=0.15, ms=520.0, fb=0.25)]
    master = master_cinematic_ambient(f)
    extras = {"filter2": filter2_dark(cutoff=2400, drive=0.1), "polyphony": 16}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["DRIFT", "GRAIN", "DEPTH", "AIR", "SHIMMER", "VOID", "MORPH", "SPACE"], None, extras


def gen_fx(v, f, rng):
    ops = [{"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": 0.15 + v * 0.01,
            "pan": 0.0, "noiseVariant": v % 4, "noiseRate": 40.0 + v * 5.0}]
    ops.append({"engine": 1, "wavetableId": f.wt_path("ambient", rng),
                "wavetableFramePosition": 0.1, "frequencyRatio": 1.0, "keyTrack": True,
                "level": 0.62, "pan": 0.0, "wtBend": 0.55 + v * 0.03})
    if v >= 5:
        ops.append({"engine": 4, "frequencyRatio": 2.0, "keyTrack": True, "level": 0.35, "pan": 0.0,
                    "phaseBend": 0.5, "phaseFold": 0.65, "phaseAsymmetry": 0.2, "phaseShape": 0.7})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 400.0 + v * 120.0, "resonance": 0.4, "keyTrack": 0.2}
    lfo1 = {"waveform": 1, "mode": 0, "rateHz": 0.12 + v * 0.02, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.05, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    attack = 0.8 if v < 5 else 0.02
    release = 4.5 if v < 5 else 1.2
    amp_env = {"attackSeconds": attack, "decaySeconds": 1.0, "sustainLevel": 0.65 if v < 5 else 0.0,
               "releaseSeconds": release, "curveShape": 2.0}
    mod_routes = [
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 36.0, "scope": 0},
        {"source": SRC_MOD_WHEEL, "destination": DST_WT_BEND, "targetIndex": 1, "amount": 0.55, "scope": 1},
        {"source": SRC_LFO1, "destination": DST_OP_LEVEL, "targetIndex": 0, "amount": 0.65, "scope": 1},
        {"source": SRC_ENV1, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 24.0, "scope": 0},
    ]
    insert = [f.saturation(mix=0.35, drive=12.0)]
    master = master_cinematic_ambient(f, long_reverb=v < 6)
    extras = {"filter2": filter2_warm(cutoff=5000, drive=0.2), "polyphony": 6}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["RISE", "IMPACT", "SWEEP", "BEND", "NOISE", "TENSION", "DROP", "FLARE"], None, extras


def gen_keys(v, f, rng):
    ops = [{"engine": 0, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.28, "pan": 0.0}]
    voice = v % 4
    if voice == 0:
        ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.85, "pan": 0.0,
                    "resonatorStructure": 0.45, "resonatorDecay": 0.35, "resonatorDamping": 0.55,
                    "resonatorBrightness": 0.72, "resonatorModeCount": 5})
    elif voice == 1:
        ops.append({"engine": 2, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.82, "pan": 0.0,
                    "fmModulatorRatio": 4.01, "fmModulatorIndex": 0.65,
                    "fmModulatorFeedback": 0.15, "fmModulatorWaveform": f.WAVEFORM_SINE})
    elif voice == 2:
        ops.append({"engine": 1, "wavetableId": f.wt_path("seq", rng),
                    "wavetableFramePosition": 0.25, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.84, "pan": 0.0})
    else:
        ops.append({"engine": 4, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.82, "pan": 0.0,
                    "phaseBend": 0.2, "phaseFold": 0.25, "phaseAsymmetry": 0.05, "phaseShape": 0.45})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 3200.0 + v * 100, "resonance": 0.2, "keyTrack": 0.45}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 5.5, "syncDivisionIndex": 6, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.3, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    amp_env = {"attackSeconds": 0.002, "decaySeconds": 0.45 + v * 0.03, "sustainLevel": 0.05,
               "releaseSeconds": 0.55, "curveShape": 1.7}
    mod_routes = [
        {"source": SRC_VELOCITY, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 16.0, "scope": 0},
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 14.0, "scope": 0},
    ]
    insert = [f.tape_delay(mix=0.28, ms=280.0, fb=0.38)]
    master = [f.eq(highDb=1.0), f.reverb(mix=0.24, size=1.4, decay=2.8), f.comp(ratio=3.0), f.limiter()]
    arp = {"enabled": v in (2, 7), "mode": 0, "rateMode": 1, "rateHz": 8.0,
           "syncDivisionIndex": 6, "octaveRange": 2, "numSteps": 8, "latch": False}
    extras = {"polyphony": 12}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["SPARK", "DECAY", "BRIGHT", "SPACE", "BODY", "CRYSTAL", "PLUCK", "AIR"], arp, extras


def gen_bell(v, f, rng):
    ops = [{"engine": 2, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 1.0,
            "keyTrack": True, "level": 0.88, "pan": 0.0,
            "fmModulatorRatio": [2.0, 3.0, 4.01, 5.0, 6.0, 3.5, 4.5, 7.0][v],
            "fmModulatorIndex": 0.75 + v * 0.04, "fmModulatorFeedback": 0.35 + v * 0.02,
            "fmModulatorWaveform": f.WAVEFORM_SINE}]
    ops.append({"engine": 1, "wavetableId": "content/wavetables/bell-glass-chime.json",
                "wavetableFramePosition": 0.15 + v * 0.05, "frequencyRatio": 2.0,
                "keyTrack": True, "level": 0.35, "pan": rng.uniform(-0.3, 0.3),
                "wtFormantShift": -0.15 + v * 0.05})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 4500.0 + v * 200, "resonance": 0.15, "keyTrack": 0.55}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.25, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 3.0, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    amp_env = {"attackSeconds": 0.001, "decaySeconds": 1.2 + v * 0.08, "sustainLevel": 0.0,
               "releaseSeconds": 1.8 + v * 0.1, "curveShape": 2.0}
    mod_routes = [
        {"source": SRC_MOD_WHEEL, "destination": DST_WT_FORMANT, "targetIndex": 1, "amount": 0.45, "scope": 1},
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 12.0, "scope": 0},
        {"source": SRC_VELOCITY, "destination": DST_OP_LEVEL, "targetIndex": 0, "amount": 0.5, "scope": 0},
    ]
    insert = [f.chorus(mix=0.38, rate=0.4, depth=6.0, base=16.0)]
    master = [f.eq(highDb=1.5), f.reverb(mix=0.35, size=2.0, decay=4.5), f.limiter()]
    extras = {"polyphony": 10}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["METAL", "RING", "CHIME", "ORBIT", "DECAY", "SHIMMER", "STRIKE", "BELL"], None, extras


def gen_stack(v, f, rng):
    # Layer A: organ/additive body
    ops_a = [
        {"engine": 0, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True, "level": 0.35, "pan": 0.0},
        {"engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.75, "pan": 0.0,
         "additivePartialCount": 44, "additiveTilt": -0.18, "additiveOddEven": 0.72, "additiveStretch": 0.01},
        {"engine": 4, "frequencyRatio": 1.002, "keyTrack": True, "level": 0.25, "pan": 0.2,
         "phaseBend": 0.1, "phaseFold": 0.2, "phaseAsymmetry": 0.05, "phaseShape": 0.4},
    ]
    # Layer B: nebula wavetable wash
    ops_b = [
        {"engine": 1, "wavetableId": f.wt_path("pad", rng),
         "wavetableFramePosition": 0.3 + v * 0.05, "frequencyRatio": 1.0, "keyTrack": True,
         "level": 0.7, "pan": 0.0, "wtFormantShift": 0.2, "wtBend": 0.15},
        {"engine": 5, "wavetableId": f.wt_path("gran", rng),
         "wavetableFramePosition": 0.2, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.35,
         "pan": 0.25, "grainDensity": 9.0, "grainSizeMs": 130.0,
         "grainPositionJitter": 0.25, "grainPitchJitter": 0.04},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1900.0 + v * 100, "resonance": 0.12, "keyTrack": 0.2}
    filter1_b = {"enabled": True, "mode": 0, "cutoffHz": 2400.0 + v * 80, "resonance": 0.08, "keyTrack": 0.15}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.05, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.03, "syncDivisionIndex": 4, "phaseOffset": 0.35}
    amp_a = {"attackSeconds": 2.5, "decaySeconds": 1.5, "sustainLevel": 0.85,
             "releaseSeconds": 3.8, "curveShape": 2.1, "legato": True}
    amp_b = {"attackSeconds": 3.0, "decaySeconds": 2.0, "sustainLevel": 0.82,
             "releaseSeconds": 4.2, "curveShape": 2.2, "legato": True}
    mod_routes = mw_bloom_routes(wt_op=1, wt_amount=0.42)
    mod_routes.append({"source": SRC_MACRO1, "destination": DST_OP_LEVEL, "targetIndex": 0, "amount": 0.35, "scope": 0})
    insert = [f.tape_delay(mix=0.2, ms=450.0, fb=0.3)]
    master = master_cinematic_pad(f)
    layer_b = active_layer_b(f, ops_b, amp_b, filter1_b, gain=0.78, insert=[f.chorus(mix=0.3)])
    extras = {
        "filter2": filter2_warm(cutoff=3000 + v * 50),
        "layer_mode": 2,
        "layer_b": layer_b,
        "unison": {"mode": 1, "voices": 3},
        "polyphony": 24,
    }
    return ops_a, amp_a, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["MORPH", "BLOOM", "ORGAN", "NEBULA", "SPACE", "BODY", "DEPTH", "STACK"], None, extras


def gen_wildcard(v, f, rng):
    ops = [{"engine": 0, "classicWaveform": f.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.3, "pan": 0.0}]
    engines = [
        ("gran", 5, {"grainDensity": 18.0, "grainSizeMs": 70.0}),
        ("noise", 6, {"noiseVariant": 4, "noiseRate": 90.0}),
        ("phase", 4, {"phaseBend": 0.6, "phaseFold": 0.7}),
        ("wt", 1, {"wtSyncRatio": 3.0, "wtSyncAmount": 0.7}),
        ("fm", 2, {"fmModulatorRatio": 7.0, "fmModulatorIndex": 1.1}),
        ("wt", 1, {"wtBend": 0.65, "wtAsymmetry": 0.4}),
        ("gran", 5, {"grainDensity": 22.0, "grainSizeMs": 55.0}),
    ]
    label, eng, extra = engines[v]
    op = {"engine": eng, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.72, "pan": rng.uniform(-0.4, 0.4)}
    if eng == 5:
        op.update({"wavetableId": f.wt_path("gran", rng), "wavetableFramePosition": 0.4,
                   "grainPositionJitter": 0.45, "grainPitchJitter": 0.12, **extra})
    elif eng == 6:
        op.update(extra)
    elif eng == 4:
        op.update({**extra, "phaseAsymmetry": 0.25, "phaseShape": 0.75})
    elif eng == 1:
        op.update({"wavetableId": f.wt_path("lead", rng), "wavetableFramePosition": 0.5, **extra})
    else:
        op.update({"classicWaveform": f.WAVEFORM_SINE, **extra,
                   "fmModulatorFeedback": 0.25, "fmModulatorWaveform": f.WAVEFORM_SINE})
    ops.append(op)

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1800.0 + v * 200, "resonance": 0.28, "keyTrack": 0.25}
    lfo1 = {"waveform": 4, "mode": 0, "rateHz": 0.08, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.22, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    amp_env = {"attackSeconds": 1.2, "decaySeconds": 1.5, "sustainLevel": 0.75,
               "releaseSeconds": 3.5, "curveShape": 2.0}
    mod_routes = [
        {"source": SRC_MOD_WHEEL, "destination": DST_WT_BEND, "targetIndex": 1, "amount": 0.5, "scope": 1},
        {"source": SRC_MOD_WHEEL, "destination": DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 22.0, "scope": 0},
        {"source": SRC_LFO1, "destination": DST_WT_POS, "targetIndex": 1, "amount": 0.55, "scope": 1},
        {"source": SRC_MACRO1, "destination": DST_PAN, "targetIndex": 0, "amount": 0.4, "scope": 0},
    ]
    insert = [f.freq_shift_echo(mix=0.18, hz=5.0 + v, delay=320.0, fb=0.45)]
    master = master_cinematic_ambient(f, long_reverb=False)
    extras = {"filter2": filter2_dark(cutoff=2600, drive=0.35), "polyphony": 12}
    return ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, \
        ["CHAOS", "GLITCH", "MORPH", "BEND", "PAN", "WARP", "DRIVE", "VOID"], None, extras


GENERATORS = {
    "cosmic_pad": gen_cosmic_pad,
    "massive_bass": gen_massive_bass,
    "pulsar": gen_pulsar,
    "lead": gen_lead,
    "texture": gen_texture,
    "fx": gen_fx,
    "keys": gen_keys,
    "bell": gen_bell,
    "stack": gen_stack,
    "wildcard": gen_wildcard,
}


def slugify(name: str) -> str:
    return name.lower().replace(" ", "-").replace("'", "")


def build_interstellar_patch(f, slot, display_name, archetype, variant, rng):
    gen = GENERATORS[archetype]
    internal_cat = ARCHETYPE_INTERNAL_CAT[archetype]
    result = gen(variant, f, rng)
    arp = None
    extras = {}
    if len(result) == 9:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names = result
    elif len(result) == 10:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, arp = result
    else:
        ops, amp_env, filter1, lfo1, lfo2, mod_routes, insert, master, macro_names, arp, extras = result

    seed = hash((BATCH_TAG, archetype, slot, display_name)) & 0xFFFFFFFF
    engines_used = sorted({o["engine"] for o in ops})
    engine_names = ["Classic", "Wavetable", "FM/PM", "Additive", "PhaseShape",
                    "Granular", "NoiseChaos", "Resonator"]
    tip = PLAYING_TIPS[archetype]
    desc = (f"Interstellar cinematic preset #{slot:03d} — {display_name}. "
            f"{tip} Engines: {', '.join(engine_names[e] for e in engines_used)}.")

    tags = [BATCH_TAG] + ARCHETYPE_TAGS[archetype] + ["factory", "score"]
    moods = ["cinematic", "cosmic", "melancholic", "vast"]

    unison = extras.pop("unison", None)
    polyphony = extras.pop("polyphony", 8)
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
        unison=unison,
        polyphony=polyphony,
    )

    patch["schemaVersion"] = 3
    patch["metadata"]["schemaVersion"] = 3
    patch["metadata"]["category"] = "interstellar"
    patch["metadata"]["author"] = AUTHOR
    patch["metadata"]["id"] = f"pw8-{BATCH_TAG}-{slot:03d}-{seed}"
    patch["metadata"]["genres"] = ["cinematic", "score", "ambient", "factory"]
    patch["metadata"]["createdAt"] = "2026-08-13T00:00:00Z"

    if "filter2" in extras:
        patch["layerA"]["filter2"] = extras["filter2"]
    if extras.get("layer_mode"):
        patch["layerMode"] = extras["layer_mode"]
    if extras.get("layer_b"):
        patch["layerB"] = extras["layer_b"]
    if extras.get("portamento"):
        patch["voiceSettings"]["portamentoSeconds"] = extras["portamento"]

    patch["layerA"]["algorithm"] = build_algorithm_graph(graph_kind_for_slot(slot), ops)

    f.normalize_preset_standards(patch)
    return patch


def feature_matrix():
    """Return coverage counts for README."""
    engines = set()
    has_filter2 = has_warp = has_granular = has_stack = has_arp = 0
    for pw8 in sorted(OUT_DIR.glob("*.pw8")):
        data = json.loads(pw8.read_text())
        for op in data.get("layerA", {}).get("operators", []):
            if op.get("level", 0) > 0:
                engines.add(op.get("engine", 0))
        f2 = data.get("layerA", {}).get("filter2", {})
        if f2.get("enabled"):
            has_filter2 += 1
        for op in data.get("layerA", {}).get("operators", []):
            if any(op.get(k, 0) for k in ("wtBend", "wtSyncAmount", "wtFormantShift")):
                has_warp += 1
                break
        if any(op.get("engine") == 5 and op.get("level", 0) > 0
               for op in data.get("layerA", {}).get("operators", [])):
            has_granular += 1
        if data.get("layerMode") == 2:
            has_stack += 1
        if data.get("arpeggiator", {}).get("enabled"):
            has_arp += 1
    engine_names = ["Classic", "Wavetable", "FM/PM", "Additive", "PhaseShape",
                    "Granular", "NoiseChaos", "Resonator"]
    used = [engine_names[e] for e in sorted(engines)]
    return {
        "engines": used,
        "filter2": has_filter2,
        "warp": has_warp,
        "granular": has_granular,
        "stack": has_stack,
        "arp": has_arp,
        "total": len(list(OUT_DIR.glob("*.pw8"))),
    }


def engine_highlight_for_slot(slot: int) -> str:
    fname = OUT_DIR / f"{slot:03d}-{slugify(PRESET_CATALOG[slot - 1][0])}.pw8"
    if not fname.exists():
        return "—"
    data = json.loads(fname.read_text())
    engine_names = ["Classic", "Wavetable", "FM", "Additive", "PhaseShape", "Granular", "Noise", "Resonator"]
    engines = sorted({op.get("engine", 0) for op in data.get("layerA", {}).get("operators", [])
                      if op.get("level", 0) > 0})
    parts = []
    for e in engines[:3]:
        parts.append(engine_names[e])
    if any(op.get("wtBend") or op.get("wtSyncAmount") or op.get("wtFormantShift")
           for op in data.get("layerA", {}).get("operators", [])):
        parts.append("Warp")
    if data.get("layerA", {}).get("filter2", {}).get("enabled"):
        parts.append("Filter2")
    if data.get("layerMode") == 2:
        parts.append("Stack")
    return ", ".join(parts) if parts else "Classic"


def write_readme():
    matrix = feature_matrix()
    highlight_rows = "\n".join(
        f"| {num:03d} | **{name.upper()}** | {tip} |"
        for num, name, tip in HIGHLIGHT_TOP10
    )
    category_sections = []
    for theme_name, theme_blurb, slot_range in THEME_CATEGORIES:
        rows = []
        for slot in slot_range:
            display_name, archetype, _variant = PRESET_CATALOG[slot - 1]
            slug = slugify(display_name)
            internal = ARCHETYPE_INTERNAL_CAT[archetype]
            eng = engine_highlight_for_slot(slot)
            tip = PLAYING_TIPS[archetype].split(";")[0]
            rows.append(
                f"| {slot:03d} | `{slug}.pw8` | {internal} | {eng} | {tip} |"
            )
        table = "\n".join(rows)
        category_sections.append(
            f"### {theme_name}\n\n{theme_blurb}\n\n"
            f"| # | File | Role | Engine highlight | Inspiration |\n"
            f"|---|------|------|------------------|-------------|\n{table}\n"
        )
    categories_block = "\n".join(category_sections)
    readme = f"""# Interstellar Factory Presets

100 cinematic, research-grounded **Interstellar** patches for MURMUR / Patchwork Eight —
real astrophysics vocabulary (nebulae, pulsars, CMB, accretion disks), sci-fi film aesthetics
(Interstellar, Contact, 2001, Arrival), and full engine coverage (8 operators, wavetable warps,
Filter 2, granular, FM, stack mode).

## Browse in Logic / your DAW

1. Insert **MURMUR** (Patchwork Eight) on an instrument track.
2. Open the **Browse** preset overlay (or Load on the preset bar).
3. Filter category to **interstellar** or search tags `interstellar`, `cinematic`, `cosmic`.
4. Files ship at `content/presets/factory/Interstellar/` — auto-indexed at plugin load.

## Highlight reel — start here

| # | Preset | Playing tip |
|---|--------|-------------|
{highlight_rows}

## Categories (10 × 10)

{categories_block}

## Archetype mix

| Archetype | Count |
|-----------|------:|
| Cosmic pads | 15 |
| Massive bass / sub | 12 |
| Pulsar / clock rhythms | 10 |
| Leads | 10 |
| Textures / drones | 10 |
| FX / transitions | 10 |
| Keys / plucks | 10 |
| Bells / mallets | 8 |
| Stack / layer showcases | 8 |
| Wildcard experimental | 7 |

## Feature coverage (collection)

| Feature | Presets |
|---------|--------:|
| Total | {matrix['total']} |
| Filter 2 enabled | {matrix['filter2']} |
| Wavetable warps (bend/sync/formant) | {matrix['warp']} |
| Granular layers | {matrix['granular']} |
| Dual-layer stack mode | {matrix['stack']} |
| Arpeggiator | {matrix['arp']} |
| Engines used | {', '.join(matrix['engines'])} |

## Regenerate

```bash
python3 scripts/generate_interstellar_presets.py
```

Authored by {AUTHOR}. Batch tag: `{BATCH_TAG}`.
"""
    (OUT_DIR / "README.md").write_text(readme)


def main():
    f = load_factory()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for stale in OUT_DIR.glob("*.pw8"):
        stale.unlink()

    manifest_entries = []
    for slot, (display_name, archetype, variant) in enumerate(PRESET_CATALOG, start=1):
        seed_rng = random.Random(hash((BATCH_TAG, slot, display_name)) & 0xFFFFFFFF)
        patch = build_interstellar_patch(f, slot, display_name, archetype, variant, seed_rng)
        fname = f"{slot:03d}-{slugify(display_name)}.pw8"
        path = OUT_DIR / fname
        path.write_text(json.dumps(patch, indent=2) + "\n")
        manifest_entries.append({
            "category": "Interstellar",
            "file": f"factory/Interstellar/{fname}",
            "name": display_name.upper(),
        })

    write_readme()
    update_manifest(manifest_entries)
    print(f"Generated {len(manifest_entries)} Interstellar presets in {OUT_DIR}")
    matrix = feature_matrix()
    print(f"Engines: {', '.join(matrix['engines'])}")
    print(f"Filter2: {matrix['filter2']}, Warp: {matrix['warp']}, Stack: {matrix['stack']}, Arp: {matrix['arp']}")


def update_manifest(new_entries: list[dict]) -> None:
    manifest = []
    if MANIFEST_PATH.exists():
        manifest = json.loads(MANIFEST_PATH.read_text())
    manifest = [e for e in manifest if "factory/Interstellar/" not in e.get("file", "")]
    manifest.extend(new_entries)
    manifest.sort(key=lambda e: (e["category"], e["file"]))
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"MANIFEST.json now lists {len(manifest)} total factory presets.")


if __name__ == "__main__":
    main()
