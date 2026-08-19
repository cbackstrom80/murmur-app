#!/usr/bin/env python3
"""Generate 50 Mew/M83-inspired factory presets (slots 51-60 per category).

Adds to content/presets/factory/{Category}/51-*.pw8 through 60-*.pw8 without
touching existing 01-50 patches. Appends entries to MANIFEST.json.

Run from repo root:
    python3 scripts/generate_mew_m83_presets.py
"""
import importlib.util
import json
import os
import pathlib
import random
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_ROOT = REPO_ROOT / "content" / "presets" / "factory"
MANIFEST_PATH = OUT_ROOT / "MANIFEST.json"
AUTHOR = "MURMUR Sound Design"
BATCH_TAG = "mew-m83"
START_INDEX = 51
COUNT_PER_CATEGORY = 10

MEW_M83_TAGS = [
    "80s-pop", "dream-pop", "shoegaze", "cinematic", "neon", "anthemic",
    "gated-80s", "arp-bell", "poly-sweet", "hymn-swell", "fm-glass", "wt-evolve",
]

# Safe evocative titles — no band names.
CURATED_NAMES = {
    "bass": [
        "Neon Motor", "Midnight Pulse", "Comfort Sub", "Shimmer Low", "Glass Drive",
        "Velvet Depth", "Motor Funk", "Beam Weight", "Glow Bass", "Horizon Sub",
    ],
    "lead": [
        "Shimmer Beam", "Anthem Stack", "Prism Lead", "Neon Sync", "Comfort Line",
        "Aurora Fifth", "Cascade Wave", "Dream Portamento", "Pulse Horizon", "Velvet Chime",
    ],
    "pad": [
        "Neon Swell", "Midnight Veil", "Comfort Hymn", "Shimmer Ensemble", "Glass Bloom",
        "Aurora Stack", "Haze Wash", "Dream Glow", "Velvet Horizon", "Cascade Pad",
    ],
    "seq": [
        "Midnight Arp", "Neon Chime", "Gated Pulse", "Glass Sequence", "Motor Run",
        "Beam Cascade", "Comfort Motif", "Shimmer Step", "Aurora Cycle", "Velvet Gate",
    ],
    "ambient": [
        "Shimmer Wash", "Comfort Drift", "Neon Expanse", "Glass Cloud", "Hymn Swell",
        "Aurora Field", "Midnight Haze", "Cascade Echo", "Dream Riser", "Velvet Hymn",
    ],
}

ARCHETYPE_BY_SLOT = {
    "bass": [
        "rubber-funk", "fm-sub", "ladder-bass", "mono-growl", "sync-low",
        "rubber-funk", "fm-sub", "ladder-bass", "mono-growl", "fm-sub",
    ],
    "lead": [
        "sync-lead", "fifth-lead", "portamento", "sync-lead", "portamento",
        "fifth-lead", "sync-lead", "portamento", "fifth-lead", "formant-vox",
    ],
    "pad": [
        "poly-sweet", "dual-layer", "hymn-swell", "string-ensemble", "wt-evolve",
        "dual-layer", "poly-sweet", "wt-evolve", "string-ensemble", "dual-layer",
    ],
    "seq": [
        "gated-80s", "arp-bell", "gated-80s", "arp-bell", "sync-seq",
        "arp-bell", "fm-pluck", "gated-80s", "arp-bell", "gated-80s",
    ],
    "ambient": [
        "gran-cloud", "hymn-swell", "wt-wash", "gran-cloud", "hymn-swell",
        "resonant-cave", "wt-wash", "fx-riser", "gran-cloud", "hymn-swell",
    ],
}

SONIC_NOTES = {
    "bass": [
        "M83 motor bass — rubber funk pulse, sidechain-ready",
        "Tight FM sub — Mew precision low end",
        "Warm analog ladder — neon nostalgia foundation",
        "Mono growl with shimmer transient — shoegaze bass bite",
        "Glass FM drive — cinematic low-mid glow",
        "Velvet depth pad-bass hybrid — dream-pop warmth",
        "Motor funk pulse — 80s gated feel",
        "Beam weight — anthemic sub stack",
        "Glow bass — chorus-widened low end",
        "Horizon sub — long-release cinematic foundation",
    ],
    "lead": [
        "Shimmer sync lead — Mew soaring guitar-as-synth",
        "Anthem fifth stack — M83 widescreen lead",
        "Prism glass FM lead — fm-glass bell tone",
        "Neon hard-sync — Midnight City energy",
        "Comfort melodic line — childlike wonder portamento",
        "Aurora detuned fifths — cinematic stack",
        "Cascade wave lead — washed delay tails",
        "Dream portamento solo — precise but ethereal",
        "Pulse horizon lead — sidechain-friendly anthem",
        "Velvet chime FM — glass bell lead",
    ],
    "pad": [
        "Neon chorus swell — wide stereo M83 pad",
        "Midnight veil — dark dream-pop wash",
        "Comfort hymn pad — Mew long-release wonder",
        "Shimmer string ensemble — detuned shoegaze layers",
        "Glass bloom wavetable — evolving fm-glass body",
        "Aurora dual-layer stack — anthemic pad lift",
        "Haze wash — chorus-heavy poly-sweet",
        "Dream glow — shimmer unison pad",
        "Velvet horizon — cinematic wide pad",
        "Cascade pad — hymn-like swell with delay",
    ],
    "seq": [
        "Midnight arp — tempo-sync 80s pattern",
        "Neon chime bells — FM glass arp",
        "Gated 80s pulse — sidechain pump sequence",
        "Glass sequence — pluck bell arp",
        "Motor run — driving M83 motor line",
        "Beam cascade — ascending arp swell",
        "Comfort motif — melodic Mew arp",
        "Shimmer step — shoegaze arp wash",
        "Aurora cycle — wide stereo arp",
        "Velvet gate — gated reverb feel sequence",
    ],
    "ambient": [
        "Shimmer wash — shoegaze granular cloud",
        "Comfort drift — hymn-like slow swell",
        "Neon expanse — wide cinematic wash",
        "Glass cloud — fm-glass resonant texture",
        "Hymn swell — long-release ambient lift",
        "Aurora field — resonant cavern wash",
        "Midnight haze — neon nostalgia drone",
        "Cascade echo — delayed shoegaze wash",
        "Dream riser — cinematic FX swell",
        "Velvet hymn — dual-layer ambient hymn",
    ],
}


def load_factory_module():
    path = REPO_ROOT / "scripts" / "generate_factory_presets.py"
    spec = importlib.util.spec_from_file_location("factory_presets", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def load_existing_names():
    used = set()
    presets_root = REPO_ROOT / "content" / "presets"
    for pw8 in sorted(presets_root.rglob("*.pw8")) + sorted(presets_root.rglob("*.murmur")):
        # Allow reusing display names when replacing our own 51-60 factory slots.
        parts = pw8.parts
        if "factory" in parts:
            stem = pw8.stem  # e.g. 51-neon-motor
            slot = stem.split("-", 1)[0]
            if slot.isdigit() and START_INDEX <= int(slot) <= START_INDEX + COUNT_PER_CATEGORY - 1:
                continue
        try:
            meta = json.loads(pw8.read_text()).get("metadata", {})
            name = meta.get("name", "").strip()
            if name:
                used.add(name.lower())
        except (json.JSONDecodeError, OSError):
            pass
    return used


def load_manifest():
    if MANIFEST_PATH.exists():
        return json.loads(MANIFEST_PATH.read_text())
    return []


def m83_master(extra_chorus=False, pump=False):
    """M83-style master chain: EQ, chorus optional, compressor, limiter."""
    fx = [
        factory.eq(lowDb=1.0, midDb=-0.5, highDb=0.5),
    ]
    if extra_chorus:
        fx.append(factory.chorus(mix=0.35, rate=0.45, depth=6.0, base=14.0))
    fx.append(factory.comp(thresh=-14.0 if pump else -16.0, ratio=3.5 if pump else 2.8,
                           atk=8.0 if pump else 15.0, rel=120.0 if pump else 180.0))
    fx.append(factory.limiter())
    return fx


def mew_shimmer_insert(delay_mix=0.22, chorus_mix=0.4):
    return [
        factory.chorus(mix=chorus_mix, rate=0.35, depth=8.0, base=16.0),
        factory.tape_delay(mix=delay_mix, ms=420.0, fb=0.35, driftDepth=8.0, driftRate=0.15),
    ]


def gen_mew_m83_bass(i, rng):
    profiles = [
        ("classic", {"wf": factory.WAVEFORM_SAW, "cutoff": 380.0, "res": 0.35}),
        ("fm", {"ratio": 0.5, "index": 0.45, "cutoff": 220.0}),
        ("classic", {"wf": factory.WAVEFORM_SQR, "cutoff": 520.0, "res": 0.25}),
        ("wt", {"frame": 0.15, "cutoff": 450.0}),
        ("fm", {"ratio": 1.0, "index": 0.65, "cutoff": 340.0}),
        ("classic", {"wf": factory.WAVEFORM_TRI, "cutoff": 280.0, "res": 0.15}),
        ("classic", {"wf": factory.WAVEFORM_SAW, "cutoff": 650.0, "res": 0.4}),
        ("fm", {"ratio": 2.0, "index": 0.35, "cutoff": 260.0}),
        ("wt", {"frame": 0.35, "cutoff": 400.0}),
        ("classic", {"wf": factory.WAVEFORM_SINE, "cutoff": 180.0, "res": 0.08}),
    ]
    kind, params = profiles[i]
    ops = [{"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.55, "pan": 0.0}]
    if kind == "classic":
        ops.append({"engine": 0, "classicWaveform": params["wf"], "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.85, "pan": 0.0})
        if i in (0, 6):
            ops.append({"engine": 0, "classicWaveform": params["wf"], "frequencyRatio": 1.004,
                        "keyTrack": True, "level": 0.4, "pan": 0.12})
    elif kind == "fm":
        ops.append({"engine": 2, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.9, "pan": 0.0,
                    "fmModulatorRatio": params["ratio"], "fmModulatorIndex": params["index"],
                    "fmModulatorFeedback": 0.12, "fmModulatorWaveform": factory.WAVEFORM_SINE})
    else:
        ops.append({"engine": 1, "wavetableId": factory.wt_path("bass", rng),
                    "wavetableFramePosition": params["frame"], "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.88, "pan": 0.0})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": params["cutoff"],
               "resonance": params.get("res", 0.2), "keyTrack": 0.25}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.25, "syncDivisionIndex": 5, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 1.5, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": 0.004, "decaySeconds": 0.25, "sustainLevel": 0.75,
              "releaseSeconds": 0.18, "curveShape": 1.6}
    modRoutes = [{"source": factory.SRC_VELOCITY, "destination": factory.DST_FILTER_CUTOFF,
                  "targetIndex": 0, "amount": 10.0, "scope": 0}]
    if i in (0, 6):
        modRoutes.append({"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF,
                          "targetIndex": 0, "amount": 4.0, "scope": 0})
    insert = [factory.saturation(mix=0.25, drive=8.0)]
    if i in (0, 4, 8):
        insert.append(factory.chorus(mix=0.2, rate=0.3, depth=3.0))
    master = m83_master(pump=(i in (0, 2, 6)))
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["PUNCH", "GLOW", "SUB", "DRIVE", "WIDTH", "MOVE", "GRIT", "DECAY"]


def gen_mew_m83_lead(i, rng):
    profiles = [
        ("classic", 1.0, 0.55),
        ("classic", 1.5, 0.45),
        ("fm", 3.0, 0.55),
        ("phase", 1.0, 0.5),
        ("classic", 1.003, 0.4),
        ("classic", 1.498, 0.42),
        ("wt", 1.0, 0.5),
        ("classic", 1.006, 0.38),
        ("fm", 2.0, 0.48),
        ("fm", 4.01, 0.35),
    ]
    kind, detune, level = profiles[i]
    wf = factory.WAVEFORM_SAW if i % 2 == 0 else factory.WAVEFORM_TRI
    ops = []
    if kind == "classic":
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": level + 0.3, "pan": 0.0})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": detune,
                    "keyTrack": True, "level": level, "pan": 0.25})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 2.0 if detune > 1.1 else 0.997,
                    "keyTrack": True, "level": level * 0.7, "pan": -0.22})
    elif kind == "fm":
        ops.append({"engine": 2, "classicWaveform": wf, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.88, "pan": 0.0,
                    "fmModulatorRatio": detune, "fmModulatorIndex": level,
                    "fmModulatorFeedback": 0.18, "fmModulatorWaveform": factory.WAVEFORM_SINE})
    elif kind == "phase":
        ops.append({"engine": 4, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.9, "pan": 0.0,
                    "phaseBend": 0.25, "phaseFold": 0.35, "phaseAsymmetry": 0.1, "phaseShape": 0.55})
    else:
        ops.append({"engine": 1, "wavetableId": factory.wt_path("lead", rng),
                    "wavetableFramePosition": 0.35 + i * 0.05, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.88, "pan": 0.0})

    cutoff = 2800.0 + i * 180.0
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": 0.28, "keyTrack": 0.45}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 4.5, "syncDivisionIndex": 6, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.12, "syncDivisionIndex": 4, "phaseOffset": 0.2}
    ampEnv = {"attackSeconds": 0.015 if i != 7 else 0.06, "decaySeconds": 0.22,
              "sustainLevel": 0.82, "releaseSeconds": 0.35 if i != 7 else 0.55, "curveShape": 1.9}
    modRoutes = [
        {"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
         "amount": 6.0, "scope": 1},
        {"source": factory.SRC_VELOCITY, "destination": factory.DST_OP_LEVEL, "targetIndex": 0,
         "amount": 0.35, "scope": 0},
    ]
    insert = mew_shimmer_insert(delay_mix=0.18 + i * 0.01)
    master = [
        factory.eq(highDb=1.5),
        factory.reverb(mix=0.28, size=1.8, decay=3.5),
        factory.comp(ratio=3.0),
        factory.limiter(),
    ]
    unison = {"mode": 1, "voices": 3} if i in (0, 3, 7) else {"mode": 0, "voices": 1}
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["EDGE", "SHIMMER", "WIDTH", "SWEEP", "AIR", "BITE", "MOTION", "DRIVE"], None, unison


def gen_mew_m83_pad(i, rng):
    body = ["wt", "additive", "wt", "additive", "wt", "reso", "wt", "additive", "wt", "wt"][i]
    ops = [{"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.4, "pan": 0.0}]
    if body == "wt":
        ops.append({"engine": 1, "wavetableId": factory.wt_path("pad", rng),
                    "wavetableFramePosition": 0.1 + i * 0.07, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.82, "pan": rng.uniform(-0.2, 0.2)})
    elif body == "additive":
        ops.append({"engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.78, "pan": 0.0,
                    "additivePartialCount": 36, "additiveTilt": -0.2, "additiveOddEven": 0.65,
                    "additiveStretch": 0.01})
    else:
        ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.72, "pan": 0.0,
                    "resonatorStructure": 0.35, "resonatorDecay": 0.72, "resonatorDamping": 0.45,
                    "resonatorBrightness": 0.55, "resonatorModeCount": 6})
    ops.append({"engine": 4, "frequencyRatio": 1.002, "keyTrack": True, "level": 0.32,
                "pan": 0.28, "phaseBend": 0.15, "phaseFold": 0.25, "phaseAsymmetry": 0.05,
                "phaseShape": 0.4})
    if i % 3 == 0:
        ops.append({"engine": 2, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 2.0,
                    "keyTrack": True, "level": 0.22, "pan": -0.25,
                    "fmModulatorRatio": 3.5, "fmModulatorIndex": 0.3,
                    "fmModulatorFeedback": 0.08, "fmModulatorWaveform": factory.WAVEFORM_SINE})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1800.0 + i * 120.0,
               "resonance": 0.12, "keyTrack": 0.2}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.06, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.04, "syncDivisionIndex": 4, "phaseOffset": 0.35}
    ampEnv = {"attackSeconds": 2.0 + i * 0.15, "decaySeconds": 1.8, "sustainLevel": 0.85,
              "releaseSeconds": min(4.0, 4.5 + i * 0.2), "curveShape": 2.1, "legato": True}
    modRoutes = [
        {"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
         "amount": 7.0, "scope": 1},
        {"source": factory.SRC_LFO2, "destination": factory.DST_WT_POS, "targetIndex": 1,
         "amount": 0.35, "scope": 1},
    ]
    insert = mew_shimmer_insert(delay_mix=0.2, chorus_mix=0.45)
    master = [
        factory.eq(lowDb=0.8, midDb=-0.8, highDb=-0.5),
        factory.chorus(mix=0.3, rate=0.25, depth=5.0),
        factory.reverb(mix=0.48, size=2.2, decay=7.5, predelay=35.0),
        factory.comp(thresh=-15.0, ratio=2.5),
        factory.limiter(),
    ]
    unison = {"mode": 1, "voices": 4}
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["WARMTH", "BLOOM", "SHIMMER", "SPACE", "BODY", "MOTION", "AIR", "MORPH"], None, unison


def gen_mew_m83_seq(i, rng):
    voice = ["wt", "fm", "classic", "wt", "phase", "fm", "fm", "wt", "fm", "classic"][i]
    ops = []
    if voice == "wt":
        ops.append({"engine": 1, "wavetableId": factory.wt_path("seq", rng),
                    "wavetableFramePosition": 0.2 + i * 0.06, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.9, "pan": 0.0})
    elif voice == "fm":
        ops.append({"engine": 2, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.92, "pan": 0.0,
                    "fmModulatorRatio": [1.0, 2.0, 3.0, 4.01][i % 4],
                    "fmModulatorIndex": 0.45 + (i % 3) * 0.15,
                    "fmModulatorFeedback": 0.15, "fmModulatorWaveform": factory.WAVEFORM_SINE})
    elif voice == "phase":
        ops.append({"engine": 4, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.88, "pan": 0.0,
                    "phaseBend": 0.4, "phaseFold": 0.3, "phaseAsymmetry": 0.15, "phaseShape": 0.5})
    else:
        wf = factory.WAVEFORM_SQR if i == 2 else factory.WAVEFORM_SAW
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.88, "pan": 0.0})
    ops.append({"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
                "keyTrack": True, "level": 0.28, "pan": 0.0})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1400.0 + i * 200.0,
               "resonance": 0.22 + (i % 3) * 0.08, "keyTrack": 0.35}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 2.0, "syncDivisionIndex": 6, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.5, "syncDivisionIndex": 5, "phaseOffset": 0.1}
    ampEnv = {"attackSeconds": 0.002, "decaySeconds": 0.12 + (i % 4) * 0.05,
              "sustainLevel": 0.25 if i in (2, 9) else 0.4, "releaseSeconds": 0.08, "curveShape": 1.5}
    modRoutes = [{"source": factory.SRC_VELOCITY, "destination": factory.DST_FILTER_CUTOFF,
                  "targetIndex": 0, "amount": 12.0, "scope": 0}]
    insert = [
        factory.tape_delay(mix=0.28, ms=180.0 + i * 20.0, fb=0.4, driftRate=0.18),
    ]
    if i in (2, 9):
        insert.append(factory.chorus(mix=0.15, rate=0.8, depth=2.0))
    master = m83_master(pump=True, extra_chorus=(i % 2 == 0))

    arp_modes = [0, 1, 2, 3, 4, 5, 1, 2, 3, 4]
    arp_steps = [8, 8, 4, 12, 16, 8, 6, 8, 12, 4]
    arpeggiator = {
        "enabled": True, "mode": arp_modes[i], "rateMode": 1, "rateHz": 8.0,
        "syncDivisionIndex": [4, 5, 6, 5, 6, 7, 5, 6, 7, 4][i],
        "octaveRange": [1, 2, 1, 2, 3, 2, 1, 2, 2, 1][i],
        "numSteps": arp_steps[i], "latch": i in (4, 7),
    }
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["RATE", "SWING", "SPREAD", "BITE", "SPACE", "RANGE", "DECAY", "MOTION"], arpeggiator


def gen_mew_m83_ambient(i, rng):
    ops = [{"engine": 0, "classicWaveform": factory.WAVEFORM_SINE, "frequencyRatio": 0.5,
            "keyTrack": True, "level": 0.35, "pan": 0.0}]
    if i in (0, 3, 8):
        ops.append({"engine": 5, "wavetableId": factory.wt_path("gran", rng),
                    "wavetableFramePosition": 0.2 + i * 0.05, "frequencyRatio": 1.0,
                    "keyTrack": True, "level": 0.5, "pan": rng.uniform(-0.35, 0.35),
                    "grainDensity": 12.0 + i, "grainSizeMs": 120.0, "grainPositionJitter": 0.35,
                    "grainPitchJitter": 0.08})
    ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.55, "pan": 0.0,
                "resonatorStructure": 0.3 + i * 0.03, "resonatorDecay": 0.75, "resonatorDamping": 0.4,
                "resonatorBrightness": 0.5, "resonatorModeCount": 6})
    if i in (2, 6, 9):
        ops.append({"engine": 1, "wavetableId": factory.wt_path("ambient", rng),
                    "wavetableFramePosition": 0.4, "frequencyRatio": 0.5, "keyTrack": True,
                    "level": 0.38, "pan": 0.2})
    if i in (1, 4, 9):
        ops.append({"engine": 4, "frequencyRatio": 1.002, "keyTrack": True, "level": 0.28,
                    "pan": -0.2, "phaseBend": 0.2, "phaseFold": 0.35, "phaseAsymmetry": 0.1,
                    "phaseShape": 0.5})
    if i == 8:
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": 0.08,
                    "pan": 0.0, "noiseVariant": 2, "noiseRate": 40.0})

    filter1 = {"enabled": True, "mode": 0, "cutoffHz": 1600.0 + i * 100.0,
               "resonance": 0.1, "keyTrack": 0.15}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": 0.05, "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": 0.03, "syncDivisionIndex": 4, "phaseOffset": 0.45}
    attack = 4.0 if i != 8 else 0.5
    release = 6.5 if i != 8 else 3.0
    ampEnv = {"attackSeconds": attack, "decaySeconds": 2.5, "sustainLevel": 0.88,
              "releaseSeconds": release, "curveShape": 2.3}
    modRoutes = [
        {"source": factory.SRC_LFO1, "destination": factory.DST_FILTER_CUTOFF, "targetIndex": 0,
         "amount": 5.0, "scope": 1},
        {"source": factory.SRC_LFO2, "destination": factory.DST_WT_POS, "targetIndex": 1,
         "amount": 0.4, "scope": 1},
    ]
    insert = mew_shimmer_insert(delay_mix=0.18, chorus_mix=0.35)
    master = [
        factory.eq(lowDb=0.5, midDb=-1.0, highDb=-1.5),
        factory.reverb(mix=0.52, size=2.5, decay=9.0, predelay=45.0),
        factory.comp(thresh=-18.0, ratio=2.2),
        factory.limiter(),
    ]
    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["DRIFT", "DEPTH", "SHIMMER", "AIR", "STRUCTURE", "SPACE", "MORPH", "DAMPING"]


CATEGORIES = {
    "bass": ("Basses", gen_mew_m83_bass),
    "lead": ("Leads", gen_mew_m83_lead),
    "pad": ("Pads", gen_mew_m83_pad),
    "seq": ("Sequences", gen_mew_m83_seq),
    "ambient": ("Ambient", gen_mew_m83_ambient),
}


def build_mew_m83_patch(cat_key, cat_label, slot_index, name, rng, gen_fn):
    local_index = slot_index - START_INDEX
    seed = hash(("mew-m83", cat_key, slot_index)) & 0xFFFFFFFF
    rng = random.Random(seed)

    result = gen_fn(local_index, rng)
    unison = None
    arpeggiator = None
    if len(result) == 9:
        ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, macroNames = result
    elif len(result) == 10:
        ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, macroNames, arpeggiator = result
    else:
        ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, macroNames, arpeggiator, unison = result

    engines_used = sorted({o["engine"] for o in ops})
    engine_names = ["Classic", "Wavetable", "FM/PM", "Additive", "PhaseShape",
                    "Granular", "NoiseChaos", "Resonator"]
    archetype = ARCHETYPE_BY_SLOT[cat_key][local_index]
    sonic = SONIC_NOTES[cat_key][local_index]
    desc = (f"Mew/M83-inspired {cat_label[:-1] if cat_label.endswith('s') else cat_label} "
            f"patch #{slot_index:02d}. {sonic}. Engines: "
            f"{', '.join(engine_names[e] for e in engines_used)}. "
            f"Archetype: {factory.ARCHETYPE_PHRASES.get(archetype, archetype)}.")

    patch_tags = [BATCH_TAG, cat_key, archetype] + MEW_M83_TAGS[:4]
    moods = ["dream-pop", "shoegaze", "cinematic", "neon"]
    if cat_key == "bass":
        moods = ["80s-pop", "dream-pop", "anthemic"]
    elif cat_key == "seq":
        moods = ["80s-pop", "neon", "anthemic"]

    patch = factory.build_patch(
        name=name.upper(),
        description=desc,
        category=cat_key,
        moods=moods,
        tags=patch_tags,
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
    patch["metadata"]["id"] = f"pw8-mew-m83-{cat_key}-{seed}"
    patch["metadata"]["genres"] = ["dream-pop", "shoegaze", "80s-pop", "cinematic"]
    patch["metadata"]["createdAt"] = "2026-08-13T00:00:00Z"
    return patch


def main():
    global factory
    factory = load_factory_module()
    used_names = load_existing_names()
    new_entries = []
    generated_paths = []

    for cat_key, (cat_label, gen_fn) in CATEGORIES.items():
        out_dir = OUT_ROOT / cat_label
        out_dir.mkdir(parents=True, exist_ok=True)
        names = CURATED_NAMES[cat_key]

        for offset, display_name in enumerate(names):
            slot = START_INDEX + offset
            # Remove any prior file occupying this slot (including stale duplicates).
            for stale in sorted(out_dir.glob(f"{slot:02d}-*.pw8")) + sorted(out_dir.glob(f"{slot:02d}-*.murmur")):
                stale.unlink()

            name_key = display_name.upper()
            if name_key.lower() in used_names:
                # Should not happen with curated names; keep deterministic fallback.
                display_name = f"{display_name} Alt"
                name_key = display_name.upper()

            seed_rng = random.Random(hash(("mew-m83", cat_key, slot)) & 0xFFFFFFFF)
            patch = build_mew_m83_patch(cat_key, cat_label, slot, display_name, seed_rng, gen_fn)
            slug = display_name.lower().replace(" ", "-")
            fname = f"{slot:02d}-{slug}.murmur"
            path = out_dir / fname
            path.write_text(json.dumps(patch, indent=2) + "\n")
            generated_paths.append(str(path.relative_to(REPO_ROOT)))
            used_names.add(name_key.lower())

            rel_file = f"factory/{cat_label}/{fname}"
            new_entries.append({"category": cat_label, "file": rel_file, "name": name_key})

    # Rebuild manifest: preserve 01-50 factory entries, replace 51-60 with new batch.
    manifest = load_manifest()
    manifest = [e for e in manifest if int(e["file"].split("/")[-1].split("-")[0]) < START_INDEX]
    manifest.extend(new_entries)
    manifest.sort(key=lambda e: (e["category"], e["file"]))
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Generated {len(generated_paths)} Mew/M83 presets (slots {START_INDEX}-{START_INDEX + COUNT_PER_CATEGORY - 1} per category).")
    print(f"MANIFEST.json now lists {len(manifest)} total factory presets.")
    return generated_paths


if __name__ == "__main__":
    sys.exit(0 if main() else 1)
