"""The pieces of .pw8's schema (docs/PATCH_FORMAT.md) this MCP server needs to
know about, mirrored from the real C++ source of truth rather than guessed:

  - engine/include/pw8/algorithm/AlgorithmTypes.hpp  (EngineType)
  - engine/include/pw8/modulation/ModMatrixTypes.hpp (ModSource, ModDestination)
  - engine/include/pw8/effects/EffectTypes.hpp        (EffectType)
  - engine/include/pw8/filter/StateVariableFilter.hpp (FilterMode)
  - engine/src/patch/PatchSerializer.cpp               (every clampNum(...) range)

Deliberately no C++ build dependency (no pybind11 requirement) -- this module
is plain Python data plus plain-Python patch construction/editing, so the MCP
server only needs `pw8-render` already built (tools/) to do anything beyond
introspection. See docs/MCP_AND_NL_PATCH_GENERATION.md Part A for why this
targets the JSON schema directly rather than bindings/python's still-PARTIAL
Operator wrapper.
"""
from __future__ import annotations

# -- EngineType -------------------------------------------------------------

ENGINE_NAMES: dict[int, str] = {
    0: "classic",
    1: "wavetable",
    2: "fm_pm",
    3: "additive",
    4: "phase_shape",
    5: "granular",
    6: "noise_chaos",
    7: "resonator",
}
ENGINE_IDS: dict[str, int] = {name: idx for idx, name in ENGINE_NAMES.items()}
ENGINE_ALIASES: dict[str, str] = {
    "fm": "fm_pm", "fmpm": "fm_pm", "fm/pm": "fm_pm",
    "noise": "noise_chaos", "chaos": "noise_chaos", "noisechaos": "noise_chaos",
    "phaseshape": "phase_shape", "phase": "phase_shape", "cz": "phase_shape",
    "wt": "wavetable", "table": "wavetable",
    "reso": "resonator", "modal": "resonator",
    "add": "additive", "harmonic": "additive",
    "grain": "granular", "grains": "granular",
}

WAVEFORM_NAMES = {0: "sine", 1: "triangle", 2: "saw", 3: "square"}
WAVEFORM_IDS = {name: idx for idx, name in WAVEFORM_NAMES.items()}

NOISE_VARIANT_NAMES = {
    0: "white", 1: "pink", 2: "brown", 3: "blue",
    4: "sample_and_hold", 5: "smooth_random", 6: "dust",
}
NOISE_VARIANT_IDS = {name: idx for idx, name in NOISE_VARIANT_NAMES.items()}

# Base fields every operator has, regardless of engine (only some are
# *meaningful* per engine -- see OperatorNode::render()'s switch -- but all are
# always present in the schema).
BASE_OPERATOR_FIELDS: dict[str, dict] = {
    "classicWaveform": {"type": "int", "range": [0, 3], "default": 2,
                         "note": "0=sine 1=triangle 2=saw 3=square. Read by Classic (own shape) and FM/PM (carrier shape)."},
    "classicMorph": {"type": "float", "range": [-1.0, 1.0], "default": -1.0},
    "pulseWidth": {"type": "float", "range": [0.01, 0.99], "default": 0.5},
    "wavetableFramePosition": {"type": "float", "range": [0.0, 1.0], "default": 0.0,
                                "note": "Wavetable engine: frame position. Granular engine: reused as each grain's base read position."},
    "wavetableId": {"type": "string", "default": "",
                     "note": "e.g. 'content/wavetables/bell-glass-chime.json' -- required for Wavetable/Granular to produce anything."},
    "frequencyRatio": {"type": "float", "range": [0.001, 128.0], "default": 1.0,
                        "note": "Read generically by every engine for carrier/root pitch, before the engine switch."},
    "fixedFrequencyHz": {"type": "float", "range": [0.01, 24000.0], "default": 440.0,
                          "note": "Only used when keyTrack is false."},
    "keyTrack": {"type": "bool", "default": True},
    "level": {"type": "float", "range": [0.0, 4.0], "default": 1.0},
    "pan": {"type": "float", "range": [-1.0, 1.0], "default": 0.0},
}

# Fields specific to one engine, appended on top of the base set above.
ENGINE_EXTRA_FIELDS: dict[str, dict[str, dict]] = {
    "fm_pm": {
        "fmModulatorRatio": {"type": "float", "range": [0.001, 32.0], "default": 1.0,
                              "note": "Modulator freq / carrier freq."},
        "fmModulatorIndex": {"type": "float", "range": [0.0, 2.0], "default": 0.5,
                              "note": "Modulation depth."},
        "fmModulatorFeedback": {"type": "float", "range": [0.0, 1.0], "default": 0.0},
        "fmModulatorWaveform": {"type": "int", "range": [0, 3], "default": 0,
                                 "note": "Same 0-3 waveform codes as classicWaveform."},
    },
    "noise_chaos": {
        "noiseVariant": {"type": "int", "range": [0, 6], "default": 0,
                          "note": "0=white 1=pink 2=brown 3=blue 4=sample_and_hold 5=smooth_random 6=dust."},
        "noiseRate": {"type": "float", "range": [0.5, 2000.0], "default": 200.0,
                       "note": "Hz. Only meaningful for sample_and_hold/smooth_random/dust variants."},
    },
    "phase_shape": {
        "phaseBend": {"type": "float", "range": [-1.0, 1.0], "default": 0.0, "note": "CZ-style phase warp."},
        "phaseFold": {"type": "float", "range": [0.0, 1.0], "default": 0.0, "note": "Post-generation wavefold amount."},
        "phaseAsymmetry": {"type": "float", "range": [-1.0, 1.0], "default": 0.0},
        "phaseShape": {"type": "float", "range": [0.0, 1.0], "default": 0.0, "note": "Blend between two base shaping curves."},
    },
    "additive": {
        "additivePartialCount": {"type": "int", "range": [1, 64], "default": 32},
        "additiveTilt": {"type": "float", "range": [-1.0, 1.0], "default": 0.0,
                          "note": "Spectral tilt; negative = darker, positive = brighter."},
        "additiveOddEven": {"type": "float", "range": [0.0, 1.0], "default": 0.5,
                             "note": "0 = odd harmonics only, 1 = all harmonics."},
        "additiveStretch": {"type": "float", "range": [-1.0, 1.0], "default": 0.0,
                             "note": "Inharmonicity (piano-string-style per-partial stretch)."},
    },
    "resonator": {
        "resonatorStructure": {"type": "float", "range": [0.0, 1.0], "default": 0.3, "note": "Inharmonicity/detune of upper modes."},
        "resonatorDecay": {"type": "float", "range": [0.0, 1.0], "default": 0.5, "note": "Overall mode decay time."},
        "resonatorDamping": {"type": "float", "range": [0.0, 1.0], "default": 0.5, "note": "High-frequency damping tilt."},
        "resonatorBrightness": {"type": "float", "range": [0.0, 1.0], "default": 0.5, "note": "Exciter noise burst's spectral tilt."},
        "resonatorModeCount": {"type": "int", "range": [2, 8], "default": 6},
    },
    "granular": {
        "grainDensity": {"type": "float", "range": [0.5, 200.0], "default": 20.0, "note": "Grains/sec."},
        "grainSizeMs": {"type": "float", "range": [1.0, 500.0], "default": 60.0},
        "grainPositionJitter": {"type": "float", "range": [0.0, 1.0], "default": 0.1},
        "grainPitchJitter": {"type": "float", "range": [0.0, 1.0], "default": 0.0, "note": "Semitone-scale random detune per grain."},
    },
}

# -- ModSource / ModDestination ----------------------------------------------

MOD_SOURCE_IDS: dict[str, int] = {"none": 0}
for _i in range(1, 9):
    MOD_SOURCE_IDS[f"lfo{_i}"] = _i
for _i in range(1, 9):
    MOD_SOURCE_IDS[f"env{_i}"] = 8 + _i
MOD_SOURCE_IDS.update({
    "velocity": 17, "channel_pressure": 18, "poly_aftertouch": 19, "mpe_slide": 20,
})
for _i in range(1, 9):
    MOD_SOURCE_IDS[f"macro{_i}"] = 20 + _i
MOD_SOURCE_NAMES = {v: k for k, v in MOD_SOURCE_IDS.items()}

MOD_DEST_IDS: dict[str, int] = {
    "none": 0, "filter_cutoff": 1, "filter_resonance": 2,
    "operator_filter_cutoff": 3, "operator_filter_resonance": 4,
    "operator_level": 5, "pan": 6,
    "operator_wavetable_position": 7, "operator_wavetable_bend": 8,
    "operator_wavetable_asymmetry": 9, "operator_wavetable_sync_ratio": 10,
    "operator_wavetable_formant": 11, "operator_wavetable_sync_amount": 12,
    "master_fx_mix": 13, "master_reverb_mix": 14, "master_reverb_size": 15,
    "master_reverb_decay": 16, "master_reverb_pre_delay": 17,
    "master_reverb_diffusion": 18, "master_reverb_mod_depth": 19,
    "master_gain": 20,
}
MOD_DEST_NAMES = {v: k for k, v in MOD_DEST_IDS.items()}
# Destinations that require targetIndex (operator index 0-7, or master slot 0-3).
MOD_DEST_NEEDS_TARGET = {3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}

MOD_SCOPE_IDS = {"voice": 0, "layer": 1, "global": 2}
MOD_SCOPE_NAMES = {v: k for k, v in MOD_SCOPE_IDS.items()}

# -- EffectType ---------------------------------------------------------------

EFFECT_TYPE_IDS: dict[str, int] = {
    "bypass": 0, "saturation": 1, "chorus": 2, "tape_delay": 3, "node_delay": 4,
    "freq_shift_echo": 5, "fractal_echo": 6, "reverb": 7, "eq": 8,
    "compressor": 9, "limiter": 10, "binaural_space": 11,
}
EFFECT_TYPE_NAMES = {v: k for k, v in EFFECT_TYPE_IDS.items()}

EFFECT_FIELDS: dict[str, dict[str, dict]] = {
    "saturation": {"saturationDriveDb": {"type": "float", "range": [0.0, 36.0], "default": 10.0}},
    "chorus": {
        "chorusRateHz": {"type": "float", "range": [0.01, 10.0], "default": 0.5},
        "chorusDepthMs": {"type": "float", "range": [0.0, 20.0], "default": 4.0},
        "chorusBaseDelayMs": {"type": "float", "range": [1.0, 40.0], "default": 12.0},
    },
    "tape_delay": {
        "tapeDelayMs": {"type": "float", "range": [1.0, 4000.0], "default": 350.0},
        "tapeFeedback": {"type": "float", "range": [0.0, 0.98], "default": 0.3},
        "tapeDriveDb": {"type": "float", "range": [0.0, 24.0], "default": 4.0},
        "tapeDuckAmount": {"type": "float", "range": [0.0, 1.0], "default": 0.25},
        "tapeDriftDepthMs": {"type": "float", "range": [0.0, 20.0], "default": 6.0},
        "tapeDriftRateHz": {"type": "float", "range": [0.01, 5.0], "default": 0.12},
        "tapePanMode": {"type": "int", "range": [0, 2], "default": 2},
    },
    "freq_shift_echo": {
        "freqShiftHz": {"type": "float", "range": [-2000.0, 2000.0], "default": 7.0},
        "freqShiftDelayMs": {"type": "float", "range": [1.0, 4000.0], "default": 280.0},
        "freqShiftFeedback": {"type": "float", "range": [0.0, 0.98], "default": 0.55},
        "freqShiftLowCutHz": {"type": "float", "range": [5.0, 20000.0], "default": 120.0},
        "freqShiftHighCutHz": {"type": "float", "range": [20.0, 20000.0], "default": 8000.0},
    },
    "reverb": {
        "reverbSizeParam": {"type": "float", "range": [0.0, 3.0], "default": 1.5},
        "reverbDecaySeconds": {"type": "float", "range": [0.1, 30.0], "default": 3.0},
        "reverbPreDelayMs": {"type": "float", "range": [0.0, 250.0], "default": 20.0},
        "reverbHighRatio": {"type": "float", "range": [0.1, 1.0], "default": 0.5},
        "reverbHighCrossoverHz": {"type": "float", "range": [500.0, 12000.0], "default": 3000.0},
        "reverbLowRatio": {"type": "float", "range": [0.5, 2.5], "default": 1.5},
        "reverbLowCrossoverHz": {"type": "float", "range": [50.0, 2000.0], "default": 300.0},
        "reverbDiffusion": {"type": "float", "range": [0.0, 1.0], "default": 0.8},
        "reverbDensity": {"type": "float", "range": [0.0, 1.0], "default": 0.85},
        "reverbModDepth": {"type": "float", "range": [0.0, 1.0], "default": 0.3},
        "reverbModRateHz": {"type": "float", "range": [0.01, 5.0], "default": 0.15},
        "reverbEarlyLevel": {"type": "float", "range": [0.0, 1.0], "default": 0.5},
        "reverbLateLevel": {"type": "float", "range": [0.0, 1.0], "default": 0.85},
        "reverbRollOffHz": {"type": "float", "range": [1000.0, 20000.0], "default": 9000.0},
        "reverbVlfCutDb": {"type": "float", "range": [-24.0, 0.0], "default": -6.0},
    },
    "eq": {
        "eqLowFreqHz": {"type": "float", "range": [20.0, 500.0], "default": 100.0},
        "eqLowGainDb": {"type": "float", "range": [-24.0, 24.0], "default": 0.0},
        "eqMidFreqHz": {"type": "float", "range": [200.0, 8000.0], "default": 600.0},
        "eqMidGainDb": {"type": "float", "range": [-24.0, 24.0], "default": 0.0},
        "eqMidQ": {"type": "float", "range": [0.1, 10.0], "default": 0.7},
        "eqHighFreqHz": {"type": "float", "range": [1000.0, 20000.0], "default": 6000.0},
        "eqHighGainDb": {"type": "float", "range": [-24.0, 24.0], "default": 0.0},
    },
    "compressor": {
        "compThresholdDb": {"type": "float", "range": [-60.0, 0.0], "default": -16.0},
        "compRatio": {"type": "float", "range": [1.0, 20.0], "default": 2.5},
        "compAttackMs": {"type": "float", "range": [0.1, 200.0], "default": 15.0},
        "compReleaseMs": {"type": "float", "range": [10.0, 2000.0], "default": 180.0},
        "compKneeDb": {"type": "float", "range": [0.0, 24.0], "default": 6.0},
        "compMakeupDb": {"type": "float", "range": [0.0, 24.0], "default": 3.0},
    },
    "limiter": {
        "limiterCeilingDb": {"type": "float", "range": [-6.0, 0.0], "default": -0.8},
        "limiterLookaheadMs": {"type": "float", "range": [0.0, 20.0], "default": 4.0},
        "limiterReleaseMs": {"type": "float", "range": [1.0, 1000.0], "default": 60.0},
    },
}

# -- FilterMode -----------------------------------------------------------

FILTER_MODE_IDS = {"lowpass": 0, "highpass": 1, "bandpass": 2, "notch": 3, "peak": 4}
FILTER_MODE_NAMES = {v: k for k, v in FILTER_MODE_IDS.items()}


def normalize_name(value: str) -> str:
    return value.strip().lower().replace(" ", "_").replace("-", "_")


def resolve_engine(value) -> int:
    """Accepts an int 0-7 or a string name/alias, returns the int id."""
    if isinstance(value, int):
        if value not in ENGINE_NAMES:
            raise ValueError(f"engine id {value} out of range 0-7")
        return value
    key = normalize_name(str(value))
    key = ENGINE_ALIASES.get(key, key)
    if key not in ENGINE_IDS:
        raise ValueError(f"unknown engine '{value}' -- valid names: {sorted(ENGINE_IDS)}")
    return ENGINE_IDS[key]


def resolve_from_map(value, id_map: dict[str, int], kind: str) -> int:
    if isinstance(value, int):
        return value
    key = normalize_name(str(value))
    if key not in id_map:
        raise ValueError(f"unknown {kind} '{value}' -- valid names: {sorted(id_map)}")
    return id_map[key]


def clamp(value, spec: dict):
    """Clamps `value` to spec['range'] if present; returns (clamped_value, warning|None)."""
    if "range" not in spec:
        return value, None
    lo, hi = spec["range"]
    if value < lo or value > hi:
        clamped = max(lo, min(hi, value))
        return clamped, f"value {value} out of range [{lo}, {hi}], clamped to {clamped}"
    return value, None
