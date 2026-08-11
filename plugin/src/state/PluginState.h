#pragma once

// Host-visible automatable parameters (docs/PLUGIN_ARCHITECTURE.md "Automation").
//
// Scope: every scalar (float/int/bool/enum) field that (a) currently has an
// audible effect on Layer A (the only voiced layer, Phase 8) or is genuinely a
// live performance control (macros, arpeggiator's scalar fields, effect slot
// scalar fields), and (b) is POD -- safe to read/write from the audio thread with
// zero allocation risk. That's 610 parameters: 8 macros, Filter1 (5), 8 LFOs x 5
// fields (40), 8 operators x 13 fields (104), 8 envelopes x 8 fields (64), layer
// gain/pan + master gain (3), 3 insert + 4 master FX slots x 54 scalar fields
// each (378, covering all 10 effect algorithms -- Reverb grew from 4 to 15
// fields in GATE 11's multiband redesign), and the arpeggiator's 8 top-level
// scalar fields -- see docs/PLUGIN_ARCHITECTURE.md "Automation" for the exact
// count and running total across passes (8 -> 270 -> 361 -> 501 -> 578 -> 610,
// docs/ROADMAP.md).
//
// Deliberately NOT exposed as flat automation (see render::Engine's "Live
// parameter API" doc comment for the parallel, more detailed rationale):
//   - Any std::string field (wavetableId, patch/macro metadata) -- not a
//     continuous-automation-shaped data type.
//   - Algorithm graph topology (nodes/edges) and the mod-route list -- structural
//     patch-editing data, not something a performer turns a knob on.
//   - The arpeggiator's per-step array (up to 64 steps x 8 fields = 512 more
//     parameters) and NodeDelay/FractalEcho's node-tree arrays (6 nodes x 7
//     fields, plus FractalEcho's 2 seeds) -- same reasoning, and each is already
//     fully covered by the native .pw8 JSON round-trip via getStateInformation/
//     setStateInformation.
//   - Unison and layer width/centerGravity -- schema-present but not yet DSP-wired
//     (ROADMAP Phase 7/8 PARTIAL); publishing automation for a parameter with no
//     audible effect yet would be misleading rather than useful.
//   - Layer B's operators/filter/LFOs/envelopes -- Layer B isn't voiced yet
//     (Phase 8); symmetric automation lands once dual-layer mixing does.
//   - voiceSettings.polyphony/a4Hz, and the patch seed -- structural/tuning-
//     reference changes, not continuous performance parameters.
//   - LFO/envelope LAYER vs. GLOBAL mod-matrix *scope* itself isn't a parameter
//     here (it lives on each ModRoute, and the mod-route list isn't automatable,
//     per above) -- automating an LFO's rate/waveform/etc. affects both its
//     VOICE-scope per-voice instance and its LAYER/GLOBAL-scope shared instance
//     simultaneously (see Engine::setLfoLive()), which is already everything a
//     host-automated LFO parameter can meaningfully mean.
//
// Implementation note: enum-valued parameters (filter mode, LFO waveform, effect
// type, etc.) are exposed as stepped `AudioParameterFloat`s (integer-valued,
// `discrete = true` in `ParamFieldSpec`) rather than `AudioParameterChoice`, for
// implementation uniformity across all these parameters -- every one of them is
// read the same way (`getRawParameterValue()->load()`), avoiding needing several
// different JUCE parameter subclasses and per-type read paths. The host UI will
// show a continuous slider rather than a named dropdown for these; a real
// PLAY/DESIGN/LAB UI (Phase 17) would present them better.

#include <array>
#include <cstddef>

#include <juce_audio_processors/juce_audio_processors.h>

namespace pw8::plugin
{
    inline constexpr std::array<const char*, 8> kMacroParameterIds = {
        "macro1", "macro2", "macro3", "macro4", "macro5", "macro6", "macro7", "macro8",
    };
    inline constexpr std::array<const char*, 8> kMacroParameterNames = {
        "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Macro 5", "Macro 6", "Macro 7", "Macro 8",
    };

    inline constexpr std::size_t kNumOperators = 8;
    inline constexpr std::size_t kNumLfos = 8;
    inline constexpr std::size_t kNumEnvelopes = 8;
    inline constexpr std::size_t kNumInsertFxSlots = 3;
    inline constexpr std::size_t kNumMasterFxSlots = 4;
    inline constexpr std::size_t kNumOperatorFields = 13;
    inline constexpr std::size_t kNumFilterFields = 5;
    inline constexpr std::size_t kNumLfoFields = 5;
    inline constexpr std::size_t kNumEnvelopeFields = 8;
    inline constexpr std::size_t kNumEffectSlotFields = 54;
    inline constexpr std::size_t kNumArpFields = 8;

    /// One automatable field's shape: a stable ID suffix, a human-readable label,
    /// its range, and its reset default. `discrete` means integer-stepped (bools,
    /// enums, and int-typed fields like syncDivisionIndex/numSteps) -- everything
    /// else is a continuous float.
    struct ParamFieldSpec
    {
        const char* idSuffix;
        const char* label;
        float minValue;
        float maxValue;
        float defaultValue;
        bool discrete = false;
    };

    // Field order matches op::OperatorParams's own field order (see Engine::setOperatorLive).
    extern const std::array<ParamFieldSpec, kNumOperatorFields> kOperatorFieldSpecs;
    // Field order matches filter::FilterParams.
    extern const std::array<ParamFieldSpec, kNumFilterFields> kFilterFieldSpecs;
    // Field order matches lfo::LfoParams.
    extern const std::array<ParamFieldSpec, kNumLfoFields> kLfoFieldSpecs;
    // Field order matches envelope::DahdsrParams.
    extern const std::array<ParamFieldSpec, kNumEnvelopeFields> kEnvelopeFieldSpecs;
    // Field order matches effects::EffectSlotParams's scalar fields (excludes
    // `nodes[]`, `fractalSeedA/B`) -- shared by both insert and master FX slots.
    extern const std::array<ParamFieldSpec, kNumEffectSlotFields> kEffectSlotFieldSpecs;
    // Field order matches sequencer::ArpeggiatorParams's scalar fields (excludes `steps[]`).
    extern const std::array<ParamFieldSpec, kNumArpFields> kArpFieldSpecs;

    [[nodiscard]] juce::String operatorParamId(std::size_t opIndex, const char* fieldSuffix);
    [[nodiscard]] juce::String lfoParamId(std::size_t lfoIndex, const char* fieldSuffix);
    [[nodiscard]] juce::String envelopeParamId(std::size_t envIndex, const char* fieldSuffix);
    [[nodiscard]] juce::String insertFxParamId(std::size_t slot, const char* fieldSuffix);
    [[nodiscard]] juce::String masterFxParamId(std::size_t slot, const char* fieldSuffix);

    inline constexpr const char* kFilterIdPrefix = "filter";
    inline constexpr const char* kArpIdPrefix = "arp";
    inline constexpr const char* kLayerGainId = "layerGain";
    inline constexpr const char* kLayerPanId = "layerPan";
    inline constexpr const char* kMasterGainId = "masterGain";

    /// Builds the plugin's full `AudioProcessorValueTreeState` parameter layout --
    /// all 610 parameters described above, generated from the field-spec tables
    /// rather than hand-written one at a time.
    [[nodiscard]] juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace pw8::plugin
