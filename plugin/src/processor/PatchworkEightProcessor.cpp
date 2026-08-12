// STATUS: PARTIAL -- see PatchworkEightProcessor.h.

#include <cstdlib>

#include "processor/PatchworkEightProcessor.h"

#include <algorithm>

#include "pw8/core/AudioBlock.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "pw8/content/ContentPaths.hpp"
#include "ui/PlayModeEditor.h"

namespace pw8::plugin
{
    namespace
    {
        [[nodiscard]] float loadF(std::atomic<float>* p) noexcept
        {
            return p != nullptr ? p->load(std::memory_order_relaxed) : 0.0f;
        }

        [[nodiscard]] int loadI(std::atomic<float>* p) noexcept { return static_cast<int>(loadF(p)); }
        [[nodiscard]] bool loadB(std::atomic<float>* p) noexcept { return loadF(p) >= 0.5f; }

        template <typename Array>
        void cacheGroup(juce::AudioProcessorValueTreeState& apvts, Array& pointers, const juce::String& prefix,
                         const auto& fieldSpecs)
        {
            for (std::size_t i = 0; i < fieldSpecs.size(); ++i)
                pointers[i] = apvts.getRawParameterValue(prefix + fieldSpecs[i].idSuffix);
        }
    } // namespace

    PatchworkEightProcessor::PatchworkEightProcessor()
        : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
    {
        cacheParameterPointers();
        syncAllParametersFromPatch(); // matches makeInit()'s defaults, in case those ever diverge from the AudioParameterFloat defaults above.

        engineStorageA_ = std::make_unique<render::Engine>();
        engineStorageA_->loadPatch(currentPatch_);
        activeEngine_.store(engineStorageA_.get(), std::memory_order_release);

#if defined(__APPLE__)
        pw8::content::addSearchRoot("/Library/Application Support/Patchwork Eight");
        if (const char* home = std::getenv("HOME"))
            pw8::content::addSearchRoot(std::string(home) + "/Library/Application Support/Patchwork Eight");
#endif
        if (const char* envRoot = std::getenv("PW8_CONTENT_ROOT"))
        {
            if (envRoot[0] != '\0')
                pw8::content::addSearchRoot(envRoot);
        }
    }

    PatchworkEightProcessor::~PatchworkEightProcessor() = default;

    void PatchworkEightProcessor::cacheParameterPointers()
    {
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            macroParamPointers_[i] = apvts.getRawParameterValue(kMacroParameterIds[i]);

        cacheGroup(apvts, filterParamPointers_, kFilterIdPrefix, kFilterFieldSpecs);
        cacheGroup(apvts, arpParamPointers_, kArpIdPrefix, kArpFieldSpecs);

        for (std::size_t lfo = 0; lfo < kNumLfos; ++lfo)
            for (std::size_t i = 0; i < kNumLfoFields; ++i)
                lfoParamPointers_[lfo][i] = apvts.getRawParameterValue(lfoParamId(lfo, kLfoFieldSpecs[i].idSuffix));

        for (std::size_t env = 0; env < kNumEnvelopes; ++env)
            for (std::size_t i = 0; i < kNumEnvelopeFields; ++i)
                envelopeParamPointers_[env][i] =
                    apvts.getRawParameterValue(envelopeParamId(env, kEnvelopeFieldSpecs[i].idSuffix));

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (std::size_t i = 0; i < kNumOperatorFields; ++i)
                operatorParamPointers_[op][i] = apvts.getRawParameterValue(operatorParamId(op, kOperatorFieldSpecs[i].idSuffix));

        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            for (std::size_t i = 0; i < kNumEffectSlotFields; ++i)
                insertFxParamPointers_[slot][i] =
                    apvts.getRawParameterValue(insertFxParamId(slot, kEffectSlotFieldSpecs[i].idSuffix));

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            for (std::size_t i = 0; i < kNumEffectSlotFields; ++i)
                masterFxParamPointers_[slot][i] =
                    apvts.getRawParameterValue(masterFxParamId(slot, kEffectSlotFieldSpecs[i].idSuffix));

        layerGainPointer_ = apvts.getRawParameterValue(kLayerGainId);
        layerPanPointer_ = apvts.getRawParameterValue(kLayerPanId);
        masterGainPointer_ = apvts.getRawParameterValue(kMasterGainId);
    }

    void PatchworkEightProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
    {
        // Rebuild both storage slots at the new sample rate and republish -- this always
        // runs on the message thread (JUCE guarantees prepareToPlay isn't concurrent with
        // processBlock), so a plain rebuild-then-swap is safe without extra locking.
        auto fresh = std::make_unique<render::Engine>();
        fresh->prepare(sampleRate);
        fresh->loadPatch(currentPatch_);
        publishEngine(std::move(fresh));
    }

    void PatchworkEightProcessor::releaseResources() {}

    void PatchworkEightProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        auto* engine = activeEngine_.load(std::memory_order_acquire);
        if (engine == nullptr)
        {
            buffer.clear();
            return;
        }

        // Tempo-synced LFOs need the host tempo; default to 120 BPM when unavailable
        // (e.g. no playhead, or a host that doesn't report it).
        float bpm = 120.0f;
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition(); position.hasValue())
                if (const auto hostBpm = position->getBpm())
                    bpm = static_cast<float>(*hostBpm);
        }
        engine->setTempo(bpm);

        pushLiveParametersToEngine(*engine);

        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                engine->noteOn(msg.getNoteNumber(), msg.getChannel() - 1, msg.getVelocity());
            else if (msg.isNoteOff())
                engine->noteOff(msg.getNoteNumber(), msg.getChannel() - 1, msg.getVelocity());
            else if (msg.isPitchWheel())
                engine->pitchBend(msg.getChannel() - 1, msg.getPitchWheelValue());
            else if (msg.isController())
                engine->controlChange(msg.getChannel() - 1, msg.getControllerNumber(), msg.getControllerValue());
            else if (msg.isChannelPressure())
                engine->channelPressure(msg.getChannel() - 1, msg.getChannelPressureValue());
            else if (msg.isAftertouch())
                engine->polyAftertouch(msg.getChannel() - 1, msg.getNoteNumber(), msg.getAfterTouchValue());
        }

        buffer.clear();
        if (buffer.getNumChannels() < 2)
            return;

        core::StereoBlockView view(buffer.getWritePointer(0), buffer.getWritePointer(1),
                                    static_cast<std::size_t>(buffer.getNumSamples()));
        engine->process(view);
    }

    void PatchworkEightProcessor::pushLiveParametersToEngine(render::Engine& engine) noexcept
    {
        // Drag-to-modulate (docs/UI.md): consume a pending mod-route publish, if the
        // message thread has posted one since the last block -- see
        // publishModRoutesLive()'s doc comment for the double-buffering scheme.
        if (const auto* pendingRoutes = pendingModRoutes_.exchange(nullptr, std::memory_order_acquire))
            engine.setModRoutesLive(*pendingRoutes);

        // Macros -- unchanged from before this pass, see Engine::setMacroValue().
        for (std::size_t i = 0; i < macroParamPointers_.size(); ++i)
            engine.setMacroValue(i, loadF(macroParamPointers_[i]));

        // Filter1 -- field order matches kFilterFieldSpecs / filter::FilterParams.
        {
            filter::FilterParams fp;
            fp.enabled = loadB(filterParamPointers_[0]);
            fp.mode = static_cast<filter::FilterMode>(loadI(filterParamPointers_[1]));
            fp.cutoffHz = loadF(filterParamPointers_[2]);
            fp.resonance = loadF(filterParamPointers_[3]);
            fp.keyTrack = loadF(filterParamPointers_[4]);
            engine.setFilterLive(fp);
        }

        // 8 LFOs -- field order matches kLfoFieldSpecs / lfo::LfoParams. One
        // setLfoLive() call covers both VOICE scope (per-voice instance) and
        // LAYER/GLOBAL scope (the shared layer-wide tick) -- see Engine::setLfoLive().
        for (std::size_t lfo = 0; lfo < kNumLfos; ++lfo)
        {
            const auto& ptrs = lfoParamPointers_[lfo];
            lfo::LfoParams lp;
            lp.waveform = static_cast<lfo::LfoWaveform>(loadI(ptrs[0]));
            lp.mode = static_cast<lfo::LfoMode>(loadI(ptrs[1]));
            lp.rateHz = loadF(ptrs[2]);
            lp.syncDivisionIndex = loadI(ptrs[3]);
            lp.phaseOffset = loadF(ptrs[4]);
            engine.setLfoLive(lfo, lp);
        }

        // 8 operators -- field order matches kOperatorFieldSpecs / op::OperatorParams.
        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            const auto& ptrs = operatorParamPointers_[op];
            op::OperatorParams params;
            params.engine = static_cast<algorithm::EngineType>(loadI(ptrs[0]));
            params.classic.waveform = static_cast<oscillator::ClassicWaveform>(loadI(ptrs[1]));
            params.classic.morph = loadF(ptrs[2]);
            params.classic.pulseWidth = loadF(ptrs[3]);
            params.wavetableFramePosition = loadF(ptrs[4]);
            params.frequencyRatio = loadF(ptrs[5]);
            params.fixedFrequencyHz = loadF(ptrs[6]);
            params.keyTrack = loadB(ptrs[7]);
            params.level = loadF(ptrs[8]);
            params.fmModulatorRatio = loadF(ptrs[9]);
            params.fmModulatorIndex = loadF(ptrs[10]);
            params.fmModulatorFeedback = loadF(ptrs[11]);
            params.fmModulatorWaveform = static_cast<oscillator::ClassicWaveform>(loadI(ptrs[12]));
            params.noiseVariant = loadF(ptrs[13]);
            params.noiseRate = loadF(ptrs[14]);
            params.phaseBend = loadF(ptrs[15]);
            params.phaseFold = loadF(ptrs[16]);
            params.phaseAsymmetry = loadF(ptrs[17]);
            params.phaseShape = loadF(ptrs[18]);
            params.additivePartialCount = loadF(ptrs[19]);
            params.additiveTilt = loadF(ptrs[20]);
            params.additiveOddEven = loadF(ptrs[21]);
            params.additiveStretch = loadF(ptrs[22]);
            params.resonatorStructure = loadF(ptrs[23]);
            params.resonatorDecay = loadF(ptrs[24]);
            params.resonatorDamping = loadF(ptrs[25]);
            params.resonatorBrightness = loadF(ptrs[26]);
            params.resonatorModeCount = loadF(ptrs[27]);
            params.grainDensity = loadF(ptrs[28]);
            params.grainSizeMs = loadF(ptrs[29]);
            params.grainPositionJitter = loadF(ptrs[30]);
            params.grainPitchJitter = loadF(ptrs[31]);
            engine.setOperatorLive(op, params);
        }

        // 8 envelopes -- field order matches kEnvelopeFieldSpecs / envelope::DahdsrParams.
        // envelopes[0] is the amp envelope; envelopes[1..7] are free-standing mod
        // sources. Each takes effect on its owner's NEXT note-on -- see
        // Engine::setEnvelopeLive().
        for (std::size_t env = 0; env < kNumEnvelopes; ++env)
        {
            const auto& ptrs = envelopeParamPointers_[env];
            envelope::DahdsrParams ep;
            ep.delaySeconds = loadF(ptrs[0]);
            ep.attackSeconds = loadF(ptrs[1]);
            ep.holdSeconds = loadF(ptrs[2]);
            ep.decaySeconds = loadF(ptrs[3]);
            ep.sustainLevel = loadF(ptrs[4]);
            ep.releaseSeconds = loadF(ptrs[5]);
            ep.curveShape = loadF(ptrs[6]);
            ep.legato = loadB(ptrs[7]);
            engine.setEnvelopeLive(env, ep);
        }

        engine.setLayerGainLive(loadF(layerGainPointer_));
        engine.setLayerPanLive(loadF(layerPanPointer_));
        engine.setMasterGainLive(loadF(masterGainPointer_));

        // Insert/master FX slots -- field order matches kEffectSlotFieldSpecs /
        // effects::EffectSlotParams's scalar fields. Read-modify-write against the
        // Engine's current value first so NodeDelay/FractalEcho's `nodes[]` and
        // FractalEcho's seeds (not exposed to automation) are preserved rather than
        // stomped with defaults every block.
        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
        {
            const auto& ptrs = insertFxParamPointers_[slot];
            effects::EffectSlotParams p = engine.getInsertEffectParams(slot);
            p.type = static_cast<effects::EffectType>(loadI(ptrs[0]));
            p.mix = loadF(ptrs[1]);
            p.saturationDriveDb = loadF(ptrs[2]);
            p.chorusRateHz = loadF(ptrs[3]);
            p.chorusDepthMs = loadF(ptrs[4]);
            p.chorusBaseDelayMs = loadF(ptrs[5]);
            p.tapeDelayMs = loadF(ptrs[6]);
            p.tapeFeedback = loadF(ptrs[7]);
            p.tapeDriveDb = loadF(ptrs[8]);
            p.tapeDuckAmount = loadF(ptrs[9]);
            p.tapeDriftDepthMs = loadF(ptrs[10]);
            p.tapeDriftRateHz = loadF(ptrs[11]);
            p.tapePanMode = static_cast<effects::DelayPanMode>(loadI(ptrs[12]));
            p.nodeInsanity = loadF(ptrs[13]);
            p.freqShiftHz = loadF(ptrs[14]);
            p.freqShiftDelayMs = loadF(ptrs[15]);
            p.freqShiftFeedback = loadF(ptrs[16]);
            p.freqShiftLowCutHz = loadF(ptrs[17]);
            p.freqShiftHighCutHz = loadF(ptrs[18]);
            p.fractalMorph = loadF(ptrs[19]);
            p.fractalBaseDelayMs = loadF(ptrs[20]);
            p.fractalRatio = loadF(ptrs[21]);
            p.fractalSpreadMs = loadF(ptrs[22]);
            p.reverbSizeParam = loadF(ptrs[23]);
            p.reverbDecaySeconds = loadF(ptrs[24]);
            p.reverbPreDelayMs = loadF(ptrs[25]);
            p.reverbHighRatio = loadF(ptrs[26]);
            p.reverbHighCrossoverHz = loadF(ptrs[27]);
            p.reverbLowRatio = loadF(ptrs[28]);
            p.reverbLowCrossoverHz = loadF(ptrs[29]);
            p.reverbDiffusion = loadF(ptrs[30]);
            p.reverbDensity = loadF(ptrs[31]);
            p.reverbModDepth = loadF(ptrs[32]);
            p.reverbModRateHz = loadF(ptrs[33]);
            p.reverbEarlyLevel = loadF(ptrs[34]);
            p.reverbLateLevel = loadF(ptrs[35]);
            p.reverbRollOffHz = loadF(ptrs[36]);
            p.reverbVlfCutDb = loadF(ptrs[37]);
            p.eqLowFreqHz = loadF(ptrs[38]);
            p.eqLowGainDb = loadF(ptrs[39]);
            p.eqMidFreqHz = loadF(ptrs[40]);
            p.eqMidGainDb = loadF(ptrs[41]);
            p.eqMidQ = loadF(ptrs[42]);
            p.eqHighFreqHz = loadF(ptrs[43]);
            p.eqHighGainDb = loadF(ptrs[44]);
            p.compThresholdDb = loadF(ptrs[45]);
            p.compRatio = loadF(ptrs[46]);
            p.compAttackMs = loadF(ptrs[47]);
            p.compReleaseMs = loadF(ptrs[48]);
            p.compKneeDb = loadF(ptrs[49]);
            p.compMakeupDb = loadF(ptrs[50]);
            p.limiterCeilingDb = loadF(ptrs[51]);
            p.limiterLookaheadMs = loadF(ptrs[52]);
            p.limiterReleaseMs = loadF(ptrs[53]);
            engine.setInsertEffectLive(slot, p);
        }

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
        {
            const auto& ptrs = masterFxParamPointers_[slot];
            effects::EffectSlotParams p = engine.getMasterEffectParams(slot);
            p.type = static_cast<effects::EffectType>(loadI(ptrs[0]));
            p.mix = loadF(ptrs[1]);
            p.saturationDriveDb = loadF(ptrs[2]);
            p.chorusRateHz = loadF(ptrs[3]);
            p.chorusDepthMs = loadF(ptrs[4]);
            p.chorusBaseDelayMs = loadF(ptrs[5]);
            p.tapeDelayMs = loadF(ptrs[6]);
            p.tapeFeedback = loadF(ptrs[7]);
            p.tapeDriveDb = loadF(ptrs[8]);
            p.tapeDuckAmount = loadF(ptrs[9]);
            p.tapeDriftDepthMs = loadF(ptrs[10]);
            p.tapeDriftRateHz = loadF(ptrs[11]);
            p.tapePanMode = static_cast<effects::DelayPanMode>(loadI(ptrs[12]));
            p.nodeInsanity = loadF(ptrs[13]);
            p.freqShiftHz = loadF(ptrs[14]);
            p.freqShiftDelayMs = loadF(ptrs[15]);
            p.freqShiftFeedback = loadF(ptrs[16]);
            p.freqShiftLowCutHz = loadF(ptrs[17]);
            p.freqShiftHighCutHz = loadF(ptrs[18]);
            p.fractalMorph = loadF(ptrs[19]);
            p.fractalBaseDelayMs = loadF(ptrs[20]);
            p.fractalRatio = loadF(ptrs[21]);
            p.fractalSpreadMs = loadF(ptrs[22]);
            p.reverbSizeParam = loadF(ptrs[23]);
            p.reverbDecaySeconds = loadF(ptrs[24]);
            p.reverbPreDelayMs = loadF(ptrs[25]);
            p.reverbHighRatio = loadF(ptrs[26]);
            p.reverbHighCrossoverHz = loadF(ptrs[27]);
            p.reverbLowRatio = loadF(ptrs[28]);
            p.reverbLowCrossoverHz = loadF(ptrs[29]);
            p.reverbDiffusion = loadF(ptrs[30]);
            p.reverbDensity = loadF(ptrs[31]);
            p.reverbModDepth = loadF(ptrs[32]);
            p.reverbModRateHz = loadF(ptrs[33]);
            p.reverbEarlyLevel = loadF(ptrs[34]);
            p.reverbLateLevel = loadF(ptrs[35]);
            p.reverbRollOffHz = loadF(ptrs[36]);
            p.reverbVlfCutDb = loadF(ptrs[37]);
            p.eqLowFreqHz = loadF(ptrs[38]);
            p.eqLowGainDb = loadF(ptrs[39]);
            p.eqMidFreqHz = loadF(ptrs[40]);
            p.eqMidGainDb = loadF(ptrs[41]);
            p.eqMidQ = loadF(ptrs[42]);
            p.eqHighFreqHz = loadF(ptrs[43]);
            p.eqHighGainDb = loadF(ptrs[44]);
            p.compThresholdDb = loadF(ptrs[45]);
            p.compRatio = loadF(ptrs[46]);
            p.compAttackMs = loadF(ptrs[47]);
            p.compReleaseMs = loadF(ptrs[48]);
            p.compKneeDb = loadF(ptrs[49]);
            p.compMakeupDb = loadF(ptrs[50]);
            p.limiterCeilingDb = loadF(ptrs[51]);
            p.limiterLookaheadMs = loadF(ptrs[52]);
            p.limiterReleaseMs = loadF(ptrs[53]);
            engine.setMasterEffectLive(slot, p);
        }

        // Arpeggiator -- field order matches kArpFieldSpecs. `.steps` is left
        // default-constructed here; Engine::setArpeggiatorScalarLive() preserves the
        // actually-loaded pattern internally (see its doc comment), so we don't need
        // a read-modify-write here the way effects need one.
        {
            sequencer::ArpeggiatorParams ap;
            ap.enabled = loadB(arpParamPointers_[0]);
            ap.mode = static_cast<sequencer::ArpMode>(loadI(arpParamPointers_[1]));
            ap.rateMode = static_cast<sequencer::ArpRateMode>(loadI(arpParamPointers_[2]));
            ap.rateHz = loadF(arpParamPointers_[3]);
            ap.syncDivisionIndex = loadI(arpParamPointers_[4]);
            ap.octaveRange = loadI(arpParamPointers_[5]);
            ap.numSteps = static_cast<std::size_t>(loadI(arpParamPointers_[6]));
            ap.latch = loadB(arpParamPointers_[7]);
            engine.setArpeggiatorScalarLive(ap);
        }
    }

    juce::AudioProcessorEditor* PatchworkEightProcessor::createEditor()
    {
        // The real PLAY-mode editor (docs/PLUGIN_ARCHITECTURE.md "Editor"; the OBSIDIAN
        // skin, docs/UI.md). DESIGN/LAB modes are PLANNED -- PLAY mode is the one
        // screen that exists today.
        return new ui::PlayModeEditor(*this);
    }

    void PatchworkEightProcessor::getStateInformation(juce::MemoryBlock& destData)
    {
        // Bake every automatable field's current (possibly host-automated-since-load)
        // value back into currentPatch_ before serializing, so a saved session
        // round-trips them exactly rather than only ever saving the preset's original
        // defaults.
        syncPatchFromAllParameters();
        const auto json = patch::savePatchToJson(currentPatch_);
        destData.replaceAll(json.data(), json.size());
    }

    void PatchworkEightProcessor::setStateInformation(const void* data, int sizeInBytes)
    {
        const std::string_view json(static_cast<const char*>(data), static_cast<std::size_t>(sizeInBytes));
        const auto result = patch::loadPatchFromJson(json);
        if (result.ok)
            loadPatch(result.patch);
    }

    bool PatchworkEightProcessor::loadPatch(const patch::Patch& newPatch)
    {
        currentPatch_ = newPatch;
        syncAllParametersFromPatch();
        auto fresh = std::make_unique<render::Engine>();
        fresh->prepare(getSampleRate() > 0.0 ? getSampleRate() : 48000.0);
        const bool ok = fresh->loadPatch(currentPatch_);
        publishEngine(std::move(fresh));
        // Discard any not-yet-consumed drag-to-modulate publish from BEFORE this
        // reload. Without this, a mod-route drag that hasn't yet been picked up by
        // the audio thread's next block could still be sitting in
        // pendingModRoutes_ and get applied to the Engine we just published above,
        // silently overwriting `newPatch`'s own (correct) mod routes with stale
        // pre-reload ones. `fresh` was already built from newPatch's real routes,
        // so there's nothing useful left for a stale publish to contribute.
        pendingModRoutes_.store(nullptr, std::memory_order_release);
        return ok;
    }

    bool PatchworkEightProcessor::loadPatchFromFile(const juce::String& filePath)
    {
        const juce::File file(filePath);
        if (!file.existsAsFile())
            return false;

        const auto json = file.loadFileAsString();
        const auto result = patch::loadPatchFromJson(json.toStdString());
        if (!result.ok)
            return false;

        currentPresetPath_ = file.getFullPathName();
        return loadPatch(result.patch);
    }

    bool PatchworkEightProcessor::setOperatorWavetableFile(std::size_t opIndex, const juce::String& filePath)
    {
        if (opIndex >= currentPatch_.layerA.operators.size())
            return false;

        auto newPatch = currentPatch_;
        newPatch.layerA.operators[opIndex].wavetableId = filePath.toStdString();
        return loadPatch(newPatch);
    }

    void PatchworkEightProcessor::publishModRoutesLive(
        const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& routes)
    {
        // Write into whichever slot wasn't published last -- see the member's doc
        // comment in the header for the full "why this is safe" reasoning.
        modRoutesStorageUsingA_ = !modRoutesStorageUsingA_;
        auto& slot = modRoutesStorage_[modRoutesStorageUsingA_ ? 0 : 1];
        slot = routes;
        pendingModRoutes_.store(&slot, std::memory_order_release);
    }

    void PatchworkEightProcessor::setOrReplaceModRouteLive(modulation::ModSource source,
                                                             modulation::ModDestination destination,
                                                             std::uint8_t targetIndex, float amount,
                                                             modulation::ModScope scope)
    {
        auto routes = currentPatch_.layerA.modRoutes;
        bool replaced = false;
        for (auto& route : routes)
        {
            if (route.destination == destination && route.targetIndex == targetIndex)
            {
                route.source = source;
                route.amount = amount;
                route.scope = scope;
                replaced = true;
                break;
            }
        }
        if (!replaced)
        {
            modulation::ModRoute route;
            route.source = source;
            route.destination = destination;
            route.targetIndex = targetIndex;
            route.amount = amount;
            route.scope = scope;
            routes.push_back(route); // A silent no-op past kMaxModRoutes (64) capacity, same as every other FixedVector use in this codebase.
        }

        currentPatch_.layerA.modRoutes = routes; // Keep the getStateInformation()/preset-save source of truth in sync.
        hasUserCreatedModRouteLive_ = true;
        publishModRoutesLive(routes);
    }

    void PatchworkEightProcessor::removeModRouteLive(modulation::ModSource source,
                                                       modulation::ModDestination destination,
                                                       std::uint8_t targetIndex)
    {
        core::FixedVector<modulation::ModRoute, core::kMaxModRoutes> routes;
        for (const auto& route : currentPatch_.layerA.modRoutes)
            if (!(route.source == source && route.destination == destination && route.targetIndex == targetIndex))
                routes.push_back(route);

        currentPatch_.layerA.modRoutes = routes;
        publishModRoutesLive(routes);
    }

    void PatchworkEightProcessor::syncAllParametersFromPatch()
    {
        auto setParam = [this](const juce::String& id, float value) {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            setParam(kMacroParameterIds[i], currentPatch_.macros[i].value);

        const auto& filter = currentPatch_.layerA.filter1;
        const std::array<float, kNumFilterFields> filterValues = {
            filter.enabled ? 1.0f : 0.0f, static_cast<float>(filter.mode), filter.cutoffHz, filter.resonance,
            filter.keyTrack,
        };
        for (std::size_t i = 0; i < kNumFilterFields; ++i)
            setParam(juce::String(kFilterIdPrefix) + kFilterFieldSpecs[i].idSuffix, filterValues[i]);

        for (std::size_t lfoIdx = 0; lfoIdx < kNumLfos; ++lfoIdx)
        {
            const auto& lfo = currentPatch_.layerA.lfos[lfoIdx];
            const std::array<float, kNumLfoFields> lfoValues = {
                static_cast<float>(lfo.waveform), static_cast<float>(lfo.mode), lfo.rateHz,
                static_cast<float>(lfo.syncDivisionIndex), lfo.phaseOffset,
            };
            for (std::size_t i = 0; i < kNumLfoFields; ++i)
                setParam(lfoParamId(lfoIdx, kLfoFieldSpecs[i].idSuffix), lfoValues[i]);
        }

        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            const auto& o = currentPatch_.layerA.operators[op];
            const std::array<float, kNumOperatorFields> opValues = {
                static_cast<float>(o.engine),       static_cast<float>(o.classicWaveform),
                o.classicMorph,                     o.pulseWidth,
                o.wavetableFramePosition,           o.frequencyRatio,
                o.fixedFrequencyHz,                 o.keyTrack ? 1.0f : 0.0f,
                o.level,
                o.fmModulatorRatio,                 o.fmModulatorIndex,
                o.fmModulatorFeedback,               static_cast<float>(o.fmModulatorWaveform),
                o.noiseVariant,                      o.noiseRate,
                o.phaseBend,                          o.phaseFold,
                o.phaseAsymmetry,                     o.phaseShape,
                o.additivePartialCount,               o.additiveTilt,
                o.additiveOddEven,                    o.additiveStretch,
                o.resonatorStructure,                 o.resonatorDecay,
                o.resonatorDamping,                   o.resonatorBrightness,
                o.resonatorModeCount,
                o.grainDensity,                       o.grainSizeMs,
                o.grainPositionJitter,                o.grainPitchJitter,
            };
            for (std::size_t i = 0; i < kNumOperatorFields; ++i)
                setParam(operatorParamId(op, kOperatorFieldSpecs[i].idSuffix), opValues[i]);
        }

        for (std::size_t envIdx = 0; envIdx < kNumEnvelopes; ++envIdx)
        {
            const auto& env = currentPatch_.layerA.envelopes[envIdx];
            const std::array<float, kNumEnvelopeFields> envValues = {
                env.delaySeconds, env.attackSeconds, env.holdSeconds,  env.decaySeconds,
                env.sustainLevel, env.releaseSeconds, env.curveShape,  env.legato ? 1.0f : 0.0f,
            };
            for (std::size_t i = 0; i < kNumEnvelopeFields; ++i)
                setParam(envelopeParamId(envIdx, kEnvelopeFieldSpecs[i].idSuffix), envValues[i]);
        }

        setParam(kLayerGainId, currentPatch_.layerA.gain);
        setParam(kLayerPanId, currentPatch_.layerA.pan);
        setParam(kMasterGainId, currentPatch_.voiceSettings.masterGain);

        auto syncFxSlot = [&](const effects::EffectSlotParams& p, const juce::String& id) {
            const std::array<float, kNumEffectSlotFields> values = {
                static_cast<float>(p.type), p.mix,        p.saturationDriveDb, p.chorusRateHz,   p.chorusDepthMs,
                p.chorusBaseDelayMs,        p.tapeDelayMs, p.tapeFeedback,      p.tapeDriveDb,    p.tapeDuckAmount,
                p.tapeDriftDepthMs,         p.tapeDriftRateHz, static_cast<float>(p.tapePanMode), p.nodeInsanity,
                p.freqShiftHz,              p.freqShiftDelayMs, p.freqShiftFeedback, p.freqShiftLowCutHz,
                p.freqShiftHighCutHz,       p.fractalMorph, p.fractalBaseDelayMs, p.fractalRatio, p.fractalSpreadMs,
                p.reverbSizeParam,          p.reverbDecaySeconds, p.reverbPreDelayMs,
                p.reverbHighRatio,          p.reverbHighCrossoverHz, p.reverbLowRatio, p.reverbLowCrossoverHz,
                p.reverbDiffusion,          p.reverbDensity, p.reverbModDepth,    p.reverbModRateHz,
                p.reverbEarlyLevel,         p.reverbLateLevel, p.reverbRollOffHz, p.reverbVlfCutDb,
                p.eqLowFreqHz,              p.eqLowGainDb,  p.eqMidFreqHz,       p.eqMidGainDb,    p.eqMidQ,
                p.eqHighFreqHz,             p.eqHighGainDb,
                p.compThresholdDb,          p.compRatio,    p.compAttackMs,      p.compReleaseMs,  p.compKneeDb,
                p.compMakeupDb,
                p.limiterCeilingDb,         p.limiterLookaheadMs, p.limiterReleaseMs,
            };
            for (std::size_t i = 0; i < kNumEffectSlotFields; ++i)
                setParam(id + kEffectSlotFieldSpecs[i].idSuffix, values[i]);
        };
        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            syncFxSlot(currentPatch_.layerA.insertEffects[slot], juce::String("insertFx") + juce::String(static_cast<int>(slot)));
        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            syncFxSlot(currentPatch_.masterEffects[slot], juce::String("masterFx") + juce::String(static_cast<int>(slot)));

        const auto& arp = currentPatch_.arpeggiator;
        const std::array<float, kNumArpFields> arpValues = {
            arp.enabled ? 1.0f : 0.0f,
            static_cast<float>(arp.mode),
            static_cast<float>(arp.rateMode),
            arp.rateHz,
            static_cast<float>(arp.syncDivisionIndex),
            static_cast<float>(arp.octaveRange),
            static_cast<float>(arp.numSteps),
            arp.latch ? 1.0f : 0.0f,
        };
        for (std::size_t i = 0; i < kNumArpFields; ++i)
            setParam(juce::String(kArpIdPrefix) + kArpFieldSpecs[i].idSuffix, arpValues[i]);
    }

    void PatchworkEightProcessor::syncPatchFromAllParameters()
    {
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            currentPatch_.macros[i].value = loadF(macroParamPointers_[i]);

        auto& filter = currentPatch_.layerA.filter1;
        filter.enabled = loadB(filterParamPointers_[0]);
        filter.mode = static_cast<pw8::filter::FilterMode>(loadI(filterParamPointers_[1]));
        filter.cutoffHz = loadF(filterParamPointers_[2]);
        filter.resonance = loadF(filterParamPointers_[3]);
        filter.keyTrack = loadF(filterParamPointers_[4]);

        for (std::size_t lfoIdx = 0; lfoIdx < kNumLfos; ++lfoIdx)
        {
            const auto& ptrs = lfoParamPointers_[lfoIdx];
            auto& lfo = currentPatch_.layerA.lfos[lfoIdx];
            lfo.waveform = static_cast<pw8::lfo::LfoWaveform>(loadI(ptrs[0]));
            lfo.mode = static_cast<pw8::lfo::LfoMode>(loadI(ptrs[1]));
            lfo.rateHz = loadF(ptrs[2]);
            lfo.syncDivisionIndex = loadI(ptrs[3]);
            lfo.phaseOffset = loadF(ptrs[4]);
        }

        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            const auto& ptrs = operatorParamPointers_[op];
            auto& o = currentPatch_.layerA.operators[op];
            o.engine = static_cast<algorithm::EngineType>(loadI(ptrs[0]));
            o.classicWaveform = static_cast<oscillator::ClassicWaveform>(loadI(ptrs[1]));
            o.classicMorph = loadF(ptrs[2]);
            o.pulseWidth = loadF(ptrs[3]);
            o.wavetableFramePosition = loadF(ptrs[4]);
            o.frequencyRatio = loadF(ptrs[5]);
            o.fixedFrequencyHz = loadF(ptrs[6]);
            o.keyTrack = loadB(ptrs[7]);
            o.level = loadF(ptrs[8]);
            o.fmModulatorRatio = loadF(ptrs[9]);
            o.fmModulatorIndex = loadF(ptrs[10]);
            o.fmModulatorFeedback = loadF(ptrs[11]);
            o.fmModulatorWaveform = static_cast<oscillator::ClassicWaveform>(loadI(ptrs[12]));
            o.noiseVariant = loadF(ptrs[13]);
            o.noiseRate = loadF(ptrs[14]);
            o.phaseBend = loadF(ptrs[15]);
            o.phaseFold = loadF(ptrs[16]);
            o.phaseAsymmetry = loadF(ptrs[17]);
            o.phaseShape = loadF(ptrs[18]);
            o.additivePartialCount = loadF(ptrs[19]);
            o.additiveTilt = loadF(ptrs[20]);
            o.additiveOddEven = loadF(ptrs[21]);
            o.additiveStretch = loadF(ptrs[22]);
            o.resonatorStructure = loadF(ptrs[23]);
            o.resonatorDecay = loadF(ptrs[24]);
            o.resonatorDamping = loadF(ptrs[25]);
            o.resonatorBrightness = loadF(ptrs[26]);
            o.resonatorModeCount = loadF(ptrs[27]);
            o.grainDensity = loadF(ptrs[28]);
            o.grainSizeMs = loadF(ptrs[29]);
            o.grainPositionJitter = loadF(ptrs[30]);
            o.grainPitchJitter = loadF(ptrs[31]);
        }

        for (std::size_t envIdx = 0; envIdx < kNumEnvelopes; ++envIdx)
        {
            const auto& ptrs = envelopeParamPointers_[envIdx];
            auto& env = currentPatch_.layerA.envelopes[envIdx];
            env.delaySeconds = loadF(ptrs[0]);
            env.attackSeconds = loadF(ptrs[1]);
            env.holdSeconds = loadF(ptrs[2]);
            env.decaySeconds = loadF(ptrs[3]);
            env.sustainLevel = loadF(ptrs[4]);
            env.releaseSeconds = loadF(ptrs[5]);
            env.curveShape = loadF(ptrs[6]);
            env.legato = loadB(ptrs[7]);
        }

        currentPatch_.layerA.gain = loadF(layerGainPointer_);
        currentPatch_.layerA.pan = loadF(layerPanPointer_);
        currentPatch_.voiceSettings.masterGain = loadF(masterGainPointer_);

        auto readFxSlot = [](const std::array<std::atomic<float>*, kNumEffectSlotFields>& ptrs,
                              effects::EffectSlotParams& p) {
            p.type = static_cast<effects::EffectType>(loadI(ptrs[0]));
            p.mix = loadF(ptrs[1]);
            p.saturationDriveDb = loadF(ptrs[2]);
            p.chorusRateHz = loadF(ptrs[3]);
            p.chorusDepthMs = loadF(ptrs[4]);
            p.chorusBaseDelayMs = loadF(ptrs[5]);
            p.tapeDelayMs = loadF(ptrs[6]);
            p.tapeFeedback = loadF(ptrs[7]);
            p.tapeDriveDb = loadF(ptrs[8]);
            p.tapeDuckAmount = loadF(ptrs[9]);
            p.tapeDriftDepthMs = loadF(ptrs[10]);
            p.tapeDriftRateHz = loadF(ptrs[11]);
            p.tapePanMode = static_cast<effects::DelayPanMode>(loadI(ptrs[12]));
            p.nodeInsanity = loadF(ptrs[13]);
            p.freqShiftHz = loadF(ptrs[14]);
            p.freqShiftDelayMs = loadF(ptrs[15]);
            p.freqShiftFeedback = loadF(ptrs[16]);
            p.freqShiftLowCutHz = loadF(ptrs[17]);
            p.freqShiftHighCutHz = loadF(ptrs[18]);
            p.fractalMorph = loadF(ptrs[19]);
            p.fractalBaseDelayMs = loadF(ptrs[20]);
            p.fractalRatio = loadF(ptrs[21]);
            p.fractalSpreadMs = loadF(ptrs[22]);
            p.reverbSizeParam = loadF(ptrs[23]);
            p.reverbDecaySeconds = loadF(ptrs[24]);
            p.reverbPreDelayMs = loadF(ptrs[25]);
            p.reverbHighRatio = loadF(ptrs[26]);
            p.reverbHighCrossoverHz = loadF(ptrs[27]);
            p.reverbLowRatio = loadF(ptrs[28]);
            p.reverbLowCrossoverHz = loadF(ptrs[29]);
            p.reverbDiffusion = loadF(ptrs[30]);
            p.reverbDensity = loadF(ptrs[31]);
            p.reverbModDepth = loadF(ptrs[32]);
            p.reverbModRateHz = loadF(ptrs[33]);
            p.reverbEarlyLevel = loadF(ptrs[34]);
            p.reverbLateLevel = loadF(ptrs[35]);
            p.reverbRollOffHz = loadF(ptrs[36]);
            p.reverbVlfCutDb = loadF(ptrs[37]);
            p.eqLowFreqHz = loadF(ptrs[38]);
            p.eqLowGainDb = loadF(ptrs[39]);
            p.eqMidFreqHz = loadF(ptrs[40]);
            p.eqMidGainDb = loadF(ptrs[41]);
            p.eqMidQ = loadF(ptrs[42]);
            p.eqHighFreqHz = loadF(ptrs[43]);
            p.eqHighGainDb = loadF(ptrs[44]);
            p.compThresholdDb = loadF(ptrs[45]);
            p.compRatio = loadF(ptrs[46]);
            p.compAttackMs = loadF(ptrs[47]);
            p.compReleaseMs = loadF(ptrs[48]);
            p.compKneeDb = loadF(ptrs[49]);
            p.compMakeupDb = loadF(ptrs[50]);
            p.limiterCeilingDb = loadF(ptrs[51]);
            p.limiterLookaheadMs = loadF(ptrs[52]);
            p.limiterReleaseMs = loadF(ptrs[53]);
        };
        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            readFxSlot(insertFxParamPointers_[slot], currentPatch_.layerA.insertEffects[slot]);
        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            readFxSlot(masterFxParamPointers_[slot], currentPatch_.masterEffects[slot]);

        auto& arp = currentPatch_.arpeggiator;
        arp.enabled = loadB(arpParamPointers_[0]);
        arp.mode = static_cast<sequencer::ArpMode>(loadI(arpParamPointers_[1]));
        arp.rateMode = static_cast<sequencer::ArpRateMode>(loadI(arpParamPointers_[2]));
        arp.rateHz = loadF(arpParamPointers_[3]);
        arp.syncDivisionIndex = loadI(arpParamPointers_[4]);
        arp.octaveRange = loadI(arpParamPointers_[5]);
        arp.numSteps = static_cast<std::size_t>(std::max(1, loadI(arpParamPointers_[6])));
        arp.latch = loadB(arpParamPointers_[7]);
    }

    void PatchworkEightProcessor::publishEngine(std::unique_ptr<render::Engine> newEngine)
    {
        // Double-buffer so the previously-active Engine (which the audio thread may have
        // just finished reading) stays alive until the NEXT swap, rather than being
        // destroyed out from under a very last in-flight processBlock() call.
        if (usingStorageA_)
        {
            engineStorageB_ = std::move(newEngine);
            activeEngine_.store(engineStorageB_.get(), std::memory_order_release);
        }
        else
        {
            engineStorageA_ = std::move(newEngine);
            activeEngine_.store(engineStorageA_.get(), std::memory_order_release);
        }
        usingStorageA_ = !usingStorageA_;
    }

} // namespace pw8::plugin

// Standard JUCE plugin entry point.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pw8::plugin::PatchworkEightProcessor();
}
