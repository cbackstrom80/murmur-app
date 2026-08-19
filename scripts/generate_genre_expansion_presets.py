#!/usr/bin/env python3
"""Generate 500 genre-focused factory presets (100 per category, slots 61-160).

Emphasis: hoover basses, dream-pop, house, cinematic. Adds to existing 01-60
factory bank without deleting prior patches. Updates MANIFEST.json.

Run from repo root:
    python3 scripts/generate_genre_expansion_presets.py
"""
import importlib.util
import json
import pathlib
import random
import re
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_ROOT = REPO_ROOT / "content" / "presets" / "factory"
MANIFEST_PATH = OUT_ROOT / "MANIFEST.json"
AUTHOR = "MURMUR Sound Design"
BATCH_TAG = "genre-expansion"
START_INDEX = 61
COUNT_PER_CATEGORY = 100
END_INDEX = START_INDEX + COUNT_PER_CATEGORY - 1

GENRE_TAGS = {
    "hoover": ["hoover-bass", "rave", "90s-rave", "house"],
    "house": ["house", "club", "gated-80s", "motor"],
    "dream-pop": ["dream-pop", "shoegaze", "neon", "80s-pop"],
    "cinematic": ["cinematic", "score", "trailer", "epic"],
}

NAME_POOLS = {
    "hoover": (
        ["Rave", "Pump", "Acid", "Club", "Turbine", "Blast", "Warp", "Roller", "Peak", "Vault"],
        ["Hoover", "Rush", "Sweep", "Growl", "Horn", "Drive", "Surge", "Wail", "Roar", "Blast"],
    ),
    "house": (
        ["Motor", "Club", "Garage", "Deep", "Pump", "Neon", "Floor", "Grid", "Pulse", "Night"],
        ["House", "Pulse", "Groove", "Line", "Kick", "Run", "Step", "Loop", "Drive", "Zone"],
    ),
    "dream-pop": (
        ["Velvet", "Shimmer", "Midnight", "Glass", "Aurora", "Comfort", "Haze", "Neon", "Cascade", "Dream"],
        ["Veil", "Glow", "Wash", "Bloom", "Drift", "Hymn", "Beam", "Echo", "Field", "Swell"],
    ),
    "cinematic": (
        ["Epic", "Trailer", "Score", "Nova", "Horizon", "Apex", "Titan", "Vast", "Oracle", "Prime"],
        ["Rise", "Fall", "Pulse", "Expanse", "Storm", "Dawn", "Tide", "Forge", "Crown", "Arc"],
    ),
}

# Per-category genre slot plans (100 entries each, index 0 = slot 61).
def _plan_hoover(n):
    return ["hoover"] * n

def _plan_mix(*pairs):
    out = []
    for genre, count in pairs:
        out.extend([genre] * count)
    return out

CATEGORY_GENRE_PLAN = {
    "bass": _plan_mix(("hoover", 28), ("house", 28), ("dream-pop", 22), ("cinematic", 22)),
    "lead": _plan_mix(("dream-pop", 30), ("house", 25), ("cinematic", 30), ("dream-pop", 15)),  # 100
    "pad": _plan_mix(("dream-pop", 35), ("cinematic", 35), ("house", 30)),
    "seq": _plan_mix(("house", 38), ("dream-pop", 28), ("cinematic", 22), ("house", 12)),
    "ambient": _plan_mix(("cinematic", 42), ("dream-pop", 33), ("house", 25)),
}

factory = None


def load_factory_module():
    path = REPO_ROOT / "scripts" / "generate_factory_presets.py"
    spec = importlib.util.spec_from_file_location("factory_presets", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def load_existing_names():
    used = set()
    for pw8 in sorted((REPO_ROOT / "content" / "presets").rglob("*.pw8")) + \
               sorted((REPO_ROOT / "content" / "presets").rglob("*.murmur")):
        if "factory" in pw8.parts:
            slot_str = pw8.stem.split("-", 1)[0]
            if slot_str.isdigit() and START_INDEX <= int(slot_str) <= END_INDEX:
                continue
        try:
            name = json.loads(pw8.read_text()).get("metadata", {}).get("name", "").strip()
            if name:
                used.add(name.lower())
        except (json.JSONDecodeError, OSError):
            pass
    return used


def load_manifest():
    if MANIFEST_PATH.exists():
        return json.loads(MANIFEST_PATH.read_text())
    return []


def make_genre_name(genre, rng, used):
    adjs, nouns = NAME_POOLS[genre]
    for _ in range(300):
        name = f"{rng.choice(adjs)} {rng.choice(nouns)}"
        key = name.lower()
        if key not in used:
            used.add(key)
            return name
    name = f"{rng.choice(adjs)} {rng.choice(nouns)} {rng.randint(2, 99)}"
    used.add(name.lower())
    return name


def house_master(pump=True):
    fx = [
        factory.eq(lowDb=1.5, midDb=-0.5, highDb=0.0),
        factory.comp(thresh=-14.0 if pump else -18.0, ratio=4.0, atk=8.0, rel=120.0, makeup=2.0),
        factory.limiter(),
    ]
    if pump:
        fx.insert(1, factory.chorus(mix=0.15, rate=0.4, depth=2.5))
    return fx


def dream_master(reverb_mix=0.32):
    return [
        factory.eq(highDb=1.0),
        factory.chorus(mix=0.28, rate=0.35, depth=5.0, base=14.0),
        factory.reverb(mix=reverb_mix, size=2.0, decay=4.5),
        factory.comp(ratio=2.5),
        factory.limiter(),
    ]


def cinematic_master(long_reverb=False):
    decay = 9.0 if long_reverb else 5.5
    mix = 0.45 if long_reverb else 0.32
    return [
        factory.eq(lowDb=0.5, midDb=-0.5, highDb=-1.0),
        factory.reverb(mix=mix, size=2.8, decay=decay, predelay=35.0),
        factory.comp(thresh=-16.0, ratio=2.2),
        factory.limiter(),
    ]


def shimmer_insert(delay_mix=0.2):
    return [
        factory.chorus(mix=0.32, rate=0.4, depth=5.5, base=12.0),
        factory.tape_delay(mix=delay_mix, ms=320.0, fb=0.35, driftDepth=8.0),
    ]


# ------------------------------------------------------------------ hoover --

def gen_hoover_bass(i, rng):
    """Classic rave hoover: detuned saws/squares, resonant LP sweep."""
    wf = factory.WAVEFORM_SAW if i % 3 != 1 else factory.WAVEFORM_SQR
    ops = [{"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.45, "pan": 0.0}]
    detunes = [1.0, 1.006 + (i % 5) * 0.001, 0.994 - (i % 4) * 0.001, 1.012]
    for j, ratio in enumerate(detunes[:3 + (i % 2)]):
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": ratio,
                    "keyTrack": True, "level": 0.55 - j * 0.08, "pan": rng.uniform(-0.15, 0.15)})
    if i % 4 == 0:
        ops.append({"engine": 4, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.35, "pan": 0.0,
                    "phaseBend": 0.35, "phaseFold": 0.55, "phaseAsymmetry": 0.15, "phaseShape": 0.65})

    cutoff = 650.0 + (i % 12) * 85.0
    res = 0.55 + (i % 6) * 0.05
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": min(res, 0.85), "keyTrack": 0.35}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 2.5 + (i % 8) * 0.4, "syncDivisionIndex": 6, "phaseOffset": 0.0}
    lfo2 = {"waveform": 1, "mode": 0, "rateHz": 0.18, "syncDivisionIndex": 4, "phaseOffset": 0.25}
    ampEnv = {"attackSeconds": 0.003, "decaySeconds": 0.12, "sustainLevel": 0.88,
              "releaseSeconds": 0.22, "curveShape": 1.4}
    modRoutes = [
        {"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
         "amount": 14.0 + (i % 5), "scope": 0},
        {"source": factory.SRC_LFO2, "destination": factory.DST_FILTER_RES, "targetIndex": 0,
         "amount": 0.25, "scope": 0},
        {"source": factory.SRC_VELOCITY, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
         "amount": 12.0, "scope": 0},
    ]
    insert = [factory.saturation(mix=0.4, drive=14.0), factory.chorus(mix=0.18, rate=0.5, depth=3.0)]
    master = house_master(pump=True)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["SWEEP", "BITE", "PUNCH", "RESONANCE", "DRIVE", "WIDTH", "SUB", "DECAY"]


# ------------------------------------------------------------------ genre bass --

def gen_house_bass(i, rng):
    wf = factory.WAVEFORM_SQR if i % 2 else factory.WAVEFORM_SAW
    ops = [{"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.6, "pan": 0.0},
           {"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True,
            "level": 0.82, "pan": 0.0}]
    if i % 3 == 0:
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.003,
                    "keyTrack": True, "level": 0.35, "pan": 0.1})
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 280.0 + (i % 10) * 40.0,
               "resonance": 0.18, "keyTrack": 0.2}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.5, "syncDivisionIndex": 5, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 2.0, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.002, "decaySeconds": 0.18, "sustainLevel": 0.8,
              "releaseSeconds": 0.15, "curveShape": 1.5}
    modRoutes = [{"source": factory.SRC_VELOCITY, "destination": factory.DST_FILTER_CUTOFF,
                  "targetIndex": 0, "amount": 8.0, "scope": 0}]
    insert = [factory.saturation(mix=0.3, drive=10.0)]
    master = house_master(pump=(i % 2 == 0))
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["PUMP", "PUNCH", "SUB", "DRIVE", "MOVE", "WIDTH", "GRIT", "DECAY"]


def gen_dream_bass(i, rng):
    ops = [{"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.5, "pan": 0.0},
           {"engine": 0, "classicWaveform": factory.WAVEFORM_TRI, "frequencyRatio": 1.0,
            "keyTrack": True, "level": 0.7, "pan": 0.0}]
    if i % 2:
        ops.append({"engine": 2, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.75, "pan": 0.0,
                    "fmModulatorRatio": 2.0, "fmModulatorIndex": 0.5, "fmModulatorFeedback": 0.1,
                    "fmModulatorWaveform": factory.WAVEFORM_SINE})
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 320.0 + i * 8.0, "resonance": 0.22, "keyTrack": 0.3}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.35, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 1.2, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.008, "decaySeconds": 0.3, "sustainLevel": 0.72,
              "releaseSeconds": 0.45, "curveShape": 1.8}
    modRoutes = [{"source": factory.SRC_VELOCITY, "destination": factory.DST_FILTER_CUTOFF,
                  "targetIndex": 0, "amount": 10.0, "scope": 0}]
    insert = shimmer_insert(0.15)
    master = dream_master(0.18)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["GLOW", "SUB", "WASH", "DRIVE", "WIDTH", "MOVE", "PUNCH", "DECAY"]


def gen_cinematic_bass(i, rng):
    ops = [{"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.55, "pan": 0.0},
           {"engine": 0, "classicWaveform": factory.WAVEFORM_SAW, "frequencyRatio": 1.0,
            "keyTrack": True, "level": 0.65, "pan": 0.0}]
    ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.35, "pan": 0.0,
                "resonatorStructure": 0.25, "resonatorDecay": 0.6, "resonatorDamping": 0.5,
                "resonatorBrightness": 0.4, "resonatorModeCount": 5})
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 220.0 + i * 6.0, "resonance": 0.15, "keyTrack": 0.15}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.08, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.5, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.02, "decaySeconds": 0.5, "sustainLevel": 0.78,
              "releaseSeconds": 0.8, "curveShape": 2.0}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF,
                  "targetIndex": 0, "amount": 5.0, "scope": 0}]
    insert = [factory.tape_delay(mix=0.12, ms=480.0, fb=0.28)]
    master = cinematic_master(False)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["DEPTH", "WEIGHT", "SUB", "SPACE", "MOVE", "TONE", "PUNCH", "DECAY"]


# ------------------------------------------------------------------ leads --

def gen_dream_lead(i, rng):
    wf = factory.WAVEFORM_SAW
    ops = [
        {"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.85, "pan": 0.0},
        {"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.006, "keyTrack": True, "level": 0.5, "pan": 0.28},
        {"engine": 0, "classicWaveform": wf, "frequencyRatio": 0.997, "keyTrack": True, "level": 0.45, "pan": -0.25},
    ]
    if i % 3 == 0:
        ops.append({"engine": 2, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.55,
                    "pan": 0.0, "fmModulatorRatio": 3.01, "fmModulatorIndex": 0.45,
                    "fmModulatorFeedback": 0.15, "fmModulatorWaveform": factory.WAVEFORM_SINE})
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 2400.0 + i * 35.0, "resonance": 0.3, "keyTrack": 0.4}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 5.0, "syncDivisionIndex": 6, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.15, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.012, "decaySeconds": 0.25, "sustainLevel": 0.8, "releaseSeconds": 0.4, "curveShape": 1.9}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 6.0, "scope": 1}]
    insert = shimmer_insert(0.22)
    master = dream_master(0.3)
    unison = {"mode": 1, "voices": 3, "detune": 0.12, "spread": 0.65}
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["SHIMMER", "BITE", "WIDTH", "SPACE", "GLOW", "MOVE", "TONE", "DECAY"], None, unison


def gen_house_lead(i, rng):
    wf = factory.WAVEFORM_SQR if i % 2 else factory.WAVEFORM_SAW
    ops = [
        {"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.9, "pan": 0.0},
        {"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.5, "keyTrack": True, "level": 0.45, "pan": 0.2},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1800.0 + i * 40.0, "resonance": 0.35, "keyTrack": 0.35}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 8.0, "syncDivisionIndex": 7, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.5, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.004, "decaySeconds": 0.15, "sustainLevel": 0.75, "releaseSeconds": 0.2, "curveShape": 1.5}
    modRoutes = [{"source": factory.SRC_VELOCITY, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 12.0, "scope": 0}]
    insert = [factory.saturation(mix=0.25, drive=8.0)]
    master = house_master(pump=True)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["STAB", "BITE", "DRIVE", "WIDTH", "PUMP", "MOVE", "TONE", "DECAY"]


def gen_cinematic_lead(i, rng):
    ops = [
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SAW, "frequencyRatio": 1.0, "keyTrack": True,
         "level": 0.8, "pan": 0.0},
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SAW, "frequencyRatio": 1.498, "keyTrack": True,
         "level": 0.55, "pan": 0.3},
        {"engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.35, "pan": -0.2,
         "additiveHarmonicCount": 8, "additiveBrightness": 0.45, "additiveDecay": 0.6},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 3200.0 + i * 25.0, "resonance": 0.22, "keyTrack": 0.5}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.25, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 3.0, "syncDivisionIndex": 5, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.35 if i % 4 == 0 else 0.08, "decaySeconds": 0.4,
              "sustainLevel": 0.85, "releaseSeconds": 0.9, "curveShape": 2.1}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 8.0, "scope": 1}]
    insert = [factory.tape_delay(mix=0.25, ms=420.0, fb=0.38)]
    master = cinematic_master(True)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["SWELL", "EPIC", "SPACE", "WIDTH", "TONE", "RISE", "MOVE", "DECAY"]


# ------------------------------------------------------------------ pads --

def gen_dream_pad(i, rng):
    ops = [
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SAW, "frequencyRatio": 1.0, "keyTrack": True,
         "level": 0.55, "pan": 0.0},
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SAW, "frequencyRatio": 1.004, "keyTrack": True,
         "level": 0.42, "pan": 0.35},
        {"engine": 1, "wavetableId": factory.wt_path("pad", rng), "wavetableFramePosition": 0.2 + (i % 10) * 0.06,
         "frequencyRatio": 1.0, "keyTrack": True, "level": 0.4, "pan": -0.3},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1400.0 + i * 15.0, "resonance": 0.12, "keyTrack": 0.2}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.06, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.04, "syncDivisionIndex": 4, "phaseOffset": 0.5}
    ampEnv = {"attackSeconds": 2.5, "decaySeconds": 1.8, "sustainLevel": 0.85, "releaseSeconds": 4.0,
              "curveShape": 2.2, "legato": True}
    modRoutes = [
        {"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 4.0, "scope": 1},
        {"source": factory.SRC_LFO2, "destination": factory.DST_WT_POS, "targetIndex": 2, "amount": 0.35, "scope": 1},
    ]
    insert = shimmer_insert(0.25)
    master = dream_master(0.42)
    unison = {"mode": 1, "voices": 4, "detune": 0.18, "spread": 0.85}
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["WASH", "SHIMMER", "WIDTH", "SPACE", "GLOW", "MORPH", "AIR", "SWELL"], None, unison


def gen_cinematic_pad(i, rng):
    ops = [
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True,
         "level": 0.4, "pan": 0.0},
        {"engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.5, "pan": 0.15,
         "additiveHarmonicCount": 12, "additiveBrightness": 0.35, "additiveDecay": 0.75},
        {"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.45, "pan": -0.2,
         "resonatorStructure": 0.4, "resonatorDecay": 0.8, "resonatorDamping": 0.35,
         "resonatorBrightness": 0.5, "resonatorModeCount": 7},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1100.0 + i * 12.0, "resonance": 0.1, "keyTrack": 0.15}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.04, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.025, "syncDivisionIndex": 4, "phaseOffset": 0.3}
    ampEnv = {"attackSeconds": 3.5, "decaySeconds": 2.0, "sustainLevel": 0.88, "releaseSeconds": 4.0,
              "curveShape": 2.3, "legato": True}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 5.0, "scope": 1}]
    insert = [factory.tape_delay(mix=0.2, ms=550.0, fb=0.32)]
    master = cinematic_master(True)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["EPIC", "DEPTH", "SPACE", "SWELL", "AIR", "TONE", "RISE", "DECAY"]


def gen_house_pad(i, rng):
    ops = [
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SQR, "frequencyRatio": 1.0, "keyTrack": True,
         "level": 0.5, "pan": 0.0},
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SAW, "frequencyRatio": 1.003, "keyTrack": True,
         "level": 0.45, "pan": 0.25},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 900.0 + i * 10.0, "resonance": 0.08, "keyTrack": 0.1}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.12, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.08, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 1.2, "decaySeconds": 1.0, "sustainLevel": 0.82, "releaseSeconds": 2.5,
              "curveShape": 2.0, "legato": True}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 3.0, "scope": 1}]
    insert = [factory.chorus(mix=0.35, rate=0.3, depth=6.0)]
    master = house_master(pump=False)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["CLUB", "WASH", "WIDTH", "SPACE", "GLOW", "MOVE", "TONE", "DECAY"]


# ------------------------------------------------------------------ sequences --

def gen_house_seq(i, rng):
    wf = factory.WAVEFORM_SQR if i % 2 else factory.WAVEFORM_SAW
    ops = [
        {"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.85, "pan": 0.0},
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True,
         "level": 0.35, "pan": 0.0},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1200.0 + i * 30.0, "resonance": 0.28, "keyTrack": 0.3}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 6.0, "syncDivisionIndex": 7, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.5, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.002, "decaySeconds": 0.12, "sustainLevel": 0.35, "releaseSeconds": 0.08, "curveShape": 1.4}
    modRoutes = [{"source": factory.SRC_VELOCITY, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 10.0, "scope": 0}]
    insert = [factory.tape_delay(mix=0.22, ms=180.0, fb=0.35)]
    master = house_master(pump=True)
    arp = {"enabled": True, "mode": i % 7, "rateMode": 1, "rateHz": 8.0,
           "syncDivisionIndex": 5 + (i % 3), "octaveRange": 2, "numSteps": 8 + (i % 3) * 4, "latch": i % 5 == 0}
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["RATE", "GATE", "PUMP", "BITE", "SPACE", "SWING", "RANGE", "MOTION"], arp


def gen_dream_seq(i, rng):
    ops = [
        {"engine": 2, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 1.0, "keyTrack": True,
         "level": 0.82, "pan": 0.0, "fmModulatorRatio": 3.0, "fmModulatorIndex": 0.55,
         "fmModulatorFeedback": 0.12, "fmModulatorWaveform": factory.WAVEFORM_SINE},
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 2.0, "keyTrack": True,
         "level": 0.25, "pan": 0.15},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 2800.0 + i * 20.0, "resonance": 0.2, "keyTrack": 0.35}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 4.0, "syncDivisionIndex": 6, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.2, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.003, "decaySeconds": 0.35, "sustainLevel": 0.2, "releaseSeconds": 0.25, "curveShape": 1.6}
    modRoutes = [{"source": factory.SRC_VELOCITY, "destination": factory.DST_OP_LEVEL, "targetIndex": 0,
                  "amount": 0.4, "scope": 0}]
    insert = shimmer_insert(0.28)
    master = dream_master(0.25)
    arp = {"enabled": True, "mode": 1, "rateMode": 1, "rateHz": 8.0,
           "syncDivisionIndex": 6, "octaveRange": 2, "numSteps": 12, "latch": False}
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["CHIME", "RATE", "SPACE", "BITE", "SWING", "GLOW", "RANGE", "MOTION"], arp


def gen_cinematic_seq(i, rng):
    ops = [
        {"engine": 1, "wavetableId": factory.wt_path("seq", rng), "wavetableFramePosition": 0.3,
         "frequencyRatio": 1.0, "keyTrack": True, "level": 0.75, "pan": 0.0},
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SAW, "frequencyRatio": 1.0, "keyTrack": True,
         "level": 0.35, "pan": 0.2},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 2200.0 + i * 18.0, "resonance": 0.18, "keyTrack": 0.4}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 2.0, "syncDivisionIndex": 5, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.1, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.01, "decaySeconds": 0.45, "sustainLevel": 0.55, "releaseSeconds": 0.5, "curveShape": 1.9}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 6.0, "scope": 1}]
    insert = [factory.tape_delay(mix=0.3, ms=380.0, fb=0.42)]
    master = cinematic_master(True)
    arp = {"enabled": True, "mode": 3, "rateMode": 1, "rateHz": 8.0,
           "syncDivisionIndex": 5, "octaveRange": 3, "numSteps": 16, "latch": i % 3 == 0}
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["SCORE", "RATE", "SPACE", "EPIC", "SWING", "RISE", "RANGE", "MOTION"], arp


# ------------------------------------------------------------------ ambient --

def gen_cinematic_ambient(i, rng):
    ops = [
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True,
         "level": 0.35, "pan": 0.0},
        {"engine": 5, "wavetableId": factory.wt_path("gran", rng), "wavetableFramePosition": 0.15,
         "frequencyRatio": 1.0, "keyTrack": True, "level": 0.45, "pan": 0.3,
         "grainDensity": 10.0, "grainSizeMs": 140.0, "grainPositionJitter": 0.3, "grainPitchJitter": 0.06},
        {"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.5, "pan": -0.25,
         "resonatorStructure": 0.45, "resonatorDecay": 0.85, "resonatorDamping": 0.3,
         "resonatorBrightness": 0.55, "resonatorModeCount": 8},
    ]
    if i % 3 == 0:
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": 0.06,
                    "pan": 0.0, "noiseVariant": 2, "noiseRate": 35.0})
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1800.0 + i * 10.0, "resonance": 0.08, "keyTrack": 0.1}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.035, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.02, "syncDivisionIndex": 4, "phaseOffset": 0.4}
    ampEnv = {"attackSeconds": 4.0, "decaySeconds": 2.5, "sustainLevel": 0.9, "releaseSeconds": 8.0, "curveShape": 2.4}
    modRoutes = [
        {"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0, "amount": 4.0, "scope": 1},
        {"source": factory.SRC_LFO2, "destination": factory.DST_WT_POS, "targetIndex": 1, "amount": 0.4, "scope": 1},
    ]
    insert = [factory.tape_delay(mix=0.22, ms=620.0, fb=0.38)]
    master = cinematic_master(True)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["EPIC", "DRIFT", "DEPTH", "SPACE", "RISE", "AIR", "SHIMMER", "DECAY"]


def gen_dream_ambient(i, rng):
    ops = [
        {"engine": 1, "wavetableId": factory.wt_path("ambient", rng), "wavetableFramePosition": 0.25 + (i % 8) * 0.05,
         "frequencyRatio": 1.0, "keyTrack": True, "level": 0.48, "pan": 0.0},
        {"engine": 5, "wavetableId": factory.wt_path("gran", rng), "wavetableFramePosition": 0.2,
         "frequencyRatio": 2.0, "keyTrack": True, "level": 0.38, "pan": 0.35,
         "grainDensity": 14.0, "grainSizeMs": 100.0, "grainPositionJitter": 0.4, "grainPitchJitter": 0.1},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1500.0 + i * 8.0, "resonance": 0.1, "keyTrack": 0.12}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.05, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.03, "syncDivisionIndex": 4, "phaseOffset": 0.5}
    ampEnv = {"attackSeconds": 3.0, "decaySeconds": 2.0, "sustainLevel": 0.88, "releaseSeconds": 5.5, "curveShape": 2.2}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 4.5, "scope": 1}]
    insert = shimmer_insert(0.2)
    master = dream_master(0.5)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["WASH", "SHIMMER", "DRIFT", "SPACE", "GLOW", "AIR", "MORPH", "DECAY"]


def gen_house_ambient(i, rng):
    ops = [
        {"engine": 0, "classicWaveform": factory.WAVEFORM_SQR, "frequencyRatio": 1.0, "keyTrack": True,
         "level": 0.35, "pan": 0.0},
        {"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": 0.08, "pan": 0.2,
         "noiseVariant": 1, "noiseRate": 45.0},
    ]
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1200.0 + i * 8.0, "resonance": 0.06, "keyTrack": 0.05}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.15, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.08, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 1.5, "decaySeconds": 1.2, "sustainLevel": 0.8, "releaseSeconds": 3.0, "curveShape": 2.0}
    modRoutes = [{"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": 3.0, "scope": 1}]
    insert = [factory.chorus(mix=0.4, rate=0.25, depth=7.0)]
    master = house_master(pump=False)
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["CLUB", "AIR", "WASH", "SPACE", "GLOW", "MOVE", "TONE", "DECAY"]


GENRE_GENERATORS = {
    "bass": {
        "hoover": gen_hoover_bass,
        "house": gen_house_bass,
        "dream-pop": gen_dream_bass,
        "cinematic": gen_cinematic_bass,
    },
    "lead": {
        "dream-pop": gen_dream_lead,
        "house": gen_house_lead,
        "cinematic": gen_cinematic_lead,
    },
    "pad": {
        "dream-pop": gen_dream_pad,
        "cinematic": gen_cinematic_pad,
        "house": gen_house_pad,
    },
    "seq": {
        "house": gen_house_seq,
        "dream-pop": gen_dream_seq,
        "cinematic": gen_cinematic_seq,
    },
    "ambient": {
        "cinematic": gen_cinematic_ambient,
        "dream-pop": gen_dream_ambient,
        "house": gen_house_ambient,
    },
}

CATEGORIES = {
    "bass": "Basses",
    "lead": "Leads",
    "pad": "Pads",
    "seq": "Sequences",
    "ambient": "Ambient",
}

GENRE_MOODS = {
    "hoover": ["rave", "house", "anthemic"],
    "house": ["house", "club", "motor"],
    "dream-pop": ["dream-pop", "shoegaze", "neon"],
    "cinematic": ["cinematic", "epic", "score"],
}

GENRE_DESCRIPTION = {
    "hoover": "rave hoover bass — detuned saws, resonant filter sweep",
    "house": "house/club energy — pumped compression, gated motion",
    "dream-pop": "dream-pop shimmer — chorus wash and neon nostalgia",
    "cinematic": "cinematic scale — wide reverbs and score-ready swells",
}


def unpack_gen_result(result):
    unison = None
    arpeggiator = None
    if len(result) == 9:
        return (*result, arpeggiator, unison)
    if len(result) == 10:
        return (*result[:9], result[9], unison)
    return (*result[:9], result[9], result[10])


def build_expansion_patch(cat_key, cat_label, slot, display_name, genre, rng):
    local_index = slot - START_INDEX
    seed = hash((BATCH_TAG, cat_key, slot)) & 0xFFFFFFFF
    rng = random.Random(seed)

    gen_fn = GENRE_GENERATORS[cat_key][genre]
    result = gen_fn(local_index, rng)
    ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, macroNames, arpeggiator, unison = unpack_gen_result(result)

    engines_used = sorted({o["engine"] for o in ops})
    engine_names = ["Classic", "Wavetable", "FM/PM", "Additive", "PhaseShape",
                    "Granular", "NoiseChaos", "Resonator"]
    genre_phrase = GENRE_DESCRIPTION[genre]
    desc = (f"Genre expansion {cat_label[:-1] if cat_label.endswith('s') else cat_label} "
            f"patch #{slot:03d}. {genre_phrase.capitalize()}. "
            f"Engines: {', '.join(engine_names[e] for e in engines_used)}.")

    tags = [BATCH_TAG, cat_key, genre] + GENRE_TAGS[genre][:3]
    patch = factory.build_patch(
        name=display_name.upper(),
        description=desc,
        category=cat_key,
        moods=GENRE_MOODS[genre],
        tags=tags,
        seed=seed,
        operators=ops,
        ampEnv=ampEnv,
        filter1=filter1,
        lfo1=lfo1,
        lfo2=lfo2,
        modRoutes=modRoutes,
        insertEffects=insert,
        masterEffects=master,
        macroNames=macroNames,
        arpeggiator=arpeggiator,
        unison=unison,
    )
    patch["metadata"]["author"] = AUTHOR
    patch["metadata"]["id"] = f"pw8-{BATCH_TAG}-{cat_key}-{seed}"
    genres = list(dict.fromkeys(GENRE_TAGS[genre] + ["factory"]))
    patch["metadata"]["genres"] = genres
    patch["metadata"]["createdAt"] = "2026-08-13T00:00:00Z"
    return patch


def slugify(name):
    slug = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
    return slug or "patch"


def main():
    global factory
    factory = load_factory_module()
    used_names = load_existing_names()
    new_entries = []
    generated = []
    genre_counts = {g: 0 for g in GENRE_TAGS}

    for cat_key, cat_label in CATEGORIES.items():
        plan = CATEGORY_GENRE_PLAN[cat_key]
        assert len(plan) == COUNT_PER_CATEGORY, f"{cat_key} plan length {len(plan)} != {COUNT_PER_CATEGORY}"
        out_dir = OUT_ROOT / cat_label
        out_dir.mkdir(parents=True, exist_ok=True)

        for offset, genre in enumerate(plan):
            slot = START_INDEX + offset
            for stale in sorted(out_dir.glob(f"{slot:02d}-*.pw8")) + sorted(out_dir.glob(f"{slot:02d}-*.murmur")):
                stale.unlink()

            display_name = make_genre_name(genre, random.Random(hash((BATCH_TAG, "name", cat_key, slot))), used_names)
            patch = build_expansion_patch(cat_key, cat_label, slot, display_name, genre, None)
            fname = f"{slot:03d}-{slugify(display_name)}.murmur"
            path = out_dir / fname
            path.write_text(json.dumps(patch, indent=2) + "\n")
            generated.append(str(path.relative_to(REPO_ROOT)))
            new_entries.append({"category": cat_label, "file": f"factory/{cat_label}/{fname}",
                                "name": display_name.upper()})
            genre_counts[genre] += 1

    manifest = load_manifest()
    manifest = [e for e in manifest if not (
        START_INDEX <= int(e["file"].split("/")[-1].split("-")[0]) <= END_INDEX
    )]
    manifest.extend(new_entries)
    manifest.sort(key=lambda e: (e["category"], e["file"]))
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2) + "\n")

    print(f"Generated {len(generated)} genre expansion presets (slots {START_INDEX}-{END_INDEX} per category).")
    print(f"MANIFEST.json now lists {len(manifest)} total factory presets.")
    print("Genre totals:", ", ".join(f"{k}={v}" for k, v in sorted(genre_counts.items())))
    hoover_count = sum(1 for p in generated if "/Basses/" in p and "hoover" in pathlib.Path(REPO_ROOT / p).read_text()[:800])
    print(f"Hoover bass patches (approx): {hoover_count}")
    return generated


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
