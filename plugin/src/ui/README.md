# plugin/src/ui/

**PLAY mode: IMPLEMENTED** (the OBSIDIAN skin -- see [docs/UI.md](../../../docs/UI.md)
for the full design writeup, architecture, and a real bug caught building it).
`MurmurProcessor::createEditor()` returns `ui::PlayModeEditor`, a real
custom `juce::AudioProcessorEditor` -- not `juce::GenericAudioProcessorEditor`
anymore.

**DESIGN and LAB modes: PLANNED.** PLAY mode is the only mode that exists
today; there is no mode switcher because there is nothing yet to switch to.

See [docs/PLUGIN_ARCHITECTURE.md](../../../docs/PLUGIN_ARCHITECTURE.md)
"Signature UI: Graph" for the algorithm-graph-centric design intent this
delivers on.
