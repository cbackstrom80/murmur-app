#!/usr/bin/env python3
"""Generates the 50-table factory wavetable library under content/wavetables/.

For each table: synthesizes its source WAV (content/wavetables/sources/), then
invokes murmur-wavetable-builder to produce the real mip-mapped JSON table
(content/wavetables/<name>.json). Also (re)writes content/wavetables/MANIFEST.md,
a discoverability index of all 50 tables' names/categories/mood tags/frame counts.

Every table uses the same dimensions as the existing basic_harmonic.json example
(2048 samples/frame, 10 mip levels) -- only frame count varies per table (1-16),
matching each table's role (a single static texture vs. a long evolving morph).

Requires murmur-wavetable-builder already built: `cmake --build --preset dev`.

Run from the repo root:
    python3 scripts/generate_wavetable_library.py
    python3 scripts/generate_wavetable_library.py --verify   # also render-tests every table
"""
import argparse
import json
import math
import pathlib
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from _wavetable_synth import (  # noqa: E402
    additive_frame, chebyshev_frame, fold_frame, formant_frame, mix_frames,
    noise_spectrum_frame, pulse_frame, quantize_frame, sparse_harmonics_frame,
    write_wavetable_source,
)

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
WAVETABLES_DIR = REPO_ROOT / "content" / "wavetables"
SOURCES_DIR = WAVETABLES_DIR / "sources"
BUILDER = REPO_ROOT / "build" / "dev" / "tools" / "murmur-wavetable-builder"
RENDERER = REPO_ROOT / "build" / "dev" / "tools" / "murmur-render"
SINGLE_NOTE_MIDI = REPO_ROOT / "content" / "test_midi" / "single-note.mid"

SAMPLES_PER_FRAME = 2048
MIP_LEVELS = 10


# ---------------------------------------------------------------------------
# Small composition helpers built on top of _wavetable_synth's primitives.
# ---------------------------------------------------------------------------

def morph(num_frames: int, frame_fn) -> list[list[float]]:
    """frame_fn(t, frame_index) -> one frame's samples, t in [0,1] (t=0 if
    num_frames==1). The shared "build N frames sweeping some parameter" shape
    every table below uses."""
    if num_frames == 1:
        return [frame_fn(0.0, 0)]
    return [frame_fn(i / (num_frames - 1), i) for i in range(num_frames)]


def sine_amps(n: int) -> list[float]:
    return [1.0] + [0.0] * (n - 1)


def saw_amps(n: int) -> list[float]:
    return [1.0 / k for k in range(1, n + 1)]


def square_amps(n: int) -> list[float]:
    return [(1.0 / k if k % 2 == 1 else 0.0) for k in range(1, n + 1)]


def triangle_amps(n: int) -> list[float]:
    amps = [0.0] * n
    for k in range(1, n + 1, 2):
        sign = 1.0 if ((k - 1) // 2) % 2 == 0 else -1.0
        amps[k - 1] = sign / (k * k)
    return amps


def lerp_amps(a: list[float], b: list[float], t: float) -> list[float]:
    n = max(len(a), len(b))
    a = a + [0.0] * (n - len(a))
    b = b + [0.0] * (n - len(b))
    return [a[i] + (b[i] - a[i]) * t for i in range(n)]


def lerp_formants(a, b, t):
    return [(fa[0] + (fb[0] - fa[0]) * t, fa[1] + (fb[1] - fa[1]) * t, fa[2] + (fb[2] - fa[2]) * t)
            for fa, fb in zip(a, b)]


def bell_frame(n: int, spread: float, decay: float, base_ratios=(1, 2, 3, 4.5, 6, 8.5), count: int = 6) -> list[float]:
    """Widely-spaced integer harmonics (see _wavetable_synth's module doc for
    why these must be integers, not the true non-integer ratios a real FM
    bell's partials would have) -- `spread` scales how far apart they land,
    `decay` how fast amplitude falls off with partial index."""
    harmonics = sorted({max(1, round(r * spread)) for r in base_ratios[:count]})
    amps = {h: decay ** i for i, h in enumerate(harmonics)}
    return sparse_harmonics_frame(n, amps)


def fm_bell_frame(n: int, carrier: int, mod: int, num_sidebands: int, decay: float) -> list[float]:
    """A geometric-decay approximation of 2-operator FM sidebands (carrier +/-
    k*mod, amplitude decay**|k|) -- not literally Bessel-function FM sideband
    amplitudes, close enough for a stylized "digital bell" character."""
    amps: dict[int, float] = {}
    for k in range(-num_sidebands, num_sidebands + 1):
        h = carrier + k * mod
        if h < 1:
            continue
        amps[h] = amps.get(h, 0.0) + decay ** abs(k)
    return sparse_harmonics_frame(n, amps)


# ---------------------------------------------------------------------------
# Vowel formant presets (harmonic-number space, stylized -- see formant_frame's
# own doc comment for why these aren't acoustically precise Hz values).
# ---------------------------------------------------------------------------

VOWEL_AA = [(3, 1.5, 1.0), (9, 2.5, 0.6), (20, 4.0, 0.3)]
VOWEL_EE = [(2, 1.0, 0.8), (18, 3.0, 1.0), (24, 3.0, 0.5)]
VOWEL_OH = [(2, 1.2, 1.0), (6, 2.0, 0.6), (14, 3.0, 0.3)]
VOWEL_OO = [(2, 1.0, 1.0), (4, 1.5, 0.5), (8, 2.0, 0.25)]


# ---------------------------------------------------------------------------
# The 50-table manifest. Each entry: (name, category, moods, frame-count, build-fn).
# build-fn takes samples_per_frame and returns list[list[float]] (the frames).
# Categories/moods drawn from docs/PATCH_FORMAT.md's existing vocabulary.
# ---------------------------------------------------------------------------

N = SAMPLES_PER_FRAME  # shorthand used throughout the lambdas below.

MANIFEST = [
    # -- Classic analog morphs --------------------------------------------
    ("classic-sine-to-saw", "lead", ["clean", "bright"], 4,
     lambda: morph(4, lambda t, i: additive_frame(N, lerp_amps(sine_amps(40), saw_amps(40), t)))),
    ("classic-sine-to-square", "lead", ["clean", "hollow"], 4,
     lambda: morph(4, lambda t, i: additive_frame(N, lerp_amps(sine_amps(40), square_amps(40), t)))),
    ("classic-sine-to-triangle", "lead", ["soft", "warm"], 4,
     lambda: morph(4, lambda t, i: additive_frame(N, lerp_amps(sine_amps(40), triangle_amps(40), t)))),
    ("classic-saw-to-square", "lead", ["bright", "digital"], 4,
     lambda: morph(4, lambda t, i: additive_frame(N, lerp_amps(saw_amps(40), square_amps(40), t)))),

    # -- PWM sweeps ---------------------------------------------------------
    ("pwm-narrow-sweep", "lead", ["thin", "bright"], 8,
     lambda: morph(8, lambda t, i: pulse_frame(N, 0.05 + 0.45 * t))),
    ("pwm-wide-sweep", "pad", ["warm", "lush"], 8,
     lambda: morph(8, lambda t, i: pulse_frame(N, 0.50 + 0.45 * t))),
    ("pwm-full-sweep", "lead", ["evolving", "bright"], 8,
     lambda: morph(8, lambda t, i: pulse_frame(N, 0.05 + 0.90 * t))),
    ("pwm-twin-detune", "pad", ["lush", "warm"], 6,
     lambda: morph(6, lambda t, i: mix_frames(pulse_frame(N, 0.4), pulse_frame(N, 0.45), 0.5))),

    # -- Formant / vocal ------------------------------------------------------
    ("formant-vowel-aa", "vocal texture", ["organic", "warm"], 1,
     lambda: morph(1, lambda t, i: formant_frame(N, VOWEL_AA))),
    ("formant-vowel-ee", "vocal texture", ["organic", "bright"], 1,
     lambda: morph(1, lambda t, i: formant_frame(N, VOWEL_EE))),
    ("formant-vowel-oh", "vocal texture", ["organic", "dark"], 1,
     lambda: morph(1, lambda t, i: formant_frame(N, VOWEL_OH))),
    ("formant-vowel-oo", "vocal texture", ["organic", "soft"], 1,
     lambda: morph(1, lambda t, i: formant_frame(N, VOWEL_OO))),
    ("formant-vowel-morph-a-to-e", "vocal texture", ["evolving", "organic"], 8,
     lambda: morph(8, lambda t, i: formant_frame(N, lerp_formants(VOWEL_AA, VOWEL_EE, t)))),
    ("formant-vowel-morph-o-to-u", "vocal texture", ["evolving", "dark"], 8,
     lambda: morph(8, lambda t, i: formant_frame(N, lerp_formants(VOWEL_OH, VOWEL_OO, t)))),

    # -- Bell / FM-style inharmonic partials ---------------------------------
    ("bell-glass-chime", "fx", ["glassy", "bright"], 6,
     lambda: morph(6, lambda t, i: bell_frame(N, spread=1.0 + 2.0 * t, decay=0.6))),
    ("bell-metallic-strike", "fx", ["metallic", "aggressive"], 6,
     lambda: morph(6, lambda t, i: bell_frame(N, spread=1.5 + 4.0 * t, decay=0.8, count=8))),
    ("bell-fm-classic", "keyboard", ["metallic", "digital"], 4,
     lambda: morph(4, lambda t, i: fm_bell_frame(N, carrier=5, mod=3, num_sidebands=5, decay=0.5 + 0.2 * t))),
    ("bell-dark-gong", "fx", ["dark", "ambient"], 4,
     lambda: morph(4, lambda t, i: bell_frame(N, spread=0.8 + 0.4 * t, decay=0.5, base_ratios=(1, 1.5, 2, 3, 4), count=5))),
    ("bell-bright-tine", "keyboard", ["bright", "glassy"], 4,
     lambda: morph(4, lambda t, i: bell_frame(N, spread=3.0 + 2.0 * t, decay=0.7, base_ratios=(1, 2, 3.5, 5.5), count=4))),

    # -- Wavefolding ----------------------------------------------------------
    ("fold-sine-triangle", "lead", ["evolving", "gritty"], 8,
     lambda: morph(8, lambda t, i: fold_frame(additive_frame(N, sine_amps(1)), amount=0.6 * t))),
    ("fold-sine-sharp", "lead", ["aggressive", "digital"], 8,
     lambda: morph(8, lambda t, i: fold_frame(additive_frame(N, sine_amps(1)), amount=0.5 + 0.5 * t))),
    ("fold-saw-soft", "bass", ["gritty", "warm"], 6,
     lambda: morph(6, lambda t, i: fold_frame(additive_frame(N, saw_amps(20)), amount=0.1 + 0.3 * t))),
    ("fold-dual-sine", "pad", ["lush", "evolving"], 6,
     lambda: morph(6, lambda t, i: fold_frame(additive_frame(N, [1.0, 0.6]), amount=0.2 + 0.4 * t))),

    # -- Chebyshev waveshaping ------------------------------------------------
    ("cheby-warm-drive", "lead", ["warm", "gritty"], 6,
     lambda: morph(6, lambda t, i: chebyshev_frame(N, [0, 1.0 - 0.3 * t, 0.3 * t, 0.1 * t]))),
    ("cheby-aggressive-drive", "lead", ["aggressive", "distorted"], 6,
     lambda: morph(6, lambda t, i: chebyshev_frame(N, [0, 1.0, 0.4 * t, 0.3 * t, 0.2 * t, 0.15 * t, 0.1 * t]))),
    ("cheby-even-harmonics", "lead", ["hollow", "digital"], 4,
     lambda: morph(4, lambda t, i: chebyshev_frame(N, [0, 0, 0.6 + 0.3 * t, 0, 0.3 + 0.2 * t, 0, 0.15 * t]))),
    ("cheby-odd-harmonics", "bass", ["punchy", "clean"], 4,
     lambda: morph(4, lambda t, i: chebyshev_frame(N, [0, 1.0, 0, 0.4 + 0.2 * t, 0, 0.2 + 0.1 * t, 0, 0.1 * t]))),

    # -- Digital / bit-reduction ------------------------------------------
    ("digital-stairstep-soft", "fx", ["digital", "gritty"], 6,
     lambda: morph(6, lambda t, i: quantize_frame(additive_frame(N, sine_amps(1)), levels=round(32 - 24 * t)))),
    ("digital-stairstep-harsh", "fx", ["aggressive", "distorted"], 6,
     lambda: morph(6, lambda t, i: quantize_frame(additive_frame(N, saw_amps(20)), levels=round(16 - 12 * t)))),
    ("digital-gritty-noise-gate", "fx", ["gritty", "digital"], 4,
     lambda: morph(4, lambda t, i: quantize_frame(
         mix_frames(additive_frame(N, saw_amps(20)), noise_spectrum_frame(N, seed=30 + i, brightness=0.5), 0.15),
         levels=round(24 - 16 * t)))),

    # -- Noise / granular textures --------------------------------------------
    ("texture-tonal-to-noise", "drone", ["evolving", "airy"], 8,
     lambda: morph(8, lambda t, i: mix_frames(additive_frame(N, saw_amps(30)), noise_spectrum_frame(N, seed=31, brightness=0.5), t))),
    ("texture-frozen-noise-warm", "drone", ["dark", "ambient"], 4,
     lambda: morph(4, lambda t, i: noise_spectrum_frame(N, seed=100 + i, brightness=0.15))),
    ("texture-frozen-noise-bright", "drone", ["airy", "digital"], 4,
     lambda: morph(4, lambda t, i: noise_spectrum_frame(N, seed=200 + i, brightness=0.85))),
    ("texture-granular-shimmer", "fx", ["evolving", "airy"], 8,
     lambda: morph(8, lambda t, i: noise_spectrum_frame(N, seed=300 + i, brightness=0.4 + 0.5 * t))),
    ("texture-drone-evolve", "drone", ["evolving", "ambient"], 16,
     lambda: morph(16, lambda t, i: mix_frames(bell_frame(N, spread=1.0 + 1.5 * t, decay=0.6),
                                                noise_spectrum_frame(N, seed=400 + i, brightness=0.3 + 0.4 * t), 0.3 + 0.3 * t))),

    # -- Metallic / inharmonic stacks ------------------------------------------
    ("metallic-ratio-stack-a", "keyboard", ["metallic", "bright"], 4,
     lambda: morph(4, lambda t, i: bell_frame(N, spread=1.0 + 1.0 * t, decay=0.65))),
    ("metallic-ratio-stack-b", "keyboard", ["metallic", "aggressive"], 4,
     lambda: morph(4, lambda t, i: bell_frame(N, spread=2.0 + 3.0 * t, decay=0.75, count=8))),
    ("metallic-glass-shimmer", "pad", ["glassy", "airy"], 6,
     lambda: morph(6, lambda t, i: mix_frames(bell_frame(N, spread=1.5 + 1.0 * t, decay=0.6),
                                               noise_spectrum_frame(N, seed=500 + i, brightness=0.7), 0.25))),
    ("metallic-detuned-cluster", "pad", ["metallic", "lush"], 4,
     lambda: morph(4, lambda t, i: sparse_harmonics_frame(N, {h: 0.9 ** j for j, h in enumerate([5, 6, 7, 19, 20, 21])}))),

    # -- Bass ------------------------------------------------------------------
    ("bass-sub-sine-plus", "bass", ["clean", "warm"], 2,
     lambda: morph(2, lambda t, i: additive_frame(N, [1.0, 0.15 + 0.1 * t, 0.05]))),
    ("bass-growl-saw", "bass", ["gritty", "aggressive"], 4,
     lambda: morph(4, lambda t, i: fold_frame(additive_frame(N, saw_amps(8)), amount=0.05 + 0.15 * t))),
    ("bass-square-punch", "bass", ["punchy", "clean"], 4,
     lambda: morph(4, lambda t, i: pulse_frame(N, 0.5 - 0.1 * t, num_harmonics=16))),

    # -- Pluck / percussive ------------------------------------------------
    ("pluck-bright-string", "pluck", ["bright", "clean"], 2,
     lambda: morph(2, lambda t, i: additive_frame(N, [1.0 / (k ** 1.5) for k in range(1, 31)]))),
    ("pluck-mellow-keys", "pluck", ["soft", "warm"], 2,
     lambda: morph(2, lambda t, i: additive_frame(N, [1.0 / (k ** 2.2) for k in range(1, 16)]))),
    ("pluck-glassy-mallet", "pluck", ["glassy", "bright"], 2,
     lambda: morph(2, lambda t, i: sparse_harmonics_frame(N, {1: 1.0, 2: 0.3, 5: 0.2, 9: 0.1}))),

    # -- Chord / harmonic-stack -------------------------------------------
    ("chord-fifth-stack", "chord", ["lush", "bright"], 4,
     lambda: morph(4, lambda t, i: sparse_harmonics_frame(N, {1: 1.0, 2: 0.5, 3: 0.8 * (0.5 + 0.5 * t), 4: 0.3, 6: 0.4 * t, 8: 0.2 * t}))),
    ("chord-octave-stack", "chord", ["clean", "bright"], 4,
     lambda: morph(4, lambda t, i: sparse_harmonics_frame(N, {1: 1.0, 2: 0.7, 4: 0.5, 8: 0.3, 16: 0.15 + 0.1 * t}))),

    # -- Ambient / evolving pads ------------------------------------------
    ("ambient-airy-drift", "pad", ["airy", "dreamy"], 12,
     lambda: morph(12, lambda t, i: mix_frames(additive_frame(N, saw_amps(8)), noise_spectrum_frame(N, seed=600 + i, brightness=0.6),
                                                0.15 + 0.15 * math.sin(t * math.pi)))),
    ("ambient-dreamy-veil", "pad", ["dreamy", "dark"], 12,
     lambda: morph(12, lambda t, i: mix_frames(formant_frame(N, lerp_formants(VOWEL_OH, VOWEL_OO, t)),
                                                noise_spectrum_frame(N, seed=700 + i, brightness=0.25), 0.2))),
    ("ambient-evolving-swell", "pad", ["evolving", "lush"], 16,
     lambda: morph(16, lambda t, i: mix_frames(
         fold_frame(additive_frame(N, lerp_amps(sine_amps(30), saw_amps(30), t)), amount=0.1 + 0.2 * t),
         noise_spectrum_frame(N, seed=800 + i, brightness=0.3 + 0.3 * t), 0.15 + 0.1 * t))),
]

assert len(MANIFEST) == 50, f"expected 50 manifest entries, got {len(MANIFEST)}"

# ---------------------------------------------------------------------------
# Granular-engine source tables (engine type 6). Longer frame counts give the
# grain pool more distinct material to scrub through; category is always
# "granular" so factory preset generation can pick from WT["gran"] exclusively.
# ---------------------------------------------------------------------------

GRAN_MANIFEST = [
    ("gran-cloud-drift", "granular", ["evolving", "airy"], 24,
     lambda: morph(24, lambda t, i: mix_frames(
         noise_spectrum_frame(N, seed=900 + i, brightness=0.25 + 0.55 * t),
         additive_frame(N, saw_amps(6)), 0.08 + 0.12 * t))),
    ("gran-glass-spray", "granular", ["glassy", "bright"], 16,
     lambda: morph(16, lambda t, i: bell_frame(N, spread=1.2 + 3.0 * t, decay=0.55 + 0.25 * t,
                                                 base_ratios=(1, 2, 3, 5, 8, 13), count=6))),
    ("gran-vocal-dust", "granular", ["organic", "evolving"], 12,
     lambda: morph(12, lambda t, i: mix_frames(
         formant_frame(N, lerp_formants(VOWEL_AA, VOWEL_OO, t)),
         noise_spectrum_frame(N, seed=1000 + i, brightness=0.35 + 0.4 * t), 0.2 + 0.35 * t))),
    ("gran-tape-warmth", "granular", ["dark", "warm"], 12,
     lambda: morph(12, lambda t, i: mix_frames(
         noise_spectrum_frame(N, seed=1100 + i, brightness=0.08 + 0.2 * t),
         fold_frame(additive_frame(N, saw_amps(4)), amount=0.05 + 0.12 * t), 0.35))),
    ("gran-crystal-burst", "granular", ["airy", "bright"], 16,
     lambda: morph(16, lambda t, i: sparse_harmonics_frame(N, {
         h: (0.85 ** j) * (0.4 + 0.6 * t) for j, h in enumerate([3, 5, 8, 13, 21, 34, 55][:4 + i % 4])}))),
    ("gran-sub-rumble", "granular", ["dark", "ambient"], 12,
     lambda: morph(12, lambda t, i: additive_frame(N, [1.0, 0.45 - 0.2 * t, 0.25 - 0.1 * t, 0.12, 0.06]))),
    ("gran-digital-glitch", "granular", ["digital", "gritty"], 12,
     lambda: morph(12, lambda t, i: quantize_frame(
         mix_frames(noise_spectrum_frame(N, seed=1200 + i, brightness=0.5 + 0.45 * t),
                    pulse_frame(N, 0.15 + 0.6 * t, num_harmonics=24), 0.25 + 0.35 * t),
         levels=max(4, round(8 + 10 * t))))),
    ("gran-ocean-swell", "granular", ["evolving", "lush"], 32,
     lambda: morph(32, lambda t, i: mix_frames(
         fold_frame(additive_frame(N, lerp_amps(sine_amps(12), saw_amps(12), 0.3 + 0.4 * math.sin(t * math.pi))),
                    amount=0.08 + 0.18 * t),
         noise_spectrum_frame(N, seed=1300 + i, brightness=0.2 + 0.5 * t), 0.18 + 0.22 * t))),
    ("gran-frozen-grit", "granular", ["gritty", "dark"], 8,
     lambda: morph(8, lambda t, i: mix_frames(
         noise_spectrum_frame(N, seed=1400 + i, brightness=0.12 + 0.35 * t),
         chebyshev_frame(N, [0, 0.5, 0.25 * t, 0.15 * t]), 0.3 + 0.25 * t))),
    ("gran-shimmer-voice", "granular", ["evolving", "organic"], 16,
     lambda: morph(16, lambda t, i: mix_frames(
         formant_frame(N, lerp_formants(VOWEL_EE, VOWEL_OH, t)),
         bell_frame(N, spread=2.0 + 2.5 * t, decay=0.7, count=5), 0.15 + 0.2 * t))),
]

ALL_MANIFEST = MANIFEST + GRAN_MANIFEST


# ---------------------------------------------------------------------------
# Generation
# ---------------------------------------------------------------------------

def generate() -> list[dict]:
    if not BUILDER.exists():
        sys.exit(f"murmur-wavetable-builder not found at {BUILDER} -- build it first:\n"
                  f"  cmake --build --preset dev")

    SOURCES_DIR.mkdir(parents=True, exist_ok=True)
    results = []
    for name, category, moods, frame_count, build_fn in ALL_MANIFEST:
        frames = build_fn()
        assert len(frames) == frame_count, f"{name}: expected {frame_count} frames, got {len(frames)}"

        source_path = SOURCES_DIR / f"{name}_source.wav"
        write_wavetable_source(source_path, frames)

        output_path = WAVETABLES_DIR / f"{name}.json"
        proc = subprocess.run(
            [str(BUILDER), "--input", str(source_path), "--output", str(output_path),
             "--frames", str(frame_count), "--samples-per-frame", str(SAMPLES_PER_FRAME),
             "--mip-levels", str(MIP_LEVELS)],
            capture_output=True, text=True,
        )
        if proc.returncode != 0:
            sys.exit(f"murmur-wavetable-builder failed for {name}:\n{proc.stderr}")

        size = output_path.stat().st_size
        print(f"  {name:32s} {category:15s} {frame_count:2d} frames  {size / 1024:8.1f} KB")
        results.append({"name": name, "category": category, "moods": moods, "frames": frame_count, "bytes": size})

    total_bytes = sum(r["bytes"] for r in results)
    print(f"\nWrote {len(results)} wavetables, {total_bytes / (1024 * 1024):.1f} MB total "
          f"(+ {len(results)} source WAVs under {SOURCES_DIR.relative_to(REPO_ROOT)}/).")
    return results


def write_manifest_doc(results: list[dict]) -> None:
    lines = [
        "# content/wavetables/",
        "",
        "Factory wavetable data, built with `murmur-wavetable-builder`"
        " (see `tools/wavetable_builder/`) via `scripts/generate_wavetable_library.py`.",
        "",
        f"{len(MANIFEST)} classic tables + {len(GRAN_MANIFEST)} granular-engine tables,"
        f" all at {SAMPLES_PER_FRAME} samples/frame, {MIP_LEVELS} mip levels"
        " (only frame count varies per table). Source WAVs (used to regenerate the JSON tables"
        " if the builder ever changes) live under `sources/`.",
        "",
        "| Name | Category | Moods | Frames |",
        "|------|----------|-------|--------|",
    ]
    for r in results:
        lines.append(f"| `{r['name']}.json` | {r['category']} | {', '.join(r['moods'])} | {r['frames']} |")
    lines.append("")
    lines.append(f"Granular-engine tables (`gran-*`) are authored for Engine Type 6 — long frame counts"
                  f" so grain position jitter has rich material to scan.")
    lines.append("")
    lines.append("Plus `basic_harmonic.json` (the original UI-GATE-5-era example, kept as-is)"
                  " and `basic_harmonic_source.wav`.")
    lines.append("")
    (WAVETABLES_DIR / "MANIFEST.md").write_text("\n".join(lines))
    print(f"wrote {WAVETABLES_DIR / 'MANIFEST.md'}")


# ---------------------------------------------------------------------------
# Verification: render each table through a minimal Wavetable-engine patch and
# confirm it's actually audible (not silence from a load failure).
# ---------------------------------------------------------------------------

def _minimal_wavetable_patch(wavetable_id: str) -> dict:
    """Smallest patch JSON that gets one operator onto the Wavetable engine
    with the given table -- everything else left at schema defaults (root
    "schemaVersion" isn't actually read on load -- PatchSerializer.cpp always
    stamps the current constant -- so it's omitted here). `isOutput` on the
    algorithm node defaults to false if omitted (PatchSerializer.cpp
    jn.value("isOutput", false)), which would silently mean this operator
    never reaches the mix bus regardless of whether the wavetable itself
    loaded correctly -- a false-negative trap for this exact verification
    script, so it's set explicitly, matching every real .pw8 preset's nodes."""
    operators = [{"engine": 0} for _ in range(8)]
    operators[0] = {"engine": 1, "wavetableId": wavetable_id, "level": 1.0}
    # Only node 0 is isOutput -- operators 1-7 stay on the (silent by default
    # amplitude envelope? no -- audible) Classic engine but MUST be excluded
    # from the mix, or their default oscillators would mask whether operator
    # 0's wavetable specifically loaded and produced sound.
    nodes = [{"id": i, "engine": (1 if i == 0 else 0), "isOutput": i == 0} for i in range(8)]
    return {
        "layerA": {
            "operators": operators,
            "algorithm": {"nodes": nodes, "edges": []},
        },
    }


def _wav_peak(path: pathlib.Path) -> float:
    data = path.read_bytes()
    idx = data.find(b"data")
    if idx < 0:
        return 0.0
    size = struct.unpack("<I", data[idx + 4:idx + 8])[0]
    samples = data[idx + 8:idx + 8 + size]
    peak = 0
    for i in range(0, len(samples) - 1, 2):
        s = struct.unpack("<h", samples[i:i + 2])[0]
        peak = max(peak, abs(s))
    return peak / 32768.0


def verify(results: list[dict]) -> None:
    if not RENDERER.exists():
        sys.exit(f"murmur-render not found at {RENDERER} -- build it first:\n  cmake --build --preset dev")
    if not SINGLE_NOTE_MIDI.exists():
        sys.exit(f"{SINGLE_NOTE_MIDI} not found -- run scripts/generate_test_midi.py first")

    passed = 0
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = pathlib.Path(tmp)
        for r in results:
            wavetable_id = str((WAVETABLES_DIR / f"{r['name']}.json").resolve())
            patch = _minimal_wavetable_patch(wavetable_id)
            patch_path = tmp_path / f"{r['name']}.murmur"
            patch_path.write_text(json.dumps(patch))
            out_wav = tmp_path / f"{r['name']}.wav"

            proc = subprocess.run(
                [str(RENDERER), "--patch", str(patch_path), "--midi", str(SINGLE_NOTE_MIDI), "--output", str(out_wav)],
                capture_output=True, text=True,
            )
            if proc.returncode != 0 or not out_wav.exists():
                print(f"  FAIL {r['name']:32s} render error: {proc.stderr.strip()[:120]}")
                continue

            peak = _wav_peak(out_wav)
            ok = peak > 0.001
            print(f"  {'PASS' if ok else 'FAIL'} {r['name']:32s} peak={peak:.4f}")
            passed += ok

    print(f"\n{passed}/{len(results)} tables PASS (produced audible, non-silent output).")
    if passed != len(results):
        sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true", help="Also render-test every generated table.")
    parser.add_argument("--skip-generate", action="store_true", help="Only run --verify against already-generated tables.")
    args = parser.parse_args()

    if args.skip_generate:
        results = [{"name": name, "category": c, "moods": m, "frames": f} for name, c, m, f, _ in ALL_MANIFEST]
    else:
        results = generate()
        write_manifest_doc(results)

    if args.verify:
        verify(results)


if __name__ == "__main__":
    main()
