# Build

## Requirements

- CMake >= 3.24
- A C++20 compiler (developed against AppleClang 17 / Xcode command line tools;
  any recent Clang, GCC, or MSVC with C++20 support should work, untested)
- Network access on first configure (CMake `FetchContent` pulls `nlohmann/json`,
  `Catch2`, and -- only if those options are enabled -- `pybind11`/Python dev
  headers or JUCE)

No dependency is vendored into the repo; everything is fetched by CMake and pinned
to a specific tag. See [THIRD_PARTY_LICENSES.md](../THIRD_PARTY_LICENSES.md) and
[third_party_dependencies.json](../third_party_dependencies.json).

## Quick start

```bash
cmake --preset dev
cmake --build --preset dev -j
ctest --preset dev --output-on-failure
```

This builds `pw8_core`, the CLI tools (`murmur-render`, `murmur-info`, `murmur-graph`,
`murmur-wavetable-builder`, `murmur-fuzz-render`), and the Catch2 test suite. Confirmed
working: 67 test cases / 811,735 assertions pass, all tools run and were smoke-tested against every
file in `content/`.

## Presets

| Preset | What it builds | Notes |
|---|---|---|
| `dev` | core + tools + tests, Debug | default day-to-day preset |
| `release` | core + tools + tests, Release | |
| `asan` | core + tests, Debug, AddressSanitizer | `cmake --build --preset asan && ctest --preset asan` |
| `ubsan` | core + tests, Debug, UndefinedBehaviorSanitizer | |
| `benchmarks` | core + Google Benchmark suite | build-verified; run `./build/benchmarks/benchmarks/pw8_benchmarks` |
| `python` | core + `murmur` pybind11 module | requires Python dev headers; output at `build/python/python/murmur.cpython-*.so`; build-verified + smoke-tested |
| `plugin` | core + `pw8_plugin` (JUCE) | build-verified: VST3/AU/Standalone all build against JUCE 8.0.6, AU passes `auval` -- see `docs/PLUGIN_ARCHITECTURE.md` for what's still missing (real UI, host-matrix testing) |

Manual (non-preset) configure equivalent:

```bash
cmake -S . -B build -DPW8_BUILD_TESTS=ON -DPW8_BUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## CMake options

| Option | Default | Purpose |
|---|---|---|
| `PW8_BUILD_TESTS` | `ON` | Catch2 suite |
| `PW8_BUILD_TOOLS` | `ON` | CLI tools |
| `PW8_BUILD_PYTHON_BINDINGS` | `OFF` | pybind11 module |
| `PW8_BUILD_BENCHMARKS` | `OFF` | Google Benchmark suite (not yet implemented) |
| `PW8_BUILD_PLUGIN` | `OFF` | JUCE plugin (scaffold, untested) |
| `PW8_ENABLE_ASAN` | `OFF` | AddressSanitizer |
| `PW8_ENABLE_UBSAN` | `OFF` | UndefinedBehaviorSanitizer |
| `PW8_WARNINGS_AS_ERRORS` | `OFF` | Promote warnings to errors |

## Running the CLI tools

```bash
./build/dev/tools/murmur-info

./build/dev/tools/murmur-render \
    --patch content/presets/dark-bass.murmur \
    --midi content/test_midi/bass-line.mid \
    --sample-rate 48000 --bpm 105 \
    --output /tmp/dark-bass.wav --receipt /tmp/dark-bass.receipt.json

./build/dev/tools/murmur-graph inspect content/presets/fm-bell.murmur

./build/dev/tools/murmur-wavetable-builder --input source.wav --output content/wavetables/my_table.json \
    --frames 4 --samples-per-frame 2048 --mip-levels 10  # --samples-per-frame must be a power of two

./build/dev/tools/murmur-fuzz-render --count 10000 --seed 1
```

## Benchmarks

```bash
cmake --preset benchmarks
cmake --build --preset benchmarks -j
./build/benchmarks/benchmarks/pw8_benchmarks
```

Covers the Classic and Wavetable oscillators, three algorithm-graph topologies
(parallel/serial/feedback), a single-voice render loop, and full-patch rendering at
44.1/48/96 kHz x 1/8/16/32 voices -- matching the master spec's benchmark matrix.

## Plugin (JUCE)

```bash
cmake --preset plugin
cmake --build --preset plugin -j
```

Produces VST3, AU, and Standalone builds under
`build/plugin/plugin/pw8_plugin_artefacts/Debug/`. On macOS, validate the AU with
Apple's own tool:

```bash
cp -R "build/plugin/plugin/pw8_plugin_artefacts/Debug/AU/MURMUR.component" \
    ~/Library/Audio/Plug-Ins/Components/
killall -9 AudioComponentRegistrar   # forces auval to see the freshly-copied component
auval -v aumu Pwe8 Pwei
```

See [docs/PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md) for what's verified
(build + full `auval` pass) versus still missing (real UI, host-matrix testing).

## Python bindings

```bash
cmake --preset python
cmake --build --preset python -j
python3 -c "
import sys; sys.path.insert(0, 'build/python/python')
import murmur as pw8
patch = pw8.Patch.load('content/presets/init-saw.murmur')
print(pw8.render(patch, 'content/test_midi/single-note.mid')['metrics'])
"
```

A proper `pip install`-able package (wheel building, `pyproject.toml`) is PLANNED --
see [PYTHON_API.md](PYTHON_API.md).

## Sanitizers

```bash
cmake --preset asan && cmake --build --preset asan -j && ctest --preset asan --output-on-failure
cmake --preset ubsan && cmake --build --preset ubsan -j && ctest --preset ubsan --output-on-failure
```

ThreadSanitizer is intentionally not wired as a blanket CMake option -- per the
master spec it's meant for targeted control-path test runs, not a general flag
across the realtime audio path.
