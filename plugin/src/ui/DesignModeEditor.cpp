#include "DesignModeEditor.h"

#include "PlayModeLayout.h"
#include "state/PluginState.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    DesignModeEditor::DesignModeEditor(PatchworkEightProcessor& processor, SharedEditorChrome& chrome)
        : engineGridPanel_(processor),
          statusBar_(processor),
          engineDetailOverlay_(processor, modAssignmentController_),
          arpPanel_(processor),
          vocoderPanel_(processor),
          fxPanel_(processor, modAssignmentController_),
          modMatrixPanel_(processor, modAssignmentController_),
          wavetablePanel_(processor),
          dualLfoPanel_(processor)
    {
        juce::ignoreUnused(chrome);
        setLookAndFeel(&lookAndFeel_);

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

        enginePage_.addAndMakeVisible(engineGridPanel_);
        enginePage_.addAndMakeVisible(statusBar_);
        addAndMakeVisible(enginePage_);

        addChildComponent(arpPanel_);
        addChildComponent(vocoderPanel_);
        addChildComponent(fxPanel_);
        addChildComponent(modMatrixPanel_);
        addChildComponent(wavetablePanel_);
        addChildComponent(dualLfoPanel_);

        arpPanel_.setEmbeddedInDesignMode(true);
        vocoderPanel_.setEmbeddedInDesignMode(true);
        fxPanel_.setEmbeddedInDesignMode(true);
        modMatrixPanel_.setEmbeddedInDesignMode(true);
        wavetablePanel_.setEmbeddedInDesignMode(true);
        dualLfoPanel_.setEmbeddedInDesignMode(true);

        arpPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        vocoderPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        vocoderPanel_.onOpenFxChain = [this] { setDesignSubPage(layout::DesignSubPage::Fx); };
        fxPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        modMatrixPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        wavetablePanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        dualLfoPanel_.onClosed = [this] { setDesignSubPage(layout::DesignSubPage::Engine); };
        dualLfoPanel_.onOpenModMatrix = [this] { setDesignSubPage(layout::DesignSubPage::ModMatrix); };

        fxPanel_.onVocoderLabRequested = [this](std::size_t slotIndex) {
            juce::ignoreUnused(slotIndex);
            setDesignSubPage(layout::DesignSubPage::Vocoder);
        };

        addChildComponent(engineDetailOverlay_);
        engineDetailOverlay_.onClosed = [this] { closeEngineDetail(); };

        applySubPageVisibility();
    }

    DesignModeEditor::~DesignModeEditor() { setLookAndFeel(nullptr); }

    void DesignModeEditor::setDesignSubPage(layout::DesignSubPage page)
    {
        if (designSubPage_ == page)
            return;

        closeEngineDetail();
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
            engineGridPanel_.setBounds(engineBounds.removeFromTop(layout::kDesignModeV2GridSectionHeight));
        }
        else
        {
            arpPanel_.setBounds(bounds);
            vocoderPanel_.setBounds(bounds);
            fxPanel_.setBounds(bounds);
            modMatrixPanel_.setBounds(bounds);
            wavetablePanel_.setBounds(bounds);
            dualLfoPanel_.setBounds(bounds);
        }

        if (engineDetailOverlay_.isVisible())
            engineDetailOverlay_.setBounds(getLocalBounds());
    }

} // namespace pw8::plugin::ui
