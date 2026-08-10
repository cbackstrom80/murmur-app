# Competitive Analysis: Serum 2 & Phase Plant

Researched at the user's request to check feature parity and UI conventions against
two commercial wavetable/modular synths: **Serum 2** (Xfer Records) and
**Phase Plant** (Kilohearts). Sources: [xferrecords.com/products/serum-2](https://xferrecords.com/products/serum-2),
[kvraudio.com/product/serum-2-by-xfer-records](https://www.kvraudio.com/product/serum-2-by-xfer-records),
[kilohearts.com/docs/phase_plant](https://kilohearts.com/docs/phase_plant),
[kilohearts.com/products/phase_plant](https://kilohearts.com/products/phase_plant).

**Boundary, per the master spec's explicit instruction ("do not copy... UI layout,
visual identity, product names" from any existing commercial synth):** this document
records *what features exist* for gap analysis, and *what UI paradigm category* each
product uses at a one-sentence level of abstraction, for roadmap planning only. It
does **not** contain pixel layouts, color schemes, panel dimensions, iconography, or
any other visual-identity detail, and no code or asset was copied from either
product. Where Patchwork Eight's own planned UI (PLUGIN_ARCHITECTURE.md "Signature
UI: Graph") is described below, it is described as already independently specified
in this repository, not as an adaptation of either product's screens.

## Feature parity matrix

| Feature area | Serum 2 | Phase Plant | Patchwork Eight |
|---|---|---|---|
| Multiple independent sound-source types per voice | 5 types (wavetable, multisample, sample, granular, spectral) | 4 generator types (analog, wavetable, sampler, noise) + filter/distortion as generators | 8 nodes/layer, each independently one of 8 `EngineType`s -- **more slots than either**, but only Classic (IMPLEMENTED) and Wavetable (PARTIAL) actually produce sound today; Additive/PhaseShape/Granular/NoiseChaos/Resonator/FmPm are PLANNED |
| Wavetable oscillator warp/distortion modes | Bend, sync, FM, phase distortion, ring mod, formant/vocode-from-another-osc, spectral warps | Wavetable osc with standard controls | `docs/DSP_ENGINE.md` "Wavetable Warp" already specifies an equivalent taxonomy (bend, mirror, fold, sync-style, phase distortion, asymmetry, spectral tilt) from the *original* master spec, independently of this research -- PLANNED, not yet implemented (mip-mapping is the current Phase 2 priority ahead of warp) |
| Unison / detune | Per-oscillator, up to ~16 voices | Per-generator | `UnisonSettings` data model exists (`FULL`/`OPERATOR`/`STEREO`/`HYPER`/`HARMONIC` modes specified); DSP wiring PLANNED (Phase 7). `content/presets/wide-saw.pw8` demonstrates the target *sound* today via hand-detuned operators |
| Filter types | 11+ (analog emulations + creative) | Multimode filter as a generator/effect Snapin | Filter 1: **IMPLEMENTED** (lowpass/highpass/bandpass/notch/peak, TPT SVF). Filter 2 ("character"/nonlinear) PLANNED |
| LFOs | Multiple, tempo-synced, custom shapes | Up to 32 modulators total (LFO/envelope/random/MIDI), stackable | LFO1: **IMPLEMENTED** per-voice (6 waveforms incl. sample-hold/smooth-random, free/retrigger/one-shot/tempo-sync). Only 1 per voice vs. their many -- the master spec's full target (8 LFOs/8 envelopes) is PLANNED |
| Envelopes | 2+ (drawable) | AHDSR with onset delay, stackable as modulators | 1 DAHDSR amplitude envelope per voice, **IMPLEMENTED**, also usable as a mod-matrix source (`ModSource::AmpEnvelope`). 8-envelope target PLANNED |
| Modulation routing UI paradigm | Drag-and-drop onto target (per community documentation) | Drag-and-drop: hover source -> click "+" -> drag onto target, target highlights blue->green | **PLANNED** (Phase 17, no UI built yet). `ModMatrixExecutor` (IMPLEMENTED, `pw8/modulation/`) is UI-agnostic -- either a matrix-grid or a drag-to-connect front end can sit on top of the same `ModRoute` data without engine changes |
| Macros | Present (general modern-synth convention) | **Exactly 8**, positioned under the patch name, each routable to multiple parameters | **Exactly 8** (`Patch::macros[8]`), each usable as a `ModSource` (`Macro1`..`Macro8`) routable to multiple `ModRoute`s -- structurally at parity today, minus a UI |
| Arpeggiator / step sequencer | Arpeggiator + clip sequencer | Not a core focus (effects-and-modulation-first design) | PLANNED (Phase 12), spec already calls for up to 64 steps -- exceeds Serum's typical arp step counts |
| Effects | Multiple independent effects buses, dynamic routing | 3 Snapin lanes (per-voice + global), serial/parallel, per-lane polyphony toggle | PLANNED (Phase 11): 3 layer insert slots + 4 master slots specified -- comparable slot count to Phase Plant's 3 lanes |
| Algorithm/signal-routing graph | No general node graph (fixed osc/filter signal flow, per-oscillator warp params) | **Explicitly not a node graph** ("modules automatically route vertically unless explicitly rerouted") | **IMPLEMENTED**: an actual typed, validated, compiled 8-node graph (`AlgorithmGraphCompiler`/`AlgorithmExecutor`) with 7 edge types (AUDIO/PHASE_MOD/FREQUENCY_MOD/AMPLITUDE_MOD/RING_MOD/SYNC/FEEDBACK). **This is a genuine structural differentiator from both products**, not a gap -- see below |
| Deterministic/reproducible rendering | Not a stated design goal (real-time performance instrument) | Not a stated design goal | **IMPLEMENTED** and a first-class requirement (`dsp::DeterministicRng`, `Patch::seed`) -- needed for AI-generated-patch evaluation, a use case neither competitor targets |
| Native headless/offline rendering (no plugin host) | No (DAW/host-only) | No (DAW/host-only) | **IMPLEMENTED** (`pw8-render`, Python bindings) -- another differentiator, not a gap |

## Where we're genuinely ahead vs. where we're genuinely behind

**Ahead (structural, not just "not implemented yet"):** the algorithm graph is a real
differentiator -- Phase Plant explicitly avoids a node-graph model, and Serum's
routing is per-oscillator parameters rather than a general typed graph. Deterministic
rendering and a native headless renderer are both things neither competitor needs or
has, because neither targets programmatic/AI-driven generation the way this project
does (see `docs/PATCHWORK_INTEGRATION.md`).

**Behind (real gaps, already tracked):** modulator *count* (they support many
simultaneous LFOs/envelopes; we support 1 of each per voice today), oscillator
*variety* (granular/spectral/multisample sound sources are shipped in Serum 2 today;
ours are PLANNED Engine Types), effects breadth, and of course the entire UI (neither
product's absence of a UI is relevant here -- we simply haven't built one yet,
correctly, per the master spec's "prove the DSP first" sequencing).

No new roadmap phases were added as a result of this research -- every gap identified
above was already tracked under an existing PLANNED phase in `docs/ROADMAP.md`. This
research validated the existing phase scope rather than changing it.

## UI paradigm notes (conceptual only -- see boundary statement above)

Both products converge on a few genuinely common, non-proprietary UI idioms worth
being aware of when Phase 17 (UI) eventually starts, described here at the paradigm
level only:

- **Direct-manipulation modulation** (drag a modulation source onto a destination
  knob, rather than only a spreadsheet-style matrix grid) is now a near-universal
  convention across modern synths, not something originating with either product.
  `ModMatrixExecutor`'s data model (`ModRoute{source, destination, targetIndex,
  amount}`) supports either presentation equally -- the engine doesn't care how the
  route was drawn.
- **A small, fixed number of prominent macro knobs** (both competitors: Serum has a
  macro row, Phase Plant has exactly 8 under the patch name) is consistent with
  Patchwork Eight's own already-specified 8-macro design (`Patch::macros[8]`) --
  independent convergence, not something adopted from this research.
- **Tabbed/paneled complexity disclosure** (hide deep parameters behind tabs so the
  default view stays simple) is the same idea behind this project's already-specified
  PLAY / DESIGN / LAB three-tier UI (`docs/PLUGIN_ARCHITECTURE.md`), which predates
  this research pass (it's in the original master spec).

None of the above changes `docs/PLUGIN_ARCHITECTURE.md`'s existing UI plan; it
confirms that plan's direction is consistent with where the rest of the market has
converged, without adopting any product's specific screens.
