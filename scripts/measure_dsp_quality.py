#!/usr/bin/env python3
"""THD+N / aliasing smoke measurement for Patchwork Eight offline renders.

Usage (from repo root, after building pw8-render):
  python3 scripts/measure_dsp_quality.py --patch content/presets/init-saw.pw8
  python3 scripts/measure_dsp_quality.py --patch content/presets/fm-bell.pw8 --tone-hz 440

Renders a short deterministic clip through pw8-render, loads the WAV, and reports:
  - RMS level
  - crude THD estimate (energy in harmonics 2-10 vs fundamental)
  - energy above Nyquist/2 as a simple aliasing proxy

Non-blocking developer tool -- not part of CI. Requires numpy (and scipy optional).
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RENDER = REPO_ROOT / "build/dev/tools/pw8-render"
DEFAULT_MIDI = REPO_ROOT / "content/test_midi/single_c4.mid"


def ensure_test_midi(path: Path) -> None:
    if path.is_file():
        return
    gen = REPO_ROOT / "scripts/generate_test_midi.py"
    if gen.is_file():
        subprocess.run([sys.executable, str(gen)], check=True, cwd=REPO_ROOT)
    if not path.is_file():
        raise FileNotFoundError(f"Missing MIDI fixture: {path} (run scripts/generate_test_midi.py)")


def read_wav_mono(path: Path):
    try:
        import numpy as np
    except ImportError:
        print("numpy is required: pip install numpy", file=sys.stderr)
        raise SystemExit(1)

    with wave.open(str(path), "rb") as wf:
        nchannels = wf.getnchannels()
        rate = wf.getframerate()
        nframes = wf.getnframes()
        sampwidth = wf.getsampwidth()
        if sampwidth != 2:
            raise ValueError(f"expected 16-bit WAV, got sampwidth={sampwidth}")
        raw = wf.readframes(nframes)

    data = np.frombuffer(raw, dtype=np.int16).astype(np.float64) / 32768.0
    if nchannels > 1:
        data = data.reshape(-1, nchannels).mean(axis=1)
    return data, rate


def analyze_tone(samples, sample_rate: float, fundamental_hz: float) -> dict:
    import numpy as np

    n = 1
    while n * 2 <= len(samples):
        n *= 2
    x = samples[:n]
    window = np.hanning(n)
    spec = np.fft.rfft(x * window)
    mag2 = (np.abs(spec) ** 2) / max(np.sum(window**2), 1e-12)

    freqs = np.fft.rfftfreq(n, d=1.0 / sample_rate)
    fund_bin = int(round(fundamental_hz * n / sample_rate))
    fund_bin = max(1, min(fund_bin, len(mag2) - 1))

    fund_energy = mag2[fund_bin]
    harmonic_energy = 0.0
    for h in range(2, 11):
        b = fund_bin * h
        if b >= len(mag2):
            break
        harmonic_energy += mag2[b]

    nyquist = sample_rate * 0.5
    alias_mask = freqs > nyquist * 0.5
    alias_energy = float(np.sum(mag2[alias_mask]))

    rms = float(np.sqrt(np.mean(x * x)))
    thd = float(np.sqrt(harmonic_energy / max(fund_energy, 1e-18)))

    return {
        "rms": rms,
        "fundamentalHz": fundamental_hz,
        "thdEstimate": thd,
        "aliasEnergyAboveHalfNyquist": alias_energy,
        "numSamples": n,
    }


def render_patch(render_bin: Path, patch: Path, midi: Path, duration: float, seed: int) -> Path:
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
        out = Path(tmp.name)

    cmd = [
        str(render_bin),
        "--patch",
        str(patch),
        "--midi",
        str(midi),
        "--output",
        str(out),
        "--sample-rate",
        "48000",
        "--duration",
        str(duration),
        "--seed",
        str(seed),
    ]
    subprocess.run(cmd, check=True, cwd=REPO_ROOT)
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--patch", required=True, help="Path to .pw8 patch (repo-relative or absolute)")
    parser.add_argument("--render-bin", default=str(DEFAULT_RENDER))
    parser.add_argument("--midi", default=str(DEFAULT_MIDI))
    parser.add_argument("--duration", type=float, default=1.5)
    parser.add_argument("--seed", type=int, default=0x507738)
    parser.add_argument("--tone-hz", type=float, default=261.6256, help="Expected fundamental for THD estimate")
    args = parser.parse_args()

    render_bin = Path(args.render_bin)
    if not render_bin.is_file():
        print(f"Missing renderer: {render_bin}", file=sys.stderr)
        return 1

    patch = Path(args.patch)
    if not patch.is_absolute():
        patch = REPO_ROOT / patch
    midi = Path(args.midi)
    if not midi.is_absolute():
        midi = REPO_ROOT / midi
    ensure_test_midi(midi)
    if not patch.is_file() or not midi.is_file():
        print("Patch or MIDI file not found", file=sys.stderr)
        return 1

    wav_path = render_patch(render_bin, patch, midi, args.duration, args.seed)
    try:
        samples, rate = read_wav_mono(wav_path)
        metrics = analyze_tone(samples, rate, args.tone_hz)
    finally:
        wav_path.unlink(missing_ok=True)

    print(f"patch: {patch.name}")
    print(f"rms: {metrics['rms']:.6f}")
    print(f"thd_estimate: {metrics['thdEstimate']:.6f}")
    print(f"alias_energy_above_half_nyquist: {metrics['aliasEnergyAboveHalfNyquist']:.6e}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
