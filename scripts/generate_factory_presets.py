#!/usr/bin/env python3
"""Generates the 250-patch factory content bank under content/presets/factory/.

50 patches each across 5 categories -- Basses, Leads, Pads, Sequences, Ambient
-- procedurally built from the same per-engine parameter ranges used
throughout this project's own showcase/demo content, so every patch is a
real, playable combination rather than a random-noise stress test (that's
what pw8-fuzz-render is for). Each category has its own operator-engine
"recipe" pool (e.g. Basses lean Classic/FM/Wavetable/Resonator with fast
envelopes and a lowpass filter; Ambient leans Resonator/Granular/NoiseChaos
with multi-second envelopes and a large reverb tail; Sequences additionally
enable the arpeggiator). Every one of the 8 engine types appears somewhere
in the bank. Deterministic: each patch's seed is `hash((category, index))`,
so re-running this script reproduces the exact same 250 patches.

Validate afterward with a quick single-note render per patch (not done here
-- see the "Add factory presets" section of the packaging notes / git log
for the exact command), which is fast (~1s per patch) since it's a single
short note, not a full render.

Run from the repo root:
    python3 scripts/generate_factory_presets.py
"""
import json, os, random, pathlib

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_ROOT = str(REPO_ROOT / "content" / "presets" / "factory")

# ---------------------------------------------------------------- helpers --

def base_algorithm():
    return {"nodes": [{"id": i, "engine": 0, "isOutput": True} for i in range(8)], "edges": []}

def layer_b():
    return {
        "operators": [{"engine": 0, "level": 0.0} for _ in range(8)],
        "algorithm": {"nodes": [{"id": i, "engine": 0, "isOutput": (i == 0)} for i in range(8)], "edges": []},
        "envelopes": envelopes_from_amp(default_envelope_slot()),
        "gain": 1.0, "pan": 0.0, "width": 1.0, "centerGravity": 0.5,
    }

def macros(names):
    return [{"id": f"m{i+1}", "name": n, "description": "Performance macro.", "value": 0.0}
            for i, n in enumerate(names)]

def op_pad(ops):
    """Pad an 8-op list out to exactly 8 entries with silent Classic ops."""
    while len(ops) < 8:
        ops.append({"engine": 0, "classicWaveform": 0, "frequencyRatio": 1.0, "keyTrack": True, "level": 0.0, "pan": 0.0})
    return ops[:8]

def default_envelope_slot():
    return {"delaySeconds": 0.0, "attackSeconds": 0.002, "holdSeconds": 0.0, "decaySeconds": 0.15,
            "sustainLevel": 0.0, "releaseSeconds": 0.05, "curveShape": 2.0, "legato": False}

def default_lfo_slot():
    return {"waveform": 0, "mode": 0, "rateHz": 1.0, "syncDivisionIndex": 4, "phaseOffset": 0.0}

def envelopes_from_amp(amp_env):
    slots = [amp_env]
    while len(slots) < 8:
        slots.append(default_envelope_slot())
    return slots[:8]

def lfos_from_pair(lfo1, lfo2):
    slots = [lfo1, lfo2]
    while len(slots) < 8:
        slots.append(default_lfo_slot())
    return slots[:8]

def build_patch(name, description, category, moods, tags, seed, operators, ampEnv, filter1, lfo1, lfo2,
                 modRoutes, insertEffects, masterEffects, macroNames, unison=None, arpeggiator=None,
                 polyphony=8, gain=1.0, ui_focus=None):
    patch = {
        "schemaVersion": 2,
        "metadata": {
            "id": f"pw8-factory-{category}-{seed}",
            "name": name,
            "author": "Patchwork Eight Engineering",
            "description": description,
            "category": category,
            "moods": moods,
            "genres": ["factory"],
            "tags": tags,
            "createdAt": "2026-08-12T00:00:00Z",
            "engineVersion": "0.1.0",
            "schemaVersion": 2,
            "seed": seed,
            "lineage": [],
        },
        "layerMode": 0,
        "layerMorph": 0.0,
        "layerA": {
            "operators": op_pad(operators),
            "algorithm": base_algorithm(),
            "envelopes": envelopes_from_amp(ampEnv),
            "unison": unison or {"mode": 0, "voices": 1},
            "filter1": filter1,
            "lfos": lfos_from_pair(lfo1, lfo2),
            "modRoutes": modRoutes,
            "gain": gain, "pan": 0.0, "width": 1.0, "centerGravity": 0.5,
            "insertEffects": insertEffects,
        },
        "layerB": layer_b(),
        "voiceSettings": {"polyphony": polyphony, "masterGain": 1.0, "a4Hz": 440.0},
        "locks": {},
        "macros": macros(macroNames),
        "arpeggiator": arpeggiator or {"enabled": False},
        "masterEffects": masterEffects,
    }
    if ui_focus is not None:
        patch["uiFocus"] = ui_focus
    return patch

def infer_ui_focus(macro_names, mod_routes, category):
    """Patch-authored PLAY-mode Knobs of Interest hints."""
    knobs = []
    primary_macros = {
        "bass": [0, 2],
        "lead": [0, 1],
        "pad": [0, 3],
        "seq": [0, 4],
        "ambient": [0, 5],
    }
    for idx in primary_macros.get(category, [0])[:2]:
        if idx < len(macro_names):
            knobs.append({"kind": "macro", "index": idx, "label": macro_names[idx]})
    for route in mod_routes:
        dest = route.get("destination", 0)
        if dest in (1, 2) and len(knobs) < 5:
            param = "filterCutoffHz" if dest == 1 else "filterResonance"
            label = "Cutoff" if dest == 1 else "Resonance"
            if not any(k.get("paramId") == param for k in knobs if k.get("kind") == "param"):
                knobs.append({"kind": "param", "paramId": param, "label": label})
        elif dest == 5 and len(knobs) < 6:
            knobs.append({"kind": "param", "paramId": "layerPan", "label": "Pan"})
    if len(knobs) < 3:
        for idx in range(min(3, len(macro_names))):
            if not any(k.get("index") == idx for k in knobs if k.get("kind") == "macro"):
                knobs.append({"kind": "macro", "index": idx, "label": macro_names[idx]})
    return {"maxKnobs": min(6, max(4, len(knobs))), "knobs": knobs[:6]}

def reverb(mix, size, decay, predelay=25.0):
    return {"type": 7, "mix": mix, "reverbSizeParam": size, "reverbDecaySeconds": decay,
            "reverbPreDelayMs": predelay, "reverbHighRatio": 0.55, "reverbHighCrossoverHz": 3200.0,
            "reverbLowRatio": 1.4, "reverbLowCrossoverHz": 300.0, "reverbDiffusion": 0.8,
            "reverbDensity": 0.85, "reverbModDepth": 0.3, "reverbModRateHz": 0.15,
            "reverbEarlyLevel": 0.5, "reverbLateLevel": 0.85, "reverbRollOffHz": 9000.0, "reverbVlfCutDb": -6.0}

def eq(lowHz=100, lowDb=0.0, midHz=600, midDb=0.0, midQ=0.7, highHz=6000, highDb=0.0):
    return {"type": 8, "mix": 1.0, "eqLowFreqHz": lowHz, "eqLowGainDb": lowDb, "eqMidFreqHz": midHz,
            "eqMidGainDb": midDb, "eqMidQ": midQ, "eqHighFreqHz": highHz, "eqHighGainDb": highDb}

def comp(thresh=-16.0, ratio=2.5, atk=15.0, rel=180.0, knee=6.0, makeup=3.0):
    return {"type": 9, "mix": 1.0, "compThresholdDb": thresh, "compRatio": ratio, "compAttackMs": atk,
            "compReleaseMs": rel, "compKneeDb": knee, "compMakeupDb": makeup}

def limiter(ceiling=-0.8, lookahead=4.0, rel=60.0):
    return {"type": 10, "mix": 1.0, "limiterCeilingDb": ceiling, "limiterLookaheadMs": lookahead, "limiterReleaseMs": rel}

def saturation(mix=0.3, drive=10.0):
    return {"type": 1, "mix": mix, "saturationDriveDb": drive}

def chorus(mix=0.3, rate=0.5, depth=4.0, base=12.0):
    return {"type": 2, "mix": mix, "chorusRateHz": rate, "chorusDepthMs": depth, "chorusBaseDelayMs": base}

def tape_delay(mix=0.2, ms=350.0, fb=0.3, drive=4.0, duck=0.25, driftDepth=6.0, driftRate=0.12, panMode=2):
    return {"type": 3, "mix": mix, "tapeDelayMs": ms, "tapeFeedback": fb, "tapeDriveDb": drive,
            "tapeDuckAmount": duck, "tapeDriftDepthMs": driftDepth, "tapeDriftRateHz": driftRate, "tapePanMode": panMode}

def freq_shift_echo(mix=0.2, hz=7.0, delay=280.0, fb=0.5, lowCut=120.0, highCut=8000.0):
    return {"type": 5, "mix": mix, "freqShiftHz": hz, "freqShiftDelayMs": delay, "freqShiftFeedback": fb,
            "freqShiftLowCutHz": lowCut, "freqShiftHighCutHz": highCut}

# ModSource / ModDestination ordinals (schema v2 — ModMatrixTypes.hpp)
SRC_LFO1, SRC_LFO2 = 1, 2
SRC_VELOCITY = 17
DST_FILTER_CUTOFF, DST_FILTER_RES, DST_OP_LEVEL, DST_PAN, DST_WT_POS = 1, 2, 3, 4, 5

WAVEFORM_SINE, WAVEFORM_TRI, WAVEFORM_SAW, WAVEFORM_SQR = 0, 1, 2, 3

WT = {
    "bass": ["bass-growl-saw.json", "bass-square-punch.json", "bass-sub-sine-plus.json",
             "digital-stairstep-harsh.json", "cheby-aggressive-drive.json", "cheby-warm-drive.json"],
    "lead": ["pwm-full-sweep.json", "pwm-narrow-sweep.json", "pwm-twin-detune.json", "pwm-wide-sweep.json",
             "classic-saw-to-square.json", "classic-sine-to-saw.json", "metallic-ratio-stack-a.json",
             "metallic-ratio-stack-b.json", "cheby-odd-harmonics.json", "cheby-even-harmonics.json",
             "digital-gritty-noise-gate.json"],
    "pad": ["ambient-airy-drift.json", "ambient-dreamy-veil.json", "ambient-evolving-swell.json",
            "texture-drone-evolve.json", "chord-fifth-stack.json", "chord-octave-stack.json",
            "formant-vowel-aa.json", "formant-vowel-oo.json", "formant-vowel-morph-a-to-e.json",
            "classic-sine-to-triangle.json"],
    "seq": ["pluck-bright-string.json", "pluck-glassy-mallet.json", "pluck-mellow-keys.json",
            "bell-bright-tine.json", "bell-dark-gong.json", "bell-fm-classic.json", "bell-glass-chime.json",
            "bell-metallic-strike.json", "metallic-detuned-cluster.json"],
    "ambient": ["texture-frozen-noise-bright.json", "texture-frozen-noise-warm.json",
                "texture-tonal-to-noise.json", "ambient-airy-drift.json",
                "ambient-dreamy-veil.json", "ambient-evolving-swell.json", "fold-dual-sine.json",
                "fold-saw-soft.json", "fold-sine-sharp.json", "fold-sine-triangle.json",
                "formant-vowel-morph-o-to-u.json"],
    "gran": ["gran-cloud-drift.json", "gran-glass-spray.json", "gran-vocal-dust.json",
             "gran-tape-warmth.json", "gran-crystal-burst.json", "gran-sub-rumble.json",
             "gran-digital-glitch.json", "gran-ocean-swell.json", "gran-frozen-grit.json",
             "gran-shimmer-voice.json", "texture-granular-shimmer.json"],
}

def wt_path(pool_key, rng):
    return f"content/wavetables/{rng.choice(WT[pool_key])}"

# ------------------------------------------------------------- name banks --

# Classic synth archetype tags for browser filtering (no trademark names).
ARCHETYPE_TAGS = {
    "bass": ["ladder-bass", "rubber-funk", "fm-sub", "acid-line", "sync-low", "mono-growl"],
    "lead": ["sync-lead", "fifth-lead", "brass-stab", "formant-vox", "portamento", "chip-lead"],
    "pad": ["poly-sweet", "dual-layer", "string-ensemble", "brass-pad", "wt-evolve", "formant-choir"],
    "seq": ["acid-seq", "gated-80s", "arp-bell", "sync-seq", "fm-pluck", "stab-seq"],
    "ambient": ["hymn-swell", "wt-wash", "noise-drone", "fx-riser", "gran-cloud", "resonant-cave"],
}

# Human-readable archetype phrases for description suffix (parallel to ARCHETYPE_TAGS).
ARCHETYPE_PHRASES = {
    "ladder-bass": "ladder-filter mono bass",
    "rubber-funk": "rubber funk bass with portamento",
    "fm-sub": "FM sub bass pulse",
    "acid-line": "resonant acid sequence bass",
    "sync-low": "sync oscillator low lead-bass",
    "mono-growl": "mono growl bass",
    "sync-lead": "hard sync lead",
    "fifth-lead": "detuned fifth stack lead",
    "brass-stab": "dual-saw brass stab",
    "formant-vox": "formant vocal lead",
    "portamento": "portamento solo line",
    "chip-lead": "8-bit square lead",
    "poly-sweet": "poly sweet chorus pad",
    "dual-layer": "dual-layer swell pad",
    "string-ensemble": "detuned string ensemble",
    "brass-pad": "brass-pad hybrid",
    "wt-evolve": "evolving wavetable pad",
    "formant-choir": "formant choir pad",
    "acid-seq": "acid squelch sequence",
    "gated-80s": "gated 80s pulse sequence",
    "arp-bell": "arp bell pluck sequence",
    "sync-seq": "sync oscillator sequence",
    "fm-pluck": "FM pluck sequence",
    "stab-seq": "chord stab sequence",
    "hymn-swell": "slow hymn swell ambient",
    "wt-wash": "wavetable wash drone",
    "noise-drone": "noise texture drone",
    "fx-riser": "noise riser FX",
    "gran-cloud": "granular cloud ambient",
    "resonant-cave": "resonant cavern ambient",
}

# Synthesis-evocative vocabulary (~30% of generated display names).
SYNTH_ADJECTIVES = {
    "bass": ["Bronze", "Squelch", "Ladder", "Rubber", "Coil", "Fifth", "Ratio", "Metallic", "Accent"],
    "lead": ["Sync", "Bronze", "Glass", "Formant", "Fifth", "Voltage", "Pixel", "Tine", "Rip"],
    "pad": ["Chorus", "Dual", "Velvet", "Drawbar", "Haze", "Swell", "Ensemble", "Bucket", "Marble"],
    "seq": ["Gate", "Chime", "Accent", "Motor", "Ratchet", "Glass", "Stab", "Tine", "Squelch"],
    "ambient": ["Photon", "Hollow", "Granular", "Resonant", "Void", "Swell", "Mist", "Drift", "Hymn"],
}

SYNTH_NOUNS = {
    "bass": ["Squelch", "Pulse", "Line", "Sub", "Growl", "Drive", "Low", "Depth"],
    "lead": ["Tear", "Stab", "Beam", "Scream", "Stack", "Section", "Line", "Sync"],
    "pad": ["Swell", "Ensemble", "Hymn", "Veil", "Stack", "Chord", "Wash", "Glow"],
    "seq": ["Motif", "Chime", "Motor", "Pattern", "Stab", "Cycle", "Line", "Step"],
    "ambient": ["Riser", "Cave", "Cloud", "Drift", "Hymn", "Wash", "Expanse", "Field"],
}

NAME_BANKS = {
    "bass": (["Sub", "Growl", "Thud", "Wobble", "Crawl", "Fang", "Rumble", "Grind", "Mono", "Anchor",
              "Undertow", "Sludge", "Root", "Broad", "Fathom", "Basalt", "Iron", "Slack", "Coil", "Trench"],
             ["Bass", "Low", "Sub", "Growl", "Pulse", "Drive", "Weight", "Depth"]),
    "lead": (["Glass", "Razor", "Spike", "Chrome", "Neon", "Static", "Solar", "Voltage", "Prism", "Comet",
              "Flare", "Wire", "Pixel", "Vector", "Halo", "Nova", "Signal", "Blade", "Ion", "Spark"],
             ["Lead", "Line", "Edge", "Cut", "Sync", "Beam", "Spike", "Wave"]),
    "pad": (["Halcyon", "Aurora", "Velvet", "Distant", "Cloud", "Amber", "Pale", "Hollow", "Wistful",
             "Faded", "Slow", "Gentle", "Vast", "Quiet", "Dawn", "Dusk", "Marble", "Feather", "Opal", "Mist"],
            ["Pad", "Sky", "Veil", "Glow", "Bloom", "Drift", "Sway", "Field"]),
    "seq": (["Ratchet", "Clockwork", "Loom", "Cascade", "Circuit", "Pixel", "Bounce", "Skitter", "Tumble",
             "Stutter", "Orbit", "Relay", "Pendulum", "Mosaic", "Ripple", "Chime", "Ticker", "Weave", "Trellis", "Gear"],
            ["Sequence", "Pulse", "Step", "Run", "Loop", "Motif", "Cycle", "Pattern"]),
    "ambient": (["Sietch", "Nebula", "Glacier", "Vapor", "Hollow", "Tundra", "Abyssal", "Ochre", "Lichen",
                 "Ash", "Threnody", "Cavern", "Fathomless", "Fen", "Wraith", "Ember", "Basin", "Shale", "Vellum", "Sere"],
                ["Drone", "Drift", "Expanse", "Bloom", "Hymn", "Wash", "Field", "Echo"]),
}

def make_name(category, rng, used):
    adjs, nouns = NAME_BANKS[category]
    synth_adjs = SYNTH_ADJECTIVES.get(category, adjs)
    synth_nouns = SYNTH_NOUNS.get(category, nouns)
    for _ in range(200):
        if rng.random() < 0.30:
            name = f"{rng.choice(synth_adjs)} {rng.choice(synth_nouns)}"
        else:
            name = f"{rng.choice(adjs)} {rng.choice(nouns)}"
        if name not in used:
            used.add(name)
            return name
    name = f"{rng.choice(adjs)} {rng.choice(nouns)} {rng.randint(2, 99)}"
    used.add(name)
    return name

def pick_archetype_tags(category, rng, count=2):
    pool = list(ARCHETYPE_TAGS.get(category, []))
    if not pool:
        return []
    n = min(count, len(pool))
    return rng.sample(pool, n)

def archetype_description_phrase(tags):
    phrases = [ARCHETYPE_PHRASES[t] for t in tags if t in ARCHETYPE_PHRASES]
    if not phrases:
        return tags[0].replace("-", " ") if tags else "general synthesis"
    return phrases[0] if len(phrases) == 1 else f"{phrases[0]} with {phrases[1]} character"

# --------------------------------------------------------- category generators --

def gen_bass(i, rng):
    engine_choice = rng.choices(["classic", "fm", "wt", "reso"], weights=[45, 25, 20, 10])[0]
    ops = [{"engine": 0, "classicWaveform": WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True,
            "level": rng.uniform(0.5, 0.8), "pan": 0.0}]

    if engine_choice == "classic":
        wf = rng.choice([WAVEFORM_SAW, WAVEFORM_SQR, WAVEFORM_TRI])
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True,
                    "level": rng.uniform(0.6, 0.95), "pan": 0.0})
        if rng.random() < 0.5:
            ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.003, "keyTrack": True,
                        "level": rng.uniform(0.3, 0.6), "pan": rng.uniform(-0.2, 0.2)})
    elif engine_choice == "fm":
        ops.append({"engine": 2, "classicWaveform": WAVEFORM_SINE, "frequencyRatio": 1.0, "keyTrack": True,
                    "level": rng.uniform(0.7, 0.95), "pan": 0.0,
                    "fmModulatorRatio": rng.choice([1.0, 2.0, 0.5, 3.0]), "fmModulatorIndex": rng.uniform(0.3, 1.2),
                    "fmModulatorFeedback": rng.uniform(0.0, 0.35), "fmModulatorWaveform": WAVEFORM_SINE})
    elif engine_choice == "wt":
        ops.append({"engine": 1, "wavetableId": wt_path("bass", rng), "wavetableFramePosition": rng.uniform(0.0, 0.5),
                    "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.7, 0.95), "pan": 0.0})
    else:  # resonator "plucked bass"
        ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.7, 0.95), "pan": 0.0,
                    "resonatorStructure": rng.uniform(0.1, 0.4), "resonatorDecay": rng.uniform(0.3, 0.6),
                    "resonatorDamping": rng.uniform(0.4, 0.8), "resonatorBrightness": rng.uniform(0.3, 0.6),
                    "resonatorModeCount": rng.randint(3, 6)})

    if rng.random() < 0.3:  # transient click layer
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": rng.uniform(0.03, 0.1),
                    "pan": 0.0, "noiseVariant": rng.choice([0, 6]), "noiseRate": rng.uniform(20.0, 200.0)})

    cutoff = rng.uniform(140.0, 950.0)
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": rng.uniform(0.05, 0.55),
               "keyTrack": rng.uniform(0.0, 0.4)}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.1, 4.0), "syncDivisionIndex": rng.randint(3, 7),
            "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.1, 3.0), "syncDivisionIndex": 4, "phaseOffset": 0.0}
    ampEnv = {"attackSeconds": rng.uniform(0.001, 0.012), "decaySeconds": rng.uniform(0.05, 0.45),
              "sustainLevel": rng.uniform(0.3, 0.9), "releaseSeconds": rng.uniform(0.04, 0.3), "curveShape": 1.5}
    modRoutes = []
    if rng.random() < 0.6:
        modRoutes.append({"source": SRC_VELOCITY, "destination": DST_FILTER_CUTOFF, "targetIndex": 0,
                          "amount": rng.uniform(6.0, 18.0), "scope": 0})
    if rng.random() < 0.3:
        modRoutes.append({"source": SRC_LFO1, "destination": DST_FILTER_CUTOFF, "targetIndex": 0,
                          "amount": rng.uniform(2.0, 8.0), "scope": 0})

    insert = []
    if rng.random() < 0.7:
        insert.append(saturation(mix=rng.uniform(0.15, 0.45), drive=rng.uniform(4.0, 16.0)))
    master = [eq(lowDb=rng.uniform(0.0, 3.0), midDb=rng.uniform(-2.0, 0.5), highDb=rng.uniform(-3.0, 0.0)),
              comp(thresh=rng.uniform(-20.0, -10.0), ratio=rng.uniform(2.0, 4.0)), limiter()]
    if rng.random() < 0.25:
        master.insert(1, reverb(mix=rng.uniform(0.05, 0.15), size=rng.uniform(0.8, 1.6), decay=rng.uniform(1.5, 3.5)))

    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["PUNCH", "GROWL", "SUB", "GRIT", "MOVE", "WIDTH", "DRIVE", "DECAY"]

def gen_lead(i, rng):
    engine_choice = rng.choices(["classic", "fm", "phase", "additive", "wt"], weights=[30, 25, 20, 15, 10])[0]
    ops = []
    wf = rng.choice([WAVEFORM_SAW, WAVEFORM_SQR, WAVEFORM_TRI])

    if engine_choice == "classic":
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True,
                    "level": rng.uniform(0.7, 0.95), "pan": 0.0})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.006, "keyTrack": True,
                    "level": rng.uniform(0.4, 0.7), "pan": rng.uniform(0.15, 0.4)})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 0.997, "keyTrack": True,
                    "level": rng.uniform(0.4, 0.7), "pan": rng.uniform(-0.4, -0.15)})
    elif engine_choice == "fm":
        ops.append({"engine": 2, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True,
                    "level": rng.uniform(0.75, 0.95), "pan": 0.0,
                    "fmModulatorRatio": rng.choice([1.0, 2.0, 3.0, 4.0]), "fmModulatorIndex": rng.uniform(0.2, 0.8),
                    "fmModulatorFeedback": rng.uniform(0.0, 0.3), "fmModulatorWaveform": WAVEFORM_SINE})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.004, "keyTrack": True,
                    "level": rng.uniform(0.25, 0.45), "pan": rng.uniform(0.2, 0.4)})
    elif engine_choice == "phase":
        ops.append({"engine": 4, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.75, 0.95), "pan": 0.0,
                    "phaseBend": rng.uniform(-0.5, 0.5), "phaseFold": rng.uniform(0.1, 0.6),
                    "phaseAsymmetry": rng.uniform(-0.3, 0.3), "phaseShape": rng.uniform(0.2, 0.8)})
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.005, "keyTrack": True,
                    "level": rng.uniform(0.2, 0.4), "pan": rng.uniform(-0.35, 0.35)})
    elif engine_choice == "additive":
        ops.append({"engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.75, 0.95), "pan": 0.0,
                    "additivePartialCount": rng.randint(16, 40), "additiveTilt": rng.uniform(0.0, 0.4),
                    "additiveOddEven": rng.uniform(0.4, 0.9), "additiveStretch": rng.uniform(-0.05, 0.05)})
    else:
        ops.append({"engine": 1, "wavetableId": wt_path("lead", rng), "wavetableFramePosition": rng.uniform(0.0, 1.0),
                    "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.75, 0.95), "pan": 0.0})

    if rng.random() < 0.2:
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": rng.uniform(0.02, 0.06),
                    "pan": 0.0, "noiseVariant": 3, "noiseRate": rng.uniform(30.0, 120.0)})

    cutoff = rng.uniform(1200.0, 7000.0)
    filter1 = {"enabled": True, "mode": rng.choice([0, 0, 0, 2]), "cutoffHz": cutoff,
               "resonance": rng.uniform(0.1, 0.6), "keyTrack": rng.uniform(0.1, 0.6)}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(2.0, 6.5), "syncDivisionIndex": rng.randint(4, 8),
            "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.05, 0.5), "syncDivisionIndex": 4, "phaseOffset": 0.3}
    ampEnv = {"attackSeconds": rng.uniform(0.003, 0.09), "decaySeconds": rng.uniform(0.08, 0.5),
              "sustainLevel": rng.uniform(0.4, 0.9), "releaseSeconds": rng.uniform(0.08, 0.55), "curveShape": 1.8}
    modRoutes = [{"source": rng.choice([SRC_LFO1, SRC_VELOCITY]), "destination": DST_FILTER_CUTOFF,
                  "targetIndex": 0, "amount": rng.uniform(3.0, 12.0), "scope": 1}]
    if rng.random() < 0.4:
        modRoutes.append({"source": SRC_VELOCITY, "destination": DST_OP_LEVEL, "targetIndex": 0,
                          "amount": rng.uniform(0.2, 0.6), "scope": 0})

    insert = []
    if rng.random() < 0.5:
        insert.append(chorus(mix=rng.uniform(0.2, 0.5), rate=rng.uniform(0.2, 1.2), depth=rng.uniform(2.0, 10.0)))
    if rng.random() < 0.5:
        insert.append(tape_delay(mix=rng.uniform(0.1, 0.3), ms=rng.uniform(150.0, 480.0), fb=rng.uniform(0.15, 0.4)))
    master = [eq(highDb=rng.uniform(-1.0, 2.0)), reverb(mix=rng.uniform(0.15, 0.35), size=rng.uniform(1.0, 2.0),
              decay=rng.uniform(1.5, 4.5)), comp(ratio=rng.uniform(2.0, 3.5)), limiter()]

    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["EDGE", "BITE", "WIDTH", "MOTION", "GRIT", "AIR", "SWEEP", "DRIVE"]

def gen_pad(i, rng):
    ops = [{"engine": 0, "classicWaveform": WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True,
            "level": rng.uniform(0.3, 0.55), "pan": 0.0}]
    body_choice = rng.choices(["wt", "additive", "reso"], weights=[45, 35, 20])[0]
    if body_choice == "wt":
        ops.append({"engine": 1, "wavetableId": wt_path("pad", rng), "wavetableFramePosition": rng.uniform(0.0, 0.4),
                    "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.6, 0.9), "pan": rng.uniform(-0.15, 0.15)})
    elif body_choice == "additive":
        ops.append({"engine": 3, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.6, 0.9), "pan": 0.0,
                    "additivePartialCount": rng.randint(24, 56), "additiveTilt": rng.uniform(-0.5, 0.1),
                    "additiveOddEven": rng.uniform(0.4, 0.8), "additiveStretch": rng.uniform(-0.03, 0.03)})
    else:
        ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.5, 0.8), "pan": 0.0,
                    "resonatorStructure": rng.uniform(0.2, 0.6), "resonatorDecay": rng.uniform(0.5, 0.85),
                    "resonatorDamping": rng.uniform(0.3, 0.6), "resonatorBrightness": rng.uniform(0.4, 0.7),
                    "resonatorModeCount": rng.randint(4, 8)})
    if rng.random() < 0.55:
        ops.append({"engine": 4, "frequencyRatio": rng.choice([1.0, 1.002, 2.0]), "keyTrack": True,
                    "level": rng.uniform(0.2, 0.45), "pan": rng.uniform(-0.3, 0.3),
                    "phaseBend": rng.uniform(-0.3, 0.3), "phaseFold": rng.uniform(0.0, 0.4),
                    "phaseAsymmetry": rng.uniform(-0.2, 0.2), "phaseShape": rng.uniform(0.0, 0.6)})
    if rng.random() < 0.45:
        ops.append({"engine": 2, "classicWaveform": WAVEFORM_SINE, "frequencyRatio": rng.choice([2.0, 3.0]),
                    "keyTrack": True, "level": rng.uniform(0.15, 0.35), "pan": rng.uniform(-0.3, 0.3),
                    "fmModulatorRatio": rng.uniform(2.0, 6.0), "fmModulatorIndex": rng.uniform(0.15, 0.5),
                    "fmModulatorFeedback": rng.uniform(0.0, 0.15), "fmModulatorWaveform": WAVEFORM_SINE})
    if rng.random() < 0.3:
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": rng.uniform(0.03, 0.09),
                    "pan": 0.0, "noiseVariant": 1, "noiseRate": rng.uniform(15.0, 60.0)})

    cutoff = rng.uniform(1200.0, 4200.0)
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": rng.uniform(0.05, 0.3),
               "keyTrack": rng.uniform(0.1, 0.35)}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.03, 0.15), "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.02, 0.1), "syncDivisionIndex": 4, "phaseOffset": 0.4}
    ampEnv = {"attackSeconds": rng.uniform(1.2, 3.8), "decaySeconds": rng.uniform(1.0, 2.5),
              "sustainLevel": rng.uniform(0.65, 0.9), "releaseSeconds": rng.uniform(2.0, 6.0), "curveShape": 2.0}
    modRoutes = [{"source": SRC_LFO1, "destination": DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": rng.uniform(4.0, 10.0), "scope": 1}]
    if len(ops) > 2 and rng.random() < 0.5:
        modRoutes.append({"source": SRC_LFO2, "destination": DST_WT_POS, "targetIndex": 1,
                          "amount": rng.uniform(0.2, 0.5), "scope": 1})

    insert = [tape_delay(mix=rng.uniform(0.1, 0.25), ms=rng.uniform(300.0, 600.0), fb=rng.uniform(0.15, 0.35))]
    master = [eq(lowDb=rng.uniform(0.0, 1.5), midDb=rng.uniform(-1.5, 0.0), highDb=rng.uniform(-2.0, 0.5)),
              reverb(mix=rng.uniform(0.35, 0.55), size=rng.uniform(1.6, 2.6), decay=rng.uniform(5.0, 9.5)),
              comp(thresh=rng.uniform(-18.0, -12.0), ratio=rng.uniform(2.0, 3.0)), limiter()]

    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["WARMTH", "BLOOM", "MOTION", "SPACE", "BODY", "SHIMMER", "AIR", "MORPH"]

def gen_seq(i, rng):
    ops = []
    voice_choice = rng.choices(["reso", "fm", "phase", "wt", "classic"], weights=[30, 25, 20, 15, 10])[0]
    if voice_choice == "reso":
        ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.75, 0.95), "pan": 0.0,
                    "resonatorStructure": rng.uniform(0.2, 0.7), "resonatorDecay": rng.uniform(0.2, 0.55),
                    "resonatorDamping": rng.uniform(0.3, 0.7), "resonatorBrightness": rng.uniform(0.4, 0.8),
                    "resonatorModeCount": rng.randint(3, 7)})
    elif voice_choice == "fm":
        ops.append({"engine": 2, "classicWaveform": WAVEFORM_SINE, "frequencyRatio": 1.0, "keyTrack": True,
                    "level": rng.uniform(0.75, 0.95), "pan": 0.0, "fmModulatorRatio": rng.choice([1.0, 1.5, 2.0, 3.0, 4.01]),
                    "fmModulatorIndex": rng.uniform(0.3, 1.0), "fmModulatorFeedback": rng.uniform(0.0, 0.25),
                    "fmModulatorWaveform": WAVEFORM_SINE})
    elif voice_choice == "phase":
        ops.append({"engine": 4, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.75, 0.95), "pan": 0.0,
                    "phaseBend": rng.uniform(-0.6, 0.6), "phaseFold": rng.uniform(0.1, 0.5),
                    "phaseAsymmetry": rng.uniform(-0.3, 0.3), "phaseShape": rng.uniform(0.1, 0.7)})
    elif voice_choice == "wt":
        ops.append({"engine": 1, "wavetableId": wt_path("seq", rng), "wavetableFramePosition": rng.uniform(0.0, 0.6),
                    "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.75, 0.95), "pan": 0.0})
    else:
        wf = rng.choice([WAVEFORM_SAW, WAVEFORM_SQR, WAVEFORM_TRI])
        ops.append({"engine": 0, "classicWaveform": wf, "frequencyRatio": 1.0, "keyTrack": True,
                    "level": rng.uniform(0.75, 0.95), "pan": 0.0})
    ops.append({"engine": 0, "classicWaveform": WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True,
                "level": rng.uniform(0.2, 0.4), "pan": 0.0})

    cutoff = rng.uniform(900.0, 5500.0)
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": rng.uniform(0.1, 0.5),
               "keyTrack": rng.uniform(0.2, 0.5)}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.5, 4.0), "syncDivisionIndex": rng.randint(3, 8),
            "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.1, 1.0), "syncDivisionIndex": 4, "phaseOffset": 0.2}
    ampEnv = {"attackSeconds": rng.uniform(0.001, 0.015), "decaySeconds": rng.uniform(0.06, 0.35),
              "sustainLevel": rng.uniform(0.15, 0.6), "releaseSeconds": rng.uniform(0.03, 0.2), "curveShape": 1.6}
    modRoutes = [{"source": SRC_VELOCITY, "destination": DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": rng.uniform(4.0, 14.0), "scope": 0}]

    insert = [tape_delay(mix=rng.uniform(0.2, 0.4), ms=rng.uniform(120.0, 380.0), fb=rng.uniform(0.25, 0.55),
              driftRate=rng.uniform(0.05, 0.3))]
    if rng.random() < 0.25:
        insert.append(freq_shift_echo(mix=rng.uniform(0.1, 0.25), hz=rng.uniform(-30.0, 30.0)))
    master = [eq(highDb=rng.uniform(0.0, 2.0)), reverb(mix=rng.uniform(0.15, 0.3), size=rng.uniform(0.8, 1.8),
              decay=rng.uniform(1.5, 3.5)), comp(ratio=rng.uniform(2.5, 4.0)), limiter()]

    arp_mode = rng.choice([0, 1, 2, 3, 4, 5, 6])
    numSteps = rng.choice([4, 6, 8, 8, 12, 16])
    arpeggiator = {"enabled": True, "mode": arp_mode, "rateMode": 1, "rateHz": 8.0,
                   "syncDivisionIndex": rng.choice([4, 5, 6, 7]), "octaveRange": rng.randint(1, 3),
                   "numSteps": numSteps, "latch": rng.random() < 0.2}

    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["RATE", "SWING", "SPREAD", "BITE", "SPACE", "RANGE", "DECAY", "MOTION"], arpeggiator

def gen_ambient(i, rng):
    ops = [{"engine": 0, "classicWaveform": WAVEFORM_SINE, "frequencyRatio": 0.5, "keyTrack": True,
            "level": rng.uniform(0.25, 0.45), "pan": 0.0}]
    ops.append({"engine": 7, "frequencyRatio": 1.0, "keyTrack": True, "level": rng.uniform(0.4, 0.65), "pan": rng.uniform(-0.25, 0.25),
                "resonatorStructure": rng.uniform(0.2, 0.65), "resonatorDecay": rng.uniform(0.55, 0.9),
                "resonatorDamping": rng.uniform(0.3, 0.65), "resonatorBrightness": rng.uniform(0.35, 0.65),
                "resonatorModeCount": rng.randint(4, 8)})
    ops.append({"engine": 5, "wavetableId": wt_path("gran", rng), "wavetableFramePosition": rng.uniform(0.0, 0.5),
                "frequencyRatio": rng.choice([1.0, 2.0]), "keyTrack": True, "level": rng.uniform(0.3, 0.55),
                "pan": rng.uniform(-0.45, 0.45), "grainDensity": rng.uniform(4.0, 22.0),
                "grainSizeMs": rng.uniform(60.0, 220.0), "grainPositionJitter": rng.uniform(0.15, 0.5),
                "grainPitchJitter": rng.uniform(0.0, 0.15)})
    if rng.random() < 0.6:
        ops.append({"engine": 6, "frequencyRatio": 1.0, "keyTrack": False, "level": rng.uniform(0.04, 0.11),
                    "pan": rng.uniform(-0.3, 0.3), "noiseVariant": rng.choice([1, 2, 3]), "noiseRate": rng.uniform(15.0, 60.0)})
    if rng.random() < 0.5:
        ops.append({"engine": 4, "frequencyRatio": rng.choice([1.0, 1.002, 1.5]), "keyTrack": True,
                    "level": rng.uniform(0.15, 0.35), "pan": rng.uniform(-0.35, 0.35),
                    "phaseBend": rng.uniform(-0.3, 0.3), "phaseFold": rng.uniform(0.1, 0.5),
                    "phaseAsymmetry": rng.uniform(-0.2, 0.2), "phaseShape": rng.uniform(0.1, 0.7)})
    if rng.random() < 0.4:
        ops.append({"engine": 1, "wavetableId": wt_path("ambient", rng), "wavetableFramePosition": rng.uniform(0.0, 1.0),
                    "frequencyRatio": rng.choice([0.5, 1.0, 1.5]), "keyTrack": True, "level": rng.uniform(0.25, 0.5),
                    "pan": rng.uniform(-0.3, 0.3)})

    cutoff = rng.uniform(1400.0, 3800.0)
    filter1 = {"enabled": True, "mode": 0, "cutoffHz": cutoff, "resonance": rng.uniform(0.05, 0.25),
               "keyTrack": rng.uniform(0.05, 0.3)}
    lfo1 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.02, 0.09), "syncDivisionIndex": 4, "phaseOffset": 0.0}
    lfo2 = {"waveform": 0, "mode": 0, "rateHz": rng.uniform(0.015, 0.06), "syncDivisionIndex": 4, "phaseOffset": 0.5}
    ampEnv = {"attackSeconds": rng.uniform(2.5, 7.0), "decaySeconds": rng.uniform(1.5, 3.5),
              "sustainLevel": rng.uniform(0.7, 0.92), "releaseSeconds": rng.uniform(4.0, 9.0), "curveShape": 2.2}
    modRoutes = [{"source": SRC_LFO1, "destination": DST_FILTER_CUTOFF, "targetIndex": 0,
                  "amount": rng.uniform(3.0, 8.0), "scope": 1},
                 {"source": SRC_LFO2, "destination": DST_WT_POS, "targetIndex": 2,
                  "amount": rng.uniform(0.3, 0.6), "scope": 1}]

    insert = [tape_delay(mix=rng.uniform(0.1, 0.22), ms=rng.uniform(350.0, 650.0), fb=rng.uniform(0.2, 0.4),
              driftDepth=rng.uniform(4.0, 12.0), driftRate=rng.uniform(0.05, 0.2))]
    master = [eq(lowDb=rng.uniform(0.0, 1.5), midDb=rng.uniform(-1.5, -0.3), highDb=rng.uniform(-2.5, 0.0)),
              reverb(mix=rng.uniform(0.4, 0.6), size=rng.uniform(2.0, 3.0), decay=rng.uniform(7.0, 12.0),
              predelay=rng.uniform(20.0, 55.0)), comp(thresh=rng.uniform(-20.0, -14.0)), limiter()]

    return ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, \
        ["DRIFT", "DEPTH", "SHIMMER", "AIR", "STRUCTURE", "DAMPING", "SPACE", "MORPH"]

CATEGORIES = {
    "bass": ("Basses", gen_bass),
    "lead": ("Leads", gen_lead),
    "pad": ("Pads", gen_pad),
    "seq": ("Sequences", gen_seq),
    "ambient": ("Ambient", gen_ambient),
}

COUNT_PER_CATEGORY = 50
manifest = []

for cat_key, (cat_label, gen_fn) in CATEGORIES.items():
    out_dir = f"{OUT_ROOT}/{cat_label}"
    os.makedirs(out_dir, exist_ok=True)
    for stale in pathlib.Path(out_dir).glob("*.pw8"):
        stale.unlink()
    used_names = set()
    for i in range(COUNT_PER_CATEGORY):
        seed = hash((cat_key, i)) & 0xFFFFFFFF
        rng = random.Random(seed)
        name = make_name(cat_key, rng, used_names)

        result = gen_fn(i, rng)
        if len(result) == 9:
            ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, macroNames = result
            arpeggiator = None
        else:
            ops, ampEnv, filter1, lfo1, lfo2, modRoutes, insert, master, macroNames, arpeggiator = result

        engines_used = sorted({o["engine"] for o in ops})
        engine_names = ["Classic", "Wavetable", "FM/PM", "Additive", "PhaseShape", "Granular", "NoiseChaos", "Resonator"]
        archetype_tags = pick_archetype_tags(cat_key, rng, count=rng.choice([1, 2]))
        archetype_phrase = archetype_description_phrase(archetype_tags)
        desc = (f"Factory {cat_label[:-1] if cat_label.endswith('s') else cat_label} patch #{i+1:02d}. "
                f"Engines: {', '.join(engine_names[e] for e in engines_used)}. Procedurally generated "
                f"(seed {seed}) as part of the 250-patch factory content bank. "
                f"Archetype: {archetype_phrase}.")

        patch_tags = ["factory", cat_key] + archetype_tags
        full_name = f"{name.upper()}"
        patch = build_patch(
            name=full_name, description=desc, category=cat_key,
            moods=[cat_label.lower()], tags=patch_tags,
            seed=seed, operators=ops, ampEnv=ampEnv, filter1=filter1, lfo1=lfo1, lfo2=lfo2,
            modRoutes=modRoutes, insertEffects=insert, masterEffects=master, macroNames=macroNames,
            arpeggiator=arpeggiator,
            ui_focus=infer_ui_focus(macroNames, modRoutes, cat_key),
        )
        fname = f"{i+1:02d}-{name.lower().replace(' ', '-')}.pw8"
        path = f"{out_dir}/{fname}"
        with open(path, "w") as f:
            json.dump(patch, f, indent=2)
        manifest.append({"category": cat_label, "file": f"factory/{cat_label}/{fname}", "name": full_name})

print(f"Generated {len(manifest)} patches across {len(CATEGORIES)} categories.")
with open(f"{OUT_ROOT}/MANIFEST.json", "w") as f:
    json.dump(manifest, f, indent=2)
