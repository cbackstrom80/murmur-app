# Architecture

## Layering

```
                      pw8_core
                          |
         +----------------+----------------+
         |                |                |
         v                v                v
     pw8_plugin       patchwork_eight   pw8-render / pw8-info / pw8-graph / ...
     (JUCE VST3/AU/     (pybind11        (native CLI tools, no host required)
      Standalone)        Python module)
```

**`pw8_core`** (`engine/`) is a framework-independent, modern C++20 static library.
It has exactly one third-party dependency in its public surface area avoidance
policy: **it must never depend on JUCE.** `pw8_core` links `nlohmann::json`
privately, only inside `pw8/patch/PatchSerializer.hpp`'s implementation -- no header
that could end up on the audio thread's include path pulls in a JSON parser. See
`docs/DSP_ENGINE.md` and [PRIOR_ART.md](PRIOR_ART.md).

Everything else is a consumer of `pw8_core`:

- **`pw8_plugin`** (`plugin/`) -- JUCE VST3/AU/Standalone wrapper. STATUS: **PARTIAL,
  build-verified** (AU passes Apple's `auval` in full; off by default,
  `PW8_BUILD_PLUGIN=OFF`; built by a non-blocking macOS CI job).
- **`patchwork_eight`** (`bindings/python/`) -- pybind11 module. STATUS:
  **IMPLEMENTED (PARTIAL API surface)**, off by default (`PW8_BUILD_PYTHON_BINDINGS=OFF`
  since it needs Python dev headers), builds and is smoke-tested.
- **CLI tools** (`tools/`) -- `pw8-render`, `pw8-info`, `pw8-graph`,
  `pw8-wavetable-builder`. STATUS: **IMPLEMENTED**, on by default, no host required.

Future targets (CLAP, Linux, embedded/ARM, render-farm workers) only need new thin
consumers of `pw8_core` -- the core itself has no platform or host assumptions baked in.

## Threading Model

Three thread roles, by contract rather than by any framework-provided guarantee:

1. **UI / message thread** -- loads presets, edits patches, triggers algorithm graph
   compilation, runs AI (in Patchwork, not here), talks to the filesystem/network.
2. **Background worker** -- prepares wavetable data, precomputes anything expensive
   before it's needed by the audio thread. Not yet a distinct thread pool in this
   pass (the native renderer and plugin scaffold both do "prepare" work inline on
   the message/control thread, which is safe because they're not concurrent with
   `process()` while doing so) -- a real background pool is PLANNED alongside the
   plugin (Phase 16) and factory-content pipeline (Phase 19).
3. **Audio thread** -- only ever touches immutable/precompiled engine state:
   `pw8::algorithm::CompiledAlgorithm`, `pw8::op::OperatorParams`,
   `pw8::envelope::DahdsrParams`. It never parses JSON, never compiles a graph,
   never allocates (see "Realtime Safety" below).

**State exchange pattern:** prepare new state off the audio thread, then publish it
with a single atomic pointer swap; the audio thread reads that pointer once per
block. This is implemented today in the plugin scaffold
(`plugin/src/processor/PatchworkEightProcessor.cpp`, `publishEngine()` /
`activeEngine_`) using a double-buffered `std::unique_ptr<Engine>` pair and
`std::atomic<Engine*>` -- deliberately not a hand-rolled lock-free queue. The native
offline renderer doesn't need this pattern at all: it is single-threaded by
construction (one thread drives `loadPatch()` and `process()` sequentially), so
`pw8::render::Engine` there is just used directly.

## Realtime Safety Rules

`pw8::render::Engine::process()` and everything it calls (`Voice::renderSample()`,
`AlgorithmExecutor::processSample()`, `OperatorState::render()`) must never:

- allocate on the heap
- touch the filesystem or network
- parse JSON
- lock a mutex or wait on a condition variable
- run Python or AI inference
- log in a way that could allocate or block
- parse a preset or compile an algorithm graph

All of `pw8_core`'s realtime-path containers are fixed-capacity
(`core::FixedVector<T, N>`, `std::array<T, core::kNodesPerLayer>`,
`voice::VoicePool` = `std::array<Voice, core::kMaxVoices>`) precisely so the audio
thread never needs to allocate. `AlgorithmGraphCompiler::compile()` is the one place
a graph gets validated/compiled, and it is never called from `process()`.

## DSP Buffers

`pw8_core` defines its own non-owning buffer views in `pw8/core/AudioBlock.hpp`
(`MonoBlockView`, `StereoBlockView`) rather than depending on `juce::AudioBuffer`.
The plugin wraps a `juce::AudioBuffer<float>`'s raw pointers in a `StereoBlockView`
right at the `processBlock()` boundary and nowhere else touches JUCE buffer types
inside DSP code.

## Sample Rate / Block Size

The engine is tested at 44.1 kHz, 48 kHz, and (via `Renderer` options) any rate in
`[8000, 384000]` Hz -- 88.2 kHz and 96 kHz work the same way, just untested by name
in the current Catch2 suite. Nothing in `pw8_core` assumes a fixed block size:
`Engine::process()` takes a `StereoBlockView` of whatever length the caller passes,
and the native renderer internally chunks into `RenderOptions::blockSize` (default
512) purely as an implementation detail of the offline render loop, not a DSP
requirement.

## See also

- [DSP_ENGINE.md](DSP_ENGINE.md) -- oscillators, envelopes, voices, filters (planned), FX (planned)
- [ALGORITHM_GRAPH.md](ALGORITHM_GRAPH.md) -- the 8-node graph model and compiler
- [PATCH_FORMAT.md](PATCH_FORMAT.md) -- the `.pw8` schema
- [PRIOR_ART.md](PRIOR_ART.md) -- design lineage / what's original vs. inspired-by-and-reimplemented
- [ROADMAP.md](ROADMAP.md) -- phase-by-phase status
