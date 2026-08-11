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

        juce::String noiseVariantToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "WHITE";
                case 1: return "PINK";
                case 2: return "BROWN";
                case 3: return "BLUE";
                case 4: return "S&H";
                case 5: return "SMOOTH";
                case 6: return "DUST";
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
        // Engine 7 (NoiseChaos) -- see the header doc comment. No UI existed for
        // these two fields before this pass.
        noiseVariantKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "NoiseVariant"), "Noise",
            noiseVariantToText);
        noiseRateKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "NoiseRate"), "Rate");

        // Engine Type 3 (FM/PM) only -- the self-contained internal modulator's
        // controls. Same "always constructed, visibility toggled by
        // updateEngineVisibility()" pattern as wavetablePosKnob_ above.
        fmModRatioKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorRatio"), "Mod Ratio");
        fmModIndexKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorIndex"), "Mod Index");
        fmModFeedbackKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorFeedback"), "Mod Fdbk");
        fmModWaveformKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorWaveform"), "Mod Wave",
            waveformToText);

        for (auto* k : {waveformKnob_.get(), levelKnob_.get(), ratioKnob_.get(), wavetablePosKnob_.get(),
                         fmModRatioKnob_.get(), fmModIndexKnob_.get(), fmModFeedbackKnob_.get(), fmModWaveformKnob_.get(),
                         noiseVariantKnob_.get(), noiseRateKnob_.get()})
            panel_.addAndMakeVisible(*k);
    }

    void OperatorEditorPanel::updateEngineVisibility()
    {
        const int engine = currentEngineOrdinal();
        lastKnownEngine_ = engine;
        const bool isWavetable = engine == static_cast<int>(algorithm::EngineType::Wavetable);
        const bool isFmPm = engine == static_cast<int>(algorithm::EngineType::FmPm);
        const bool isNoiseChaos = engine == static_cast<int>(algorithm::EngineType::NoiseChaos);

        // Waveform (classic.waveform) only matters for engines that actually read
        // params.classic -- Classic itself, and FM/PM (it's the CARRIER's shape
        // there). Wavetable's and NoiseChaos's cases never touch params.classic,
        // so it's hidden only for those two engines, swapped for the stack
        // preview + WT Pos or the Noise/Rate pair respectively.
        //
        // Ratio (frequencyRatio), by contrast, is read GENERICALLY: render()
        // computes carrierHz from it once, before the engine switch, so it
        // affects every engine's pitch (or, for NoiseChaos, its
        // phaseMod/freqModHz bookkeeping even though its own output ignores
        // pitch) including Wavetable -- always visible. (Hiding it for
        // Wavetable was a real GATE-5-era gap: that operator's pitch ratio was
        // a live, working, automatable parameter with no way to reach it from
        // the UI. Caught and fixed while touching this exact function for
        // FM/PM, not a deliberate part of this pass.)
        //
        // None of these knobs' parameter IDs depend on the engine (only on
        // selectedNode_), so an engine-only change never needs to reconstruct
        // them -- doing so used to silently abort any in-progress mouse drag on
        // a knob the instant a host automated the Engine parameter mid-gesture.
        waveformKnob_->setVisible(!isWavetable && !isNoiseChaos);
        ratioKnob_->setVisible(true);
        wavetablePosKnob_->setVisible(isWavetable);
        wavetableStackView_.setVisible(isWavetable);
        noiseVariantKnob_->setVisible(isNoiseChaos);
        noiseRateKnob_->setVisible(isNoiseChaos);
        levelKnob_->setVisible(true);

        for (auto* k : {fmModRatioKnob_.get(), fmModIndexKnob_.get(), fmModFeedbackKnob_.get(), fmModWaveformKnob_.get()})
            k->setVisible(isFmPm);

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
            // 0.55, not GATE 5's original 0.62 -- ratioKnob_ joining this row
            // (see updateEngineVisibility()'s doc comment on why it's always
            // visible now) means 3 knobs share the remaining width instead of 2.
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
        else if (fmModRatioKnob_ && fmModRatioKnob_->isVisible())
        {
            // FM/PM: carrier's own Wave/Level/Ratio plus the internal
            // modulator's 4 knobs -- 7 across, the same row-width budget
            // FxChainStrip's 7 FX slots already prove works at this panel width.
            const int knobWidth = content.getWidth() / 7;
            for (auto* k : {waveformKnob_.get(), levelKnob_.get(), ratioKnob_.get(), fmModRatioKnob_.get(),
                             fmModIndexKnob_.get(), fmModFeedbackKnob_.get(), fmModWaveformKnob_.get()})
                if (k != nullptr)
                    k->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        }
        else if (noiseVariantKnob_ && noiseVariantKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 4;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (noiseVariantKnob_)
                noiseVariantKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            if (noiseRateKnob_)
                noiseRateKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
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
