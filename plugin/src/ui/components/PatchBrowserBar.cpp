#include "PatchBrowserBar.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        // Was a bare `280` repeated independently in both paint() and resized() --
        // harmless while they happened to agree, but a real bug waiting to happen
        // the first time only one of the two got edited (e.g. a wider wordmark
        // needing more room, changed in paint() and silently not in resized(), or
        // vice versa).
        constexpr int kWordmarkWidth = 280;
    } // namespace

    PatchBrowserBar::PatchBrowserBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        patchNameLabel_.setJustificationType(juce::Justification::centredRight);
        patchNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        patchNameLabel_.setFont(fonts::title(16.0f));
        addAndMakeVisible(patchNameLabel_);

        loadButton_.onClick = [this] { loadPatchFromFile(); };
        loadButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        loadButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        loadButton_.setColour(juce::ComboBox::outlineColourId, palette::kBorderBright);
        addAndMakeVisible(loadButton_);

        startTimerHz(2);
        timerCallback();
    }

    PatchBrowserBar::~PatchBrowserBar()
    {
        stopTimer();
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
        g.setFont(fonts::title(18.0f));
        g.drawText("PATCHWORK EIGHT", wordmarkArea.removeFromTop(22), juce::Justification::bottomLeft);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        g.drawText("8-ENGINE ALGORITHMIC SYNTHESIZER", wordmarkArea, juce::Justification::topLeft);
    }

    void PatchBrowserBar::resized()
    {
        auto bounds = getLocalBounds().withTrimmedLeft(kWordmarkWidth);
        loadButton_.setBounds(bounds.removeFromRight(80).reduced(0, 6));
        bounds.removeFromRight(8);
        patchNameLabel_.setBounds(bounds);
    }

    void PatchBrowserBar::loadPatchFromFile()
    {
        fileChooser_ = std::make_unique<juce::FileChooser>("Load a Patchwork Eight patch...", juce::File(),
                                                             "*.pw8");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            const auto file = chooser.getResult();
            if (!file.existsAsFile())
                return; // Cancelled -- not an error.

            const auto json = file.loadFileAsString();
            processor_.setStateInformation(json.toRawUTF8(), static_cast<int>(json.getNumBytesAsUTF8()));
            timerCallback(); // Refresh the displayed name immediately rather than waiting for the next timer tick.
        });
    }

} // namespace pw8::plugin::ui
