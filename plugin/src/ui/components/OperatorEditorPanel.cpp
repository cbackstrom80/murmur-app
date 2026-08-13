#include "OperatorEditorPanel.h"

#include "AlgorithmGraphView.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
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

    OperatorEditorPanel::OperatorEditorPanel(PatchworkEightProcessor& processor,
                                             ModAssignmentController& assignmentController)
        : processor_(processor), assignmentController_(assignmentController), oscWireframeHost_(processor)
    {
        addAndMakeVisible(panel_);
        panel_.setInterceptsMouseClicks(false, true);
        panel_.addAndMakeVisible(oscWireframeHost_);
        showNode(0);
        startTimerHz(4);
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
        oscWireframeHost_.showNode(selectedNode_);
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
        panKnob_ = std::make_unique<GlowKnob>(apvts, juce::String(kLayerPanId), "Pan");
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

        // Engine 4 (Additive) -- see the header doc comment. No UI existed for
        // these four fields before this pass.
        additivePartialsKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditivePartialCount"), "Partials");
        additiveTiltKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditiveTilt"), "Tilt");
        additiveOddEvenKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditiveOddEven"), "Odd/Even");
        additiveStretchKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditiveStretch"), "Stretch");

        // Engine 8 (Resonator) -- see the header doc comment. No UI existed for
        // these five fields before this pass.
        resonatorStructureKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorStructure"), "Structure");
        resonatorDecayKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorDecay"), "Decay");
        resonatorDampingKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorDamping"), "Damping");
        resonatorBrightnessKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorBrightness"), "Brightness");
        resonatorModesKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorModeCount"), "Modes");

        // Engine 6 (Granular) -- see the header doc comment. No UI existed for
        // these four fields before this pass.
        grainDensityKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainDensity"), "Density");
        grainSizeKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainSizeMs"), "Size");
        grainPosJitterKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainPositionJitter"), "Pos Jit");
        grainPitchJitterKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainPitchJitter"), "Pitch Jit");
        wtBendKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "WtBend"), "WT Bend");
        wtAsymmetryKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "WtAsymmetry"), "WT Asym");

        for (auto* k : {waveformKnob_.get(), levelKnob_.get(), panKnob_.get(), ratioKnob_.get(), wavetablePosKnob_.get(),
                         fmModRatioKnob_.get(), fmModIndexKnob_.get(), fmModFeedbackKnob_.get(), fmModWaveformKnob_.get(),
                         noiseVariantKnob_.get(), noiseRateKnob_.get(),
                         phaseBendKnob_.get(), phaseFoldKnob_.get(), phaseAsymmetryKnob_.get(), phaseShapeKnob_.get(),
                         additivePartialsKnob_.get(), additiveTiltKnob_.get(), additiveOddEvenKnob_.get(),
                         additiveStretchKnob_.get(),
                         resonatorStructureKnob_.get(), resonatorDecayKnob_.get(), resonatorDampingKnob_.get(),
                         resonatorBrightnessKnob_.get(), resonatorModesKnob_.get(),
                         grainDensityKnob_.get(), grainSizeKnob_.get(), grainPosJitterKnob_.get(),
                         grainPitchJitterKnob_.get(), wtBendKnob_.get(), wtAsymmetryKnob_.get()})
            panel_.addAndMakeVisible(*k);
        wireModTargets();
    }

    void OperatorEditorPanel::wireModTargets()
    {
        const auto targetIndex = static_cast<std::uint8_t>(selectedNode_);
        if (levelKnob_ != nullptr)
        {
            levelKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorLevel, targetIndex);
            levelKnob_->setModAssignmentController(&assignmentController_);
        }
        if (wavetablePosKnob_ != nullptr)
        {
            wavetablePosKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorWavetablePosition,
                                                      targetIndex);
            wavetablePosKnob_->setModAssignmentController(&assignmentController_);
        }
        if (panKnob_ != nullptr)
        {
            panKnob_->enableModulationTarget(processor_, modulation::ModDestination::Pan, 0);
            panKnob_->setModAssignmentController(&assignmentController_);
        }
    }

    void OperatorEditorPanel::updateEngineVisibility()
    {
        const int engine = currentEngineOrdinal();
        lastKnownEngine_ = engine;
        const bool isWavetable = engine == static_cast<int>(algorithm::EngineType::Wavetable);
        const bool isFmPm = engine == static_cast<int>(algorithm::EngineType::FmPm);
        const bool isNoiseChaos = engine == static_cast<int>(algorithm::EngineType::NoiseChaos);
        const bool isPhaseShape = engine == static_cast<int>(algorithm::EngineType::PhaseShape);
        const bool isAdditive = engine == static_cast<int>(algorithm::EngineType::Additive);
        const bool isResonator = engine == static_cast<int>(algorithm::EngineType::Resonator);
        const bool isGranular = engine == static_cast<int>(algorithm::EngineType::Granular);
        // Granular grains read the same wavetableId-loaded data the Wavetable
        // engine uses (see op::OperatorPatch's Granular fields doc comment), so
        // it shares the stack preview + WT Pos (reused as grain base position)
        // rather than hiding them the way every other non-Wavetable engine does.
        const bool showsWavetableStack = isWavetable || isGranular;

        // Waveform (classic.waveform) only matters for engines that actually read
        // params.classic -- Classic itself, and FM/PM (it's the CARRIER's shape
        // there). Wavetable's, NoiseChaos's, PhaseShape's, Additive's,
        // Resonator's, and Granular's cases never touch params.classic, so it's
        // hidden only for those engines, swapped for the stack preview + WT Pos
        // (+ 4 grain knobs for Granular), the Noise/Rate pair, the 4 phase-shape
        // knobs, the 4 additive knobs, or the 5 resonator knobs respectively.
        //
        // Ratio (frequencyRatio), by contrast, is read GENERICALLY: render()
        // computes carrierHz from it once, before the engine switch, so it
        // affects every engine's pitch (or, for NoiseChaos, its
        // phaseMod/freqModHz bookkeeping even though its own output ignores
        // pitch; or, for Resonator, tuning each mode; or, for Granular, its
        // root-key-relative playback rate) including Wavetable -- always
        // visible. (Hiding it for Wavetable was a real GATE-5-era gap: that
        // operator's pitch ratio was a live, working, automatable parameter
        // with no way to reach it from the UI. Caught and fixed while touching
        // this exact function for FM/PM, not a deliberate part of that pass.)
        //
        // None of these knobs' parameter IDs depend on the engine (only on
        // selectedNode_), so an engine-only change never needs to reconstruct
        // them -- doing so used to silently abort any in-progress mouse drag on
        // a knob the instant a host automated the Engine parameter mid-gesture.
        waveformKnob_->setVisible(!showsWavetableStack && !isNoiseChaos && !isPhaseShape && !isAdditive && !isResonator);
        ratioKnob_->setVisible(true);
        wavetablePosKnob_->setVisible(showsWavetableStack);
        oscWireframeHost_.setVisible(true);
        oscWireframeHost_.setEngine(engineForIndex(engine));
        if (showsWavetableStack)
            oscWireframeHost_.ensureDefaultWavetableLoaded();
        noiseVariantKnob_->setVisible(isNoiseChaos);
        noiseRateKnob_->setVisible(isNoiseChaos);
        phaseBendKnob_->setVisible(isPhaseShape);
        phaseFoldKnob_->setVisible(isPhaseShape);
        phaseAsymmetryKnob_->setVisible(isPhaseShape);
        phaseShapeKnob_->setVisible(isPhaseShape);
        additivePartialsKnob_->setVisible(isAdditive);
        additiveTiltKnob_->setVisible(isAdditive);
        additiveOddEvenKnob_->setVisible(isAdditive);
        additiveStretchKnob_->setVisible(isAdditive);
        resonatorStructureKnob_->setVisible(isResonator);
        resonatorDecayKnob_->setVisible(isResonator);
        resonatorDampingKnob_->setVisible(isResonator);
        resonatorBrightnessKnob_->setVisible(isResonator);
        resonatorModesKnob_->setVisible(isResonator);
        grainDensityKnob_->setVisible(isGranular);
        grainSizeKnob_->setVisible(isGranular);
        grainPosJitterKnob_->setVisible(isGranular);
        grainPitchJitterKnob_->setVisible(isGranular);
        wtBendKnob_->setVisible(isWavetable);
        wtAsymmetryKnob_->setVisible(isWavetable);
        levelKnob_->setVisible(true);
        panKnob_->setVisible(true);

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
        // Keep patch.engine aligned with the pill (wavetable arrows reload via loadPatch).
        processor_.syncCurrentPatchFromApvts();
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

        const bool isGranular = grainDensityKnob_ && grainDensityKnob_->isVisible();
        const float wireframeFrac = isGranular ? 0.40f : 0.45f;
        auto wireframeArea = content.removeFromLeft(static_cast<int>(static_cast<float>(content.getWidth()) * wireframeFrac));
        oscWireframeHost_.setBounds(wireframeArea.reduced(5));

        if (isGranular)
        {
            const int knobWidth = content.getWidth() / 8;
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (wavetablePosKnob_)
                wavetablePosKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (grainDensityKnob_)
                grainDensityKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (grainSizeKnob_)
                grainSizeKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (grainPosJitterKnob_)
                grainPosJitterKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (grainPitchJitterKnob_)
                grainPitchJitterKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (fmModRatioKnob_ && fmModRatioKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 8;
            for (auto* k : {waveformKnob_.get(), levelKnob_.get(), panKnob_.get(), ratioKnob_.get(), fmModRatioKnob_.get(),
                             fmModIndexKnob_.get(), fmModFeedbackKnob_.get(), fmModWaveformKnob_.get()})
                if (k != nullptr)
                    k->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (noiseVariantKnob_ && noiseVariantKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 5;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (noiseVariantKnob_)
                noiseVariantKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (noiseRateKnob_)
                noiseRateKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (phaseBendKnob_ && phaseBendKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 7;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (phaseBendKnob_)
                phaseBendKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (phaseFoldKnob_)
                phaseFoldKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (phaseAsymmetryKnob_)
                phaseAsymmetryKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (phaseShapeKnob_)
                phaseShapeKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (additivePartialsKnob_ && additivePartialsKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 7;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (additivePartialsKnob_)
                additivePartialsKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (additiveTiltKnob_)
                additiveTiltKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (additiveOddEvenKnob_)
                additiveOddEvenKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (additiveStretchKnob_)
                additiveStretchKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (resonatorStructureKnob_ && resonatorStructureKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 8;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (resonatorStructureKnob_)
                resonatorStructureKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (resonatorDecayKnob_)
                resonatorDecayKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (resonatorDampingKnob_)
                resonatorDampingKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (resonatorBrightnessKnob_)
                resonatorBrightnessKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (resonatorModesKnob_)
                resonatorModesKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (wtBendKnob_ && wtBendKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 6;
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (wavetablePosKnob_)
                wavetablePosKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (wtBendKnob_)
                wtBendKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (wtAsymmetryKnob_)
                wtAsymmetryKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (wavetablePosKnob_ && wavetablePosKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 4;
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (wavetablePosKnob_)
                wavetablePosKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else
        {
            const int knobWidth = content.getWidth() / 4;
            if (waveformKnob_)
                waveformKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
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
