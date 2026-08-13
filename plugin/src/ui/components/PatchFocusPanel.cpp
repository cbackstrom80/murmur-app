#include "PatchFocusPanel.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    PatchFocusPanel::PatchFocusPanel(PatchworkEightProcessor& processor) : processor_(processor)
    {
        addAndMakeVisible(panel_);

        introLabel_.setText("This patch's main controls — turn these while you play.",
                            juce::dontSendNotification);
        introLabel_.setJustificationType(juce::Justification::centredLeft);
        introLabel_.setFont(fonts::value(11.5f));
        introLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        panel_.addAndMakeVisible(introLabel_);

        panel_.addAndMakeVisible(subtitleLabel_);
        subtitleLabel_.setJustificationType(juce::Justification::centredLeft);
        subtitleLabel_.setFont(fonts::value(11.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);

        advancedButton_.setButtonText("Mod Matrix (M)");
        advancedButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        advancedButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        advancedButton_.onClick = [this] {
            if (onAdvancedClicked)
                onAdvancedClicked();
        };
        panel_.addAndMakeVisible(advancedButton_);
        advancedButton_.setVisible(false);

        setBasicPerformanceLayout(true);
        startTimerHz(2);
        timerCallback();
    }

    PatchFocusPanel::~PatchFocusPanel()
    {
        stopTimer();
    }

    void PatchFocusPanel::setBasicPerformanceLayout(bool basicLayout)
    {
        basicLayout_ = basicLayout;
        introLabel_.setVisible(basicLayout);
        applyLayoutMode();
        resized();
    }

    void PatchFocusPanel::applyLayoutMode()
    {
        const int dialCap = basicLayout_ ? 120 : 72;
        for (auto& knob : knobs_)
            knob->setMaxDialDiameter(dialCap);
    }

    void PatchFocusPanel::timerCallback()
    {
        const auto& patch = processor_.getCurrentPatch();
        const auto specs = inferPatchFocusKnobs(patch, basicLayout_ ? 8 : 6);

        const auto patchName = patch.metadata.name.empty() ? juce::String("Init") : juce::String(patch.metadata.name);
        const bool authored = !patch.uiFocus.knobs.empty();
        subtitleLabel_.setText("Playing " + patchName + " — " + juce::String(static_cast<int>(specs.size())) +
                                   (authored ? " patch-authored controls" : " inferred controls"),
                               juce::dontSendNotification);

        if (specs != lastSpecs_)
        {
            lastSpecs_ = specs;
            rebuildKnobs(specs);
            applyLayoutMode();
            resized();
        }
    }

    void PatchFocusPanel::rebuildKnobs(const std::vector<PatchFocusKnobSpec>& specs)
    {
        knobs_.clear();
        panel_.removeAllChildren();
        panel_.addAndMakeVisible(introLabel_);
        panel_.addAndMakeVisible(subtitleLabel_);
        panel_.addAndMakeVisible(advancedButton_);

        for (const auto& spec : specs)
        {
            std::unique_ptr<GlowKnob> knob;
            if (spec.kind == PatchFocusKnobKind::Macro)
            {
                knob = std::make_unique<GlowKnob>(processor_.apvts, kMacroParameterIds[spec.macroIndex], spec.label,
                                                  nullptr, palette::kAccentWarm);
            }
            else
            {
                knob = std::make_unique<GlowKnob>(processor_.apvts, spec.paramId, spec.label);
            }
            panel_.addAndMakeVisible(*knob);
            knobs_.push_back(std::move(knob));
        }
    }

    void PatchFocusPanel::resized()
    {
        panel_.setBounds(getLocalBounds());

        auto bounds = panel_.getContentBounds().reduced(4, 0);
        if (introLabel_.isVisible())
        {
            introLabel_.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(4);
        }

        auto header = bounds.removeFromTop(22);
        subtitleLabel_.setBounds(header);

        bounds.removeFromTop(basicLayout_ ? 8 : 4);

        if (knobs_.empty())
            return;

        constexpr int kMinKnobCellWidthBasic = 140;
        constexpr int kMinKnobCellWidthCompact = 108;
        const int minCellWidth = basicLayout_ ? kMinKnobCellWidthBasic : kMinKnobCellWidthCompact;

        int columns = static_cast<int>(knobs_.size());
        int rows = 1;
        if (columns > 1 && bounds.getWidth() / columns < minCellWidth)
        {
            columns = juce::jmax(1, bounds.getWidth() / minCellWidth);
            rows = static_cast<int>((knobs_.size() + static_cast<std::size_t>(columns) - 1) /
                                    static_cast<std::size_t>(columns));
        }

        const int cellWidth = bounds.getWidth() / juce::jmax(1, columns);
        const int cellHeight = bounds.getHeight() / juce::jmax(1, rows);

        for (std::size_t i = 0; i < knobs_.size(); ++i)
        {
            const int col = static_cast<int>(i) % columns;
            const int row = static_cast<int>(i) / columns;
            knobs_[i]->setBounds(bounds.getX() + col * cellWidth, bounds.getY() + row * cellHeight, cellWidth,
                                 cellHeight);
        }
    }

} // namespace pw8::plugin::ui
