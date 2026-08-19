#pragma once

#include <algorithm>

#include "../PlayModeLayout.h"

namespace pw8::plugin::ui::figma
{
    /// KOIN HUD radial frame — large center readout, thin peripheral glow arcs (8-ENGINE mockup).
    /// Canonical @ 180px hero frame; rings sit near the rim with clear dead space inside the cap.
    struct DualRingRatios
    {
        static constexpr float outer = 172.0f / 180.0f;
        static constexpr float middle = 158.0f / 180.0f;
        static constexpr float cap = 128.0f / 180.0f;
    };

    struct TripleRingRatios
    {
        static constexpr float outer = 172.0f / 180.0f;
        static constexpr float middle = 162.0f / 180.0f;
        static constexpr float inner = 152.0f / 180.0f;
        static constexpr float cap = 128.0f / 180.0f;
    };

    /// Thin HUD stroke weights @ 180px — rings read as ~1/10 cap diameter, not chunky decks.
    struct HeroStrokeAt180
    {
        static constexpr float frameDiameter = 180.0f;
        static constexpr float dualOuter = 3.5f;
        static constexpr float dualMiddle = 3.0f;
        static constexpr float tripleOuter = 3.5f;
        static constexpr float tripleMiddle = 3.0f;
        static constexpr float tripleInner = 2.5f;
    };

    /// KOIN functional ring colours (8-ENGINE neon HUD).
    inline const juce::Colour kKoinRingOuter{0xff00ffd2};
    inline const juce::Colour kKoinRingMiddle{0xff9f80ff};
    inline const juce::Colour kKoinRingInner{0xffff9d00};

    [[nodiscard]] inline float scaledHeroStroke(float heroStroke, float dialDiameter) noexcept
    {
        const float scale = dialDiameter / HeroStrokeAt180::frameDiameter;
        return std::max(heroStroke * 0.42f, heroStroke * scale);
    }

    enum class DeckSize
    {
        Small,
        Medium,
        Large,
    };

    enum class KnobContext
    {
        ChromeMaster,
        CompactMacro,
        CompactMaster,
        EngineCard,
        DashboardFilter,
        DesignFxDetail,
        DesignVocoder,
        DesktopMacroFeature,
        DesktopMacroStandard,
        ArpTiming,
        PanelGridMedium,
        WavetableTuning,
        WavetableUnison,
        LfoMotionDual,
        PlayBlades,
        BasicPortamento,
        BasicMacro,
        UtilityPeaks,
        DesignLabGrid,
        OperatorHero,
        PlayHeaderMaster,
        DesignEngineMaster,
        DesignEngineCard,
    };

    struct KnobSpec
    {
        int diameter = 64;
        DeckSize deckSize = DeckSize::Medium;
        float labelFontPt = 11.0f;
        bool headerCompact = false;
        bool hideValueBox = false;
    };

    [[nodiscard]] inline KnobSpec specFor(KnobContext ctx) noexcept
    {
        switch (ctx)
        {
            case KnobContext::ChromeMaster:
                return {layout::kChromeMasterKnobSize, DeckSize::Small, 8.0f, true, true};
            case KnobContext::CompactMacro:
                return {layout::kCompactMacroKnobSize, DeckSize::Small, 10.0f, false, true};
            case KnobContext::CompactMaster:
                return {layout::kCompactMasterKnobSize, DeckSize::Small, 10.0f, false, true};
            case KnobContext::EngineCard:
                return {layout::kEngineCardKnobDialSize, DeckSize::Small, 8.0f, false, true};
            case KnobContext::DashboardFilter:
                return {layout::kDashboardGlobalFilterKnobSize, DeckSize::Small, 9.0f, false, true};
            case KnobContext::DesignFxDetail:
                return {layout::kDesignFxPageDetailKnobDialSize, DeckSize::Small, 8.0f, false, true};
            case KnobContext::DesignVocoder:
                return {layout::kDesignVocoderKnobDiameter, DeckSize::Medium, 9.0f, false, true};
            case KnobContext::DesktopMacroFeature:
                return {layout::kDesktopPlayModeMacroKnobTouchSize, DeckSize::Large, 11.0f, false, true};
            case KnobContext::DesktopMacroStandard:
                return {layout::kDesktopPlayModeMacroKnobTouchSize, DeckSize::Medium, 10.0f, false, true};
            case KnobContext::ArpTiming:
                return {layout::kArpTimingKnobDialSize, DeckSize::Small, 8.0f, false, true};
            case KnobContext::PanelGridMedium:
                return {56, DeckSize::Medium, 9.0f, false, true};
            case KnobContext::WavetableTuning:
                return {56, DeckSize::Small, 8.0f, false, true};
            case KnobContext::WavetableUnison:
                return {64, DeckSize::Medium, 8.0f, false, true};
            case KnobContext::LfoMotionDual:
                return {72, DeckSize::Medium, 9.0f, false, true};
            case KnobContext::PlayBlades:
                return {layout::kPlayBladesKnobDialSize, DeckSize::Medium, 8.0f, false, true};
            case KnobContext::BasicPortamento:
                return {layout::kMurmurBasicViewPortamentoKnobSize, DeckSize::Medium, 9.0f, false, true};
            case KnobContext::BasicMacro:
                return {layout::kMurmurBasicViewMacroKnobSize, DeckSize::Large, 10.0f, false, true};
            case KnobContext::UtilityPeaks:
                return {layout::kDesignUtilityPeaksKnobSize, DeckSize::Medium, 8.0f, false, true};
            case KnobContext::DesignLabGrid:
                return {52, DeckSize::Medium, 9.0f, false, true};
            case KnobContext::OperatorHero:
                return {64, DeckSize::Medium, 9.0f, false, true};
            case KnobContext::PlayHeaderMaster:
                return {40, DeckSize::Small, 9.0f, true, true};
            case KnobContext::DesignEngineMaster:
                return {layout::kDesignModeV2MasterEnvelopeKnobSize, DeckSize::Medium, 9.0f, false, true};
            case KnobContext::DesignEngineCard:
                return {layout::kDesignModeV2CardKnobDialSize, DeckSize::Small, 7.0f, false, true};
            default:
                return {};
        }
    }

    [[nodiscard]] inline int chromeMasterDialDiameter() noexcept { return layout::kChromeMasterKnobSize; }
    [[nodiscard]] inline int compactMasterDialDiameter() noexcept { return layout::kCompactMasterKnobSize; }
    [[nodiscard]] inline int compactMacroDialDiameter() noexcept { return layout::kCompactMacroKnobSize; }
    [[nodiscard]] inline int macroFeatureDialDiameter() noexcept { return layout::kDesktopPlayModeMacroKnobTouchSize; }
    [[nodiscard]] inline int macroStandardDialDiameter() noexcept { return 72; }
    [[nodiscard]] inline int engineCardDialDiameter() noexcept { return layout::kEngineCardKnobDialSize; }
    [[nodiscard]] inline int globalFilterDialDiameter() noexcept { return layout::kDashboardGlobalFilterKnobSize; }
    [[nodiscard]] inline int arpTimingDialDiameter() noexcept { return layout::kArpTimingKnobDialSize; }
    [[nodiscard]] inline int designFxDetailDialDiameter() noexcept { return layout::kDesignFxPageDetailKnobDialSize; }
    [[nodiscard]] inline int designVocoderDialDiameter() noexcept { return layout::kDesignVocoderKnobDiameter; }

    [[nodiscard]] inline DeckSize deckSizeForDiameter(int diameter) noexcept
    {
        if (diameter >= 88)
            return DeckSize::Large;
        if (diameter <= 36)
            return DeckSize::Small;
        return DeckSize::Medium;
    }

} // namespace pw8::plugin::ui::figma
