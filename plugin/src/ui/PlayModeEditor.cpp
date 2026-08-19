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

    PlayModeEditor::PlayModeEditor(MurmurProcessor& processor, SharedEditorChrome& chrome)
        : processor_(processor),
          chrome_(chrome),
          nodeSelectorRow_(processor),
          liveTopologyStrip_(processor),
          engineGridPanel_(processor, modAssignmentController_),
          dashboardStrip_(processor, modAssignmentController_),
          vstBottomBar_(processor),
          engineDetailOverlay_(processor, modAssignmentController_),
          topologyGraphOverlay_(processor),
          contextStrip_(processor),
          desktopScope_(processor),
          masterOutputDeck_(processor),
          ipadPlayMasterStrip_(processor),
          masterEnvelopePanel_(processor),
          basicPerformanceSidebar_(processor),
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
          masterMotionLabPanel_(processor),
          masterQuasarPanel_(processor),
          wavetableLabPanel_(processor, modAssignmentController_)
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

        modLauncherPanel_.onOpenAdvanced = [this] { requestDesignSubPage(layout::DesignSubPage::ModMatrix); };

        addChildComponent(modRoutingOverlay_);
        modRoutingOverlay_.onClosed = [this] { closeModRoutingOverlay(); };

        addChildComponent(arpPanelOverlay_);
        arpPanelOverlay_.onClosed = [this] {
            closeArpPanel();
            syncIpadFooterPill();
        };

        addChildComponent(vocoderLabPanel_);
        vocoderLabPanel_.onClosed = [this] { closeVocoderLab(); };

        addChildComponent(dualLfoLabPanel_);
        dualLfoLabPanel_.onClosed = [this] { closeDualLfoLab(); };
        dualLfoLabPanel_.onOpenModMatrix = [this] {
            closeDualLfoLab();
            requestDesignSubPage(layout::DesignSubPage::ModMatrix);
        };

        addChildComponent(masterMotionLabPanel_);
        masterMotionLabPanel_.onClosed = [this] { closeMasterMotionLab(); };
        masterMotionLabPanel_.onOpenModMatrix = [this] {
            closeMasterMotionLab();
            requestDesignSubPage(layout::DesignSubPage::ModMatrix);
        };
        masterMotionLabPanel_.onOpenMorphEditor = [this] {
            closeMasterMotionLab();
            requestDesignSubPage(layout::DesignSubPage::Morph);
        };
        masterMotionLabPanel_.onOpenEnvelopeSegments = [this] {
            closeMasterMotionLab();
            requestDesignSubPage(layout::DesignSubPage::EnvelopeSegments);
        };

        addChildComponent(masterQuasarPanel_);
        masterQuasarPanel_.onClosed = [this] { closeMasterQuasarLab(); };
        masterQuasarPanel_.onOpenFxChain = [this] {
            closeMasterQuasarLab();
            requestDesignSubPage(layout::DesignSubPage::Fx);
        };

        addChildComponent(wavetableLabPanel_);
        wavetableLabPanel_.onClosed = [this] { closeWavetableLab(); };

        dashboardStrip_.onVocoderLabRequested = [this](std::size_t) {
            requestDesignSubPage(layout::DesignSubPage::Vocoder);
        };
        dashboardStrip_.onLfoLabRequested = [this] { requestDesignSubPage(layout::DesignSubPage::DualLfo); };
        fxChainStrip_.onQuasarLabRequested = [this](std::size_t slotIndex) {
            openMasterQuasarLab(slotIndex >= 3 ? slotIndex : preferredQuasarFxSlotIndex());
        };

        vstBottomBar_.onVocoderLabRequested = [this] { requestDesignSubPage(layout::DesignSubPage::Vocoder); };
        vstBottomBar_.onQuasarLabRequested = [this] { openMasterQuasarLab(preferredQuasarFxSlotIndex()); };
        vstBottomBar_.onLfoLabRequested = [this] { requestDesignSubPage(layout::DesignSubPage::DualLfo); };
        vstBottomBar_.onMotionLabRequested = [this] { requestDesignSubPage(layout::DesignSubPage::Morph); };
        vstBottomBar_.onModMatrixRequested = [this] { requestDesignSubPage(layout::DesignSubPage::ModMatrix); };
        vstBottomBar_.onIpadFooterPillSelected = [this](layout::IpadFooterPill pill) { handleIpadFooterPill(pill); };

        addChildComponent(compactEditor_);

        addChildComponent(topologyGraphOverlay_);
        topologyGraphOverlay_.onDismissed = [this] { closeGraphOverlay(); };
        topologyGraphOverlay_.onNodeSelected = [this](int node) { syncNodeSelection(node); };

        advancedGridPage_.addAndMakeVisible(engineGridPanel_);
        advancedGridPage_.addAndMakeVisible(dashboardStrip_);
        addAndMakeVisible(vstBottomBar_);
        addChildComponent(advancedGridPage_);
        vstBottomBar_.setVisible(false);
        engineGridPanel_.onEngineDoubleClicked = [this](int engine) {
            requestDesignSubPage(layout::DesignSubPage::Engine);
            juce::ignoreUnused(engine);
        };

        addChildComponent(engineDetailOverlay_);
        engineDetailOverlay_.onClosed = [this] { closeEngineDetail(); };

        addAndMakeVisible(desktopScope_);
        desktopScope_.setVisible(false);

        addAndMakeVisible(masterOutputDeck_);
        masterOutputDeck_.setVisible(false);

        addAndMakeVisible(ipadPlayMasterStrip_);
        ipadPlayMasterStrip_.setVisible(false);

        addAndMakeVisible(masterEnvelopePanel_);
        masterEnvelopePanel_.setVisible(false);

        addAndMakeVisible(basicPerformanceSidebar_);
        basicPerformanceSidebar_.setVisible(false);

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
        patchFocusPanel_.onAdvancedClicked = [this] { requestDesignSubPage(layout::DesignSubPage::ModMatrix); };

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
        {
            auto& btn = tabButtons_[i];
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(9001);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::buttonOnColourId, branding::glowColour().withAlpha(0.28f));
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn.setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
            btn.onClick = [this, page = static_cast<Page>(i)] {
                juce::ignoreUnused(page);
                requestDesignSubPage(layout::DesignSubPage::Engine);
            };
            addChildComponent(btn);
        }

        boardTabButton_.setClickingTogglesState(true);
        boardTabButton_.setRadioGroupId(9001);
        boardTabButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        boardTabButton_.setColour(juce::TextButton::buttonOnColourId, branding::glowColour().withAlpha(0.28f));
        boardTabButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        boardTabButton_.setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
        boardTabButton_.onClick = [this] { requestDesignSubPage(layout::DesignSubPage::Engine); };
        addChildComponent(boardTabButton_);

        motionTabButton_.setClickingTogglesState(false);
        motionTabButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        motionTabButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        motionTabButton_.onClick = [this] { requestDesignSubPage(layout::DesignSubPage::Morph); };
        addChildComponent(motionTabButton_);

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
        if (mode == layout::PlayViewMode::Advanced)
        {
            requestDesignSubPage(layout::DesignSubPage::Engine);
            return;
        }

        if (settingPlayViewMode_)
            return;

        if (viewMode_ == mode && playViewLayoutApplied_)
            return;

        const juce::ScopedValueSetter<bool> guard(settingPlayViewMode_, true);

        if (viewMode_ == layout::PlayViewMode::Compact && mode != layout::PlayViewMode::Compact)
            closeArpPanel();

        viewMode_ = mode;
        playViewLayoutApplied_ = true;

        const bool compact = mode == layout::PlayViewMode::Compact;
        const bool advanced = mode == layout::PlayViewMode::Advanced;
        const bool desktopPlay = layout::isDesktopPlayLayout(mode);

        compactEditor_.setVisible(compact);
        if (compact)
            compactEditor_.setBounds(getLocalBounds());

        nodeSelectorRow_.setVisible(false);
        liveTopologyStrip_.setVisible(false);
        contextStrip_.setVisible(false);
        advancedGridPage_.setVisible(advanced);
        desktopScope_.setVisible(desktopPlay);
        desktopScope_.setDesktopPlayModeLayout(desktopPlay);
        desktopScope_.setIpadPlayLayout(false);
        masterOutputDeck_.setVisible(false);
        ipadPlayMasterStrip_.setVisible(false);
        masterEnvelopePanel_.setVisible(false);
        masterEnvelopePanel_.setCompactSectionMode(false);
        basicPerformanceSidebar_.setVisible(false);
        if (desktopPlay)
        {
            masterEnvelopePanel_.setCompactSectionMode(true);
            masterEnvelopePanel_.setVisible(true);
        }
        vstBottomBar_.setVisible(advanced || desktopPlay);
        vstBottomBar_.setPlayBoardMode(advanced);
        vstBottomBar_.setDesktopPlayMode(desktopPlay);
        vstBottomBar_.setIpadPlayMode(false);
        vstBottomBar_.setMurmurBasicViewMode(false);
        patchFocusPanel_.setDesktopPlayModeLayout(desktopPlay);
        patchFocusPanel_.setIpadPlayLayout(false);

        if (!advanced)
        {
            closeGraphOverlay();
            closeEngineDetail();
            closeVocoderLab();
            closeDualLfoLab();
            closeMasterMotionLab();
            closeWavetableLab();
        }

        boardTabButton_.setVisible(advanced);
        motionTabButton_.setVisible(advanced);
        for (auto& btn : tabButtons_)
            btn.setVisible(advanced);

        patchFocusPanel_.setBasicPerformanceLayout(desktopPlay);
        patchFocusPanel_.setVisible(desktopPlay);

        if (compact || desktopPlay)
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
            advancedSubview_ = AdvancedSubview::Board;
            nodeSelectorRow_.setVisible(false);
            contextStrip_.setVisible(false);
            liveTopologyStrip_.setVisible(false);
            showBoard();
        }

        if (onLayoutOrViewModeChanged)
            onLayoutOrViewModeChanged();
        else
            resized();

        syncIpadFooterPill();
    }

    void PlayModeEditor::syncIpadFooterPill()
    {
        if (viewMode_ != layout::PlayViewMode::Basic)
            return;

        if (arpPanelOverlay_.isVisible())
            vstBottomBar_.setIpadFooterPillActive(layout::IpadFooterPill::Arp);
        else
            vstBottomBar_.setIpadFooterPillActive(layout::IpadFooterPill::Play);
    }

    void PlayModeEditor::requestDesignSubPage(layout::DesignSubPage page)
    {
        closeModRoutingOverlay();
        closeVocoderLab();
        closeDualLfoLab();
        closeMasterMotionLab();
        closeMasterQuasarLab();
        closeWavetableLab();
        closeEngineDetail();
        closeGraphOverlay();
        modAssignmentController_.disarm();

        if (onEditorModeChangeRequested)
            onEditorModeChangeRequested(layout::EditorMode::Design);
        if (onDesignSubPageChangeRequested)
            onDesignSubPageChangeRequested(page);
    }

    void PlayModeEditor::handleIpadFooterPill(layout::IpadFooterPill pill)
    {
        switch (pill)
        {
            case layout::IpadFooterPill::Play:
                closeArpPanel();
                if (onEditorModeChangeRequested)
                    onEditorModeChangeRequested(layout::EditorMode::Play);
                syncIpadFooterPill();
                break;
            case layout::IpadFooterPill::Design:
                requestDesignSubPage(layout::DesignSubPage::Engine);
                break;
            case layout::IpadFooterPill::Voc:
                requestDesignSubPage(layout::DesignSubPage::Vocoder);
                break;
            case layout::IpadFooterPill::Arp:
                if (onEditorModeChangeRequested)
                    onEditorModeChangeRequested(layout::EditorMode::Play);
                openArpPanel();
                vstBottomBar_.setIpadFooterPillActive(layout::IpadFooterPill::Arp);
                break;
            case layout::IpadFooterPill::Lfo:
                requestDesignSubPage(layout::DesignSubPage::DualLfo);
                break;
        }
    }

    void PlayModeEditor::openArpDrawer() { openArpPanel(); }

    void PlayModeEditor::refreshFromPatch()
    {
        nodeSelectorRow_.repaint();
        if (viewMode_ == layout::PlayViewMode::Advanced)
            liveTopologyStrip_.repaint();
        patchFocusPanel_.refreshFromPatch();
        basicPerformanceSidebar_.refreshFromPatch();
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
        juce::ignoreUnused(filter);
    }

    bool PlayModeEditor::isCompactView() const noexcept { return viewMode_ == layout::PlayViewMode::Compact; }

    bool PlayModeEditor::keyPressed(const juce::KeyPress& key)
    {
        if (vocoderLabPanel_.isVisible())
            return vocoderLabPanel_.keyPressed(key);

        if (dualLfoLabPanel_.isVisible())
            return dualLfoLabPanel_.keyPressed(key);

        if (masterMotionLabPanel_.isVisible())
            return masterMotionLabPanel_.keyPressed(key);

        if (masterQuasarPanel_.isVisible())
            return masterQuasarPanel_.keyPressed(key);

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

        if (key == juce::KeyPress('m', juce::ModifierKeys::noModifiers, 0))
        {
            requestDesignSubPage(layout::DesignSubPage::ModMatrix);
            return true;
        }

        if (key == juce::KeyPress('v', juce::ModifierKeys::noModifiers, 0))
        {
            requestDesignSubPage(layout::DesignSubPage::Vocoder);
            return true;
        }

        if (key == juce::KeyPress('l', juce::ModifierKeys::noModifiers, 0))
        {
            requestDesignSubPage(layout::DesignSubPage::DualLfo);
            return true;
        }

        if (key == juce::KeyPress('t', juce::ModifierKeys::noModifiers, 0))
        {
            requestDesignSubPage(layout::DesignSubPage::Wavetable);
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

    std::size_t PlayModeEditor::preferredQuasarFxSlotIndex() const
    {
        auto& apvts = processor_.apvts;
        for (std::size_t i = 3; i < 7; ++i)
        {
            if (readFxType(apvts, fxSlotParamPrefix(i)) == 13)
                return i;
        }
        return 5;
    }

    void PlayModeEditor::openVocoderLab(std::size_t fxSlotIndex)
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        closeDualLfoLab();
        closeEngineDetail();
        closeGraphOverlay();
        closeMasterQuasarLab();

        addAndMakeVisible(vocoderLabPanel_);
        vocoderLabPanel_.setBounds(getLocalBounds());
        vocoderLabPanel_.showForFxSlot(fxSlotIndex);
        vocoderLabPanel_.toFront(false);
        playLabOverlay_ = layout::PlayLabOverlay::Vocoder;
    }

    void PlayModeEditor::closeVocoderLab()
    {
        vocoderLabPanel_.dismiss();
        removeChildComponent(&vocoderLabPanel_);
        if (playLabOverlay_ == layout::PlayLabOverlay::Vocoder)
            playLabOverlay_ = layout::PlayLabOverlay::None;
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

    void PlayModeEditor::openMasterMotionLab()
    {
        if (viewMode_ != layout::PlayViewMode::Advanced && viewMode_ != layout::PlayViewMode::Basic)
            return;

        closeVocoderLab();
        closeDualLfoLab();
        closeEngineDetail();
        closeGraphOverlay();
        closeWavetableLab();
        closeMasterQuasarLab();

        addAndMakeVisible(masterMotionLabPanel_);
        masterMotionLabPanel_.setBounds(getLocalBounds());
        masterMotionLabPanel_.showOverlay();
        masterMotionLabPanel_.toFront(false);
    }

    void PlayModeEditor::closeMasterMotionLab()
    {
        masterMotionLabPanel_.dismiss();
        removeChildComponent(&masterMotionLabPanel_);
    }

    void PlayModeEditor::openMasterQuasarLab(std::size_t fxSlotIndex)
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        closeVocoderLab();
        closeDualLfoLab();
        closeEngineDetail();
        closeGraphOverlay();
        closeMasterMotionLab();
        closeWavetableLab();

        addAndMakeVisible(masterQuasarPanel_);
        masterQuasarPanel_.setBounds(getLocalBounds());
        masterQuasarPanel_.showForFxSlot(fxSlotIndex);
        masterQuasarPanel_.toFront(false);
        playLabOverlay_ = layout::PlayLabOverlay::Quasar;
    }

    void PlayModeEditor::closeMasterQuasarLab()
    {
        masterQuasarPanel_.dismiss();
        removeChildComponent(&masterQuasarPanel_);
        if (playLabOverlay_ == layout::PlayLabOverlay::Quasar)
            playLabOverlay_ = layout::PlayLabOverlay::None;
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
        if (viewMode_ == layout::PlayViewMode::Basic)
            vstBottomBar_.setIpadFooterPillActive(layout::IpadFooterPill::Arp);
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

    void PlayModeEditor::showBoard()
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        advancedSubview_ = AdvancedSubview::Board;
        advancedGridPage_.setVisible(true);
        nodeSelectorRow_.setVisible(false);
        contextStrip_.setVisible(false);
        liveTopologyStrip_.setVisible(false);

        oscPage_.setVisible(false);
        filterPage_.setVisible(false);
        envPage_.setVisible(false);
        modPage_.setVisible(false);
        fxPage_.setVisible(false);
        globalPage_.setVisible(false);

        boardTabButton_.setToggleState(true, juce::dontSendNotification);
        for (auto& btn : tabButtons_)
            btn.setToggleState(false, juce::dontSendNotification);

        resized();
    }

    void PlayModeEditor::layoutAdvancedTabRow(juce::Rectangle<int> tabRow)
    {
        const int motionW = 64;
        motionTabButton_.setBounds(tabRow.removeFromRight(motionW));
        tabRow.removeFromRight(4);

        const int boardW = 56;
        boardTabButton_.setBounds(tabRow.removeFromLeft(boardW));
        tabRow.removeFromLeft(4);

        const int tabW = juce::jmax(44, tabRow.getWidth() / static_cast<int>(tabButtons_.size()));
        for (auto& btn : tabButtons_)
        {
            btn.setBounds(tabRow.removeFromLeft(tabW).reduced(0, 2));
            tabRow.removeFromLeft(2);
        }
    }

    void PlayModeEditor::showPage(Page page)
    {
        if (viewMode_ != layout::PlayViewMode::Advanced)
            return;

        if (nodeSelectorRow_.isGlobalScope() && page == Page::Osc)
            page = Page::Filter;

        advancedSubview_ = AdvancedSubview::Paged;
        advancedGridPage_.setVisible(false);
        nodeSelectorRow_.setVisible(true);
        contextStrip_.setVisible(true);
        liveTopologyStrip_.setVisible(false);

        currentPage_ = page;
        oscPage_.setVisible(page == Page::Osc);
        filterPage_.setVisible(page == Page::Filter);
        envPage_.setVisible(page == Page::Env);
        modPage_.setVisible(page == Page::Mod);
        fxPage_.setVisible(page == Page::Fx);
        globalPage_.setVisible(page == Page::Global);

        boardTabButton_.setToggleState(false, juce::dontSendNotification);
        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
            tabButtons_[i].setToggleState(static_cast<Page>(i) == page, juce::dontSendNotification);

        if (page == Page::Filter)
            refreshFilterPanelScope();

        updateScopeUi();
        resized();
    }

    void PlayModeEditor::openFilterPage()
    {
        requestDesignSubPage(layout::DesignSubPage::FilterLab);
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
        const bool desktopPlay = layout::isDesktopPlayLayout(viewMode_);

        if (desktopPlay)
        {
            auto body = bounds.reduced(layout::kDesktopPlayModeOuterMargin, 0);

            vstBottomBar_.setBounds(body.removeFromBottom(layout::kDesktopPlayModeBottomBarHeight));
            body.removeFromBottom(layout::kDesktopPlayModeSectionGap);

            const int macroMin = layout::kDesktopPlayModePerformanceDeckHeight;
            const int envelopeMin = layout::kDesktopPlayModeMasterEnvelopePlayHeight;
            const int scopeMin = layout::kDesktopPlayModeOscilloscopeHeight;
            const int gapCount = 3;
            const int fixedGaps = layout::kDesktopPlayModeSectionGap * gapCount;
            const int minStack = scopeMin + envelopeMin + macroMin;
            const int flexExtra = juce::jmax(0, body.getHeight() - minStack - fixedGaps);

            const int scopeH = scopeMin + flexExtra;

            desktopScope_.setBounds(body.removeFromTop(scopeH));
            body.removeFromTop(layout::kDesktopPlayModeSectionGap);

            masterEnvelopePanel_.setBounds(body.removeFromTop(envelopeMin));
            body.removeFromTop(layout::kDesktopPlayModeSectionGap);

            patchFocusPanel_.setBounds(body);
        }
        else if (advanced)
        {
            if (modAssignmentBanner_.isVisible())
            {
                modAssignmentBanner_.setBounds(bounds.removeFromTop(layout::kModBannerHeight));
                bounds.removeFromTop(layout::kSectionGap);
            }

            if (advancedSubview_ == AdvancedSubview::Paged)
            {
                auto tabRow = bounds.removeFromTop(layout::kTabRowHeight);
                layoutAdvancedTabRow(tabRow);
                bounds.removeFromTop(layout::kSectionGap);

                nodeSelectorRow_.setBounds(bounds.removeFromTop(36));
                bounds.removeFromTop(4);
                contextStrip_.setBounds(bounds.removeFromTop(layout::kContextRowHeight));
                bounds.removeFromTop(layout::kSectionGap);
            }
            else
            {
                auto tabRow = bounds.removeFromTop(layout::kTabRowHeight);
                layoutAdvancedTabRow(tabRow);
                bounds.removeFromTop(layout::kSectionGap);
            }

            vstBottomBar_.setBounds(bounds.removeFromBottom(layout::kVstBottomBarHeight));
            bounds.removeFromBottom(layout::kSectionGap);

            if (advancedSubview_ == AdvancedSubview::Board)
            {
                advancedGridPage_.setBounds(bounds);
                auto gridBounds = advancedGridPage_.getLocalBounds();
                dashboardStrip_.setBounds(gridBounds.removeFromBottom(layout::kDashboardStripHeight));
                gridBounds.removeFromBottom(layout::kSectionGap);
                engineGridPanel_.setBounds(gridBounds);
            }
        }

        if (advanced)
        {
            if (advancedSubview_ == AdvancedSubview::Paged)
            {
                oscPage_.setBounds(bounds);
                filterPage_.setBounds(bounds);
                envPage_.setBounds(bounds);
                modPage_.setBounds(bounds);
                fxPage_.setBounds(bounds);
                globalPage_.setBounds(bounds);
            }
            else
            {
                oscPage_.setBounds(0, 0, 0, 0);
                filterPage_.setBounds(0, 0, 0, 0);
                envPage_.setBounds(0, 0, 0, 0);
                modPage_.setBounds(0, 0, 0, 0);
                fxPage_.setBounds(0, 0, 0, 0);
                globalPage_.setBounds(0, 0, 0, 0);
            }
        }
        else if (!desktopPlay)
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
        masterMotionLabPanel_.setBounds(getLocalBounds());
        masterQuasarPanel_.setBounds(getLocalBounds());
        wavetableLabPanel_.setBounds(getLocalBounds());
    }

} // namespace pw8::plugin::ui
