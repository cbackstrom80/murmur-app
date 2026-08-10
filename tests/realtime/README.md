# tests/realtime/

PLANNED. Reserved for tests that assert realtime-safety properties directly (e.g.
running the audio callback under an allocation-counting/interception harness to
prove `Engine::process()` never allocates). Today that guarantee is enforced by
code review and the coding standards in `docs/DSP_ENGINE.md`, not by an automated
check -- see `docs/TESTING.md`.
