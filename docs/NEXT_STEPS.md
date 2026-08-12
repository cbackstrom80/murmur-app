# Next Steps

A prioritized plan from a full-repo review (engine, plugin, UI, tests, CI, docs),
following on from UI GATE 4 (`docs/UI.md`). Ordered by leverage/risk, not by
roadmap phase number -- the roadmap (`docs/ROADMAP.md`) is the source of truth for
scope; this doc is about sequencing what's already PARTIAL/PLANNED there, plus a
few things that fall outside any phase (CI, testing gaps).

## P0 -- Risk: nothing has touched a real DAW yet

Everything verified so far (`auval`, `pluginval` at strictness 5, Standalone
launch) is a strong *signal*, not proof the plugin behaves in an actual host.
This is the single biggest gap between "verified" and "shippable":

- **Manually load the built VST3/AU into Ableton, Logic, Reaper, and Bitwig.**
  Specifically check: parameter automation lanes read sane names/ranges (578
  params, easy to get an off-by-one or truncated name), state save/reload
  round-trips a saved project, and the editor doesn't misbehave on host
  resize/rescan. This is manual work no test suite substitutes for --
  `docs/PLUGIN_ARCHITECTURE.md` "What's still missing" #1 already flags it, but
  it's not scheduled anywhere. **Still outstanding** -- everything below this
  bullet was DONE by the same PR that introduced this doc.
- ~~Wire `pluginval` into CI~~ -- **DONE**, same PR: `.github/workflows/ci.yml`'s
  `plugin` job now runs `pluginval --strictness-level 5` on both VST3 and AU
  right after `auval`. (One follow-up bug this surfaced and also fixed in the
  same PR: the cask-installed `pluginval` has no CLI on `PATH` -- the step
  invokes `pluginval.app/Contents/MacOS/pluginval` directly.)
- ~~Add a Windows/Linux plugin CI job~~ -- **DONE** for Windows, same PR: the new
  `plugin-windows` job does a VST3/Standalone compile-only check (no
  AU/auval/pluginval -- AU is Apple-only). Linux remains undone.

## P1 -- Correctness gaps that undercut what's already built

- **Layer B / dual-layer (Phase 8)**: schema's complete, nothing renders. This
  is the largest "looks done, isn't" gap -- `LayerMode` exists and presets
  could reference it today with no audible effect, which is a worse experience
  than not having the field at all (silent failure vs. an honest "not
  supported" error). Either wire `SINGLE_A`+`STACK`+`LAYER_MORPH` rendering, or
  have `Engine::loadPatch()` reject/warn on a non-`SINGLE_A` `LayerMode` until
  it's real, matching the "no engine pretends to work when it doesn't" honesty
  the UI already applies elsewhere (dashed rings, disabled pills).
- **Unison (Phase 7)**: same shape of gap -- `UnisonSettings` exists in the
  schema with no DSP behind it. Same recommendation: implement, or reject
  patches that set voice count > 1 until it's real.
- ~~1 of 8 operator engines still renders silence~~ -- **DONE**. All 6
  originally-silent operator engines (FM/PM, NoiseChaos, PhaseShape,
  Additive, Resonator, Granular -- Phase 4/10) now have real DSP, each
  shipped as its own separate, independently-verified PR and merged into
  `main` one at a time (with real conflict resolution across the shared
  files each PR touched additively: `OperatorNode.hpp`, `PluginState.h/.cpp`,
  `PatchworkEightProcessor.cpp`, `OperatorEditorPanel.h/.cpp`). All 8 of the
  headline "8-engine algorithmic synthesizer" engines now actually exist on
  `main`.
- **A real patch browser** -- see `docs/PATCH_BROWSER.md`. Promoted here
  from P2 by explicit direction (UA's plugin browser named as the concrete
  UX reference: search, category filter, prev/next stepping, favorites).
  `PatchBrowserBar` can currently load one file via a native dialog and
  nothing else, against a library that's now 278 presets deep
  (`content/presets/` root + the 250-patch factory bank) -- the gap this
  closes is bigger now than when it was first flagged as PLANNED, not
  smaller. `docs/PATCH_BROWSER.md` phases it: `PresetIndex` first, then
  prev/next arrows on the existing bar (the smallest useful slice), then a
  real search/filter panel, then favorites.

## P2 -- Highest-leverage PLANNED items (per ROADMAP, ranked)

In order of audible/usable impact per unit of engineering effort:

1. **Filter 2** (Phase 6) -- Filter 1 already proved the pattern
   (`StateVariableFilter.hpp` + mod-matrix wiring); a second, differently
   -voiced filter is mostly repeating known work, not new design.
2. **DESIGN mode** (Phase 17) -- graph topology editing, full mod-matrix UI,
   per-algorithm FX detail. This is where PLAY mode's honest scoping
   (dashed rings, "3 sources x 2 destinations only") stops being a feature and
   starts being a ceiling. Worth sequencing after P1's engine gaps close,
   since DESIGN mode editing engines that render silence doesn't help anyone.
3. **Unison + Layer B DSP** (see P1) -- listed again here because once fixed
   they're also roadmap phase completions, not just correctness fixes.

## P3 -- Polish and hardening, lower urgency

- **Embedded cross-platform font** (`docs/UI.md`) -- only matters once
  Windows/Linux builds are real (see P0's CI item); premature before that.
- **GPU-accelerated visuals / spectrum/scope** (Phase 17, `docs/
  GPU_ACCELERATION_RESEARCH.md`) -- explicitly deferred correctly; nothing to
  accelerate until there's a live audio-thread tap, which doesn't exist yet.
- **Soak testing / perf optimization** (Phase 20) -- `pw8-fuzz-render` covers
  correctness breadth (5,000 patches, 0 failures) but not long-duration
  stability (memory growth, denormal handling under hours of playback). Worth
  a dedicated soak-test tool once P0's DAW testing surfaces whether this is
  actually a problem in practice -- don't build it speculatively first.
- **Code signing/notarization** -- only needed at actual distribution time,
  correctly deferred.

## Ideas -- captured, not prioritized, not scoped into P0-P3 above

- **MCP server + natural-language patch generation** -- see
  `docs/MCP_AND_NL_PATCH_GENERATION.md`. The MCP server half (patch read/
  edit/render tools for Claude Desktop/Code and similar clients) is now a
  working prototype, `mcp_server/` -- but real usage, not just this repo's
  own smoke tests, hasn't happened yet, and the much larger, separately-
  decided idea for an in-app "make me a laser sound" chat box
  (bring-your-own-model or a hosted subscription service) hasn't been
  started at all. Deliberately listed here rather than in P0-P3: unlike
  everything above, neither half has had a real prioritization pass yet.

## Not on this list on purpose

`wavetableId` resource resolution, Layer B's remaining non-DSP fields, the
patch-migration mechanism, and the other items in `docs/
PLUGIN_ARCHITECTURE.md`'s "What's still missing" #3 are each individually
low-impact and already correctly triaged as deliberate exclusions, not gaps --
no change recommended until a concrete use case needs one of them.
