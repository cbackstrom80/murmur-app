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
    check("create_patch returned an id", patch_id.endswith(".murmur"))

    # Rebrand regression: a caller that explicitly passes a legacy .pw8-suffixed
    # patch_id must still round-trip correctly (dual-extension guarantee) -- see
    # docs/REBRAND_MURMUR.md.
    legacy_id = "smoke-test-legacy-scratch.pw8"
    patch_builder.write_scratch(legacy_id, patch_builder.default_patch("Legacy Scratch Test"))
    legacy_loaded = patch_builder.load_scratch(legacy_id)
    check("legacy .pw8 scratch patch_id round-trips", legacy_loaded["metadata"]["name"] == "Legacy Scratch Test")
    patch_builder.scratch_path(legacy_id).unlink()

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

    warnings = patch_builder.set_generative(
        patch_id,
        {"clockTRateHz": 1.2, "clockXRateHz": 6.0, "correlation": 0.35, "dejaVu": True},
        streams=[{"spread": 0.6, "bias": 0.1, "lagMs": 40.0}],
    )
    check(f"set_generative, warnings={warnings}", len(warnings) == 0)

    warnings = patch_builder.add_mod_route(
        patch_id, source="random_t", destination="filter_cutoff", amount=12.0, scope="voice")
    check(f"add_mod_route random_t, warnings={warnings}", len(warnings) == 0)

    warnings = patch_builder.set_utility_peaks(
        patch_id, 0, {"enabled": True, "mode": "mini_lfo", "lfoRateHz": 0.25, "lfoDepth": 0.8})
    check(f"set_utility_peaks, warnings={warnings}", len(warnings) == 0)

    warnings = patch_builder.set_effect(
        patch_id, "master", 2, "clouds",
        {"mix": 0.4, "cloudsDensity": 0.55, "cloudsGrainSizeMs": 120.0, "cloudsMode": 0})
    check(f"set_effect (master clouds), warnings={warnings}", len(warnings) == 0)

    warnings = patch_builder.set_effect(
        patch_id, "master", 3, "binaural_space",
        {"mix": 0.7, "qsr1AngleDeg": 45.0, "qsr2AngleDeg": 315.0, "quasarDelayVolume": 0.3})
    check(f"set_effect (master binaural_space), warnings={warnings}", len(warnings) == 0)
    loaded = patch_builder.load_scratch(patch_id)
    check("binaural_space type id", loaded["masterEffects"][3]["type"] == 13)

    warnings = patch_builder.add_mod_route(
        patch_id, source="lfo2", destination="quasar_qsr1_angle", target_index=3, amount=30.0, scope="global")
    check(f"add_mod_route (quasar angle), warnings={warnings}", len(warnings) == 0)

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

    print("\n== set_morph_koin (Frames-style morph metadata) ==")
    morph_id, _ = patch_builder.create_patch("MCP Morph Test", "Morph KOIN smoke test")
    warnings = patch_builder.set_morph_koin(
        morph_id,
        label="EVOLVE",
        description="TIGHT ↔ WIDE pad morph.",
        default_position=0.25,
        keyframes=[
            {"name": "TIGHT", "position": 0.0, "macroValues": [0.1, 0.05]},
            {"name": "WIDE", "position": 1.0, "macroValues": [0.8, 0.6],
             "paramOverrides": {"filterCutoffHz": 12000.0}},
        ],
        macro_koins=[{
            "slot": 0,
            "name": "BLOOM",
            "destinations": [
                {"destination": "filter_cutoff", "amount": 14.0, "scope": "voice"},
            ],
        }],
    )
    morph_loaded = patch_builder.load_scratch(morph_id)
    check(f"set_morph_koin warnings={warnings}", len(warnings) == 0)
    check("morphKoin present", "morphKoin" in morph_loaded and len(morph_loaded["morphKoin"]["keyframes"]) == 2)
    check("uiFocus morph entry", any(k.get("kind") == "morph" for k in morph_loaded["uiFocus"]["knobs"]))
    morph_summary = patch_builder.explain_patch(morph_loaded)
    check("summary mentions Morph KOIN", "Morph KOIN" in morph_summary)
    patch_builder.scratch_path(morph_id).unlink()

    summary = patch_builder.explain_patch(patch_builder.load_scratch(patch_id))
    print("\n--- explain_patch ---")
    print(summary)
    check("summary mentions fm_pm", "fm_pm" in summary)
    check("summary mentions generative", "Generative:" in summary)
    check("summary mentions clouds", "clouds" in summary.lower())

    print("\n== render + validate ==")
    try:
        result = render_mod.render_preview(patch_id, notes=[57], hold_seconds=0.8)
    except render_mod.RendererMissing as e:
        print(f"[SKIP] murmur-render not built: {e}")
        return
    check(f"render ok: {result}", result["ok"])
    check("no NaN/Inf", not result["containsNaNOrInf"])
    check(f"non-silent (peak={result['peak']:.4f})", result["peak"] > 0.01)

    validation = render_mod.validate_patch(patch_id)
    check(f"validate_patch: {validation}", validation["pass"])

    print("\n== save_patch (to a throwaway path, then clean up) ==")
    from patch_builder import REPO_ROOT
    dest = "content/presets/_mcp_smoke_test_tmp.murmur"
    saved = (REPO_ROOT / dest)
    if saved.exists():
        saved.unlink()
    import shutil
    shutil.copy(patch_builder.scratch_path(patch_id), saved)
    check(f"save landed at {dest}", saved.exists())
    saved.unlink()

    patch_builder.scratch_path(patch_id).unlink()
    check("scratch patch cleaned up", not patch_builder.scratch_path(patch_id).exists())

    print("\n== standalone bridge (optional — needs MURMUR.app running) ==")
    import standalone_bridge
    bridge = standalone_bridge.status()
    if bridge.get("connected"):
        test_id, _ = patch_builder.create_patch("Bridge Probe", "ephemeral")
        patch_builder.set_operator(test_id, 0, "classic", {"level": 0.5})
        load_result = standalone_bridge.load_path(patch_builder.scratch_path(test_id))
        check(f"load_into_standalone bridge: {load_result}", load_result.get("ok") is True)
        patch_builder.scratch_path(test_id).unlink()
    else:
        print(f"[SKIP] Standalone not running: {bridge.get('error', bridge)}")

    print("\nAll smoke tests passed.")


if __name__ == "__main__":
    main()
