"""Renders a scratch patch through the real engine via the murmur-render CLI
(tools/render/main.cpp) -- no pybind11/live-Engine dependency, matching
docs/MCP_AND_NL_PATCH_GENERATION.md Part A's "reuse the CLI tools for
one-shot renders" design. Same real DSP every other verification pass in
this project uses, not a synthetic check."""
from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

from midi_writer import write_chord_midi
from patch_builder import REPO_ROOT, load_scratch, scratch_path

RENDERER = REPO_ROOT / "build" / "dev" / "tools" / "murmur-render"
RENDER_OUT_DIR = Path(__file__).resolve().parent / "scratch" / "renders"
RENDER_OUT_DIR.mkdir(parents=True, exist_ok=True)


class RendererMissing(RuntimeError):
    pass


def render_preview(patch_id: str, notes: list[int] | None = None, hold_seconds: float = 1.5,
                    velocity: int = 90) -> dict:
    if not RENDERER.exists():
        raise RendererMissing(
            f"murmur-render not built at {RENDERER} -- run: cmake --build --preset dev"
        )
    patch_path = scratch_path(patch_id)
    if not patch_path.exists():
        load_scratch(patch_id)  # raises a clear FileNotFoundError

    notes = notes or [60]
    with tempfile.TemporaryDirectory() as tmp:
        midi_path = Path(tmp) / "preview.mid"
        write_chord_midi(midi_path, notes, velocity=velocity, hold_seconds=hold_seconds)

        wav_path = RENDER_OUT_DIR / f"{patch_path.stem}.wav"
        receipt_path = Path(tmp) / "receipt.json"

        result = subprocess.run(
            [str(RENDERER), "--patch", str(patch_path), "--midi", str(midi_path),
             "--output", str(wav_path), "--receipt", str(receipt_path), "--release-tail", "1.0"],
            capture_output=True, text=True, timeout=30, cwd=REPO_ROOT,
            # cwd matters: OperatorPatch::wavetableId is resolved relative to the
            # render process's working directory (engine/src/render/Engine.cpp),
            # and every patch in this repo uses repo-relative paths like
            # "content/wavetables/...". An MCP client can launch server.py from
            # anywhere, so this can't rely on inheriting the caller's cwd.
        )
        if result.returncode != 0:
            return {"ok": False, "error": result.stderr.strip() or result.stdout.strip()}

        receipt = json.loads(receipt_path.read_text())
        metrics = receipt["metrics"]
        return {
            "ok": True,
            "wavPath": str(wav_path),
            "durationSeconds": receipt["duration"],
            "peak": metrics["peak"],
            "rms": metrics["rms"],
            "containsNaNOrInf": metrics["containsNaNOrInf"],
        }


def validate_patch(patch_id: str) -> dict:
    """A quick pass/fail: renders a low note and a high note, checks for
    NaN/Inf and for suspiciously silent (<0.005 peak) or over-limiter-ceiling
    (>0.98 peak) output -- the same bar the factory preset bank's 250 patches
    were checked against before being bundled into the installer."""
    reasons = []
    for label, notes in (("low", [36]), ("mid", [60]), ("high", [84])):
        result = render_preview(patch_id, notes=notes, hold_seconds=1.0)
        if not result["ok"]:
            reasons.append(f"{label} note render failed: {result['error']}")
            continue
        if result["containsNaNOrInf"]:
            reasons.append(f"{label} note: NaN/Inf in output")
        if result["peak"] < 0.005:
            reasons.append(f"{label} note: suspiciously silent (peak={result['peak']:.4f})")
        if result["peak"] > 0.98:
            reasons.append(f"{label} note: near/at clipping (peak={result['peak']:.4f})")
    return {"pass": len(reasons) == 0, "reasons": reasons}
