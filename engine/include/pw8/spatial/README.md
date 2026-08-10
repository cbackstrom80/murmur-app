# pw8/spatial/

PLANNED (docs/ROADMAP.md Phase 7 / Phase 11). Will hold the native spatial DSP
(pan/balance/width/mid-side/low-frequency-mono/per-voice spread/FX return width) and
`centerGravity` wiring described in `docs/DSP_ENGINE.md` "Center Gravity" and "Low-End
Control". Today, `Voice::renderSample()` (`pw8/voice/Voice.hpp`) applies only a
simple equal-power pan per voice; layer-level width/centerGravity fields exist on
`pw8::patch::LayerPatch` but are not yet wired into the signal path.
