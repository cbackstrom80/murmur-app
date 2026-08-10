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

None of this has been tested inside a real DAW host yet, and there is no host-matrix
CI job beyond a single macOS build-and-`auval` check (`.github/workflows/ci.yml`'s
`plugin` job) -- see "What's still missing" below. Per the master spec's phasing,
the plugin is Phase 16, well after this pass's Phase 0/1(+2/3) target; what's here
now goes beyond "compiles" to "passes Apple's own validator," which is further than
the phase strictly requires, but was verified directly rather than assumed.

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

Per the master spec, not every internal patch parameter is exposed as flat DAW
automation. `plugin/src/state/PluginState.h` documents the intended parameter ID
scheme (`macro1`..`macro8`, matching `Patch::macros[0..7]`) for the eventual
`juce::AudioProcessorValueTreeState` wiring -- **not yet implemented**
(`plugin/src/parameters/` is an empty, documented placeholder). `auval` reports zero
published parameters today, consistent with this.

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

## What's still missing

1. `pluginval` (beyond `auval`) -- PLANNED, not run in this pass.
2. Real DAW host-matrix testing (Ableton, Logic, Reaper, Bitwig, etc.) -- not done;
   `auval` and a bare Standalone launch are strong but not equivalent signals.
3. `juce::AudioProcessorValueTreeState` parameter wiring (`plugin/src/parameters/`).
4. The real PLAY/DESIGN/LAB UI (Phase 17) -- `GenericAudioProcessorEditor` is a
   placeholder, not a step toward it.
5. Code signing / notarization for actual distribution (the build today produces an
   ad-hoc-signed VST3, sufficient for local testing only).
