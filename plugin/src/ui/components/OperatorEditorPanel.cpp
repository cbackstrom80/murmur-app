#include "OperatorEditorPanel.h"

#include "AlgorithmGraphView.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNumBuiltinEngines = 8;
        constexpr int kNumEnginesWithExt = 9;
        constexpr int kPillRowHeight = 30;
        constexpr int kNoteHeight = 16;

        int numEnginePillsForNode(int nodeIndex) noexcept
        {
            return nodeIndex == 0 ? kNumEnginesWithExt : kNumBuiltinEngines;
        }

        algorithm::EngineType engineForPillIndex(int nodeIndex, int pillIndex) noexcept
        {
            const int maxOrdinal = nodeIndex == 0 ? static_cast<int>(algorithm::EngineType::External)
                                                : static_cast<int>(algorithm::EngineType::Resonator);
            return static_cast<algorithm::EngineType>(juce::jlimit(0, maxOrdinal, pillIndex));
        }

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
            return engineForPillIndex(0, index);
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
        const int pillWidth = row.getWidth() / numEnginePillsForNode(selectedNode_);
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
        fmModTripleKnob_ = std::make_unique<TripleGlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorRatio"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorIndex"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorFeedback"), "RATIO", "INDEX",
            "FDBK");
        fmModTripleKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        fmModWaveformKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FmModulatorWaveform"), "Mod Wave",
            waveformToText);

        // Engine 5 (PhaseShape) -- see the header doc comment. No UI existed for
        // these four fields before this pass.
        phaseTripleKnob_ = std::make_unique<TripleGlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseBend"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseFold"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseAsymmetry"), "BEND", "FOLD", "ASYM");
        phaseTripleKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        phaseShapeKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "PhaseShape"), "Shape");

        additiveTripleKnob_ = std::make_unique<TripleGlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditivePartialCount"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditiveTilt"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditiveOddEven"), "PART", "TILT", "ODD");
        additiveTripleKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        additiveStretchKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "AdditiveStretch"), "Stretch");

        resonatorTripleKnob_ = std::make_unique<TripleGlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorStructure"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorDecay"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorDamping"), "STR", "DEC", "DAMP");
        resonatorTripleKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        resonatorBrightnessKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorBrightness"), "Bright");
        resonatorModesKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "ResonatorModeCount"), "Modes");

        grainTripleKnob_ = std::make_unique<TripleGlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainDensity"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainSizeMs"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainPositionJitter"), "DENS", "SIZE", "POS");
        grainTripleKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        grainPitchJitterKnob_ = std::make_unique<GlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainPitchJitter"), "Pitch Jit");
        wtWarpKnob_ = std::make_unique<ConcentricGlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "WtAsymmetry"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "WtBend"), "WT Asym", "WT Bend");
        wtWarpKnob_->setInnerAccentColour(palette::kMurmurViolet);
        wtSyncFormantKnob_ = std::make_unique<ConcentricGlowKnob>(
            apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "WtFormantShift"),
            operatorParamId(static_cast<std::size_t>(selectedNode_), "WtSyncAmount"), "Formant", "Sync Amt");
        wtSyncFormantKnob_->setInnerAccentColour(palette::kAccentWarm);

        for (auto* k : {waveformKnob_.get(), levelKnob_.get(), panKnob_.get(), ratioKnob_.get(), wavetablePosKnob_.get(),
                         fmModWaveformKnob_.get(), noiseVariantKnob_.get(), noiseRateKnob_.get(), phaseShapeKnob_.get(),
                         additiveStretchKnob_.get(), resonatorBrightnessKnob_.get(), resonatorModesKnob_.get(),
                         grainPitchJitterKnob_.get()})
        {
            panel_.addAndMakeVisible(*k);
            if (k != nullptr)
            {
                k->setDeckedStyle(true, GlowKnob::DeckedKnobSize::Medium);
                k->applyFigmaContext(figma::KnobContext::OperatorHero);
            }
        }
        if (wtWarpKnob_ != nullptr)
        {
            wtWarpKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
            panel_.addAndMakeVisible(*wtWarpKnob_);
        }
        if (wtSyncFormantKnob_ != nullptr)
        {
            wtSyncFormantKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
            panel_.addAndMakeVisible(*wtSyncFormantKnob_);
        }
        if (fmModTripleKnob_ != nullptr)
            panel_.addAndMakeVisible(*fmModTripleKnob_);
        for (auto* triple :
             {phaseTripleKnob_.get(), additiveTripleKnob_.get(), resonatorTripleKnob_.get(), grainTripleKnob_.get()})
            if (triple != nullptr)
                panel_.addAndMakeVisible(*triple);
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
        if (wtWarpKnob_ != nullptr)
        {
            wtWarpKnob_->enableInnerModulationTarget(processor_, modulation::ModDestination::OperatorWavetableAsymmetry,
                                                     targetIndex);
            wtWarpKnob_->enableOuterModulationTarget(processor_, modulation::ModDestination::OperatorWavetableBend,
                                                     targetIndex);
            wtWarpKnob_->setModAssignmentController(&assignmentController_);
        }
        if (wtSyncFormantKnob_ != nullptr)
        {
            wtSyncFormantKnob_->enableInnerModulationTarget(processor_,
                                                            modulation::ModDestination::OperatorWavetableFormant,
                                                            targetIndex);
            wtSyncFormantKnob_->enableOuterModulationTarget(processor_,
                                                            modulation::ModDestination::OperatorWavetableSyncAmount,
                                                            targetIndex);
            wtSyncFormantKnob_->setModAssignmentController(&assignmentController_);
        }
        if (panKnob_ != nullptr)
        {
            panKnob_->enableModulationTarget(processor_, modulation::ModDestination::Pan, 0);
            panKnob_->setModAssignmentController(&assignmentController_);
        }
        if (ratioKnob_ != nullptr)
        {
            ratioKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorFreqRatio, targetIndex);
            ratioKnob_->setModAssignmentController(&assignmentController_);
        }
        if (additiveStretchKnob_ != nullptr)
        {
            additiveStretchKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorAdditiveStretch,
                                                          targetIndex);
            additiveStretchKnob_->setModAssignmentController(&assignmentController_);
        }
        if (resonatorBrightnessKnob_ != nullptr)
        {
            resonatorBrightnessKnob_->enableModulationTarget(processor_,
                                                             modulation::ModDestination::OperatorResonatorBrightness,
                                                             targetIndex);
            resonatorBrightnessKnob_->setModAssignmentController(&assignmentController_);
        }
        if (resonatorModesKnob_ != nullptr)
        {
            resonatorModesKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorResonatorModeCount,
                                                        targetIndex);
            resonatorModesKnob_->setModAssignmentController(&assignmentController_);
        }
        if (grainPitchJitterKnob_ != nullptr)
        {
            grainPitchJitterKnob_->enableModulationTarget(processor_, modulation::ModDestination::OperatorGrainPitchJitter,
                                                          targetIndex);
            grainPitchJitterKnob_->setModAssignmentController(&assignmentController_);
        }
        const auto wireTriple = [&](TripleGlowKnob* triple, modulation::ModDestination outer,
                                    modulation::ModDestination middle, modulation::ModDestination inner) {
            if (triple == nullptr)
                return;
            triple->enableOuterModulationTarget(processor_, outer, targetIndex);
            triple->enableMiddleModulationTarget(processor_, middle, targetIndex);
            triple->enableInnerModulationTarget(processor_, inner, targetIndex);
            triple->setModAssignmentController(&assignmentController_);
        };
        wireTriple(fmModTripleKnob_.get(), modulation::ModDestination::OperatorFmModulatorRatio,
                   modulation::ModDestination::OperatorFmModulatorIndex,
                   modulation::ModDestination::OperatorFmModulatorFeedback);
        wireTriple(phaseTripleKnob_.get(), modulation::ModDestination::OperatorPhaseBend,
                   modulation::ModDestination::OperatorPhaseFold, modulation::ModDestination::OperatorPhaseAsymmetry);
        wireTriple(additiveTripleKnob_.get(), modulation::ModDestination::OperatorAdditivePartialCount,
                   modulation::ModDestination::OperatorAdditiveTilt, modulation::ModDestination::OperatorAdditiveOddEven);
        wireTriple(resonatorTripleKnob_.get(), modulation::ModDestination::OperatorResonatorStructure,
                   modulation::ModDestination::OperatorResonatorDecay,
                   modulation::ModDestination::OperatorResonatorDamping);
        wireTriple(grainTripleKnob_.get(), modulation::ModDestination::OperatorGrainDensity,
                   modulation::ModDestination::OperatorGrainSizeMs,
                   modulation::ModDestination::OperatorGrainPositionJitter);
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
        const bool isExternal = engine == static_cast<int>(algorithm::EngineType::External);
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
        waveformKnob_->setVisible(!showsWavetableStack && !isNoiseChaos && !isPhaseShape && !isAdditive && !isResonator
                                  && !isExternal);
        ratioKnob_->setVisible(!isExternal);
        wavetablePosKnob_->setVisible(showsWavetableStack);
        oscWireframeHost_.setVisible(true);
        oscWireframeHost_.setEngine(engineForPillIndex(selectedNode_, engine));
        if (showsWavetableStack)
            oscWireframeHost_.ensureDefaultWavetableLoaded();
        noiseVariantKnob_->setVisible(isNoiseChaos);
        noiseRateKnob_->setVisible(isNoiseChaos);
        phaseTripleKnob_->setVisible(isPhaseShape);
        phaseShapeKnob_->setVisible(isPhaseShape);
        additiveTripleKnob_->setVisible(isAdditive);
        additiveStretchKnob_->setVisible(isAdditive);
        resonatorTripleKnob_->setVisible(isResonator);
        resonatorBrightnessKnob_->setVisible(isResonator);
        resonatorModesKnob_->setVisible(isResonator);
        grainTripleKnob_->setVisible(isGranular);
        grainPitchJitterKnob_->setVisible(isGranular);
        wtWarpKnob_->setVisible(isWavetable);
        wtSyncFormantKnob_->setVisible(isWavetable);
        levelKnob_->setVisible(true);
        panKnob_->setVisible(true);

        if (fmModTripleKnob_ != nullptr)
            fmModTripleKnob_->setVisible(isFmPm);
        if (fmModWaveformKnob_ != nullptr)
            fmModWaveformKnob_->setVisible(isFmPm);

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
        for (int i = 0; i < numEnginePillsForNode(selectedNode_); ++i)
            if (pillBounds(i).contains(pos))
                return i;
        return -1;
    }

    juce::String OperatorEditorPanel::getTooltip()
    {
        const int i = pillIndexAt(getMouseXYRelative());
        if (i < 0)
            return {};
        const auto engine = engineForPillIndex(selectedNode_, i);
        if (algorithm::isEngineImplemented(engine))
            return {}; // No tooltip needed over an already-usable pill.
        return juce::String(engineShortName(engine)) + " renders silence today -- not yet implemented.";
    }

    void OperatorEditorPanel::mouseDown(const juce::MouseEvent& event)
    {
        const int i = pillIndexAt(event.getPosition());
        if (i < 0)
            return;

        const auto engine = engineForPillIndex(selectedNode_, i);
        if (!algorithm::isEngineImplemented(engine))
            return; // Honest no-op -- see the header doc comment on why this stays disabled, not hidden.

        auto& apvts = processor_.apvts;
        const auto paramId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
        if (auto* param = apvts.getParameter(paramId))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
        // Keep patch.engine aligned with the pill (wavetable arrows reload via loadPatch).
        processor_.syncCurrentPatchFromApvts();
        processor_.notifyPatchMetadataChanged();
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

        const bool isGranular = grainTripleKnob_ && grainTripleKnob_->isVisible();
        const float wireframeFrac = isGranular ? 0.40f : 0.45f;
        auto wireframeArea = content.removeFromLeft(static_cast<int>(static_cast<float>(content.getWidth()) * wireframeFrac));
        oscWireframeHost_.setBounds(wireframeArea.reduced(5));

        if (isGranular)
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
            if (grainTripleKnob_)
                grainTripleKnob_->setBounds(content.removeFromLeft(knobWidth * 2).reduced(5));
            if (grainPitchJitterKnob_)
                grainPitchJitterKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (fmModTripleKnob_ && fmModTripleKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 6;
            for (auto* k : {waveformKnob_.get(), levelKnob_.get(), panKnob_.get(), ratioKnob_.get()})
                if (k != nullptr)
                    k->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (fmModTripleKnob_ != nullptr)
                fmModTripleKnob_->setBounds(content.removeFromLeft(knobWidth * 2).reduced(5));
            if (fmModWaveformKnob_ != nullptr)
                fmModWaveformKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
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
        else if (phaseTripleKnob_ && phaseTripleKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 6;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (phaseTripleKnob_)
                phaseTripleKnob_->setBounds(content.removeFromLeft(knobWidth * 2).reduced(5));
            if (phaseShapeKnob_)
                phaseShapeKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (additiveTripleKnob_ && additiveTripleKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 6;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (additiveTripleKnob_)
                additiveTripleKnob_->setBounds(content.removeFromLeft(knobWidth * 2).reduced(5));
            if (additiveStretchKnob_)
                additiveStretchKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        else if (resonatorTripleKnob_ && resonatorTripleKnob_->isVisible())
        {
            const int knobWidth = content.getWidth() / 6;
            if (ratioKnob_)
                ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (levelKnob_)
                levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (panKnob_)
                panKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (resonatorTripleKnob_)
                resonatorTripleKnob_->setBounds(content.removeFromLeft(knobWidth * 2).reduced(5));
            if (resonatorBrightnessKnob_)
                resonatorBrightnessKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (resonatorModesKnob_)
                resonatorModesKnob_->setBounds(content.reduced(5));
        }
        else if (wtWarpKnob_ && wtWarpKnob_->isVisible())
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
            if (wtWarpKnob_)
                wtWarpKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            if (wtSyncFormantKnob_)
                wtSyncFormantKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
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

        for (int i = 0; i < numEnginePillsForNode(selectedNode_); ++i)
        {
            const auto bounds = pillBounds(i).toFloat();
            const auto engine = engineForPillIndex(selectedNode_, i);
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
        const auto activeEngine = engineForPillIndex(selectedNode_, currentEngine);
        if (activeEngine == algorithm::EngineType::External)
        {
            auto content = panel_.getContentBounds();
            auto noteRow = content.removeFromBottom(kNoteHeight).toFloat();
            g.setColour(palette::kMurmurViolet.withAlpha(0.85f));
            g.setFont(fonts::value(9.5f));
            g.drawText("EXT reads the AU Sidechain bus — pick a bus in Logic's Side Chain menu.", noteRow,
                       juce::Justification::centredLeft);
        }
        else if (!algorithm::isEngineImplemented(activeEngine))
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
