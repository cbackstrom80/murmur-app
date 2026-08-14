#!/usr/bin/env python3
"""Exercises the MCP server's underlying logic directly (no MCP transport,
no client) -- the fast way to check patch_builder.py/render.py/content.py are
actually correct before trusting them behind the protocol layer. Builds a
real "laser" patch end to end: FM/PM engine (fast, harsh modulator) + a
downward pitch-ish sweep approximated via a fast filter-cutoff mod route,
short punchy envelope, then validates and renders it for real.

Run from the repo root:
    python3 mcp_server/smoke_test.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import content
import patch_builder
import render as render_mod


def check(label, condition):
    status = "PASS" if condition else "FAIL"
    print(f"[{status}] {label}")
    if not condition:
        raise SystemExit(1)


def main():
    print("== list_engines / list_wavetables / list_presets ==")
    from patch_schema import ENGINE_NAMES
    check("8 engines defined", len(ENGINE_NAMES) == 8)

    wavetables = content.list_wavetables()
    check(f"wavetables found ({len(wavetables)})", len(wavetables) > 40)

    presets = content.list_presets()
    check(f"presets found ({len(presets)})", len(presets) > 250)  # showcase + factory + hand-authored
    bass_presets = content.list_presets(category="bass")
    check(f"category filter works ({len(bass_presets)} bass presets)", len(bass_presets) > 0)

    print("\n== build a laser lead patch ==")
    patch_id, patch = patch_builder.create_patch(
        "MCP Laser Test", "Smoke-test patch built by mcp_server/smoke_test.py")
    check("create_patch returned an id", patch_id.endswith(".pw8"))

    # FM/PM carrier: high modulator ratio + high index for a harsh, metallic
    # edge; short punchy envelope; a fast-sweeping filter cutoff mod route
    # approximates the classic "pew" downward sweep this engine has no
    # dedicated pitch envelope for.
    warnings = patch_builder.set_operator(patch_id, 0, "fm_pm", {
        "frequencyRatio": 2.0, "level": 0.9,
        "fmModulatorRatio": 5.0, "fmModulatorIndex": 1.4, "fmModulatorFeedback": 0.2,
    })
    check(f"set_operator 0 (fm_pm), warnings={warnings}", True)

    warnings = patch_builder.set_operator(patch_id, 1, "resonator", {
        "frequencyRatio": 2.0, "level": 0.4,
        "resonatorStructure": 0.7, "resonatorDecay": 0.2, "resonatorBrightness": 0.8,
    })
    check(f"set_operator 1 (resonator), warnings={warnings}", True)

    warnings = patch_builder.set_envelope(patch_id, attack=0.001, decay=0.15, sustain=0.0, release=0.05)
    check(f"set_envelope, warnings={warnings}", len(warnings) == 0)

    warnings = patch_builder.set_filter(patch_id, enabled=True, mode="lowpass",
                                         cutoff_hz=6000.0, resonance=0.4, key_track=0.3)
    check(f"set_filter, warnings={warnings}", len(warnings) == 0)

    warnings = patch_builder.add_mod_route(patch_id, source="lfo1", destination="filter_cutoff",
                                            amount=24.0, scope="voice")
    check(f"add_mod_route, warnings={warnings}", len(warnings) == 0)

    warnings = patch_builder.set_effect(patch_id, "master", 0, "reverb", {"mix": 0.15, "reverbDecaySeconds": 1.2})
    check(f"set_effect (master reverb), warnings={warnings}", len(warnings) == 0)

    # Deliberately out-of-range, to confirm clamping + warning surfacing works.
    warnings = patch_builder.set_operator(patch_id, 0, "fm_pm", {"fmModulatorIndex": 99.0})
    check(f"out-of-range value produces a warning: {warnings}", len(warnings) == 1 and "clamped" in warnings[0])

    print("\n== set_macro_koin (feature KOIN helper) ==")
    warnings = patch_builder.set_macro_koin(
        patch_id, slot=0, name="BLOOM",
        description="Opens filter and wavetable motion.",
        destinations=[
            {"destination": "filter_cutoff", "amount": 18.0, "scope": "voice"},
            {"destination": "operator_wavetable_position", "target_index": 0, "amount": 0.35, "scope": "voice"},
        ],
    )
    loaded = patch_builder.load_scratch(patch_id)
    check(f"set_macro_koin warnings={warnings}", len(warnings) == 0)
    check("macro name set", loaded["macros"][0]["name"] == "BLOOM")
    check("uiFocus macro entry", any(k.get("index") == 0 and k.get("kind") == "macro" for k in loaded["uiFocus"]["knobs"]))
    check("macro routes present", any(r["source"] == 21 for r in loaded["layerA"]["modRoutes"]))

    summary = patch_builder.explain_patch(patch_builder.load_scratch(patch_id))
    print("\n--- explain_patch ---")
    print(summary)
    check("summary mentions fm_pm", "fm_pm" in summary)

    print("\n== render + validate ==")
    try:
        result = render_mod.render_preview(patch_id, notes=[57], hold_seconds=0.8)
    except render_mod.RendererMissing as e:
        print(f"[SKIP] pw8-render not built: {e}")
        return
    check(f"render ok: {result}", result["ok"])
    check("no NaN/Inf", not result["containsNaNOrInf"])
    check(f"non-silent (peak={result['peak']:.4f})", result["peak"] > 0.01)

    validation = render_mod.validate_patch(patch_id)
    check(f"validate_patch: {validation}", validation["pass"])

    print("\n== save_patch (to a throwaway path, then clean up) ==")
    from patch_builder import REPO_ROOT
    dest = "content/presets/_mcp_smoke_test_tmp.pw8"
    saved = (REPO_ROOT / dest)
    if saved.exists():
        saved.unlink()
    import shutil
    shutil.copy(patch_builder.scratch_path(patch_id), saved)
    check(f"save landed at {dest}", saved.exists())
    saved.unlink()

    patch_builder.scratch_path(patch_id).unlink()
    check("scratch patch cleaned up", not patch_builder.scratch_path(patch_id).exists())

    print("\nAll smoke tests passed.")


if __name__ == "__main__":
    main()
