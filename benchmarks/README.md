# benchmarks/

**IMPLEMENTED.** Google Benchmark suite (`PW8_BUILD_BENCHMARKS` / `benchmarks`
preset). Covers:

- `OscillatorBenchmarks.cpp` -- Classic oscillator (saw, morph sweep) and Wavetable
  oscillator, at 44.1/48/96 kHz.
- `AlgorithmGraphBenchmarks.cpp` -- parallel-8, serial-8, and feedback-bell topologies.
- `VoiceBenchmarks.cpp` -- single-voice full render loop (oscillator + envelope +
  algorithm graph).
- `RenderBenchmarks.cpp` -- full-patch rendering across the master spec's exact
  matrix: 44.1/48/96 kHz x 1/8/16/32 voices.

```bash
cmake --preset benchmarks
cmake --build --preset benchmarks -j
./build/benchmarks/benchmarks/pw8_benchmarks
```

See [docs/TESTING.md](../docs/TESTING.md) for sample captured numbers.
