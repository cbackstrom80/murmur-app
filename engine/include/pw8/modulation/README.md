# pw8/modulation/

**IMPLEMENTED (VOICE scope).** `ModMatrixTypes.hpp` (`ModSource`, `ModDestination`,
`ModRoute`) + `ModMatrixExecutor.hpp` (per-voice, per-sample execution). Fixed
capacity: `core::FixedVector<ModRoute, core::kMaxModRoutes>` (64) lives on
`LayerPatch::modRoutes`. Sources: LFO1, amplitude envelope, velocity, channel
pressure, poly aftertouch, MPE slide, mod wheel (CC1), 8 macros. Destinations: filter cutoff
(exponential/semitone), filter resonance, per-operator level, pan.

LAYER and GLOBAL scope (sharing one computed value across all voices in a layer, or
across the whole patch, rather than recomputing per-voice) are PLANNED -- see
`docs/MODULATION.md`. Meta-modulation (modulating a route's own depth) is PLANNED.
