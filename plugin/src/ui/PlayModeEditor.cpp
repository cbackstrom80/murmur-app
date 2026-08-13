#include "PlayModeEditor.h"

#include "PlayModeLayout.h"
#include "components/ModSourceChip.h"
#include "theme/BrandingAssets.h"
#include "theme/ObsidianFonts.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PlayModeEditor::PlayModeEditor(PatchworkEightProcessor& processor, SharedEditorChrome& chrome)
        : chrome_(chrome),
          arpLauncherChip_(processor),
          nodeSelectorRow_(processor),
          contextStrip_(processor),
          patchFocusPanel_(processor),
          compactEditor_(processor),
          operatorEditorPanel_(processor, modAssignmentController_),
          filterLfoPanel_(processor, modAssignmentController_),
          ampEnvelopePanel_(processor),
          modLauncherPanel_(processor, modAssignmentController_),
          fxChainStrip_(processor),
          modRoutingOverlay_(processor, modAssignmentController_),
          arpPanelOverlay_(processor)
    {
        setLookAndFeel(&lookAndFeel_);

        modAssignmentBanner_.setJustificationType(juce::Justification::centred);
        modAssignmentBanner_.setFont(fonts::label(fonts::kBodyLabelSize));
        modAssignmentBanner_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        modAssignmentBanner_.setColour(juce::Label::backgroundColourId, branding::glowColour().withAlpha(0.22f));
        modAssignmentBanner_.setVisible(false);
        addChildComponent(modAssignmentBanner_);

        modAssignmentController_.onChanged = [this] {
            updateModAssignmentBanner();
            modRoutingOverlay_.repaintModAssignmentState();
            modLauncherPanel_.repaintModAssignmentState();
            filterLfoPanel_.repaintModAssignmentState();
        };

        modLauncherPanel_.onOpenAdvanced = [this] { openModRoutingOverlay(); };

        addChildComponent(modRoutingOverlay_);
        modRoutingOverlay_.onClosed = [this] { closeModRoutingOverlay(); };

        addChildComponent(arpPanelOverlay_);
        arpPanelOverlay_.onClosed = [this] { closeArpPanel(); };
        arpLauncherChip_.onOpenDrawer = [this] { openArpPanel(); };

        addAndMakeVisible(arpLauncherChip_);

        compactEditor_.setPresetIndex(&chrome_.patchBrowserBar.getPresetIndex());
        compactEditor_.setFavoritesStore(&chrome_.favoritesStore);
        addChildComponent(compactEditor_);

        for (auto* btn : {&basicViewButton_, &advancedViewButton_, &compactViewButton_})
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(9000);
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::buttonOnColourId, branding::glowColour().withAlpha(0.28f));
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn->setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
            addAndMakeVisible(*btn);
        }
        basicViewButton_.onClick = [this] { setViewMode(ViewMode::Basic); };
        advancedViewButton_.onClick = [this] { setViewMode(ViewMode::Advanced); };
        compactViewButton_.onClick = [this] { setViewMode(ViewMode::Compact); };

        addChildComponent(nodeSelectorRow_);
        nodeSelectorRow_.onNodeSelected = [this](int node) {
            operatorEditorPanel_.showNode(node);
            updateScopeUi();
        };
        nodeSelectorRow_.onGlobalSelected = [this] {
            if (currentPage_ == Page::Osc)
                showPage(Page::Filter);
            updateScopeUi();
        };

        addChildComponent(contextStrip_);

        addAndMakeVisible(patchFocusPanel_);

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
            addChildComponent(btn);
        }

        addChildComponent(oscPage_);
        addChildComponent(filterPage_);
        addChildComponent(envPage_);
        addChildComponent(modPage_);
        addChildComponent(fxPage_);

        oscPage_.addAndMakeVisible(oscPanel_);
        oscPanel_.addAndMakeVisible(operatorEditorPanel_);
        filterPage_.addAndMakeVisible(filterLfoPanel_);
        envPage_.addAndMakeVisible(ampEnvelopePanel_);
        modPage_.addAndMakeVisible(modLauncherPanel_);
        fxPage_.addAndMakeVisible(fxChainStrip_);

        setViewMode(ViewMode::Basic);
    }

    PlayModeEditor::~PlayModeEditor() { setLookAndFeel(nullptr); }

    void PlayModeEditor::refreshFromPatch()
    {
        nodeSelectorRow_.repaint();
        patchFocusPanel_.refreshFromPatch();
        compactEditor_.refreshFromPatch();
    }

    void PlayModeEditor::setBrowseFilter(const content::PresetMetadataFilter& filter)
    {
        compactEditor_.setBrowseFilter(filter);
    }

    bool PlayModeEditor::isCompactView() const noexcept { return viewMode_ == ViewMode::Compact; }

    bool PlayModeEditor::keyPressed(const juce::KeyPress& key)
    {
        if (arpPanelOverlay_.isVisible())
            return arpPanelOverlay_.keyPressed(key);

        if (modRoutingOverlay_.isVisible())
            return modRoutingOverlay_.keyPressed(key);

        if (key == juce::KeyPress('a', juce::ModifierKeys::noModifiers, 0))
        {
            openArpPanel();
            return true;
        }

        if (viewMode_ == ViewMode::Advanced && key == juce::KeyPress('m', juce::ModifierKeys::noModifiers, 0))
        {
            openModRoutingOverlay();
            return true;
        }

        return false;
    }

    void PlayModeEditor::setViewMode(ViewMode mode)
    {
        if (viewMode_ == ViewMode::Compact && mode != ViewMode::Compact)
            closeArpPanel();

        viewMode_ = mode;
        basicViewButton_.setToggleState(mode == ViewMode::Basic, juce::dontSendNotification);
        advancedViewButton_.setToggleState(mode == ViewMode::Advanced, juce::dontSendNotification);
        compactViewButton_.setToggleState(mode == ViewMode::Compact, juce::dontSendNotification);

        const bool compact = mode == ViewMode::Compact;
        const bool advanced = mode == ViewMode::Advanced;

        arpLauncherChip_.setVisible(!compact);

        basicViewButton_.setVisible(true);
        advancedViewButton_.setVisible(true);
        compactViewButton_.setVisible(true);

        compactEditor_.setVisible(compact);
        if (compact)
        {
            compactEditor_.setBrowseFilter(chrome_.patchBrowserBar.browseFilter());
            compactEditor_.setBounds(getLocalBounds());
        }

        nodeSelectorRow_.setVisible(advanced);
        contextStrip_.setVisible(advanced);

        for (auto& btn : tabButtons_)
            btn.setVisible(advanced);

        patchFocusPanel_.setBasicPerformanceLayout(mode == ViewMode::Basic);
        patchFocusPanel_.setVisible(mode == ViewMode::Basic);

        if (compact || mode == ViewMode::Basic)
        {
            closeModRoutingOverlay();
            modAssignmentController_.disarm();
            oscPage_.setVisible(false);
            filterPage_.setVisible(false);
            envPage_.setVisible(false);
            modPage_.setVisible(false);
            fxPage_.setVisible(false);
            modAssignmentBanner_.setVisible(false);
        }
        else if (advanced)
        {
            if (currentPage_ == Page::Filter && !filterPage_.isVisible())
                showPage(Page::Filter);
            else
                showPage(currentPage_);
            updateScopeUi();
        }

        if (onLayoutOrViewModeChanged)
            onLayoutOrViewModeChanged();
        else
            resized();
    }

    void PlayModeEditor::openModRoutingOverlay()
    {
        if (viewMode_ != ViewMode::Advanced)
            return;

        addAndMakeVisible(modRoutingOverlay_);
        modRoutingOverlay_.setBounds(getLocalBounds());
        modRoutingOverlay_.showOverlay();
    }

    void PlayModeEditor::closeModRoutingOverlay()
    {
        removeChildComponent(&modRoutingOverlay_);
    }

    void PlayModeEditor::openArpPanel()
    {
        addAndMakeVisible(arpPanelOverlay_);
        arpPanelOverlay_.setBounds(getLocalBounds());
        arpPanelOverlay_.showDrawer();
    }

    void PlayModeEditor::closeArpPanel()
    {
        removeChildComponent(&arpPanelOverlay_);
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
        if (viewMode_ != ViewMode::Advanced)
            return;

        const bool global = nodeSelectorRow_.isGlobalScope();
        contextStrip_.setScope(global ? FilterPanelScope::Global : FilterPanelScope::Engine,
                               nodeSelectorRow_.getSelectedNode());

        tabButtons_[static_cast<std::size_t>(Page::Osc)].setVisible(!global);

        if (global && currentPage_ == Page::Osc)
            showPage(Page::Filter);

        refreshFilterPanelScope();
        const auto routingScope = global ? FilterPanelScope::Global : FilterPanelScope::Engine;
        modRoutingOverlay_.setRoutingContext(routingScope, nodeSelectorRow_.getSelectedNode());
        modLauncherPanel_.setRoutingContext(routingScope, nodeSelectorRow_.getSelectedNode());
        contextStrip_.repaint();
        resized();
    }

    void PlayModeEditor::updateModAssignmentBanner()
    {
        if (viewMode_ != ViewMode::Advanced)
        {
            modAssignmentBanner_.setVisible(false);
            resized();
            return;
        }

        const auto source = modAssignmentController_.armedSource();
        if (!source.has_value())
        {
            modAssignmentBanner_.setVisible(false);
            resized();
            return;
        }

        modAssignmentBanner_.setText("Step 2: click a ringed knob — OSC (Level/WT Pos/Pan), FILTER (Cutoff/Reso), "
                                     "or destination buttons in MOD.",
                                     juce::dontSendNotification);
        modAssignmentBanner_.setVisible(true);
        resized();
    }

    void PlayModeEditor::showPage(Page page)
    {
        if (viewMode_ != ViewMode::Advanced)
            return;

        if (nodeSelectorRow_.isGlobalScope() && page == Page::Osc)
            page = Page::Filter;

        currentPage_ = page;
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
        if (viewMode_ == ViewMode::Compact)
        {
            auto bounds = getLocalBounds().reduced(layout::kCompactOuterMargin);
            auto modeRow = bounds.removeFromTop(layout::kViewModeRowHeight);
            basicViewButton_.setBounds(modeRow.removeFromLeft(72).reduced(2));
            advancedViewButton_.setBounds(modeRow.removeFromLeft(88).reduced(2));
            compactViewButton_.setBounds(modeRow.removeFromLeft(80).reduced(2));
            compactEditor_.setBounds(bounds);
            return;
        }

        auto bounds = getLocalBounds();

        auto modeRow = bounds.removeFromTop(layout::kViewModeRowHeight);
        basicViewButton_.setBounds(modeRow.removeFromLeft(72).reduced(2));
        advancedViewButton_.setBounds(modeRow.removeFromLeft(88).reduced(2));
        compactViewButton_.setBounds(modeRow.removeFromLeft(80).reduced(2));
        arpLauncherChip_.setBounds(modeRow.removeFromRight(168).reduced(2, 1));
        bounds.removeFromTop(layout::kBlockGap);

        const bool advanced = viewMode_ == ViewMode::Advanced;

        if (advanced)
        {
            nodeSelectorRow_.setBounds(bounds.removeFromTop(layout::kEngineRowHeight));
            bounds.removeFromTop(layout::kSectionGap);
            contextStrip_.setBounds(bounds.removeFromTop(layout::kContextRowHeight));
            bounds.removeFromTop(layout::kBlockGap);
        }

        if (advanced)
        {
            auto tabRow = bounds.removeFromTop(layout::kTabRowHeight);
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
            bounds.removeFromTop(layout::kBlockGap);
        }

        if (advanced && modAssignmentBanner_.isVisible())
        {
            modAssignmentBanner_.setBounds(bounds.removeFromTop(layout::kModBannerHeight));
            bounds.removeFromTop(layout::kSectionGap);
        }

        if (advanced)
        {
            oscPage_.setBounds(bounds);
            filterPage_.setBounds(bounds);
            envPage_.setBounds(bounds);
            modPage_.setBounds(bounds);
            fxPage_.setBounds(bounds);
        }
        else
        {
            patchFocusPanel_.setBounds(bounds);
            oscPage_.setBounds(0, 0, 0, 0);
            filterPage_.setBounds(0, 0, 0, 0);
            envPage_.setBounds(0, 0, 0, 0);
            modPage_.setBounds(0, 0, 0, 0);
            fxPage_.setBounds(0, 0, 0, 0);
        }

        oscPanel_.setBounds(oscPage_.getLocalBounds());
        operatorEditorPanel_.setBounds(oscPanel_.getContentBounds());
        filterLfoPanel_.setBounds(filterPage_.getLocalBounds());
        ampEnvelopePanel_.setBounds(envPage_.getLocalBounds());
        modLauncherPanel_.setBounds(modPage_.getLocalBounds());
        fxChainStrip_.setBounds(fxPage_.getLocalBounds());

        modRoutingOverlay_.setBounds(getLocalBounds());
        arpPanelOverlay_.setBounds(getLocalBounds());
    }

} // namespace pw8::plugin::ui
