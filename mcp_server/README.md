# Patchwork Eight MCP Server

**PROTOTYPE.** First real implementation of Part A from
[docs/MCP_AND_NL_PATCH_GENERATION.md](../docs/MCP_AND_NL_PATCH_GENERATION.md)
-- an [MCP](https://modelcontextprotocol.io) server exposing Patchwork Eight
patch introspection, construction, editing, and rendering as tools to any
MCP-capable client (Claude Desktop, Claude Code, etc.). Part B of that doc
(the in-app "make me a laser sound" chat box) is unrelated, unstarted, and
depends on real product decisions this prototype doesn't need.

Operates directly on the `.pw8` JSON schema
([docs/PATCH_FORMAT.md](../docs/PATCH_FORMAT.md)) rather than
`bindings/python`'s still-PARTIAL `Operator` wrapper -- no C++ build
dependency beyond the already-built `pw8-render` CLI (used for
`render_preview`/`validate_patch` only; every other tool is pure Python).

## Setup

```bash
pip install -r mcp_server/requirements.txt
cmake --build --preset dev   # builds tools/pw8-render, needed for render_preview/validate_patch
```

## Try it

```bash
python3 mcp_server/smoke_test.py    # exercises the underlying logic directly, no MCP transport
```

builds a real FM/PM + Resonator "laser" patch end to end -- creates it,
edits it, renders it through the real engine, and validates the result --
without needing an MCP client at all. Good first thing to run after any
change here.

## Connect a real MCP client

Claude Desktop (`~/Library/Application Support/Claude/claude_desktop_config.json`
on macOS) or Claude Code:

```json
{
  "mcpServers": {
    "patchwork-eight": {
      "command": "python3",
      "args": ["/absolute/path/to/patchwork-eight/mcp_server/server.py"]
    }
  }
}
```

Restart the client, then ask it something like *"list the Patchwork Eight
engines"* or *"build me a laser sound using the FM/PM engine"* -- it should
discover and call the tools below on its own.

## Tools

Read-only introspection: `list_engines`, `list_mod_sources_and_destinations`,
`list_effect_types`, `list_wavetables`, `list_presets`, `read_patch`,
`explain_patch`.

Construction/editing (operate on a scratch patch identified by `patch_id`,
returned by `create_patch` -- nothing here touches `content/presets/` until
`save_patch` is called explicitly): `create_patch`, `set_operator`,
`add_mod_route`, `set_effect`, `set_envelope`, `set_filter`,
`list_scratch_patches`, `save_patch`, `delete_scratch_patch`.

Rendering: `render_preview` (real audio through `pw8-render`, returns
peak/rms/NaN-Inf metrics + the rendered WAV's path -- open it directly to
listen, audio isn't streamed back through the tool call in this pass),
`validate_patch` (low/mid/high-note pass/fail check, the same bar the
250-patch factory preset bank was checked against).

## Files

- `server.py` -- the FastMCP server; thin `@mcp.tool()` wrappers only.
- `patch_schema.py` -- engine/effect/mod-matrix field tables mirrored from
  the real C++ source of truth (`PatchSerializer.cpp`'s `clampNum(...)`
  calls, `AlgorithmTypes.hpp`, etc.), plus name<->id resolution helpers.
- `patch_builder.py` -- the actual patch construction/editing logic,
  deliberately undecorated so it's directly unit-testable (see
  `smoke_test.py`) without an MCP transport in the loop.
- `content.py` -- read-only `content/presets/` + `content/wavetables/`
  browsing.
- `render.py` -- shells out to `pw8-render --receipt` and parses the
  resulting JSON (peak/rms/NaN-Inf) -- the same real DSP path, not a
  synthetic check.
- `midi_writer.py` -- a minimal hand-rolled Standard MIDI File writer for
  preview renders, kept dependency-free rather than adding `mido` (matches
  this project's general preference for small, obvious dependencies).
- `smoke_test.py` -- end-to-end check of the logic above, no MCP client
  needed.

## A real gotcha hit while building this

`server.py` cannot use `from __future__ import annotations`. FastMCP's tool
registration inspects `param.annotation` at decoration time expecting real
type objects (to detect a `Context`-typed parameter before generating each
tool's JSON schema); PEP 563 postponed evaluation turns every annotation
into a plain string instead, and `issubclass("Optional[str]", Context)`
throws `TypeError: issubclass() arg 1 must be a class`. `patch_schema.py`/
`patch_builder.py`/etc. don't go through FastMCP's decorator, so they keep
the `__future__` import fine -- it's specifically the file with `@mcp.tool()`
in it that can't have it.

## What's not built yet

- Streaming rendered audio back through the tool call itself (currently:
  path + metrics only, open the WAV yourself).
- Anything from Part B of the idea doc -- no chat UI, no LLM backend
  wiring, no BYO-model or hosted-subscription decision.
- `bindings/python`'s `Operator` wrapper isn't used at all here on purpose
  (see the top of this file) -- worth revisiting once that wrapper's
  coverage catches up to the full schema, if a live in-process `Engine`
  (rather than shelling out to `pw8-render` per call) turns out to matter
  for latency.
