#include "PatchBrowserBar.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kWordmarkWidth = 220;
    } // namespace

    PatchBrowserBar::PatchBrowserBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        patchNameLabel_.setJustificationType(juce::Justification::centred);
        patchNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        patchNameLabel_.setFont(fonts::title(15.0f));
        addAndMakeVisible(patchNameLabel_);

        prevButton_.onClick = [this] { stepPreset(-1); };
        nextButton_.onClick = [this] { stepPreset(1); };
        for (auto* btn : {&prevButton_, &nextButton_})
        {
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            addAndMakeVisible(*btn);
        }

        loadButton_.onClick = [this] { loadPatchFromFile(); };
        loadButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        loadButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        loadButton_.setColour(juce::ComboBox::outlineColourId, palette::kBorderBright);
        addAndMakeVisible(loadButton_);

        refreshPresetIndex();
        startTimerHz(2);
        timerCallback();
    }

    PatchBrowserBar::~PatchBrowserBar()
    {
        stopTimer();
    }

    void PatchBrowserBar::refreshPresetIndex()
    {
        presetIndex_.rescan();
        prevButton_.setEnabled(!presetIndex_.allEntries().isEmpty());
        nextButton_.setEnabled(!presetIndex_.allEntries().isEmpty());
    }

    void PatchBrowserBar::timerCallback()
    {
        const auto name = juce::String(processor_.getCurrentPatch().metadata.name);
        if (patchNameLabel_.getText() != name)
            patchNameLabel_.setText(name.isEmpty() ? "INIT" : name, juce::dontSendNotification);
    }

    void PatchBrowserBar::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds();
        auto wordmarkArea = bounds.removeFromLeft(kWordmarkWidth);

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(16.0f));
        g.drawText("PATCHWORK EIGHT", wordmarkArea.removeFromTop(20), juce::Justification::bottomLeft);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("8-ENGINE ALGORITHMIC SYNTH", wordmarkArea, juce::Justification::topLeft);
    }

    void PatchBrowserBar::resized()
    {
        auto bounds = getLocalBounds().withTrimmedLeft(kWordmarkWidth);
        loadButton_.setBounds(bounds.removeFromRight(72).reduced(0, 6));
        bounds.removeFromRight(4);
        nextButton_.setBounds(bounds.removeFromRight(28).reduced(0, 8));
        prevButton_.setBounds(bounds.removeFromRight(28).reduced(0, 8));
        bounds.removeFromRight(6);
        patchNameLabel_.setBounds(bounds);
    }

    void PatchBrowserBar::stepPreset(int direction)
    {
        const auto current = processor_.getCurrentPresetPath();
        std::optional<content::PresetEntry> entry;
        if (direction > 0)
            entry = presetIndex_.nextAfter(current);
        else
            entry = presetIndex_.prevBefore(current);

        if (!entry.has_value())
            return;

        processor_.loadPatchFromFile(entry->absolutePath);
        timerCallback();
    }

    void PatchBrowserBar::loadPatchFromFile()
    {
        fileChooser_ = std::make_unique<juce::FileChooser>("Load a Patchwork Eight patch...", juce::File(),
                                                             "*.pw8");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            const auto file = chooser.getResult();
            if (!file.existsAsFile())
                return;

            processor_.loadPatchFromFile(file.getFullPathName());
            timerCallback();
        });
    }

} // namespace pw8::plugin::ui
