#pragma once

// STATUS: SCAFFOLD / PARTIAL.
//
// Host-visible automatable parameters (docs/ROADMAP.md "Automation"): the plugin
// does NOT expose every internal pw8::patch::Patch field as a flat DAW automation
// lane. Only macros (M1-M8) and a small set of performance-critical parameters are
// planned to be wired to a juce::AudioProcessorValueTreeState; the rest of the patch
// travels through getStateInformation()/setStateInformation() as the native .pw8
// JSON document (see PatchworkEightProcessor) so there is exactly one source of
// truth for patch data, per docs/PLUGIN_ARCHITECTURE.md "Host State".
//
// This header currently only documents the intended parameter ID scheme; the
// juce::AudioProcessorValueTreeState wiring itself is PLANNED (Phase 16).

#include <array>
#include <string>

namespace pw8::plugin
{
    /// Stable macro parameter IDs exposed to host automation, matching
    /// pw8::patch::Patch::macros[0..7]. These strings are the DAW-automation contract:
    /// once shipped, they must never be renamed (see docs/PATCH_FORMAT.md "Parameter
    /// System" for the same stability rule applied to the engine's own parameter IDs).
    inline constexpr std::array<const char*, 8> kMacroParameterIds = {
        "macro1", "macro2", "macro3", "macro4", "macro5", "macro6", "macro7", "macro8",
    };

} // namespace pw8::plugin
