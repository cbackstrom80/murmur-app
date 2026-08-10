# Third-Party Licenses

Human-readable companion to [third_party_dependencies.json](third_party_dependencies.json)
(machine-readable). See [docs/LICENSING.md](docs/LICENSING.md) for the
commercial-closed-source compatibility analysis.

None of these are vendored into the repository -- all are fetched by CMake
`FetchContent` at a pinned tag/version, at configure time, from their upstream
repositories.

---

## nlohmann/json

- **Purpose:** JSON parsing/serialization for the `.pw8` patch format, isolated to
  `pw8/patch/PatchSerializer.cpp` (never included from a header on the audio-thread
  include path).
- **License:** MIT
- **Version:** v3.11.3
- **Source:** https://github.com/nlohmann/json
- **Commercial closed-source compatible:** Yes, no action required beyond this notice.

## Catch2

- **Purpose:** Test framework (`tests/`). Never linked into any shipped binary.
- **License:** Boost Software License 1.0
- **Version:** v3.8.1 (bumped from v3.6.0 after hitting a test-discovery/CMake-version
  interaction bug in the older release -- see `docs/TESTING.md`)
- **Source:** https://github.com/catchorg/Catch2
- **Commercial closed-source compatible:** Yes.

## pybind11

- **Purpose:** C++ <-> Python bindings (`bindings/python/`), optional build
  (`PW8_BUILD_PYTHON_BINDINGS`).
- **License:** BSD-3-Clause
- **Version:** v2.13.6
- **Source:** https://github.com/pybind/pybind11
- **Commercial closed-source compatible:** Yes.

## JUCE

- **Purpose:** VST3/AU/Standalone plugin wrapper (`plugin/`), optional build
  (`PW8_BUILD_PLUGIN`, off by default). Build-verified: all three formats compile,
  and the AU target passes Apple's `auval` validation tool in full.
- **License:** GPLv3 **or** JUCE commercial license (dual-licensed)
- **Version:** 8.0.6 (pinned in `plugin/CMakeLists.txt`; 7.0.12 was tried first and
  failed to compile against the current macOS SDK -- `CGWindowListCreateImage` was
  obsoleted in macOS 15)
- **Source:** https://github.com/juce-framework/JUCE
- **Commercial closed-source compatible:** **Only with a paid JUCE commercial
  license.** Building/shipping `pw8_plugin` closed-source under the free GPLv3 tier
  is not compliant. This is the one dependency in the graph that requires an
  explicit business decision before any closed-source plugin ships -- see
  `docs/LICENSING.md`.

## Google Benchmark

- **Purpose:** Performance benchmarking (`benchmarks/`). Build-verified: oscillator,
  algorithm graph, voice, and full-patch-render benchmarks all run and produce real
  numbers (see `docs/TESTING.md`).
- **License:** Apache License 2.0
- **Version:** v1.9.1
- **Source:** https://github.com/google/benchmark
- **Commercial closed-source compatible:** Yes.

---

## Conceptual (non-code) references

## Mutable Instruments `eurorack`

- **What was referenced:** architectural concepts only (macro-oscillator-per-node
  model, exciter/resonator role separation, cross-modulation-as-first-class-algorithm,
  fixed-capacity-grain-pool granular design, one-pole-smoothing/PolyBLEP DSP
  conventions). **No source code was copied or vendored.** See `docs/PRIOR_ART.md`
  for the full per-module mapping.
- **License of the referenced (MIT-only) portion:** MIT (STM32F-target projects,
  which includes Braids/Plaits/Elements/Rings/Clouds/Warps/Tides and the shared
  `stmlib` utility library). The AVR-target projects in that repository are GPLv3
  and were explicitly excluded from consideration.
- **Source:** https://github.com/pichenettes/eurorack
- **Commercial closed-source compatible:** N/A -- not a code dependency, no code
  copied. Listed here for attribution transparency per the master spec's dependency
  disclosure requirement, even though it isn't a build dependency.

## VCV Rack `Befaco`

- **What was referenced:** module *concept names only* (Rampage, Morphader, Chopping
  Kinky, Muxlicer, Spring Reverb) as corroboration for already-planned features
  (layer/algorithm morph, wavefolding, step sequencing, reverb character variety).
  **No source code, DSP implementation detail, or circuit topology was read or
  copied.** See `docs/PRIOR_ART.md`.
- **License:** GPLv3 (entire repository -- unlike the Mutable reference above, there
  is no permissively-licensed subset)
- **Source:** https://github.com/VCVRack/Befaco
- **Commercial closed-source compatible:** N/A -- not a code dependency, no code
  copied, treated more strictly than the Mutable reference specifically because the
  whole repo is GPLv3 with no MIT-licensed portion to point to.
