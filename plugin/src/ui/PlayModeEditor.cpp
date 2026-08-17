#include "PlayModeEditor.h"

#include <cmath>

#include "PlayModeLayout.h"
#include "components/ModSourceChip.h"
#include "state/PluginState.h"
#include "theme/BrandingAssets.h"
#include "theme/ObsidianFonts.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] int readFxType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        [[nodiscard]] juce::String fxSlotParamPrefix(std::size_t slotIndex)
        {
            if (slotIndex < 3)
                return insertFxParamId(slotIndex, "");
            return masterFxParamId(slotIndex - 3, "");
        }
    } // namespace

    PlayModeEditor::PlayModeEditor(PatchworkEightProcessor& processor, SharedEditorChrome& chrome)
        : processor_(processor),
          chrome_(chrome),
          nodeSelectorRow_(processor),
          liveTopologyStrip_(processor),
          engineGridPanel_(processor),
          dashboardStrip_(processor, modAssignmentController_),
          vstBottomBar_(processor),
          engineDetailOverlay_(processor, modAssignmentController_),
          topologyGraphOverlay_(processor),
          contextStrip_(processor),
          desktopScope_(processor),
          masterOutputDeck_(processor),
          patchFocusPanel_(processor),
          compactEditor_(processor),
          operatorEditorPanel_(processor, modAssignmentController_),
          filterLfoPanel_(processor, modAssignmentController_),
          ampEnvelopePanel_(processor),
          modLauncherPanel_(processor, modAssignmentController_),
          fxChainStrip_(processor),
          globalPanel_(processor),
          modRoutingOverlay_(processor, modAssignmentController_),
          arpPanelOverlay_(processor),
          vocoderLabPanel_(processor),
          dualLfoLabPanel_(processor),
          wavetableLabPanel_(processor)
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

        addChildComponent(vocoderLabPanel_);
        vocoderLabPanel_.onClosed = [this] { closeVocoderLab(); };

        addChildComponent(dualLfoLabPanel_);
        dualLfoLabPanel_.onClosed = [this] { closeDualLfoLab(); };
        dualLfoLabPanel_.onOpenModMatrix = [this] {
            closeDualLfoLab();
            openModRoutingOverlay();
        };

        addChildComponent(wavetableLabPanel_);
        wavetableLabPanel_.onClosed = [this] { closeWavetableLab(); };

        dashboardStrip_.onVocoderLabRequested = [this](std::size_t slotIndex) { openVocoderLab(slotIndex); };
        dashboardStrip_.onLfoLabRequested = [this] { openDualLfoLab(); };

        vstBottomBar_.onVocoderLabRequested = [this] { openVocoderLab(preferredVocoderFxSlotIndex()); };
        vstBottomBar_.onLfoLabRequested = [this] { openDualLfoLab(); };
        vstBottomBar_.onModMatrixRequested = [this] { openModRoutingOverlay(); };

        addChildComponent(compactEditor_);
        compactEditor_.setPresetIndex(&chrome_.patchBrowserBar.getPresetIndex());
        compactEditor_.setFavoritesStore(&chrome_.favoritesStore);

        addChildComponent(topologyGraphOverlay_);
        topologyGraphOverlay_.onDismissed = [this] { closeGraphOverlay(); };
        topologyGraphOverlay_.onNodeSelected = [this](int node) { syncNodeSelection(node); };

        advancedGridPage_.addAndMakeVisible(engineGridPanel_);
        advancedGridPage_.addAndMakeVisible(dashboardStrip_);
        advancedGridPage_.addAndMakeVisible(vstBottomBar_);
        addChildComponent(advancedGridPage_);
        vstBottomBar_.setVisible(false);
        engineGridPanel_.onEngineDoubleClicked = [this](int engine) { openEngineDetail(engine); };

        addChildComponent(engineDetailOverlay_);
        engineDetailOverlay_.onClosed = [this] { closeEngineDetail(); };

        addAndMakeVisible(desktopScope_);
        desktopScope_.setVisible(false);

        addAndMakeVisible(masterOutputDeck_);
        masterOutputDeck_.setVisible(false);

        addAndMakeVisible(liveTopologyStrip_);
        liveTopologyStrip_.onExpandRequested = [this] { openGraphOverlay(); };
        liveTopologyStrip_.onNodeSelected = [this](int node) { syncNodeSelection(node); };

        addChildComponent(nodeSelectorRow_);
        nodeSelectorRow_.onNodeSelected = [this](int node) { syncNodeSelection(node); };
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
        addChildComponent(globalPage_);

        oscPage_.addAndMakeVisible(oscPanel_);
        oscPanel_.addAndMakeVisible(operatorEditorPanel_);
        filterPage_.addAndMakeVisible(filterLfoPanel_);
        envPage_.addAndMakeVisible(ampEnvelopePanel_);
        modPage_.addAndMakeVisible(modLauncherPanel_);
        fxPage_.addAndMakeVisible(fxChainStrip_);
        globalPage_.addAndMakeVisible(globalPanel_);
    }

    PlayModeEditor::~PlayModeEditor() { setLookAndFeel(nullptr); }

    void PlayModeEditor::setPlayViewMode(layout::PlayViewMode mode)
    {
        if (viewMode_ == mode || settingPlayViewMode_)
            return;

        const juce::ScopedValueSetter<bool> guard(settingPlayViewMode_, true);

        if (viewMode_ == layout::PlayViewMode::Compact && mode != layout::PlayViewMode::Compact)
            closeArpPanel();

        viewMode_ = mode;

        const bool compact = mode == layout::PlayViewMode::Compact;
        const bool advanced = mode == layout::PlayViewMode::Advanced;
        const bool basic = mode == layout::PlayViewMode::Basic;

        compactEditor_.setVisible(compact);
        if (compact)
        {
            compactEditor_.setBrowseFilter(chrome_.patchBrowserBar.browseFilter());
            compactEditor_.setBounds(getLocalBounds());
        }

        nodeSelectorRow_.setVisible(false);
        liveTopologyStrip_.setVisible(false);
        contextStrip_.setVisible(false);
        advancedGridPage_.setVisible(advanced);
        desktopScope_.setVisible(basic);
        desktopScope_.setDesktopPlayModeLayout(basic);
        desktopScope_.setIpadPlayLayout(basic);
        masterOutputDeck_.setVisible(basic);
        vstBottomBar_.setVisible(advanced || basic);
        vstBottomBar_.setPlayBoardMode(advanced);
        vstBottomBar_.setDesktopPlayMode(basic);
        patchFocusPanel_.setDesktopPlayModeLayout(basic);
        patchFocusPanel_.setIpadPlayLayout(basic);

        if (!advanced)
        {
            closeGraphOverlay();
            closeEngineDetail();
            closeVocoderLab();
            closeDualLfoLab();
            closeWavetableLab();
        }

        for (auto& btn : tabButtons_)
            btn.setVisible(false);

        patchFocusPanel_.setBasicPerformanceLayout(basic);
        patchFocusPanel_.setVisible(basic);

        if (compact || basic)
        {
            closeModRoutingOverlay();
            modAssignmentController_.disarm();
            oscPage_.setVisible(false);
            filterPage_.setVisible(false);
            envPage_.setVisible(false);
            modPage_.setVisible(false);
            fxPage_.setVisible(false);
            globalPage_.setVisible(false);
            modAssignmentBanner_.setVisible(false);
        }
        else if (advanced)
        {
            closeModRoutingOverlay();
            modAssignmentController_.disarm();
            oscPage_.setVisible(false);
            filterPage_.setVisible(false);
            envPage_.setVisible(false);
            modPage_.setVisible(false);
            fxPage_.setVisible(false);
            globalPage_.setVisible(false);
            modAssignmentBanner_.setVisible(false);
        }

        if (onLayoutOrViewModeChanged)
            onLayoutOrViewModeChanged();
        else
            resized();
    }

    void PlayModeEditor::openArpDrawer() { openArpPanel(); }

    void PlayModeEditor::refreshFromPatch()
    {
        nodeSelectorRow_.repaint();
        if (viewMode_ == layout::PlayViewMode::Advanced)
            liveTopologyStrip_.repaint();
        patchFocusPanel_.refreshFromPatch();
        compactEditor_.refreshFromPatch();
    }

    void PlayModeEditor::syncNodeSelection(int nodeIndex)
    {
        nodeSelectorRow_.setSelectedNode(nodeIndex);
        liveTopologyStrip_.setSelectedNode(nodeIndex);
        liveTopologyStrip_.setActiveOperator(nodeIndex);
        operatorEditorPanel_.showNode(nodeIndex);
        if (topologyGraphOverlay_.isVisible())
            topologyGraphOverlay_.showOverlay(nodeIndex);
        updateScopeUi();
    }

    void PlayModeEditor::openGraphOverlay()
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        addAndMakeVisible(topologyGraphOverlay_);
        topologyGraphOverlay_.setBounds(getLocalBounds());
        topologyGraphOverlay_.showOverlay(nodeSelectorRow_.getSelectedNode());
        topologyGraphOverlay_.toFront(false);
    }

    void PlayModeEditor::closeGraphOverlay()
    {
        removeChildComponent(&topologyGraphOverlay_);
    }

    void PlayModeEditor::setBrowseFilter(const content::PresetMetadataFilter& filter)
    {
        compactEditor_.setBrowseFilter(filter);
    }

    bool PlayModeEditor::isCompactView() const noexcept { return viewMode_ == layout::PlayViewMode::Compact; }

    bool PlayModeEditor::keyPressed(const juce::KeyPress& key)
    {
        if (vocoderLabPanel_.isVisible())
            return vocoderLabPanel_.keyPressed(key);

        if (dualLfoLabPanel_.isVisible())
            return dualLfoLabPanel_.keyPressed(key);

        if (wavetableLabPanel_.isVisible())
            return wavetableLabPanel_.keyPressed(key);

        if (arpPanelOverlay_.isVisible())
            return arpPanelOverlay_.keyPressed(key);

        if (engineDetailOverlay_.isVisible())
            return engineDetailOverlay_.keyPressed(key);

        if (modRoutingOverlay_.isVisible())
            return modRoutingOverlay_.keyPressed(key);

        if (viewMode_ == layout::PlayViewMode::Advanced && topologyGraphOverlay_.isVisible())
            return topologyGraphOverlay_.keyPressed(key);

        if (key == juce::KeyPress('a', juce::ModifierKeys::noModifiers, 0))
        {
            openArpPanel();
            return true;
        }

        if (viewMode_ == layout::PlayViewMode::Advanced && key == juce::KeyPress('m', juce::ModifierKeys::noModifiers, 0))
        {
            openModRoutingOverlay();
            return true;
        }

        if (viewMode_ == layout::PlayViewMode::Advanced && key == juce::KeyPress('v', juce::ModifierKeys::noModifiers, 0))
        {
            openVocoderLab(preferredVocoderFxSlotIndex());
            return true;
        }

        if (viewMode_ == layout::PlayViewMode::Advanced && key == juce::KeyPress('l', juce::ModifierKeys::noModifiers, 0))
        {
            openDualLfoLab();
            return true;
        }

        if (viewMode_ == layout::PlayViewMode::Advanced && key == juce::KeyPress('t', juce::ModifierKeys::noModifiers, 0))
        {
            openWavetableLab(nodeSelectorRow_.getSelectedNode());
            return true;
        }

        return false;
    }

    void PlayModeEditor::openEngineDetail(int engineIndex)
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        addAndMakeVisible(engineDetailOverlay_);
        engineDetailOverlay_.setBounds(getLocalBounds());
        engineDetailOverlay_.showForEngine(engineIndex);
        engineDetailOverlay_.toFront(false);
        syncNodeSelection(engineIndex);
    }

    void PlayModeEditor::closeEngineDetail()
    {
        engineDetailOverlay_.dismiss();
        removeChildComponent(&engineDetailOverlay_);
    }

    std::size_t PlayModeEditor::preferredVocoderFxSlotIndex() const
    {
        auto& apvts = processor_.apvts;
        for (std::size_t i = 0; i < 7; ++i)
        {
            if (readFxType(apvts, fxSlotParamPrefix(i)) == 11)
                return i;
        }
        return 2;
    }

    void PlayModeEditor::openVocoderLab(std::size_t fxSlotIndex)
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        closeDualLfoLab();
        closeEngineDetail();
        closeGraphOverlay();

        addAndMakeVisible(vocoderLabPanel_);
        vocoderLabPanel_.setBounds(getLocalBounds());
        vocoderLabPanel_.showForFxSlot(fxSlotIndex);
        vocoderLabPanel_.toFront(false);
    }

    void PlayModeEditor::closeVocoderLab()
    {
        vocoderLabPanel_.dismiss();
        removeChildComponent(&vocoderLabPanel_);
    }

    void PlayModeEditor::openDualLfoLab()
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        closeVocoderLab();
        closeEngineDetail();
        closeGraphOverlay();

        addAndMakeVisible(dualLfoLabPanel_);
        dualLfoLabPanel_.setBounds(getLocalBounds());
        dualLfoLabPanel_.showOverlay();
        dualLfoLabPanel_.toFront(false);
    }

    void PlayModeEditor::closeDualLfoLab()
    {
        dualLfoLabPanel_.dismiss();
        removeChildComponent(&dualLfoLabPanel_);
    }

    void PlayModeEditor::openWavetableLab(int engineIndex)
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        addAndMakeVisible(wavetableLabPanel_);
        wavetableLabPanel_.setBounds(getLocalBounds());
        wavetableLabPanel_.showForEngine(engineIndex);
        wavetableLabPanel_.toFront(false);
    }

    void PlayModeEditor::closeWavetableLab()
    {
        if (!wavetableLabPanel_.isVisible() && wavetableLabPanel_.getParentComponent() == nullptr)
            return;

        wavetableLabPanel_.dismiss();
        removeChildComponent(&wavetableLabPanel_);
    }

    void PlayModeEditor::openModRoutingOverlay()
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
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
        if (viewMode_ != layout::PlayViewMode::Advanced)
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
        if (viewMode_ != layout::PlayViewMode::Advanced)
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
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        if (nodeSelectorRow_.isGlobalScope() && page == Page::Osc)
            page = Page::Filter;

        currentPage_ = page;
        oscPage_.setVisible(page == Page::Osc);
        filterPage_.setVisible(page == Page::Filter);
        envPage_.setVisible(page == Page::Env);
        modPage_.setVisible(page == Page::Mod);
        fxPage_.setVisible(page == Page::Fx);
        globalPage_.setVisible(page == Page::Global);

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
        auto bounds = getLocalBounds();

        if (viewMode_ == layout::PlayViewMode::Compact)
        {
            compactEditor_.setBounds(bounds);
            modRoutingOverlay_.setBounds(getLocalBounds());
            arpPanelOverlay_.setBounds(getLocalBounds());
            return;
        }

        const bool advanced = viewMode_ == layout::PlayViewMode::Advanced;
        const bool basic = viewMode_ == layout::PlayViewMode::Basic;

        if (basic)
        {
            auto upperDeck = bounds.removeFromTop(layout::kIpadPlayUpperDeckHeight);
            auto masterDeck = upperDeck.removeFromRight(layout::kIpadPlayMasterDeckWidth);
            upperDeck.removeFromRight(layout::kIpadPlayUpperDeckGap);
            desktopScope_.setBounds(upperDeck);
            masterOutputDeck_.setBounds(masterDeck);

            bounds.removeFromTop(layout::kDesktopPlayModeSectionGap);
            patchFocusPanel_.setBounds(bounds.removeFromTop(layout::kIpadPlayMacrosDeckHeight));
            bounds.removeFromTop(layout::kDesktopPlayModeSectionGap);
            vstBottomBar_.setBounds(bounds.removeFromTop(layout::kDesktopPlayModeBottomBarHeight));
        }
        else if (advanced)
        {
            if (modAssignmentBanner_.isVisible())
            {
                modAssignmentBanner_.setBounds(bounds.removeFromTop(layout::kModBannerHeight));
                bounds.removeFromTop(layout::kSectionGap);
            }

            advancedGridPage_.setBounds(bounds);
            auto gridBounds = advancedGridPage_.getLocalBounds();
            vstBottomBar_.setBounds(gridBounds.removeFromBottom(layout::kVstBottomBarHeight));
            gridBounds.removeFromBottom(layout::kSectionGap);
            dashboardStrip_.setBounds(gridBounds.removeFromBottom(layout::kDashboardStripHeight));
            gridBounds.removeFromBottom(layout::kSectionGap);
            engineGridPanel_.setBounds(gridBounds);
        }

        if (advanced)
        {
            oscPage_.setBounds(bounds);
            filterPage_.setBounds(bounds);
            envPage_.setBounds(bounds);
            modPage_.setBounds(bounds);
            fxPage_.setBounds(bounds);
            globalPage_.setBounds(bounds);
        }
        else if (!basic)
        {
            patchFocusPanel_.setBounds(bounds);
            oscPage_.setBounds(0, 0, 0, 0);
            filterPage_.setBounds(0, 0, 0, 0);
            envPage_.setBounds(0, 0, 0, 0);
            modPage_.setBounds(0, 0, 0, 0);
            fxPage_.setBounds(0, 0, 0, 0);
            globalPage_.setBounds(0, 0, 0, 0);
        }

        oscPanel_.setBounds(oscPage_.getLocalBounds());
        operatorEditorPanel_.setBounds(oscPanel_.getContentBounds());
        filterLfoPanel_.setBounds(filterPage_.getLocalBounds());
        ampEnvelopePanel_.setBounds(envPage_.getLocalBounds());
        modLauncherPanel_.setBounds(modPage_.getLocalBounds());
        fxChainStrip_.setBounds(fxPage_.getLocalBounds());
        globalPanel_.setBounds(globalPage_.getLocalBounds());

        modRoutingOverlay_.setBounds(getLocalBounds());
        arpPanelOverlay_.setBounds(getLocalBounds());
        topologyGraphOverlay_.setBounds(getLocalBounds());
        engineDetailOverlay_.setBounds(getLocalBounds());
        vocoderLabPanel_.setBounds(getLocalBounds());
        dualLfoLabPanel_.setBounds(getLocalBounds());
        wavetableLabPanel_.setBounds(getLocalBounds());
    }

} // namespace pw8::plugin::ui
