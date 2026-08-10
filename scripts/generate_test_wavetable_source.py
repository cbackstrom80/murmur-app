#!/usr/bin/env python3
"""Generates a synthesized single-cycle-per-frame WAV source for
pw8-wavetable-builder, and (for convenience) invokes the builder to produce a real
factory wavetable table.

Produces content/wavetables/basic_harmonic_source.wav: 4 frames of 2048 samples,
morphing from a pure sine (frame 0) to a harmonically rich sawtooth-ish wave built
from the first 40 harmonics (frame 3) -- deliberately harmonic-rich so mip-mapping
has real content to band-limit.

Run from the repo root:
    python3 scripts/generate_test_wavetable_source.py
    ./build/dev/tools/pw8-wavetable-builder \\
        --input content/wavetables/basic_harmonic_source.wav \\
        --output content/wavetables/basic_harmonic.json \\
        --frames 4 --samples-per-frame 2048 --mip-levels 10
"""
import math
import pathlib
import struct

SAMPLES_PER_FRAME = 2048
NUM_FRAMES = 4
MAX_HARMONICS = 40

OUT_DIR = pathlib.Path(__file__).resolve().parent.parent / "content" / "wavetables"


def make_frame(harmonic_mix: float) -> list[float]:
    """harmonic_mix in [0, 1]: 0 == pure sine, 1 == full odd+even harmonic stack
    (sawtooth-like, with amplitude 1/k per harmonic k) up to MAX_HARMONICS."""
    frame = [0.0] * SAMPLES_PER_FRAME
    num_harmonics = 1 + round(harmonic_mix * (MAX_HARMONICS - 1))
    for k in range(1, num_harmonics + 1):
        amp = 1.0 if k == 1 else (harmonic_mix / k)
        for i in range(SAMPLES_PER_FRAME):
            phase = 2.0 * math.pi * k * i / SAMPLES_PER_FRAME
            frame[i] += amp * math.sin(phase)
    peak = max(abs(x) for x in frame) or 1.0
    return [x / peak * 0.95 for x in frame]


def write_wav(path: pathlib.Path, samples: list[float], sample_rate: int = 48000) -> None:
    pcm = b"".join(struct.pack("<h", max(-32768, min(32767, round(s * 32767)))) for s in samples)
    data_size = len(pcm)
    with open(path, "wb") as f:
        f.write(b"RIFF")
        f.write(struct.pack("<I", 36 + data_size))
        f.write(b"WAVE")
        f.write(b"fmt ")
        f.write(struct.pack("<IHHIIHH", 16, 1, 1, sample_rate, sample_rate * 2, 2, 16))
        f.write(b"data")
        f.write(struct.pack("<I", data_size))
        f.write(pcm)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    all_samples: list[float] = []
    for frame_idx in range(NUM_FRAMES):
        mix = frame_idx / (NUM_FRAMES - 1)
        all_samples.extend(make_frame(mix))

    out_path = OUT_DIR / "basic_harmonic_source.wav"
    write_wav(out_path, all_samples)
    print(f"wrote {out_path} ({NUM_FRAMES} frames x {SAMPLES_PER_FRAME} samples, "
          f"up to {MAX_HARMONICS} harmonics)")


if __name__ == "__main__":
    main()
