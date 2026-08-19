#!/usr/bin/env python3
"""Render .pw8 patches and publish a Patchforge catalog + preview assets.

Reads a manifest (scripts/patchforge/manifests/*.json), renders each patch
through murmur-render, computes waveform peaks + quality scores, and writes:

  <out>/catalog.generated.json
  <out>/audio/ingest/<pack-slug>/<patch-id>.wav

Run from repo root after a dev build:

  python3 scripts/patchforge_ingest.py \\
    --manifest scripts/patchforge/manifests/mvp.json \\
    --out ../Downloads/patchforge/public

See docs/PATCHFORGE_INGEST.md
"""
from __future__ import annotations

import argparse
import json
import math
import struct
import subprocess
import sys
import tempfile
from datetime import date
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "mcp_server"))
from midi_writer import write_chord_midi  # noqa: E402

RENDERER = REPO_ROOT / "build" / "dev" / "tools" / "murmur-render"

CATEGORY_TYPE = {
    "bass": "Bass",
    "pad": "Pad",
    "lead": "Lead",
    "sequence": "Sequence",
    "ambient": "Texture",
    "fx": "FX",
    "keys": "Keys",
    "pluck": "Pluck",
}

DEFAULT_NOTES = {
    "bass": [36],
    "pad": [48, 55, 60],
    "lead": [60, 72],
    "sequence": [60],
    "ambient": [48, 55, 60],
}


def load_patch_meta(pw8_path: Path) -> dict:
    data = json.loads(pw8_path.read_text())
    return data.get("metadata", {})


def preview_notes(meta: dict, override: list[int] | None) -> list[int]:
    if override:
        return override
    cat = (meta.get("category") or "pad").lower()
    return DEFAULT_NOTES.get(cat, [60])


def render_patch(pw8_path: Path, notes: list[int], wav_out: Path, hold: float = 1.6) -> dict:
    with tempfile.TemporaryDirectory() as tmp:
        midi_path = Path(tmp) / "preview.mid"
        receipt_path = Path(tmp) / "receipt.json"
        write_chord_midi(midi_path, notes, velocity=92, hold_seconds=hold)

        result = subprocess.run(
            [
                str(RENDERER),
                "--patch",
                str(pw8_path),
                "--midi",
                str(midi_path),
                "--output",
                str(wav_out),
                "--receipt",
                str(receipt_path),
                "--release-tail",
                "1.2",
            ],
            capture_output=True,
            text=True,
            timeout=120,
            cwd=REPO_ROOT,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr.strip() or result.stdout.strip() or "render failed")

        receipt = json.loads(receipt_path.read_text())
        metrics = receipt["metrics"]
        return {
            "durationSeconds": receipt["duration"],
            "peak": float(metrics["peak"]),
            "rms": float(metrics["rms"]),
            "containsNaNOrInf": bool(metrics["containsNaNOrInf"]),
        }


def read_wav_mono_samples(wav_path: Path) -> list[float]:
    data = wav_path.read_bytes()
    if len(data) < 12 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"not a RIFF/WAVE file: {wav_path}")

    offset = 12
    fmt_tag: int | None = None
    n_channels = 1
    audio_data: bytes | None = None

    while offset + 8 <= len(data):
        chunk_id = data[offset : offset + 4]
        chunk_size = struct.unpack("<I", data[offset + 4 : offset + 8])[0]
        chunk_start = offset + 8
        chunk_end = min(len(data), chunk_start + chunk_size)
        chunk = data[chunk_start:chunk_end]

        if chunk_id == b"fmt " and len(chunk) >= 16:
            fmt_tag = struct.unpack("<H", chunk[0:2])[0]
            n_channels = struct.unpack("<H", chunk[2:4])[0]
        elif chunk_id == b"data":
            audio_data = chunk

        offset = chunk_end + (chunk_size % 2)

    if fmt_tag is None or audio_data is None:
        raise ValueError(f"missing fmt/data chunks: {wav_path}")

    if fmt_tag == 3:
        count = len(audio_data) // 4
        interleaved = struct.unpack(f"<{count}f", audio_data[: count * 4])
    elif fmt_tag == 1:
        count = len(audio_data) // 2
        interleaved = [s / 32768.0 for s in struct.unpack(f"<{count}h", audio_data[: count * 2])]
    else:
        raise ValueError(f"unsupported WAV format tag {fmt_tag}: {wav_path}")

    if n_channels <= 1:
        return list(interleaved)

    mono: list[float] = []
    for i in range(0, len(interleaved), n_channels):
        block = interleaved[i : i + n_channels]
        mono.append(sum(block) / len(block))
    return mono


def waveform_peaks(wav_path: Path, bins: int = 70) -> list[float]:
    mono = read_wav_mono_samples(wav_path)
    if not mono:
        return [0.0] * bins

    chunk = max(1, len(mono) // bins)
    peaks: list[float] = []
    max_peak = 1e-9
    for i in range(bins):
        start = i * chunk
        end = min(len(mono), start + chunk)
        block = mono[start:end]
        peak = max(abs(s) for s in block) if block else 0.0
        peaks.append(peak)
        max_peak = max(max_peak, peak)

    return [min(1.0, p / max_peak) for p in peaks]


def quality_score(metrics: dict, validation_notes: list[dict]) -> int:
    score = 100.0
    if metrics.get("containsNaNOrInf"):
        score -= 50
    peak = float(metrics.get("peak", 0.0))
    rms = float(metrics.get("rms", 0.0))
    if peak > 0.98:
        score -= 18
    elif peak > 0.92:
        score -= 6
    if peak < 0.005:
        score -= 25
    elif peak < 0.02:
        score -= 10
    if rms < 0.008:
        score -= 12
    elif rms > 0.45:
        score -= 8

    for probe in validation_notes:
        p = float(probe.get("peak", 0.0))
        if probe.get("containsNaNOrInf"):
            score -= 20
        if p < 0.005:
            score -= 8
        if p > 0.98:
            score -= 8

    return int(max(0, min(100, round(score))))


def validate_multi_note(pw8_path: Path) -> list[dict]:
    probes = []
    for notes in ([36], [60], [84]):
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
            wav = Path(tmp.name)
        try:
            probes.append(render_patch(pw8_path, notes, wav, hold=0.9))
        finally:
            wav.unlink(missing_ok=True)
    return probes


def ingest_pack(pack_def: dict, out_root: Path, validate: bool) -> dict:
    slug = pack_def["slug"]
    audio_dir = out_root / "audio" / "ingest" / slug
    audio_dir.mkdir(parents=True, exist_ok=True)

    patches_out = []
    patch_scores: list[int] = []

    for i, patch_def in enumerate(pack_def["patches"]):
        pw8_rel = patch_def["pw8"]
        pw8_path = REPO_ROOT / pw8_rel
        if not pw8_path.is_file():
            raise FileNotFoundError(pw8_rel)

        meta = load_patch_meta(pw8_path)
        patch_id = meta.get("id") or f"{slug}-{i + 1}"
        safe_name = patch_id.replace("/", "-").lower()
        wav_name = f"{safe_name}.wav"
        wav_path = audio_dir / wav_name

        notes = preview_notes(meta, patch_def.get("notes"))
        print(f"  render {meta.get('name', pw8_path.name)}  notes={notes}")
        metrics = render_patch(pw8_path, notes, wav_path)

        validation = validate_multi_note(pw8_path) if validate else []
        score = quality_score(metrics, validation)
        patch_scores.append(score)

        cat = (meta.get("category") or "pad").lower()
        patches_out.append(
            {
                "id": patch_id,
                "name": meta.get("name") or pw8_path.stem.upper(),
                "type": CATEGORY_TYPE.get(cat, "Patch"),
                "key": patch_def.get("key"),
                "bpm": patch_def.get("bpm"),
                "audioSrc": f"/audio/ingest/{slug}/{wav_name}",
                "waveform": waveform_peaks(wav_path),
                "qualityScore": score,
                "pw8Path": pw8_rel,
                "peak": round(metrics["peak"], 6),
                "rms": round(metrics["rms"], 6),
            }
        )

    pack_score = int(round(sum(patch_scores) / len(patch_scores))) if patch_scores else 0

    return {
        "id": pack_def["id"],
        "slug": slug,
        "title": pack_def["title"],
        "shortTitle": pack_def["shortTitle"],
        "description": pack_def["description"],
        "genre": pack_def["genre"],
        "mood": pack_def["mood"],
        "synth": "STARFIGHTER",
        "price": pack_def.get("price", 14.99),
        "patchCount": len(patches_out),
        "releaseDate": pack_def.get("releaseDate") or date.today().isoformat(),
        "badge": pack_def.get("badge"),
        "qualityScore": pack_score,
        "palette": pack_def.get("palette") or ["#5ecfff", "#7c3aed", "#0b0c0f"],
        "patches": patches_out,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Ingest .pw8 packs into Patchforge catalog JSON + WAV previews")
    parser.add_argument("--manifest", type=Path, required=True, help="Pack manifest JSON")
    parser.add_argument("--out", type=Path, required=True, help="Patchforge public/ directory")
    parser.add_argument("--no-validate", action="store_true", help="Skip low/mid/high validation renders (faster)")
    args = parser.parse_args()

    if not RENDERER.is_file():
        print(f"ERROR: murmur-render not found at {RENDERER}\n  Run: cmake --build --preset dev", file=sys.stderr)
        return 1

    manifest = json.loads(args.manifest.read_text())
    out_root = args.out.resolve()
    out_root.mkdir(parents=True, exist_ok=True)

    catalog = {
        "version": manifest.get("version", 1),
        "generatedAt": date.today().isoformat(),
        "engine": "murmur",
        "renderer": str(RENDERER.relative_to(REPO_ROOT)),
        "packs": [],
    }

    for pack_def in manifest.get("packs", []):
        print(f"\n== pack: {pack_def['title']}")
        catalog["packs"].append(ingest_pack(pack_def, out_root, validate=not args.no_validate))

    catalog_path = out_root / "catalog.generated.json"
    catalog_path.write_text(json.dumps(catalog, indent=2) + "\n")
    print(f"\nWrote {catalog_path} ({len(catalog['packs'])} packs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
