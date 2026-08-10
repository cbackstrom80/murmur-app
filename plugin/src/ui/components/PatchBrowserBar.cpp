#include "PatchBrowserBar.h"

#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PatchBrowserBar::PatchBrowserBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        patchNameLabel_.setJustificationType(juce::Justification::centredLeft);
        patchNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        patchNameLabel_.setFont(juce::Font(juce::FontOptions(15.0f)).withExtraKerningFactor(0.02f));
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
        g.setColour(palette::kTextDim);
        g.setFont(juce::Font(juce::FontOptions(11.0f)).withExtraKerningFactor(0.15f));
        auto bounds = getLocalBounds();
        g.drawText("PATCHWORK EIGHT", bounds.removeFromRight(140), juce::Justification::centredRight);
    }

    void PatchBrowserBar::resized()
    {
        patchNameLabel_.setBounds(getLocalBounds().withTrimmedRight(140));
    }

} // namespace pw8::plugin::ui
