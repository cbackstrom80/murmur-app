#include "ModMatrixRow.h"

#include <algorithm>

#include "ModRoutingUi.h"
#include "ModSourceChip.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/core/Types.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] juce::String formatAmountForSlider(modulation::ModDestination destination, float amount)
        {
            const auto range = modAmountRangeFor(destination);
            if (range.min >= -1.01f && range.max <= 1.01f)
            {
                const float pct = juce::jlimit(-100.0f, 100.0f, amount * 100.0f);
                const auto sign = pct >= 0.0f ? "+" : "";
                return sign + juce::String(pct, 0) + "%";
            }
            return formatModRouteAmount(destination, amount);
        }
    } // namespace

    std::vector<ModMatrixDestChoice> allModMatrixDestChoices()
    {
        return {
            {modulation::ModDestination::FilterCutoff, false, "Global Filter Cutoff"},
            {modulation::ModDestination::FilterResonance, false, "Global Filter Resonance"},
            {modulation::ModDestination::OperatorFilterCutoff, true, "Engine Filter Cutoff"},
            {modulation::ModDestination::OperatorFilterResonance, true, "Engine Filter Resonance"},
            {modulation::ModDestination::OperatorLevel, true, "Operator Level"},
            {modulation::ModDestination::OperatorWavetablePosition, true, "WT Position"},
            {modulation::ModDestination::OperatorWavetableBend, true, "WT Bend"},
            {modulation::ModDestination::OperatorWavetableAsymmetry, true, "WT Asymmetry"},
            {modulation::ModDestination::OperatorWavetableSyncRatio, true, "WT Sync Ratio"},
            {modulation::ModDestination::OperatorWavetableSyncAmount, true, "WT Sync Amt"},
            {modulation::ModDestination::OperatorWavetableFormant, true, "WT Formant"},
            {modulation::ModDestination::Pan, false, "Layer Pan"},
        };
    }

    std::vector<modulation::ModSource> allModMatrixSources()
    {
        return {
            modulation::ModSource::ModWheel,       modulation::ModSource::Expression,
            modulation::ModSource::Velocity,       modulation::ModSource::ChannelPressure,
            modulation::ModSource::PolyAftertouch, modulation::ModSource::MpeSlide,
            modulation::ModSource::Lfo1,           modulation::ModSource::Lfo2,
            modulation::ModSource::Lfo3,           modulation::ModSource::Lfo4,
            modulation::ModSource::Lfo5,           modulation::ModSource::Lfo6,
            modulation::ModSource::Lfo7,           modulation::ModSource::Lfo8,
            modulation::ModSource::Env1,           modulation::ModSource::Env2,
            modulation::ModSource::Env3,           modulation::ModSource::Env4,
            modulation::ModSource::Env5,           modulation::ModSource::Env6,
            modulation::ModSource::Env7,           modulation::ModSource::Env8,
            modulation::ModSource::Macro1,         modulation::ModSource::Macro2,
            modulation::ModSource::Macro3,         modulation::ModSource::Macro4,
            modulation::ModSource::Macro5,         modulation::ModSource::Macro6,
            modulation::ModSource::Macro7,         modulation::ModSource::Macro8,
        };
    }

    ModMatrixRow::ModMatrixRow(PatchworkEightProcessor& processor, const modulation::ModRoute& route, bool enabled,
                               const std::vector<ModMatrixDestChoice>& destChoices,
                               std::vector<modulation::ModRoute>& disabledRoutes, RebuildCallback onRebuild)
        : processor_(processor),
          destChoices_(destChoices),
          disabledRoutes_(disabledRoutes),
          onRebuild_(std::move(onRebuild)),
          route_(route),
          enabled_(enabled)
    {
        setLookAndFeel(&matrixLookAndFeel_);

        bypassButton_.setButtonText("");
        bypassButton_.setClickingTogglesState(true);
        bypassButton_.setToggleState(!enabled_, juce::dontSendNotification);
        bypassButton_.onClick = [this] { handleBypassToggle(); };
        addAndMakeVisible(bypassButton_);

        setupMenu(sourceCombo_, "Select Source...");
        for (const auto source : allModMatrixSources())
            sourceCombo_.addItem(modSourceLabel(source), static_cast<int>(source));
        sourceCombo_.onChange = [this] { commitSourceChange(); };
        addAndMakeVisible(sourceCombo_);

        setupMenu(destCombo_, "Select Destination...");
        for (int i = 0; i < static_cast<int>(destChoices_.size()); ++i)
            destCombo_.addItem(destChoices_[static_cast<std::size_t>(i)].label, i + 1);
        destCombo_.onChange = [this] { commitDestChange(); };
        addAndMakeVisible(destCombo_);

        setupMenu(targetCombo_, "Op...");
        for (int i = 0; i < static_cast<int>(core::kNodesPerLayer); ++i)
            targetCombo_.addItem("Op " + juce::String(i), i + 1);
        targetCombo_.onChange = [this] { commitTargetChange(); };
        addAndMakeVisible(targetCombo_);

        amountSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        amountSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 45, 18);
        amountSlider_.setColour(juce::Slider::textBoxTextColourId, palette::kTextSecondary);
        amountSlider_.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        amountSlider_.onValueChange = [this] { commitAmountChange(); };
        addAndMakeVisible(amountSlider_);

        syncFromRoute();
    }

    ModMatrixRow::~ModMatrixRow()
    {
        setLookAndFeel(nullptr);
    }

    void ModMatrixRow::setupMenu(juce::ComboBox& box, const juce::String& placeholder)
    {
        box.setTextWhenNothingSelected(placeholder);
        box.setJustificationType(juce::Justification::centred);
    }

    void ModMatrixRow::syncFromRoute()
    {
        route_ = findLiveRoute().value_or(route_);
        enabled_ = route_.isActive();
        bypassButton_.setToggleState(!enabled_, juce::dontSendNotification);

        if (sourceCombo_.indexOfItemId(static_cast<int>(route_.source)) >= 0)
            sourceCombo_.setSelectedId(static_cast<int>(route_.source), juce::dontSendNotification);

        for (int i = 0; i < static_cast<int>(destChoices_.size()); ++i)
        {
            if (destChoices_[static_cast<std::size_t>(i)].destination == route_.destination)
            {
                destCombo_.setSelectedId(i + 1, juce::dontSendNotification);
                break;
            }
        }

        targetCombo_.setSelectedId(static_cast<int>(route_.targetIndex) + 1, juce::dontSendNotification);
        updateTargetVisibility();
        updateAmountSlider();
    }

    bool ModMatrixRow::isRowActive() const noexcept
    {
        return route_.source != modulation::ModSource::None && route_.destination != modulation::ModDestination::None &&
               !bypassButton_.getToggleState() && route_.isActive();
    }

    void ModMatrixRow::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        const bool active = isRowActive();

        g.setColour(active ? palette::kAccent.withAlpha(0.08f) : palette::kTextPrimary.withAlpha(0.02f));
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(active ? palette::kAccent.withAlpha(0.25f) : palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }

    void ModMatrixRow::resized()
    {
        auto area = getLocalBounds().reduced(8, 4);

        bypassButton_.setBounds(area.removeFromLeft(24).reduced(0, 4));
        area.removeFromLeft(8);

        const bool showTarget = targetCombo_.isVisible();
        const int columns = showTarget ? 4 : 3;
        const int widgetWidth = area.getWidth() / columns;

        sourceCombo_.setBounds(area.removeFromLeft(widgetWidth).reduced(4, 2));
        destCombo_.setBounds(area.removeFromLeft(widgetWidth).reduced(4, 2));

        if (showTarget)
            targetCombo_.setBounds(area.removeFromLeft(widgetWidth).reduced(4, 2));

        amountSlider_.setBounds(area.reduced(4, 2));
    }

    std::optional<modulation::ModRoute> ModMatrixRow::findLiveRoute() const
    {
        for (const auto& live : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (live.destination == route_.destination && live.targetIndex == route_.targetIndex)
                return live;
        }
        for (const auto& disabled : disabledRoutes_)
        {
            if (disabled.destination == route_.destination && disabled.targetIndex == route_.targetIndex)
                return disabled;
        }
        return std::nullopt;
    }

    void ModMatrixRow::updateTargetVisibility()
    {
        const int destIndex = destCombo_.getSelectedId() - 1;
        const bool usesTarget =
            destIndex >= 0 && destIndex < static_cast<int>(destChoices_.size()) &&
            destChoices_[static_cast<std::size_t>(destIndex)].usesTargetIndex;
        if (targetCombo_.isVisible() != usesTarget)
        {
            targetCombo_.setVisible(usesTarget);
            resized();
        }
    }

    void ModMatrixRow::updateAmountSlider()
    {
        const auto range = modAmountRangeFor(route_.destination);
        amountSlider_.setRange(range.min, range.max, (range.max - range.min) / 200.0);
        amountSlider_.setValue(route_.amount, juce::dontSendNotification);
        amountSlider_.textFromValueFunction = [dest = route_.destination](double value) {
            return formatAmountForSlider(dest, static_cast<float>(value));
        };
    }

    void ModMatrixRow::handleBypassToggle()
    {
        if (bypassButton_.getToggleState())
        {
            if (route_.isActive())
            {
                disabledRoutes_.push_back(route_);
                processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
            }
            enabled_ = false;
        }
        else
        {
            for (auto it = disabledRoutes_.begin(); it != disabledRoutes_.end(); ++it)
            {
                if (it->destination == route_.destination && it->targetIndex == route_.targetIndex)
                {
                    const auto restored = *it;
                    disabledRoutes_.erase(it);
                    processor_.setOrReplaceModRouteLive(restored.source, restored.destination, restored.targetIndex,
                                                        restored.amount, restored.scope);
                    route_ = restored;
                    enabled_ = true;
                    syncFromRoute();
                    repaint();
                    return;
                }
            }
            enabled_ = true;
            if (route_.source != modulation::ModSource::None && route_.destination != modulation::ModDestination::None)
            {
                processor_.setOrReplaceModRouteLive(route_.source, route_.destination, route_.targetIndex,
                                                    route_.amount, route_.scope);
            }
        }
        syncFromRoute();
        repaint();
    }

    void ModMatrixRow::commitSourceChange()
    {
        const auto newSource = static_cast<modulation::ModSource>(sourceCombo_.getSelectedId());
        if (newSource == modulation::ModSource::None || newSource == route_.source)
            return;
        if (route_.isActive())
            processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
        route_.source = newSource;
        if (enabled_)
            processor_.setOrReplaceModRouteLive(route_.source, route_.destination, route_.targetIndex, route_.amount,
                                                route_.scope);
        syncFromRoute();
        repaint();
    }

    void ModMatrixRow::commitDestChange()
    {
        const int destIndex = destCombo_.getSelectedId() - 1;
        if (destIndex < 0 || destIndex >= static_cast<int>(destChoices_.size()))
            return;
        const auto& choice = destChoices_[static_cast<std::size_t>(destIndex)];
        if (choice.destination == route_.destination)
        {
            updateTargetVisibility();
            return;
        }
        if (route_.isActive())
            processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
        disabledRoutes_.erase(
            std::remove_if(disabledRoutes_.begin(), disabledRoutes_.end(),
                           [&](const modulation::ModRoute& r) {
                               return r.destination == route_.destination && r.targetIndex == route_.targetIndex;
                           }),
            disabledRoutes_.end());
        route_.destination = choice.destination;
        if (!choice.usesTargetIndex)
            route_.targetIndex = 0;
        route_.amount = defaultModAmountFor(route_.destination);
        if (enabled_)
            processor_.setOrReplaceModRouteLive(route_.source, route_.destination, route_.targetIndex, route_.amount,
                                                route_.scope);
        updateTargetVisibility();
        updateAmountSlider();
        if (onRebuild_)
            onRebuild_();
    }

    void ModMatrixRow::commitTargetChange()
    {
        const auto newTarget = static_cast<std::uint8_t>(
            juce::jlimit(0, static_cast<int>(core::kNodesPerLayer) - 1, targetCombo_.getSelectedId() - 1));
        if (newTarget == route_.targetIndex)
            return;
        if (route_.isActive())
            processor_.removeModRouteLive(route_.source, route_.destination, route_.targetIndex);
        route_.targetIndex = newTarget;
        if (enabled_)
            processor_.setOrReplaceModRouteLive(route_.source, route_.destination, route_.targetIndex, route_.amount,
                                                route_.scope);
        if (onRebuild_)
            onRebuild_();
    }

    void ModMatrixRow::commitAmountChange()
    {
        route_.amount = static_cast<float>(amountSlider_.getValue());
        if (enabled_ && route_.isActive())
            updateModRouteAmount(processor_, route_, route_.amount);
        repaint();
    }

} // namespace pw8::plugin::ui
