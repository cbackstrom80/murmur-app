# pw8/filter/

**Filter 1: IMPLEMENTED** -- `StateVariableFilter.hpp`, a trapezoidal-integrator
state-variable filter (lowpass/highpass/bandpass/notch/peak), per-voice, wired into
the signal path between the algorithm graph and the amplitude envelope (see
`pw8::voice::Voice::renderSample()`). See `docs/DSP_ENGINE.md` "Filter System".

**Filter 2 ("character" filter -- nonlinear ladder-style/OTA-style/diode-style/
saturated cascade topologies): PLANNED**, per docs/ROADMAP.md Phase 6 continuation.
