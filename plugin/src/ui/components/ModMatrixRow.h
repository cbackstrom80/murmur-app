#pragma once

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../theme/ObsidianMatrixLookAndFeel.h"
#include "processor/PatchworkEightProcessor.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    struct ModMatrixDestChoice
    {
        modulation::ModDestination destination = modulation::ModDestination::None;
        bool usesTargetIndex = false;
        juce::String label;
    };

    [[nodiscard]] std::vector<ModMatrixDestChoice> allModMatrixDestChoices();
    [[nodiscard]] std::vector<modulation::ModSource> allModMatrixSources();

    /// Single mod-matrix route row — deluxe glass styling wired to live patch routes.
    class ModMatrixRow : public juce::Component
    {
    public:
        using RebuildCallback = std::function<void()>;

        ModMatrixRow(PatchworkEightProcessor& processor, const modulation::ModRoute& route, bool enabled,
                     const std::vector<ModMatrixDestChoice>& destChoices,
                     std::vector<modulation::ModRoute>& disabledRoutes, RebuildCallback onRebuild);

        ~ModMatrixRow() override;

        void syncFromRoute();
        [[nodiscard]] const modulation::ModRoute& route() const noexcept { return route_; }

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void setupMenu(juce::ComboBox& box, const juce::String& placeholder);
        void updateTargetVisibility();
        void updateAmountSlider();
        [[nodiscard]] std::optional<modulation::ModRoute> findLiveRoute() const;
        [[nodiscard]] bool isRowActive() const noexcept;

        void handleBypassToggle();
        void commitSourceChange();
        void commitDestChange();
        void commitTargetChange();
        void commitAmountChange();

        PatchworkEightProcessor& processor_;
        const std::vector<ModMatrixDestChoice>& destChoices_;
        std::vector<modulation::ModRoute>& disabledRoutes_;
        RebuildCallback onRebuild_;

        modulation::ModRoute route_;
        bool enabled_ = true;

        ObsidianMatrixLookAndFeel matrixLookAndFeel_;
        juce::ToggleButton bypassButton_;
        juce::ComboBox sourceCombo_;
        juce::ComboBox destCombo_;
        juce::ComboBox targetCombo_;
        juce::Slider amountSlider_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixRow)
    };

} // namespace pw8::plugin::ui
