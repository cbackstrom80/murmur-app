#include "PlayModeEditor.h"

#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PlayModeEditor::PlayModeEditor(PatchworkEightProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          processor_(processor),
          patchBrowserBar_(processor),
          nodeSelectorRow_(processor),
          macroStrip_(processor),
          operatorEditorPanel_(processor),
          modSourceStrip_(processor),
          filterLfoPanel_(processor),
          fxChainStrip_(processor.apvts)
    {
        setLookAndFeel(&lookAndFeel_);

        addAndMakeVisible(patchBrowserBar_);
        addAndMakeVisible(nodeSelectorRow_);
        nodeSelectorRow_.onNodeSelected = [this](int node) { operatorEditorPanel_.showNode(node); };

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
        {
            auto& btn = tabButtons_[i];
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(9001);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::buttonOnColourId, palette::kAccent.withAlpha(0.35f));
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn.setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
            btn.onClick = [this, page = static_cast<Page>(i)] { showPage(page); };
            addAndMakeVisible(btn);
        }

        addAndMakeVisible(basicPage_);
        addAndMakeVisible(oscPage_);
        addAndMakeVisible(filterPage_);
        addAndMakeVisible(modPage_);
        addAndMakeVisible(fxPage_);

        basicPage_.addAndMakeVisible(macroPanel_);
        macroPanel_.addAndMakeVisible(macroStrip_);
        oscPage_.addAndMakeVisible(oscPanel_);
        oscPanel_.addAndMakeVisible(operatorEditorPanel_);
        filterPage_.addAndMakeVisible(filterLfoPanel_);
        modPage_.addAndMakeVisible(modSourceStrip_);
        fxPage_.addAndMakeVisible(fxChainStrip_);

        showPage(Page::Basic);
        setResizable(false, false);
        // Shorter than the old single-screen stack (docs/UI_PAGED_LAYOUT.md): tabs
        // replace ~960px of always-visible utility strips with one page at a time.
        setSize(980, 920);
    }

    PlayModeEditor::~PlayModeEditor()
    {
        setLookAndFeel(nullptr);
    }

    void PlayModeEditor::showPage(Page page)
    {
        currentPage_ = page;
        basicPage_.setVisible(page == Page::Basic);
        oscPage_.setVisible(page == Page::Osc);
        filterPage_.setVisible(page == Page::Filter);
        modPage_.setVisible(page == Page::Mod);
        fxPage_.setVisible(page == Page::Fx);

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
            tabButtons_[i].setToggleState(static_cast<Page>(i) == page, juce::dontSendNotification);

        resized();
    }

    void PlayModeEditor::paint(juce::Graphics& g)
    {
        const auto w = static_cast<float>(getWidth());
        const auto h = static_cast<float>(getHeight());

        juce::ColourGradient bg(palette::kBackgroundTop, 0.0f, 0.0f, palette::kBackgroundBottom, 0.0f, h, false);
        g.setGradientFill(bg);
        g.fillAll();

        g.setColour(palette::kTopHighlight.withAlpha(0.05f));
        constexpr float kSpacing = 26.0f;
        for (float gy = 10.0f; gy < h; gy += kSpacing)
        {
            for (float gx = 10.0f; gx < w; gx += kSpacing)
            {
                const auto hash = static_cast<juce::uint32>(gx * 7919.0f + gy * 104729.0f);
                if ((hash & 3u) != 0u)
                    continue;
                g.fillEllipse(gx, gy, 1.1f, 1.1f);
            }
        }

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
        bounds.removeFromTop(8);
        nodeSelectorRow_.setBounds(bounds.removeFromTop(36));
        bounds.removeFromTop(8);

        auto tabRow = bounds.removeFromTop(32);
        const int tabWidth = tabRow.getWidth() / static_cast<int>(tabButtons_.size());
        for (auto& btn : tabButtons_)
            btn.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2, 2));
        bounds.removeFromTop(8);

        basicPage_.setBounds(bounds);
        oscPage_.setBounds(bounds);
        filterPage_.setBounds(bounds);
        modPage_.setBounds(bounds);
        fxPage_.setBounds(bounds);

        macroPanel_.setBounds(basicPage_.getLocalBounds());
        macroStrip_.setBounds(macroPanel_.getContentBounds());

        oscPanel_.setBounds(oscPage_.getLocalBounds());
        operatorEditorPanel_.setBounds(oscPanel_.getContentBounds());

        filterLfoPanel_.setBounds(filterPage_.getLocalBounds());
        modSourceStrip_.setBounds(modPage_.getLocalBounds());
        fxChainStrip_.setBounds(fxPage_.getLocalBounds());
    }

} // namespace pw8::plugin::ui
