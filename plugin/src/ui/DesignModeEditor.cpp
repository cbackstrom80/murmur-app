#include "DesignModeEditor.h"

#include "PlayModeLayout.h"
#include "state/PluginState.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    DesignModeEditor::DesignModeEditor(MurmurProcessor& processor, SharedEditorChrome& chrome)
        : masterEnvelopePanel_(processor),
          engineGridPanel_(processor, modAssignmentController_),
          statusBar_(processor),
          engineDetailOverlay_(processor, modAssignmentController_),
          arpPanel_(processor),
          vocoderPanel_(processor),
          fxPanel_(processor, modAssignmentController_),
          masterQuasarPanel_(processor),
          modMatrixPanel_(processor, modAssignmentController_),
          wavetablePanel_(processor, modAssignmentController_),
          dualLfoPanel_(processor),
          morphPanel_(processor),
          filterLabPanel_(processor, modAssignmentController_),
          dynamicsLabPanel_(processor),
          generativeLabPanel_(processor),
          utilityPeaksPanel_(processor),
          envelopeSegmentsPanel_(processor)
    {
        juce::ignoreUnused(chrome);
        setLookAndFeel(&lookAndFeel_);

        masterQuasarPanel_.setEmbeddedInDesignMode(true);

        engineGridPanel_.setDesignModeV2Layout(true);
        engineGridPanel_.onEngineDoubleClicked = [this](int engine) { openEngineDetail(engine); };
        engineGridPanel_.onWavetableLabRequested = [this](int engine) {
            wavetableEngineIndex_ = engine;
            setDesignSubPage(layout::DesignSubPage::Wavetable);
        };
        statusBar_.setDesignModeV2Layout(true);
        statusBar_.onVocoderLabRequested = [this] { setDesignSubPage(layout::DesignSubPage::Vocoder); };
        statusBar_.onLfoLabRequested = [this] { setDesignSubPage(layout::DesignSubPage::DualLfo); };
        statusBar_.onModMatrixRequested = [this] { setDesignSubPage(layout::DesignSubPage::ModMatrix); };
        statusBar_.onMorphEditorRequested = [this] { setDesignSubPage(layout::DesignSubPage::Morph); };
        statusBar_.onFilterLabRequested = [this] { setDesignSubPage(layout::DesignSubPage::FilterLab); };
        statusBar_.onDynamicsLabRequested = [this] { setDesignSubPage(layout::DesignSubPage::DynamicsLab); };
        statusBar_.onGenerativeLabRequested = [this] { setDesignSubPage(layout::DesignSubPage::GenerativeLab); };
        statusBar_.onUtilityPeaksRequested = [this] { setDesignSubPage(layout::DesignSubPage::UtilityPeaks); };
        statusBar_.onEnvelopeSegmentsRequested = [this] { setDesignSubPage(layout::DesignSubPage::EnvelopeSegments); };

        masterEnvelopePanel_.setDesignEngineLayout(true);

        enginePage_.addAndMakeVisible(masterEnvelopePanel_);
        enginePage_.addAndMakeVisible(engineGridPanel_);
        enginePage_.addAndMakeVisible(statusBar_);
        addAndMakeVisible(enginePage_);

        addChildComponent(arpPanel_);
        addChildComponent(vocoderPanel_);
        addChildComponent(fxPanel_);
        addChildComponent(masterQuasarPanel_);
        addChildComponent(modMatrixPanel_);
        addChildComponent(wavetablePanel_);
        addChildComponent(dualLfoPanel_);
        addChildComponent(morphPanel_);
        addChildComponent(filterLabPanel_);
        addChildComponent(dynamicsLabPanel_);
        addChildComponent(generativeLabPanel_);
        addChildComponent(utilityPeaksPanel_);
        addChildComponent(envelopeSegmentsPanel_);

        arpPanel_.setEmbeddedInDesignMode(true);
        vocoderPanel_.setEmbeddedInDesignMode(true);
        fxPanel_.setEmbeddedInDesignMode(true);
        modMatrixPanel_.setEmbeddedInDesignMode(true);
        wavetablePanel_.setEmbeddedInDesignMode(true);
        dualLfoPanel_.setEmbeddedInDesignMode(true);
        morphPanel_.setEmbeddedInDesignMode(true);
        filterLabPanel_.setEmbeddedInDesignMode(true);
        dynamicsLabPanel_.setEmbeddedInDesignMode(true);
        generativeLabPanel_.setEmbeddedInDesignMode(true);
        utilityPeaksPanel_.setEmbeddedInDesignMode(true);
        envelopeSegmentsPanel_.setEmbeddedInDesignMode(true);

        arpPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        vocoderPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        vocoderPanel_.onOpenFxChain = [this] { setDesignSubPage(layout::DesignSubPage::Fx); };
        fxPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        modMatrixPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        wavetablePanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        dualLfoPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        dualLfoPanel_.onOpenModMatrix = [this] { setDesignSubPage(layout::DesignSubPage::ModMatrix); };
        morphPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        morphPanel_.onOpenModMatrix = [this] { setDesignSubPage(layout::DesignSubPage::ModMatrix); };
        filterLabPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        filterLabPanel_.onOpenModMatrix = [this] { setDesignSubPage(layout::DesignSubPage::ModMatrix); };
        filterLabPanel_.onOpenPlayFilter = [this] {
            if (onOpenPlayFilterRequested)
                onOpenPlayFilterRequested();
        };
        dynamicsLabPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        dynamicsLabPanel_.onOpenPlayOutput = [this] {
            if (onOpenPlayFilterRequested)
                onOpenPlayFilterRequested();
        };
        generativeLabPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        generativeLabPanel_.onOpenModMatrix = [this] { setDesignSubPage(layout::DesignSubPage::ModMatrix); };
        utilityPeaksPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        envelopeSegmentsPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        envelopeSegmentsPanel_.onOpenPlayMotion = [this] {
            if (onOpenPlayFilterRequested)
                onOpenPlayFilterRequested();
        };

        fxPanel_.onVocoderLabRequested = [this](std::size_t slotIndex) {
            juce::ignoreUnused(slotIndex);
            setDesignSubPage(layout::DesignSubPage::Vocoder);
        };
        fxPanel_.onQuasarLabRequested = [this](std::size_t slotIndex) {
            addAndMakeVisible(masterQuasarPanel_);
            masterQuasarPanel_.setBounds(getLocalBounds());
            masterQuasarPanel_.showForFxSlot(slotIndex);
            masterQuasarPanel_.toFront(false);
        };
        masterQuasarPanel_.onClosed = [this] { closeMasterQuasarLab(); };
        masterQuasarPanel_.onOpenFxChain = [this] {
            closeMasterQuasarLab();
            fxPanel_.setFxViewMode(DesignFxPanel::FxViewMode::Chain);
        };

        addChildComponent(engineDetailOverlay_);
        engineDetailOverlay_.onClosed = [this] { closeEngineDetail(); };

        applySubPageVisibility();
    }

    DesignModeEditor::~DesignModeEditor() { setLookAndFeel(nullptr); }

    void DesignModeEditor::closeMasterQuasarLab()
    {
        masterQuasarPanel_.dismiss();
        removeChildComponent(&masterQuasarPanel_);
    }

    void DesignModeEditor::setDesignSubPage(layout::DesignSubPage page)
    {
        if (designSubPage_ == page)
            return;

        closeEngineDetail();
        closeMasterQuasarLab();
        designSubPage_ = page;
        applySubPageVisibility();
        resized();

        if (onDesignSubPageChanged)
            onDesignSubPageChanged(page);
    }

    void DesignModeEditor::applySubPageVisibility()
    {
        enginePage_.setVisible(designSubPage_ == layout::DesignSubPage::Engine);
        arpPanel_.setVisible(designSubPage_ == layout::DesignSubPage::Arp);
        vocoderPanel_.setVisible(designSubPage_ == layout::DesignSubPage::Vocoder);
        fxPanel_.setVisible(designSubPage_ == layout::DesignSubPage::Fx);
        modMatrixPanel_.setVisible(designSubPage_ == layout::DesignSubPage::ModMatrix);
        wavetablePanel_.setVisible(designSubPage_ == layout::DesignSubPage::Wavetable);
        dualLfoPanel_.setVisible(designSubPage_ == layout::DesignSubPage::DualLfo);
        morphPanel_.setVisible(designSubPage_ == layout::DesignSubPage::Morph);
        filterLabPanel_.setVisible(designSubPage_ == layout::DesignSubPage::FilterLab);
        dynamicsLabPanel_.setVisible(designSubPage_ == layout::DesignSubPage::DynamicsLab);
        generativeLabPanel_.setVisible(designSubPage_ == layout::DesignSubPage::GenerativeLab);
        utilityPeaksPanel_.setVisible(designSubPage_ == layout::DesignSubPage::UtilityPeaks);
        envelopeSegmentsPanel_.setVisible(designSubPage_ == layout::DesignSubPage::EnvelopeSegments);

        if (designSubPage_ == layout::DesignSubPage::Arp)
            arpPanel_.showDrawer();
        else if (designSubPage_ == layout::DesignSubPage::Vocoder)
            vocoderPanel_.showForFxSlot(2);
        else if (designSubPage_ == layout::DesignSubPage::ModMatrix)
            modMatrixPanel_.showOverlay();
        else if (designSubPage_ == layout::DesignSubPage::Wavetable)
            wavetablePanel_.showForEngine(wavetableEngineIndex_);
        else if (designSubPage_ == layout::DesignSubPage::DualLfo)
            dualLfoPanel_.showOverlay();
        else if (designSubPage_ == layout::DesignSubPage::Morph)
            morphPanel_.showOverlay();
        else if (designSubPage_ == layout::DesignSubPage::FilterLab)
            filterLabPanel_.showOverlay();
        else if (designSubPage_ == layout::DesignSubPage::DynamicsLab)
            dynamicsLabPanel_.showOverlay();
        else if (designSubPage_ == layout::DesignSubPage::GenerativeLab)
            generativeLabPanel_.showOverlay();
        else if (designSubPage_ == layout::DesignSubPage::UtilityPeaks)
            utilityPeaksPanel_.showOverlay();
        else if (designSubPage_ == layout::DesignSubPage::EnvelopeSegments)
        {
            envelopeSegmentsPanel_.refreshFromPatch();
            envelopeSegmentsPanel_.showOverlay();
        }
    }

    void DesignModeEditor::refreshFromPatch()
    {
        engineGridPanel_.repaint();
        statusBar_.repaint();
    }

    void DesignModeEditor::openEngineDetail(int engineIndex)
    {
        addAndMakeVisible(engineDetailOverlay_);
        engineDetailOverlay_.setBounds(getLocalBounds());
        engineDetailOverlay_.showForEngine(engineIndex);
        engineDetailOverlay_.toFront(false);
    }

    void DesignModeEditor::closeEngineDetail()
    {
        engineDetailOverlay_.dismiss();
        removeChildComponent(&engineDetailOverlay_);
    }

    void DesignModeEditor::paint(juce::Graphics& g) { juce::ignoreUnused(g); }

    void DesignModeEditor::resized()
    {
        auto bounds = getLocalBounds();

        if (designSubPage_ == layout::DesignSubPage::Engine)
        {
            enginePage_.setBounds(bounds);
            auto engineBounds = enginePage_.getLocalBounds();
            statusBar_.setBounds(engineBounds.removeFromBottom(layout::kDesignModeV2StatusBarHeight));
            engineBounds.removeFromBottom(layout::kDesignModeV2SectionGap);
            masterEnvelopePanel_.setBounds(engineBounds.removeFromTop(layout::kDesignModeV2MasterEnvelopePanelHeight));
            engineBounds.removeFromTop(layout::kDesignModeV2SectionGap);
            engineGridPanel_.setBounds(engineBounds);
        }
        else
        {
            arpPanel_.setBounds(bounds);
            vocoderPanel_.setBounds(bounds);
            fxPanel_.setBounds(bounds);
            modMatrixPanel_.setBounds(bounds);
            wavetablePanel_.setBounds(bounds);
            dualLfoPanel_.setBounds(bounds);
            morphPanel_.setBounds(bounds);
            filterLabPanel_.setBounds(bounds);
            dynamicsLabPanel_.setBounds(bounds);
            generativeLabPanel_.setBounds(bounds);
            utilityPeaksPanel_.setBounds(bounds);
            envelopeSegmentsPanel_.setBounds(bounds);
        }

        if (engineDetailOverlay_.isVisible())
            engineDetailOverlay_.setBounds(getLocalBounds());
        if (masterQuasarPanel_.isVisible())
            masterQuasarPanel_.setBounds(getLocalBounds());
    }

} // namespace pw8::plugin::ui
