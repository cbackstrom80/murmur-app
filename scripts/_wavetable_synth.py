"""Shared wavetable-source synthesis helpers, used by both
generate_test_wavetable_source.py (the original single-table example) and
generate_wavetable_library.py (the 50-table factory library).

Every recipe function here returns exactly one frame's samples as a plain
list[float], sized `samples_per_frame`. The one invariant every recipe MUST
preserve: each frame has to be an exactly periodic waveform over its own
length, because pw8-wavetable-builder treats a frame as one full FFT cycle
(bin k IS harmonic k) and the runtime oscillator loops each frame directly.
Concretely, that means:
  - Any harmonic/partial content must sit at an INTEGER multiple of the
    frame's fundamental (1 cycle per frame) -- never a non-integer ratio.
    A non-integer partial would both click at the frame's loop point (the
    waveform wouldn't return to its start value) and smear energy across
    every FFT bin once the builder band-limits it for the lower mip levels,
    instead of cleanly preserving the intended partial. This is why the
    "metallic/bell" recipes below use widely, unevenly SPACED integer
    harmonics (e.g. 1, 3, 7, 12, 19) rather than true non-integer FM-style
    ratios (e.g. 1, 3.5, 5.17) -- inharmonic-sounding, but still click-free
    and FFT-truncation-clean.
  - Pointwise nonlinearities (fold/quantize/Chebyshev) applied to an
    already-periodic base signal stay periodic automatically -- a
    nonlinearity only adds harmonics, it can't introduce a fractional one.
"""
import math
import struct


def write_wav(path, samples: list[float], sample_rate: int = 48000) -> None:
    """Writes mono 16-bit PCM WAV -- identical to the original
    generate_test_wavetable_source.py implementation this was extracted from."""
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


def write_wavetable_source(path, frames: list[list[float]], sample_rate: int = 48000) -> None:
    """Flattens `frames` (one wavetable's worth) and writes them as a single
    source WAV, the layout pw8-wavetable-builder expects (--frames N reads N
    consecutive --samples-per-frame chunks)."""
    all_samples: list[float] = []
    for frame in frames:
        all_samples.extend(frame)
    write_wav(path, all_samples, sample_rate)


def normalize(frame: list[float], peak_target: float = 0.95) -> list[float]:
    peak = max((abs(x) for x in frame), default=0.0) or 1.0
    return [x / peak * peak_target for x in frame]


def mix_frames(a: list[float], b: list[float], mix: float) -> list[float]:
    """Elementwise crossfade between two already-periodic, same-length frames
    (mixing preserves periodicity -- a linear combination of two periodic
    signals with the same period is itself periodic with that period),
    renormalized afterward."""
    mix = max(0.0, min(1.0, mix))
    return normalize([x + (y - x) * mix for x, y in zip(a, b)])


def additive_frame(samples_per_frame: int, harmonic_amps: list[float],
                    harmonic_phases: list[float] | None = None, peak_target: float = 0.95) -> list[float]:
    """harmonic_amps[i] is the amplitude of harmonic (i+1) -- i.e. 1-indexed
    harmonics stored at 0-indexed list positions. Generalizes the original
    script's make_frame() to an arbitrary harmonic profile, not just the
    sine-to-saw interpolation it was hardcoded to."""
    n = samples_per_frame
    phases = harmonic_phases if harmonic_phases is not None else [0.0] * len(harmonic_amps)
    frame = [0.0] * n
    for idx, amp in enumerate(harmonic_amps):
        if amp == 0.0:
            continue
        k = idx + 1
        ph = phases[idx]
        for i in range(n):
            frame[i] += amp * math.sin(2.0 * math.pi * k * i / n + ph)
    return normalize(frame, peak_target)


def sparse_harmonics_frame(samples_per_frame: int, harmonic_amps: dict[int, float],
                            peak_target: float = 0.95) -> list[float]:
    """Convenience over additive_frame for a sparse (harmonic_number -> amp)
    mapping -- used by the metallic/bell recipes, where most harmonics are
    silent and only a handful of widely-spaced ones carry energy."""
    max_h = max(harmonic_amps)
    amps = [harmonic_amps.get(k, 0.0) for k in range(1, max_h + 1)]
    return additive_frame(samples_per_frame, amps, peak_target=peak_target)


def pulse_frame(samples_per_frame: int, duty: float, num_harmonics: int = 60) -> list[float]:
    """Analytic band-limited pulse wave at the given duty cycle (0..1,
    clamped away from the edges where the series degenerates to silence)."""
    duty = max(0.02, min(0.98, duty))
    amps = [(2.0 / (k * math.pi)) * math.sin(k * math.pi * duty) for k in range(1, num_harmonics + 1)]
    return additive_frame(samples_per_frame, amps)


def formant_frame(samples_per_frame: int, formants: list[tuple[float, float, float]],
                   num_harmonics: int = 60) -> list[float]:
    """formants: list of (center_harmonic, bandwidth_harmonics, amplitude)
    Gaussian bumps in harmonic-number space -- a stylized, not acoustically
    precise, approximation of vowel formants (there's no fixed fundamental
    frequency to place true Hz-based formants against in a wavetable)."""
    amps = [0.0] * num_harmonics
    for k in range(1, num_harmonics + 1):
        amp = 0.0
        for center, bandwidth, formant_amp in formants:
            amp += formant_amp * math.exp(-((k - center) ** 2) / (2.0 * bandwidth ** 2))
        amps[k - 1] = amp
    return additive_frame(samples_per_frame, amps)


def chebyshev_frame(samples_per_frame: int, coeffs: list[float]) -> list[float]:
    """Classic controlled-harmonic waveshaping: y = sum(coeffs[k] * T_k(x)),
    x = sin(2*pi*i/N) (already periodic), evaluated via the standard
    Chebyshev recurrence T_0=1, T_1=x, T_k=2x*T_(k-1) - T_(k-2). Because the
    base x is periodic, every T_k(x) -- and their weighted sum -- stays
    periodic too; a pointwise nonlinearity can only add harmonics, not
    fractional ones."""
    n = samples_per_frame
    frame = [0.0] * n
    for i in range(n):
        x = math.sin(2.0 * math.pi * i / n)
        t_prev2, t_prev1 = 1.0, x
        y = coeffs[0] * t_prev2 if len(coeffs) > 0 else 0.0
        if len(coeffs) > 1:
            y += coeffs[1] * t_prev1
        for k in range(2, len(coeffs)):
            t_k = 2.0 * x * t_prev1 - t_prev2
            y += coeffs[k] * t_k
            t_prev2, t_prev1 = t_prev1, t_k
        frame[i] = y
    return normalize(frame)


def fold_frame(base: list[float], amount: float) -> list[float]:
    """Smooth triangle-style wavefolding via asin(sin(...)); `amount` in
    [0, 1] increases fold density. Pointwise on an already-periodic `base`,
    so stays periodic -- always bounded in [-1, 1], no iteration needed."""
    drive = 1.0 + 3.0 * max(0.0, min(1.0, amount))
    folded = [math.asin(math.sin(x * (math.pi / 2.0) * drive)) / (math.pi / 2.0) for x in base]
    return normalize(folded)


def quantize_frame(base: list[float], levels: int) -> list[float]:
    """Bit-reduction-style amplitude quantization to `levels` steps (pointwise
    on an already-periodic base, stays periodic)."""
    half = max(1, levels // 2)
    quantized = [max(-1.0, min(1.0, round(x * half) / half)) for x in base]
    return normalize(quantized)


def noise_spectrum_frame(samples_per_frame: int, seed: int, brightness: float, num_harmonics: int = 60) -> list[float]:
    """Deterministic pseudo-random harmonic amplitude+phase spectrum -- seeded,
    not real randomness, matching the rest of this codebase's "no
    non-deterministic generated content" convention (e.g.
    AlgorithmGraphView's positional-hash background texture).
    `brightness` in [0, 1] biases energy toward higher harmonics."""
    # A small deterministic hash-based PRNG (avoids importing `random` and its
    # global-state/seed-reproducibility-across-versions surface for something
    # this simple) -- xorshift32, seeded per call.
    state = (seed * 2654435761 + 1) & 0xFFFFFFFF or 0x9E3779B9

    def next_unit() -> float:
        nonlocal state
        state ^= (state << 13) & 0xFFFFFFFF
        state ^= (state >> 17)
        state ^= (state << 5) & 0xFFFFFFFF
        state &= 0xFFFFFFFF
        return state / 0xFFFFFFFF

    amps = []
    phases = []
    for k in range(1, num_harmonics + 1):
        tilt = (k / num_harmonics) ** (1.0 - brightness)  # brightness=1 -> flat; brightness=0 -> steep rolloff.
        amps.append(next_unit() * tilt)
        phases.append(next_unit() * 2.0 * math.pi)
    return additive_frame(samples_per_frame, amps, phases)
