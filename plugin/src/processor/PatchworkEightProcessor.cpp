// STATUS: PARTIAL -- see PatchworkEightProcessor.h.

#include <algorithm>
#include <array>
#include <cstdlib>

#include "processor/PatchworkEightProcessor.h"
#include "processor/PerformanceMidiMap.hpp"
#include "content/WavetableIndex.h"

#include "pw8/content/ContentPaths.hpp"
#include "pw8/content/WavetableCache.hpp"
#include "pw8/core/AudioBlock.hpp"
#include "pw8/patch/PatchModDefaults.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "pw8/dsp/SidechainFollower.hpp"
#include "pw8/modulation/MorphKoinExecutor.hpp"
#include "pw8/modulation/ModMatrixExecutor.hpp"
#include "pw8/sequencer/ArpeggiatorTypes.hpp"
#include "processor/EffectLatency.hpp"
#include "ui/MurmurRootEditor.h"
#if JucePlugin_Build_Standalone
#include "standalone/StandaloneMcpBridge.h"
#endif

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

        [[nodiscard]] float computeMixGain(bool enabled, bool mute, bool solo, bool anySolo) noexcept
        {
            if (!enabled || mute)
                return 0.0f;
            if (anySolo)
                return solo ? 1.0f : 0.0f;
            return 1.0f;
        }

        template <typename Array>
        void cacheGroup(juce::AudioProcessorValueTreeState& apvts, Array& pointers, const juce::String& prefix,
                         const auto& fieldSpecs)
        {
            for (std::size_t i = 0; i < fieldSpecs.size(); ++i)
                pointers[i] = apvts.getRawParameterValue(prefix + fieldSpecs[i].idSuffix);
        }

        struct SidechainInputView
        {
            const float* left = nullptr;
            const float* right = nullptr;
            bool valid = false;
        };

        /// Logic / JUCE convention: optional main input bus 0, sidechain audio on bus 1.
        /// Legacy MURMUR builds exposed a single "Sidechain" bus at index 0 — keep that fallback.
        [[nodiscard]] SidechainInputView resolveSidechainInput(juce::AudioProcessor& proc,
                                                                juce::AudioBuffer<float>& buffer,
                                                                int numSamples) noexcept
        {
            SidechainInputView out;
            const int busCount = proc.getBusCount(true);
            if (busCount <= 0 || numSamples <= 0)
                return out;

            const auto tryBus = [&](int busIndex) -> bool {
                if (busIndex < 0 || busIndex >= busCount)
                    return false;
                if (proc.getChannelLayoutOfBus(true, busIndex).isDisabled())
                    return false;

                const auto block = proc.getBusBuffer(buffer, true, busIndex);
                if (block.getNumChannels() <= 0 || block.getNumSamples() < numSamples)
                    return false;

                out.left = block.getReadPointer(0);
                out.right = block.getNumChannels() > 1 ? block.getReadPointer(1) : out.left;
                out.valid = true;
                return true;
            };

            for (int i = busCount - 1; i >= 0; --i)
            {
                if (const auto* bus = proc.getBus(true, i);
                    bus != nullptr && bus->getName().containsIgnoreCase("sidechain") && tryBus(i))
                    return out;
            }

            if (tryBus(1))
                return out;
            if (tryBus(0))
                return out;
            return out;
        }
    } // namespace

    PatchworkEightProcessor::PatchworkEightProcessor()
#if JucePlugin_Build_AU || JucePlugin_Build_VST3
        : juce::AudioProcessor(BusesProperties()
                                   .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#else
        : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#endif
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
#ifdef PW8_DEV_CONTENT_ROOT
        pw8::content::addSearchRoot(PW8_DEV_CONTENT_ROOT);
#endif

#if JucePlugin_Build_Standalone
        standaloneMcpBridge_ = std::make_unique<StandaloneMcpBridge>(
            [this](const juce::String& path) { return loadPatchFromFile(path); },
            [this](const juce::String& json) { return loadPatchFromJsonString(json); },
            [this]() {
                juce::DynamicObject::Ptr body(new juce::DynamicObject());
                body->setProperty("ok", true);
                body->setProperty("product", "MURMUR");
                body->setProperty("patch_name", juce::String(currentPatch_.metadata.name));
                body->setProperty("preset_path", currentPresetPath_);
                if (standaloneMcpBridge_ != nullptr)
                    body->setProperty("port", standaloneMcpBridge_->getPort());
                return juce::var(body.get());
            });
#endif
    }

    PatchworkEightProcessor::~PatchworkEightProcessor()
    {
#if JucePlugin_Build_Standalone
        standaloneMcpBridge_.reset();
#endif
    }

    void PatchworkEightProcessor::cacheParameterPointers()
    {
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            macroParamPointers_[i] = apvts.getRawParameterValue(kMacroParameterIds[i]);

        cacheGroup(apvts, filterParamPointers_, kFilterIdPrefix, kFilterFieldSpecs);
        cacheGroup(apvts, filter2ParamPointers_, kFilter2IdPrefix, kFilter2FieldSpecs);
        filterRoutingPointer_ = apvts.getRawParameterValue(kFilterRoutingId);
        for (std::size_t i = 0; i < kNumMasterDynamicsFields; ++i)
            masterDynamicsParamPointers_[i] =
                apvts.getRawParameterValue(juce::String(kMasterDynamicsIdPrefix) + kMasterDynamicsFieldSpecs[i].idSuffix);
        for (std::size_t i = 0; i < kNumGenerativeFields; ++i)
            generativeParamPointers_[i] =
                apvts.getRawParameterValue(juce::String(kGenerativeIdPrefix) + kGenerativeFieldSpecs[i].idSuffix);
        for (std::size_t slot = 0; slot < kNumPeaksUtilitySlots; ++slot)
            for (std::size_t i = 0; i < kNumPeaksUtilitySlotFields; ++i)
                peaksUtilityParamPointers_[slot][i] = apvts.getRawParameterValue(
                    peaksUtilitySlotParamId(slot, kPeaksUtilitySlotFieldSpecs[i].idSuffix));
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

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (std::size_t i = 0; i < kNumOperatorMixFields; ++i)
                operatorMixParamPointers_[op][i] =
                    apvts.getRawParameterValue(operatorMixParamId(op, kOperatorMixFieldSpecs[i].idSuffix));

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
        portamentoPointer_ = apvts.getRawParameterValue(kPortamentoId);
        modWheelParamPointer_ = apvts.getRawParameterValue(kModWheelId);
        expressionParamPointer_ = apvts.getRawParameterValue(kExpressionId);
        morphPositionPointer_ = apvts.getRawParameterValue(kMorphPositionId);
        unisonVoicesPointer_ = apvts.getRawParameterValue(kUnisonVoicesId);
        unisonDetunePointer_ = apvts.getRawParameterValue(kUnisonDetuneId);
        unisonSpreadPointer_ = apvts.getRawParameterValue(kUnisonSpreadId);
        unisonPhaseRandomPointer_ = apvts.getRawParameterValue(kUnisonPhaseRandomId);
        fxRoutingPrePostPointer_ = apvts.getRawParameterValue(kFxRoutingPrePostId);
        fxGlobalBypassPointer_ = apvts.getRawParameterValue(kFxGlobalBypassId);
        fxGlobalWetMixPointer_ = apvts.getRawParameterValue(kFxGlobalWetMixId);
        fxSendAPointer_ = apvts.getRawParameterValue(kFxSendAId);
        fxSendBPointer_ = apvts.getRawParameterValue(kFxSendBId);
    }

    bool PatchworkEightProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
#if JucePlugin_Build_AU || JucePlugin_Build_VST3
        if (layouts.inputBuses.size() > 0)
        {
            const auto sidechain = layouts.getChannelSet(true, 0);
            if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono() &&
                sidechain != juce::AudioChannelSet::stereo())
                return false;
        }
#endif
        return true;
    }

    void PatchworkEightProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
    {
        currentSampleRate_ = sampleRate;
        scopeAudioTap_.reset();
        sidechainFollower_.prepare(sampleRate);
        sidechainScratch_.setSize(2, std::max(1, getBlockSize()), false, false, true);
        for (auto& lfo : modPreviewLfos_)
            lfo.prepare(sampleRate);
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
        const double blockStartMs = juce::Time::getMillisecondCounterHiRes();

        auto* engine = activeEngine_.load(std::memory_order_acquire);
        if (engine == nullptr)
        {
            buffer.clear();
            cpuLoadPercent_.store(0.0f, std::memory_order_relaxed);
            activeVoiceCount_.store(0, std::memory_order_relaxed);
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

        const int numSamples = buffer.getNumSamples();
        const float* engineSidechainL = nullptr;
        const float* engineSidechainR = nullptr;

#if JucePlugin_Build_AU || JucePlugin_Build_VST3
        const SidechainInputView sidechainInput = resolveSidechainInput(*this, buffer, numSamples);
        if (sidechainInput.valid && numSamples > 0)
        {
            if (sidechainScratch_.getNumSamples() < numSamples)
                sidechainScratch_.setSize(2, numSamples, false, false, true);
            sidechainScratch_.copyFrom(0, 0, sidechainInput.left, numSamples);
            if (sidechainInput.right != nullptr && sidechainInput.right != sidechainInput.left)
                sidechainScratch_.copyFrom(1, 0, sidechainInput.right, numSamples);
            else
                sidechainScratch_.copyFrom(1, 0, sidechainInput.left, numSamples);
            engineSidechainL = sidechainScratch_.getReadPointer(0);
            engineSidechainR = sidechainScratch_.getReadPointer(1);
        }
#endif

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
            {
                applyPerformanceCcToApvts(msg.getControllerNumber(), msg.getControllerValue(), macroParamPointers_,
                                          masterGainPointer_);
                noteMidiLearnCc(msg.getControllerNumber(), msg.getControllerValue());
            }
        }

#if JucePlugin_Build_AU || JucePlugin_Build_VST3
        if (engineSidechainL != nullptr)
        {
            sidechainFollower_.processBlock(engineSidechainL, engineSidechainR,
                                            static_cast<std::size_t>(numSamples));
        }
        else
        {
            sidechainFollower_.processBlock(nullptr, nullptr, static_cast<std::size_t>(numSamples));
        }
        const float scLevel = sidechainFollower_.envelope();
        sidechainLevel_.store(scLevel, std::memory_order_relaxed);
        sidechainActive_.store(sidechainFollower_.isActive(), std::memory_order_relaxed);
        engine->setSidechainLevel(scLevel);
#else
        sidechainActive_.store(false, std::memory_order_relaxed);
        engine->setSidechainLevel(0.0f);
#endif

        pushLiveParametersToEngine(*engine);
        updateMorphTimelineModulation(*engine, bpm, numSamples);

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
#if JucePlugin_Build_AU || JucePlugin_Build_VST3
        engine->process(view, blockMidi.data(), blockMidiCount, engineSidechainL, engineSidechainR);
#else
        engine->process(view, blockMidi.data(), blockMidiCount);
#endif
        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            insertCompressorGrDb_[slot].store(engine->getInsertCompressorGainReductionDb(slot),
                                              std::memory_order_relaxed);
        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            masterCompressorGrDb_[slot].store(engine->getMasterCompressorGainReductionDb(slot),
                                              std::memory_order_relaxed);
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

        if (const float* left = buffer.getReadPointer(0))
        {
            const float* right = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : left;
            float colMin = 0.0f;
            float colMax = 0.0f;
            float peakL = 0.0f;
            float peakR = 0.0f;
            float sumL = 0.0f;
            float sumR = 0.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                const float l = left[i];
                const float r = right[i];
                colMin = juce::jmin(colMin, l, r);
                colMax = juce::jmax(colMax, l, r);
                peakL = juce::jmax(peakL, std::abs(l));
                peakR = juce::jmax(peakR, std::abs(r));
                sumL += l * l;
                sumR += r * r;
            }
            const float inv = numSamples > 0 ? 1.0f / static_cast<float>(numSamples) : 0.0f;
            visualizerBus_.pushWaveformColumn(colMin, colMax);
            visualizerBus_.pushLevels(peakL, peakR, std::sqrt(sumL * inv), std::sqrt(sumR * inv));
            const float envLevel = juce::jmax(peakL, peakR);
            visualizerBus_.pushEnvelope(envLevel > 0.001f ? murmur8::EnvelopeSnapshot::Stage::Sustain
                                                          : murmur8::EnvelopeSnapshot::Stage::Idle,
                                        envLevel, envLevel);
        }

        activeVoiceCount_.store(static_cast<int>(engine->countActiveVoices()), std::memory_order_relaxed);

        const double blockEndMs = juce::Time::getMillisecondCounterHiRes();
        const double blockDurationMs =
            static_cast<double>(numSamples) / (currentSampleRate_ > 0.0 ? currentSampleRate_ : 48000.0) * 1000.0;
        if (blockDurationMs > 0.0)
        {
            const float instantLoad =
                static_cast<float>((blockEndMs - blockStartMs) / blockDurationMs * 100.0);
            cpuLoadSmootherState_ = cpuLoadSmootherState_ * 0.9f + instantLoad * 0.1f;
            cpuLoadPercent_.store(juce::jlimit(0.0f, 100.0f, cpuLoadSmootherState_), std::memory_order_relaxed);
        }
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
            fp.modeMorph = loadF(filterParamPointers_[5]);
            engine.setFilterLive(fp);

            filter::CharacterFilterParams f2;
            f2.enabled = loadB(filter2ParamPointers_[0]);
            f2.cutoffHz = loadF(filter2ParamPointers_[1]);
            f2.resonance = loadF(filter2ParamPointers_[2]);
            f2.drive = loadF(filter2ParamPointers_[3]);
            f2.keyTrack = loadF(filter2ParamPointers_[4]);
            f2.cutoffOffsetSemitones = loadF(filter2ParamPointers_[5]);
            engine.setFilter2Live(f2);
            engine.setFilterRoutingLive(loadF(filterRoutingPointer_));
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
        std::array<bool, kNumOperators> mixEnabled{};
        std::array<bool, kNumOperators> mixMute{};
        std::array<bool, kNumOperators> mixSolo{};
        bool anyMixSolo = false;
        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            const auto& mixPtrs = operatorMixParamPointers_[op];
            mixEnabled[op] = loadB(mixPtrs[0]);
            mixMute[op] = loadB(mixPtrs[1]);
            mixSolo[op] = loadB(mixPtrs[2]);
            if (mixSolo[op])
                anyMixSolo = true;
        }

        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            const auto group = static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Op0) + op);
            if (!needs(group))
                continue;
            const auto& ptrs = operatorParamPointers_[op];
            op::OperatorParams params;
            params.engine = algorithm::sanitizeEngineForNode(
                core::NodeId(static_cast<std::uint8_t>(op)), static_cast<algorithm::EngineType>(loadI(ptrs[0])));
            params.classic.waveform = static_cast<oscillator::ClassicWaveform>(loadI(ptrs[1]));
            params.classic.morph = loadF(ptrs[2]);
            params.classic.pulseWidth = loadF(ptrs[3]);
            params.wavetableFramePosition = loadF(ptrs[4]);
            params.frequencyRatio = loadF(ptrs[5]);
            params.fixedFrequencyHz = loadF(ptrs[6]);
            params.keyTrack = loadB(ptrs[7]);
            params.level = loadF(ptrs[8]);
            params.mixEnabled = mixEnabled[op];
            params.mixMute = mixMute[op];
            params.mixSolo = mixSolo[op];
            params.mixGain = computeMixGain(mixEnabled[op], mixMute[op], mixSolo[op], anyMixSolo);
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
            params.wtMorphMode = loadF(ptrs[37]);
            params.externalInputSource = loadF(ptrs[38]);
            engine.setOperatorLive(op, params);

            const auto& fptrs = operatorFilterParamPointers_[op];
            filter::FilterParams fp;
            fp.enabled = loadB(fptrs[0]);
            fp.mode = static_cast<filter::FilterMode>(loadI(fptrs[1]));
            fp.cutoffHz = loadF(fptrs[2]);
            fp.resonance = loadF(fptrs[3]);
            fp.keyTrack = loadF(fptrs[4]);
            fp.modeMorph = filter::modeMorphFromMode(fp.mode);
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
        if (needs(ParamGroup::Unison))
        {
            patch::UnisonSettings uni;
            uni.mode = patch::UnisonMode::Full;
            uni.voices = juce::jlimit(1, static_cast<int>(core::kMaxUnisonVoices),
                                      loadI(unisonVoicesPointer_));
            uni.detuneCents = loadF(unisonDetunePointer_);
            uni.spread = loadF(unisonSpreadPointer_);
            uni.phaseRandom = loadF(unisonPhaseRandomPointer_);
            uni.blend = 1.0f;
            engine.setUnisonLive(uni);
        }
        if (needs(ParamGroup::MasterGain))
            engine.setMasterGainLive(loadF(masterGainPointer_));
        if (needs(ParamGroup::MasterDynamics))
        {
            patch::MasterDynamics md;
            md.enabled = loadB(masterDynamicsParamPointers_[0]);
            md.mode = static_cast<patch::MasterDynamicsMode>(loadI(masterDynamicsParamPointers_[1]));
            md.thresholdDb = loadF(masterDynamicsParamPointers_[2]);
            md.ratio = loadF(masterDynamicsParamPointers_[3]);
            md.attackMs = loadF(masterDynamicsParamPointers_[4]);
            md.releaseMs = loadF(masterDynamicsParamPointers_[5]);
            md.sidechainGain = loadF(masterDynamicsParamPointers_[6]);
            md.vactrolSlewMs = loadF(masterDynamicsParamPointers_[7]);
            md.makeupDb = loadF(masterDynamicsParamPointers_[8]);
            md.mix = loadF(masterDynamicsParamPointers_[9]);
            engine.setMasterDynamicsLive(md);
        }
        if (needs(ParamGroup::Generative))
        {
            modulation::GenerativeParams g{};
            g.dejaVu = loadB(generativeParamPointers_[0]);
            g.seedLocked = loadB(generativeParamPointers_[1]);
            g.clockTRateHz = loadF(generativeParamPointers_[2]);
            g.clockXRateHz = loadF(generativeParamPointers_[3]);
            g.correlation = loadF(generativeParamPointers_[4]);
            for (std::size_t s = 0; s < modulation::kNumGenerativeStreams; ++s)
            {
                const std::size_t base = 5 + s * 3;
                g.streams[s].spread = loadF(generativeParamPointers_[base]);
                g.streams[s].bias = loadF(generativeParamPointers_[base + 1]);
                g.streams[s].lagMs = loadF(generativeParamPointers_[base + 2]);
            }
            for (std::size_t r = 0; r < modulation::kNumGenerativeOutputs; ++r)
                g.outputRouting[r] = static_cast<std::uint8_t>(loadI(generativeParamPointers_[17 + r]));
            engine.setGenerativeLive(g);
        }
        if (needs(ParamGroup::PeaksUtility))
        {
            modulation::PeaksUtilityParams peaks{};
            for (std::size_t slot = 0; slot < kNumPeaksUtilitySlots; ++slot)
            {
                const auto& ptrs = peaksUtilityParamPointers_[slot];
                auto& p = peaks.slots[slot];
                p.enabled = loadB(ptrs[0]);
                p.mode = static_cast<modulation::PeaksUtilityMode>(loadI(ptrs[1]));
                p.attackMs = loadF(ptrs[2]);
                p.releaseMs = loadF(ptrs[3]);
                p.lfoRateHz = loadF(ptrs[4]);
                p.lfoDepth = loadF(ptrs[5]);
            }
            engine.setUtilityPeaksLive(peaks);
        }
        if (needs(ParamGroup::Portamento))
            engine.setPortamentoLive(loadF(portamentoPointer_));

        if (needs(ParamGroup::FxRouting))
        {
            engine.setFxSendLevels(loadF(fxSendAPointer_), loadF(fxSendBPointer_),
                                   loadB(fxRoutingPrePostPointer_));
        }

        const float fxGlobalWet = loadF(fxGlobalWetMixPointer_);
        const bool fxGlobalBypass = loadB(fxGlobalBypassPointer_);
        const auto effectiveFxMix = [&](float slotMix) {
            if (fxGlobalBypass)
                return 0.0f;
            return juce::jlimit(0.0f, 1.0f, slotMix * fxGlobalWet);
        };

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
            std::array<float, kNumEffectSlotFields> fxValues{};
            for (std::size_t i = 0; i < kNumEffectSlotFields; ++i)
                fxValues[i] = loadF(ptrs[i]);
            applyEffectSlotFieldValues(p, fxValues);
            p.mix = effectiveFxMix(p.mix);
            engine.setInsertEffectLive(slot, p);
        }

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
        {
            const auto group = static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::MasterFx0) + slot);
            if (!needs(group))
                continue;
            const auto& ptrs = masterFxParamPointers_[slot];
            effects::EffectSlotParams p = engine.getMasterEffectParams(slot);
            std::array<float, kNumEffectSlotFields> fxValues{};
            for (std::size_t i = 0; i < kNumEffectSlotFields; ++i)
                fxValues[i] = loadF(ptrs[i]);
            applyEffectSlotFieldValues(p, fxValues);
            p.mix = effectiveFxMix(p.mix);
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
            ap.swing = loadF(arpParamPointers_[7]);
            ap.latch = loadB(arpParamPointers_[8]);
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
            loadPatch(result.patch, PatchReloadIntent::ExternalLoad);
    }

    void PatchworkEightProcessor::setPatchDirty(bool dirty) noexcept
    {
        if (patchDirty_ == dirty)
            return;
        patchDirty_ = dirty;
        if (onPatchDirtyChanged)
            onPatchDirtyChanged();
    }

    juce::File PatchworkEightProcessor::userPresetsDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("MURMUR")
            .getChildFile("Presets")
            .getChildFile("user");
    }

    bool PatchworkEightProcessor::isUserPresetPath(const juce::String& filePath)
    {
        if (filePath.isEmpty())
            return false;

        const juce::File file(filePath);
        const juce::File userDir = userPresetsDirectory();
        return file == userDir || file.isAChildOf(userDir);
    }

    bool PatchworkEightProcessor::saveCurrentPatchToFile(const juce::String& filePath)
    {
        if (filePath.isEmpty())
            return false;

        syncPatchFromAllParameters();
        const auto json = patch::savePatchToJson(currentPatch_);
        juce::File file(filePath);
        if (!file.getParentDirectory().createDirectory())
            return false;
        if (!file.replaceWithText(juce::String(json)))
            return false;

        currentPresetPath_ = file.getFullPathName();
        setPatchDirty(false);
        if (onPatchMetadataChanged)
            onPatchMetadataChanged();
        return true;
    }

    bool PatchworkEightProcessor::loadPatch(const patch::Patch& newPatch, PatchReloadIntent intent)
    {
        currentPatch_ = newPatch;
        patch::ensureDefaultModWheelRoute(currentPatch_.layerA);
        patch::ensureDefaultExpressionRoute(currentPatch_.layerA);
        patch::ensureMinimumMacroKoinRoutes(currentPatch_.layerA);
        if (currentPatch_.morphKoin.keyframes.size() >= 2)
        {
            const float pos = currentPatch_.morphKoin.position > 0.0f ? currentPatch_.morphKoin.position
                                                                        : currentPatch_.morphKoin.defaultPosition;
            modulation::applyMorphKoin(currentPatch_, pos);
        }
        syncAllParametersFromPatch();
        ensureDefaultWavetablesForPatch();
        auto fresh = std::make_unique<render::Engine>();
        fresh->prepare(getSampleRate() > 0.0 ? getSampleRate() : 48000.0);
        const bool ok = fresh->loadPatch(currentPatch_);
        publishEngine(std::move(fresh));
        lastMorphTimelinePos_ = -1.0f;
        morphKeyframeCrossedIndex_.store(-1, std::memory_order_relaxed);
        morphKeyframeCrossedFlash_.store(false, std::memory_order_relaxed);
        pendingModRoutes_.store(nullptr, std::memory_order_release);
        paramChangeQueue_.pushAllGroups();
        setPatchDirty(intent == PatchReloadIntent::UserEdit);
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
        return loadPatch(result.patch, PatchReloadIntent::ExternalLoad);
    }

    bool PatchworkEightProcessor::loadPatchFromJsonString(const juce::String& jsonUtf8)
    {
        const auto result = patch::loadPatchFromJson(jsonUtf8.toStdString());
        if (!result.ok)
            return false;

        currentPresetPath_ = {};
        return loadPatch(result.patch, PatchReloadIntent::ExternalLoad);
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

        syncCurrentPatchFromApvts();
        currentPatch_.layerA.algorithm = def;
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::commitModMatrix(
        const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& routes)
    {
        currentPatch_.layerA.modRoutes = routes;
        publishModRoutesLive(routes);
        setPatchDirty(true);
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

    void PatchworkEightProcessor::noteMidiLearnCc(int controller, int value7) noexcept
    {
        if (controller < 0 || controller > 127 || value7 < 0 || value7 > 127)
            return;

        const auto packed = (static_cast<std::uint32_t>(controller + 1) << 16)
                            | static_cast<std::uint32_t>(value7);
        midiLearnCcEvent_.store(packed, std::memory_order_relaxed);
    }

    bool PatchworkEightProcessor::consumeMidiLearnCc(int& controller, int& value7) noexcept
    {
        const auto packed = midiLearnCcEvent_.exchange(0, std::memory_order_acq_rel);
        if (packed == 0)
            return false;

        controller = static_cast<int>((packed >> 16) - 1);
        value7 = static_cast<int>(packed & 0xFFFFu);
        return controller >= 0 && controller <= 127 && value7 >= 0 && value7 <= 127;
    }

    float PatchworkEightProcessor::getHostBpm() const noexcept
    {
        float bpm = 120.0f;
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition(); position.hasValue())
            {
                if (const auto hostBpm = position->getBpm())
                    bpm = static_cast<float>(*hostBpm);
            }
        }
        return bpm;
    }

    bool PatchworkEightProcessor::isSustainPedalHeld(int channel) const noexcept
    {
        const auto* engine = activeEngine_.load(std::memory_order_acquire);
        return engine != nullptr ? engine->isSustainPedalHeld(channel) : false;
    }

    modulation::ModSourceValues PatchworkEightProcessor::buildModPreviewSources(float bpm) noexcept
    {
        modulation::ModSourceValues sources;
        sources.modWheel = getModWheelValue();
        sources.expression = getExpressionValue();
        sources.sidechain = getSidechainLevel();
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
        {
            if (macroParamPointers_[i] != nullptr)
                sources.macros[i] = macroParamPointers_[i]->load(std::memory_order_relaxed);
        }
        for (std::size_t i = 0; i < modPreviewLfos_.size() && i < currentPatch_.layerA.lfos.size(); ++i)
            sources.layerLfos[i] =
                modPreviewLfos_[i].renderSample(currentPatch_.layerA.lfos[i], bpm > 0.0f ? bpm : 120.0f);
        return sources;
    }

    bool PatchworkEightProcessor::hasModRouteTo(modulation::ModDestination destination,
                                                 std::uint8_t targetIndex) const noexcept
    {
        for (const auto& route : currentPatch_.layerA.modRoutes)
        {
            if (route.isActive() && route.destination == destination && route.targetIndex == targetIndex)
                return true;
        }
        return false;
    }

    bool PatchworkEightProcessor::macroHasActiveRoutes(std::size_t macroIndex) const noexcept
    {
        if (macroIndex >= 8)
            return false;
        const auto source = static_cast<modulation::ModSource>(
            static_cast<int>(modulation::ModSource::Macro1) + static_cast<int>(macroIndex));
        for (const auto& route : currentPatch_.layerA.modRoutes)
        {
            if (route.isActive() && route.source == source)
                return true;
        }
        return false;
    }

    void PatchworkEightProcessor::syncCurrentPatchFromApvts() noexcept
    {
        syncPatchFromAllParameters();
    }

    void PatchworkEightProcessor::notifyPatchMetadataChanged() noexcept
    {
        if (onPatchMetadataChanged)
            onPatchMetadataChanged();
    }

    void PatchworkEightProcessor::panicAllNotes() noexcept
    {
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->allSoundOff();
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

    bool PatchworkEightProcessor::applyFxProcessOrderFromChipDisplay(
        const std::array<std::size_t, 12>& chipDisplayOrder)
    {
        static constexpr int kEngineSlotByChip[] = {0, 0, 1, 2, 2, 3, 4, 2, 5, 6, 6, 2};

        patch::FxProcessOrder order{};
        std::array<bool, effects::kNumLayerInsertSlots> insertSeen{};
        std::size_t insertCount = 0;
        std::array<bool, effects::kNumMasterSlots> masterSeen{};
        std::size_t masterCount = 0;

        for (const std::size_t chip : chipDisplayOrder)
        {
            if (chip >= 12 || chip == 0)
                continue;
            const int engineSlot = kEngineSlotByChip[chip];
            if (engineSlot < 0)
                continue;
            if (engineSlot < static_cast<int>(effects::kNumLayerInsertSlots))
            {
                const std::size_t insertIndex = static_cast<std::size_t>(engineSlot);
                if (!insertSeen[insertIndex])
                {
                    insertSeen[insertIndex] = true;
                    order.insert[insertCount++] = static_cast<std::uint8_t>(insertIndex);
                }
            }
            else
            {
                const std::size_t masterIndex =
                    static_cast<std::size_t>(engineSlot - static_cast<int>(effects::kNumLayerInsertSlots));
                if (masterIndex < effects::kNumMasterSlots && !masterSeen[masterIndex])
                {
                    masterSeen[masterIndex] = true;
                    order.master[masterCount++] = static_cast<std::uint8_t>(masterIndex);
                }
            }
        }

        for (std::uint8_t slot = 0; slot < effects::kNumLayerInsertSlots; ++slot)
        {
            if (!insertSeen[slot])
                order.insert[insertCount++] = slot;
        }
        for (std::uint8_t slot = 0; slot < effects::kNumMasterSlots; ++slot)
        {
            if (!masterSeen[slot])
                order.master[masterCount++] = slot;
        }

        syncCurrentPatchFromApvts();
        currentPatch_.fxProcessOrder = order;
        return loadPatch(currentPatch_);
    }

    namespace
    {
        patch::MorphKoinKeyframe captureMorphKeyframeFromPatch(const patch::Patch& patch)
        {
            patch::MorphKoinKeyframe kf;
            kf.hasMacroValues = true;
            for (std::size_t i = 0; i < patch.macros.size(); ++i)
                kf.macroValues[i] = patch.macros[i].value;

            kf.paramOverrides["layerA.filter1.cutoffHz"] = {patch.layerA.filter1.cutoffHz, "", ""};
            kf.paramOverrides["layerA.filter1.resonance"] = {patch.layerA.filter1.resonance, "", ""};
            return kf;
        }
    } // namespace

    bool PatchworkEightProcessor::addMorphKeyframeAt(float position, const std::string& name)
    {
        syncCurrentPatchFromApvts();
        auto& mk = currentPatch_.morphKoin;
        if (mk.keyframes.size() >= core::kMaxMorphKeyframes)
            return false;

        auto kf = captureMorphKeyframeFromPatch(currentPatch_);
        kf.name = name.empty() ? ("Keyframe " + std::to_string(mk.keyframes.size() + 1)) : name;
        kf.position = juce::jlimit(0.0f, 1.0f, position);
        mk.keyframes.push_back(std::move(kf));
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::removeMorphKeyframe(const std::size_t index)
    {
        syncCurrentPatchFromApvts();
        auto& mk = currentPatch_.morphKoin;
        if (index >= mk.keyframes.size())
            return false;

        mk.keyframes.erase(mk.keyframes.begin() + static_cast<std::ptrdiff_t>(index));
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::recaptureMorphKeyframe(const std::size_t index)
    {
        syncCurrentPatchFromApvts();
        auto& mk = currentPatch_.morphKoin;
        if (index >= mk.keyframes.size())
            return false;

        auto kf = captureMorphKeyframeFromPatch(currentPatch_);
        kf.name = mk.keyframes[index].name;
        kf.position = mk.keyframes[index].position;
        kf.color = mk.keyframes[index].color;
        kf.paramOverrides = mk.keyframes[index].paramOverrides;
        mk.keyframes[index] = std::move(kf);
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::setMorphKeyframeColor(const std::size_t index, const std::string& color)
    {
        syncCurrentPatchFromApvts();
        auto& mk = currentPatch_.morphKoin;
        if (index >= mk.keyframes.size())
            return false;

        mk.keyframes[index].color = color;
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::setMorphKeyframeParamEasing(const std::size_t index, const std::string& path,
                                                              const std::string& easing)
    {
        syncCurrentPatchFromApvts();
        auto& mk = currentPatch_.morphKoin;
        if (index >= mk.keyframes.size())
            return false;

        auto it = mk.keyframes[index].paramOverrides.find(path);
        if (it == mk.keyframes[index].paramOverrides.end())
            return false;

        it->second.easing = easing;
        return loadPatch(currentPatch_);
    }

    bool PatchworkEightProcessor::setMorphKoinSettings(const std::string& curve, const bool wrap,
                                                       const std::string& autoplaySource,
                                                       const bool morphDissemination)
    {
        syncCurrentPatchFromApvts();
        currentPatch_.morphKoin.curve = curve;
        currentPatch_.morphKoin.wrap = wrap;
        currentPatch_.morphKoin.autoplaySource = autoplaySource;
        currentPatch_.voiceSettings.morphDissemination = morphDissemination;
        return loadPatch(currentPatch_);
    }

    pw8::envelope::SegmentEnvelopeChain PatchworkEightProcessor::getSegmentEnvelopeChain(
        const std::size_t envIndex) const noexcept
    {
        if (envIndex >= core::kNumEnvelopesPerLayer)
            return {};
        return currentPatch_.layerA.segmentEnvelopeChains[envIndex];
    }

    bool PatchworkEightProcessor::setSegmentEnvelopeChain(const std::size_t envIndex,
                                                           const pw8::envelope::SegmentEnvelopeChain& chain)
    {
        if (envIndex >= core::kNumEnvelopesPerLayer)
            return false;
        syncCurrentPatchFromApvts();
        currentPatch_.layerA.segmentEnvelopeChains[envIndex] = chain;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setSegmentEnvelopeChainLive(envIndex, chain);
        return true;
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

    bool PatchworkEightProcessor::ensureOperatorWavetableLoaded(std::size_t opIndex)
    {
        if (opIndex >= currentPatch_.layerA.operators.size())
            return false;

        syncCurrentPatchFromApvts();

        const auto engineParamId = operatorParamId(opIndex, "Engine");
        int engineOrdinal = 0;
        if (auto* raw = apvts.getRawParameterValue(engineParamId))
            engineOrdinal = static_cast<int>(raw->load() + 0.5f);

        const bool needsTable = engineOrdinal == static_cast<int>(algorithm::EngineType::Wavetable)
                                || engineOrdinal == static_cast<int>(algorithm::EngineType::Granular);
        const auto& op = currentPatch_.layerA.operators[opIndex];
        if (!needsTable || !op.wavetableId.empty())
            return !op.wavetableId.empty();

        content::WavetableIndex index;
        index.rescan();
        if (index.allEntries().isEmpty())
            return false;

        return setOperatorWavetableFile(opIndex, index.allEntries().getFirst().absolutePath);
    }

    void PatchworkEightProcessor::ensureDefaultWavetablesForPatch()
    {
        for (std::size_t i = 0; i < currentPatch_.layerA.operators.size(); ++i)
            ensureOperatorWavetableLoaded(i);
    }

    bool PatchworkEightProcessor::morphTimelineIsModulated() const noexcept
    {
        if (currentPatch_.morphKoin.keyframes.size() < 2)
            return false;

        const auto& autoplay = currentPatch_.morphKoin.autoplaySource;
        if (!autoplay.empty() && autoplay != "none")
            return true;

        for (const auto& route : currentPatch_.layerA.modRoutes)
        {
            if (!route.isActive() || route.destination != modulation::ModDestination::MorphPosition)
                continue;
            if (route.scope != modulation::ModScope::Voice)
                return true;
        }
        return false;
    }

    void PatchworkEightProcessor::updateMorphTimelineModulation(render::Engine& engine, float bpm,
                                                                int numSamples) noexcept
    {
        if (!morphTimelineIsModulated())
            return;

        modulation::ModSourceValues sources;
        sources.modWheel = engine.getChannelModWheel(0);
        sources.expression = engine.getChannelExpression(0);
        sources.sidechain = sidechainLevel_.load(std::memory_order_relaxed);
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            sources.macros[i] = loadF(macroParamPointers_[i]);

        const auto& lfoParams = currentPatch_.layerA.lfos;
        for (std::size_t i = 0; i < modPreviewLfos_.size() && i < lfoParams.size(); ++i)
        {
            float value = sources.layerLfos[i];
            for (int sample = 0; sample < numSamples; ++sample)
                value = modPreviewLfos_[i].renderSample(lfoParams[i], bpm > 0.0f ? bpm : 120.0f);
            sources.layerLfos[i] = value;
        }

        float position = morphPositionPointer_ != nullptr ? loadF(morphPositionPointer_) : 0.0f;
        const float autoplay =
            modulation::resolveAutoplayMorphPosition(currentPatch_.morphKoin.autoplaySource, sources);
        if (autoplay >= 0.0f)
            position = autoplay;

        position += modulation::ModMatrixExecutor::computeMorphPositionOffset(engine.getModRoutes(), sources);

        if (currentPatch_.morphKoin.wrap)
            position = std::fmod(position, 1.0f);
        position = juce::jlimit(0.0f, 1.0f, position);

        if (lastMorphTimelinePos_ >= 0.0f)
        {
            const int crossed = modulation::detectMorphKeyframeCrossing(currentPatch_.morphKoin, lastMorphTimelinePos_,
                                                                        position);
            if (crossed >= 0)
            {
                morphKeyframeCrossedIndex_.store(crossed, std::memory_order_relaxed);
                morphKeyframeCrossedFlash_.store(true, std::memory_order_relaxed);
            }
        }
        lastMorphTimelinePos_ = position;

        engine.applyMorphPositionLive(position);
        currentPatch_.morphKoin.position = position;
    }

    void PatchworkEightProcessor::applyMorphFromPosition(float position) noexcept
    {
        if (currentPatch_.morphKoin.keyframes.size() < 2)
            return;

        modulation::applyMorphKoin(currentPatch_, position);
        currentPatch_.morphKoin.position = position;

        auto setParam = [this](const juce::String& id, float value) {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            setParam(kMacroParameterIds[i], currentPatch_.macros[i].value);

        setParam(juce::String(kFilterIdPrefix) + "CutoffHz", currentPatch_.layerA.filter1.cutoffHz);
        setParam(juce::String(kFilterIdPrefix) + "Resonance", currentPatch_.layerA.filter1.resonance);

        auto syncFx = [&](const effects::EffectSlotParams& p, const juce::String& id) {
            const auto values = effectSlotFieldValues(p);
            for (std::size_t i = 0; i < kNumEffectSlotFields; ++i)
                setParam(id + kEffectSlotFieldSpecs[i].idSuffix, values[i]);
        };

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            syncFx(currentPatch_.masterEffects[slot], juce::String("masterFx") + juce::String(static_cast<int>(slot)));
    }

    ParamGroup PatchworkEightProcessor::paramGroupForId(const juce::String& parameterID) const noexcept
    {
        if (parameterID.startsWith("macro"))
            return ParamGroup::Macros;
        if (parameterID.startsWith(kFilterIdPrefix) || parameterID.startsWith(kFilter2IdPrefix) ||
            parameterID == kFilterRoutingId)
            return ParamGroup::Filter;
        if (parameterID.startsWith(kLayerGainId) || parameterID.startsWith(kLayerPanId))
            return ParamGroup::LayerGainPan;
        if (parameterID.startsWith(kMasterGainId))
            return ParamGroup::MasterGain;
        if (parameterID.startsWith(kMasterDynamicsIdPrefix))
            return ParamGroup::MasterDynamics;
        if (parameterID.startsWith(kGenerativeIdPrefix))
            return ParamGroup::Generative;
        if (parameterID.startsWith(kPeaksUtilityIdPrefix))
            return ParamGroup::PeaksUtility;
        if (parameterID.startsWith(kPortamentoId))
            return ParamGroup::Portamento;
        if (parameterID.startsWith(kUnisonVoicesId) || parameterID.startsWith(kUnisonDetuneId) ||
            parameterID.startsWith(kUnisonSpreadId) || parameterID.startsWith(kUnisonPhaseRandomId))
            return ParamGroup::Unison;
        if (parameterID.startsWith(kFxRoutingPrePostId) || parameterID.startsWith(kFxGlobalBypassId) ||
            parameterID.startsWith(kFxGlobalWetMixId) || parameterID.startsWith(kFxSendAId) ||
            parameterID.startsWith(kFxSendBId))
            return ParamGroup::FxRouting;
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

        if (parameterID == kMorphPositionId)
        {
            if (!morphTimelineIsModulated())
                applyMorphFromPosition(apvts.getRawParameterValue(kMorphPositionId)->load());
            paramChangeQueue_.pushAllGroups();
            return;
        }

        paramChangeQueue_.push(paramGroupForId(parameterID));

        if (parameterID.startsWith(kFxRoutingPrePostId) || parameterID.startsWith(kFxGlobalBypassId) ||
            parameterID.startsWith(kFxGlobalWetMixId))
        {
            for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
                paramChangeQueue_.push(static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::InsertFx0) + slot));
            for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
                paramChangeQueue_.push(static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::MasterFx0) + slot));
        }

        if (parameterID.contains("MixEnabled") || parameterID.contains("MixMute") ||
            parameterID.contains("MixSolo"))
        {
            for (std::size_t op = 0; op < kNumOperators; ++op)
                paramChangeQueue_.push(static_cast<ParamGroup>(static_cast<std::uint16_t>(ParamGroup::Op0) + op));
        }

        if (parameterID.contains("tapeDelayMs") || parameterID.contains("DelayMs") ||
            parameterID.contains("limiterLookaheadMs") || parameterID.contains("reverbPreDelayMs") ||
            parameterID.contains("EffectType"))
        {
            updateReportedLatency();
        }

        setPatchDirty(true);
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
        setPatchDirty(true);
    }

    void PatchworkEightProcessor::setModRouteCurveLive(modulation::ModSource source,
                                                        modulation::ModDestination destination,
                                                        std::uint8_t targetIndex, modulation::ModCurve curve)
    {
        auto routes = currentPatch_.layerA.modRoutes;
        for (auto& route : routes)
        {
            if (route.source == source && route.destination == destination && route.targetIndex == targetIndex)
            {
                route.curve = curve;
                currentPatch_.layerA.modRoutes = routes;
                publishModRoutesLive(routes);
                setPatchDirty(true);
                return;
            }
        }
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
        setPatchDirty(true);
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

    void PatchworkEightProcessor::toggleArpStepAccent(std::size_t stepIndex) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        auto& step = currentPatch_.arpeggiator.steps[stepIndex];
        step.accent = !step.accent;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setArpStepLive(stepIndex, step);
    }

    void PatchworkEightProcessor::cycleArpStepRatchet(std::size_t stepIndex) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        auto& step = currentPatch_.arpeggiator.steps[stepIndex];
        step.ratchetCount = step.ratchetCount >= sequencer::kMaxRatchet ? 1 : step.ratchetCount + 1;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setArpStepLive(stepIndex, step);
    }

    void PatchworkEightProcessor::toggleArpStepTie(std::size_t stepIndex) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        auto& step = currentPatch_.arpeggiator.steps[stepIndex];
        step.tie = !step.tie;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setArpStepLive(stepIndex, step);
    }

    void PatchworkEightProcessor::cycleArpStepProbability(std::size_t stepIndex) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        auto& step = currentPatch_.arpeggiator.steps[stepIndex];
        static constexpr float kLevels[] = {1.0f, 0.75f, 0.5f, 0.25f};
        int idx = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (std::abs(step.probability - kLevels[i]) < 0.06f)
            {
                idx = (i + 1) % 4;
                break;
            }
        }
        step.probability = kLevels[idx];
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setArpStepLive(stepIndex, step);
    }

    void PatchworkEightProcessor::cycleArpStepOctaveOffset(std::size_t stepIndex) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        auto& step = currentPatch_.arpeggiator.steps[stepIndex];
        step.octaveOffset = step.octaveOffset >= 2 ? -2 : step.octaveOffset + 1;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setArpStepLive(stepIndex, step);
    }

    void PatchworkEightProcessor::setArpStepVelocity(std::size_t stepIndex, float velocity01) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        auto& step = currentPatch_.arpeggiator.steps[stepIndex];
        step.velocityScale = juce::jlimit(0.05f, 1.0f, velocity01);
        step.enabled = true;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->setArpStepLive(stepIndex, step);
    }

    void PatchworkEightProcessor::setAllArpStepsGate(float gate01) noexcept
    {
        const float gate = juce::jlimit(0.05f, 1.0f, gate01);
        const auto numSteps = currentPatch_.arpeggiator.numSteps;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
        {
            for (std::size_t i = 0; i < numSteps && i < sequencer::kMaxArpSteps; ++i)
            {
                currentPatch_.arpeggiator.steps[i].gate = gate;
                engine->setArpStepLive(i, currentPatch_.arpeggiator.steps[i]);
            }
        }
        else
        {
            for (std::size_t i = 0; i < numSteps && i < sequencer::kMaxArpSteps; ++i)
                currentPatch_.arpeggiator.steps[i].gate = gate;
        }
    }

    void PatchworkEightProcessor::setAllArpStepsEnabled(bool enabled) noexcept
    {
        const auto numSteps = currentPatch_.arpeggiator.numSteps;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
        {
            for (std::size_t i = 0; i < numSteps && i < sequencer::kMaxArpSteps; ++i)
            {
                currentPatch_.arpeggiator.steps[i].enabled = enabled;
                engine->setArpStepLive(i, currentPatch_.arpeggiator.steps[i]);
            }
        }
        else
        {
            for (std::size_t i = 0; i < numSteps && i < sequencer::kMaxArpSteps; ++i)
                currentPatch_.arpeggiator.steps[i].enabled = enabled;
        }
    }

    void PatchworkEightProcessor::randomizeArpPattern(float activeDensity01) noexcept
    {
        const auto numSteps = currentPatch_.arpeggiator.numSteps;
        const float density = juce::jlimit(0.15f, 1.0f, activeDensity01);
        juce::Random rng(static_cast<int>(juce::Time::getMillisecondCounter()));

        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
        {
            for (std::size_t i = 0; i < numSteps && i < sequencer::kMaxArpSteps; ++i)
            {
                auto& step = currentPatch_.arpeggiator.steps[i];
                step.enabled = rng.nextFloat() <= density;
                step.accent = step.enabled && rng.nextFloat() < 0.22f;
                step.ratchetCount = rng.nextFloat() < 0.12f ? rng.nextInt({2, 4}) : 1;
                step.velocityScale = 0.45f + rng.nextFloat() * 0.55f;
                step.probability = rng.nextFloat() < 0.15f ? 0.5f : 1.0f;
                step.tie = false;
                step.octaveOffset = rng.nextFloat() < 0.08f ? rng.nextInt({-1, 1}) : 0;
                engine->setArpStepLive(i, step);
            }
        }
        else
        {
            for (std::size_t i = 0; i < numSteps && i < sequencer::kMaxArpSteps; ++i)
            {
                auto& step = currentPatch_.arpeggiator.steps[i];
                step.enabled = rng.nextFloat() <= density;
                step.accent = step.enabled && rng.nextFloat() < 0.22f;
                step.ratchetCount = rng.nextFloat() < 0.12f ? rng.nextInt({2, 4}) : 1;
                step.velocityScale = 0.45f + rng.nextFloat() * 0.55f;
                step.probability = rng.nextFloat() < 0.15f ? 0.5f : 1.0f;
                step.tie = false;
                step.octaveOffset = rng.nextFloat() < 0.08f ? rng.nextInt({-1, 1}) : 0;
            }
        }
    }

    std::size_t PatchworkEightProcessor::getArpNumSteps() const noexcept
    {
        return static_cast<std::size_t>(std::max(1, loadI(arpParamPointers_[6])));
    }

    void PatchworkEightProcessor::setArpNumSteps(int steps) noexcept
    {
        const int clamped = juce::jlimit(1, static_cast<int>(sequencer::kMaxArpSteps), steps);
        const auto oldCount = currentPatch_.arpeggiator.numSteps;
        const auto newCount = static_cast<std::size_t>(clamped);
        currentPatch_.arpeggiator.numSteps = newCount;

        if (newCount > oldCount)
        {
            for (std::size_t i = oldCount; i < newCount; ++i)
            {
                auto& step = currentPatch_.arpeggiator.steps[i];
                step.enabled = true;
                step.velocityScale = 0.85f;
                if (auto* engine = activeEngine_.load(std::memory_order_acquire))
                    engine->setArpStepLive(i, step);
            }
        }

        if (auto* param = apvts.getParameter(juce::String(kArpIdPrefix) + "NumSteps"))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(clamped)));

        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
        {
            sequencer::ArpeggiatorParams ap;
            ap.enabled = loadB(arpParamPointers_[0]);
            ap.mode = static_cast<sequencer::ArpMode>(loadI(arpParamPointers_[1]));
            ap.rateMode = static_cast<sequencer::ArpRateMode>(loadI(arpParamPointers_[2]));
            ap.rateHz = loadF(arpParamPointers_[3]);
            ap.syncDivisionIndex = loadI(arpParamPointers_[4]);
            ap.octaveRange = loadI(arpParamPointers_[5]);
            ap.numSteps = newCount;
            ap.swing = loadF(arpParamPointers_[7]);
            ap.latch = loadB(arpParamPointers_[8]);
            engine->setArpeggiatorScalarLive(ap);
        }

        setPatchDirty(true);
    }

    void PatchworkEightProcessor::applyEuclideanArpPattern(int pulses) noexcept
    {
        const auto numSteps = getArpNumSteps();
        currentPatch_.arpeggiator.numSteps = numSteps;
        if (numSteps == 0)
            return;

        const int steps = static_cast<int>(numSteps);
        pulses = juce::jlimit(1, steps, pulses);

        int bucket = 0;
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
        {
            for (int i = 0; i < steps; ++i)
            {
                bucket += pulses;
                const bool on = bucket >= steps;
                if (on)
                    bucket -= steps;
                auto& step = currentPatch_.arpeggiator.steps[static_cast<std::size_t>(i)];
                step.enabled = on;
                engine->setArpStepLive(static_cast<std::size_t>(i), step);
            }
        }
        else
        {
            for (int i = 0; i < steps; ++i)
            {
                bucket += pulses;
                const bool on = bucket >= steps;
                if (on)
                    bucket -= steps;
                currentPatch_.arpeggiator.steps[static_cast<std::size_t>(i)].enabled = on;
            }
        }
    }

    bool PatchworkEightProcessor::getHostIsPlaying() const noexcept
    {
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition(); position.hasValue())
                return position->getIsPlaying();
        }
        return false;
    }

    void PatchworkEightProcessor::syncAllParametersFromPatch()
    {
        auto setParam = [this](const juce::String& id, float value) {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            setParam(kMacroParameterIds[i], currentPatch_.macros[i].value);

        if (!currentPatch_.morphKoin.keyframes.empty())
            setParam(kMorphPositionId, currentPatch_.morphKoin.position);

        const auto& filter = currentPatch_.layerA.filter1;
        const std::array<float, kNumFilterFields> filterValues = {
            filter.enabled ? 1.0f : 0.0f, static_cast<float>(filter.mode), filter.cutoffHz, filter.resonance,
            filter.keyTrack, filter.modeMorph,
        };
        for (std::size_t i = 0; i < kNumFilterFields; ++i)
            setParam(juce::String(kFilterIdPrefix) + kFilterFieldSpecs[i].idSuffix, filterValues[i]);

        setParam(kFilterRoutingId, currentPatch_.layerA.filterRouting);

        const auto& filter2 = currentPatch_.layerA.filter2;
        const std::array<float, kNumFilter2Fields> filter2Values = {
            filter2.enabled ? 1.0f : 0.0f, filter2.cutoffHz, filter2.resonance, filter2.drive, filter2.keyTrack,
            filter2.cutoffOffsetSemitones,
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
                o.wtFormantShift,                     o.wtMorphMode,
                o.externalInputSource,
            };
            for (std::size_t i = 0; i < kNumOperatorFields; ++i)
                setParam(operatorParamId(op, kOperatorFieldSpecs[i].idSuffix), opValues[i]);

            const auto& ef = o.filter1;
            const std::array<float, kNumOperatorFilterFields> engineFilterValues = {
                ef.enabled ? 1.0f : 0.0f, static_cast<float>(ef.mode), ef.cutoffHz, ef.resonance, ef.keyTrack,
            };
            for (std::size_t i = 0; i < kNumOperatorFilterFields; ++i)
                setParam(operatorFilterParamId(op, kOperatorFilterFieldSpecs[i].idSuffix), engineFilterValues[i]);

            const auto& oMix = o;
            const std::array<float, kNumOperatorMixFields> mixValues = {
                oMix.mixEnabled ? 1.0f : 0.0f,
                oMix.mixMute ? 1.0f : 0.0f,
                oMix.mixSolo ? 1.0f : 0.0f,
            };
            for (std::size_t i = 0; i < kNumOperatorMixFields; ++i)
                setParam(operatorMixParamId(op, kOperatorMixFieldSpecs[i].idSuffix), mixValues[i]);
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
        setParam(kPortamentoId, currentPatch_.voiceSettings.portamentoSeconds);

        const auto& md = currentPatch_.masterDynamics;
        const std::array<float, kNumMasterDynamicsFields> mdValues = {
            md.enabled ? 1.0f : 0.0f,
            static_cast<float>(md.mode),
            md.thresholdDb,
            md.ratio,
            md.attackMs,
            md.releaseMs,
            md.sidechainGain,
            md.vactrolSlewMs,
            md.makeupDb,
            md.mix,
        };
        for (std::size_t i = 0; i < kNumMasterDynamicsFields; ++i)
            setParam(juce::String(kMasterDynamicsIdPrefix) + kMasterDynamicsFieldSpecs[i].idSuffix, mdValues[i]);

        const auto& gen = currentPatch_.generative;
        const std::array<float, kNumGenerativeFields> genValues = {
            gen.dejaVu ? 1.0f : 0.0f,
            gen.seedLocked ? 1.0f : 0.0f,
            gen.clockTRateHz,
            gen.clockXRateHz,
            gen.correlation,
            gen.streams[0].spread,
            gen.streams[0].bias,
            gen.streams[0].lagMs,
            gen.streams[1].spread,
            gen.streams[1].bias,
            gen.streams[1].lagMs,
            gen.streams[2].spread,
            gen.streams[2].bias,
            gen.streams[2].lagMs,
            gen.streams[3].spread,
            gen.streams[3].bias,
            gen.streams[3].lagMs,
            static_cast<float>(gen.outputRouting[0]),
            static_cast<float>(gen.outputRouting[1]),
            static_cast<float>(gen.outputRouting[2]),
            static_cast<float>(gen.outputRouting[3]),
            static_cast<float>(gen.outputRouting[4]),
            static_cast<float>(gen.outputRouting[5]),
        };
        for (std::size_t i = 0; i < kNumGenerativeFields; ++i)
            setParam(juce::String(kGenerativeIdPrefix) + kGenerativeFieldSpecs[i].idSuffix, genValues[i]);

        for (std::size_t slot = 0; slot < kNumPeaksUtilitySlots; ++slot)
        {
            const auto& p = currentPatch_.utilityPeaks.slots[slot];
            const std::array<float, kNumPeaksUtilitySlotFields> peakValues = {
                p.enabled ? 1.0f : 0.0f,
                static_cast<float>(p.mode),
                p.attackMs,
                p.releaseMs,
                p.lfoRateHz,
                p.lfoDepth,
            };
            for (std::size_t i = 0; i < kNumPeaksUtilitySlotFields; ++i)
                setParam(peaksUtilitySlotParamId(slot, kPeaksUtilitySlotFieldSpecs[i].idSuffix), peakValues[i]);
        }

        const auto& uni = currentPatch_.layerA.unison;
        setParam(kUnisonVoicesId, static_cast<float>(juce::jmax(1, uni.voices)));
        setParam(kUnisonDetuneId, uni.detuneCents);
        setParam(kUnisonSpreadId, uni.spread);
        setParam(kUnisonPhaseRandomId, uni.phaseRandom);

        auto syncFxSlot = [&](const effects::EffectSlotParams& p, const juce::String& id) {
            const auto values = effectSlotFieldValues(p);
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
            arp.swing,
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
        filter.modeMorph = loadF(filterParamPointers_[5]);
        currentPatch_.layerA.filterRouting = loadF(filterRoutingPointer_);

        auto& filter2 = currentPatch_.layerA.filter2;
        filter2.enabled = loadB(filter2ParamPointers_[0]);
        filter2.cutoffHz = loadF(filter2ParamPointers_[1]);
        filter2.resonance = loadF(filter2ParamPointers_[2]);
        filter2.drive = loadF(filter2ParamPointers_[3]);
        filter2.keyTrack = loadF(filter2ParamPointers_[4]);
        filter2.cutoffOffsetSemitones = loadF(filter2ParamPointers_[5]);

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
            o.engine = algorithm::sanitizeEngineForNode(
                core::NodeId(static_cast<std::uint8_t>(op)), static_cast<algorithm::EngineType>(loadI(ptrs[0])));
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
            o.wtMorphMode = loadF(ptrs[37]);
            o.externalInputSource = loadF(ptrs[38]);

            const auto& mixPtrs = operatorMixParamPointers_[op];
            o.mixEnabled = loadB(mixPtrs[0]);
            o.mixMute = loadB(mixPtrs[1]);
            o.mixSolo = loadB(mixPtrs[2]);

            const auto& fptrs = operatorFilterParamPointers_[op];
            auto& ef = o.filter1;
            ef.enabled = loadB(fptrs[0]);
            ef.mode = static_cast<pw8::filter::FilterMode>(loadI(fptrs[1]));
            ef.cutoffHz = loadF(fptrs[2]);
            ef.resonance = loadF(fptrs[3]);
            ef.keyTrack = loadF(fptrs[4]);
            ef.modeMorph = filter::modeMorphFromMode(ef.mode);
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
        currentPatch_.voiceSettings.portamentoSeconds = loadF(portamentoPointer_);

        auto& md = currentPatch_.masterDynamics;
        md.enabled = loadB(masterDynamicsParamPointers_[0]);
        md.mode = static_cast<patch::MasterDynamicsMode>(loadI(masterDynamicsParamPointers_[1]));
        md.thresholdDb = loadF(masterDynamicsParamPointers_[2]);
        md.ratio = loadF(masterDynamicsParamPointers_[3]);
        md.attackMs = loadF(masterDynamicsParamPointers_[4]);
        md.releaseMs = loadF(masterDynamicsParamPointers_[5]);
        md.sidechainGain = loadF(masterDynamicsParamPointers_[6]);
        md.vactrolSlewMs = loadF(masterDynamicsParamPointers_[7]);
        md.makeupDb = loadF(masterDynamicsParamPointers_[8]);
        md.mix = loadF(masterDynamicsParamPointers_[9]);

        auto& gen = currentPatch_.generative;
        gen.dejaVu = loadB(generativeParamPointers_[0]);
        gen.seedLocked = loadB(generativeParamPointers_[1]);
        gen.clockTRateHz = loadF(generativeParamPointers_[2]);
        gen.clockXRateHz = loadF(generativeParamPointers_[3]);
        gen.correlation = loadF(generativeParamPointers_[4]);
        for (std::size_t s = 0; s < modulation::kNumGenerativeStreams; ++s)
        {
            const std::size_t base = 5 + s * 3;
            gen.streams[s].spread = loadF(generativeParamPointers_[base]);
            gen.streams[s].bias = loadF(generativeParamPointers_[base + 1]);
            gen.streams[s].lagMs = loadF(generativeParamPointers_[base + 2]);
        }
        for (std::size_t r = 0; r < modulation::kNumGenerativeOutputs; ++r)
            gen.outputRouting[r] = static_cast<std::uint8_t>(loadI(generativeParamPointers_[17 + r]));

        for (std::size_t slot = 0; slot < kNumPeaksUtilitySlots; ++slot)
        {
            const auto& ptrs = peaksUtilityParamPointers_[slot];
            auto& p = currentPatch_.utilityPeaks.slots[slot];
            p.enabled = loadB(ptrs[0]);
            p.mode = static_cast<modulation::PeaksUtilityMode>(loadI(ptrs[1]));
            p.attackMs = loadF(ptrs[2]);
            p.releaseMs = loadF(ptrs[3]);
            p.lfoRateHz = loadF(ptrs[4]);
            p.lfoDepth = loadF(ptrs[5]);
        }

        auto& uni = currentPatch_.layerA.unison;
        uni.voices = juce::jlimit(1, static_cast<int>(core::kMaxUnisonVoices), loadI(unisonVoicesPointer_));
        uni.detuneCents = loadF(unisonDetunePointer_);
        uni.spread = loadF(unisonSpreadPointer_);
        uni.phaseRandom = loadF(unisonPhaseRandomPointer_);
        if (uni.voices > 1 && uni.mode == patch::UnisonMode::Off)
            uni.mode = patch::UnisonMode::Full;

        auto readFxSlot = [](const std::array<std::atomic<float>*, kNumEffectSlotFields>& ptrs,
                              effects::EffectSlotParams& p) {
            std::array<float, kNumEffectSlotFields> values{};
            for (std::size_t i = 0; i < kNumEffectSlotFields; ++i)
                values[i] = loadF(ptrs[i]);
            applyEffectSlotFieldValues(p, values);
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
        arp.swing = loadF(arpParamPointers_[7]);
        arp.latch = loadB(arpParamPointers_[8]);

        // Keep algorithm graph node engines aligned with operator engine pills.
        if (currentPatch_.layerA.algorithm.nodes.size() == kNumOperators)
        {
            for (std::size_t i = 0; i < kNumOperators; ++i)
                currentPatch_.layerA.algorithm.nodes[i].engine = currentPatch_.layerA.operators[i].engine;
        }
    }

    void PatchworkEightProcessor::triggerPeaksUtilitySlot(std::size_t slotIndex) noexcept
    {
        if (auto* engine = activeEngine_.load(std::memory_order_acquire))
            engine->triggerPeaksUtilitySlot(slotIndex);
    }

    std::array<float, 2> PatchworkEightProcessor::getPeaksUtilityLevels() const noexcept
    {
        if (const auto* engine = activeEngine_.load(std::memory_order_acquire))
        {
            const auto values = engine->getPeaksUtilityOutputValues();
            return {values.values[0], values.values[1]};
        }
        return {0.0f, 0.0f};
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
