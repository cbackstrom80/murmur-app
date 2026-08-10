# Plugin Architecture

**STATUS: SCAFFOLD / PARTIAL.** `plugin/` is real, structurally-considered code
(`plugin/CMakeLists.txt`, `plugin/src/processor/PatchworkEightProcessor.{h,cpp}`,
`plugin/src/state/PluginState.h/.cpp`) but has **not been compiled against an actual
JUCE checkout in this pass** -- `PW8_BUILD_PLUGIN` defaults `OFF`, and CI does not
build it. Per the master spec's phasing, the plugin is Phase 16, well after the
Phase 0/1(+2/3) target of this pass; what exists now is written to prove the
architecture reads correctly against the JUCE `AudioProcessor` API, not to claim a
working plugin.

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
drift apart.

### Automation

Per the master spec, not every internal patch parameter is exposed as flat DAW
automation. `plugin/src/state/PluginState.h` documents the intended parameter ID
scheme (`macro1`..`macro8`, matching `Patch::macros[0..7]`) for the eventual
`juce::AudioProcessorValueTreeState` wiring -- **not yet implemented**
(`plugin/src/parameters/` is an empty, documented placeholder).

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

`plugin/src/ui/` is an empty, documented placeholder
(`PatchworkEightProcessor::createEditor()` currently returns `nullptr`).

## What to build first, once JUCE is actually wired up

1. Confirm `plugin/CMakeLists.txt` actually configures/builds against real JUCE
   (FetchContent'd at `7.0.12` in this scaffold -- pin/verify against whatever JUCE
   version is current when this phase starts).
2. `pluginval` in CI once the target builds (docs/TESTING.md).
3. A `GenericAudioProcessorEditor` as the pragmatic placeholder editor before the
   real PLAY/DESIGN/LAB UI (Phase 17).
