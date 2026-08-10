# DSP Engine

Status key used throughout: **IMPLEMENTED** (in the signal path, tested) /
**PARTIAL** (real code, known gaps) / **PLANNED** (architected, not yet built).

## Denormal Handling

`dsp::ScopedDenormalGuard` (`pw8/dsp/Denormal.hpp`) flips FTZ/DAZ (x86 SSE) or the
ARM FPCR FZ bit for the lifetime of a render call. `render::Engine::process()`
constructs one at the top of every call. This is the free, blanket protection;
specific feedback paths additionally use `dsp::flushIfNotFinite()` after divisions
or unstable nonlinear stages rather than relying on FTZ alone (FTZ doesn't stop
NaN/Inf propagation). **IMPLEMENTED.**

## Engine Type 1 — Classic

**IMPLEMENTED.** `oscillator::ClassicOscillator` (`pw8/oscillator/ClassicOscillator.hpp`).

Band-limiting strategy: **PolyBLEP** (polynomial band-limited step), applied to Saw
and Square/Pulse; Triangle is a leaky-integrated PolyBLEP square (a standard trick:
integrating a band-limited square removes the value-discontinuity aliasing
entirely); Sine needs no correction.

Why PolyBLEP over minBLEP/BLIT/wavetable-bandlimiting for this engine specifically:

- It needs only the current phase and phase increment -- no lookup table, no
  wraparound convolution bookkeeping.
- That makes it trivial to keep *correct under per-sample pitch modulation*, which
  matters here because this oscillator is also the building block inside the 8-node
  algorithm graph, where FM/PM/sync edges can modulate its frequency and phase every
  single sample. minBLEP-style techniques are more fiddly to keep numerically sound
  under that kind of modulation.
- The residual quality gap vs. minBLEP at extreme oversampling ratios is not
  audible at 44.1-96 kHz for the waveform set here.

Known limitation: the triangle's slope discontinuities are not further corrected
(a polyBLAMP pass would remove the last bit of high-frequency artifacting) -- tracked
as a future refinement, not required to clear the MVP quality bar (`tests/dsp/ClassicOscillatorTests.cpp`
verifies tuning accuracy, bounded output at 4 kHz, and continuous morph sweeps).

Supported: sine/triangle/saw/square waveforms, continuous morph across that ordered
sequence, pulse width (square), phase reset, key tracking (via the algorithm
executor's frequency computation), external phase modulation (for PM/feedback
edges). **Not yet implemented on this engine specifically:** hard sync as a
standalone toggle (the algorithm graph's `SYNC` edge type covers this at the graph
level instead -- see ALGORITHM_GRAPH.md), free-run vs. reset-on-note-on mode choice
(currently always resets on note-on, with a small seeded phase offset for voice
variation).

## Engine Type 2 — Wavetable

**PARTIAL.** `oscillator::WavetableOscillator` (`pw8/oscillator/WavetableOscillator.hpp`).

Implemented: multi-frame tables, linear interpolation across both sample position
and frame position (continuous frame morphing), non-owning `WavetableView` so the
oscillator itself never allocates (tables are prepared off-thread).

**Not yet band-limited/mip-mapped.** The master spec is explicit that a production
wavetable engine must not tolerate resynthesizing full-band tables at high pitch --
this implementation does exactly that today, so it will alias on high-harmonic-content
tables played at high fundamental frequencies. The storage layout (frame-major,
fixed `samplesPerFrame`) was chosen specifically so mip levels can be added later as
additional `WavetableView`s per octave without changing the oscillator's read logic.
Mip-map generation is PLANNED for `tools/wavetable_builder` (currently emits exactly
one full-bandwidth mip level, see its `--help` output and inline comments).

`tools/wavetable_builder` (`pw8-wavetable-builder`): imports 16-bit PCM mono WAV,
splits into equal frames, per-frame-normalizes, emits a `.pw8wt`-shaped JSON table.
No factory tables are shipped in `content/wavetables/` yet.

## Engine Type 3 — FM / PM

**PARTIAL (at the algorithm-graph level, not as a standalone engine).** There is no
dedicated `FmPm` engine implementation yet (`algorithm::EngineType::FmPm` renders
silence, matching every other unimplemented engine type -- see "Fuzz-Safe Design"
below). However, phase modulation, frequency modulation, and one-sample-delayed
feedback are fully implemented at the *algorithm graph* level
(`algorithm::AlgorithmExecutor`, edge types `PHASE_MOD` / `FREQUENCY_MOD` /
`FEEDBACK`) and work today between `Classic` oscillators -- see
`content/presets/fm-bell.pw8` and `content/algorithms/feedback_bell.json` for a
working example, and `ALGORITHM_GRAPH.md` for the exact semantics (including the
feedback gain guard and soft saturation). A dedicated ratio/fixed-frequency-mode
ready-made engine type with its own stability guarantees at frequency extremes is
PLANNED (Phase 4/10) but the underlying modulation mechanics already exist and are
tested (`tests/unit/AlgorithmGraphCompilerTests.cpp`,
`tests/regression/RenderSanityTests.cpp`'s self-feedback case).

## Engine Type 4 — Additive

**PLANNED** (Phase 10). Target: 64-128 partials via a vectorized oscillator-bank
design (not independent oscillator objects per partial). Controls to implement:
harmonic tilt, odd/even, stretch, inharmonicity, formant, spectral blur, partial
randomization.

## Engine Type 5 — Phase / Shape

**PLANNED** (Phase 10). Target: phase distortion, wavefold, bend, asymmetry, bias,
shape, with selective oversampling around the nonlinear stages.

## Engine Type 6 — Granular

**PLANNED** (Phase 10). Architectural commitment made now: fixed-capacity grain
pool, samples prepared off-thread, zero realtime allocation (see PRIOR_ART.md for
the Clouds-style grain-pool influence on this decision).

## Engine Type 7 — Noise / Chaos

**PLANNED** (Phase 10). All variants (white/pink/brown/blue/digital/sample-hold/
smooth-random/dust/crackle/metallic/chaotic) will be seeded through
`dsp::DeterministicRng` (already implemented, see below) so patch rendering stays
repeatable.

## Engine Type 8 — Resonator / Spectral

**PLANNED** (Phase 10). `content/algorithms/spectral_exciter.json` is a
forward-looking algorithm template for this engine (see PRIOR_ART.md).

## Deterministic Randomness

**IMPLEMENTED.** `dsp::DeterministicRng` (`pw8/dsp/Random.hpp`) is a splitmix64 PRNG.
`DeterministicRng::deriveSeed(patchSeed, voiceId, noteGenerationId)` produces a
decorrelated-but-reproducible sub-seed per voice/note without sharing one running
stream (which would make voice N's output depend on how many notes preceded it --
unacceptable for reproducible AI-generated-patch rendering). Used today for the
small per-voice phase-offset variation in `Voice::noteOn()`
(`pw8/voice/Voice.hpp`); every future generative feature (noise engines, humanize,
grain position, step probability) is required to seed through this same mechanism
rather than an unseeded/global RNG.

## Parameter Smoothing

**PARTIAL.** `dsp::OnePoleSmoother` and `dsp::LinearRamp` are implemented
(`pw8/dsp/Smoother.hpp`) but are not yet wired onto every continuous parameter --
Filter 1's cutoff/resonance are recomputed and reapplied fresh every sample instead
(see "Filter System" below, which explains why that's the right call for a filter
specifically), and drive/FX-mix don't exist yet to smooth. `OnePoleSmoother`/
`LinearRamp` remain unused by any current code path; wiring them onto
block-rate-only parameters (e.g. gain/pan set at note-on) is lower priority than it
looked before Filter 1 shipped, since the highest-risk zipper-noise source (cutoff
sweeping under mod-matrix control) turned out not to need them.

## Filter System

**Filter 1: IMPLEMENTED.** `filter::StateVariableFilter` (`pw8/filter/StateVariableFilter.hpp`),
a trapezoidal-integrator ("TPT") state-variable filter -- lowpass, highpass,
bandpass, notch, and peak modes from the same two integrator states. One instance
per voice, sitting in the signal chain between the algorithm graph's output and the
amplitude envelope: `Algorithm Graph -> Filter 1 -> Amp Envelope (VCA) -> Pan -> Output`.

Cutoff and resonance are recomputed from scratch every sample rather than smoothed
toward a target -- deliberately: the mod matrix can drive cutoff every sample (LFO,
envelope, velocity, macros), and a TPT SVF's coefficients are cheap enough (one
`tan()` plus a handful of multiplies) that recomputing them per sample is both
simpler and more responsive than maintaining a smoother on top of an
already-continuous per-sample source. `tests/dsp/StateVariableFilterTests.cpp`
verifies: lowpass/highpass/bandpass frequency-response direction, stability and
bounded output at maximum resonance across the full cutoff range, and clean state
reset. `tests/regression/RenderSanityTests.cpp` verifies the filter has an
audible, measurable effect through the full render pipeline (RMS drop with a
lowpass engaged on a bright saw).

Key tracking (`FilterParams::keyTrack`, -1..1) scales cutoff by the voice's
effective (pitch-bent) frequency relative to middle C, so a patch's brightness can
follow the keyboard the way an analog filter naturally would.

**Filter 2** ("character" -- nonlinear ladder-style/OTA-style/diode-style/saturated
cascade topologies): **PLANNED**, ROADMAP.md Phase 6 continuation.

## Voice, Envelope, Algorithm Graph, Filters, FX

See dedicated docs:
- [ALGORITHM_GRAPH.md](ALGORITHM_GRAPH.md) -- the 8-node graph, compiler, execution semantics
- [MODULATION.md](MODULATION.md) -- envelope, LFO, and mod matrix (all IMPLEMENTED at VOICE scope)
- Filter 1 is IMPLEMENTED (above); Filter 2 and FX (`pw8/effects/`) are PLANNED -- see ROADMAP.md Phase 6 / 11.

## Voice Architecture

**IMPLEMENTED.** `voice::Voice` / `voice::VoicePool` / `voice::VoiceAllocator`
(`pw8/voice/Voice.hpp`, `pw8/voice/VoiceAllocator.hpp`). Fixed-capacity pool
(`core::kMaxVoices` = 32, default configured polyphony 16, configurable per-patch up
to the ceiling via `Patch::voiceSettings.polyphony`). Each voice owns independent
operator state (8 `op::OperatorState`), one amplitude envelope, one LFO, one Filter
1 instance, a VOICE-scoped mod matrix, and MPE-ready per-note expression
(`voice::NoteExpression`: pitch bend, channel pressure, poly aftertouch, MPE
slide/pitch -- captured from MIDI). Pitch bend directly affects pitch; channel
pressure, poly aftertouch, and MPE slide are mod matrix *sources*
(`ModSource::ChannelPressure/PolyAftertouch/MpeSlide`) -- audible only if a patch's
`modRoutes` actually routes them somewhere, same as any other source.

**Voice stealing policy** (`VoiceAllocator::allocate()`): free voice first; else the
quietest *released* voice; else the quietest voice overall, ties broken by oldest.
No crossfade on steal yet (PLANNED, Phase 7) -- a stolen voice's envelope restarts
from Delay rather than being hard-cut, which avoids the worst clicks in the common
case but isn't a true ramped crossfade.

## Quality Modes

**PARTIAL (data model only).** `render::QualityMode` (Eco/Normal/High/Ultra/Offline)
exists and is threaded through `RenderOptions`/the Python bindings, but nothing in
the DSP path currently branches on it -- there's no oversampling, no
adjustable-quality interpolation, and no reverb to vary yet. It's real, typed,
plumbed-through state waiting for the DSP that will consume it.

## CPU Governor / Complexity Estimator

**PLANNED.** Not implemented in this pass.
