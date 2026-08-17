# 0-Coast CLS Mod — Aspirational Goal (Horizon 3)

**Status:** DEFERRED — documented for future work; **not scheduled** for current priority gap closure.  
**Scope:** West Coast timbre path as a **sub-mode of `EngineType::Classic` (CLS)**, not a ninth engine.  
**Companion:** Make Noise 0-Coast manual / SOS review; `docs/MAKE_NOISE_POLIMATHS_RESEARCH.md`

---

## Why defer

Priority work (Design FX parity, PLAY/DESIGN Obsidian UI, APVTS/schema stability) ships first. The CLS mod is architecturally clean (~8–10 dev days for v1) but is **differentiation**, not a release blocker.

## Target (v1 — when picked up)

| 0-Coast block | MURMUR mapping |
|---------------|----------------|
| Triangle-core VCO (TRI + SQR outs) | `ClassicOscillator::renderDual()` — shared phase |
| Overtone (odd → even → slope `!!`) | `CoastShaper` stage 1 |
| Multiply (harmonic densify) | `CoastShaper` stage 2; slope CV default normalling |
| Balance (FUND vs OVRTN) | `CoastParams.balance` |
| Cycling slope (audio-rate LFO/osc) | `CoastSlopeGenerator` per voice in `OperatorState` |

**Out of scope for v1:** Contour envelope, Dynamics LPG/VCA, MIDI PGM pages, Control Processor summer.

## Parameters (schema bump when implemented)

~8 new operator fields on `OperatorPatch` / APVTS, active only when `engine == Classic && coast.enabled`:

- `CoastEnabled`, `CoastOvertone`, `CoastMultiply`, `CoastBalance`
- `CoastSlopeRise`, `CoastSlopeFall`, `CoastSlopeCurve`, `CoastSlopeCycle`, `CoastSlopeToMult`

## UI

- CLS sub-picker pill: **STD | COAST** in `EngineOscillatorPicker` (Figma `28:921`)
- Coast knobs replace waveform grid when active; hero viz shows dual trace + gold-wire path

## Acceptance presets (factory, when built)

Default Drone, Simple Bass, Slope Ring, Multiply Sweep, FM Coast (graph FM from op1)

## Effort estimate

~8–10 dev days (DSP 3–4, schema 1, UI 2, presets/tests 1–2)

---

See conversation plan (Aug 2026) for full signal-flow diagram and phased delivery breakdown.
