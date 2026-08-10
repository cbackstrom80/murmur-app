# plugin/src/midi/

PLANNED (docs/ROADMAP.md Phase 16). MIDI-in handling currently lives inline in
`PatchworkEightProcessor::processBlock()` (translating `juce::MidiBuffer` into
`pw8::render::Engine` calls). This directory is reserved for MPE zone
configuration/detection and MIDI learn once those grow past a few lines.
