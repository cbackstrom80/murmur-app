# FX Bank

**IMPLEMENTED** (Phase 11: FX, PLANNED -> PARTIAL). 10 real, independently-voiced
effect algorithms, 3 layer insert slots + 4 master slots (`pw8/effects/`): 6 from
the initial pass (backed by research into three named commercial delay/echo
plugins plus one algorithm this project invented itself), plus Reverb/Eq/
Compressor/Limiter from the later GATE 10 pass rounding out the master spec's
"first effect set" basics. See docs/ROADMAP.md "Follow-up pass: FX bank" and
"GATE 10" for the summary and test/verification counts.

## Research: three plugins, three different approaches to "delay"

Requested by name; researched for their *architecture and feature set*, never
their code, UI, or product identity -- the same boundary this project has held
for every competitive-research pass (COMPETITIVE_ANALYSIS.md, PRIOR_ART.md).
Nothing below is copied code or a UI recreation; each entry states what was
*learned* and which of this project's effects that learning informed.

### ChowMatrix (Chowdhury DSP)

A free, open-source ([GitHub](https://github.com/Chowdhury-DSP/ChowMatrix),
[manual](https://chowdsp.com/manuals/ChowMatrixManual.pdf)) "multitap delay made
up of an infinitely growable tree of delay lines." Each node delays its
*parent* node's output (not necessarily the original input), with its own
delay time, feedback, pan, and distortion; an "Insanity" parameter lets each
node's delay time wander randomly. The core idea -- a *tree*, not a flat list of
taps, so a node's input can itself already be delayed/distorted/panned -- is
what makes it capable of "complex multi-tap, glitch textures, sound design"
rather than a simple repeating echo.

**What informed `effects::NodeDelay`:** the tree-of-nodes structure itself
(`DelayNodeParams::parentIndex` pointing at another node, not just the input)
and the "Insanity" wander concept (`nodeInsanity`, docs/FX_BANK.md
"NodeDelay" below). **What's deliberately different:** ChowMatrix's tree is
runtime-growable to an arbitrary size; this codebase's realtime-safety rules
(docs/ARCHITECTURE.md -- fixed-capacity containers everywhere on the audio
thread) make that a poor fit, so `NodeDelay` uses a fixed `kMaxDelayNodes = 6`
instead. That's a real capability difference, not hidden here: six nodes is
enough for a genuinely branching, non-repeating texture (see
`content/presets/fx-node-tree.murmur`), but not an "infinitely growable" one.

### Cocoa Delay (tesselode)

A free, open-source ([GitHub](https://github.com/tesselode/cocoa-delay),
[manual](https://github.com/tesselode/cocoa-delay/blob/master/manual.md)) warm
analog/tape-style delay. Feature set: delay time drift (wow/flutter), Airwindos-
based saturation on the drive control, adjustable wet-level ducking under the
input signal, and Static/PingPong/Circular pan modes.

**What informed `effects::TapeDelay`:** all four of the above -- shared
wow/flutter drift LFO, drive-normalized saturation
(`dsp::softSaturate`) on the feedback path, an envelope-follower-based ducking
gain, and the same three pan-mode names/behaviors. The DSP itself
(one-pole envelope follower, tanh saturation, cross-feedback ping-pong) is this
project's own from-scratch implementation -- none of Cocoa Delay's Lua/C++
source or Airwindows saturation curve was read or ported.

### ValhallaFreqEcho (Valhalla DSP)

A commercial, closed-source plugin
([product page](https://valhalladsp.com/shop/delay/valhalla-freq-echo/)).
Combines a Bode-style frequency shifter with a delay by placing the shifter
*inside the feedback loop*: every repeat's frequency content is shifted by a
further fixed Hz amount (not scaled proportionally, the way a pitch shifter
would), so a held chord's harmonics drift progressively out of tune with each
echo -- small shifts give subtle chorusing, larger ones give barberpole phasing,
endless glissandos, and self-oscillating runaway sweeps at high feedback.
Low/high-pass filtering in the loop shapes the repeats' tone.

**What informed `effects::FreqShiftEcho`:** the core architectural idea --
frequency shifter *inside* the feedback loop, not applied once to the dry
signal -- plus the low-cut/high-cut shaping in that loop. The frequency-shifting
DSP itself (a from-scratch FIR Hilbert transformer + single-sideband
modulator, `dsp::HilbertTransformer`/`dsp::FrequencyShifter`) is this project's
own implementation of the openly-documented Bode/SSB frequency-shifting
technique (see "The FrequencyShifter primitive" below) -- Valhalla's specific
DSP internals are proprietary and were never available to read.

## The invented algorithm: FractalEcho

Requested explicitly: "consider ideas that have never been implemented before.
Invent a new way to do it and implement." `effects::FractalEcho`
(`pw8/effects/FractalEcho.hpp`) is this project's own design, not modeled on
any of the three plugins above or anything else researched.

**The idea.** Every delay-tree effect researched here -- ChowMatrix included --
is *manually patched*: a sound designer sets each node's parameters by hand.
FractalEcho instead **procedurally generates** a small (6-node), self-similar
delay tree from a single 64-bit seed: `dsp::DeterministicRng`-driven, with each
node's base delay time scaled by a fixed ratio raised to that node's depth in a
small fixed binary-tree connectivity (so deeper nodes get proportionally
*shorter*, self-similar delay times -- a Cantor-set-like rhythmic structure,
which is where the name comes from), plus per-node feedback/pan/level also
drawn from the seed.

That alone would just be "random preset generator," which isn't new. The actual
invention is what happens with **two** such generated topologies (`seedA`,
`seedB`) and a single continuous `fractalMorph` parameter: instead of the usual
signal-domain crossfade (run both networks, blend their audio output) or a
hard preset-to-preset snap, **every node's coefficients themselves are
interpolated in real time** -- `lerp(topologyA[i].delayMs, topologyB[i].delayMs,
morph)`, and likewise for feedback/pan/level, recomputed every sample. Turning
one knob continuously reshapes the delay network's actual rhythm and stereo
image, not just how much of a fixed effect is mixed in. Because both
topologies came from the same generator with the same bounded ranges
(feedback always clamped to `[0, 0.92]`, for instance), every point along the
morph is automatically stable -- there's no discontinuity to hide with an
envelope or crossfade timer, because there isn't one.

This is a genuinely novel *combination* (procedural fractal-tree generation +
continuous coefficient-space topology morphing, tied to this project's existing
deterministic-seed philosophy that already drives the arpeggiator's Random mode
and every LFO's sample-and-hold) -- not a claim that delay effects, fractals, or
parameter interpolation are individually new ideas.

**Verified, not just designed:** `tests/unit/EffectsTests.cpp` proves
determinism (same seed -> same topology, byte-for-byte), proves the two seeds
actually diverge, proves the depth-scaling rule holds exactly (a `ratio=0.5`
generator produces node depths at 200ms/100ms/50ms, matching the geometric
series by construction), and sweeps `fractalMorph` continuously from 0 to 1
across 20,000 samples while asserting every single sample stays finite and
bounded -- there is no morph position that glitches.
`content/presets/fx-fractal-morph.murmur` demonstrates it audibly.

## Architecture

`effects::EffectSlotParams` is one flat struct with fields for all six effect
types plus `type` (an `EffectType` enum) and a shared `mix` -- the same
"every field always present, only the active type's are used" pattern already
used by `patch::OperatorPatch` for its 5 synthesis engine types, so changing a
slot's type at runtime never reallocates or restructures the patch.

- `LayerPatch::insertEffects` -- 3 slots, applied to that layer's summed voice
  output before the master bus (per the master spec's "3 layer insert slots").
  Only Layer A is voiced in this pass (Phase 8), so only its insert chain does
  anything yet.
- `Patch::masterEffects` -- 4 slots, applied to the final mixed stereo bus
  (per the master spec's "4 master slots").

Each slot's *processor* (`effects::EffectSlotProcessor`) holds one instance of
all six algorithms' state (delay lines, RNG, Hilbert transformer state, etc.) --
a plain struct-of-all rather than a `std::variant`/tagged union, matching this
codebase's existing dispatch-by-enum style used everywhere else (`EngineType`,
`FilterMode`, `ArpMode`). This costs more memory per slot than a variant would
(each slot reserves buffers for every algorithm, not just the active one -- a
few megabytes per slot, entirely reasonable for a desktop synth) in exchange for
a live effect-type change never glitching from reading another effect's stale
buffer, and no variant-visitor boilerplate.

All delay buffers are `dsp::DelayLine`, a mono circular buffer sized once in
`prepare()` (control-thread, `std::vector::assign` -- the same
control-thread-allocates/audio-thread-only-reads pattern already used by
`oscillator::WavetableTable::MipLevel::samples`) and read with linear
interpolation on the audio thread thereafter, never resized there. `Engine`
resets both chains (clears delay tails, does not reallocate) on every
`loadPatch()`, so rendering stays deterministic from a fresh load regardless of
what was loaded before.

## The FrequencyShifter primitive

`dsp::HilbertTransformer` + `dsp::FrequencyShifter` (`pw8/dsp/HilbertTransformer.hpp`)
implement single-sideband frequency shifting -- shifting *every* frequency
component of a signal by a fixed Hz amount, which is what makes a "psychedelic
echo" sound different from a pitch-shifting one (pitch shifting scales
frequencies proportionally, preserving harmonic ratios; frequency shifting adds
a constant offset, breaking them).

The technique: approximate the input's *analytic signal* (`x(t) + j*H{x(t)}`,
where `H` is the Hilbert transform -- a 90-degree phase shift at every
frequency) using a finite-length linear-phase FIR filter built from the
textbook ideal discrete Hilbert kernel `h[n] = 2/(pi*n)` for odd `n`, 0 for even
`n` (see Oppenheim & Schafer, *Discrete-Time Signal Processing*), tapered with a
Hamming window. Because that kernel is antisymmetric about its center tap, the
filter's group delay is *exactly* `kCenter` samples by construction -- provable
directly from the filter's symmetry, which is why the companion "real" branch
is simply the input delayed by that same amount rather than a second filter
that needs independent tuning. The two time-aligned branches are then
single-sideband modulated against a quadrature oscillator
(`real*cos(phase) - imag*sin(phase)`) to produce the shifted output.

**This replaced an earlier, broken attempt.** The first implementation used a
cascaded-allpass-filter Hilbert transformer (the technique analog Bode shifters
and many digital ones use) with a specific published coefficient set recalled
from memory. It compiled and ran, but direct measurement
(`tests/unit/EffectsTests.cpp`'s `FrequencyShifter shifts a pure tone` test,
built to *prove* the shift amount via FFT peak analysis rather than assume it)
caught that the two allpass branches were producing nearly identical output --
not the ~90-degree-apart quadrature pair the technique requires -- meaning the
remembered coefficients didn't actually belong to the difference-equation
convention implemented alongside them. Rather than debug unfamiliar magic
numbers, the FIR approach above was substituted: every property it relies on
(exact group delay from symmetry, correct kernel shape) is independently
verifiable from the design itself rather than trusted from a remembered table.
The lesson generalizes: **measure the actual frequency-domain behavior a DSP
block produces before shipping it, especially when a technique depends on
precise coefficients** -- consistent with this project's established pattern of
tracing expected behavior (or, here, literally measuring it) before trusting
that a subtle DSP claim holds.

## GATE 10: Reverb, EQ, Compressor, Limiter

The master spec's "first effect set" basics -- absent from the initial FX bank
pass above -- landed in a later GATE 10 pass (docs/ROADMAP.md), rounding the
bank out to 10 algorithms. All four are original implementations of
well-documented, openly-published DSP techniques (same citation discipline as
everything else in this codebase), not ports of any product:

- **Reverb**: a compact 4-line Feedback Delay Network (Jean-Marc Jot-style,
  Householder reflection matrix for feedback mixing -- `mixed[i] = lineOut[i]
  - (2/N)*sum(lineOut)`, unitary/energy-preserving for any N, no explicit
  matrix multiply needed). Pre-delay, per-line damping (a one-pole lowpass in
  the feedback path), and an RT60-derived per-line decay gain. The 4 delay
  lines use mutually-irrational-ish base lengths to avoid metallic,
  rational-ratio comb ringing.
- **Eq**: a 3-band parametric (low shelf, mid peak, high shelf), each band a
  `dsp::Biquad` using the RBJ Audio EQ Cookbook formulas -- the standard,
  openly-published reference (same citation category as this codebase's
  PolyBLEP oscillator and TPT state-variable filter).
- **Compressor**: feedforward, stereo-linked peak detection (a single shared
  envelope driven by `max(|L|,|R|)`, so a transient in either channel ducks
  both equally rather than shifting the stereo image), a quadratic soft-knee
  gain computer, and separate attack/release smoothing of the gain-reduction
  envelope itself in dB (so perceived speed stays consistent across levels).
- **Limiter**: a *lookahead* peak limiter, not just a fast compressor --
  guarantees no overshoot above the ceiling by construction. Every incoming
  sample's own "gain needed to not exceed the ceiling" is recorded into a
  small ring buffer; the gain actually applied to the output (which reads
  `limiterLookaheadMs` behind the input) is the minimum of every recorded gain
  within that lookahead window, so by the time a loud sample reaches the
  output tap, the gain has already had the full window to ramp down to what
  it needs. Deliberately the simple O(window) sliding-minimum version (a real
  mastering-grade limiter would want an O(1)-amortized monotonic deque for a
  much longer lookahead; a synth's insert/master limiter doesn't need one).

**A real bug caught before shipping, not after**: Reverb's pre-delay,
parameterized down to 0ms, initially produced ~200ms of total silence instead
of near-zero pre-delay. Cause: `dsp::DelayLine` reads before writing (the
convention every delay-based effect in this codebase uses), so a delay of
*exactly* 0 samples reads whatever was written a full buffer length ago, not
"this sample, undelayed." Caught immediately by this effect's own render-level
test (expected a short decaying tail shortly after an impulse, measured
literal silence) -- fixed by flooring the pre-delay at 1 sample rather than
allowing a true 0, the same floor every other delay-based effect in this
codebase already has (TapeDelay/NodeDelay/FreqShiftEcho all clamp their delay
parameters' minimums to 1ms+ for exactly this reason, just never needed a
0ms-*meaning*-"none" case before Reverb's pre-delay did).

## GATE 11: Reverb redesign -- "nuanced and massive"

GATE 10's Reverb (above) was deliberately the simplest correct FDN that could
prove the integration end to end: 4 lines, one damping filter, no early/late
split. Per explicit user direction ("research Bricasti M7 and make Reverb algos
adhere to these principles. It needs to be nuanced and massive"), it was
redesigned from the ground up, informed by (not ported from) a family of
well-published, openly-documented algorithmic reverb techniques rather than
any single product:

- **The late tank grew from 4 to 8 Householder-mixed FDN lines.** CloudSeed's
  documented approach to "massive, modulated ambient spaces" is specifically to
  use many delay lines for density -- more lines is the single biggest lever
  against the metallic/flutter character a small FDN has, and the Householder
  reflection matrix (kept from GATE 10) is already O(N) and unitary regardless
  of N, so this was a mechanical, low-risk extension of the existing math.
- **A Schroeder/Dattorro-style series-allpass input diffuser** (4 stages) now
  sits between pre-delay and the tank, standard practice in essentially every
  published algorithmic reverb design in this family (Freeverb/Dragonfly's
  underlying engine, FAUST's reverb library, STK's classic JCRev/PRCRev).
  `reverbDensity` controls how many stages are engaged (echo density *building
  up* over the chain -- each stage always keeps processing and updating its own
  internal delay line regardless of how much it's blended into the output, so
  live-automating density crossfades smoothly instead of popping a stage in
  with stale buffer content); `reverbDiffusion` controls each engaged stage's
  own allpass coefficient (instantaneous smoothness). These are two genuinely
  different, independently-useful controls, not one knob wearing two names.
- **Frequency-dependent (multiband) decay** is the single most distinctive
  researched principle, and the one most directly informed by (not ported
  from) the parameter architecture documented in the actual Bricasti M7 owner's
  manual (Rev 5.02.08, fetched and read directly, not inferred from secondhand
  summaries -- an earlier web search surfaced a plausible-sounding but *wrong*
  claim that the M7 has "Spin"/"Wander" controls, which are actually Lexicon's
  terms; the real manual doesn't use them, and the actual controls it does
  document are considerably more specific and useful as an engineering
  reference than that would have been). The manual defines "Reverb Time" as
  explicitly *mid-frequency* reverb time, with independent "HF RT MPY" (0.2 to
  1.0x)/"HF Crossover" and "LF RT MPY" (0.2 to 4.0x, i.e. bass can ring
  *longer* than mid, not just shorter)/"LF Crossover" controls layered on top.
  `reverbHighRatio`/`reverbHighCrossoverHz`/`reverbLowRatio`/
  `reverbLowCrossoverHz` are a direct implementation of that same parameter
  shape. The DSP technique realizing it -- per Jot's own published work on FDN
  "absorptive filters" -- computes an independent target per-pass gain for the
  low/mid/high bands from each band's own RT60 and how much of it a given
  line's specific length represents, then combines them as a low-shelf +
  high-shelf `dsp::Biquad` pair around the flat mid-band gain (which is why
  `dsp::Biquad` gained a `setLowpass` method alongside its existing shelf/peak
  formulas -- reused again for the final "Roll Off" stage below).
  This computes fresh biquad coefficients per line per sample (16 shelf
  coefficient recomputes for 8 lines), matching this codebase's established
  precedent of prioritizing correctness/live-automation-safety over avoiding
  recomputation (Saturation/Compressor/etc. already recompute their own
  derived quantities fresh every sample); benchmark/fuzz timing after the
  change confirmed this stays comfortably realtime (see "What's covered by
  tests" below).
- **Per-line, per-line-decorrelated delay-length modulation** in the late tank
  (`reverbModDepth`/`reverbModRateHz`, each line getting its own rate ratio and
  phase offset so the 8 lines' modulation never synchronizes) is the M7
  manual's "Reverb Modulation: amount of modulation and pitch variation in the
  later part of the reverberant field" -- the mechanism that keeps a dense
  multi-line network sounding smooth and alive rather than ringing at fixed
  comb frequencies, the same principle behind Dattorro's tank allpasses and
  CloudSeed/Greyhole-style modulated delay networks.
- **Early reflections are now a separate, parallel, independently-leveled
  engine** -- a fixed 8-tap discrete cluster off one shared delay line, scaled
  by `reverbSizeParam`, panned alternating L/R for width, mixed against the
  late tank via independent `reverbEarlyLevel`/`reverbLateLevel` controls.
  This matches the M7 manual's explicit early/late engine split ("Early/Reverb
  Mix": two independent 0-20 levels) and the early/late separation described
  in Dattorro's reverberator paper and SuperCollider's JPverb. The M7's
  further "Early Select" (choice of early-reflection build-up/decay character)
  and its dedicated below-80Hz early engine are not implemented -- one
  well-designed fixed early-tap pattern, PLANNED for more variety later (see
  "What's PLANNED" below), consistent with how NodeDelay/FractalEcho are each
  one well-designed algorithm rather than exposing every possible variant.
- **`reverbSizeParam` is explicitly decoupled from `reverbDecaySeconds`**
  (scales delay-line/early-tap lengths, not decay time) and a dedicated
  **`reverbRollOffHz`** (final output lowpass) and **`reverbVlfCutDb`**
  (low-shelf cut of very-low-frequency wet content, -18 to 0dB) round out the
  M7's "Roll Off" and "VLF Cut" controls -- both explicit, named, separate
  controls in the manual, not folded into the multiband decay controls the way
  a less careful design might.

`EffectSlotParams`'s Reverb field count grew from 4 to 15 (`reverbDampingHz`
retired, replaced with the 12 fields above); `EffectSlotParams` overall grew
43 -> 54 fields, and plugin automation grew 501 -> 578 parameters (7 slots x 54
fields is 378, replacing 7 x 43's 301). `reverbDampingHz` is still *read* from
old documents for backward compatibility (seeding `reverbHighCrossoverHz` with
its old frequency value and defaulting `reverbHighRatio` to a fixed 0.5 as an
honest approximation of what a single one-pole feedback-path filter sounded
like) -- handled entirely inside `EffectSlotParams`'s own JSON defaulting logic
via key presence, not a schema version bump, the same kind of per-field
compatibility decision GATE 10 itself made when it grew `EffectSlotParams` from
23 to 43 fields without needing one.

## What's covered by tests

- `tests/unit/EffectsTests.cpp` (32 cases): Saturation transparency/compression;
  Chorus transparency and fixed-delay impulse response; TapeDelay Static-mode
  echo spacing/decay and PingPong's channel alternation; NodeDelay's
  parent-child chaining (proving a child hears its *parent's output*, not just
  the input) and disabled-node exclusion; the `FrequencyShifter` primitive's
  measured shift amount (FFT peak analysis) and `FreqShiftEcho`'s bounded
  output; FractalEcho's topology determinism, seed divergence, depth-scaling
  rule, and finite/bounded output across a full continuous morph sweep;
  `EffectChain`'s Bypass transparency and in-series slot processing; Eq's
  low-shelf boost and mid-peak cut measured directly via steady-state RMS;
  Compressor's measured gain reduction matching the expected hard-knee formula
  within margin, and leaving a below-threshold tone untouched; Limiter's
  no-overshoot guarantee under a sustained tone well above its ceiling plus a
  sharp mid-render transient, and an exact (undistorted, just delayed) mix=0
  passthrough found via empirical lag search; Reverb's decaying (not static or
  runaway) tail after an impulse, transparent mix=0 passthrough, and 7 GATE 11
  cases measuring the redesign's specific claims directly: high-band energy
  fading relative to low-band energy across the tail when `reverbHighRatio` is
  well below `reverbLowRatio` (multiband decay); a measurably lower early-window
  crest factor with diffusion/density engaged vs. off (density/diffusion);
  finite, bounded output at maximum modulation depth/rate over a long 20-second
  decay (modulation stability); an early-only render going silent well before a
  late-only render's tail does (early/late independence); measured low-band
  attenuation with VLF Cut engaged and high-band attenuation with a low Roll
  Off (output tone shaping); and the first meaningful response arriving
  measurably later at `reverbSizeParam=3.0` than at `0.3` while
  `reverbDecaySeconds` stays fixed (size/decay decoupling).
- `tests/regression/RenderSanityTests.cpp`: a master TapeDelay slot turning one
  short hit into several measured amplitude onsets (the same windowed-RMS
  onset-counting technique proven for the arpeggiator), a layer insert
  Saturation slot measurably lowering a deliberately loud patch's peak, a
  master Limiter slot capping a deliberately hot chord's peak below its
  ceiling, and a master Compressor slot (no makeup gain, so the claim reduces
  to "gain reduction can only lower peak") measurably lowering a loud chord's
  peak.
- 125 total tests, 895,937 assertions, all passing. `murmur-fuzz-render`'s
  `randomPatch()` randomizes all 3 insert + 4 master `EffectSlotParams` slots,
  including every one of Reverb's new fields -- 1,000 patches (seed 14, post
  GATE 11), zero failures (198.14s, 5.0 patches/sec -- down slightly from GATE
  10's 5.9, expected given the reverb's 8 lines/diffuser/early cluster/two
  extra output filters vs. the old 4-line version, still well clear of the
  fuzz tool's own 2-second-per-patch "slow" flag).
- Full cross-build verification: dev/benchmarks/python/plugin all rebuild
  clean; `auval` reports 578 published parameters and passes in full;
  `pluginval --strictness-level 5` SUCCESS on both VST3 and AU at that count.

## Content

Five engineering presets: `content/presets/fx-node-tree.murmur` (NodeDelay, plus a
subtle layer-insert Chorus in the same patch -- demonstrating both slot types
together), `content/presets/fx-fractal-morph.murmur` (FractalEcho),
`content/presets/fx-freq-echo.murmur` (FreqShiftEcho),
`content/presets/fx-master-chain.murmur` (all 4 GATE 10/11 algorithms arranged as
a realistic mastering chain across the 4 master slots: Eq -> Reverb ->
Compressor -> Limiter, its Reverb slot updated in GATE 11 to demonstrate
extended low-frequency ring, faster high-frequency decay, high diffusion/
density, and gentle late-tank modulation), and
`content/presets/gate4-massive-dark-metallic-bass.murmur` (predates the FX bank,
listed here for completeness of the preset count). TapeDelay is exercised by a
render regression test rather than a dedicated preset.

## What's PLANNED, not implemented

- **bitcrush, wavefold, ensemble, flanger, phaser, diffusion delay** -- still
  PLANNED per the original `pw8/effects/README.md` scope; this project has
  added 10 algorithms across two passes rather than working through that
  original list top-to-bottom, prioritized by explicit user requests both times.
- **Reverb "Early Select" and a dedicated below-80Hz early-reflection engine**
  -- the M7 manual documents both; this pass implements one well-designed fixed
  early-tap pattern instead (see "GATE 11" above). A discrete choice of
  early-reflection topologies (the way NodeDelay/FractalEcho are each one
  algorithm, not several) would be the natural next increment if more early-
  reflection variety is wanted later.
- **Mod-matrix-modulatable effect parameters** -- `EffectSlotParams` fields
  (e.g. `fractalMorph`, `freqShiftHz`, `compThresholdDb`) aren't yet mod matrix
  destinations, so a live-automated sweep requires host/plugin automation of
  the underlying patch field directly (which does exist for all of them now,
  see docs/PLUGIN_ARCHITECTURE.md "Automation"), not an in-engine LFO/envelope
  route. Natural next increment once the mod matrix's destination enum is
  extended.
- **Layer B insert effects** -- `LayerPatch::insertEffects` exists on both
  layers structurally, but only Layer A's chain is exercised (Layer B isn't
  voiced yet, Phase 8).
- **Sidechain input** -- Compressor's detector reads the same signal it's
  compressing; there's no external sidechain source (e.g. keying a pad's
  compressor off a kick) since effects only ever see the one signal already
  flowing through their slot.
- **`murmur-fuzz-render` effect randomization** -- see "What's covered by tests"
  above.
