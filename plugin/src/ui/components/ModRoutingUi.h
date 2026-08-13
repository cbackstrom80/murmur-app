#pragma once

#include <cstdint>
#include <optional>

#include <juce_core/juce_core.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/patch/Patch.hpp"

namespace pw8::plugin::ui
{
    inline constexpr std::size_t kStandardKoinCount = 6;
    inline constexpr std::size_t kMinimumKoinCount = 4;

    struct ModDestinationParam
    {
        juce::String paramId;
        juce::String label;
    };

    [[nodiscard]] float defaultModAmountFor(modulation::ModDestination destination) noexcept;

    struct ModAmountRange
    {
        float min = -1.0f;
        float max = 1.0f;
    };

    [[nodiscard]] ModAmountRange modAmountRangeFor(modulation::ModDestination destination) noexcept;

    void assignModRoute(PatchworkEightProcessor& processor, modulation::ModSource source,
                        modulation::ModDestination destination, std::uint8_t targetIndex);

    void assignModRoute(PatchworkEightProcessor& processor, modulation::ModSource source,
                        modulation::ModDestination destination, std::uint8_t targetIndex, float amount);

    void updateModRouteAmount(PatchworkEightProcessor& processor, const modulation::ModRoute& route, float amount);

    [[nodiscard]] std::optional<ModDestinationParam> modDestinationParam(modulation::ModDestination destination,
                                                                           std::uint8_t targetIndex);

    enum class PatchFocusKnobKind
    {
        Macro,
        ApvtsParam,
    };

    struct PatchFocusKnobSpec
    {
        PatchFocusKnobKind kind = PatchFocusKnobKind::Macro;
        std::size_t macroIndex = 0;
        juce::String paramId;
        juce::String label;

        [[nodiscard]] bool operator==(const PatchFocusKnobSpec& other) const noexcept
        {
            return kind == other.kind && macroIndex == other.macroIndex && paramId == other.paramId &&
                   label == other.label;
        }
    };

    /// When `apvtsForValidation` is set, param knobs whose ids are missing from APVTS are skipped.
    [[nodiscard]] std::vector<PatchFocusKnobSpec> inferPatchFocusKnobs(
        const patch::Patch& patch, std::size_t maxKnobs = 8,
        const juce::AudioProcessorValueTreeState* apvtsForValidation = nullptr);

    [[nodiscard]] juce::String formatModRouteAmount(modulation::ModDestination destination, float amount);

    /// First active mod-wheel route on Layer A, if any.
    [[nodiscard]] std::optional<modulation::ModRoute> findModWheelRoute(const patch::Patch& patch) noexcept;

    [[nodiscard]] juce::String formatModWheelStatus(const patch::Patch& patch, float modWheelValue01) noexcept;

    [[nodiscard]] std::optional<modulation::ModRoute> findExpressionRoute(const patch::Patch& patch) noexcept;

    [[nodiscard]] juce::String formatExpressionStatus(const patch::Patch& patch, float expressionValue01) noexcept;

} // namespace pw8::plugin::ui
