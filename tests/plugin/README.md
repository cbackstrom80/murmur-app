# tests/plugin/

No automated `ctest`-registered plugin tests yet. What exists today is a manual
verification pass (see `docs/PLUGIN_ARCHITECTURE.md`): `plugin/` builds against real
JUCE 8.0.6 and the AU target passes Apple's `auval` validation tool in full
(format tests, render tests across 5 sample rates and multiple block sizes, a MIDI
test). The Standalone app was also confirmed to launch and run without crashing or
triggering JUCE assertions.

PLANNED (docs/ROADMAP.md Phase 20): `pluginval`, a real DAW host-matrix, and wiring
the `auval`/build check into a scripted, repeatable form rather than the manual
verification described above -- see `.github/workflows/ci.yml`'s `plugin` job for
the current (build + auval, macOS only) automation.
