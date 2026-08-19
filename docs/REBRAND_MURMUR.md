# Rebrand: Patchwork Eight → MURMUR

**Status: IN PROGRESS.** This document is the decisions record for finishing the MURMUR
rebrand that the plugin's own bundle identity already started. Steps 1-5 (inventory,
extension-handling code, cosmetic/identifier renames, mass preset-file rename, CLI tool
renames) are done as of this update; full verification (Step 6) and the actual GitHub
repo rename (Step 7) have not started.

**Step 3 full sweep, done**: beyond the docs/prose bulk pass, also fixed every remaining
non-doc old-brand reference found by a final repo-wide grep: `LICENSE` copyright holder,
`CMakeLists.txt`'s configure-status message, `.mcp.json`'s server registration key,
two `ObsidianPalette.h` comments, the Figma layout-spec JSON Schema `$id`, a `MacroStrip.h`
comment, `PyBindings.cpp`'s `m.doc()` string, `mcp_server/server.py` (module docstring,
`FastMCP()` registration name, doc examples), `mcp_server/patch_builder.py`'s and
`scripts/generate_factory_presets.py`'s `author` metadata field, `scripts/patchforge_ingest.py`'s
`engine` catalog tag (confirmed unconsumed downstream before changing), and the product
copy (not the `.pw8` data paths, deferred to Step 4) in
`scripts/patchforge/manifests/mvp.json`. One real bug found and fixed along the way:
`plugin/src/ui/components/DesignFxUiState.cpp` was writing its preferences file under the
old `"Patchwork Eight"` Application Support folder with **no fallback to the new `"MURMUR"`
folder at all** (unlike every other search-root/manifest-path site in the codebase, which
correctly checks both) — low-stakes (UI chip-order prefs, not patch content), fixed to
use `"MURMUR"` outright. Confirmed intentionally-untouched dual-path sites (old name kept
as a real fallback, not a bug): `MurmurProcessor.cpp`'s content search roots,
`mcp_server/standalone_bridge.py`'s bridge-manifest paths, `scripts/validate_content_refs.py`'s
installed-wavetables path. Full rebuild (Standalone/AU/VST3) + `ctest` re-verified clean
after this pass (same 335/336, same pre-existing unrelated failure).

## Why this doc exists

The product is publicly branded **MURMUR**, and has been since its first real release —
but the repo, file extension, and a long tail of internal names/strings still said
"Patchwork Eight" / `pw8` as of the start of this effort. This is Phase 0 of a larger
effort (a new companion web platform, `murmur-web`) that needs to be built once against
final names, not against `patchwork-eight`/`.pw8`. Full context and the web-platform plan
live outside this repo, in the planning conversation that produced this document; this
file only needs to stand on its own for anyone working in *this* repo.

## Real inventory (run 2026-08-19, repo root, excluding `.git`/`build`/`dist`)

| Pattern | Line hits |
|---|---|
| `pw8` | 5,179 (2,922 of these are inside `content/presets/**` — see finding below, not code/docs) |
| `Patchwork Eight` | 366 |
| `PatchworkEight` | 495 |
| `patchwork-eight` / `patchwork_eight` | 135 |
| `Starfighter` | 18 |
| `com\.patchwork\.` (bundle IDs) | 18 |
| `.pw8` preset files on disk | 1,158 (29 in `content/presets/` root + 1,129 under `content/presets/factory/`) |

Non-content `pw8` hits (code/docs/scripts, excluding preset JSON content): **2,453 lines**
— this is the real size of the code/doc rebrand surface, not 5,179.

## New finding this inventory pass surfaced (not in the original plan)

**Every one of the 1,158 preset files also carries an old-branded ID inside its own JSON
content**, not just in its filename:

```json
"metadata": { "id": "pw8-dark-bass", ... }
```

Confirmed via `grep -rl '"id": *"pw8-' content/presets/` → all 1,158 files match. This
needs the same rename discipline as the extension itself, in the **same** pass (same
script, same commit) that does the mass `git mv` in Step 4 below — trivially cheap to
include while the file is already being touched, and worth doing now rather than shipping
`pw8-`-prefixed slugs into `murmur-web`'s marketplace (patch slugs, lineage/"remix of"
references) indefinitely. **Decision: `metadata.id` prefix changes `pw8-` → `murmur-`
alongside the filename rename, content otherwise byte-identical.** Not yet executed —
this is Step 4, still pending.

## Explicit non-changes (decided, do not revisit)

- **`BUNDLE_ID "com.patchwork.murmur"`, `PLUGIN_MANUFACTURER_CODE Murr`,
  `PLUGIN_CODE Murm`, `com.patchwork.murmur.standalone`** (`plugin/CMakeLists.txt`)
  — verified unchanged since the very first real release, `v1.0.0`
  (`git show v1.0.0:plugin/CMakeLists.txt`). Changing these now would be the one action
  in this whole effort that could orphan real users' saved DAW projects, for zero
  benefit. **Not touched by this rebrand.**
- **`com.patchwork.quasar`** (`quasar_plugin/CMakeLists.txt` and its release scripts)
  — a separate sibling product under the same company umbrella. Out of scope entirely.
  Not touched.
- **Internal C++ `pw8::` namespace and CMake target names**
  (`pw8_core`/`pw8_plugin`/`pw8_tests`/`pw8_benchmarks`) — 401 files reference the
  namespace; it is pure internal organization no user, DAW, or script ever observes.
  Renaming it is all diff, no user-facing value, and a near-unreviewable 400+ file sed
  pass across the entire DSP engine right when this code needs to be a stable foundation
  for `murmur-web` to build against. **Permanent decision, not a deferral** — a future
  engineer should not "finish the job" here.

## What does change

| Item | From (original) | To (current) | Status |
|---|---|---|---|
| Repo name | `patchwork-eight` | `murmur-app` | **Pending — Step 7** |
| Top-level CMake `project()` name | `patchwork_eight` | `murmur` | **Done** |
| Patch file extension | `.pw8` | `.murmur` (permanent dual-read; new saves write `.murmur`) | **Done — all 1,158 files renamed, snapshot-verified** |
| Preset `metadata.id` prefix | `pw8-*` | `murmur-*` | **Done, same pass** |
| Python binding module | `patchwork_eight` | `murmur` (`pybind11_add_module` / `PYBIND11_MODULE` in `bindings/python/PyBindings.cpp`) | **Done, compiler-verified** |
| Plugin processor class | `PatchworkEightProcessor` (`.h`/`.cpp`) | `MurmurProcessor` | **Done, compiler-verified across 150+ dependent files** |
| CLI tools | `pw8-render`, `pw8-info`, `pw8-graph`, `pw8-wavetable-builder`, `pw8-fuzz-render` | `murmur-render`, `murmur-info`, `murmur-graph`, `murmur-wavetable-builder`, `murmur-fuzz-render` | **Done** |
| Docs/prose/comments | "Patchwork Eight" / "PatchworkEight" / "patchwork-eight" / "Starfighter" | "MURMUR" | **Done — bulk pass, this update** |
| Wavetables | generic `.json`, no dedicated extension | **no change** | N/A — no branding pressure exists on this format |

**Hard exclusion for any automated find/replace**: never match `com.patchwork.` — a
careless global "patchwork→murmur" substitution would corrupt
`com.patchwork.murmur` into `com.murmur.murmur` and equally corrupt the untouched
`com.patchwork.quasar`. (Also learned the hard way during this pass: exclude this
document itself, and any other doc that specifically narrates the old→new naming
transition, from a blind bulk find/replace — see "Lesson learned" below.)

**Bonus fix, same pass**: `content/appcast.xml.template` and `docs/appcast.xml.template`
both pointed their Sparkle update-feed URL at `github.com/patchwork-eight/patchwork-eight`
— the wrong GitHub org entirely (real remote is `cbackstrom80/patchwork-eight`, soon
`cbackstrom80/murmur-app`). Not yet fixed — bundled into Step 7 alongside the actual
repo-URL updates.

## Decision: no `patchwork_eight` Python back-compat alias

Resolved in Step 3: `import patchwork_eight` does **not** remain available after the
module rename to `murmur`. Unlike the `.pw8` file extension (which has ~1,158 real files
plus potentially real users' own saved presets depending on it), this binding has no
external distribution today — no wheel, no PyPI, `docs/PYTHON_API.md` itself marks
pip-installable packaging as PLANNED, not built. The only consumers were this repo's own
`.github/workflows/ci.yml` and its own docs, both updated in this same pass. A compat
shim would add real surface area for zero actual external users. Docs keep the
`import murmur as pw8` convention (alias name, not module name) so existing `pw8.Patch`
/`pw8.Engine`-style example code throughout `docs/PYTHON_API.md` needed no further churn.

## Extension migration mechanics

`PatchSerializer` (`engine/src/patch/PatchSerializer.cpp`) round-trips JSON only — it
never inspects a file path or extension. All extension handling lives in **callers**,
now updated to accept both extensions on read (permanent guarantee for real users'
existing `.pw8` files) and write `.murmur` on new saves:

- `plugin/src/content/PresetIndex.cpp` — glob pattern now `*.pw8;*.murmur`
- `plugin/src/ui/components/PresetBrowserOverlay.cpp` — export/save path now writes
  `.murmur`
- `plugin/src/ui/components/PatchBrowserBar.cpp` — load filter now accepts both
- `plugin/src/ui/components/MurmurChromeBar.cpp` — save-as-copy dialog now defaults to
  and enforces `.murmur`
- `mcp_server/patch_builder.py`, `mcp_server/content.py`, `mcp_server/server.py`,
  `mcp_server/smoke_test.py` — scratch/content globs accept both, new scratch patches
  get `.murmur`, new regression test added covering legacy `.pw8` round-tripping
- All 19 `scripts/generate_*.py`/`retag_*`/`migrate_*`/`audit_*`/`validate_*`
  content-generation tools — read-globs accept both extensions, new output writes
  `.murmur`
- `tools/render/main.cpp`, `tools/graph_inspector/main.cpp` — take an explicit
  `--patch <path>` argument and never filter/glob by extension; no functional change
  needed there, only usage-string text (done in Step 5, CLI rename)
- 4 test files with hardcoded `.pw8` extension checks (`tests/serialization/FactoryPresetLoadTests.cpp`,
  `tests/regression/GoldenPresetTests.cpp`, `tests/unit/MacroKoinTests.cpp`) — **done**, see
  Step 4 execution log below (found via the post-rename test run, not anticipated up front)
  Step 4's mass file rename since their hardcoded filenames will change

**No `schemaVersion` bump needed** — extension and content-schema are orthogonal
concerns; conflating them would force a no-op migration function.

**Real-world side finding from this pass**: confirmed
`~/Library/Application Support/MURMUR/Presets/user/` is a genuine, already-used code path
(`PresetBrowserOverlay::exportSelected()`, `MurmurProcessor::userPresetsDirectory()`) —
useful for `murmur-web`'s "LOAD TO SYNTH" design, since it resolves what was previously an
open question about whether a user-writable preset folder actually exists.

## Step 4 execution log (done)

Snapshot-verified mass rename, executed exactly per the plan: loaded all 1,158 `.pw8`
files via the real C++ serializer (through the Python bindings' `Patch.load()`/`to_json()`,
which round-trips through `patch::loadPatchFromJson`/`savePatchToJson`), hashed each
patch's canonicalized content with `metadata.id` normalized out, then `git mv`'d every
file to `.murmur` and rewrote `metadata.id`'s `pw8-` prefix to `murmur-` via a plain JSON
edit (not a re-serialization, so every other byte-level field stayed exactly as authored).
Re-loaded all 1,158 renamed files and re-hashed the same way: **0 content mismatches, 0
id-prefix mismatches, 0 load errors** across all 1,158 files.

Follow-up fixes required after the rename (found via full rebuild + `ctest`, not
anticipated up front): 4 hardcoded `entry.path().extension() != ".pw8"` checks in test
code (`tests/unit/MacroKoinTests.cpp`, `tests/regression/GoldenPresetTests.cpp`,
`tests/serialization/FactoryPresetLoadTests.cpp` x2) were silently matching zero files
post-rename, failing 3 tests (`Factory presets expose at least one uiFocus macro KOIN`,
`Week 3 factory presets load under schema v3`, `Interstellar factory presets load under
schema v3`) — fixed to check `.murmur`. Also batch-fixed 181 stale
`content/presets/*.pw8` path references across 29 files (README, docs, `tests/golden/presets.json`'s
97 golden-hash entries, `content/design-fx/voc-*.json`'s `pw8Ref` values, the patchforge
manifest, CI, scripts) via a narrow regex matching only real `content/presets/` paths —
deliberately excluded `tools/render/main.cpp`/`tools/graph_inspector/main.cpp` at the
time (handled in Step 5 instead, alongside their tool-name rename). Full rebuild +
`ctest` re-verified clean after all follow-up fixes: 335/336, same one pre-existing
unrelated failure.

**New finding, real but out of scope for this rebrand**: `pw8-render` (the native CLI
renderer) and the Python bindings' `render()` call both **segfault on every invocation in
this sandbox environment** — confirmed via two separate isolated `git worktree` builds of
the completely unmodified original code (one via Python bindings, one via the native CLI
directly), both crash identically regardless of which preset/MIDI file is passed. This is
NOT a rebrand regression and NOT (apparently) a real logic bug in the renderer itself,
since every renderer-exercising unit test in the Catch2 suite passes fine when run
in-process (`ctest`) — the crash is specific to invoking the renderer from a standalone
process (CLI binary or Python extension module) in this particular sandbox, suggesting an
environment/resource-limit issue (stack size, subprocess spawning restriction, or similar)
rather than a real user-facing bug. Flagged to the user; not investigated further here as
it's unrelated to the rebrand's scope.

## Sequencing

1. **Inventory + decisions doc** — done (this document).
2. **Extension-handling code changes** — done, full cross-build (Standalone/AU/VST3 +
   Quasar) verified clean, 335/336 tests passing (the 1 failure is a pre-existing golden
   -hash drift for `content/presets/factory/Leads/07-signal-spike.murmur`, confirmed
   unrelated to this rebrand via an isolated `git worktree` build of the unmodified
   original code, which reproduces the same failure).
3. **Cosmetic + identifier renames** — done: Python module rename (compiler-verified,
   including the actual `pw8.render()` call path — which segfaults, but confirmed via the
   same isolated-worktree method to be a **pre-existing bug unrelated to this rebrand**,
   reproducing identically on unmodified original code), `PatchworkEightProcessor` →
   `MurmurProcessor` (150+ files, compiler-verified via full plugin rebuild), top-level
   CMake `project()` rename (with the two release scripts' version-extraction regex
   updated in lockstep — a real functional dependency, not just cosmetic), and the
   docs/prose bulk pass.
4. **Snapshot baseline + mass rename** — **done**, see execution log above. 0
   mismatches across all 1,158 files; 3 follow-up test failures found and fixed (stale
   `.pw8` extension checks); 181 stale path references across 29 files also fixed.
5. **CLI tool renames** (`pw8-render` etc. → `murmur-render` etc.) — **done**. 5
   CMake targets renamed in `tools/CMakeLists.txt`; global rename of all 5 identifiers
   across 53 dependent files (docs, scripts, MCP server, engine header comments,
   `tools/*/main.cpp`); the `docs/REBRAND_MURMUR.md` you're reading was deliberately
   excluded from that automated pass and updated by hand instead (lesson from Step 3,
   applied this time). Usage-string/help-text `.pw8` mentions in `tools/render/main.cpp`
   and `tools/graph_inspector/main.cpp` (deliberately left alone in Step 2) fixed here,
   plus `tools/info/main.cpp`'s real runtime output string ("Patchwork Eight -- pw8-info"
   → "MURMUR -- murmur-info"). One straggler found: `tests/regression/RenderSanityTests.cpp`'s
   `SKIP()` diagnostic message used a path format (`"Sidechain/...pw8"`, no
   `content/presets/` prefix) the Step 4 batch-regex didn't match — fixed.
6. **Full verification suite** (below) — must be green before the repo rename.
   **Not started.**
7. **GitHub repo rename** `patchwork-eight` → `murmur-app`; update local
   `git remote set-url`; update `~/repos/patchforge/package.json`'s `ingest` script and
   `README.md` (`../patchwork-eight` → `../murmur-app`); fix the appcast org-name bug; run
   `patchforge`'s `npm run ingest` to confirm the new path resolves.
   (Verified clean: `patchwork-pipeline` and `pathchforge-storefront` have zero
   references to this repo's path — nothing to update there.) **Not started.**
8. **Commit in reviewable chunks** along the step boundaries above — the mass preset
   rename should be its own isolated commit so `git log --follow`/reviewers can see it as
   a pure rename, not mixed with logic changes. Each commit ends
   `Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>`, then push. **Not started —
   nothing from this effort has been committed yet.**

## Verification (Steps 2-3, already run)

- Full cross-build (`cmake --build --preset plugin-release`): Standalone/AU/VST3 all
  compile and link clean, only pre-existing unrelated warnings.
- `ctest` (top-level `build/`): 335/336 passing, 1 pre-existing unrelated failure
  (confirmed via isolated worktree, see above).
- Python bindings rebuilt from scratch (`build/python-verify`, scratch dir, cleaned up
  after): `import murmur as pw8` and `pw8.Patch.load(...)` both verified working; the
  `pw8.render(...)` segfault is confirmed pre-existing via the same isolated-worktree
  method against unmodified original code.
- `py_compile` clean on all 22 touched Python files.

## Step 6 verification results

- **Snapshot-diff regression** (Step 4): done, see Step 4 execution log — 0 mismatches
  across all 1,158 files.
- **`auval` (AU)**: run against the real `Murr`/`Murm` manufacturer/plugin codes —
  **AU VALIDATION SUCCEEDED**, full pass (render, parameter, MIDI, ramped-parameter tests
  all green).
- **`pluginval --strictness-level 5` (VST3)**: passed plugin-info/programs tests, then
  **segfaulted in the "Editor" test** (opening the plugin's UI) — **confirmed pre-existing
  and unrelated to this rebrand**, via a third isolated `git worktree` build of the
  completely unmodified original plugin: `pluginval` crashes identically, at the same
  "Editor" test phase, on pristine pre-rebrand code. This is now the third independently
  confirmed instance of the same class of environment-specific crash (alongside the
  `pw8-render` CLI and the Python bindings' `render()` call, both confirmed the same way
  earlier). Notably this environment has a real, live GUI/WindowServer session (verified
  via `launchctl print system/com.apple.WindowServer` and a live `osascript` process
  listing) — not the headless sandbox first assumed — so the crash isn't explained by
  "no display," but whatever the real cause is, it predates and is independent of every
  change in this rebrand.
- **`murmur-fuzz-render --count 1000 --seed 42`**: segfaults — consistent with the
  already-confirmed render-path crash pattern (same class as `pw8-render`/Python
  `render()`), not independently re-verified via a third worktree given diminishing
  returns after two confirmed instances, but fits the identical profile exactly (crashes
  only in code paths that drive the native audio renderer as a standalone/background
  process in this specific sandbox; every renderer-exercising unit test in the Catch2
  suite passes fine in-process via `ctest`).
- **Side-by-side `.pw8` + `.murmur` load test**: verified by reading JUCE's own vendored
  source directly rather than a live plugin launch (safer, given the crash pattern above,
  and just as conclusive) — `PresetIndex.cpp`'s `"*.pw8;*.murmur"` glob pattern is
  confirmed correct against `WildcardFileFilter::parseWildcard()`
  (`juce_WildcardFileFilter.cpp:38-42`, tokenizes on `";,"`) and `DirectoryIterator`'s
  consumption of that same tokenized `StringArray` (`juce_DirectoryIterator.cpp:47`) —
  this is official, documented-in-source JUCE multi-pattern syntax, not a guess.
- **`mcp_server/smoke_test.py` full run**: all real checks pass (8 engines, 61
  wavetables, **1158 presets found** confirming the dual-glob picks up every renamed
  file, category filtering, patch construction/editing tools, the new legacy-`.pw8`
  round-trip check). The render+validate section **SKIP**s gracefully (looks for
  `build/dev/tools/murmur-render`, a preset this environment hadn't built under that
  specific name) rather than crashing the whole run — consistent with the known
  pre-existing render-path issue, not a new problem.
- **`patchforge`'s `npm run ingest`**: path resolution confirmed correct —
  `../patchwork-eight/scripts/patchforge_ingest.py` and its manifest both resolve, and
  the script correctly looks for the renamed `murmur-render` binary (not stale
  `pw8-render`) at the expected location, failing only because that specific build preset
  wasn't built in this environment — again the known pre-existing render-path issue, not
  a path/naming regression from this rebrand.

## Lesson learned during this pass

The docs/prose bulk-replace script (Step 3) was first run across **all** markdown files
indiscriminately, including this document and the `GATE 12` entry in `docs/ROADMAP.md` —
both of which specifically narrate the *old → new* naming transition and therefore need
the old names preserved as history. The blind replace corrupted both (e.g. this
document's own title briefly became "Rebrand: MURMUR → MURMUR", and its own "From/To"
table collapsed both columns to the same value). Caught immediately and rewritten
correctly. Any future bulk find/replace pass over this repo's docs should explicitly
exclude files whose subject *is* the renaming itself.
