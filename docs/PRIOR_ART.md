# Prior Art & Design Lineage

This document records where Patchwork Eight's architecture deliberately draws on
prior work, per the master spec's requirement that inspiration be explicit and that
nothing be cloned from a commercial product without attribution and reimplementation.

## Mutable Instruments `eurorack` (pichenettes/eurorack)

At the user's request, Patchwork Eight's engine-type architecture and several DSP
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

| Mutable module | Concept borrowed | Where it shows up in Patchwork Eight |
|---|---|---|
| **Plaits** | A single oscillator slot that can become one of several unrelated synthesis algorithms, selected per-slot rather than hard-wired | `algorithm::EngineType` on every `AlgorithmNode` -- any of the 8 nodes in a layer can independently be Classic, Wavetable, FM/PM, Additive, Phase/Shape, Granular, Noise/Chaos, or Resonator. Plaits picks one engine per *voice*; we generalized it to one engine per *node*, composed through the algorithm graph. |
| **Braids** | A single "macro-oscillator" spanning a wide waveform continuum via one morph parameter | `oscillator::ClassicOscillator`'s `morph` parameter (continuous Sine -> Triangle -> Saw -> Square sweep) in `ClassicOscillatorParams`. |
| **Rings** | Excite a resonator bank with an external or internal signal; separate "exciter" and "resonator" roles | `content/algorithms/spectral_exciter.json` -- a hub node summing multiple excitation sources plus a modulator, written as the routing shape Engine Type 8 (Resonator, PLANNED -- see ROADMAP Phase 10) will want once implemented. |
| **Clouds** | Grain-pool-based texture synthesis with fixed grain capacity (no per-grain heap allocation) | Informs the *architectural* commitment already stated for Engine Type 6 (Granular, PLANNED): fixed-capacity grain pool, samples prepared off-thread, zero realtime allocation -- see `docs/DSP_ENGINE.md` "Engine Type 6 — Granular". Not yet implemented. |
| **Warps** | Cross-modulation / ring-modulation between two signal paths as a first-class algorithm, not a side effect | `algorithm::EdgeType::RingMod` and the `cross_mod.json` / `ring_network.json` algorithm templates. |
| **stmlib** (shared utility library) | Small, dependency-free DSP utility conventions: one-pole smoothing coefficients, linear/allpass interpolation, PolyBLEP correction | `dsp::OnePoleSmoother` (`pw8/dsp/Smoother.hpp`) and `oscillator::polyBlep()` (`pw8/oscillator/ClassicOscillator.hpp`) follow the same well-established mathematical formulations (these are standard DSP techniques described in the broader literature, e.g. Valimaki's PolyBLEP papers -- stmlib's implementation was one of several references, not the source copied from). Our implementations are original code. |
| **Tides** | A single generator that's simultaneously usable as an LFO, an envelope-like contour, or an audio-rate oscillator depending on rate | Noted as an influence on the *planned* unification of `pw8/lfo/` and `pw8/envelope/` rate ranges (Phase 5) -- not yet implemented; today `DahdsrEnvelope` and the future LFO are separate, more conventional designs. |

## What we deliberately did NOT adopt

- **Exact filter topologies.** Ripples/Blades' specific analog-modeled filter
  circuits are not reproduced. `pw8/filter/` (PLANNED, Phase 6) will use an
  original TPT/state-variable design for Filter 1 and original nonlinear topologies
  for Filter 2, per the master spec's explicit instruction not to copy filter
  implementations from any existing product.
- **Any AVR/GPLv3-licensed module** (Grids, Peaks, Kinks, etc.) -- excluded entirely
  from consideration to avoid GPL entanglement in a codebase that may ship
  closed-source.
- **Panel/UI/hardware design** -- out of scope; Patchwork Eight is software-only.
- **Preset content, product names, or visual identity** -- none referenced.

## Other influences

General polyBLEP band-limiting (Valimaki et al.) and standard DAHDSR envelope shapes
are textbook DSP techniques, independently implemented; see
[DSP_ENGINE.md](DSP_ENGINE.md) for the specific rationale behind each choice.
