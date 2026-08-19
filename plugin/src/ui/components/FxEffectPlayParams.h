#pragma once

#include <array>
#include <cstddef>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "DesignFxUiState.h"

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
            {{{"CompThresholdDb", "Thresh"}, {"CompRatio", "Ratio"}, {"CompAttackMs", "Atk"}, {"CompKneeDb", "Knee"}}}};
        static constexpr FxTypePlaySpec kLimiter{
            "LIMITER", "LIM",
            {{{"LimiterCeilingDb", "Ceil"}, {"LimiterLookaheadMs", "Look"}, {"LimiterReleaseMs", "Rel"}, {nullptr, nullptr}}}};
        static constexpr FxTypePlaySpec kVocoder{
            "VOCODER", "VOC",
            {{{"VocoderBandCount", "Bands"}, {"VocoderFormant", "Formant"}, {"VocoderReleaseMs", "Release"},
              {"VocoderScGainDb", "SC Gain"}}}};
        static constexpr FxTypePlaySpec kClouds{
            "CLOUDS", "CLD",
            {{{"CloudsDensity", "Density"}, {"CloudsGrainSizeMs", "Grain"}, {"CloudsPitch", "Pitch"},
              {"CloudsFreeze", "Freeze"}}}};
        static constexpr FxTypePlaySpec kQuasar{
            "QUASAR", "QSR",
            {{{"Qsr1Level", "QSR1"}, {"Qsr1AngleDeg", "Azimuth"}, {"Qsr1Distance", "Distance"},
              {"CntrLevel", "Center"}}}};

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
            case 11: return kVocoder;
            case 12: return kClouds;
            case 13: return kQuasar;
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
        if (label == "VOCODER" || label == "VOC") return 11;
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
            case 11: return "VOCODER";
            case 12: return "CLOUDS";
            case 13: return "QUASAR";
            default: return "OFF";
        }
    }

    inline constexpr std::size_t kDesignFxKnobCount = 6;

    struct DesignFxChipSpec
    {
        const char* heroTitle;
        const char* slotLabel;
        const char* vizTitle;
        const char* defaultPreset;
        std::array<FxParamKnobDef, kDesignFxKnobCount> knobs{};
        std::array<const char*, 6> modePills{};
        std::size_t modePillCount = 0;
    };

    [[nodiscard]] inline DesignFxChipSpec fxDesignSpecForChip(std::size_t chipIndex) noexcept
    {
        static constexpr DesignFxChipSpec kBypass{
            "BYPASS", "I1", "SIGNAL PASSTHROUGH", "PRESET: DRY INSERT", {}, {}, 0};
        static constexpr DesignFxChipSpec kSat{
            "SATURATION HARMONIC ENGINE", "I2", "SATURATION TRANSFER CURVE (INPUT VS OUTPUT)",
            "PRESET: CLASS A TUBE CRUNCH",
            {{{"SaturationDrive", "DRIVE"}, {nullptr, "TONE"}, {nullptr, "COLOR"}, {"Mix", "MIX"}, {nullptr, "OUTPUT"},
              {nullptr, "BIAS"}}},
            {"TUBE", "TAPE", "DIODE", "FOLD", "CRUSH", nullptr},
            5};
        static constexpr DesignFxChipSpec kChr{
            "CHORUS MULTI-VOICE SPATIALIZER", "I3", "STEREO CHORUS SPATIALIZER", "PRESET: WIDE ENSEMBLE",
            {{{"ChorusRate", "RATE"}, {"ChorusDepth", "DEPTH"}, {nullptr, "FEEDBACK"}, {"Mix", "MIX"},
              {nullptr, "VOICES"}, {"ChorusBaseDelay", "SPREAD"}}},
            {},
            0};
        static constexpr DesignFxChipSpec kTape{
            "ANALOG TAPE SATURATION & DRIFT", "I4", "TAPE WOW / FLUTTER DRIFT", "PRESET: VINTAGE REEL",
            {{{"TapeDriftRate", "SPEED"}, {"TapeDriftDepth", "WOW"}, {nullptr, "FLUTTER"},
              {"TapeDrive", "SATURATION"}, {"TapeDuck", "HISS"}, {"TapeFeedback", "AGE"}}},
            {},
            0};
        static constexpr DesignFxChipSpec kMood{
            "MOOD SPECTRAL RESONATOR FILTER", "I5", "SPECTRAL MORPHING FREQUENCY RESPONSE",
            "PRESET: WARM ANALOG GLUE",
            {{{nullptr, "INTENSITY"}, {nullptr, "COLOR"}, {nullptr, "FILTER FREQ"}, {nullptr, "RESONANCE"},
              {nullptr, "DRIVE"}, {"Mix", "MIX"}}},
            {"WARM", "DARK", "BRIGHT", "ACID", "ETHEREAL", nullptr},
            5};
        static constexpr DesignFxChipSpec kFshf{
            "FREQUENCY SHIFTER & BODE SYSTEM", "M1", "BODE FREQUENCY SHIFT", "PRESET: BODE CASCADE",
            {{{"FreqShiftHz", "SHIFT"}, {"FreqShiftFeedback", "FEEDBACK"}, {"Mix", "MIX"},
              {"FreqShiftLowCutHz", "SCALE"}, {nullptr, "DETUNE"}, {nullptr, "STEREO"}}},
            {},
            0};
        static constexpr DesignFxChipSpec kFrac{
            "FRACTAL GRANULAR PROCESSOR", "M2", "FRACTAL GRANULAR CLOUD DUST", "PRESET: DUST CLOUD",
            {{{"FractalMorph", "DENSITY"}, {"FractalBaseDelayMs", "GRAIN SIZE"}, {"FractalRatio", "PITCH"},
              {"FractalSpreadMs", "SCATTER"}, {nullptr, "POSITION"}, {"Mix", "MIX"}}},
            {},
            0};
        static constexpr DesignFxChipSpec kRev{
            "REVERB SPACE DESIGNER", "I8", "FREQUENCY SPECTRAL DECAY ENVELOPE", "PRESET: LARGE CONCERT HALL",
            {{{"ReverbPreDelayMs", "PREDELAY"}, {"ReverbDecaySeconds", "DECAY"}, {"ReverbSize", "SIZE"},
              {"ReverbHighRatio", "DAMPING"}, {"ReverbDiffusion", "DIFFUSION"}, {"Mix", "MIX"}}},
            {"HALL", "PLATE", "ROOM", "SPRING", "SHIMMER", nullptr},
            5};
        static constexpr DesignFxChipSpec kEq{
            "PARAMETRIC EQUALIZER", "M3", "EQ FREQUENCY RESPONSE", "PRESET: MASTER CLEAN-UP",
            {{{"EqLowGainDb", "LOW"}, {"EqMidGainDb", "MID"}, {"EqHighGainDb", "HIGH"}, {"EqMidQ", "Q"},
              {"Mix", "MIX"}, {nullptr, "OUT GAIN"}}},
            {},
            0};
        static constexpr DesignFxChipSpec kComp{
            "DYNAMICS COMPRESSOR", "M4", "COMPRESSOR TRANSFER & GAIN REDUCTION", "PRESET: BUS GLUE",
            {{{"CompThresholdDb", "THRESHOLD"}, {"CompRatio", "RATIO"}, {"CompAttackMs", "ATTACK"},
              {"CompReleaseMs", "RELEASE"}, {"CompKneeDb", "KNEE"}, {"CompMakeupDb", "MAKEUP"}}},
            {"VCA", "FET", "OPTO", nullptr, nullptr, nullptr},
            3};
        static constexpr DesignFxChipSpec kLim{
            "BRICKWALL LIMITER", "M4", "TRUE PEAK CEILING METER", "PRESET: TRUE PEAK MAXIMIZER",
            {{{"LimiterCeilingDb", "CEILING"}, {"LimiterReleaseMs", "RELEASE"}, {"LimiterLookaheadMs", "LOOKAHEAD"},
              {nullptr, "ISP"}, {nullptr, "GAIN"}, {"Mix", "MIX"}}},
            {},
            0};
        static constexpr DesignFxChipSpec kVoc{
            "VOCODER LAB", "I3", "VOCODER BAND SPECTRUM", "PRESET: TALKBOX CLASSIC",
            {{{"VocoderBandCount", "BANDS"}, {"VocoderFormant", "FORMANT"}, {"VocoderReleaseMs", "RELEASE"},
              {"VocoderScGainDb", "SC GAIN"}, {"VocoderSibilance", "SIBILANCE"}, {"Mix", "MIX"}}},
            {},
            0};

        switch (chipIndex)
        {
            case 1: return kSat;
            case 2: return kChr;
            case 3: return kTape;
            case 4: return kMood;
            case 5: return kFshf;
            case 6: return kFrac;
            case 7: return kRev;
            case 8: return kEq;
            case 9: return kComp;
            case 10: return kLim;
            case 11: return kVoc;
            default: return kBypass;
        }
    }

    [[nodiscard]] inline int reverbCharacterFromDesignPill(const juce::String& pill) noexcept
    {
        if (pill == "HALL") return 2;
        if (pill == "PLATE") return 1;
        if (pill == "ROOM") return 3;
        if (pill == "SPRING") return 4;
        return 0;
    }

    [[nodiscard]] inline int saturationCharacterFromDesignPill(const juce::String& pill) noexcept
    {
        if (pill == "TAPE") return 1;
        if (pill == "DIODE") return 2;
        if (pill == "FOLD") return 3;
        if (pill == "CRUSH") return 4;
        return 0;
    }

    [[nodiscard]] inline juce::String saturationDesignPillFromCharacter(int character) noexcept
    {
        switch (character)
        {
            case 1: return "TAPE";
            case 2: return "DIODE";
            case 3: return "FOLD";
            case 4: return "CRUSH";
            default: return "TUBE";
        }
    }

    [[nodiscard]] inline int compCharacterFromDesignPill(const juce::String& pill) noexcept
    {
        if (pill == "FET") return 1;
        if (pill == "OPTO") return 2;
        return 0;
    }

    [[nodiscard]] inline juce::String compDesignPillFromCharacter(int character) noexcept
    {
        return character == 1 ? "FET" : character == 2 ? "OPTO" : "VCA";
    }

    [[nodiscard]] inline juce::String reverbDesignPillFromCharacter(int character, float modDepth) noexcept
    {
        switch (character)
        {
            case 1: return "PLATE";
            case 2: return "HALL";
            case 3: return "ROOM";
            case 4: return "SPRING";
            default: return modDepth > 0.65f ? "SHIMMER" : "HALL";
        }
    }

    [[nodiscard]] inline std::size_t fxDesignPresetCount(std::size_t chipIndex) noexcept
    {
        switch (chipIndex)
        {
            case 1: return 4;
            case 2: return 3;
            case 3: return 3;
            case 4: return 5;
            case 5: return 3;
            case 6: return 3;
            case 7: return 5;
            case 8: return 4;
            case 9: return 3;
            case 10: return 3;
            case 11: return 3;
            default: return 1;
        }
    }

    [[nodiscard]] inline const char* fxDesignPresetLabel(std::size_t chipIndex, std::size_t presetIndex) noexcept
    {
        static constexpr const char* kSat[] = {"CLASS A TUBE CRUNCH", "TAPE SATURATOR", "DIODE CRUNCH", "FOLD DESTROYER"};
        static constexpr const char* kChr[] = {"WIDE ENSEMBLE", "SUBTLE DOUBLER", "DEEP MOD CHORUS"};
        static constexpr const char* kTape[] = {"VINTAGE REEL", "WORN CASSETTE", "STUDIO SLAPBACK"};
        static constexpr const char* kMood[] = {"WARM ANALOG GLUE", "DARK RESONANT", "BRIGHT SHIMMER", "ACID RESONANCE",
                                               "ETHEREAL AIR"};
        static constexpr const char* kFshf[] = {"BODE CASCADE", "SUB OCTAVE DRIFT", "STEREO PHASER SHIFT"};
        static constexpr const char* kFrac[] = {"DUST CLOUD", "CRYSTAL SHARDS", "GRAIN STORM"};
        static constexpr const char* kRev[] = {"LARGE CONCERT HALL", "PLATE VOCAL", "SMALL ROOM", "SPRING TANK",
                                               "SHIMMER PAD"};
        static constexpr const char* kEq[] = {"MASTER CLEAN-UP", "LOW CUT RUMBLE", "AIR BOOST", "MID SCOOP"};
        static constexpr const char* kComp[] = {"BUS GLUE", "VCA PUNCH", "FET SQUEEZE"};
        static constexpr const char* kLim[] = {"TRUE PEAK MAXIMIZER", "TRANSPARENT CEILING", "LOUD MASTER"};
        static constexpr const char* kVoc[] = {"TALKBOX CLASSIC", "ROBOT VOICES", "FORMANT MORPH"};

        const char* const* table = nullptr;
        std::size_t count = 1;
        switch (chipIndex)
        {
            case 1: table = kSat; count = 4; break;
            case 2: table = kChr; count = 3; break;
            case 3: table = kTape; count = 3; break;
            case 4: table = kMood; count = 5; break;
            case 5: table = kFshf; count = 3; break;
            case 6: table = kFrac; count = 3; break;
            case 7: table = kRev; count = 5; break;
            case 8: table = kEq; count = 4; break;
            case 9: table = kComp; count = 3; break;
            case 10: table = kLim; count = 3; break;
            case 11: table = kVoc; count = 3; break;
            default: return "DRY INSERT";
        }
        return table[presetIndex % count];
    }

    /// Maps MOOD ui knobs + pill to insert-slot Eq params (lightweight spectral macro).
    inline void applyMoodKnobsToEq(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                   const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float intensity = uiState.knobValue(4, 0);
        const float colorKnob = uiState.knobValue(4, 1);
        const float filterT = uiState.knobValue(4, 2);
        const float resonance = uiState.knobValue(4, 3);
        const float drive = uiState.knobValue(4, 4);
        const juce::String mode = uiState.moodPill();

        float lowGain = juce::jmap(colorKnob, 0.0f, 1.0f, -4.0f, 6.0f) + drive * 4.0f;
        float midGain = juce::jmap(intensity, 0.0f, 1.0f, -8.0f, 10.0f);
        float highGain = juce::jmap(colorKnob, 0.0f, 1.0f, 6.0f, -4.0f) + drive * 2.0f;
        float midFreq = 220.0f * std::pow(40.0f, filterT);
        float midQ = 0.35f + resonance * 7.5f;
        float lowFreq = 120.0f + filterT * 80.0f;
        float highFreq = 4000.0f + filterT * 12000.0f;

        if (mode == "DARK")
        {
            lowGain += 3.0f;
            midGain -= 2.0f;
            highGain -= 5.0f;
            midFreq *= 0.75f;
        }
        else if (mode == "BRIGHT")
        {
            lowGain -= 2.0f;
            highGain += 5.0f;
            midFreq *= 1.35f;
        }
        else if (mode == "ACID")
        {
            midGain += 6.0f;
            midQ += 2.5f;
            midFreq = juce::jlimit(400.0f, 3200.0f, midFreq);
        }
        else if (mode == "ETHEREAL")
        {
            highGain += 4.0f;
            midGain *= 0.55f;
            midQ *= 0.65f;
        }

        setF("EqLowFreqHz", juce::jlimit(40.0f, 400.0f, lowFreq));
        setF("EqLowGainDb", juce::jlimit(-18.0f, 18.0f, lowGain));
        setF("EqMidFreqHz", juce::jlimit(120.0f, 12000.0f, midFreq));
        setF("EqMidGainDb", juce::jlimit(-18.0f, 18.0f, midGain));
        setF("EqMidQ", juce::jlimit(0.2f, 8.0f, midQ));
        setF("EqHighFreqHz", juce::jlimit(2000.0f, 18000.0f, highFreq));
        setF("EqHighGainDb", juce::jlimit(-18.0f, 18.0f, highGain));
        setF("Mix", juce::jlimit(0.0f, 1.0f, 0.35f + intensity * 0.55f + drive * 0.15f));
    }

    /// Maps EQ ui stub (OUT GAIN) to insert-slot Eq params.
    inline void applyEqStubKnobs(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                 const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float outGainNorm = uiState.knobValue(8, 5);
        setF("EqOutGainDb", juce::jmap(outGainNorm, 0.0f, 1.0f, -12.0f, 12.0f));
    }

    /// Maps LIM ui stubs (ISP/GAIN) to limiter APVTS; true-peak mode extends lookahead range.
    inline void applyLimStubKnobs(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                  const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float isp = uiState.knobValue(10, 3);
        const float gain = uiState.knobValue(10, 4);
        const bool truePeak = uiState.limTruePeakActive();
        const float lookaheadMax = truePeak ? 14.0f : 6.0f;
        setF("LimiterLookaheadMs", juce::jmap(isp, 0.0f, 1.0f, 1.5f, lookaheadMax));
        setF("LimiterCeilingDb", juce::jmap(gain, 0.0f, 1.0f, -8.0f, -0.1f));
    }

    /// Maps SAT ui stubs (TONE/COLOR/OUTPUT/BIAS) to Saturation APVTS alongside DRIVE/MIX knobs.
    inline void applySatStubKnobs(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                  const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float drive = uiState.knobValue(1, 0);
        const float tone = uiState.knobValue(1, 1);
        const float color = uiState.knobValue(1, 2);
        const float mixKnob = uiState.knobValue(1, 3);
        const float output = uiState.knobValue(1, 4);
        const float bias = uiState.knobValue(1, 5);

        const float driveDb = juce::jmap(drive, 0.0f, 1.0f, 0.0f, 48.0f) * juce::jmap(tone, 0.0f, 1.0f, 0.55f, 1.45f)
                              + juce::jmap(bias, 0.0f, 1.0f, -12.0f, 12.0f);
        setF("SaturationDrive", juce::jlimit(0.0f, 48.0f, driveDb));
        setF("SaturationCharacter", static_cast<float>(juce::jlimit(0, 4, static_cast<int>(color * 4.99f))));
        setF("Mix", juce::jlimit(0.0f, 1.0f, mixKnob * juce::jmap(output, 0.0f, 1.0f, 0.35f, 1.0f)));
    }

    inline void applyChrStubKnobs(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                  const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float feedback = uiState.knobValue(2, 2);
        const float voices = uiState.knobValue(2, 4);
        const float rate = uiState.knobValue(2, 0);
        setF("ChorusDepth", juce::jmap(feedback, 0.0f, 1.0f, 1.0f, 16.0f));
        setF("ChorusBaseDelay", juce::jmap(voices, 0.0f, 1.0f, 8.0f, 32.0f));
        setF("ChorusRate", juce::jmap(rate, 0.0f, 1.0f, 0.08f, 2.5f));
    }

    inline void applyTapeStubKnobs(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                   const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float flutter = uiState.knobValue(3, 2);
        const float wow = uiState.knobValue(3, 1);
        setF("TapeDriftRate", juce::jmap(flutter, 0.0f, 1.0f, 0.2f, 9.0f));
        setF("TapeDriftDepth", juce::jmap(wow, 0.0f, 1.0f, 0.2f, 6.0f));
    }

    inline void applyFshfStubKnobs(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                     const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float shift = uiState.knobValue(5, 0);
        const float detune = uiState.knobValue(5, 4);
        const float stereo = uiState.knobValue(5, 5);
        const float baseHz = juce::jmap(shift, 0.0f, 1.0f, -48.0f, 48.0f);
        const float detuneHz = juce::jmap(detune, 0.0f, 1.0f, -12.0f, 12.0f);
        setF("FreqShiftHz", baseHz + detuneHz * 0.35f);
        setF("FreqShiftHighCutHz", juce::jmap(stereo, 0.0f, 1.0f, 1800.0f, 16000.0f));
        setF("FreqShiftLowCutHz", juce::jmap(stereo, 0.0f, 1.0f, 420.0f, 80.0f));
    }

    inline void applyFracStubKnobs(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                     const DesignFxUiState& uiState)
    {
        if (paramPrefix.isEmpty())
            return;

        const auto setF = [&](const char* suffix, float value) {
            if (auto* param = apvts.getParameter(paramPrefix + suffix))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        const float density = uiState.knobValue(6, 0);
        const float position = uiState.knobValue(6, 4);
        setF("FractalMorph", juce::jlimit(0.0f, 1.0f, density * 0.65f + position * 0.35f));
        setF("FractalSpreadMs", juce::jmap(position, 0.0f, 1.0f, 4.0f, 48.0f));
    }

} // namespace pw8::plugin::ui
