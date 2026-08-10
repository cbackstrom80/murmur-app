#include "PatchBrowserBar.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PatchBrowserBar::PatchBrowserBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        patchNameLabel_.setJustificationType(juce::Justification::centredRight);
        patchNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        patchNameLabel_.setFont(fonts::title(16.0f));
        addAndMakeVisible(patchNameLabel_);
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
        auto wordmarkArea = bounds.removeFromLeft(280);

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(18.0f));
        g.drawText("PATCHWORK EIGHT", wordmarkArea.removeFromTop(22), juce::Justification::bottomLeft);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        g.drawText("8-ENGINE ALGORITHMIC SYNTHESIZER", wordmarkArea, juce::Justification::topLeft);
    }

    void PatchBrowserBar::resized()
    {
        patchNameLabel_.setBounds(getLocalBounds().withTrimmedLeft(280));
    }

} // namespace pw8::plugin::ui
