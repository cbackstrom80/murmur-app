#include "OperatorEditorPanel.h"

#include "AlgorithmGraphView.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNumEngines = 8;
        constexpr int kPillRowHeight = 30;
        constexpr int kNoteHeight = 16;

        juce::String waveformToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "SINE";
                case 1: return "TRIANGLE";
                case 2: return "SAW";
                case 3: return "SQUARE";
                default: return juce::String(v);
            }
        }

        algorithm::EngineType engineForIndex(int index) noexcept
        {
            return static_cast<algorithm::EngineType>(juce::jlimit(0, kNumEngines - 1, index));
        }
    } // namespace

    OperatorEditorPanel::OperatorEditorPanel(PatchworkEightProcessor& processor)
        : processor_(processor), wavetableStackView_(processor)
    {
        addAndMakeVisible(panel_);
        // panel_ is a plain paint()-only shell with no interactivity of its own
        // (SectionPanel's own doc comment) -- without this, its full-bounds hit
        // region would swallow every click in the pill row before this
        // component's own mouseDown() ever saw it. `true` for the children
        // parameter keeps the knobs (panel_'s own children) fully clickable.
        panel_.setInterceptsMouseClicks(false, true);
        // wavetableStackView_ holds no APVTS attachment (unlike the GlowKnobs
        // below), so it's a plain always-alive member, added once here rather
        // than reconstructed per showNode() -- showNode() just re-points it at
        // the newly-selected node via its own showNode().
        panel_.addAndMakeVisible(wavetableStackView_);
        showNode(0);
        startTimerHz(4); // Matches WavetableStackView's own poll rate -- see header doc comment.
    }

    OperatorEditorPanel::~OperatorEditorPanel()
    {
        stopTimer();
    }

    juce::Rectangle<int> OperatorEditorPanel::pillRowBounds() const
    {
        auto content = panel_.getContentBounds();
        content.removeFromBottom(kNoteHeight);
        return content.removeFromTop(kPillRowHeight);
    }

    juce::Rectangle<int> OperatorEditorPanel::pillBounds(int engineIndex) const
    {
        auto row = pillRowBounds();
        const int pillWidth = row.getWidth() / kNumEngines;
        return row.withX(row.getX() + pillWidth * engineIndex).withWidth(pillWidth).reduced(2, 3);
    }

    int OperatorEditorPanel::currentEngineOrdinal() const noexcept
    {
        const auto engineParamId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
        if (auto* raw = processor_.apvts.getRawParameterValue(engineParamId))
            return static_cast<int>(raw->load());
        return 0;
    }

    void OperatorEditorPanel::showNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, static_cast<int>(pw8::core::kNodesPerLayer) - 1, nodeIndex);
        panel_.setTitle("Operator " + juce::String(selectedNode_));
        wavetableStackView_.showNode(selectedNode_);
        // Every knob's APVTS attachment needs to be re-pointed at the new node's
        // parameter IDs, so this always rebuilds regardless of whether the new
        // node happens to share the old one's engine ordinal.
        rebuildKnobsForNode();
        updateEngineVisibility();
    }

    void OperatorEditorPanel::rebuildKnobsForNode()
    {
        auto& apvts = processor_.apvts;
        waveformKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "Waveform"), "Wave", waveformToText);
        levelKnob_ =
            std::make_unique<GlowKnob>(apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "Level"), "Level");
        ratioKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FreqRatio"),
                                                  "Ratio");
        // Previously had no UI anywhere -- PluginState.h's "WavetablePos" field is
        // a real automatable parameter, but nothing in PLAY mode exposed a knob
        // for it until this pass (UI GATE 5).
        wavetablePosKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "WavetablePos"), "WT Pos");
        // Engine 5 (PhaseShape) -- see the header doc comment. No UI existed for
        // these four fields before this pass.
        phaseBendKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseBend"), "Bend");
        phaseFoldKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseFold"), "Fold");
        phaseAsymmetryKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseAsymmetry"), "Asym");
        phaseShapeKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseShape"), "Shape");

        for (auto* k : {waveformKnob_.get(), levelKnob_.get(), ratioKnob_.get(), wavetablePosKnob_.get(),
                         phaseBendKnob_.get(), phaseFoldKnob_.get(), phaseAsymmetryKnob_.get(), phaseShapeKnob_.get()})
            panel_.addAndMakeVisible(*k);
    }

    void OperatorEditorPanel::updateEngineVisibility()
    {
        const int engine = currentEngineOrdinal();
        lastKnownEngine_ = engine;
        const bool isWavetable = engine == static_cast<int>(algorithm::EngineType::Wavetable);
        const bool isPhaseShape = engine == static_cast<int>(algorithm::EngineType::PhaseShape);

        // Wave isn't meaningful for a Wavetable- or PhaseShape-engine node (neither
        // reads classicWaveform) -- swapped for the stack preview+WT Pos or the
        // 4 phase-shape knobs instead. Ratio stays visible for every engine
        // (including PhaseShape -- carrierHz still comes from the generic
        // keyTrack/frequencyRatio computation in render() even though PhaseShape
        // reads its own oscillator; hiding it would repeat the same mistake the
        // Wavetable branch used to make, see the ratioKnob_ comment below). Level
        // applies to every engine. None of these knobs' parameter IDs depend on
        // the engine (only on selectedNode_), so an engine-only change never needs
        // to reconstruct them -- doing so used to silently abort any in-progress
        // mouse drag on a knob the instant a host automated the Engine parameter
        // mid-gesture.
        waveformKnob_->setVisible(!isWavetable && !isPhaseShape);
        // frequencyRatio is read generically by render() before the engine switch
        // for every engine's carrier pitch computation -- always shown, not gated
        // per-engine (a prior pass hid it for Wavetable, making a real working,
        // automatable parameter unreachable from the UI; fixed to always show it).
        ratioKnob_->setVisible(true);
        wavetablePosKnob_->setVisible(isWavetable);
        wavetableStackView_.setVisible(isWavetable);
        phaseBendKnob_->setVisible(isPhaseShape);
        phaseFoldKnob_->setVisible(isPhaseShape);
        phaseAsymmetryKnob_->setVisible(isPhaseShape);
        phaseShapeKnob_->setVisible(isPhaseShape);
        levelKnob_->setVisible(true);

        resized();
        repaint();
    }

    void OperatorEditorPanel::timerCallback()
    {
        // Catches an engine change that happens WITHOUT a node reselection --
        // clicking a different pill on the currently-selected node (mouseDown()
        // below already handles that case immediately, so this mostly matters for
        // a host-driven change: automation, or loading a different patch/session
        // while this node stays selected).
        if (currentEngineOrdinal() != lastKnownEngine_)
            updateEngineVisibility();
    }

    int OperatorEditorPanel::pillIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < kNumEngines; ++i)
            if (pillBounds(i).contains(pos))
                return i;
        return -1;
    }

    juce::String OperatorEditorPanel::getTooltip()
    {
        const int i = pillIndexAt(getMouseXYRelative());
        if (i < 0)
            return {};
        const auto engine = engineForIndex(i);
        if (algorithm::isEngineImplemented(engine))
            return {}; // No tooltip needed over an already-usable pill.
        return juce::String(engineShortName(engine)) + " renders silence today -- not yet implemented.";
    }

    void OperatorEditorPanel::mouseDown(const juce::MouseEvent& event)
    {
        const int i = pillIndexAt(event.getPosition());
        if (i < 0)
            return;

        const auto engine = engineForIndex(i);
        if (!algorithm::isEngineImplemented(engine))
            return; // Honest no-op -- see the header doc comment on why this stays disabled, not hidden.

        auto& apvts = processor_.apvts;
        const auto paramId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
        if (auto* param = apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
        // setValueNotifyingHost() updates the underlying value synchronously
        // before notifying listeners, so currentEngineOrdinal() below already
        // sees the new value -- no need to wait for the next timerCallback().
        updateEngineVisibility();
    }

    void OperatorEditorPanel::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds();
        content.removeFromTop(kPillRowHeight);
        content.removeFromBottom(kNoteHeight);

        if (wavetableStackView_.isVisible())
        {
            // Narrower than the other branches' full-width knob row (0.55 vs the
            // stack getting the rest) -- ratioKnob_ joining this row (now shown for
            // every engine, see updateEngineVisibility()) needs a 3-way split here
            // too, not just 2.
            auto stackArea = content.removeFromLeft(static_cast<int>(static_cast<float>(content.getWidth()) * 0.55f));
            wavetableStackView_.setBounds(stackArea.reduced(3));

            const int knobWidth = content.getWidth() / 3;
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (wavetablePosKnob_)
                wavetablePosKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        }
        else if (phaseBendKnob_ && phaseBendKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 6;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (phaseBendKnob_)
                phaseBendKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (phaseFoldKnob_)
                phaseFoldKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (phaseAsymmetryKnob_)
                phaseAsymmetryKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (phaseShapeKnob_)
                phaseShapeKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        }
        else
        {
            const int knobWidth = content.getWidth() / 3;
            if (waveformKnob_)
                waveformKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        }
    }

    void OperatorEditorPanel::paintOverChildren(juce::Graphics& g)
    {
        const int currentEngine = currentEngineOrdinal();

        for (int i = 0; i < kNumEngines; ++i)
        {
            const auto bounds = pillBounds(i).toFloat();
            const auto engine = engineForIndex(i);
            const bool implemented = algorithm::isEngineImplemented(engine);
            const bool isCurrent = i == currentEngine;

            g.setColour(isCurrent ? palette::kAccentDim : palette::kPanelRaised);
            g.fillRoundedRectangle(bounds, 5.0f);
            g.setColour(isCurrent ? palette::kAccent : palette::kBorder);
            g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);

            g.setColour(!implemented ? palette::kTextDim : (isCurrent ? palette::kTextPrimary : palette::kTextSecondary));
            g.setFont(fonts::label(9.5f));
            g.drawText(engineShortName(engine), bounds, juce::Justification::centred);
        }

        // A reserved note line under the knobs -- only filled in when the
        // currently-selected operator is running an unimplemented engine, so a
        // player who picks (or inherits, from a hand-authored .pw8) one of those 6
        // engines is told plainly why it's silent rather than left to guess.
        if (!algorithm::isEngineImplemented(engineForIndex(currentEngine)))
        {
            auto content = panel_.getContentBounds();
            auto noteRow = content.removeFromBottom(kNoteHeight).toFloat();
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(9.5f));
            g.drawText("This engine renders silence today -- not yet implemented.", noteRow,
                       juce::Justification::centredLeft);
        }
    }

} // namespace pw8::plugin::ui
