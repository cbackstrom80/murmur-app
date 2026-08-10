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
- **Version:** v3.6.0
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
  (`PW8_BUILD_PLUGIN`, off by default, untested scaffold in this pass).
- **License:** GPLv3 **or** JUCE commercial license (dual-licensed)
- **Version:** 7.0.12 (pinned in `plugin/CMakeLists.txt`)
- **Source:** https://github.com/juce-framework/JUCE
- **Commercial closed-source compatible:** **Only with a paid JUCE commercial
  license.** Building/shipping `pw8_plugin` closed-source under the free GPLv3 tier
  is not compliant. This is the one dependency in the graph that requires an
  explicit business decision before any closed-source plugin ships -- see
  `docs/LICENSING.md`.

## Google Benchmark

- **Purpose:** Performance benchmarking (`benchmarks/`), not yet implemented in this
  pass; declared here in advance since it's named explicitly in the master spec's
  technology stack.
- **License:** Apache License 2.0
- **Version:** not yet pinned (no `FetchContent_Declare` exists yet -- `benchmarks/`
  has no CMakeLists.txt in this pass)
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
