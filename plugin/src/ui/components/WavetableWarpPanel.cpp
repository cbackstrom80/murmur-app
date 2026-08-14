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
        addAndMakeVisible(nodeSelector_);

        addAndMakeVisible(stackView_);

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
        wtBendKnob_.reset();
        wtAsymmetryKnob_.reset();
        wtSyncRatioKnob_.reset();
        wtSyncAmountKnob_.reset();
        wtFormantKnob_.reset();

        auto& apvts = processor_.apvts;
        const auto op = static_cast<std::size_t>(selectedNode_);

        wavetablePosKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WavetablePos"), "WT Pos");
        wtBendKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtBend"), "WT Bend");
        wtAsymmetryKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtAsymmetry"), "WT Asym");
        wtSyncRatioKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtSyncRatio"), "Sync Ratio");
        wtSyncAmountKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtSyncAmount"), "Sync Amt");
        wtFormantKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(op, "WtFormantShift"), "Formant");

        for (auto* k : {wavetablePosKnob_.get(), wtBendKnob_.get(), wtAsymmetryKnob_.get(), wtSyncRatioKnob_.get(),
                        wtSyncAmountKnob_.get(), wtFormantKnob_.get()})
            addAndMakeVisible(*k);

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
        if (wtBendKnob_ != nullptr)
        {
            wtBendKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorWavetableBend,
                                                targetIndex);
            wtBendKnob_->setModAssignmentController(&assignmentController_);
        }
        if (wtAsymmetryKnob_ != nullptr)
        {
            wtAsymmetryKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorWavetableAsymmetry,
                                                     targetIndex);
            wtAsymmetryKnob_->setModAssignmentController(&assignmentController_);
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
        stackView_.setGranularOverlay(isGranular);
        if (showsStack)
            stackView_.ensureDefaultWavetableLoaded();

        for (auto* k : {wavetablePosKnob_.get(), wtBendKnob_.get(), wtAsymmetryKnob_.get(), wtSyncRatioKnob_.get(),
                        wtSyncAmountKnob_.get(), wtFormantKnob_.get()})
        {
            if (k != nullptr)
                k->setVisible(isWavetable);
        }

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
        const int knobCount = 6;
        const int knobWidth = knobRow.getWidth() / knobCount;
        for (auto* k : {wavetablePosKnob_.get(), wtBendKnob_.get(), wtAsymmetryKnob_.get(), wtSyncRatioKnob_.get(),
                        wtSyncAmountKnob_.get(), wtFormantKnob_.get()})
        {
            if (k != nullptr && k->isVisible())
                k->setBounds(knobRow.removeFromLeft(knobWidth).reduced(4));
        }

        stackView_.setBounds(bounds);
    }

    void WavetableWarpPanel::timerCallback()
    {
        const int engine = currentEngineOrdinal();
        if (engine != lastKnownEngine_)
            updateEngineVisibility();
    }

} // namespace pw8::plugin::ui
