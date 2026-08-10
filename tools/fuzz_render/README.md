# tools/fuzz_render/

PLANNED (docs/TESTING.md "Property / Fuzz Testing"). Will hold `pw8-fuzz-render`: a
developer executable that generates random but *schema-valid* patches (bounded
parameters, compiler-validated algorithm graphs), renders each through
`pw8::render::render()`, and asserts no crash / no NaN / no Inf / bounded output /
reasonable runtime. Target: 10,000 patches for a quick local run, 1,000,000+ for an
overnight soak. Not implemented in this pass -- today that guarantee rests on the
targeted Catch2 regression cases in `tests/regression/RenderSanityTests.cpp` (e.g.
the self-feedback-algorithm finite-output case) rather than broad fuzzing.
