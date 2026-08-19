# content/wavetables/

Factory wavetable data, built with `murmur-wavetable-builder` (see `tools/wavetable_builder/`) via `scripts/generate_wavetable_library.py`.

50 classic tables + 10 granular-engine tables, all at 2048 samples/frame, 10 mip levels (only frame count varies per table). Source WAVs (used to regenerate the JSON tables if the builder ever changes) live under `sources/`.

| Name | Category | Moods | Frames |
|------|----------|-------|--------|
| `classic-sine-to-saw.json` | lead | clean, bright | 4 |
| `classic-sine-to-square.json` | lead | clean, hollow | 4 |
| `classic-sine-to-triangle.json` | lead | soft, warm | 4 |
| `classic-saw-to-square.json` | lead | bright, digital | 4 |
| `pwm-narrow-sweep.json` | lead | thin, bright | 8 |
| `pwm-wide-sweep.json` | pad | warm, lush | 8 |
| `pwm-full-sweep.json` | lead | evolving, bright | 8 |
| `pwm-twin-detune.json` | pad | lush, warm | 6 |
| `formant-vowel-aa.json` | vocal texture | organic, warm | 1 |
| `formant-vowel-ee.json` | vocal texture | organic, bright | 1 |
| `formant-vowel-oh.json` | vocal texture | organic, dark | 1 |
| `formant-vowel-oo.json` | vocal texture | organic, soft | 1 |
| `formant-vowel-morph-a-to-e.json` | vocal texture | evolving, organic | 8 |
| `formant-vowel-morph-o-to-u.json` | vocal texture | evolving, dark | 8 |
| `bell-glass-chime.json` | fx | glassy, bright | 6 |
| `bell-metallic-strike.json` | fx | metallic, aggressive | 6 |
| `bell-fm-classic.json` | keyboard | metallic, digital | 4 |
| `bell-dark-gong.json` | fx | dark, ambient | 4 |
| `bell-bright-tine.json` | keyboard | bright, glassy | 4 |
| `fold-sine-triangle.json` | lead | evolving, gritty | 8 |
| `fold-sine-sharp.json` | lead | aggressive, digital | 8 |
| `fold-saw-soft.json` | bass | gritty, warm | 6 |
| `fold-dual-sine.json` | pad | lush, evolving | 6 |
| `cheby-warm-drive.json` | lead | warm, gritty | 6 |
| `cheby-aggressive-drive.json` | lead | aggressive, distorted | 6 |
| `cheby-even-harmonics.json` | lead | hollow, digital | 4 |
| `cheby-odd-harmonics.json` | bass | punchy, clean | 4 |
| `digital-stairstep-soft.json` | fx | digital, gritty | 6 |
| `digital-stairstep-harsh.json` | fx | aggressive, distorted | 6 |
| `digital-gritty-noise-gate.json` | fx | gritty, digital | 4 |
| `texture-tonal-to-noise.json` | drone | evolving, airy | 8 |
| `texture-frozen-noise-warm.json` | drone | dark, ambient | 4 |
| `texture-frozen-noise-bright.json` | drone | airy, digital | 4 |
| `texture-granular-shimmer.json` | fx | evolving, airy | 8 |
| `texture-drone-evolve.json` | drone | evolving, ambient | 16 |
| `metallic-ratio-stack-a.json` | keyboard | metallic, bright | 4 |
| `metallic-ratio-stack-b.json` | keyboard | metallic, aggressive | 4 |
| `metallic-glass-shimmer.json` | pad | glassy, airy | 6 |
| `metallic-detuned-cluster.json` | pad | metallic, lush | 4 |
| `bass-sub-sine-plus.json` | bass | clean, warm | 2 |
| `bass-growl-saw.json` | bass | gritty, aggressive | 4 |
| `bass-square-punch.json` | bass | punchy, clean | 4 |
| `pluck-bright-string.json` | pluck | bright, clean | 2 |
| `pluck-mellow-keys.json` | pluck | soft, warm | 2 |
| `pluck-glassy-mallet.json` | pluck | glassy, bright | 2 |
| `chord-fifth-stack.json` | chord | lush, bright | 4 |
| `chord-octave-stack.json` | chord | clean, bright | 4 |
| `ambient-airy-drift.json` | pad | airy, dreamy | 12 |
| `ambient-dreamy-veil.json` | pad | dreamy, dark | 12 |
| `ambient-evolving-swell.json` | pad | evolving, lush | 16 |
| `gran-cloud-drift.json` | granular | evolving, airy | 24 |
| `gran-glass-spray.json` | granular | glassy, bright | 16 |
| `gran-vocal-dust.json` | granular | organic, evolving | 12 |
| `gran-tape-warmth.json` | granular | dark, warm | 12 |
| `gran-crystal-burst.json` | granular | airy, bright | 16 |
| `gran-sub-rumble.json` | granular | dark, ambient | 12 |
| `gran-digital-glitch.json` | granular | digital, gritty | 12 |
| `gran-ocean-swell.json` | granular | evolving, lush | 32 |
| `gran-frozen-grit.json` | granular | gritty, dark | 8 |
| `gran-shimmer-voice.json` | granular | evolving, organic | 16 |

Granular-engine tables (`gran-*`) are authored for Engine Type 6 — long frame counts so grain position jitter has rich material to scan.

Plus `basic_harmonic.json` (the original UI-GATE-5-era example, kept as-is) and `basic_harmonic_source.wav`.
