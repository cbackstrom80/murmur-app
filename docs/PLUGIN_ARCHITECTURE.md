# Plugin Architecture

**STATUS: PARTIAL, build-verified.** `plugin/` builds against real JUCE 8.0.6
(`cmake --preset plugin && cmake --build --preset plugin`) and produces working
VST3, AU, and Standalone artifacts:

- **AU passes Apple's `auval` validation tool in full**: format tests, render tests
  at 22.05k/44.1k/48k/96k/192k Hz and multiple block sizes (64/137/512/4096 frames),
  a 1-channel test, a bad-max-frames failure-mode test, and a MIDI test all pass
  (`AU VALIDATION SUCCEEDED`).
- The **Standalone app launches and runs cleanly** (verified: process stays alive,
  no crash, no JUCE assertion failures).
- The **VST3** bundle builds and ad-hoc-signs successfully.
- **690 host-automatable parameters** (`juce::AudioProcessorValueTreeState`) --
  macros, Filter1, all 8 LFOs, all 8 operators, all 8 envelopes, layer gain/pan,
  master gain, all 7 FX slots' scalar controls (all 10 algorithms including
  GATE 11's redesigned 15-field multiband Reverb plus Eq/Compressor/Limiter),
  and the arpeggiator's scalar fields -- are live-wired end to end -- automating
  one mid-note-hold audibly
  changes a currently-sustaining voice (where physically meaningful; see
  "Automation" for the envelope exception), not just the next one -- and
  **`pluginval` passes at strictness level 5 (maximum)** on both the VST3 and
  the AU at this full parameter count. See "Automation" and "pluginval" below.

None of this has been tested inside a real DAW host yet, and there is no host-matrix
CI job beyond a single macOS build-and-`auval` check (`.github/workflows/ci.yml`'s
`plugin` job) -- see "What's still missing" below. Per the master spec's phasing,
the plugin is Phase 16, well after this pass's Phase 0/1(+2/3) target; what's here
now goes beyond "compiles" to "passes Apple's own validator," which is further than
the phase strictly requires, but was verified directly rather than assumed.

## pluginval

**IMPLEMENTED, `--strictness-level 5` (the maximum).** `pluginval` 1.0.4 (installed
via `brew install --cask pluginval` for this verification pass, not vendored into
the repo or CI yet -- see "What's still missing") was run directly against both
built artifacts:

```bash
pluginval --strictness-level 5 --validate "Patchwork Eight.vst3"
pluginval --strictness-level 5 --validate "Patchwork Eight.component"  # AU must be
    # copied into ~/Library/Audio/Plug-Ins/Components/ first -- macOS's AudioComponent
    # registry, not a direct file load, is how AU discovery works; pluginval fails
    # with "No types found" against an AU bundle path that isn't registered, which is
    # a discovery-mechanism quirk, not a plugin defect (confirmed by `auval`, which
    # uses the same registry, passing against the identical bundle).
```

Both **SUCCESS** at every suite: Open plugin (cold/warm), Plugin info, Editor, Open
editor whilst processing, Audio processing (44.1/48/96kHz x 64/128/256/512/1024
sample block sizes), Plugin state, Automation (same sample-rate/block-size matrix,
32-sample sub-blocks -- this is the suite that actually drives all 690
parameters through automation-style value changes mid-stream), Editor Automation,
Automatable Parameters, and (embedded) `auval`. This is a materially stronger signal
than `auval` alone: `auval` is AU-specific and doesn't exercise
block-size-varying/automation-under-processing scenarios the way `pluginval` does.
Re-run and re-confirmed SUCCESS after each expansion of the parameter count
(8 -> 270 -> 361 -> 501 -> 578 -> 610 -> 626 -> 658 -> 690) -- this is not a stale result from an earlier, smaller set.

## Design

`pw8_plugin` wraps `pw8::render::Engine` (the same class the native renderer and
Python bindings use) inside a `juce::AudioProcessor`. VST3, AU, and Standalone
formats are all produced from one `juce_add_plugin()` call in
`plugin/CMakeLists.txt`; CLAP is deliberately not attempted yet (the master spec:
"architect CLAP support but do not derail initial implementation").

### Host State

The plugin's `getStateInformation()`/`setStateInformation()` serialize the native
`.pw8` logical patch (via `patch::savePatchToJson`/`loadPatchFromJson`) directly --
there is exactly one patch format, not a divergent host-specific one. See
`PatchworkEightProcessor::getStateInformation/setStateInformation`.

### Threading

`processBlock()` never loads a patch, parses JSON, or allocates. Patch changes
(`PatchworkEightProcessor::loadPatch()`, and `prepareToPlay()` on sample-rate
change) build a fresh `pw8::render::Engine` off the audio thread and publish it via
a single atomic pointer swap (`publishEngine()`, `std::atomic<Engine*> activeEngine_`,
double-buffered `std::unique_ptr<Engine>` storage so the previous instance survives
until the *next* swap rather than being destroyed under a still-in-flight
`processBlock()` call). This is the prepare-then-atomic-swap pattern from
`docs/ARCHITECTURE.md`, deliberately not a hand-rolled lock-free queue.

### MIDI

`processBlock()` walks the incoming `juce::MidiBuffer` and translates note on/off,
pitch wheel, CC, channel pressure, and poly aftertouch directly into
`pw8::render::Engine` calls -- the same entry points the native renderer's MIDI
dispatch uses (`pw8::render::Engine::noteOn/noteOff/pitchBend/controlChange/
channelPressure/polyAftertouch`), so plugin and offline-render MIDI handling can't
drift apart. `auval`'s MIDI test exercises this path directly and passes.

### Automation

**IMPLEMENTED, 690 parameters.** The original design here exposed only the 8
macros to host automation (matching Phase Plant's "8 routable macros" model,
per the COMPETITIVE_ANALYSIS.md research pass). Per explicit user direction
("every param should be automatable"), that scope was deliberately widened to
every scalar field that is both (a) POD -- safe to read/write from the audio
thread with zero allocation risk -- and (b) currently audible (Layer A, the
only voiced layer). It was widened again in the GATE 5 pass (docs/ROADMAP.md)
when the DSP itself grew from 1 LFO/1 envelope to 8 of each, again in the
GATE 10 pass when the FX bank grew from 6 to 10 algorithms (adding Reverb, Eq,
Compressor, Limiter -- 20 new scalar fields per slot), again in the GATE 11
pass when Reverb itself was redesigned from 4 to 15 fields (a net +11 per
slot) for its multiband/diffuser/early-late architecture, again when Engine
Type 3 (FM/PM) shipped, adding 4 fields per operator for its self-contained
internal modulator, again when Engine Type 7 (NoiseChaos) shipped
(`noiseVariant`/`noiseRate`, +2 fields per operator, +16 total), again when
Engine Type 5 (PhaseShape) shipped (`phaseBend`/`phaseFold`/
`phaseAsymmetry`/`phaseShape`, +4 fields per operator, +32 total), and again
when Engine Type 4 (Additive) shipped (`additivePartialCount`/
`additiveTilt`/`additiveOddEven`/`additiveStretch`, +4 fields per operator,
+32 total) -- the first four of the 6 previously-silent operator engines to
gain real automatable parameters.
`plugin/src/state/PluginState.h`/`.cpp` builds a real
`juce::AudioProcessorValueTreeState` (`PatchworkEightProcessor::apvts`) with:

| Group | Count | Fields |
|---|---|---|
| Macros | 8 | `macro1`..`macro8` |
| Filter1 | 5 | enabled, mode, cutoffHz, resonance, keyTrack |
| 8 LFOs | 40 | waveform, mode, rateHz, syncDivisionIndex, phaseOffset (x8) |
| 8 operators | 184 | engine, waveform, morph, pulseWidth, wavetableFramePosition, frequencyRatio, fixedFrequencyHz, keyTrack, level, fmModulatorRatio, fmModulatorIndex, fmModulatorFeedback, fmModulatorWaveform, noiseVariant, noiseRate, phaseBend, phaseFold, phaseAsymmetry, phaseShape, additivePartialCount, additiveTilt, additiveOddEven, additiveStretch (x8) |
| 8 envelopes | 64 | delay, attack, hold, decay, sustain, release, curve, legato (x8) |
| Layer gain/pan, master gain | 3 | `layerGain`, `layerPan`, `masterGain` |
| 3 insert + 4 master FX slots | 378 | type, mix, and every scalar knob for all 10 algorithms incl. GATE 11's redesigned Reverb/Eq/Compressor/Limiter (54 fields x 7 slots) |
| Arpeggiator | 8 | enabled, mode, rateMode, rateHz, syncDivisionIndex, octaveRange, numSteps, latch |
| **Total** | **690** | |

Automating an LFO affects both its VOICE-scope per-voice instance and its
LAYER/GLOBAL-scope shared instance simultaneously (`Engine::setLfoLive()`) --
scope itself lives on each `ModRoute`, and the mod-route list isn't
automatable (see below), so one parameter per LFO field is already everything
a host-automated LFO can meaningfully mean. See MODULATION.md "LFOs" for what
LAYER/GLOBAL scope actually does.

**Explicitly out of scope**, with rationale (same POD/audibility bar applied
consistently, documented in full in `plugin/src/state/PluginState.h`):
std::string fields (wavetableId, all patch/macro metadata) -- not a
continuous-automation-shaped data type; algorithm graph topology and the
mod-route list -- structural patch-editing data, not a performance knob (the
mod-route list did gain its own non-automation live-editing path in UI GATE 3,
`Engine::setModRoutesLive()`/drag-to-modulate -- see [UI.md](UI.md) -- but
still isn't a host-automatable parameter, for the same reason);
the arpeggiator's 64-step array and NodeDelay/FractalEcho's node-tree
arrays/seeds -- same reasoning, and already fully covered by the native `.pw8`
JSON round-trip regardless; unison and layer width/centerGravity -- schema-
present but not DSP-wired yet (Phase 7/8 PARTIAL), so publishing automation
for them would audibly do nothing, which is worse than honestly excluding it;
Layer B's operators/filter/LFOs/envelopes -- Layer B isn't voiced yet (Phase 8);
`voiceSettings.polyphony`/`a4Hz` and the patch seed -- structural/tuning-
reference changes, not continuous performance parameters.

Three things had to be true for this to actually *work* at this scale, not
just compile:

1. **A live parameter change must reach a currently-held note, not just the
   next one, for every group where that's physically meaningful.**
   `render::Engine`'s "Live parameter API" (`setFilterLive`/`setLfoLive`/
   `setOperatorLive`/`setLayerGainLive`/`setLayerPanLive`/`setMasterGainLive`)
   writes straight into every active voice's live per-sample-read fields (the
   same pattern `setMacroValue()` established) -- proven by
   `tests/unit/EngineLiveParamsTests.cpp`: a lowpass filter closing mid-hold
   measurably darkens a still-ringing saw, and muting an operator's level
   mid-hold measurably silences it. Effect slots don't even need a
   voice-push step: `Engine::process()` already reads
   `patch_.layerA.insertEffects`/`patch_.masterEffects` fresh every sample, so
   `setInsertEffectLive()`/`setMasterEffectLive()` just overwrite those
   structs directly and the very next sample picks it up (also tested).
   The one deliberate exception is the amp envelope: `DahdsrEnvelope` captures
   its targets/coefficients once at `noteOn()` with no live mid-ramp-retarget
   API, so `setEnvelopeLive()` takes effect on the *next* note-on, not an
   already-ringing voice's current ramp -- documented on the method itself
   rather than silently glossed over.
2. **Automating the arpeggiator's rate/mode/etc. must not restart its
   pattern.** `Arpeggiator::configure()` resets held notes, pattern position,
   and pending ratchet/tie events -- calling it every block would make the arp
   re-start from step 0 constantly and never actually play. A new
   `Arpeggiator::setLiveParams()` swaps `params_` without resetting anything;
   `Engine::setArpeggiatorScalarLive()` uses it, and additionally preserves the
   loaded per-step pattern (`.steps`) by merging only the 8 scalar fields into
   the existing `patch_.arpeggiator` rather than overwriting the whole struct.
   `tests/unit/EngineLiveParamsTests.cpp` proves this directly: two held notes
   keep producing arp-triggered onsets across a live rate change with no
   intervening note-on/note-off.
3. **A saved session must round-trip every automatable field's current
   value, not just a preset's defaults**, and NodeDelay/FractalEcho's
   non-automated `nodes[]`/seed fields must survive a live effect-parameter
   push untouched. `getStateInformation()` calls
   `syncPatchFromAllParameters()` (APVTS -> `currentPatch_`) before
   serializing; `loadPatch()`/`setStateInformation()` call the reverse
   (`syncAllParametersFromPatch()`, via `setValueNotifyingHost()` so the
   host's own UI/automation lane sees it). Effect-slot pushes in
   `processBlock()` read the engine's current `EffectSlotParams` first (via
   `getInsertEffectParams()`/`getMasterEffectParams()`) and only overwrite the
   54 automated scalar fields, preserving whatever isn't exposed. This all
   keeps `currentPatch_` the single source of truth (see "Host State" above)
   rather than a second, divergent store of the same ~690 values.

`auval` reports exactly 690 published parameters (confirmed:
`grep -c "Parameter ID:"` against its verbose output) and passes
`Checking parameter setting`/`Checking ramped parameter scheduling` in full.
`pluginval --strictness-level 5` (the maximum) passes on both the VST3 and the
AU at this full parameter count, including its `Automation`, `Plugin state`,
and `Automatable Parameters` suites -- see "pluginval" below.

Implementation note: enum-valued parameters (filter mode, effect type, etc.)
are exposed as stepped `AudioParameterFloat`s rather than
`AudioParameterChoice`, for implementation uniformity across all ~690
parameters (one read path, `getRawParameterValue()->load()`, for everything).
A host's own generic automation-lane UI shows a continuous slider rather than
a named dropdown for these regardless (that's inherent to `AudioParameterFloat`
vs. `AudioParameterChoice`, independent of our own editor); PLAY mode's
`GlowKnob` now presents formatted text ("LOWPASS", "SINE", ...) for the ones
it surfaces directly, via an optional per-knob formatter -- see
[UI.md](UI.md). Frequency-ratio-style parameters (`op{N}FreqRatio`,
0.001..128) are also linear rather than log-scaled -- a known, minor UX rough
edge, not a correctness issue.

## Editor

**IMPLEMENTED -- PLAY mode, the OBSIDIAN skin.** `createEditor()` returns
`ui::PlayModeEditor`, a real custom `juce::AudioProcessorEditor` --
`juce::GenericAudioProcessorEditor` is no longer used. `hasEditor()` returning
`true` with a non-null `createEditor()` result is a JUCE internal-consistency
invariant (`AudioProcessor::createEditorIfNeeded()` asserts on exactly this,
and `pluginval`'s Editor/Editor Automation suites exercise it directly under
automation-style value changes and resize scenarios); both AU and VST3
re-confirmed `pluginval --strictness-level 5` SUCCESS against the real editor,
not just the parameter layer underneath it. Full design writeup, architecture,
and a real geometry bug caught and fixed while building it: see
[UI.md](UI.md).

## Signature UI: Graph

**IMPLEMENTED** as `ui::AlgorithmGraphView`, PLAY mode's centerpiece -- a
rendering of exactly the same structure `AlgorithmGraphCompiler` already
produces and `pw8-graph inspect` already prints in text form, with the
8 nodes at fixed positions on a circle (not a draggable modular patcher --
the master spec's "no visible patch-cable spaghetti" constraint, satisfied
architecturally rather than just cosmetically) and edges rendered as curved,
`EdgeType`-colored lines with an animated traveling pulse. The graph's
*shape* stays read-only (editing it is DESIGN-mode/PLANNED), but as of UI
GATE 3, clicking a node is interactive: it opens that operator's controls in
`OperatorEditorPanel`. See [UI.md](UI.md) "The algorithm graph view" and "UI
GATE 3" for the full detail, including what the pulse does and doesn't
represent (structural, not literal audio-level metering).

## Visualization: spectrum, oscilloscope, waveform/wavetable previews (PLANNED)

**Update:** the wavetable-preview third of this section shipped as
`WavetableStackView` -- see `docs/VISUALIZATION_UI_GATE5.md` for what's built
(not yet wired into `PlayModeEditor`) versus what's still a spec (spectrum,
oscilloscope -- both need the audio-thread tap this section originally
described, which still doesn't exist).

Researched and architected at the user's request; **not implemented** -- this is
the same documentation-only treatment as `docs/GPU_ACCELERATION_RESEARCH.md`, and
the two documents are deliberately about two *different* uses of "GPU" that
shouldn't be conflated:

- `GPU_ACCELERATION_RESEARCH.md` is about offloading **DSP compute** (synthesis,
  convolution, resonator banks) to a GPU compute API (CUDA) -- declined for now,
  for portability and complexity reasons that have nothing to do with graphics.
- This section is about **GPU-accelerated UI rendering** -- using `juce::OpenGLContext`
  to hardware-accelerate whatever the editor draws. This is low-risk, standard
  practice for real-time-updating plugin UIs (spectrum analyzers, scopes, and
  wavetable displays are the canonical case a GPU-backed `Component` render path
  exists for), has no NVIDIA-specific dependency (OpenGL is cross-vendor; JUCE also
  has a Metal-adjacent path on Apple platforms), and doesn't touch `pw8_core` at
  all -- it's purely a `plugin/` concern.

Three visualization surfaces, all PLANNED for Phase 17, sharing the same
audio-tap-plus-render-loop shape:

1. **Spectrum analyzer** (live). Needs a realtime-safe tap of `processBlock()`'s
   output into a fixed-capacity ring buffer (single-producer/single-consumer:
   audio thread writes, UI-thread timer reads a snapshot -- no allocation, no
   locking, matching the realtime rules in `docs/ARCHITECTURE.md`). The UI-thread
   timer callback runs `pw8::dsp::fft` (already implemented, `pw8/dsp/Fft.hpp` --
   built for wavetable mip generation but equally usable for analysis) over the
   latest window and paints a magnitude spectrum. `juce::OpenGLContext` attached to
   the top-level editor component accelerates that repainting at animation rates
   without the DSP core needing to know a UI exists.
2. **Oscilloscope** (live). Same ring-buffer tap as the spectrum analyzer; paints
   raw time-domain samples instead of an FFT magnitude. Shares the tap
   infrastructure and the OpenGL-accelerated paint path -- the two are really one
   piece of plumbing with two different `paint()` implementations on top.
3. **Waveform/wavetable previews** (static, no audio tap needed). Rendering a
   `oscillator::WavetableTable` frame -- or a sweep across its frames/mips -- is
   just reading `MipLevel::samples` and drawing a line; unlike the two live views
   above, this needs no realtime audio-thread involvement at all, since the table
   data already lives in memory once loaded (`Engine::loadPatch()`). This is the
   natural place to *see* what `pw8-wavetable-builder` produced (mip level,
   harmonic content, frame morph) without leaving the plugin. Still benefits from
   the same `juce::OpenGLContext` acceleration if drawn inside the same
   GPU-backed editor, but doesn't require it functionally the way the live views do
   (nothing here is animating at audio-block rate).

None of the three requires any change to `pw8_core`'s realtime-safety contract --
the tap is a plugin-side addition, `pw8::dsp::fft` already exists and is control-path/
UI-thread-only here (not called from `processBlock()`), and the wavetable preview
reads data the engine already owns. All three wait on Phase 17 starting for the
same reason the rest of the UI does: proving the DSP first.

## What's still missing

1. Real DAW host-matrix testing (Ableton, Logic, Reaper, Bitwig, etc.) -- not done;
   `auval`, `pluginval` at max strictness, and a bare Standalone launch are strong
   but not equivalent signals to an actual DAW session.
2. `pluginval` isn't wired into CI yet -- it was installed and run locally
   (`brew install --cask pluginval`) for this verification pass, not vendored or
   added as a `.github/workflows/ci.yml` job.
3. A deliberately-excluded set of fields are still NOT host-automatable -- see
   "Automation" above's full list (Layer B entirely, unison, layer width/
   centerGravity, the arpeggiator's per-step array, effect delay-tree node
   arrays/seeds, algorithm graph topology, mod-route list, `wavetableId` and
   all string metadata, `voiceSettings.polyphony`/`a4Hz`, the patch seed).
   Each is excluded for a documented reason (string-typed, structural, or not
   DSP-wired yet), not an oversight -- but they're real gaps if a use case
   needs them.
4. The real PLAY/DESIGN/LAB UI (Phase 17) -- `GenericAudioProcessorEditor` is a
   placeholder (now showing all 690 real parameters as generic sliders, not an
   empty list, but still not a step toward the signature UI -- 690 flat
   sliders is not how this synth should actually be played).
5. Code signing / notarization for actual distribution (the build today produces an
   ad-hoc-signed VST3, sufficient for local testing only).
6. Spectrum analyzer / oscilloscope / wavetable preview visualization (architected
   above, GPU-accelerated via `juce::OpenGLContext`) -- documentation only, no code.
