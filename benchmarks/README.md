# benchmarks/

Google Benchmark suite (docs/TESTING.md "Benchmarks"). PLANNED -- not implemented in
this pass. Intended coverage once `PW8_BUILD_BENCHMARKS=ON` is wired up:

- Classic oscillator, wavetable oscillator, algorithm graph execution, voice
  rendering, full-patch rendering, reverb (once implemented)
- At 44.1 / 48 / 96 kHz
- At 1 / 8 / 16 / 32 voices

See `docs/ROADMAP.md` and the top-level `CMakeLists.txt`'s `PW8_BUILD_BENCHMARKS` option.
