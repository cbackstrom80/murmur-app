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
        const auto w = static_cast<float>(getWidth());
        const auto h = static_cast<float>(getHeight());

        juce::ColourGradient bg(palette::kBackgroundTop, 0.0f, 0.0f, palette::kBackgroundBottom, 0.0f, h, false);
        g.setGradientFill(bg);
        g.fillAll();

        // A very sparse, deterministic dot grain -- the "milled panel" material
        // read extended to the whole background, not just card edges. Cheap
        // (no image asset, no per-frame recompute -- paint() only runs on
        // repaint, and this background never changes shape) and subtle enough
        // that it reads as texture, not as visible dots at normal viewing scale.
        g.setColour(palette::kTopHighlight.withAlpha(0.05f));
        constexpr float kSpacing = 26.0f;
        for (float gy = 10.0f; gy < h; gy += kSpacing)
        {
            for (float gx = 10.0f; gx < w; gx += kSpacing)
            {
                // A cheap positional hash, not real randomness -- deterministic so
                // the texture never swims between repaints, which matters since
                // several child components repaint on their own timers.
                const auto hash = static_cast<juce::uint32>(gx * 7919.0f + gy * 104729.0f);
                if ((hash & 3u) != 0u)
                    continue;
                g.fillEllipse(gx, gy, 1.1f, 1.1f);
            }
        }

        // Soft vignette: a radial gradient darkening the corners, the last touch
        // that keeps the eye pulled toward the centre (the algorithm graph)
        // rather than the flat edges of the window.
        juce::ColourGradient vignette(juce::Colours::transparentBlack, w * 0.5f, h * 0.42f,
                                       palette::kBackgroundTop.withAlpha(0.55f), w, h, true);
        vignette.addColour(0.75, juce::Colours::transparentBlack);
        g.setGradientFill(vignette);
        g.fillAll();
    }

    void PlayModeEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(12);

        patchBrowserBar_.setBounds(bounds.removeFromTop(40));
        bounds.removeFromTop(10);

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
