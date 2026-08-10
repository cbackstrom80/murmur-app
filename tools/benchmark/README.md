# tools/benchmark/

Superseded in scope by `benchmarks/pw8_benchmarks` itself, which is a normal Google
Benchmark executable and already accepts the standard gbenchmark CLI flags
(`--benchmark_filter`, `--benchmark_min_time`, `--benchmark_format=json`, etc.) --
see [docs/BUILD.md](../../docs/BUILD.md) "Benchmarks". This directory is reserved
in case a higher-level wrapper (e.g. one that runs the suite across multiple build
configurations and diffs results) becomes useful later.
