# standalone/

The JUCE Standalone plugin format (a self-contained app wrapping `pw8_plugin` with
JUCE's built-in audio/MIDI device selector) is produced automatically by
[`plugin/CMakeLists.txt`](../plugin/CMakeLists.txt)'s `juce_add_plugin(... FORMATS
... Standalone ...)` call -- no separate code is required for the MVP.

This directory is reserved for standalone-specific extensions once they're needed
(a custom device-selection UI, a headless/CLI standalone mode distinct from
`murmur-render`, session/autosave behavior that doesn't apply to VST3/AU hosting).
Currently empty. See `docs/PLUGIN_ARCHITECTURE.md`.
