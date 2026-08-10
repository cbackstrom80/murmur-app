# Patchwork Eight

An AI-native, dual-layer, 8-engine algorithmic software synthesizer. This is a new,
standalone repository -- it does not depend on, embed, or modify the existing
Patchwork AI repository (see [docs/PATCHWORK_INTEGRATION.md](docs/PATCHWORK_INTEGRATION.md)
for how they're meant to connect).

`patchwork-eight` / **PATCHWORK EIGHT** are working/codename names, chosen to be
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
| LFO (6 waveforms, free/retrigger/one-shot/tempo-sync, per-voice) | **IMPLEMENTED** (1 of the eventual 8 per patch) |
| Mod matrix (LFO/envelope/velocity/pressure/aftertouch/slide/8 macros -> filter cutoff/resonance, operator level, pan) | **IMPLEMENTED** (VOICE scope; LAYER/GLOBAL scope planned) |
| Arpeggiator (7 modes, tempo-sync, per-step gate/probability/ratchet/tie/accent, latch, polymetric) | **IMPLEMENTED** |
| FX bank (3 layer insert + 4 master slots; Saturation/Chorus/TapeDelay/NodeDelay/FreqShiftEcho/FractalEcho) | **IMPLEMENTED** (reverb/EQ/compressor still PLANNED, see FX_BANK.md) |
| `.pw8` patch format (JSON, versioned schema, untrusted-input-hardened) | **IMPLEMENTED** |
| Native offline renderer (no plugin host / DAW required) -> WAV + JSON receipt | **IMPLEMENTED** |
| Standard MIDI File input (hand-rolled reader, tempo map, running status) | **IMPLEMENTED** |
| Python bindings (pybind11) | **IMPLEMENTED** (partial API surface, see docs/PYTHON_API.md) |
| Deterministic/seeded randomness throughout | **IMPLEMENTED** |
| JUCE VST3/AU/Standalone plugin | **PARTIAL, build-verified** (AU passes Apple's `auval` in full; off by default) |
| Google Benchmark suite | **IMPLEMENTED** (oscillators, algorithm graph, voice, full-patch render, at 44.1/48/96 kHz x 1/8/16/32 voices) |
| Fuzz-render harness (`pw8-fuzz-render`) | **IMPLEMENTED** (verified across 3 batches, 13,000 random valid patches total, 0 failures) |
| Filter 2 (character), reverb/EQ/compressor, MSEG, dual-layer mixing, algorithm morph, additional engine types (additive/phase-shape/granular/noise/resonator) | **PLANNED** |

## Architecture

```
                      pw8_core
                          |
         +----------------+----------------+
         |                |                |
         v                v                v
     pw8_plugin      patchwork_eight    pw8-render / pw8-info / pw8-graph / ...
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

96 test cases, 895,837 assertions, all passing. See [docs/BUILD.md](docs/BUILD.md)
for every preset (release/asan/ubsan/benchmarks/python/plugin).

## Running the renderer

```bash
./build/dev/tools/pw8-render \
    --patch content/presets/dark-bass.pw8 \
    --midi content/test_midi/bass-line.mid \
    --sample-rate 48000 --bpm 105 \
    --output /tmp/dark-bass.wav --receipt /tmp/dark-bass.receipt.json

./build/dev/tools/pw8-graph inspect content/presets/fm-bell.pw8
./build/dev/tools/pw8-info
```

12 engineering test patches ship in `content/presets/`: `INIT SINE`, `INIT SAW`,
`WIDE SAW`, `SUB BASS`, `FM BELL`, `DARK BASS`, `SOFT PAD`, `WT MORPH`,
`ARP PLUCK`, `FX NODE TREE`, `FX FRACTAL MORPH`, `FX FREQ ECHO`. These are
engineering patches proving specific capabilities, not curated factory content
(see docs/ROADMAP.md Phase 19).

## Documentation

- [ARCHITECTURE.md](docs/ARCHITECTURE.md) -- layering, threading model, realtime rules
- [BUILD.md](docs/BUILD.md) -- presets, options, dependency fetching
- [DSP_ENGINE.md](docs/DSP_ENGINE.md) -- every engine type's design rationale and status
- [ALGORITHM_GRAPH.md](docs/ALGORITHM_GRAPH.md) -- the 8-node graph, compiler, execution semantics
- [PATCH_FORMAT.md](docs/PATCH_FORMAT.md) -- the `.pw8` schema
- [MODULATION.md](docs/MODULATION.md) -- envelope (done), LFO/matrix/macros (planned)
- [ARPEGGIATOR.md](docs/ARPEGGIATOR.md) -- modes, per-step modifiers, polymetric design
- [FX_BANK.md](docs/FX_BANK.md) -- researched-plugin analysis, the 6 algorithms, the invented FractalEcho
- [RENDERER.md](docs/RENDERER.md) -- native offline rendering, MIDI input, WAV output
- [PYTHON_API.md](docs/PYTHON_API.md) -- pybind11 bindings
- [PLUGIN_ARCHITECTURE.md](docs/PLUGIN_ARCHITECTURE.md) -- JUCE scaffold design
- [TESTING.md](docs/TESTING.md) -- what's covered, what isn't yet
- [ROADMAP.md](docs/ROADMAP.md) -- phase-by-phase status against the full product spec
- [LICENSING.md](docs/LICENSING.md) -- dependency license analysis
- [PATCHWORK_INTEGRATION.md](docs/PATCHWORK_INTEGRATION.md) -- how this connects to Patchwork AI
- [PRIOR_ART.md](docs/PRIOR_ART.md) -- design lineage, incl. the Mutable Instruments `eurorack` concept mapping
- [COMPETITIVE_ANALYSIS.md](docs/COMPETITIVE_ANALYSIS.md) -- feature-parity check against Serum 2, Phase Plant, and Zebra 3 (research only, no UI/code copied)
- [GPU_ACCELERATION_RESEARCH.md](docs/GPU_ACCELERATION_RESEARCH.md) -- CUDA/GPU Audio research and why this repo stays CPU-only for now (decision record, no GPU code)

## License

No license has been chosen yet for this project's own code (see
[docs/LICENSING.md](docs/LICENSING.md)); third-party dependencies retain their own
licenses ([THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)).
