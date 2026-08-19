#!/usr/bin/env python3
"""Generates a synthesized single-cycle-per-frame WAV source for
murmur-wavetable-builder, and (for convenience) invokes the builder to produce a real
factory wavetable table.

Produces content/wavetables/basic_harmonic_source.wav: 4 frames of 2048 samples,
morphing from a pure sine (frame 0) to a harmonically rich sawtooth-ish wave built
from the first 40 harmonics (frame 3) -- deliberately harmonic-rich so mip-mapping
has real content to band-limit.

Run from the repo root:
    python3 scripts/generate_test_wavetable_source.py
    ./build/dev/tools/murmur-wavetable-builder \\
        --input content/wavetables/basic_harmonic_source.wav \\
        --output content/wavetables/basic_harmonic.json \\
        --frames 4 --samples-per-frame 2048 --mip-levels 10
"""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from _wavetable_synth import additive_frame, write_wavetable_source  # noqa: E402

SAMPLES_PER_FRAME = 2048
NUM_FRAMES = 4
MAX_HARMONICS = 40

OUT_DIR = pathlib.Path(__file__).resolve().parent.parent / "content" / "wavetables"


def make_frame(harmonic_mix: float) -> list[float]:
    """harmonic_mix in [0, 1]: 0 == pure sine, 1 == full odd+even harmonic stack
    (sawtooth-like, with amplitude 1/k per harmonic k) up to MAX_HARMONICS."""
    num_harmonics = 1 + round(harmonic_mix * (MAX_HARMONICS - 1))
    amps = [1.0 if k == 1 else (harmonic_mix / k) for k in range(1, num_harmonics + 1)]
    return additive_frame(SAMPLES_PER_FRAME, amps)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames = [make_frame(frame_idx / (NUM_FRAMES - 1)) for frame_idx in range(NUM_FRAMES)]

    out_path = OUT_DIR / "basic_harmonic_source.wav"
    write_wavetable_source(out_path, frames)
    print(f"wrote {out_path} ({NUM_FRAMES} frames x {SAMPLES_PER_FRAME} samples, "
          f"up to {MAX_HARMONICS} harmonics)")


if __name__ == "__main__":
    main()
