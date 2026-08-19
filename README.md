# MURMUR

An AI-native, dual-layer, 8-engine algorithmic software synthesizer. This is a new,
standalone repository -- it does not depend on, embed, or modify the existing
Patchwork AI repository (see [docs/PATCHWORK_INTEGRATION.md](docs/PATCHWORK_INTEGRATION.md)
for how they're meant to connect).

`murmur-app` / **PATCHWORK EIGHT** are working/codename names, chosen to be
easy to rename later.

## What this is, right now

This is an early engineering pass: a real, tested, framework-independent C++20 DSP
core plus enough surrounding tooling to prove the architecture end-to-end --
**not** a finished synth. Every capability below is explicitly labeled
**IMPLEMENTED**, **PARTIAL**, or **PLANNED**; see [docs/ROADMAP.md](docs/ROADMAP.md)
for the full phase-by-phase breakdown against the product spec.

| Capability | Status |
|---|---|
| Framework-independent `pw8_core` DSP library (no JUCE dependency) | **IMPLEMENTED** |
| Band-limited (PolyBLEP) Classic oscillator: sine/triangle/saw/square, continuous morph | **IMPLEMENTED** |
| Wavetable oscillator (FFT-based mip-mapping, measured >2x aliasing reduction) | **IMPLEMENTED** |
| DAHDSR envelope | **IMPLEMENTED** |
| Polyphonic voice allocation (configurable, default 16 / max 32 voices), sensible stealing policy | **IMPLEMENTED** |
| 8-node-per-layer algorithm graph: AUDIO/PHASE_MOD/FREQUENCY_MOD/AMPLITUDE_MOD/RING_MOD/SYNC/FEEDBACK edges, validated + compiled + executed | **IMPLEMENTED** |
| MPE-shaped per-note expression capture (pitch bend, pressure, aftertouch, slide) | **IMPLEMENTED** (pitch bend affects pitch directly; pressure/aftertouch/slide are mod matrix sources) |
| Filter 1 (TPT state-variable: LP/HP/BP/notch/peak, per-voice, key-tracked) | **IMPLEMENTED** |
| LFO (6 waveforms, free/retrigger/one-shot/tempo-sync, per-voice + shared per-layer bank) | **IMPLEMENTED** (8 per patch) |
| Modulation: 8 envelopes + 8 LFOs per layer, mod matrix (29 sources incl. all LFOs/envelopes/8 macros -> filter cutoff/resonance, operator level, pan) | **IMPLEMENTED** (VOICE scope for all sources; LAYER/GLOBAL scope for LFOs) |
| Arpeggiator (7 modes, tempo-sync, per-step gate/probability/ratchet/tie/accent, latch, polymetric) | **IMPLEMENTED** |
| FX bank (3 layer insert + 4 master slots; Saturation/Chorus/TapeDelay/NodeDelay/FreqShiftEcho/FractalEcho/Reverb/Eq/Compressor/Limiter -- 10 algorithms; Reverb is an 8-line multiband FDN with input diffusion, early/late split, and late-tank modulation, researched from Jot/Dattorro/CloudSeed/Bricasti-informed principles) | **IMPLEMENTED** |
| `.pw8` patch format (JSON, versioned schema, untrusted-input-hardened) | **IMPLEMENTED** |
| Native offline renderer (no plugin host / DAW required) -> WAV + JSON receipt | **IMPLEMENTED** |
| Standard MIDI File input (hand-rolled reader, tempo map, running status) | **IMPLEMENTED** |
| Python bindings (pybind11) | **IMPLEMENTED** (partial API surface, see docs/PYTHON_API.md) |
| Deterministic/seeded randomness throughout | **IMPLEMENTED** |
| All 8 operator engines (Classic, Wavetable, FM/PM, Additive, PhaseShape, Granular, NoiseChaos, Resonator) | **IMPLEMENTED** |
| JUCE VST3/AU/Standalone plugin, 762 parameters live-automatable via `AudioProcessorValueTreeState`, real PLAY-mode UI (the OBSIDIAN skin) | **PARTIAL, build-verified** (AU passes `auval`, both AU and VST3 pass `pluginval` at max strictness against the real custom editor; DESIGN/LAB modes and other skins PLANNED, see docs/UI.md; off by default) |
| MCP server (18 tools: patch introspection/construction/editing/rendering for Claude Desktop/Code and similar clients) | **PROTOTYPE** (`mcp_server/`, see its README; separate from the plugin, not built by CMake) |
| Google Benchmark suite | **IMPLEMENTED** (oscillators, algorithm graph, voice, full-patch render, at 44.1/48/96 kHz x 1/8/16/32 voices) |
| Fuzz-render harness (`murmur-fuzz-render`) | **IMPLEMENTED** (verified across 5 batches, 15,000 random valid patches total, 0 failures) |
| Filter 2 (character), bitcrush/wavefold/ensemble/flanger/phaser/diffusion delay, MSEG, dual-layer mixing, algorithm morph, unison DSP | **PLANNED** |

## Architecture

```
                      pw8_core
                          |
         +----------------+----------------+
         |                |                |
         v                v                v
     pw8_plugin         murmur         murmur-render / murmur-info / murmur-graph / ...
  (JUCE, build-       (pybind11)        (native CLI tools)
    verified)
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full threading model,
realtime-safety rules, and buffer design.

## Building

```bash
cmake --preset dev
cmake --build --preset dev -j
ctest --preset dev --output-on-failure
```

166 test cases, 9,802,196 assertions, all passing. See [docs/BUILD.md](docs/BUILD.md)
for every preset (release/asan/ubsan/benchmarks/python/plugin/plugin-release).

**MVP gate:** `scripts/mvp_check.sh` — build, test, content validation, MCP smoke,
render + fuzz sample. Scope: [docs/MVP.md](docs/MVP.md).

## Running the renderer

```bash
./build/dev/tools/murmur-render \
    --patch content/presets/dark-bass.murmur \
    --midi content/test_midi/bass-line.mid \
    --sample-rate 48000 --bpm 105 \
    --output /tmp/dark-bass.wav --receipt /tmp/dark-bass.receipt.json

./build/dev/tools/murmur-graph inspect content/presets/fm-bell.murmur
./build/dev/tools/murmur-info
```

`content/presets/` has three tiers: ~28 root-level engineering/showcase
patches proving specific capabilities (`INIT SINE`, `FM BELL`, `WT MORPH`,
`FX MASTER CHAIN`, the ambient engine-showcase tracks, etc.), plus
`content/presets/factory/` -- a 250-patch factory bank (50 each of Basses/
Leads/Pads/Sequences/Ambient, procedurally generated by
`scripts/generate_factory_presets.py`, every one validated with a real
single-note render before being bundled). Still not curated/hand-tuned
factory content in the traditional sense (see docs/ROADMAP.md Phase 19).

## Installing the plugin/app

**There's no public release channel yet** -- no website, no notarized
signed build, no App Store listing. What exists is a local installer you
build yourself:

```bash
./scripts/package_macos.sh          # builds Release VST3/AU/Standalone + packages them
open dist/Murmur-*-macOS.pkg                                    # GUI, asks for your admin password
# or:
sudo installer -pkg dist/Murmur-*-macOS.pkg -target /           # CLI
```

Installs to the same system-wide locations every commercial plugin
installer uses -- `/Library/Audio/Plug-Ins/VST3`, `/Library/Audio/Plug-Ins/
Components`, `/Applications` -- plus the 250-patch factory bank under
`/Library/Application Support/MURMUR/Presets/factory` (there's no
in-app preset browser yet, just PLAY mode's "Load..." file picker -- browse
there). **Ad-hoc code-signed, not notarized** (no Apple Developer ID in
this environment) -- fine for running a build you just made yourself, but
Gatekeeper will refuse it with a plain right-click-and-download if it's
ever moved to another machine. See `scripts/package_macos.sh`'s own header
comment and [docs/LICENSING.md](docs/LICENSING.md) -- this repo's code has
no license granted yet and JUCE needs a paid commercial license before any
real (non-local) distribution, so treat this installer as for-your-own-
machine only.

## Documentation

- [ARCHITECTURE.md](docs/ARCHITECTURE.md) -- layering, threading model, realtime rules
- [BUILD.md](docs/BUILD.md) -- presets, options, dependency fetching
- [DSP_ENGINE.md](docs/DSP_ENGINE.md) -- every engine type's design rationale and status
- [ALGORITHM_GRAPH.md](docs/ALGORITHM_GRAPH.md) -- the 8-node graph, compiler, execution semantics
- [PATCH_FORMAT.md](docs/PATCH_FORMAT.md) -- the `.pw8` schema
- [MODULATION.md](docs/MODULATION.md) -- 8 envelopes, 8 LFOs (VOICE+LAYER/GLOBAL scope), 29-source mod matrix, macros
- [ARPEGGIATOR.md](docs/ARPEGGIATOR.md) -- modes, per-step modifiers, polymetric design
- [FX_BANK.md](docs/FX_BANK.md) -- researched-plugin analysis, the 10 algorithms, the invented FractalEcho
- [RENDERER.md](docs/RENDERER.md) -- native offline rendering, MIDI input, WAV output
- [PYTHON_API.md](docs/PYTHON_API.md) -- pybind11 bindings
- [PLUGIN_ARCHITECTURE.md](docs/PLUGIN_ARCHITECTURE.md) -- JUCE scaffold design
- [UI.md](docs/UI.md) -- PLAY mode, the OBSIDIAN skin, the algorithm graph view
- [TESTING.md](docs/TESTING.md) -- what's covered, what isn't yet
- [ROADMAP.md](docs/ROADMAP.md) -- phase-by-phase status against the full product spec
- [LICENSING.md](docs/LICENSING.md) -- dependency license analysis
- [PATCHWORK_INTEGRATION.md](docs/PATCHWORK_INTEGRATION.md) -- how this connects to Patchwork AI
- [PRIOR_ART.md](docs/PRIOR_ART.md) -- design lineage, incl. the Mutable Instruments `eurorack` concept mapping
- [COMPETITIVE_ANALYSIS.md](docs/COMPETITIVE_ANALYSIS.md) -- feature-parity check against Serum 2, Phase Plant, and Zebra 3 (research only, no UI/code copied)
- [GPU_ACCELERATION_RESEARCH.md](docs/GPU_ACCELERATION_RESEARCH.md) -- CUDA/GPU Audio research and why this repo stays CPU-only for now (decision record, no GPU code)
- [MCP_AND_NL_PATCH_GENERATION.md](docs/MCP_AND_NL_PATCH_GENERATION.md) -- the MCP server (built, see `mcp_server/README.md`) and the still-idea-stage natural-language "make me a laser sound" chat box

## License

No license has been chosen yet for this project's own code (see
[docs/LICENSING.md](docs/LICENSING.md)); third-party dependencies retain their own
licenses ([THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)).
