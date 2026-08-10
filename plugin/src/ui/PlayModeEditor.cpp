#include "PlayModeEditor.h"

#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PlayModeEditor::PlayModeEditor(PatchworkEightProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          processor_(processor),
          patchBrowserBar_(processor),
          graphView_(processor),
          filterLfoPanel_(processor.apvts),
          macroStrip_(processor.apvts),
          fxChainStrip_(processor.apvts)
    {
        setLookAndFeel(&lookAndFeel_);

        addAndMakeVisible(patchBrowserBar_);
        addAndMakeVisible(graphPanel_);
        graphPanel_.addAndMakeVisible(graphView_);
        addAndMakeVisible(filterLfoPanel_);
        addAndMakeVisible(macroStrip_);
        addAndMakeVisible(fxChainStrip_);

        setResizable(false, false);
        setSize(980, 820);
    }

    PlayModeEditor::~PlayModeEditor()
    {
        setLookAndFeel(nullptr);
    }

    void PlayModeEditor::paint(juce::Graphics& g)
    {
        juce::ColourGradient bg(palette::kBackgroundTop, 0.0f, 0.0f, palette::kBackgroundBottom, 0.0f,
                                 static_cast<float>(getHeight()), false);
        g.setGradientFill(bg);
        g.fillAll();
    }

    void PlayModeEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(12);

        patchBrowserBar_.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(8);

        // Bottom-up: the three utility strips get fixed heights generous enough
        // for a knob + label + value box to never be cramped (the exact bug a
        // too-small allotment caused here during development -- a starved rotary
        // slider's derived radii went negative, which juce::Graphics's own debug
        // assertions caught as a malformed fillEllipse call). The algorithm graph,
        // as the centerpiece, gets whatever's left rather than a fixed share.
        auto fxArea = bounds.removeFromBottom(120);
        bounds.removeFromBottom(8);
        auto macroArea = bounds.removeFromBottom(120);
        bounds.removeFromBottom(8);
        auto filterLfoArea = bounds.removeFromBottom(130);
        bounds.removeFromBottom(8);

        graphPanel_.setBounds(bounds);
        graphView_.setBounds(graphPanel_.getContentBounds());

        filterLfoPanel_.setBounds(filterLfoArea);
        macroStrip_.setBounds(macroArea);
        fxChainStrip_.setBounds(fxArea);
    }

} // namespace pw8::plugin::ui
