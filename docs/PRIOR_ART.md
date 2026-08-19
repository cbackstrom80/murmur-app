# Prior Art & Design Lineage

This document records where MURMUR's architecture deliberately draws on
prior work, per the master spec's requirement that inspiration be explicit and that
nothing be cloned from a commercial product without attribution and reimplementation.

## Mutable Instruments `eurorack` (pichenettes/eurorack)

At the user's request, MURMUR's engine-type architecture and several DSP
technique choices were reviewed against
[pichenettes/eurorack](https://github.com/pichenettes/eurorack), Émilie Gillet's
open-source Eurorack module firmware (Mutable Instruments). This is Eurorack module
firmware, not a software synthesizer, and its STM32F-target code (which includes the
musically-relevant DSP -- Braids, Plaits, Elements, Rings, Clouds, Warps, Tides) is
**MIT licensed**; the older AVR-target modules are GPLv3 and are explicitly *not*
referenced here or anywhere in this codebase, to keep the commercial-closed-source
option open (see [LICENSING.md](LICENSING.md)).

**No code from that repository is vendored or copied into this one.** What follows
is the conceptual mapping: which of our architectural decisions were informed by
which module, and what we did differently or left for a later phase.

| Mutable module | Concept borrowed | Where it shows up in MURMUR |
|---|---|---|
| **Plaits** | A single oscillator slot that can become one of several unrelated synthesis algorithms, selected per-slot rather than hard-wired | `algorithm::EngineType` on every `AlgorithmNode` -- any of the 8 nodes in a layer can independently be Classic, Wavetable, FM/PM, Additive, Phase/Shape, Granular, Noise/Chaos, or Resonator. Plaits picks one engine per *voice*; we generalized it to one engine per *node*, composed through the algorithm graph. |
| **Braids** | A single "macro-oscillator" spanning a wide waveform continuum via one morph parameter | `oscillator::ClassicOscillator`'s `morph` parameter (continuous Sine -> Triangle -> Saw -> Square sweep) in `ClassicOscillatorParams`. |
| **Rings** | Excite a resonator bank with an external or internal signal; separate "exciter" and "resonator" roles | `content/algorithms/spectral_exciter.json` -- a hub node summing multiple excitation sources plus a modulator, written as the routing shape Engine Type 8 (Resonator, PLANNED -- see ROADMAP Phase 10) will want once implemented. |
| **Clouds** | Grain-pool-based texture synthesis with fixed grain capacity (no per-grain heap allocation) | Informs the *architectural* commitment already stated for Engine Type 6 (Granular, PLANNED): fixed-capacity grain pool, samples prepared off-thread, zero realtime allocation -- see `docs/DSP_ENGINE.md` "Engine Type 6 — Granular". Not yet implemented. |
| **Warps** | Cross-modulation / ring-modulation between two signal paths as a first-class algorithm, not a side effect | `algorithm::EdgeType::RingMod` and the `cross_mod.json` / `ring_network.json` algorithm templates. |
| **stmlib** (shared utility library) | Small, dependency-free DSP utility conventions: one-pole smoothing coefficients, linear/allpass interpolation, PolyBLEP correction | `dsp::OnePoleSmoother` (`pw8/dsp/Smoother.hpp`) and `oscillator::polyBlep()` (`pw8/oscillator/ClassicOscillator.hpp`) follow the same well-established mathematical formulations (these are standard DSP techniques described in the broader literature, e.g. Valimaki's PolyBLEP papers -- stmlib's implementation was one of several references, not the source copied from). Our implementations are original code. |
| **Tides** | A single generator that's simultaneously usable as an LFO, an envelope-like contour, or an audio-rate oscillator depending on rate | Noted as an influence on the *planned* unification of `pw8/lfo/` and `pw8/envelope/` rate ranges (Phase 5) -- not yet implemented; today `DahdsrEnvelope` and the future LFO are separate, more conventional designs. |
| **Frames** | Keyframe timeline morph with per-segment easing curves | `patch::MorphKoin`, `MorphKoinExecutor.hpp`, `MorphEasing.hpp`, `morphPosition` APVTS, Spatial factory presets -- see [`MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md`](MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md). |
| **Streams** | Dynamics gate (envelope, vactrol, follower, compressor) with sidechain | Sidechain follower MVP (`SidechainFollower.hpp`); full Streams-style master dynamics PLANNED (Track C). |
| **Stages** | Multi-segment CV / envelope chains | Master Motion Lab shell; segment generator PLANNED (Track D). |
| **Marbles** | Generative random / quantized CV | PLANNED generative mod sources (Track E). |
| **Peaks** | Dual trigger → envelope / LFO / drum utility | PLANNED optional utility processors (Track F). **STM32 / MIT** (not AVR/GPL). |
| **Blades** | Dual multimode filter with routing morph and pre-filter drive | Filter 1 SVF + Filter 2 character ship serial-only; routing/mode morph PLANNED (Track B). Hardware-only in repo -- UX spec from public manual, not circuit clone. |

## VCV Rack `Befaco` (VCVRack/Befaco)

At the user's request, also reviewed:
[VCVRack/Befaco](https://github.com/VCVRack/Befaco), the official VCV Rack port of
Befaco's Eurorack module lineup (Rampage, EvenVCO, PonyVCO, Mixer, Muxlicer,
Morphader, Chopping Kinky, Spring Reverb, ADSR, Kickall, and others).

**This entire repository is GPLv3** (unlike the Mutable repo above, there is no
MIT-licensed subset to point to). Accordingly, the treatment here is stricter than
for `pichenettes/eurorack`: **nothing from this repository is a reference for
implementation details at all, let alone code** -- only the *existence and general
purpose* of a module concept was noted, at the same level of abstraction as "modular
synths commonly have a spring reverb module" (a decades-old, non-proprietary circuit
concept predating this specific implementation by a long way). This repository is
explicitly excluded from the dependency graph and from `THIRD_PARTY_LICENSES.md`
because nothing was taken from it beyond concept names.

| Befaco module (concept only) | Where the *concept* (not implementation) shows up |
|---|---|
| **Rampage** (dual function generator: slew/rise-fall shaping usable as envelope, LFO, or audio-rate) | Reinforces the already-noted (Tides-inspired) case for eventually unifying envelope/LFO rate ranges into one generator family -- still PLANNED, Phase 5. |
| **Morphader** (CV-controlled crossfader between two signal paths) | Reinforces the existing `layerMorph` / algorithm-morph design direction (`patch::Patch::layerMorph`, PLANNED DSP wiring in Phase 8/9) -- an equal-power crossfade between two full signal paths, which is exactly what `LayerMode::Morph` is specified to do. |
| **Chopping Kinky** (wavefolder/kink-style waveshaper) | Corroborates Engine Type 5 (Phase/Shape)'s planned "wavefold" control (docs/DSP_ENGINE.md) as a standard, non-proprietary waveshaping technique worth including. |
| **Muxlicer** (CV/gate step sequencer via analog multiplexer) | Corroborates the step-sequencer design direction in `pw8/sequencer/` (PLANNED, Phase 12) -- per-step CV/gate/probability is the same shape our sequencer doc already specifies. |
| **Spring Reverb** | Corroborates "room/plate/hall/large ambient" as the right category breadth for the PLANNED original FDN reverb (`pw8/effects/`, Phase 11) -- spring reverb specifically is not planned as its own algorithm, but reinforces that reverb *character* variety matters. |

No DSP code, circuit topology, or implementation detail was read or reproduced from
this repository -- it only confirmed that several already-planned MURMUR
features (layer/algorithm morph, step sequencing) correspond to
well-established, generic modular-synthesis building blocks rather than needing
invention from scratch conceptually.

## Super Synthesis `eurorack` (supersynthesis/eurorack)

At the user's request, also reviewed:
[supersynthesis/eurorack — Production Modules](https://github.com/supersynthesis/eurorack/tree/main/Production%20Modules),
Chris McDowell's open-source Eurorack module designs (Super Synthesis). Ten production
modules ship in-repo (hardware + JLCPCB artifacts; four with STM32 firmware). The
entire repository is **CC0** (public domain) — techniques may be studied and
reimplemented with optional attribution; **no code from that repository is vendored or
copied into this one.**

Super Synthesis occupies a complementary niche to Mutable Instruments: smaller
STM32 modules, LUT-based control mapping, and gestural UX (live loop recording,
crossfade geometry) rather than deep menu diving. The digital voice/FX code is
original CC0 work, not MI-derived.

| Super Synthesis module | Concept borrowed | Where it shows up in MURMUR |
|---|---|---|
| **PHRSR** (dual 1–16 step CV recorder) | Live loop-length recording; dual independent sequences; pow()-shaped rate; clock-delay compensation for DAC lag | Arp step strip, Morph timeline, Master Motion Lab — gestural step/motion recording UX (PLANNED / PARTIAL). |
| **SCANNER** (4-way CV crossfade) | Single 0–5V input → four overlapping triangular 0–5V outputs | Morph hub, filter routing morph, layer morph — multi-target crossfade window geometry (adjacent to `MorphKoinExecutor`, `FilterRouting.hpp`). |
| **`dynamic_smooth`** (shared DSP util) | Adaptive 2-pole smoother — cutoff rises with signal band energy | Mod matrix smoothing, macro spread, morph easing — adjacent to `ModCurveShaping.hpp`, `MacroSpread.hpp`, `MorphEasing.hpp` (original reimplementation if adopted). |
| **2OPFM** (2-op FM voice) | Expo pitch LUT; envelope-squared modulator drive; trigger-divider / one-shot envelope logic | FM/PM engine, generative triggers (PLANNED). |
| **CHORUS** (modulated delay) | Modulated delay + feedback SVF; LFO→delay depth; dry/wet balance | FX / modulation paths (PLANNED). |
| **ROOM** (modulated reverb) | Modulated all-pass network; multi-LFO size modulation; HP/LP in feedback | Clouds-adjacent reverb character (PLANNED). |
| **SVFs** (dual DC-coupled analog SVF) | Wide-range rubbery SVF; resonance→oscillation; CV-as-audio DC coupling ethos | Blades Filter 1 SVF + Filter Lab UX (`StateVariableFilter.hpp`, `DesignFilterLabPanel`) — original TPT/SVF code, not V2164 circuit clone. |
| **EG** (fast AD envelope) | Fast attack-decay; EOC chaining; re-trig divider mode | Master Envelope, motion segments (PLANNED / PARTIAL). |
| **TVCA** (tanh VCA) | LM13700-style soft clipping / tanh transfer | Character filter drive (`CharacterFilter.hpp`) — curve shape reference, not OTA circuit clone. |
| **VCAs** (4ch normalled mixer) | Per-channel attenuverter + CV; normalled mix topology | Mod routing UI (`ModRoutingUi`, mod matrix) — routing topology reference. |

**Research notes (2026-08-17):** CHORUS and ROOM are production-complete in-repo but
omitted from the folder README; analog modules (SVFs, EG, VCAs, SCANNER, TVCA) ship
schematics/BOM only — no firmware. Digital modules share STM32CubeIDE firmware at
~44.1 kHz with `expo_lut`, `dynamic_smooth`, and compact SVF helpers in the audio ISR.

**GitHub:** [Production Modules index](https://github.com/supersynthesis/eurorack/tree/main/Production%20Modules) ·
[2OPFM](https://github.com/supersynthesis/eurorack/tree/main/Production%20Modules/2OPFM) ·
[PHRSR](https://github.com/supersynthesis/eurorack/tree/main/Production%20Modules/PHRSR) ·
[CHORUS](https://github.com/supersynthesis/eurorack/tree/main/Production%20Modules/CHORUS) ·
[ROOM](https://github.com/supersynthesis/eurorack/tree/main/Production%20Modules/ROOM) ·
[Unreleased](https://github.com/supersynthesis/eurorack/tree/main/Unreleased) (PNGBL, OTAVCAs, S&H lineage).

## What we deliberately did NOT adopt

- **Exact filter topologies.** Ripples/Blades' specific analog-modeled filter
  circuits and Super Synthesis V2164/LM13700 analog SVF/OTA circuits are not
  reproduced. `pw8/filter/` uses an original TPT/state-variable design for
  Filter 1 and original nonlinear topologies for Filter 2, per the master spec's
  explicit instruction not to copy filter implementations from any existing product.
- **Any AVR/GPLv3-licensed Mutable module** (Grids, Branches, Edges, etc.) and **all of
  `VCVRack/Befaco`** (entirely GPLv3) -- excluded from any implementation-level
  consideration, referenced (where referenced at all) only at the level of "this
  general category of module exists," to avoid GPL entanglement in a codebase that
  may ship closed-source. **Peaks is STM32/MIT** and is in scope for conceptual
  reference (see integration plan); it was previously misclassified here.
- **Panel/UI/hardware design** -- out of scope; MURMUR is software-only.
  Super Synthesis DipTrace panels, Thonkiconn jack placement, and product industrial
  design are explicitly not referenced.
- **Preset content, product names, or visual identity** -- none referenced.
  Super Synthesis product names (2OPFM, PHRSR, etc.) appear here only as module
  identifiers for attribution, not as MURMUR branding.

## Other influences

General polyBLEP band-limiting (Valimaki et al.) and standard DAHDSR envelope shapes
are textbook DSP techniques, independently implemented; see
[DSP_ENGINE.md](DSP_ENGINE.md) for the specific rationale behind each choice.
