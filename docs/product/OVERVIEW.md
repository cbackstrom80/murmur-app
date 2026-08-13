# MURMUR — Overview

## What is MURMUR?

**MURMUR** is an **8-engine algorithmic synthesizer** for macOS. It combines FM-style operator graphs, wavetables, additive and phase-distortion synthesis, resonators, and noise — routed through a global filter, a full modulation matrix, and a studio-grade FX chain — in a single instrument designed for **performance first, depth second**.

Think of it as a modern polyphonic synth where every patch is a small **algorithm**: eight operators wired together, eight envelopes, eight LFOs, eight macros, and hundreds of automatable parameters — but surfaced on the **PLAY** screen as six **Knobs of Interest** so you can shape sound without opening a manual.

---

## Tagline

**Eight engines. One voice.**

---

## Who is it for?

| You are… | MURMUR gives you… |
|----------|-------------------|
| A **Logic Pro** producer | Native Audio Unit, 800 factory presets, Smart Controls-friendly macros |
| A **keyboard player** | Mod wheel, expression, aftertouch, and CC-mapped macros out of the box |
| A **sound designer** | Eight engine types, seven algorithm topologies, mod matrix, `.pw8` patch format |
| A **preset browser** | Category/mood/tag search, favorites, prev/next stepping |

---

## What's included (Ben MVP 1.0.0)

| Component | Details |
|-----------|---------|
| **Plug-in** | Audio Unit (`MURMUR.component`) — Apple Silicon arm64 |
| **Factory presets** | **800** patches across Basses, Leads, Pads, Sequences, Ambient (160 each) |
| **Wavetables** | 61 JSON wavetable files |
| **Showcase presets** | Additional demo and genre patches |
| **Documentation** | Product guides + Logic / MP11SE setup |

Maintainer builds also include **VST3** and **Standalone** (`scripts/build_release_pkg.sh --full`).

---

## Core capabilities

### Synthesis

- **8 operator slots** per layer — Classic, Wavetable, FM, Phase, Additive, Noise, Granular, Resonator
- **7 algorithm graphs** — serial FM stacks, parallel carriers, feedback bells, triple-carrier, and more
- **Dual layer + Stack mode** — Layer B summed for unison stacks and split textures
- **Unison** — per-layer detune and spread

### Modulation

- **8 LFOs + 8 envelopes** per layer
- **Mod matrix** — route any source to filter, operator level, pan, wavetable position
- **Performance sources** — velocity, aftertouch, mod wheel (CC1), expression (CC11), CC74 brightness, macros 1–8

### Effects

- **3 insert + 4 master FX slots**
- Tape delay, chorus, saturation, frequency-shift echo, node delay tree, reverb, EQ, compressor, limiter

### Performance UI (OBSIDIAN skin)

- **PLAY mode** — algorithm graph, Knobs of Interest, patch browser
- **Mod Matrix** — drag sources onto knobs, live connection list
- **BROWSE / LOAD** — searchable preset index with favorites

---

## What MURMUR is not (yet)

- Not a sample player or rompler
- Not a full DAW — it's an instrument plug-in
- No built-in sequencer (use your DAW + optional arpeggiator per patch)
- Intel Mac builds not included in Ben MVP release
- In-plugin auto-update not yet available — check GitHub Releases

---

## Next steps

→ [**Quick Start**](QUICK_START.md) — install and make your first sound in Logic  
→ [**Presets**](PRESETS.md) — explore the 800-patch factory bank  
→ [**Performance**](PERFORMANCE.md) — mod wheel, MP11SE, Smart Controls
