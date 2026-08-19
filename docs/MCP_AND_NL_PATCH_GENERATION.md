# MCP Server + Natural-Language Patch Generation

**Part A: PROTOTYPE EXISTS** (`mcp_server/`, see its README) -- a real,
working MCP server with 18 tools, smoke-tested end to end including through
the actual MCP stdio protocol (not just its underlying Python functions).
**Part B: still IDEA -- captured for future scoping, not committed, no
timeline.** This doc exists so the idea has a real home with real
constraints attached, the same way every other PLANNED item in this project
gets a documented rationale instead of living only in someone's head. See
`docs/ROADMAP.md` Phase 18 and `docs/PATCHWORK_INTEGRATION.md`'s "Generate /
Mutate / Breed / Lock" section, which this doc extends rather than replaces
-- the core rule there (**AI inference never runs inside `pw8_core`'s audio
thread, and doesn't run inside this repository at all**) still holds for
everything below; the MCP server is a tool-calling surface an LLM client
uses from *outside* this repository, not inference running inside it.

Two distinct features got proposed together; they're architecturally very
different problems and are split out on purpose so neither accidentally
blocks the other.

## Part A: an MCP server over Patchwork Eight

[Model Context Protocol](https://modelcontextprotocol.io) exposes a set of
callable "tools" to any MCP-capable client -- Claude Desktop, Claude Code,
and a growing list of others. A Patchwork Eight MCP server would let any of
those clients read, build, and audition `.pw8` patches conversationally,
without a plugin host or DESIGN-mode UI in the loop at all. This is
low-risk, mostly-engineering, and buildable independent of Part B.

**Status: built.** `mcp_server/` has a real `FastMCP`-based server (18
tools: introspection, scratch-patch construction/editing, rendering/
validation) verified three ways -- direct unit-style calls
(`smoke_test.py`), a real client session over the actual MCP stdio
transport, and a real `pw8-render` pass on the resulting patch (no NaN/Inf,
audible, passes the same low/mid/high-note validation the factory preset
bank was checked against). See `mcp_server/README.md` for setup, the tool
list, and a real gotcha it hit (`from __future__ import annotations`
breaks FastMCP's tool registration). What's below is the original design
rationale; it held up building the real thing.

### Why this is buildable today, cheaply

Two of `PATCHWORK_INTEGRATION.md`'s four existing integration boundaries are
already complete enough to build tools directly on top of:

- **The patch schema** (boundary #3) -- `PatchSerializer`'s `toJson`/
  `fromJson` cover all 8 engine types' fields today (`docs/PATCH_FORMAT.md`),
  not just the subset the Python API wraps.
- **The CLI** (boundary #1) -- `pw8-render`, `pw8-info`, `pw8-graph` are
  already scriptable, file-in/file-out, no shared process state.

Concretely: MCP tools should generate/edit raw `.pw8` JSON directly rather
than going through `bindings/python`'s `Operator` wrapper object. Per
`docs/PYTHON_API.md`'s own coverage table, that wrapper is still **PARTIAL**
(engine/waveform/ratio/level only -- no mod-matrix, FX, or macro editing
yet). Working against the JSON schema directly sidesteps waiting on that
surface to catch up, since the schema itself has no such gap.

### A real, existing safety net

Every field `PatchSerializer::fromJson` reads goes through `clampNum(...)`
to its real valid range (`docs/PATCH_FORMAT.md`) -- the same defensive
parsing a hand-authored or Python-authored patch already gets. An
LLM-hallucinated field value (`"resonatorDecay": 400`) can't produce an
out-of-range DSP parameter; it gets silently clamped like any other bad
input. This safety net already exists for free -- nothing new to build to
make LLM-authored patches as safe as human-authored ones.

### Sketch of the tool surface

- `list_engines()` -- the 8 `EngineType`s and each one's fields (mirrors
  `AlgorithmTypes.hpp` / `OperatorPatch`).
- `list_wavetables()` -- `content/wavetables/MANIFEST.md`'s index.
- `list_presets(category?)` -- browse `content/presets/` (including the
  factory bank and showcase content already checked in).
- `create_patch(name, description)` -- start from an init patch.
- `set_operator(patch_id, index, engine, params)` -- one operator's engine +
  fields.
- `add_mod_route(patch_id, source, destination, amount, scope)` -- wire the
  mod matrix.
- `set_effect(patch_id, slot, type, params)` -- configure an insert/master
  FX slot.
- `render_preview(patch_id, notes, duration)` -- render audio back to the
  client as an attachment, so whoever's on the other end can actually hear
  the result, not just read JSON.
- `save_patch(patch_id, path)` -- explicit write, never implicit/silent
  (same "no engine pretends to work" honesty this project already applies
  elsewhere).
- `validate_patch(patch_id)` -- the same finite/bounded check
  `pw8-fuzz-render` already does, run against one specific patch before
  handing it back.

### Where it would live

Its own package, most likely Python (an `mcp` SDK already exists there),
under `bindings/` or a new top-level `mcp/` -- reusing the pybind11 module
for live-render tool calls and the CLI tools for one-shot renders.
Deliberately not woven into `pw8_core` or `pw8_plugin` -- same "standalone
repository, integrates through structured boundaries" philosophy
`PATCHWORK_INTEGRATION.md` already commits to.

### Rough phasing

1. ~~Read-only tools first (list engines/wavetables/presets, explain a
   loaded patch back in words) -- zero risk, pure introspection, immediately
   useful on its own even before any generation tool exists.~~ **DONE.**
2. ~~Patch construction/editing tools, writing to a scratch `.pw8`, always
   behind an explicit `save_patch` call.~~ **DONE.**
3. `render_preview` returns metrics + a WAV file path today; still open:
   streaming the audio itself back over MCP's resource mechanism so a
   client doesn't need filesystem access to hear the result.
4. Not started: wiring this server into an actual client's default config,
   real usage beyond this repo's own smoke tests, and revisiting whether a
   live in-process `Engine` (instead of shelling out to `pw8-render` per
   call) matters once real usage patterns exist.

### Standalone live bridge (2026-08-17)

**Built:** MURMUR Standalone starts a **localhost-only HTTP bridge**
(`plugin/src/standalone/StandaloneMcpBridge.*`) and writes
`~/Library/Application Support/MURMUR/mcp-bridge.json`. MCP tools
`standalone_status`, `load_into_standalone`, and `load_preset_into_standalone`
push scratch or saved `.pw8` files into the running app via the same
message-thread `loadPatchFromFile()` path as the preset browser.

Repo `.mcp.json` registers the server for Cursor. VST3/AU builds omit the
bridge (Standalone-first trust boundary from Part B below).

## Part B: an in-app chat box ("make me a laser sound")

A much bigger surface: a natural-language prompt box *inside* the plugin (or
a companion app) where a phrase gets turned into a loaded, audible patch.
The chat box itself could literally be an MCP client embedded in the
plugin, calling the exact same tool surface as Part A -- build the "text in,
patch out" capability once, reuse it here.

### Two backend models, both worth planning for

**1. Bring-your-own-model (BYO).** The user supplies their own API key
(OpenAI/Anthropic/etc.) or points at a local model (Ollama/LM Studio, say).
Low ops burden -- no hosting, no billing, no rate-limiting infrastructure;
the user owns their own cost and their own data.

Real constraint worth flagging now rather than discovering later: **a
plugin (VST3/AU) making outbound network calls is unusual**, and some hosts
and security-conscious users actively distrust it -- "why is my synth
phoning home" is a legitimate question regardless of what the calls are
actually for. Worth restricting this feature to the **Standalone app
first**, keeping it away from the plugin formats (and therefore away from
any DAW session / host relationship) until there's a real reason to expand
it -- the same kind of honest trust-boundary caveat already applied to
ad-hoc signing / notarization elsewhere in this project.

**2. Hosted subscription (Curtis-operated).** A real backend service,
subscribers pay for. This is explicitly a **business decision, not an
engineering default** -- same framing `docs/LICENSING.md` already applies to
the still-unresolved JUCE commercial license. It needs an account/auth
system, billing integration, a real inference backend (self-hosted model,
or a metered reseller markup on a provider API), rate limiting/abuse
prevention, and content moderation (an open "describe any sound" text box
is also an open text box for anything else, and needs the same guardrails
any hosted LLM product needs). Only makes sense once Part A's tool-calling
quality is proven out and there's a real demand signal -- a phase-3-plus
idea, not a first build.

### What both share

- The engine-facing half is identical either way: whatever calls into the
  running instance to load the LLM's resulting patch goes through the exact
  same `loadPatch()` / atomic `publishEngine()` swap every other patch load
  already uses (`docs/PLUGIN_ARCHITECTURE.md` "Threading"). The chat feature
  is a new patch *source*, not a new patch-loading code path -- nothing
  about the audio thread's discipline changes.
- Needs a UI home. Doesn't fit today's PLAY-mode-only editor
  (`docs/UI.md`) -- most naturally lands as part of a future DESIGN/LAB mode
  (already **PLANNED**, Phase 17), not bolted onto PLAY mode.

## Explicitly out of scope for now

- AI inference inside `pw8_core`'s audio thread -- never, unchanged from
  `PATCHWORK_INTEGRATION.md`.
- Which LLM vendor/API, hosting infrastructure, and pricing are all
  deliberately undecided here -- this is scoping, not a build plan.
- Whether this ships as part of Patchwork Eight itself vs. as a feature of
  the separate "Patchwork" AI product it already has an integration
  boundary with (`PATCHWORK_INTEGRATION.md`'s "standalone repository, not
  coupled" framing) -- worth an explicit call before any real engineering
  starts on Part B specifically. Part A (the MCP server) is small and
  self-contained enough that it doesn't need that decision made first.
