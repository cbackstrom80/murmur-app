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
- **8 host-automatable parameters** (macros 1-8, `juce::AudioProcessorValueTreeState`)
  are live-wired end to end -- automating one mid-note-hold audibly changes a
  currently-sustaining voice, not just the next one -- and **`pluginval` passes at
  strictness level 5 (maximum)** on both the VST3 and the AU. See "Automation" and
  "pluginval" below.

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
32-sample sub-blocks -- this is the suite that actually drives the 8 macro
parameters through automation-style value changes mid-stream), Editor Automation,
Automatable Parameters, and (embedded) `auval`. This is a materially stronger signal
than `auval` alone: `auval` is AU-specific and doesn't exercise
block-size-varying/automation-under-processing scenarios the way `pluginval` does.

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

**IMPLEMENTED** (as far as the master spec's scope for this control surface goes).
Per the master spec, not every internal patch parameter is exposed as flat DAW
automation -- only the 8 macros are, matching the "8 routable macros" model
validated against Phase Plant's in the COMPETITIVE_ANALYSIS.md research pass.
`plugin/src/state/PluginState.h`/`.cpp` build a real
`juce::AudioProcessorValueTreeState` (`PatchworkEightProcessor::apvts`) with 8
`AudioParameterFloat`s (`macro1`..`macro8`, 0..1, matching `Patch::macros[0..7]`).

Two things had to be true for this to actually *work*, not just compile:

1. **A live macro change must reach a currently-held note, not just the next
   one.** `render::Engine::setMacroValue()` writes straight into every active
   voice's `macroValues` array -- a plain per-sample-read array (see
   `Voice::renderSample`), so a mod-matrix route from a macro (e.g. to filter
   cutoff or operator level) responds immediately. `processBlock()` pushes all 8
   parameters' current values into the running Engine every block via cached
   `std::atomic<float>*` pointers (`getRawParameterValue()`, the standard
   audio-thread-safe JUCE read path) -- proven, not just asserted, by
   `tests/unit/EngineMacroLiveUpdateTests.cpp`: mid-hold, muting a macro-routed
   operator via `setMacroValue()` measurably drops the still-playing voice's RMS,
   and un-muting restores it.
2. **A saved session must round-trip the macros' current values, not just a
   preset's defaults.** `getStateInformation()` calls
   `syncPatchMacrosFromParameters()` (APVTS -> `currentPatch_.macros`) before
   serializing; `loadPatch()`/`setStateInformation()` call the reverse
   (`syncMacroParametersFromPatch()`, `currentPatch_.macros` -> APVTS, via
   `setValueNotifyingHost()` so the host's own UI/automation lane sees it). This
   keeps `currentPatch_` the single source of truth (docs/PLUGIN_ARCHITECTURE.md
   "Host State") rather than a second, divergent store of the same 8 floats.

`auval` now reports 8 published parameters (`Checking parameter setting`/
`Checking ramped parameter scheduling` both pass -- previously skipped entirely
when there were zero parameters to test). `pluginval --strictness-level 5`
(the maximum) passes on both the VST3 and the AU, including its `Automation`,
`Plugin state`, and `Automatable Parameters` suites -- see "pluginval" below.

## Editor

`createEditor()` returns a `juce::GenericAudioProcessorEditor` -- JUCE's built-in
generic parameter-list editor, exactly the pragmatic placeholder this doc previously
said it *would* use once the target actually built. It's empty today (no
`AudioProcessorParameter`s are registered yet), but it's a real, consistent editor:
`hasEditor()` returning `true` with a non-null `createEditor()` result is a JUCE
internal-consistency invariant (`AudioProcessor::createEditorIfNeeded()` asserts on
exactly this), and standalone-launch testing caught the earlier
`nullptr`-returning version violating it.

## Signature UI: Graph

Per the master spec, UI work is explicitly deferred until the DSP graph is proven
stable ("do not spend substantial UI time until DSP graph is stable"). The eventual
centerpiece is a visual rendering of exactly the same structure
`AlgorithmGraphCompiler` already produces and `pw8-graph inspect` already prints in
text form:

```
        OP8
         |
         v
OP6 -> OP4 --+
             |
OP7 -> OP3 --+--> OUT
             |
OP2 -> OP1 --+
```

`plugin/src/ui/` is an empty, documented placeholder for that future work -- the
`GenericAudioProcessorEditor` above is explicitly a placeholder, not a step toward it.

## Visualization: spectrum, oscilloscope, waveform/wavetable previews (PLANNED)

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
3. Beyond the 8 macros, no other patch parameters are host-automatable (by
   design, per the master spec -- see "Automation" above) -- there's no
   automation lane for, say, filter cutoff or an effect's mix directly; that
   requires either mapping specific fields to more APVTS parameters or a
   generic "any patch field" automation scheme, neither of which exists yet.
4. The real PLAY/DESIGN/LAB UI (Phase 17) -- `GenericAudioProcessorEditor` is a
   placeholder (now showing 8 real macro sliders, not an empty list, but still
   not a step toward the signature UI).
5. Code signing / notarization for actual distribution (the build today produces an
   ad-hoc-signed VST3, sufficient for local testing only).
6. Spectrum analyzer / oscilloscope / wavetable preview visualization (architected
   above, GPU-accelerated via `juce::OpenGLContext`) -- documentation only, no code.
