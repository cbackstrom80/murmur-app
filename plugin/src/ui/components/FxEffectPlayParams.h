#pragma once

#include <array>
#include <cstddef>

#include <juce_core/juce_core.h>

// Maps PLAY-mode knobs: up to 4 primary controls per effect type (plus Mix).
// All 10 algorithms are live in DSP — see docs/FX_BANK.md.
namespace pw8::plugin::ui
{
    struct FxParamKnobDef
    {
        const char* fieldSuffix; ///< APVTS suffix after slot prefix, e.g. "TapeDelayMs"
        const char* label;
    };

    inline constexpr std::size_t kMaxFxPlayParams = 4;

    struct FxTypePlaySpec
    {
        const char* name;       ///< Full name for type row
        const char* flowAbbrev; ///< 4-char label on chain diagram
        std::array<FxParamKnobDef, kMaxFxPlayParams> params{};
    };

    [[nodiscard]] inline const FxTypePlaySpec& fxPlaySpecForType(int typeOrdinal) noexcept
    {
        static constexpr FxTypePlaySpec kBypass{"BYPASS", "----", {}};
        static constexpr FxTypePlaySpec kSaturation{
            "SATURATION", "SAT",
            {{{"SaturationDrive", "Drive"}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}}}};
        static constexpr FxTypePlaySpec kChorus{
            "CHORUS", "CHR",
            {{{"ChorusRate", "Rate"}, {"ChorusDepth", "Depth"}, {"ChorusBaseDelay", "Delay"}, {nullptr, nullptr}}}};
        static constexpr FxTypePlaySpec kTapeDelay{
            "TAPE DELAY", "TAPE",
            {{{"TapeDelayMs", "Time"}, {"TapeFeedback", "Fdbk"}, {"TapeDrive", "Drive"}, {"TapeDriftDepth", "Wow"}}}};
        static constexpr FxTypePlaySpec kNodeDelay{
            "NODE DELAY", "NODE",
            {{{"NodeInsanity", "Insanity"}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}}}};
        static constexpr FxTypePlaySpec kFreqShift{
            "FREQ SHIFT", "FSHF",
            {{{"FreqShiftHz", "Shift"}, {"FreqShiftDelayMs", "Time"}, {"FreqShiftFeedback", "Fdbk"}, {nullptr, nullptr}}}};
        static constexpr FxTypePlaySpec kFractal{
            "FRACTAL ECHO", "FRAC",
            {{{"FractalMorph", "Morph"}, {"FractalBaseDelayMs", "Time"}, {"FractalRatio", "Ratio"}, {nullptr, nullptr}}}};
        static constexpr FxTypePlaySpec kReverb{
            "REVERB", "REV",
            {{{"ReverbDecaySeconds", "Decay"}, {"ReverbSize", "Size"}, {"ReverbPreDelayMs", "Pre"}, {"ReverbDiffusion", "Diff"}}}};
        static constexpr FxTypePlaySpec kEq{
            "EQ", "EQ",
            {{{"EqLowGainDb", "Low"}, {"EqMidGainDb", "Mid"}, {"EqHighGainDb", "High"}, {nullptr, nullptr}}}};
        static constexpr FxTypePlaySpec kCompressor{
            "COMPRESSOR", "COMP",
            {{{"CompThresholdDb", "Thresh"}, {"CompRatio", "Ratio"}, {"CompAttackMs", "Atk"}, {"CompMakeupDb", "Makeup"}}}};
        static constexpr FxTypePlaySpec kLimiter{
            "LIMITER", "LIM",
            {{{"LimiterCeilingDb", "Ceil"}, {"LimiterLookaheadMs", "Look"}, {"LimiterReleaseMs", "Rel"}, {nullptr, nullptr}}}};

        switch (typeOrdinal)
        {
            case 1: return kSaturation;
            case 2: return kChorus;
            case 3: return kTapeDelay;
            case 4: return kNodeDelay;
            case 5: return kFreqShift;
            case 6: return kFractal;
            case 7: return kReverb;
            case 8: return kEq;
            case 9: return kCompressor;
            case 10: return kLimiter;
            default: return kBypass;
        }
    }

    [[nodiscard]] inline int fxTypeOrdinalFromChipLabel(const juce::String& label) noexcept
    {
        if (label == "SATUR") return 1;
        if (label == "CHORUS") return 2;
        if (label == "TAPE") return 3;
        if (label == "NODE") return 4;
        if (label == "FSHF") return 5;
        if (label == "FRACT") return 6;
        if (label == "REVERB") return 7;
        if (label == "EQ") return 8;
        if (label == "COMP") return 9;
        if (label == "LIMIT") return 10;
        return 0;
    }

    [[nodiscard]] inline juce::String fxChipLabelForType(int typeOrdinal) noexcept
    {
        switch (typeOrdinal)
        {
            case 1: return "SATUR";
            case 2: return "CHORUS";
            case 3: return "TAPE";
            case 4: return "NODE";
            case 5: return "FSHF";
            case 6: return "FRACT";
            case 7: return "REVERB";
            case 8: return "EQ";
            case 9: return "COMP";
            case 10: return "LIMIT";
            default: return "OFF";
        }
    }

} // namespace pw8::plugin::ui
