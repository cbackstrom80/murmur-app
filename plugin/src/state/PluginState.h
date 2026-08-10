#pragma once

// Host-visible automatable parameters (docs/PLUGIN_ARCHITECTURE.md "Automation"):
// the plugin does NOT expose every internal pw8::patch::Patch field as a flat DAW
// automation lane -- only macros (M1-M8) are, matching the master spec's "8
// routable macros" as the primary host-facing control surface (the same macro
// model validated against Phase Plant's in docs/ROADMAP.md's competitive-research
// pass). The rest of the patch travels through getStateInformation()/
// setStateInformation() as the native .pw8 JSON document (see
// PatchworkEightProcessor), so there is exactly one source of truth for patch
// data, per docs/PLUGIN_ARCHITECTURE.md "Host State" -- macro parameter *values*
// are additionally kept in sync with that document (see
// PatchworkEightProcessor::syncMacroParametersFromPatch/syncPatchMacrosFromParameters)
// rather than being a second, divergent store of the same 8 floats.

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

namespace pw8::plugin
{
    /// Stable macro parameter IDs exposed to host automation, matching
    /// pw8::patch::Patch::macros[0..7]. These strings are the DAW-automation contract:
    /// once shipped, they must never be renamed (see docs/PATCH_FORMAT.md "Parameter
    /// System" for the same stability rule applied to the engine's own parameter IDs).
    inline constexpr std::array<const char*, 8> kMacroParameterIds = {
        "macro1", "macro2", "macro3", "macro4", "macro5", "macro6", "macro7", "macro8",
    };

    inline constexpr std::array<const char*, 8> kMacroParameterNames = {
        "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Macro 5", "Macro 6", "Macro 7", "Macro 8",
    };

    /// Builds the plugin's full `AudioProcessorValueTreeState` parameter layout --
    /// currently just the 8 macros, each a plain 0..1 float. Kept as a free function
    /// (rather than inline in the processor) so it's easy to extend with the "small
    /// set of performance-critical parameters" docs/PLUGIN_ARCHITECTURE.md leaves
    /// room for, without processor constructor clutter.
    [[nodiscard]] juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace pw8::plugin
