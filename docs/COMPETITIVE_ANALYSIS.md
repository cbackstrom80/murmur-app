# Competitive Analysis: Serum 2, Phase Plant & Zebra 3

Researched at the user's request to check feature parity and UI conventions against
three commercial wavetable/modular synths: **Serum 2** (Xfer Records),
**Phase Plant** (Kilohearts), and **Zebra 3** (u-he). Sources:
[xferrecords.com/products/serum-2](https://xferrecords.com/products/serum-2),
[kvraudio.com/product/serum-2-by-xfer-records](https://www.kvraudio.com/product/serum-2-by-xfer-records),
[kilohearts.com/docs/phase_plant](https://kilohearts.com/docs/phase_plant),
[kilohearts.com/products/phase_plant](https://kilohearts.com/products/phase_plant),
[u-he.com/products/synths/zebra3](https://u-he.com/products/synths/zebra3/),
[attackmagazine.com/news/u-he-announces-zebra-3](https://www.attackmagazine.com/news/u-he-announces-zebra-3/).

**Boundary, per the master spec's explicit instruction ("do not copy... UI layout,
visual identity, product names" from any existing commercial synth):** this document
records *what features exist* for gap analysis, and *what UI paradigm category* each
product uses at a one-sentence level of abstraction, for roadmap planning only. It
does **not** contain pixel layouts, color schemes, panel dimensions, iconography, or
any other visual-identity detail, and no code or asset was copied from any product.
Where MURMUR's own planned UI (PLUGIN_ARCHITECTURE.md "Signature UI:
Graph") is described below, it is described as already independently specified in
this repository, not as an adaptation of any product's screens.

## Feature parity matrix

| Feature area | Serum 2 | Phase Plant | Zebra 3 | MURMUR |
|---|---|---|---|---|
| Multiple independent sound-source types per voice | 5 types (wavetable, multisample, sample, granular, spectral) | 4 generator types (analog, wavetable, sampler, noise) + filter/distortion as generators | 4 main oscillators, each independently a spline/hand-drawn morph, classic wavetable, or additive engine | 8 nodes/layer, each independently one of 8 `EngineType`s -- **more slots than any of the three**, and (updated 2026-08-21, was stale) **all 7 self-contained engines now actually produce sound** (Classic, Wavetable, FM/PM, Additive, PhaseShape, Granular, NoiseChaos, Resonator -- confirmed via a real per-engine parameter sweep this session); only `External` (host sidechain input) remains a distinct, intentionally-different case |
| Additive synthesis partial count | -- | -- | Up to **1024** sine partials | Engine Type 4 ships **64 partials** (real clamp, `PatchSerializer.cpp`) -- Zebra's 1024 is a useful upper-bound data point but not adopted as a new target; a vectorized oscillator-bank design (already specified) is what would make any larger partial count tractable |
| Wavetable oscillator warp/distortion modes | Bend, sync, FM, phase distortion, ring mod, formant/vocode-from-another-osc, spectral warps | Wavetable osc with standard controls | Spline-based morphing between hand-drawn curves (continuous, not frame-interpolated) | `docs/DSP_ENGINE.md` "Wavetable Warp" already specifies an equivalent taxonomy (bend, mirror, fold, sync-style, phase distortion, asymmetry, spectral tilt) from the *original* master spec, independently of this research -- PARTIAL: mip-mapping (this same pass) closes the anti-aliasing gap; warp modes remain PLANNED |
| Wavetable band-limiting / mip-mapping | Yes (implied by "smooth interpolation... near-infinite frame positions") | Not a stated focus | Spline curves are continuous, sidestepping frame-mip-mapping entirely (a different technique) | **IMPLEMENTED** this pass: FFT-based harmonic-truncation mip levels (`pw8::dsp::fft`, `oscillator::WavetableTable`), runtime mip selection by note frequency vs. actual sample rate -- see DSP_ENGINE.md |
| FM oscillator stability at extremes | -- | -- | **Through-zero FM** specifically called out for stability under extreme modulation | Validates the master spec's existing "be extremely careful with... frequency extremes" instruction. Engine Type 3 (FM/PM) is now **IMPLEMENTED** and through-zero-safe by construction (`ClassicOscillator`'s plain phase accumulator handles a negative instantaneous frequency correctly, no special-casing needed -- explicitly tested in `tests/dsp/FmPmOperatorTests.cpp`); graph-level PM/FM edges remain separately soft-guarded (feedback gain clamp, `flushIfNotFinite`) |
| Unison / detune | Per-oscillator, up to ~16 voices | Per-generator | Wavetable mode: up to 16-voice unison | `UnisonSettings` data model exists (`FULL`/`OPERATOR`/`STEREO`/`HYPER`/`HARMONIC` modes specified, max 16 voices -- `core::kMaxUnisonVoices`); DSP wiring PLANNED (Phase 7). `content/presets/wide-saw.murmur` demonstrates the target *sound* today via hand-detuned operators |
| Filter types | 11+ (analog emulations + creative) | Multimode filter as a generator/effect Snapin | **13 models x up to 12 responses each** (Ladder, Cascade, SVF, etc.) | (updated 2026-08-21) Filter 1: **IMPLEMENTED** (LP/HP/BP/notch/peak, TPT SVF, plus continuous `modeMorph`). Filter 2: **IMPLEMENTED** (4-pole soft ladder + tanh drive, plus its own `modeMorph` giving real LP/BP/HP variety from the same cascade). **2 topologies today** (SVF x5 responses, ladder x3 responses via morph) vs. Zebra's 13x12 -- still a real, open scale gap, but no longer a missing category; OTA-style/diode-style/saturated-cascade additional topologies remain unbuilt, unscheduled |
| Physical modeling / resonators | -- | -- | Modal resonators + comb filters, detailed feedback/damping control | Engine Type 8 (Resonator/Spectral) is **IMPLEMENTED** -- a noise-excited modal filter bank (`ResonatorOscillator.hpp`). A real near-silent-output bug at moderate/high `decay` (Q) was found and fixed 2026-08-21 (exciter burst length + output gain were never calibrated for the engine's full Q range); see the class's own fix comment and `tests/unit/ResonatorEngineLevelTests.cpp` |
| Ring modulation variants | Standard ring mod | -- | **Bode-style frequency shifting** (an established, non-proprietary analog technique -- Harald Bode) alongside standard ring mod | `EdgeType::RingMod` (IMPLEMENTED) is multiplicative ring mod only; a dedicated frequency-shifter effect is already listed (not newly added) under the PLANNED second effects wave in DSP_ENGINE.md ("frequency shift") |
| LFOs | Multiple, tempo-synced, custom shapes | Up to 32 modulators total (LFO/envelope/random/MIDI), stackable | Oscillators double as modulation sources (audio-rate or sub-audio) | (updated 2026-08-21) **8 LFOs per layer, IMPLEMENTED** (`core::kNumLfosPerLayer`), 6 waveforms incl. sample-hold/smooth-random, free/retrigger/one-shot/tempo-sync, VOICE + LAYER/GLOBAL scope -- at parity with the master spec's original target |
| Envelopes | 2+ (drawable) | AHDSR with onset delay, stackable as modulators | Multiple, stackable | (updated 2026-08-21) **8 DAHDSR envelopes per layer, IMPLEMENTED** (`core::kNumEnvelopesPerLayer`), each usable as a mod-matrix source -- at parity with the master spec's original target |
| Modulation routing UI paradigm | Drag-and-drop onto target (per community documentation) | Drag-and-drop: hover source -> click "+" -> drag onto target, target highlights blue->green | **"Wireless modular"**: connections assigned via UI without a visible patch-cable graphic (grid/rack-based, not a literal wire canvas) | **PLANNED** (Phase 17, no UI built yet). `ModMatrixExecutor` (IMPLEMENTED, `pw8/modulation/`) is UI-agnostic -- a matrix-grid, a drag-to-connect wire, *or* a wireless/assign-from-list front end can all sit on top of the same `ModRoute` data without engine changes |
| Macros | Present (general modern-synth convention) | **Exactly 8**, positioned under the patch name, each routable to multiple parameters | Present (general convention) | **Exactly 8** (`Patch::macros[8]`), each usable as a `ModSource` (`Macro1`..`Macro8`) routable to multiple `ModRoute`s -- structurally at parity with Phase Plant today, minus a UI |
| Per-oscillator insert effects | -- | -- | **2 serial FX slots per oscillator**, ~20 processes (spectral filtering, phase distortion, animated spectral decay) | Not planned at per-*operator* granularity -- `docs/DSP_ENGINE.md`'s FX architecture is per-*layer* (3 insert slots) and per-*master* (4 slots). Noted as a real granularity gap, not currently scheduled to change |
| Arpeggiator / step sequencer | Arpeggiator + clip sequencer | Not a core focus (effects-and-modulation-first design) | -- | (updated 2026-08-21) **IMPLEMENTED** -- real `sequencer::ArpeggiatorParams`/`ArpStep` (mode, rate, sync division, octave range, per-step enabled/gate/probability/ratchet/tie/velocity/accent), also exposed to the AI patch-generation schema |
| Effects | Multiple independent effects buses, dynamic routing | 3 Snapin lanes (per-voice + global), serial/parallel, per-lane polyphony toggle | Per-oscillator FX (above) plus presumed master bus | (updated 2026-08-21) **IMPLEMENTED**: 3 layer insert slots + 4 master slots, **13 real effect types** (Saturation, Chorus, TapeDelay, NodeDelay, FreqShiftEcho, FractalEcho, Reverb, EQ, Compressor, Limiter, Vocoder, Clouds, BinauralSpace) -- exceeds Phase Plant's 3-lane slot count in type variety; Vocoder/Clouds/BinauralSpace have no equivalent in any of the three competitors |
| Algorithm/signal-routing graph | No general node graph (fixed osc/filter signal flow, per-oscillator warp params) | **Explicitly not a node graph** ("modules automatically route vertically unless explicitly rerouted") | Grid-based "wireless modular" (four-lane rack), not a free-form node graph | **IMPLEMENTED**: an actual typed, validated, compiled 8-node graph (`AlgorithmGraphCompiler`/`AlgorithmExecutor`) with 7 edge types (AUDIO/PHASE_MOD/FREQUENCY_MOD/AMPLITUDE_MOD/RING_MOD/SYNC/FEEDBACK). **This is a genuine structural differentiator from all three products**, not a gap -- see below |
| Deterministic/reproducible rendering | Not a stated design goal (real-time performance instrument) | Not a stated design goal | Not a stated design goal | **IMPLEMENTED** and a first-class requirement (`dsp::DeterministicRng`, `Patch::seed`) -- needed for AI-generated-patch evaluation, a use case none of the three target |
| Native headless/offline rendering (no plugin host) | No (DAW/host-only) | No (DAW/host-only) | No (DAW/host-only) | **IMPLEMENTED** (`murmur-render`, Python bindings) -- another differentiator, not a gap |

## Where we're genuinely ahead vs. where we're genuinely behind

**Ahead (structural, not just "not implemented yet"):** the algorithm graph is a real
differentiator -- Phase Plant explicitly avoids a node-graph model, Zebra 3 uses a
fixed four-lane grid rather than a free-form graph, and Serum's routing is
per-oscillator parameters rather than a general typed graph. Deterministic rendering
and a native headless renderer are things none of the three competitors need or
have, because none targets programmatic/AI-driven generation the way this project
does (see `docs/PATCHWORK_INTEGRATION.md`).

**Behind (real gaps, current as of 2026-08-21):** filter *breadth* (Zebra's 13
models x 12 responses vs. our 2 topologies/8 total responses today -- the one
concrete DSP gap this doc still tracks), per-oscillator effect granularity (Zebra
only), and -- the largest real gap by far -- **the UI**: a Figma-vs-code audit
(`docs/FIGMA_UI_AUDIT.md`) found 0 of 63 tracked screens at full parity with their
design spec (irrelevant that competitors have a finished UI -- we simply haven't
finished ours yet). Modulator count and oscillator variety, previously listed here
as gaps, are **not** gaps anymore -- both were stale claims (8 LFOs/8 envelopes and
all 7 self-contained engine types now ship; see the table above).

No new roadmap phases were added as a result of this research -- every gap
identified above was already tracked under an existing PLANNED phase in
`docs/ROADMAP.md`, or (per-oscillator FX granularity) is noted as a deliberate,
undecided scope question rather than a silently-dropped requirement. One concrete
change *did* land as a direct result of this research pass, though: **wavetable
mip-mapping was implemented** (see `docs/DSP_ENGINE.md` "Wavetable"), closing the
one gap in this matrix that was both squarely in-scope for the current phase (Phase
2) and clearly demonstrated as solved-by-competitors (Serum's smooth interpolation,
Zebra's continuous splines) rather than left as a known limitation.

## UI paradigm notes (conceptual only -- see boundary statement above)

All three products converge on, or diverge instructively around, a few genuinely
common, non-proprietary UI idioms worth being aware of when Phase 17 (UI) eventually
starts, described here at the paradigm level only:

- **Direct-manipulation modulation** (drag a modulation source onto a destination
  knob, rather than only a spreadsheet-style matrix grid) is now a near-universal
  convention across modern synths (Serum, Phase Plant), not something originating
  with either product. Zebra 3's "wireless modular" approach is a useful third data
  point: modulation connections exist and are assignable without ever rendering a
  visible cable/wire graphic at all -- a genuinely different paradigm from the other
  two, useful to know exists. `ModMatrixExecutor`'s data model (`ModRoute{source,
  destination, targetIndex, amount}`) supports any of the three presentations
  equally -- the engine doesn't care how the route was drawn or assigned.
- **A small, fixed number of prominent macro knobs** (Serum has a macro row, Phase
  Plant has exactly 8 under the patch name) is consistent with MURMUR's own
  already-specified 8-macro design (`Patch::macros[8]`) -- independent convergence,
  not something adopted from this research.
- **Tabbed/paneled complexity disclosure** (hide deep parameters behind tabs, or --
  Zebra's approach -- only show a module's controls once it's actually been added to
  the rack, so the default view stays simple) is the same idea behind this project's
  already-specified PLAY / DESIGN / LAB three-tier UI (`docs/PLUGIN_ARCHITECTURE.md`),
  which predates this research pass (it's in the original master spec).
- **Grid/rack-based modular layout** (Phase Plant's three panes; Zebra's four-lane
  grid) versus a **free-form node graph** (this project's algorithm graph) are
  different enough that neither offers a template to adapt -- our graph UI will need
  original design work when Phase 17 starts, not a reskin of either paradigm.

None of the above changes `docs/PLUGIN_ARCHITECTURE.md`'s existing UI plan; it
confirms that plan's direction is consistent with where the rest of the market has
converged (or instructively diverged), without adopting any product's specific
screens.
