#!/usr/bin/env python3
"""Patchwork Eight MCP server -- docs/MCP_AND_NL_PATCH_GENERATION.md Part A.

Exposes patch introspection, construction, editing, and rendering as tools
to any MCP-capable client (Claude Desktop, Claude Code, etc.) over stdio.
Operates directly on the .pw8 JSON schema (docs/PATCH_FORMAT.md) rather than
bindings/python's still-PARTIAL Operator wrapper -- see that doc for why.

IDEA-STAGE, first real implementation. Read-only tools (list_engines,
list_wavetables, list_presets, read_patch, explain_patch) plus construction/
editing tools that operate on scratch patches under mcp_server/scratch/
(gitignored) -- nothing here writes into content/presets/ except through the
explicit save_patch tool.

Setup:
    pip install mcp
    cmake --build --preset dev   # pw8-render, needed for render_preview/validate_patch

Run directly for a smoke test (also see smoke_test.py, which exercises the
underlying functions directly without the MCP transport):
    python3 mcp_server/server.py

Point an MCP client at it, e.g. Claude Desktop's config
(~/Library/Application Support/Claude/claude_desktop_config.json on macOS):
    {
      "mcpServers": {
        "patchwork-eight": {
          "command": "python3",
          "args": ["/absolute/path/to/patchwork-eight/mcp_server/server.py"]
        }
      }
    }
"""
import shutil
import sys
from pathlib import Path
from typing import Optional

sys.path.insert(0, str(Path(__file__).resolve().parent))

from mcp.server.fastmcp import FastMCP

import content
import patch_builder
import render as render_mod
from patch_schema import (
    BASE_OPERATOR_FIELDS, EFFECT_FIELDS, EFFECT_TYPE_IDS, ENGINE_EXTRA_FIELDS,
    ENGINE_NAMES, MOD_DEST_IDS, MOD_SCOPE_IDS, MOD_SOURCE_IDS,
)

mcp = FastMCP("patchwork-eight")


# -- Read-only introspection (Phase 1) ---------------------------------------

@mcp.tool()
def list_engines() -> dict:
    """Lists all 8 Patchwork Eight synthesis engines, each with its extra
    parameter fields (name/type/range/default/note). Every operator also has
    the base fields (waveform/ratio/level/pan/etc.) regardless of engine --
    see the 'base_fields' key."""
    return {
        "base_fields": BASE_OPERATOR_FIELDS,
        "engines": {
            name: {"id": idx, "extra_fields": ENGINE_EXTRA_FIELDS.get(name, {})}
            for idx, name in ENGINE_NAMES.items()
        },
    }


@mcp.tool()
def list_mod_sources_and_destinations() -> dict:
    """The mod matrix's source/destination/scope vocabulary for add_mod_route."""
    return {
        "sources": sorted(MOD_SOURCE_IDS),
        "destinations": sorted(MOD_DEST_IDS),
        "scopes": sorted(MOD_SCOPE_IDS),
        "note": "operator_level and operator_wavetable_position destinations need target_index (0-7).",
    }


@mcp.tool()
def list_effect_types() -> dict:
    """The FX types set_effect accepts, each with its own parameter fields."""
    return {name: EFFECT_FIELDS.get(name, {}) for name in EFFECT_TYPE_IDS}


@mcp.tool()
def list_wavetables() -> list[dict]:
    """content/wavetables/*.json -- pass one's 'id' as an operator's
    wavetable_id param for the wavetable or granular engine."""
    return content.list_wavetables()


@mcp.tool()
def list_presets(
    category: Optional[str] = None,
    mood: Optional[str] = None,
    genre: Optional[str] = None,
    tag: Optional[str] = None,
) -> list[dict]:
    """Browses content/presets/ (including the 250-patch factory bank),
    summarized. Pass category/mood/genre/tag to filter (AND semantics)."""
    return content.list_presets(category, mood, genre, tag)


@mcp.tool()
def read_patch(path: str) -> dict:
    """Reads one .pw8 file's full JSON, given a repo-relative or absolute path."""
    return content.read_patch_file(path)


@mcp.tool()
def explain_patch(path: str) -> str:
    """A human-readable summary of a saved patch (repo-relative or absolute
    path): active operators/engines, envelope, filter, mod routes, FX --
    easier to reason about than raw JSON."""
    return patch_builder.explain_patch(content.read_patch_file(path))


# -- Construction / editing (Phase 2, scratch patches) -----------------------

@mcp.tool()
def create_patch(name: str, description: str = "") -> dict:
    """Starts a new scratch patch (8 silent operators except operator 0, a
    plain sine). Returns patch_id -- pass it to every other editing tool
    below, then save_patch when done."""
    patch_id, patch = patch_builder.create_patch(name, description)
    return {"patch_id": patch_id, "summary": patch_builder.explain_patch(patch)}


@mcp.tool()
def set_operator(patch_id: str, index: int, engine: str, params: Optional[dict] = None) -> dict:
    """Sets operator `index` (0-7) to `engine` (see list_engines for names:
    classic, wavetable, fm_pm, additive, phase_shape, granular, noise_chaos,
    resonator) with the given field overrides. Unset fields keep their
    defaults. Out-of-range values are clamped, not rejected -- check
    'warnings' in the result."""
    warnings = patch_builder.set_operator(patch_id, index, engine, params)
    return {"patch_id": patch_id, "warnings": warnings,
            "summary": patch_builder.explain_patch(patch_builder.load_scratch(patch_id))}


@mcp.tool()
def add_mod_route(patch_id: str, source: str, destination: str, target_index: int = 0,
                   amount: float = 1.0, scope: str = "voice") -> dict:
    """Adds one mod matrix route. See list_mod_sources_and_destinations for
    valid names. operator_level/operator_wavetable_position destinations
    need target_index (which operator, 0-7)."""
    warnings = patch_builder.add_mod_route(patch_id, source, destination, target_index, amount, scope)
    return {"patch_id": patch_id, "warnings": warnings}


@mcp.tool()
def set_effect(patch_id: str, layer: str, slot: int, effect_type: str, params: Optional[dict] = None) -> dict:
    """Configures one FX slot. layer is 'insert' (3 slots, 0-2) or 'master'
    (4 slots, 0-3). See list_effect_types for effect_type names and fields."""
    warnings = patch_builder.set_effect(patch_id, layer, slot, effect_type, params)
    return {"patch_id": patch_id, "warnings": warnings}


@mcp.tool()
def set_envelope(patch_id: str, attack: float, decay: float, sustain: float,
                  release: float, curve: float = 2.0) -> dict:
    """Sets the layer's amp envelope (seconds for attack/decay/release, 0-1
    for sustain level)."""
    warnings = patch_builder.set_envelope(patch_id, attack, decay, sustain, release, curve)
    return {"patch_id": patch_id, "warnings": warnings}


@mcp.tool()
def set_filter(patch_id: str, enabled: bool = True, mode: str = "lowpass",
                cutoff_hz: float = 2000.0, resonance: float = 0.2, key_track: float = 0.2) -> dict:
    """Sets Filter1. mode: lowpass/highpass/bandpass/notch/peak."""
    warnings = patch_builder.set_filter(patch_id, enabled, mode, cutoff_hz, resonance, key_track)
    return {"patch_id": patch_id, "warnings": warnings}


@mcp.tool()
def set_macro_koin(patch_id: str, slot: int, name: str, destinations: list[dict],
                   description: str = "") -> dict:
    """Configure one feature macro KOIN (Macro1–3): name, optional description,
    uiFocus entry, and mod routes. destinations is a list of dicts, each with
    destination (see list_mod_sources_and_destinations), amount, and optional
    target_index / scope."""
    warnings = patch_builder.set_macro_koin(patch_id, slot, name, destinations, description)
    return {"patch_id": patch_id, "warnings": warnings,
            "summary": patch_builder.explain_patch(patch_builder.load_scratch(patch_id))}


@mcp.tool()
def set_morph_koin(patch_id: str, label: str, keyframes: list[dict],
                   default_position: float = 0.0, description: str = "",
                   curve: str = "linear", wrap: bool = False,
                   position: float | None = None,
                   macro_koins: list[dict] | None = None) -> dict:
    """Configure a Frames-style morph KOIN (Horizon 2 metadata): 2–4 named keyframe
    snapshots interpolated by one performance knob. Writes morphKoin + uiFocus kind morph.
    Optional macro_koins sets paired Spread-style macro KOINS (slot, name, destinations).
    Runtime morph lerp is Horizon 3 — this stores patch metadata only."""
    warnings = patch_builder.set_morph_koin(
        patch_id, label, keyframes, default_position, description,
        curve, wrap, position, macro_koins)
    return {"patch_id": patch_id, "warnings": warnings,
            "summary": patch_builder.explain_patch(patch_builder.load_scratch(patch_id))}


@mcp.tool()
def list_scratch_patches() -> list[dict]:
    """Lists patches currently under construction (not yet saved anywhere permanent)."""
    out = []
    for path in sorted(patch_builder.SCRATCH_DIR.glob("*.pw8")):
        try:
            patch = patch_builder.load_scratch(path.stem)
        except Exception:
            continue
        out.append({"patch_id": path.name, "name": patch["metadata"]["name"]})
    return out


@mcp.tool()
def save_patch(patch_id: str, dest_path: str, overwrite: bool = False) -> dict:
    """Copies a scratch patch to a permanent, repo-relative path (e.g.
    'content/presets/my-laser.pw8'). Refuses to overwrite an existing file
    unless overwrite=True."""
    src = patch_builder.scratch_path(patch_id)
    if not src.exists():
        raise FileNotFoundError(f"no scratch patch '{patch_id}'")
    dest = patch_builder.REPO_ROOT / dest_path
    if dest.exists() and not overwrite:
        raise FileExistsError(f"{dest_path} already exists -- pass overwrite=True to replace it")
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy(src, dest)
    return {"saved_to": str(dest.relative_to(patch_builder.REPO_ROOT))}


@mcp.tool()
def delete_scratch_patch(patch_id: str) -> dict:
    """Deletes a scratch patch that was never saved anywhere permanent."""
    path = patch_builder.scratch_path(patch_id)
    if path.exists():
        path.unlink()
        return {"deleted": True}
    return {"deleted": False, "reason": "not found"}


# -- Rendering / validation ---------------------------------------------------

@mcp.tool()
def render_preview(patch_id: str, notes: Optional[list[int]] = None, hold_seconds: float = 1.5) -> dict:
    """Renders a scratch patch for real (through pw8-render, the same
    offline renderer every other verification pass in this project uses) --
    a chord of MIDI note numbers (default: middle C, 60) held for
    hold_seconds. Returns peak/rms/NaN-Inf metrics and the rendered WAV's
    path (open it directly to listen -- audio isn't streamed back through
    this tool call)."""
    return render_mod.render_preview(patch_id, notes, hold_seconds)


@mcp.tool()
def validate_patch(patch_id: str) -> dict:
    """Renders low/mid/high notes and checks for NaN/Inf, silence, and
    clipping -- the same bar the 250-patch factory preset bank was checked
    against before being bundled into the installer. Run this before
    save_patch."""
    return render_mod.validate_patch(patch_id)


if __name__ == "__main__":
    mcp.run()
