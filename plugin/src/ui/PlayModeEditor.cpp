#include "PlayModeEditor.h"

#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PlayModeEditor::PlayModeEditor(PatchworkEightProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          processor_(processor),
          patchBrowserBar_(processor),
          graphView_(processor),
          operatorEditorPanel_(processor),
          modSourceStrip_(processor),
          filterLfoPanel_(processor),
          macroStrip_(processor.apvts),
          fxChainStrip_(processor.apvts)
    {
        setLookAndFeel(&lookAndFeel_);

        addAndMakeVisible(patchBrowserBar_);
        addAndMakeVisible(graphPanel_);
        graphPanel_.addAndMakeVisible(graphView_);
        graphPanel_.addAndMakeVisible(operatorEditorPanel_);
        graphView_.onNodeSelected = [this](int node) { operatorEditorPanel_.showNode(node); };
        addAndMakeVisible(modSourceStrip_);
        addAndMakeVisible(filterLfoPanel_);
        addAndMakeVisible(macroStrip_);
        addAndMakeVisible(fxChainStrip_);

        setResizable(false, false);
        // +160 over the original 890 for OperatorEditorPanel's fixed allotment
        // inside the graph card, +80 more for ModSourceStrip's connections list
        // (UI GATE 3), +48 more (UI GATE 4) so FxChainStrip's 7 slots -- each
        // paying a 24px type-label tax on top of its knob, unlike every other
        // strip's knobs -- stop flooring out at ObsidianLookAndFeel's 16px
        // defensive-minimum diameter. Every one of these, growing the window
        // rather than squeezing a control back down toward illegibility, the
        // exact "negative/undersized geometry" failure mode UI GATE 1 already
        // found once via lldb (docs/UI.md).
        setSize(980, 1178);
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
        // FxChainStrip pays a 24px type-label tax per slot on top of its knob
        // (unlike MacroStrip's plain knob row), so it needs more height than the
        // other utility strips to land its knob at a comparable, legible size --
        // see the UI GATE 4 note above `setSize()`.
        auto fxArea = bounds.removeFromBottom(168);
        bounds.removeFromBottom(8);
        auto macroArea = bounds.removeFromBottom(120);
        bounds.removeFromBottom(8);
        auto filterLfoArea = bounds.removeFromBottom(130);
        bounds.removeFromBottom(8);
        auto modSourceArea = bounds.removeFromBottom(140); // Chip row + connections list (UI GATE 3).
        bounds.removeFromBottom(8);

        graphPanel_.setBounds(bounds);
        auto graphContent = graphPanel_.getContentBounds();
        auto opEditorArea = graphContent.removeFromBottom(140);
        graphContent.removeFromBottom(8);
        graphView_.setBounds(graphContent);
        operatorEditorPanel_.setBounds(opEditorArea);

        modSourceStrip_.setBounds(modSourceArea);
        filterLfoPanel_.setBounds(filterLfoArea);
        macroStrip_.setBounds(macroArea);
        fxChainStrip_.setBounds(fxArea);
    }

} // namespace pw8::plugin::ui
