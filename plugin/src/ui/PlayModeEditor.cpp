#include "PlayModeEditor.h"

#include "theme/BrandingAssets.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PlayModeEditor::PlayModeEditor(PatchworkEightProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          processor_(processor),
          patchBrowserBar_(processor),
          nodeSelectorRow_(processor),
          contextStrip_(processor),
          macroStrip_(processor),
          operatorEditorPanel_(processor),
          filterLfoPanel_(processor),
          engineSummingStrip_(processor),
          ampEnvelopePanel_(processor),
          modSourceStrip_(processor),
          fxChainStrip_(processor),
          presetBrowserOverlay_(processor, patchBrowserBar_.getPresetIndex(), favoritesStore_)
    {
        setLookAndFeel(&lookAndFeel_);

        addAndMakeVisible(patchBrowserBar_);
        patchBrowserBar_.setFavoritesStore(&favoritesStore_);
        patchBrowserBar_.onBrowseClicked = [this] {
            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
        };
        presetBrowserOverlay_.onClosed = [this] {
            patchBrowserBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
            removeChildComponent(&presetBrowserOverlay_);
        };

        addAndMakeVisible(nodeSelectorRow_);
        nodeSelectorRow_.onNodeSelected = [this](int node) {
            operatorEditorPanel_.showNode(node);
            engineSummingStrip_.setHighlightedEngine(node);
            updateScopeUi();
        };
        nodeSelectorRow_.onGlobalSelected = [this] {
            if (currentPage_ == Page::Osc)
                showPage(Page::Filter);
            updateScopeUi();
        };

        addAndMakeVisible(contextStrip_);

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
        {
            auto& btn = tabButtons_[i];
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(9001);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::buttonOnColourId, branding::glowColour().withAlpha(0.28f));
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn.setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
            btn.onClick = [this, page = static_cast<Page>(i)] { showPage(page); };
            addAndMakeVisible(btn);
        }

        addAndMakeVisible(basicPage_);
        addAndMakeVisible(oscPage_);
        addAndMakeVisible(filterPage_);
        addAndMakeVisible(envPage_);
        addAndMakeVisible(modPage_);
        addAndMakeVisible(fxPage_);

        basicPage_.addAndMakeVisible(macroPanel_);
        macroPanel_.addAndMakeVisible(macroStrip_);
        oscPage_.addAndMakeVisible(oscPanel_);
        oscPanel_.addAndMakeVisible(operatorEditorPanel_);
        filterPage_.addAndMakeVisible(engineSummingStrip_);
        filterPage_.addAndMakeVisible(filterLfoPanel_);
        envPage_.addAndMakeVisible(ampEnvelopePanel_);
        modPage_.addAndMakeVisible(modSourceStrip_);
        fxPage_.addAndMakeVisible(fxChainStrip_);

        showPage(Page::Osc);
        updateScopeUi();
        setResizable(false, false);
        setSize(980, 920 + branding::headerBarHeight() - 40);
    }

    PlayModeEditor::~PlayModeEditor()
    {
        setLookAndFeel(nullptr);
    }

    void PlayModeEditor::refreshFilterPanelScope()
    {
        if (nodeSelectorRow_.isGlobalScope())
            filterLfoPanel_.setScope(FilterPanelScope::Global, 0);
        else
            filterLfoPanel_.setScope(FilterPanelScope::Engine, nodeSelectorRow_.getSelectedNode());
    }

    void PlayModeEditor::updateScopeUi()
    {
        const bool global = nodeSelectorRow_.isGlobalScope();
        contextStrip_.setScope(global ? FilterPanelScope::Global : FilterPanelScope::Engine,
                               nodeSelectorRow_.getSelectedNode());

        tabButtons_[static_cast<std::size_t>(Page::Basic)].setButtonText(global ? "PERF" : "MACROS");
        tabButtons_[static_cast<std::size_t>(Page::Basic)].setVisible(global);
        tabButtons_[static_cast<std::size_t>(Page::Osc)].setVisible(!global);

        if (global && currentPage_ == Page::Osc)
            showPage(Page::Filter);
        if (!global && currentPage_ == Page::Basic)
            showPage(Page::Osc);

        refreshFilterPanelScope();
        engineSummingStrip_.setVisible(global);
        contextStrip_.repaint();
        resized();
    }

    void PlayModeEditor::showPage(Page page)
    {
        if (nodeSelectorRow_.isGlobalScope() && page == Page::Osc)
            page = Page::Filter;
        if (!nodeSelectorRow_.isGlobalScope() && page == Page::Basic)
            page = Page::Osc;

        currentPage_ = page;
        basicPage_.setVisible(page == Page::Basic);
        oscPage_.setVisible(page == Page::Osc);
        filterPage_.setVisible(page == Page::Filter);
        envPage_.setVisible(page == Page::Env);
        modPage_.setVisible(page == Page::Mod);
        fxPage_.setVisible(page == Page::Fx);

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
            tabButtons_[i].setToggleState(static_cast<Page>(i) == page, juce::dontSendNotification);

        if (page == Page::Filter)
            refreshFilterPanelScope();

        resized();
    }

    void PlayModeEditor::paint(juce::Graphics& g)
    {
        const auto w = static_cast<float>(getWidth());
        const auto h = static_cast<float>(getHeight());

        juce::ColourGradient bg(palette::kBackgroundTop, 0.0f, 0.0f, palette::kBackgroundBottom, 0.0f, h, false);
        g.setGradientFill(bg);
        g.fillAll();

        juce::ColourGradient brandGlow(branding::glowColour().withAlpha(0.14f), 0.0f, 0.0f,
                                       juce::Colours::transparentBlack, w * 0.42f, h * 0.18f, true);
        g.setGradientFill(brandGlow);
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

        patchBrowserBar_.setBounds(bounds.removeFromTop(branding::headerBarHeight()));
        bounds.removeFromTop(8);
        nodeSelectorRow_.setBounds(bounds.removeFromTop(36));
        bounds.removeFromTop(6);
        contextStrip_.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(8);

        auto tabRow = bounds.removeFromTop(32);
        int visibleTabs = 0;
        for (const auto& btn : tabButtons_)
            if (btn.isVisible())
                ++visibleTabs;
        const int tabWidth = visibleTabs > 0 ? tabRow.getWidth() / visibleTabs : tabRow.getWidth();
        for (auto& btn : tabButtons_)
        {
            if (!btn.isVisible())
                continue;
            btn.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2, 2));
        }
        bounds.removeFromTop(8);

        basicPage_.setBounds(bounds);
        oscPage_.setBounds(bounds);
        filterPage_.setBounds(bounds);
        envPage_.setBounds(bounds);
        modPage_.setBounds(bounds);
        fxPage_.setBounds(bounds);

        macroPanel_.setBounds(basicPage_.getLocalBounds());
        macroStrip_.setBounds(macroPanel_.getContentBounds());

        oscPanel_.setBounds(oscPage_.getLocalBounds());
        operatorEditorPanel_.setBounds(oscPanel_.getContentBounds());

        if (nodeSelectorRow_.isGlobalScope() && engineSummingStrip_.isVisible())
        {
            auto filterBounds = filterPage_.getLocalBounds();
            engineSummingStrip_.setBounds(filterBounds.removeFromTop(static_cast<int>(filterBounds.getHeight() * 0.42f)));
            filterLfoPanel_.setBounds(filterBounds);
        }
        else
        {
            engineSummingStrip_.setBounds({});
            filterLfoPanel_.setBounds(filterPage_.getLocalBounds());
        }

        ampEnvelopePanel_.setBounds(envPage_.getLocalBounds());
        modSourceStrip_.setBounds(modPage_.getLocalBounds());
        fxChainStrip_.setBounds(fxPage_.getLocalBounds());

        presetBrowserOverlay_.setBounds(getLocalBounds());
    }

} // namespace pw8::plugin::ui
