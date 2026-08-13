// STATUS: PARTIAL -- see PatchworkEightProcessor.h.

#include <algorithm>
#include <array>
#include <cstdlib>

#include "processor/PatchworkEightProcessor.h"
#include "processor/PerformanceMidiMap.hpp"

#include "pw8/content/ContentPaths.hpp"
#include "pw8/content/WavetableCache.hpp"
#include "pw8/core/AudioBlock.hpp"
#include "pw8/patch/PatchModDefaults.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "pw8/render/BlockMidi.hpp"
#include "pw8/sequencer/ArpeggiatorTypes.hpp"
#include "processor/EffectLatency.hpp"
#include "ui/MurmurRootEditor.h"

#include <juce_core/juce_core.h>

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
        registerParamListeners();
        syncAllParametersFromPatch(); // matches makeInit()'s defaults, in case those ever diverge from the AudioParameterFloat defaults above.

        engineStorageA_ = std::make_unique<render::Engine>();
        engineStorageA_->loadPatch(currentPatch_);
        activeEngine_.store(engineStorageA_.get(), std::memory_order_release);

#if defined(__APPLE__)
        pw8::content::addSearchRoot("/Library/Application Support/MURMUR");
        pw8::content::addSearchRoot("/Library/Application Support/Patchwork Eight");
        if (const char* home = std::getenv("HOME"))
        {
            pw8::content::addSearchRoot(std::string(home) + "/Library/Application Support/MURMUR");
            pw8::content::addSearchRoot(std::string(home) + "/Library/Application Support/Patchwork Eight");
        }
#endif
        if (const char* envRoot = std::getenv("PW8_CONTENT_ROOT"))
        {
            if (envRoot[0] != '\0')
                pw8::content::addSearchRoot(envRoot);
        }

        // Standalone/VST launched from Finder often have cwd=/; walk from the
        // binary to find the dev repo's content/wavetables/ tree.
        const auto exeFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        pw8::content::addSearchRootsFromAncestorWalk(exeFile.getFullPathName().toStdString());
    }

    PatchworkEightProcessor::~PatchworkEightProcessor() = default;

    void PatchworkEightProcessor::cacheParameterPointers()
    {
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            macroParamPointers_[i] = apvts.getRawParameterValue(kMacroParameterIds[i]);

        cacheGroup(apvts, filterParamPointers_, kFilterIdPrefix, kFilterFieldSpecs);
        cacheGroup(apvts, filter2ParamPointers_, kFilter2IdPrefix, kFilter2FieldSpecs);
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

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (std::size_t i = 0; i < kNumOperatorFilterFields; ++i)
                operatorFilterParamPointers_[op][i] =
                    apvts.getRawParameterValue(operatorFilterParamId(op, kOperatorFilterFieldSpecs[i].idSuffix));

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
        modWheelParamPointer_ = apvts.getRawParameterValue(kModWheelId);
        expressionParamPointer_ = apvts.getRawParameterValue(kExpressionId);
    }

    void PatchworkEightProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
    {
        currentSampleRate_ = sampleRate;
        scopeAudioTap_.reset();
        // Rebuild both storage slots at the new sample rate and republish -- this always
        // runs on the message thread (JUCE guarantees prepareToPlay isn't concurrent with
        // processBlock), so a plain rebuild-then-swap is safe without extra locking.
        auto fresh = std::make_unique<render::Engine>();
        fresh->prepare(sampleRate);
        fresh->setQualityMode(qualityMode_);
        fresh->loadPatch(currentPatch_);
        publishEngine(std::move(fresh));
        paramChangeQueue_.pushAllGroups();
        updateReportedLatency();
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
        bool hostIsPlaying = false;
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition(); position.hasValue())
            {
                if (const auto hostBpm = position->getBpm())
                    bpm = static_cast<float>(*hostBpm);
                hostIsPlaying = position->getIsPlaying();
            }
        }
        engine->setTempo(bpm);

        // Logic and many hosts do not send note-offs when transport stops — silence voices
        // and reset the arpeggiator so notes do not hang indefinitely.
        if (hostWasPlaying_ && !hostIsPlaying)
            engine->allSoundOff();
        hostWasPlaying_ = hostIsPlaying;

        // Map performance CCs to macros/master before pushing APVTS → engine (MP11SE layout).
        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            if (msg.isController())
                applyPerformanceCcToApvts(msg.getControllerNumber(), msg.getControllerValue(), macroParamPointers_,
                                          masterGainPointer_);
        }

        pushLiveParametersToEngine(*engine);

        std::array<render::BlockMidiEvent, 256> blockMidi{};
        std::size_t blockMidiCount = 0;
        for (const auto metadata : midiMessages)
        {
            if (blockMidiCount >= blockMidi.size())
                break;

            const auto msg = metadata.getMessage();
            render::BlockMidiEvent ev{};
            ev.sampleOffset = static_cast<std::size_t>(metadata.samplePosition);
            ev.channel = msg.getChannel() - 1;

            if (msg.isNoteOn())
            {
                ev.type = render::BlockMidiType::NoteOn;
                ev.note = msg.getNoteNumber();
                ev.velocity = msg.getVelocity();
            }
            else if (msg.isNoteOff())
            {
                ev.type = render::BlockMidiType::NoteOff;
                ev.note = msg.getNoteNumber();
                ev.velocity = msg.getVelocity();
            }
            else if (msg.isPitchWheel())
            {
                ev.type = render::BlockMidiType::PitchBend;
                ev.value = msg.getPitchWheelValue();
            }
            else if (msg.isController())
            {
                ev.type = render::BlockMidiType::ControlChange;
                ev.controller = msg.getControllerNumber();
                ev.value = msg.getControllerValue();
            }
            else if (msg.isChannelPressure())
            {
                ev.type = render::BlockMidiType::ChannelPressure;
                ev.value = msg.getChannelPressureValue();
            }
            else if (msg.isAftertouch())
            {
                ev.type = render::BlockMidiType::PolyAftertouch;
                ev.note = msg.getNoteNumber();
                ev.velocity = msg.getAfterTouchValue();
            }
            else
            {
                continue;
            }

            blockMidi[blockMidiCount++] = ev;
        }

        std::sort(blockMidi.begin(), blockMidi.begin() + static_cast<std::ptrdiff_t>(blockMidiCount),
                  [](const render::BlockMidiEvent& a, const render::BlockMidiEvent& b) {
                      return a.sampleOffset < b.sampleOffset;
                  });

        buffer.clear();
        if (buffer.getNumChannels() < 2)
            return;

        core::StereoBlockView view(buffer.getWritePointer(0), buffer.getWritePointer(1),
                                    static_cast<std::size_t>(buffer.getNumSamples()));
        engine->process(view, blockMidi.data(), blockMidiCount);
        if (modWheelParamPointer_ != nullptr)
        {
            const float wheel = engine->getChannelModWheel(0);
            modWheelParamPointer_->store(wheel, std::memory_order_relaxed);
            mirroredModWheel_.store(wheel, std::memory_order_relaxed);
        }
        if (expressionParamPointer_ != nullptr)
        {
            const float expr = engine->getChannelExpression(0);
            expressionParamPointer_->store(expr, std::memory_order_relaxed);
            mirroredExpression_.store(expr, std::memory_order_relaxed);
        }
        scopeAudioTap_.pushStereoBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getNumSamples());
    }

    void PatchworkEightProcessor::pushLiveParametersToEngine(render::Engine& engine) noexcept
    {
        // Drag-to-modulate (docs/UI.md): consume a pending mod-route publish, if the
        // message thread has posted one since the last block -- see
        // publishModRoutesLive()'s doc comment for the double-buffering scheme.
        if (const auto* pendingRoutes = pendingModRoutes_.exchange(nullptr, std::memory_order_acquire))
            engine.setModRoutesLive(*pendingRoutes);

        // Drain APVTS change notifications (also used by updateReportedLatency side paths).
        // Always push every live group: ParamChangeQueue is lossy (256-capacity drops) and
        // focus-panel knob drags must reach the engine even when their group was dropped.
        ParamGroup queued{};
        while (paramChangeQueue_.pop(queued)) {}

        const auto needs = [&](ParamGroup /*group*/) { return true; };

        // Macros -- unchanged from before this pass, see Engine::setMacroValue().
        if (needs(ParamGroup::Macros))
        {
            for (std::size_t i = 0; i < macroParamPointers_.size(); ++i)
                engine.setMacroValue(i, loadF(macroParamPointers_[i]));
        }

        // Filter1 -- field order matches kFilterFieldSpecs / filter::FilterParams.
        if (needs(ParamGroup::Filter))
        {
            filter::FilterParams fp;
            fp.enabled = loadB(filterParamPointers_[0]);
            fp.mode = static_cast<filter::FilterMode>(loadI(filterParamPointers_[1]));
            fp.cutoffHz = loadF(filterParamPointers_[2]);
            fp.resonance = loadF(filterParamPointers_[3]);
            fp.keyTrack = loadF(filterParamPointers_[4]);
            engine.setFilterLive(fp);

            filter::CharacterFilterParams f2;
            f2.enabled = loadB(filter2ParamPointers_[0]);
            f2.cutoffHz = loadF(filter2ParamPointers_[1]);
            f2.resonance = loadF(filter2ParamPointers_[2]);
            f2.drive = loadF(filter2ParamPointers_[3]);
            f2.keyTrack = loadF(filter2ParamPointers_[4]);
            engine.setFilter2Live(f2);
        }

        // 8 LFOs -- field order matches kLfoFieldSpecs / lfo::LfoParams. One
        // setLfoLive() call covers both VOICE scope (per-voice instance) and
        // LAYER/GLOBAL scope (the shared layer-wide tick) -- see Engine::setLfoLive().
        for (std::size_t lfo = 0; lfo < kNumLfos; ++lfo)
        {
            const auto group = static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Lfo0) + lfo);
            if (!needs(group))
                continue;
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
            const auto group = static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Op0) + op);
            if (!needs(group))
                continue;
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
            params.wtBend = loadF(ptrs[32]);
            params.wtAsymmetry = loadF(ptrs[33]);
            params.wtSyncRatio = loadF(ptrs[34]);
            params.wtSyncAmount = loadF(ptrs[35]);
            params.wtFormantShift = loadF(ptrs[36]);
            engine.setOperatorLive(op, params);

            const auto& fptrs = operatorFilterParamPointers_[op];
            filter::FilterParams fp;
            fp.enabled = loadB(fptrs[0]);
            fp.mode = static_cast<filter::FilterMode>(loadI(fptrs[1]));
            fp.cutoffHz = loadF(fptrs[2]);
            fp.resonance = loadF(fptrs[3]);
            fp.keyTrack = loadF(fptrs[4]);
            engine.setOperatorFilterLive(op, fp);
        }

        // 8 envelopes -- field order matches kEnvelopeFieldSpecs / envelope::DahdsrParams.
        for (std::size_t env = 0; env < kNumEnvelopes; ++env)
        {
            const auto group = static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Env0) + env);
            if (!needs(group))
                continue;
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

        if (needs(ParamGroup::LayerGainPan))
        {
            engine.setLayerGainLive(loadF(layerGainPointer_));
            engine.setLayerPanLive(loadF(layerPanPointer_));
        }
        if (needs(ParamGroup::MasterGain))
            engine.setMasterGainLive(loadF(masterGainPointer_));

        // Insert/master FX slots -- field order matches kEffectSlotFieldSpecs /
        // effects::EffectSlotParams's scalar fields. Read-modify-write against the
        // Engine's current value first so NodeDelay/FractalEcho's `nodes[]` and
        // FractalEcho's seeds (not exposed to automation) are preserved rather than
        // stomped with defaults every block.
        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
        {
            const auto group = static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::InsertFx0) + slot);
            if (!needs(group))
                continue;
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
            p.compTransformerCore = loadF(ptrs[51]);
            p.compTransformerBrand = loadF(ptrs[52]);
            p.compTransformerAmount = loadF(ptrs[53]);
            p.limiterCeilingDb = loadF(ptrs[54]);
            p.limiterLookaheadMs = loadF(ptrs[55]);
            p.limiterReleaseMs = loadF(ptrs[56]);
            engine.setInsertEffectLive(slot, p);
        }

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
        {
            const auto group = static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::MasterFx0) + slot);
            if (!needs(group))
                continue;
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
            p.compTransformerCore = loadF(ptrs[51]);
            p.compTransformerBrand = loadF(ptrs[52]);
            p.compTransformerAmount = loadF(ptrs[53]);
            p.limiterCeilingDb = loadF(ptrs[54]);
            p.limiterLookaheadMs = loadF(ptrs[55]);
            p.limiterReleaseMs = loadF(ptrs[56]);
            engine.setMasterEffectLive(slot, p);
        }

        // Arpeggiator -- field order matches kArpFieldSpecs. `.steps` is left
        // default-constructed here; Engine::setArpeggiatorScalarLive() preserves the
        // actually-loaded pattern internally (see its doc comment), so we don't need
        // a read-modify-write here the way effects need one.
        if (needs(ParamGroup::Arp))
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
        return new ui::MurmurRootEditor(*this);
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
        patch::ensureDefaultModWheelRoute(currentPatch_.layerA);
        patch::ensureDefaultExpressionRoute(currentPatch_.layerA);
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
        paramChangeQueue_.pushAllGroups();
        if (onPatchLoaded)
            onPatchLoaded();
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

    GraphEditResult PatchworkEightProcessor::tryCompileAlgorithm(
        const algorithm::AlgorithmGraphDefinition& def) const
    {
        GraphEditResult result;
        algorithm::CompiledAlgorithm compiled;
        result.status = algorithm::AlgorithmGraphCompiler::compile(def, compiled);
        result.ok = result.status == algorithm::CompileStatus::Ok;
        result.detail = algorithm::toString(result.status);
        if (result.ok)
        {
            juce::String order;
            for (std::size_t i = 0; i < compiled.executionOrder.size(); ++i)
            {
                if (i > 0)
                    order += ",";
                order += juce::String(compiled.executionOrder[i].get());
            }
            juce::String outputs;
            for (std::size_t i = 0; i < compiled.outputNodes.size(); ++i)
            {
                if (i > 0)
                    outputs += ",";
                outputs += juce::String(compiled.outputNodes[i].get());
            }
            result.detail = "OK — order " + order + "  outputs: " + outputs;
        }
        return result;
    }

    bool PatchworkEightProcessor::commitAlgorithmGraph(const algorithm::AlgorithmGraphDefinition& def)
    {
        const auto preview = tryCompileAlgorithm(def);
        if (!preview.ok)
            return false;

        currentPatch_.layerA.algorithm = def;
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::commitModMatrix(
        const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& routes)
    {
        currentPatch_.layerA.modRoutes = routes;
        publishModRoutesLive(routes);
        return true;
    }

    float PatchworkEightProcessor::getModWheelValue() const noexcept
    {
        return mirroredModWheel_.load(std::memory_order_relaxed);
    }

    float PatchworkEightProcessor::getExpressionValue() const noexcept
    {
        return mirroredExpression_.load(std::memory_order_relaxed);
    }

    void PatchworkEightProcessor::syncCurrentPatchFromApvts() noexcept
    {
        syncPatchFromAllParameters();
    }

    bool PatchworkEightProcessor::swapEffectSlots(bool masterChain, std::size_t indexA, std::size_t indexB)
    {
        syncCurrentPatchFromApvts();
        if (masterChain)
        {
            if (indexA >= kNumMasterFxSlots || indexB >= kNumMasterFxSlots)
                return false;
            std::swap(currentPatch_.masterEffects[indexA], currentPatch_.masterEffects[indexB]);
        }
        else
        {
            if (indexA >= kNumInsertFxSlots || indexB >= kNumInsertFxSlots)
                return false;
            std::swap(currentPatch_.layerA.insertEffects[indexA], currentPatch_.layerA.insertEffects[indexB]);
        }
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::setOperatorWavetableFile(std::size_t opIndex, const juce::String& filePath)
    {
        if (opIndex >= currentPatch_.layerA.operators.size())
            return false;

        syncCurrentPatchFromApvts();
        juce::File file(filePath);
        const juce::String storedPath = file.existsAsFile() ? file.getFullPathName() : filePath;
        currentPatch_.layerA.operators[opIndex].wavetableId = storedPath.toStdString();

        const auto resolved = pw8::content::resolveWavetablePath(storedPath.toStdString());
        const auto& pathToLoad = resolved.has_value() ? *resolved : storedPath.toStdString();
        auto table = pw8::content::WavetableCache::instance().getOrLoad(pathToLoad);

        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setOperatorWavetableLive(opIndex, std::move(table));

        return table != nullptr;
    }

    ParamGroup PatchworkEightProcessor::paramGroupForId(const juce::String& parameterID) const noexcept
    {
        if (parameterID.startsWith("macro"))
            return ParamGroup::Macros;
        if (parameterID.startsWith(kFilterIdPrefix) || parameterID.startsWith(kFilter2IdPrefix))
            return ParamGroup::Filter;
        if (parameterID.startsWith(kLayerGainId) || parameterID.startsWith(kLayerPanId))
            return ParamGroup::LayerGainPan;
        if (parameterID.startsWith(kMasterGainId))
            return ParamGroup::MasterGain;
        if (parameterID.startsWith(kArpIdPrefix))
            return ParamGroup::Arp;

        for (std::size_t lfo = 0; lfo < kNumLfos; ++lfo)
            if (parameterID.startsWith(lfoParamId(lfo, "")))
                return static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Lfo0) + lfo);

        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            if (parameterID.startsWith(operatorParamId(op, "")) || parameterID.startsWith(operatorFilterParamId(op, "")))
                return static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Op0) + op);
        }

        for (std::size_t env = 0; env < kNumEnvelopes; ++env)
            if (parameterID.startsWith(envelopeParamId(env, "")))
                return static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Env0) + env);

        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            if (parameterID.startsWith(insertFxParamId(slot, "")))
                return static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::InsertFx0) + slot);

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            if (parameterID.startsWith(masterFxParamId(slot, "")))
                return static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::MasterFx0) + slot);

        return ParamGroup::Macros;
    }

    void PatchworkEightProcessor::registerParamListeners()
    {
        for (auto* param : getParameters())
        {
            if (param == nullptr)
                continue;
            if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
                apvts.addParameterListener(withId->paramID, this);
        }
    }

    void PatchworkEightProcessor::parameterChanged(const juce::String& parameterID, float /*newValue*/)
    {
        if (parameterID == kModWheelId || parameterID == kExpressionId)
            return;

        paramChangeQueue_.push(paramGroupForId(parameterID));

        if (parameterID.contains("tapeDelayMs") || parameterID.contains("DelayMs") ||
            parameterID.contains("limiterLookaheadMs") || parameterID.contains("reverbPreDelayMs") ||
            parameterID.contains("EffectType"))
        {
            updateReportedLatency();
        }
    }

    void PatchworkEightProcessor::updateReportedLatency() noexcept
    {
        const int latency = computeTotalEffectLatencySamples(currentPatch_, currentSampleRate_, qualityMode_);
        setLatencySamples(latency);
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

    std::size_t PatchworkEightProcessor::getArpPlayheadStep() const noexcept
    {
        const auto* engine = activeEngine_.load(std::memory_order_acquire);
        return engine != nullptr ? engine->getArpCurrentStepIndex() : 0;
    }

    std::size_t PatchworkEightProcessor::getArpNoteSequenceIndex() const noexcept
    {
        const auto* engine = activeEngine_.load(std::memory_order_acquire);
        return engine != nullptr ? engine->getArpNoteSequenceIndex() : 0;
    }

    void PatchworkEightProcessor::toggleArpStepEnabled(std::size_t stepIndex) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        auto& step = currentPatch_.arpeggiator.steps[stepIndex];
        step.enabled = !step.enabled;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setArpStepLive(stepIndex, step);
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

        const auto& filter2 = currentPatch_.layerA.filter2;
        const std::array<float, kNumFilter2Fields> filter2Values = {
            filter2.enabled ? 1.0f : 0.0f, filter2.cutoffHz, filter2.resonance, filter2.drive, filter2.keyTrack,
        };
        for (std::size_t i = 0; i < kNumFilter2Fields; ++i)
            setParam(juce::String(kFilter2IdPrefix) + kFilter2FieldSpecs[i].idSuffix, filter2Values[i]);

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
                o.wtBend,                             o.wtAsymmetry,
                o.wtSyncRatio,                        o.wtSyncAmount,
                o.wtFormantShift,
            };
            for (std::size_t i = 0; i < kNumOperatorFields; ++i)
                setParam(operatorParamId(op, kOperatorFieldSpecs[i].idSuffix), opValues[i]);

            const auto& ef = o.filter1;
            const std::array<float, kNumOperatorFilterFields> engineFilterValues = {
                ef.enabled ? 1.0f : 0.0f, static_cast<float>(ef.mode), ef.cutoffHz, ef.resonance, ef.keyTrack,
            };
            for (std::size_t i = 0; i < kNumOperatorFilterFields; ++i)
                setParam(operatorFilterParamId(op, kOperatorFilterFieldSpecs[i].idSuffix), engineFilterValues[i]);
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
                p.compTransformerCore,      p.compTransformerBrand, p.compTransformerAmount,
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

        auto& filter2 = currentPatch_.layerA.filter2;
        filter2.enabled = loadB(filter2ParamPointers_[0]);
        filter2.cutoffHz = loadF(filter2ParamPointers_[1]);
        filter2.resonance = loadF(filter2ParamPointers_[2]);
        filter2.drive = loadF(filter2ParamPointers_[3]);
        filter2.keyTrack = loadF(filter2ParamPointers_[4]);

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
            o.wtBend = loadF(ptrs[32]);
            o.wtAsymmetry = loadF(ptrs[33]);
            o.wtSyncRatio = loadF(ptrs[34]);
            o.wtSyncAmount = loadF(ptrs[35]);
            o.wtFormantShift = loadF(ptrs[36]);

            const auto& fptrs = operatorFilterParamPointers_[op];
            auto& ef = o.filter1;
            ef.enabled = loadB(fptrs[0]);
            ef.mode = static_cast<pw8::filter::FilterMode>(loadI(fptrs[1]));
            ef.cutoffHz = loadF(fptrs[2]);
            ef.resonance = loadF(fptrs[3]);
            ef.keyTrack = loadF(fptrs[4]);
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
            p.compTransformerCore = loadF(ptrs[51]);
            p.compTransformerBrand = loadF(ptrs[52]);
            p.compTransformerAmount = loadF(ptrs[53]);
            p.limiterCeilingDb = loadF(ptrs[54]);
            p.limiterLookaheadMs = loadF(ptrs[55]);
            p.limiterReleaseMs = loadF(ptrs[56]);
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
