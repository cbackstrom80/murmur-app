# FX Bank

**IMPLEMENTED** (Phase 11: FX, PLANNED -> PARTIAL). 6 real, independently-voiced
effect algorithms, 3 layer insert slots + 4 master slots (`pw8/effects/`), backed
by research into three named commercial delay/echo plugins plus one algorithm
this project invented itself. See docs/ROADMAP.md "Follow-up pass: FX bank" for
the summary and test/verification counts.

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
`content/presets/fx-node-tree.pw8`), but not an "infinitely growable" one.

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
`content/presets/fx-fractal-morph.pw8` demonstrates it audibly.

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

## What's covered by tests

- `tests/unit/EffectsTests.cpp` (16 cases): Saturation transparency/compression;
  Chorus transparency and fixed-delay impulse response; TapeDelay Static-mode
  echo spacing/decay and PingPong's channel alternation; NodeDelay's
  parent-child chaining (proving a child hears its *parent's output*, not just
  the input) and disabled-node exclusion; the `FrequencyShifter` primitive's
  measured shift amount (FFT peak analysis) and `FreqShiftEcho`'s bounded
  output; FractalEcho's topology determinism, seed divergence, depth-scaling
  rule, and finite/bounded output across a full continuous morph sweep;
  `EffectChain`'s Bypass transparency and in-series slot processing.
- `tests/regression/RenderSanityTests.cpp`: a master TapeDelay slot turning one
  short hit into several measured amplitude onsets (the same windowed-RMS
  onset-counting technique proven for the arpeggiator), and a layer insert
  Saturation slot measurably lowering a deliberately loud patch's peak.
- 96 total tests, all passing. `pw8-fuzz-render` (1,500 patches, seed 6) --
  zero failures; `randomPatch()` does not yet randomize `EffectSlotParams`
  (same documented gap as the arpeggiator's fuzz coverage, see docs/TESTING.md).
- Full cross-build verification: dev/benchmarks/python/plugin all rebuild
  clean; AU re-validated with `auval` in full.

## Content

Three new engineering presets, one per new algorithm family plus the invented
one: `content/presets/fx-node-tree.pw8` (NodeDelay, plus a subtle layer-insert
Chorus in the same patch -- demonstrating both slot types together),
`content/presets/fx-fractal-morph.pw8` (FractalEcho), and
`content/presets/fx-freq-echo.pw8` (FreqShiftEcho). TapeDelay is exercised by
the render regression test above rather than a dedicated preset in this pass.

## What's PLANNED, not implemented

- **Reverb** -- the master spec's first-effect-set item (basic reverb) and
  second-wave item (large FDN reverb) are both still absent; none of the six
  algorithms here is a reverb.
- **EQ, compressor, limiter, bitcrush, wavefold, ensemble, flanger, phaser,
  diffusion delay** -- all still PLANNED per the original `pw8/effects/README.md`
  scope; this pass added six *new* algorithms rather than working through that
  original list top-to-bottom, prioritized by the user's explicit request.
- **Mod-matrix-modulatable effect parameters** -- `EffectSlotParams` fields
  (e.g. `fractalMorph`, `freqShiftHz`) aren't yet mod matrix destinations, so a
  live-automated morph sweep (as `fx-fractal-morph.pw8`'s description
  describes) requires host/plugin automation of the underlying patch field
  directly, not an in-engine LFO/envelope route. Natural next increment once
  the mod matrix's destination enum is extended.
- **Layer B insert effects** -- `LayerPatch::insertEffects` exists on both
  layers structurally, but only Layer A's chain is exercised (Layer B isn't
  voiced yet, Phase 8).
- **`pw8-fuzz-render` effect randomization** -- see "What's covered by tests"
  above.
