# pw8/lfo/

**IMPLEMENTED** (per-voice) -- `Lfo.hpp`: sine/triangle/saw/square/sample-and-hold/
smooth-random waveforms, free/retrigger/one-shot/tempo-sync modes, one instance
(`lfo1`) owned by each `pw8::voice::Voice`. Wired to the mod matrix as `ModSource::Lfo1`
(see `pw8/modulation/`). A shared **global** LFO mode (one instance, phase-locked
across all voices, rather than one independent instance per voice) is PLANNED -- see
`docs/MODULATION.md`.
