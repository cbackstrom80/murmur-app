# tools/fuzz_render/

**IMPLEMENTED.** `pw8-fuzz-render`: generates random-but-schema-valid patches
(bounded parameters, compiler-guaranteed-acyclic algorithm graphs), renders each
through `pw8::render::render()`, and asserts no crash / no NaN / no Inf / bounded
output / reasonable runtime.

```bash
./build/dev/tools/pw8-fuzz-render --count 10000 --seed 1 [--duration 1.5] [--sample-rate 48000] [--verbose] [--stop-on-first-failure]
```

Verified: 5,000 patches at seed 1, zero failures, zero NaN/Inf (Debug build). The
master spec's overnight target of 1,000,000+ patches is left to whoever runs it --
`--count`/`--seed` are both exposed for exactly that; a Release build
(`cmake --preset release`) will run substantially faster than a Debug one.

See [docs/TESTING.md](../../docs/TESTING.md) "Property / Fuzz Testing".
