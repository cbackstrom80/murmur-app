#include "CompactModeEditor.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    CompactModeEditor::CompactModeEditor(PatchworkEightProcessor& processor)
        : processor_(processor),
          circularScope_(processor),
          focusPanel_(processor)
    {
        patchNameLabel_.setJustificationType(juce::Justification::centred);
        patchNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        patchNameLabel_.setFont(fonts::title(14.0f));
        addAndMakeVisible(patchNameLabel_);

        for (auto* btn : {&prevButton_, &nextButton_})
        {
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            addAndMakeVisible(*btn);
        }
        prevButton_.onClick = [this] { stepPreset(-1); };
        nextButton_.onClick = [this] { stepPreset(1); };

        addAndMakeVisible(circularScope_);
        circularScope_.setViewMode(processor_.getScopeViewMode());

        scopeModeToggle_.setMode(processor_.getScopeViewMode());
        scopeModeToggle_.onModeChanged = [this](ScopeViewMode mode) {
            processor_.setScopeViewMode(mode);
            circularScope_.setViewMode(mode);
        };
        addAndMakeVisible(scopeModeToggle_);

        volumeKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "VOL", nullptr, palette::kAccentWarm);
        volumeKnob_->setMaxDialDiameter(52);
        addAndMakeVisible(*volumeKnob_);

        focusPanel_.setCompactLayout(true);
        addAndMakeVisible(focusPanel_);

        startTimerHz(2);
        timerCallback();
    }

    CompactModeEditor::~CompactModeEditor()
    {
        stopTimer();
    }

    void CompactModeEditor::timerCallback()
    {
        const auto name = juce::String(processor_.getCurrentPatch().metadata.name);
        if (patchNameLabel_.getText() != name)
            patchNameLabel_.setText(name.isEmpty() ? "INIT" : name, juce::dontSendNotification);
    }

    void CompactModeEditor::stepPreset(int direction)
    {
        if (presetIndex_ == nullptr)
            return;

        const auto current = processor_.getCurrentPresetPath();
        const juce::StringArray* favoritesOnly =
            browseFilter_.favoritesOnly && favoritesStore_ != nullptr ? &favoritesStore_->paths() : nullptr;
        std::optional<content::PresetEntry> entry;
        if (direction > 0)
            entry = presetIndex_->nextAfter(current, browseFilter_, favoritesOnly);
        else
            entry = presetIndex_->prevBefore(current, browseFilter_, favoritesOnly);

        if (!entry.has_value())
            return;

        processor_.loadPatchFromFile(entry->absolutePath);
        timerCallback();
    }

    void CompactModeEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kCompactOuterMargin);

        auto headerRow = bounds.removeFromTop(layout::kCompactHeaderHeight);
        prevButton_.setBounds(headerRow.removeFromLeft(28).reduced(2, 4));
        nextButton_.setBounds(headerRow.removeFromRight(28).reduced(2, 4));
        patchNameLabel_.setBounds(headerRow.reduced(4, 0));

        bounds.removeFromTop(layout::kCompactBlockGap);
        circularScope_.setBounds(bounds.removeFromTop(layout::kCompactScopeSize).withSizeKeepingCentre(
            layout::kCompactScopeSize, layout::kCompactScopeSize));
        scopeModeToggle_.setBounds(circularScope_.getBounds().removeFromTop(18).removeFromRight(56).reduced(6, 4));

        bounds.removeFromTop(layout::kCompactBlockGap);
        volumeKnob_->setBounds(bounds.removeFromTop(layout::kCompactVolumeHeight).withSizeKeepingCentre(
            layout::kCompactVolumeKnobWidth, layout::kCompactVolumeHeight));

        bounds.removeFromTop(layout::kCompactBlockGap);
        focusPanel_.setBounds(bounds);
    }

} // namespace pw8::plugin::ui
