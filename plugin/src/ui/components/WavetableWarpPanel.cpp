#include "WavetableWarpPanel.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNodeRowHeight = 34;
        constexpr int kHintHeight = 18;
        constexpr int kKnobRowHeight = 92;
    } // namespace

    WavetableWarpPanel::WavetableWarpPanel(PatchworkEightProcessor& processor)
        : processor_(processor), nodeSelector_(processor), stackView_(processor)
    {
        nodeSelector_.onNodeSelected = [this](int nodeIndex) { showNode(nodeIndex); };
        nodeSelector_.setGlobalPillVisible(false);
        addAndMakeVisible(nodeSelector_);

        addAndMakeVisible(meshFrame_);
        meshFrame_.addAndMakeVisible(stackView_);

        engineHint_.setFont(fonts::value(11.0f));
        engineHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        engineHint_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(engineHint_);

        showNode(0);
        startTimerHz(4);
    }

    int WavetableWarpPanel::currentEngineOrdinal() const noexcept
    {
        const auto engineParamId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
        if (auto* raw = processor_.apvts.getRawParameterValue(engineParamId))
            return static_cast<int>(raw->load() + 0.5f);
        return static_cast<int>(algorithm::EngineType::Wavetable);
    }

    void WavetableWarpPanel::showNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, static_cast<int>(pw8::core::kNodesPerLayer) - 1, nodeIndex);
        nodeSelector_.setSelectedNode(selectedNode_);
        stackView_.showNode(selectedNode_);
        rebuildKnobsForNode();
        updateEngineVisibility();
    }

    void WavetableWarpPanel::rebuildKnobsForNode()
    {
        wavetablePosKnob_.reset();
        wtWarpKnob_.reset();
        wtSyncRatioKnob_.reset();
        wtSyncAmountKnob_.reset();
        wtFormantKnob_.reset();

        auto& apvts = processor_.apvts;
        const auto op = static_cast<std::size_t>(selectedNode_);

        wavetablePosKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WavetablePos"), "WT Pos");
        wtWarpKnob_ = std::make_unique<ConcentricGlowKnob>(apvts, operatorParamId(op, "WtAsymmetry"),
                                                           operatorParamId(op, "WtBend"), "WT Asym", "WT Bend");
        wtWarpKnob_->setInnerAccentColour(palette::kMurmurViolet);
        wtSyncRatioKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtSyncRatio"), "Sync Ratio");
        wtSyncAmountKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtSyncAmount"), "Sync Amt");
        wtFormantKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtFormantShift"), "Formant");

        for (auto* k : {wavetablePosKnob_.get(), wtSyncRatioKnob_.get(), wtSyncAmountKnob_.get(), wtFormantKnob_.get()})
        {
            addAndMakeVisible(*k);
            if (k != nullptr)
                k->setDeckedStyle(true, GlowKnob::DeckedKnobSize::Medium);
        }
        if (wtWarpKnob_ != nullptr)
            addAndMakeVisible(*wtWarpKnob_);

        wireModTargets();
    }

    void WavetableWarpPanel::wireModTargets()
    {
        const auto targetIndex = static_cast<std::uint8_t>(selectedNode_);
        if (wavetablePosKnob_ != nullptr)
        {
            wavetablePosKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorWavetablePosition,
                                                      targetIndex);
            wavetablePosKnob_->setModAssignmentController(&assignmentController_);
        }
        if (wtWarpKnob_ != nullptr)
        {
            wtWarpKnob_->enableInnerModulationTarget(processor_, modulation::ModDestination::OperatorWavetableAsymmetry,
                                                     targetIndex);
            wtWarpKnob_->enableOuterModulationTarget(processor_, modulation::ModDestination::OperatorWavetableBend,
                                                     targetIndex);
            wtWarpKnob_->setModAssignmentController(&assignmentController_);
        }
        if (wtSyncRatioKnob_ != nullptr)
        {
            wtSyncRatioKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorWavetableSyncRatio,
                                                     targetIndex);
            wtSyncRatioKnob_->setModAssignmentController(&assignmentController_);
        }
        if (wtSyncAmountKnob_ != nullptr)
        {
            wtSyncAmountKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorWavetableSyncAmount,
                                                       targetIndex);
            wtSyncAmountKnob_->setModAssignmentController(&assignmentController_);
        }
        if (wtFormantKnob_ != nullptr)
        {
            wtFormantKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorWavetableFormant,
                                                   targetIndex);
            wtFormantKnob_->setModAssignmentController(&assignmentController_);
        }
    }

    void WavetableWarpPanel::updateEngineVisibility()
    {
        const int engine = currentEngineOrdinal();
        lastKnownEngine_ = engine;
        const bool isWavetable = engine == static_cast<int>(algorithm::EngineType::Wavetable);
        const bool isGranular = engine == static_cast<int>(algorithm::EngineType::Granular);
        const bool showsStack = isWavetable || isGranular;

        stackView_.setVisible(showsStack);
        meshFrame_.setVisible(showsStack);
        stackView_.setGranularOverlay(isGranular);
        if (showsStack)
            stackView_.ensureDefaultWavetableLoaded();

        const auto setKnobVisible = [&](juce::Component* k) {
            if (k != nullptr)
                k->setVisible(isWavetable);
        };
        setKnobVisible(wavetablePosKnob_.get());
        setKnobVisible(wtWarpKnob_.get());
        setKnobVisible(wtSyncRatioKnob_.get());
        setKnobVisible(wtSyncAmountKnob_.get());
        setKnobVisible(wtFormantKnob_.get());

        if (isWavetable)
            engineHint_.setText("Warp preview matches audio — adjust Sync Ratio here (PLAY has Sync Amt only).",
                                juce::dontSendNotification);
        else if (isGranular)
            engineHint_.setText("Granular uses the wavetable buffer — warp knobs apply when engine is Wavetable.",
                                juce::dontSendNotification);
        else
            engineHint_.setText("Select a Wavetable engine operator to edit warps.", juce::dontSendNotification);

        resized();
    }

    void WavetableWarpPanel::refreshFromPatch()
    {
        nodeSelector_.repaint();
        stackView_.showNode(selectedNode_);
        updateEngineVisibility();
    }

    void WavetableWarpPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(4);

        nodeSelector_.setBounds(bounds.removeFromTop(kNodeRowHeight));
        bounds.removeFromTop(4);

        engineHint_.setBounds(bounds.removeFromTop(kHintHeight));
        bounds.removeFromTop(4);

        auto knobRow = bounds.removeFromBottom(kKnobRowHeight);
        const int knobCount = 5;
        const int knobWidth = knobRow.getWidth() / knobCount;
        const auto placeKnob = [&](juce::Component* k) {
            if (k != nullptr && k->isVisible())
                k->setBounds(knobRow.removeFromLeft(knobWidth).reduced(4));
        };
        placeKnob(wavetablePosKnob_.get());
        placeKnob(wtWarpKnob_.get());
        placeKnob(wtSyncRatioKnob_.get());
        placeKnob(wtSyncAmountKnob_.get());
        placeKnob(wtFormantKnob_.get());

        meshFrame_.setBounds(bounds);
        stackView_.setBounds(meshFrame_.getContentBounds());
    }

    void WavetableWarpPanel::timerCallback()
    {
        const int engine = currentEngineOrdinal();
        if (engine != lastKnownEngine_)
            updateEngineVisibility();
    }

} // namespace pw8::plugin::ui
