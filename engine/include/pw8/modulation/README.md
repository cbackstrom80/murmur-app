# pw8/modulation/

PLANNED (docs/ROADMAP.md Phase 5). Will hold `ModulationRoute`, the fixed-capacity
64-route mod matrix, macros-to-destination wiring, and (later) meta-modulation. See
`docs/MODULATION.md`. Per-note expression (`pw8::voice::NoteExpression` in
`pw8/voice/Voice.hpp`) is already captured from MIDI/MPE today but not yet routed
through a matrix -- only pitch bend directly affects pitch in this pass.
