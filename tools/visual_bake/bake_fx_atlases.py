#!/usr/bin/env python3
"""Generate FX atlas PNGs + manifest for MURMUR CPU visuals (BinaryData embedding)."""

from __future__ import annotations

import argparse
import json
import math
import struct
import zlib
from pathlib import Path


def _png_chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def write_png(path: Path, width: int, height: int, rgba_rows: list[bytes]) -> None:
    assert len(rgba_rows) == height
    raw = b"".join(b"\x00" + row for row in rgba_rows)
    compressed = zlib.compress(raw, 9)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + _png_chunk(b"IHDR", ihdr) + _png_chunk(b"IDAT", compressed) + _png_chunk(b"IEND", b"")
    path.write_bytes(png)


def bake_chorus(frame_w: int, frame_h: int, frames: int) -> list[bytes]:
    rows: list[bytes] = []
    for y in range(frame_h):
        row = bytearray(frame_w * frames * 4)
        for x in range(frame_w * frames):
            fx = x % frame_w
            fi = x // frame_w
            phase = (fi / max(1, frames - 1)) * math.tau
            wave = math.sin(fx / frame_w * math.tau * 2 + phase)
            v = int(128 + wave * 90)
            alpha = 255 if 8 < y < frame_h - 8 else 0
            i = x * 4
            row[i : i + 4] = bytes([20, v, 180, alpha])
        rows.append(bytes(row))
    return rows


def bake_tape(frame_w: int, frame_h: int, frames: int) -> list[bytes]:
    rows: list[bytes] = []
    mid = frame_h // 2
    for y in range(frame_h):
        row = bytearray(frame_w * frames * 4)
        for x in range(frame_w * frames):
            fx = x % frame_w
            fi = x // frame_w
            phase = (fi / max(1, frames - 1)) * math.tau
            wobble = math.sin(fx / frame_w * math.tau * 3 + phase) * frame_h * 0.12
            py = mid + wobble
            dist = abs(y - py)
            alpha = max(0, 255 - int(dist * 40))
            i = x * 4
            row[i : i + 4] = bytes([232, 163, 61, alpha])
        rows.append(bytes(row))
    return rows


def bake_reverb(frame_w: int, frame_h: int, frames: int) -> list[bytes]:
    rows: list[bytes] = []
    for y in range(frame_h):
        row = bytearray(frame_w * frames * 4)
        for x in range(frame_w * frames):
            fx = x % frame_w
            fi = x // frame_w
            t = fx / frame_w
            sweep = fi / max(1, frames - 1)
            amp = math.exp(-t * 4.5) * (1.0 + 0.08 * math.sin(sweep * math.tau + t * 8))
            py = frame_h - amp * frame_h * 0.88
            dist = abs(y - py)
            alpha = max(0, 255 - int(dist * 35))
            i = x * 4
            row[i : i + 4] = bytes([0, 255, 208, alpha])
        rows.append(bytes(row))
    return rows


def bake_clouds(frame_w: int, frame_h: int, frames: int) -> list[bytes]:
    rows: list[bytes] = []
    rng_state = 0xC10D5
    particles: list[tuple[float, float, float, int]] = []
    for _ in range(28):
        rng_state = (rng_state * 1103515245 + 12345) & 0x7FFFFFFF
        px = (rng_state % 1000) / 1000.0
        rng_state = (rng_state * 1103515245 + 12345) & 0x7FFFFFFF
        py = (rng_state % 1000) / 1000.0
        rng_state = (rng_state * 1103515245 + 12345) & 0x7FFFFFFF
        pr = 1.2 + (rng_state % 1000) / 1000.0 * 2.2
        colour = 0 if _ % 3 == 0 else 1
        particles.append((px, py, pr, colour))

    for y in range(frame_h):
        row = bytearray(frame_w * frames * 4)
        for x in range(frame_w * frames):
            fx = x % frame_w
            fi = x // frame_w
            drift = fi / max(1, frames - 1)
            alpha = 0
            r, g, b = 0, 0, 0
            for px, py, pr, colour in particles:
                cx = (px + drift * 0.35) % 1.0 * frame_w
                cy = py * frame_h
                dist = math.hypot(fx - cx, y - cy)
                if dist <= pr:
                    a = int(180 * (1.0 - dist / pr))
                    if a > alpha:
                        alpha = a
                        if colour == 0:
                            r, g, b = 0, 255, 208
                        else:
                            r, g, b = 232, 163, 61
            i = x * 4
            row[i : i + 4] = bytes([r, g, b, alpha])
        rows.append(bytes(row))
    return rows


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=Path("plugin/resources/fx_atlases"))
    parser.add_argument("--frames", type=int, default=24)
    parser.add_argument("--frame-w", type=int, default=128)
    parser.add_argument("--frame-h", type=int, default=64)
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    manifest = {"frames": args.frames, "frameWidth": args.frame_w, "frameHeight": args.frame_h, "atlases": []}

    bakers = [
        ("chorus", bake_chorus),
        ("tape", bake_tape),
        ("reverb", bake_reverb),
        ("clouds", bake_clouds),
    ]

    for name, baker in bakers:
        atlas_w = args.frame_w * args.frames
        rows = baker(args.frame_w, args.frame_h, args.frames)
        out = args.out / f"{name}_atlas.png"
        write_png(out, atlas_w, args.frame_h, rows)
        manifest["atlases"].append({"name": name, "file": out.name})

    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=2))
    print(f"Wrote {len(manifest['atlases'])} atlases to {args.out}")


if __name__ == "__main__":
    main()
