#include "pw8/render/Engine.hpp"

#include "pw8/content/ContentPaths.hpp"
#include "pw8/content/WavetableCache.hpp"
#include "pw8/dsp/Denormal.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/modulation/ModMatrixExecutor.hpp"
#include "pw8/modulation/MorphKoinExecutor.hpp"
#include "pw8/effects/QuasarSpatialMacros.hpp"
#include "pw8/oscillator/WavetableTableLoader.hpp"

#include <cmath>

namespace pw8::render
{
    namespace
    {
        int effectiveUnisonVoices(const patch::UnisonSettings& uni) noexcept
        {
            int voices = uni.voices;
            if (voices < 1)
                voices = 1;
            if (voices > static_cast<int>(core::kMaxUnisonVoices))
                voices = static_cast<int>(core::kMaxUnisonVoices);
            if (uni.mode == patch::UnisonMode::Off && voices <= 1)
                return 1;
            return voices;
        }

        float centsToRatio(float cents) noexcept
        {
            return std::pow(2.0f, cents / 1200.0f);
        }

        /// PoliMATHS Modulation Dissemination (MVP): sample featured macro values at note-on.
        void assignVoicePerformanceSnapshot(voice::Voice& voice, const patch::Patch& patch, std::uint64_t voiceSeed,
                                            std::uint64_t noteGenerationId) noexcept
        {
            const float defaultMorph = patch.morphKoin.keyframes.size() >= 2 ? patch.morphKoin.position
                                                                             : patch.morphKoin.defaultPosition;

            if (!patch.voiceSettings.macroDissemination)
            {
                for (std::size_t i = 0; i < voice.macroValues.size(); ++i)
                    voice.macroValues[i] = patch.macros[i].value;
                voice.morphPosition = defaultMorph;
                return;
            }

            const float spread = dsp::clamp(patch.voiceSettings.disseminationDepth, 0.0f, 1.0f);
            const auto rngSeed =
                dsp::DeterministicRng::deriveSeed(voiceSeed, noteGenerationId, patch.seed ^ 0xD155E111u);
            dsp::DeterministicRng rng(rngSeed);

            for (std::size_t i = 0; i < voice.macroValues.size(); ++i)
            {
                const float base = patch.macros[i].value;
                if (i < 3 && spread > 0.0f)
                {
                    const float offset = (rng.nextFloat() * 2.0f - 1.0f) * spread;
                    voice.macroValues[i] = dsp::clamp(base + offset, 0.0f, 1.0f);
                }
                else
                    voice.macroValues[i] = base;
            }

            if (patch.voiceSettings.morphDissemination && patch.morphKoin.keyframes.size() >= 2)
            {
                const float baseMorph = patch.morphKoin.position;
                const float morphOffset = spread > 0.0f ? (rng.nextFloat() * 2.0f - 1.0f) * spread * 0.35f : 0.0f;
                voice.morphPosition = dsp::clamp(baseMorph + morphOffset, 0.0f, 1.0f);
            }
            else
                voice.morphPosition = defaultMorph;
        }

        float layerOutputGain(const patch::LayerPatch& layer, float masterGain) noexcept
        {
            const int unisonVoices = effectiveUnisonVoices(layer.unison);
            const float gainScale = layer.unison.blend / static_cast<float>(unisonVoices);
            return layer.gain * masterGain * gainScale;
        }

        patch::UnisonSettings applyUnisonModOffsets(const patch::UnisonSettings& base,
                                                      const modulation::ModOutputs& mod) noexcept
        {
            patch::UnisonSettings out = base;
            const int modulatedVoices = static_cast<int>(std::lround(static_cast<float>(out.voices) +
                                                                     mod.unisonVoicesOffset));
            out.voices = modulatedVoices;
            if (out.voices < 1)
                out.voices = 1;
            if (out.voices > static_cast<int>(core::kMaxUnisonVoices))
                out.voices = static_cast<int>(core::kMaxUnisonVoices);
            out.detuneCents = dsp::clamp(out.detuneCents + mod.unisonDetuneOffset, 0.0f, 100.0f);
            out.spread = dsp::clamp(out.spread + mod.unisonSpreadOffset, 0.0f, 1.0f);
            return out;
        }

        /// Voice-bus summation sub-block size (Sprint 4 prototype). Per-voice FM/sync
        /// graph rendering stays per-sample inside Voice::renderSample().
        static constexpr std::size_t kVoiceSumSubBlockSize = 4;

        void kahanAdd(float& sum, float& compensation, float value) noexcept
        {
            const float y = value - compensation;
            const float t = sum + y;
            compensation = (t - sum) - y;
            sum = t;
        }

        void unisonSpread(const patch::UnisonSettings& uni, int index, int count, float& detuneCentsOut,
                          float& panOut) noexcept
        {
            if (count <= 1)
            {
                detuneCentsOut = 0.0f;
                panOut = 0.0f;
                return;
            }

            const float center = static_cast<float>(count - 1) * 0.5f;
            const float norm = (static_cast<float>(index) - center) / (center > 0.0f ? center : 1.0f);
            detuneCentsOut = norm * uni.detuneCents * uni.spread;
            panOut = norm * uni.spread;
        }

        modulation::ModSourceValues buildMasterBusModSources(
            const patch::Patch& patch, const std::array<float, core::kNumLfosPerLayer>& layerLfoValues, float modWheel,
            float expression, float sidechain, const modulation::GenerativeOutputValues& generative) noexcept
        {
            modulation::ModSourceValues sources;
            sources.layerLfos = layerLfoValues;
            sources.modWheel = modWheel;
            sources.expression = expression;
            sources.sidechain = sidechain;
            sources.randomT = generative.randomT;
            sources.randomX = generative.randomX;
            sources.randomOutputs = generative.outputs;
            for (std::size_t i = 0; i < sources.macros.size() && i < patch.macros.size(); ++i)
                sources.macros[i] = patch.macros[i].value;
            return sources;
        }

        void applyMasterModToEffects(std::array<effects::EffectSlotParams, effects::kNumMasterSlots>& slots,
                                     const modulation::MasterModOutputs& mod) noexcept
        {
            for (std::size_t i = 0; i < slots.size(); ++i)
            {
                auto& e = slots[i];
                e.mix = dsp::clamp(e.mix + mod.mixOffset[i], 0.0f, 1.0f);
                if (e.type == effects::EffectType::Reverb)
                {
                    e.reverbSizeParam = dsp::clamp(e.reverbSizeParam + mod.reverbSizeOffset[i], 0.2f, 3.0f);
                    e.reverbDecaySeconds = dsp::clamp(e.reverbDecaySeconds + mod.reverbDecayOffset[i], 0.05f, 20.0f);
                    e.reverbPreDelayMs = dsp::clamp(e.reverbPreDelayMs + mod.reverbPreDelayOffset[i], 0.0f, 200.0f);
                    e.reverbDiffusion = dsp::clamp(e.reverbDiffusion + mod.reverbDiffusionOffset[i], 0.0f, 1.0f);
                    e.reverbModDepth = dsp::clamp(e.reverbModDepth + mod.reverbModDepthOffset[i], 0.0f, 1.0f);
                }
                else if (e.type == effects::EffectType::Vocoder)
                {
                    e.mix = dsp::clamp(e.mix + mod.masterVocoderMixOffset[i], 0.0f, 1.0f);
                    e.vocoderFormant =
                        dsp::clamp(e.vocoderFormant + mod.masterVocoderFormantOffset[i], 0.0f, 1.0f);
                }
                else if (e.type == effects::EffectType::Compressor)
                {
                    e.compThresholdDb = dsp::clamp(e.compThresholdDb + mod.compThresholdOffset[i], -60.0f, 0.0f);
                }
                else if (e.type == effects::EffectType::BinauralSpace)
                {
                    e.qsr1AngleDeg = effects::wrapDegrees360(e.qsr1AngleDeg + mod.quasarQsr1AngleOffset[i]);
                    e.qsr2AngleDeg = effects::wrapDegrees360(e.qsr2AngleDeg + mod.quasarQsr2AngleOffset[i]);
                    e.qsr1RoomAmount = dsp::clamp(e.qsr1RoomAmount + mod.quasarRoomAmountOffset[i], 0.0f, 1.0f);
                    e.quasarCrossfeed = dsp::clamp(e.quasarCrossfeed + mod.quasarCrossfeedOffset[i], 0.0f, 1.0f);
                    e.quasarDelayVolume = dsp::clamp(e.quasarDelayVolume + mod.quasarDelayVolumeOffset[i], 0.0f, 1.0f);
                    e.qsr1Distance = dsp::clamp(e.qsr1Distance + mod.quasarQsr1DistanceOffset[i], 0.0f, 1.0f);
                    e.qsr2Distance = dsp::clamp(e.qsr2Distance + mod.quasarQsr2DistanceOffset[i], 0.0f, 1.0f);
                    e.quasarDelayTimeMs =
                        dsp::clamp(e.quasarDelayTimeMs + mod.quasarDelayTimeOffset[i], 3.0f, 20000.0f);
                    e.quasarDelayFeedback =
                        dsp::clamp(e.quasarDelayFeedback + mod.quasarDelayFeedbackOffset[i], 0.0f, 0.95f);
                    e.qsr1Height = dsp::clamp(e.qsr1Height + mod.quasarQsr1HeightOffset[i], -1.0f, 1.0f);
                    e.qsr2Height = dsp::clamp(e.qsr2Height + mod.quasarQsr2HeightOffset[i], -1.0f, 1.0f);
                    e.cntrLevel = dsp::clamp(e.cntrLevel + mod.quasarCntrLevelOffset[i], 0.0f, 1.0f);
                    e.qsr1Level = dsp::clamp(e.qsr1Level + mod.quasarQsr1LevelOffset[i], 0.0f, 1.0f);
                    e.qsr2Level = dsp::clamp(e.qsr2Level + mod.quasarQsr2LevelOffset[i], 0.0f, 1.0f);
                }
            }
        }

        void applyQuasarMacroKoinsToMasterEffects(std::array<effects::EffectSlotParams, effects::kNumMasterSlots>& slots,
                                                  float orbitMacro, float spreadMacro) noexcept
        {
            const effects::QuasarMacroKoinValues macros{orbitMacro, spreadMacro};
            for (auto& e : slots)
            {
                if (e.type != effects::EffectType::BinauralSpace)
                    continue;

                const effects::QuasarSpatialTargets base{e.qsr1AngleDeg, e.qsr2AngleDeg, e.qsr1Distance, e.qsr2Distance,
                                                         e.qsr1Height, e.qsr2Height};
                const auto out = effects::applyQuasarMacroKoins(base, macros);
                e.qsr1AngleDeg = out.qsr1AngleDeg;
                e.qsr2AngleDeg = out.qsr2AngleDeg;
                e.qsr1Distance = out.qsr1Distance;
                e.qsr2Distance = out.qsr2Distance;
                e.qsr1Height = out.qsr1Height;
                e.qsr2Height = out.qsr2Height;
            }
        }

        void applyInsertModToEffects(std::array<effects::EffectSlotParams, effects::kNumLayerInsertSlots>& slots,
                                     const modulation::MasterModOutputs& mod) noexcept
        {
            for (std::size_t i = 0; i < slots.size(); ++i)
            {
                auto& e = slots[i];
                if (e.type != effects::EffectType::Vocoder)
                    continue;
                e.mix = dsp::clamp(e.mix + mod.vocoderMixOffset[i], 0.0f, 1.0f);
                e.vocoderFormant = dsp::clamp(e.vocoderFormant + mod.vocoderFormantOffset[i], 0.0f, 1.0f);
            }
        }

        float masterGainMultiplier(float baseGain, float gainOffset) noexcept
        {
            const float base = baseGain > 0.0f ? baseGain : 1.0f;
            return dsp::clamp(base + gainOffset, 0.0f, 4.0f) / base;
        }
    } // namespace

    void Engine::prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        for (auto& v : voices_)
            v.prepare(sampleRate_);
        for (auto& v : voicesB_)
            v.prepare(sampleRate_);
        for (auto& l : layerLfosA_)
            l.prepare(sampleRate_);
        for (auto& l : layerLfosB_)
            l.prepare(sampleRate_);
        arpeggiator_.prepare(sampleRate_);
        layerAInsertChain_.prepare(sampleRate_);
        layerBInsertChain_.prepare(sampleRate_);
        masterChain_.prepare(sampleRate_);
        masterDynamicsProcessor_.prepare(sampleRate_);
        generativeProcessor_.prepare(sampleRate_);
        peaksUtilityProcessor_.prepare(sampleRate_);
        sendReturnA_.prepare(sampleRate_);
        sendReturnB_.prepare(sampleRate_);
        synthBusPeak_.store(0.0f, std::memory_order_relaxed);
        masterOutPeak_.store(0.0f, std::memory_order_relaxed);
        for (auto& peak : operatorPeaks_)
            peak.store(0.0f, std::memory_order_relaxed);
    }

    op::OperatorParams Engine::toOperatorParams(const patch::OperatorPatch& p) noexcept
    {
        op::OperatorParams out;
        out.engine = p.engine;
        out.classic.waveform = p.classicWaveform;
        out.classic.morph = p.classicMorph;
        out.classic.pulseWidth = p.pulseWidth;
        out.wavetableFramePosition = p.wavetableFramePosition;
        out.frequencyRatio = p.frequencyRatio;
        out.fixedFrequencyHz = p.fixedFrequencyHz;
        out.keyTrack = p.keyTrack;
        out.level = p.level;
        out.mixEnabled = p.mixEnabled;
        out.mixMute = p.mixMute;
        out.mixSolo = p.mixSolo;
        out.mixGain = 1.0f;
        out.fmModulatorRatio = p.fmModulatorRatio;
        out.fmModulatorIndex = p.fmModulatorIndex;
        out.fmModulatorFeedback = p.fmModulatorFeedback;
        out.fmModulatorWaveform = p.fmModulatorWaveform;
        out.noiseVariant = p.noiseVariant;
        out.noiseRate = p.noiseRate;
        out.phaseBend = p.phaseBend;
        out.phaseFold = p.phaseFold;
        out.phaseAsymmetry = p.phaseAsymmetry;
        out.phaseShape = p.phaseShape;
        out.additivePartialCount = p.additivePartialCount;
        out.additiveTilt = p.additiveTilt;
        out.additiveOddEven = p.additiveOddEven;
        out.additiveStretch = p.additiveStretch;
        out.resonatorStructure = p.resonatorStructure;
        out.resonatorDecay = p.resonatorDecay;
        out.resonatorDamping = p.resonatorDamping;
        out.resonatorBrightness = p.resonatorBrightness;
        out.resonatorModeCount = p.resonatorModeCount;
        out.grainDensity = p.grainDensity;
        out.grainSizeMs = p.grainSizeMs;
        out.grainPositionJitter = p.grainPositionJitter;
        out.grainPitchJitter = p.grainPitchJitter;
        out.wtBend = p.wtBend;
        out.wtAsymmetry = p.wtAsymmetry;
        out.wtSyncRatio = p.wtSyncRatio;
        out.wtSyncAmount = p.wtSyncAmount;
        out.wtFormantShift = p.wtFormantShift;
        out.wtMorphMode = p.wtMorphMode;
        out.externalInputSource = p.externalInputSource;
        return out;
    }

    void Engine::loadLayerResources(const patch::LayerPatch& layer, algorithm::CompiledAlgorithm& compiledOut,
                                       std::array<op::OperatorParams, core::kNodesPerLayer>& templatesOut,
                                       std::array<std::shared_ptr<const oscillator::WavetableTable>, core::kNodesPerLayer>& sharedOut,
                                       std::array<const oscillator::WavetableTable*, core::kNodesPerLayer>& tablesOut,
                                       std::array<lfo::Lfo, core::kNumLfosPerLayer>& lfosOut, voice::VoicePool& voices,
                                       algorithm::CompileStatus& statusOut) noexcept
    {
        algorithm::CompiledAlgorithm compiled;
        statusOut = algorithm::AlgorithmGraphCompiler::compile(layer.algorithm, compiled);
        if (statusOut == algorithm::CompileStatus::Ok)
            compiledOut = compiled;
        else
        {
            algorithm::CompiledAlgorithm fallback;
            [[maybe_unused]] const auto fallbackStatus =
                algorithm::AlgorithmGraphCompiler::compile(algorithm::AlgorithmGraphDefinition::makeDefaultParallel8(), fallback);
            compiledOut = fallback;
        }

        for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
            templatesOut[i] = toOperatorParams(layer.operators[i]);

        bool anyMixSolo = false;
        for (const auto& op : layer.operators)
        {
            if (op.mixSolo)
            {
                anyMixSolo = true;
                break;
            }
        }
        for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
        {
            const auto& op = layer.operators[i];
            if (!op.mixEnabled || op.mixMute)
                templatesOut[i].mixGain = 0.0f;
            else if (anyMixSolo)
                templatesOut[i].mixGain = op.mixSolo ? 1.0f : 0.0f;
            else
                templatesOut[i].mixGain = 1.0f;
        }

        for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
        {
            sharedOut[i].reset();
            tablesOut[i] = nullptr;
            const auto& op = layer.operators[i];
            if ((op.engine == algorithm::EngineType::Wavetable || op.engine == algorithm::EngineType::Granular) &&
                !op.wavetableId.empty())
            {
                const auto resolved = content::resolveWavetablePath(op.wavetableId);
                const auto& pathToLoad = resolved.has_value() ? *resolved : op.wavetableId;
                sharedOut[i] = content::WavetableCache::instance().getOrLoad(pathToLoad);
                tablesOut[i] = sharedOut[i] ? sharedOut[i].get() : nullptr;
            }
        }

        for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
            lfosOut[i].noteOn(layer.lfos[i],
                               dsp::DeterministicRng::deriveSeed(patch_.seed, static_cast<std::uint32_t>(i), patch_.seed));

        std::array<float, 8> macroValues{};
        for (std::size_t i = 0; i < macroValues.size(); ++i)
            macroValues[i] = patch_.macros[i].value;

        for (auto& v : voices)
        {
            v.operatorParams = templatesOut;
            v.filterParams = layer.filter1;
            v.filter2Params = layer.filter2;
            v.filterRouting = layer.filterRouting;
            for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
                v.operatorFilterParams_[i] = layer.operators[i].filter1;
            v.lfoParams = layer.lfos;
            v.segmentEnvelopeChains = layer.segmentEnvelopeChains;
            v.macroValues = macroValues;
        }
    }

    bool Engine::loadPatch(const patch::Patch& patchToLoad) noexcept
    {
        patch_ = patchToLoad;
        patch::ensureDefaultModWheelRoute(patch_.layerA);
        patch::ensureDefaultExpressionRoute(patch_.layerA);
        patch::ensureMinimumMacroKoinRoutes(patch_.layerA);

        // Only SingleA and Stack are rendered today; other layer modes stay clamped.
        if (patch_.layerMode != patch::LayerMode::SingleA && patch_.layerMode != patch::LayerMode::Stack)
            patch_.layerMode = patch::LayerMode::SingleA;

        const bool stackActive = patch_.layerMode == patch::LayerMode::Stack;

        algorithm::CompileStatus statusA = algorithm::CompileStatus::Ok;
        loadLayerResources(patch_.layerA, compiledLayerA_, operatorParamsTemplateA_, wavetableSharedA_,
                           wavetableTablesA_, layerLfosA_, voices_, statusA);

        algorithm::CompileStatus statusB = algorithm::CompileStatus::Ok;
        if (stackActive)
        {
            loadLayerResources(patch_.layerB, compiledLayerB_, operatorParamsTemplateB_, wavetableSharedB_,
                               wavetableTablesB_, layerLfosB_, voicesB_, statusB);
        }

        lastCompileStatus_ = statusA;
        if (stackActive && statusB != algorithm::CompileStatus::Ok)
            lastCompileStatus_ = statusB;

        const std::size_t totalPoly = patch_.voiceSettings.polyphony < 1 ? 1 : patch_.voiceSettings.polyphony;
        const std::size_t layerPoly = stackActive ? std::max<std::size_t>(1, totalPoly / 2) : totalPoly;
        allocator_.configure(layerPoly);
        allocatorB_.configure(stackActive ? layerPoly : 1);

        tuning_.setA4(patch_.voiceSettings.a4Hz);
        arpeggiator_.configure(patch_.arpeggiator, patch_.seed);

        layerAInsertChain_.reset();
        layerBInsertChain_.reset();
        masterChain_.reset();
        masterDynamicsProcessor_.reset();
        generativeProcessor_.reset(patch_.generative, patch_.seed);
        dynamicsGatePrev_ = false;

        return lastCompileStatus_ == algorithm::CompileStatus::Ok;
    }

    void Engine::triggerLayerNoteOn(const patch::LayerPatch& layer, const patch::UnisonSettings& unison,
                                     const std::array<op::OperatorParams, core::kNodesPerLayer>& templates,
                                     voice::VoicePool& voices, voice::VoiceAllocator& allocator, int note, int channel,
                                     int velocity7) noexcept
    {
        const int unisonVoices = effectiveUnisonVoices(unison);
        const float baseFreqHz = tuning_.noteToFrequency(static_cast<float>(note));
        const float velUnit = static_cast<float>(velocity7) / 127.0f;
        const float portamentoSeconds = patch_.voiceSettings.portamentoSeconds;

        for (int u = 0; u < unisonVoices; ++u)
        {
            std::size_t voiceIndex = 0;
            voice::AllocateResult allocationResult = voice::AllocateResult::Free;
            bool sameNoteRetrigger = false;

            if (u == 0)
            {
                if (const auto existing = allocator.findGatedVoice(voices, note, channel))
                {
                    voiceIndex = *existing;
                    allocationResult = voice::AllocateResult::Released;
                    sameNoteRetrigger = true;
                }
                else
                {
                    const auto allocation = allocator.allocate(voices);
                    voiceIndex = allocation.index;
                    allocationResult = allocation.result;
                }
            }
            else
            {
                const auto allocation = allocator.allocate(voices);
                voiceIndex = allocation.index;
                allocationResult = allocation.result;
            }

            auto& v = voices[voiceIndex];
            v.id = static_cast<std::uint32_t>(voiceIndex);
            v.operatorParams = templates;
            v.filterParams = layer.filter1;
            v.filter2Params = layer.filter2;
            v.filterRouting = layer.filterRouting;
            for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
                v.operatorFilterParams_[i] = layer.operators[i].filter1;
            v.lfoParams = layer.lfos;
            v.segmentEnvelopeChains = layer.segmentEnvelopeChains;
            const auto age = allocator.nextAge();
            const auto noteGen = ++noteGenerationCounter_;
            const auto voiceSeed = patch_.seed ^ static_cast<std::uint64_t>(u + 1);
            assignVoicePerformanceSnapshot(v, patch_, voiceSeed, noteGen);

            float detuneCents = 0.0f;
            float unisonPan = 0.0f;
            unisonSpread(unison, u, unisonVoices, detuneCents, unisonPan);

            const float detunedHz = baseFreqHz * centsToRatio(detuneCents);
            const bool crossfadeRetrigger = !sameNoteRetrigger &&
                                            (allocationResult == voice::AllocateResult::Stolen
                                             || (allocationResult == voice::AllocateResult::Released && v.isSounding()));
            if (crossfadeRetrigger)
                v.beginStealCrossfadeNoteOn(note, channel, velUnit, detunedHz, layer.envelopes, age, noteGen, voiceSeed,
                                            portamentoSeconds);
            else
                v.noteOn(note, channel, velUnit, detunedHz, layer.envelopes, age, noteGen, voiceSeed,
                         portamentoSeconds);
            if (channel >= 0 && channel < static_cast<int>(channelModWheel_.size()))
                v.expression.modWheel = channelModWheel_[static_cast<std::size_t>(channel)];
            if (channel >= 0 && channel < static_cast<int>(channelExpression_.size()))
                v.expression.expression = channelExpression_[static_cast<std::size_t>(channel)];
            v.outputGain = layerOutputGain(layer, patch_.voiceSettings.masterGain);
            v.pan = dsp::clamp(layer.pan + unisonPan, -1.0f, 1.0f);
        }
    }

    void Engine::noteOn(int note, int channel, int velocity7) noexcept
    {
        if (velocity7 <= 0)
        {
            noteOff(note, channel, 0);
            return;
        }

        if (patch_.arpeggiator.enabled)
        {
            arpeggiator_.noteHeld(note, channel, static_cast<float>(velocity7) / 127.0f);
            return;
        }

        triggerNoteOnDirect(note, channel, velocity7);
    }

    void Engine::noteOff(int note, int channel, int velocity7) noexcept
    {
        if (patch_.arpeggiator.enabled)
        {
            arpeggiator_.noteReleased(note, channel);
            return;
        }

        triggerNoteOffDirect(note, channel, velocity7);
    }

    void Engine::triggerNoteOnDirect(int note, int channel, int velocity7) noexcept
    {
        if (velocity7 <= 0)
        {
            triggerNoteOffDirect(note, channel, 0);
            return;
        }

        triggerLayerNoteOn(patch_.layerA, modulatedUnisonForNoteOn(patch_.layerA, layerLfosA_, channel),
                           operatorParamsTemplateA_, voices_, allocator_, note, channel, velocity7);

        if (isStackModeActive())
            triggerLayerNoteOn(patch_.layerB, modulatedUnisonForNoteOn(patch_.layerB, layerLfosB_, channel),
                               operatorParamsTemplateB_, voicesB_, allocatorB_, note, channel, velocity7);
    }

    patch::UnisonSettings Engine::modulatedUnisonForNoteOn(
        const patch::LayerPatch& layer, std::array<lfo::Lfo, core::kNumLfosPerLayer>& lfos, int channel) noexcept
    {
        std::array<float, core::kNumLfosPerLayer> lfoValues{};
        for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
            lfoValues[i] = lfos[i].renderSample(layer.lfos[i], bpm_);

        const int ch = channel < 0 ? 0 : (channel > 15 ? 15 : channel);
        modulation::GenerativeOutputValues gen{};
        const auto sources =
            buildMasterBusModSources(patch_, lfoValues, channelModWheel_[static_cast<std::size_t>(ch)],
                                     channelExpression_[static_cast<std::size_t>(ch)], sidechainLevel_, gen);
        const auto mod = modulation::ModMatrixExecutor::computeLayerUnisonMod(layer.modRoutes, sources);
        return applyUnisonModOffsets(layer.unison, mod);
    }

    void Engine::triggerNoteOffDirect(int note, int channel, int velocity7) noexcept
    {
        const float relVel = static_cast<float>(velocity7) / 127.0f;
        const bool deferForSustain =
            channel >= 0 && channel < static_cast<int>(sustainPedalHeld_.size()) && sustainPedalHeld_[static_cast<std::size_t>(channel)];

        if (deferForSustain)
        {
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            {
                auto& v = voices_[i];
                if (v.gateOn && v.midiChannel == channel && v.noteNumber == note)
                {
                    v.sustainPendingRelease = true;
                    v.releaseVelocity = relVel;
                }
            }
            if (isStackModeActive())
            {
                for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                {
                    auto& v = voicesB_[i];
                    if (v.gateOn && v.midiChannel == channel && v.noteNumber == note)
                    {
                        v.sustainPendingRelease = true;
                        v.releaseVelocity = relVel;
                    }
                }
            }
            return;
        }

        allocator_.release(voices_, note, channel, relVel);
        if (isStackModeActive())
            allocatorB_.release(voicesB_, note, channel, relVel);
    }

    void Engine::pitchBend(int channel, int value14) noexcept
    {
        const float normalized = (static_cast<float>(value14) - 8192.0f) / 8192.0f; // -1..~1
        const float semitones = normalized * kPitchBendRangeSemitones;

        if (channel == 0)
        {
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
                if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                    voices_[i].expression.pitchBendSemitones = semitones;
            if (isStackModeActive())
            {
                for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                    if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel)
                        voicesB_[i].expression.pitchBendSemitones = semitones;
            }
            return;
        }

        // MPE member channels: per-note pitch bend on the voice allocated to that channel.
        for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                voices_[i].expression.mpePitch = semitones;
        if (isStackModeActive())
        {
            for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel)
                    voicesB_[i].expression.mpePitch = semitones;
        }
    }

    void Engine::controlChange(int channel, int controller, int value7) noexcept
    {
        if (channel < 0 || channel >= static_cast<int>(sustainPedalHeld_.size()))
            return;

        if (controller == 64)
        {
            const bool wasHeld = sustainPedalHeld_[static_cast<std::size_t>(channel)];
            const bool nowHeld = value7 >= 64;
            sustainPedalHeld_[static_cast<std::size_t>(channel)] = nowHeld;
            if (wasHeld && !nowHeld)
            {
                for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
                {
                    auto& v = voices_[i];
                    if (v.sustainPendingRelease && v.midiChannel == channel)
                    {
                        v.sustainPendingRelease = false;
                        v.noteOff(v.releaseVelocity);
                    }
                }
                if (isStackModeActive())
                {
                    for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                    {
                        auto& v = voicesB_[i];
                        if (v.sustainPendingRelease && v.midiChannel == channel)
                        {
                            v.sustainPendingRelease = false;
                            v.noteOff(v.releaseVelocity);
                        }
                    }
                }
            }
            return;
        }

        // CC120 All Sound Off — immediate silence on this channel.
        if (controller == 120)
        {
            allSoundOff(channel);
            return;
        }

        // CC121 Reset All Controllers — clear sustain latch (common on transport stop).
        if (controller == 121)
        {
            sustainPedalHeld_[static_cast<std::size_t>(channel)] = false;
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            {
                auto& v = voices_[i];
                if (v.sustainPendingRelease && v.midiChannel == channel)
                {
                    v.sustainPendingRelease = false;
                    v.noteOff(v.releaseVelocity);
                }
            }
            if (isStackModeActive())
            {
                for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                {
                    auto& v = voicesB_[i];
                    if (v.sustainPendingRelease && v.midiChannel == channel)
                    {
                        v.sustainPendingRelease = false;
                        v.noteOff(v.releaseVelocity);
                    }
                }
            }
            return;
        }

        // CC123 All Notes Off — enter release on this channel.
        if (controller == 123)
        {
            allNotesOff(channel);
            return;
        }

        // CC1 Mod Wheel — channel-wide, latched; affects new notes via channelModWheel_ at note-on.
        if (controller == 1)
        {
            const float v = static_cast<float>(value7) / 127.0f;
            if (channel >= 0 && channel < static_cast<int>(channelModWheel_.size()))
                channelModWheel_[static_cast<std::size_t>(channel)] = v;
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
                if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                    voices_[i].expression.modWheel = v;
            if (isStackModeActive())
            {
                for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                    if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel)
                        voicesB_[i].expression.modWheel = v;
            }
            return;
        }

        // CC11 Expression — expression pedal / assignable knob (Kawai MP11SE default).
        if (controller == 11)
        {
            const float v = static_cast<float>(value7) / 127.0f;
            if (channel >= 0 && channel < static_cast<int>(channelExpression_.size()))
                channelExpression_[static_cast<std::size_t>(channel)] = v;
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
                if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                    voices_[i].expression.expression = v;
            if (isStackModeActive())
            {
                for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                    if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel)
                        voicesB_[i].expression.expression = v;
            }
            return;
        }

        // CC74 (MPE slide/timbre).
        if (controller == 74)
        {
            const float v = static_cast<float>(value7) / 127.0f;
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
                if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                    voices_[i].expression.mpeSlide = v;
            if (isStackModeActive())
            {
                for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                    if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel)
                        voicesB_[i].expression.mpeSlide = v;
            }
        }
    }

    void Engine::channelPressure(int channel, int value7) noexcept
    {
        const float v = static_cast<float>(value7) / 127.0f;
        for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                voices_[i].expression.channelPressure = v;
        if (isStackModeActive())
        {
            for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel)
                    voicesB_[i].expression.channelPressure = v;
        }
    }

    void Engine::polyAftertouch(int channel, int note, int value7) noexcept
    {
        const float v = static_cast<float>(value7) / 127.0f;
        for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            if (voices_[i].gateOn && voices_[i].midiChannel == channel && voices_[i].noteNumber == note)
                voices_[i].expression.polyAftertouch = v;
        if (isStackModeActive())
        {
            for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel && voicesB_[i].noteNumber == note)
                    voicesB_[i].expression.polyAftertouch = v;
        }
    }

    void Engine::allNotesOff(int channel) noexcept
    {
        if (channel < 0)
            sustainPedalHeld_.fill(false);
        else if (channel < static_cast<int>(sustainPedalHeld_.size()))
            sustainPedalHeld_[static_cast<std::size_t>(channel)] = false;

        const auto releasePool = [&](voice::VoicePool& voices, voice::VoiceAllocator& allocator) {
            if (channel < 0)
            {
                for (auto& v : voices)
                    v.sustainPendingRelease = false;
                allocator.releaseAll(voices, 0.5f);
                return;
            }

            for (std::size_t i = 0; i < allocator.getPolyphony(); ++i)
            {
                auto& v = voices[i];
                if (v.gateOn && v.midiChannel == channel)
                {
                    v.sustainPendingRelease = false;
                    v.noteOff(v.releaseVelocity);
                }
            }
        };

        releasePool(voices_, allocator_);
        if (isStackModeActive())
            releasePool(voicesB_, allocatorB_);

        arpeggiator_.reset();
    }

    void Engine::allSoundOff(int channel) noexcept
    {
        if (channel < 0)
            sustainPedalHeld_.fill(false);
        else if (channel < static_cast<int>(sustainPedalHeld_.size()))
            sustainPedalHeld_[static_cast<std::size_t>(channel)] = false;

        const auto killPool = [&](voice::VoicePool& voices, voice::VoiceAllocator& allocator) {
            for (std::size_t i = 0; i < allocator.getPolyphony(); ++i)
            {
                auto& v = voices[i];
                if (channel >= 0 && v.midiChannel != channel)
                    continue;
                v.sustainPendingRelease = false;
                if (v.isSounding() || v.gateOn)
                    v.hardKill();
            }
        };

        killPool(voices_, allocator_);
        if (isStackModeActive())
            killPool(voicesB_, allocatorB_);

        arpeggiator_.reset();
    }

    void Engine::setMacroValue(std::size_t index, float value) noexcept
    {
        if (index >= patch_.macros.size())
            return;

        patch_.macros[index].value = value;
        if (patch_.voiceSettings.macroDissemination)
            return;

        for (auto& v : voices_)
            if (index < v.macroValues.size())
                v.macroValues[index] = value;
        for (auto& v : voicesB_)
            if (index < v.macroValues.size())
                v.macroValues[index] = value;
    }

    void Engine::applyMorphPositionLive(float position) noexcept
    {
        if (patch_.morphKoin.keyframes.size() < 2)
            return;

        float pos = dsp::clamp(position, 0.0f, 1.0f);
        if (patch_.morphKoin.wrap)
            pos = std::fmod(pos, 1.0f);

        modulation::applyMorphKoin(patch_, pos);
        patch_.morphKoin.position = pos;

        if (!patch_.voiceSettings.macroDissemination)
        {
            for (std::size_t i = 0; i < patch_.macros.size(); ++i)
                setMacroValue(i, patch_.macros[i].value);
        }

        setFilterLive(patch_.layerA.filter1);
        setFilter2Live(patch_.layerA.filter2);

        for (std::size_t slot = 0; slot < effects::kNumMasterSlots; ++slot)
            setMasterEffectLive(slot, patch_.masterEffects[slot]);

        if (!patch_.voiceSettings.morphDissemination)
        {
            for (auto& v : voices_)
                v.morphPosition = pos;
            for (auto& v : voicesB_)
                v.morphPosition = pos;
        }
    }

    void Engine::setFilterLive(const filter::FilterParams& params) noexcept
    {
        patch_.layerA.filter1 = params;
        for (auto& v : voices_)
            v.filterParams = params;
    }

    void Engine::setFilter2Live(const filter::CharacterFilterParams& params) noexcept
    {
        patch_.layerA.filter2 = params;
        for (auto& v : voices_)
            v.filter2Params = params;
    }

    void Engine::setFilterRoutingLive(float routing) noexcept
    {
        patch_.layerA.filterRouting = dsp::clamp(routing, 0.0f, 1.0f);
        for (auto& v : voices_)
            v.filterRouting = patch_.layerA.filterRouting;
    }

    void Engine::setOperatorFilterLive(std::size_t opIndex, const filter::FilterParams& params) noexcept
    {
        if (opIndex >= core::kNodesPerLayer)
            return;

        patch_.layerA.operators[opIndex].filter1 = params;
        for (auto& v : voices_)
            v.operatorFilterParams_[opIndex] = params;
    }

    void Engine::setLfoLive(std::size_t lfoIndex, const lfo::LfoParams& params) noexcept
    {
        if (lfoIndex >= core::kNumLfosPerLayer)
            return;

        // patch_.layerA.lfos[lfoIndex] is what process() reads fresh every sample for
        // the shared LAYER/GLOBAL-scope tick (layerLfosA_[lfoIndex].renderSample(
        // patch_.layerA.lfos[lfoIndex], ...)), so this one write covers both scopes.
        patch_.layerA.lfos[lfoIndex] = params;
        for (auto& v : voices_)
            v.lfoParams[lfoIndex] = params;
    }

    void Engine::setOperatorLive(std::size_t opIndex, const op::OperatorParams& params) noexcept
    {
        if (opIndex >= core::kNodesPerLayer)
            return;

        operatorParamsTemplateA_[opIndex] = params;
        for (auto& v : voices_)
            v.operatorParams[opIndex] = params;

        // Keep patch_ (the getStateInformation()/preset-save source of truth) in sync
        // too -- mapping back into OperatorPatch's flattened field names, and
        // deliberately NOT touching wavetableId or pan (not part of the live API).
        auto& stored = patch_.layerA.operators[opIndex];
        stored.engine =
            algorithm::sanitizeEngineForNode(core::NodeId(static_cast<std::uint8_t>(opIndex)), params.engine);
        stored.classicWaveform = params.classic.waveform;
        stored.classicMorph = params.classic.morph;
        stored.pulseWidth = params.classic.pulseWidth;
        stored.wavetableFramePosition = params.wavetableFramePosition;
        stored.frequencyRatio = params.frequencyRatio;
        stored.fixedFrequencyHz = params.fixedFrequencyHz;
        stored.keyTrack = params.keyTrack;
        stored.level = params.level;
        stored.mixEnabled = params.mixEnabled;
        stored.mixMute = params.mixMute;
        stored.mixSolo = params.mixSolo;
    }

    void Engine::setEnvelopeLive(std::size_t envIndex, const envelope::DahdsrParams& params) noexcept
    {
        if (envIndex >= core::kNumEnvelopesPerLayer)
            return;
        patch_.layerA.envelopes[envIndex] = params;
        for (auto& v : voices_)
            if (v.isSounding())
                v.envelopes[envIndex].retargetParams(params);
    }

    void Engine::setSegmentEnvelopeChainLive(std::size_t envIndex,
                                              const envelope::SegmentEnvelopeChain& chain) noexcept
    {
        if (envIndex >= core::kNumEnvelopesPerLayer)
            return;
        patch_.layerA.segmentEnvelopeChains[envIndex] = chain;
        for (auto& v : voices_)
        {
            v.segmentEnvelopeChains[envIndex] = chain;
            if (v.isSounding() && chain.isActive())
                v.segmentEnvelopes_[envIndex].noteOn(chain, patch_.layerA.envelopes[envIndex].releaseSeconds);
        }
    }

    void Engine::setOperatorWavetableLive(std::size_t opIndex,
                                           std::shared_ptr<const oscillator::WavetableTable> table) noexcept
    {
        if (opIndex >= core::kNodesPerLayer)
            return;
        wavetableSharedA_[opIndex] = std::move(table);
        wavetableTablesA_[opIndex] = wavetableSharedA_[opIndex] ? wavetableSharedA_[opIndex].get() : nullptr;
    }

    void Engine::dispatchBlockMidiEvent(const BlockMidiEvent& ev) noexcept
    {
        switch (ev.type)
        {
            case BlockMidiType::NoteOn:
                noteOn(ev.note, ev.channel, ev.velocity);
                break;
            case BlockMidiType::NoteOff:
                noteOff(ev.note, ev.channel, ev.velocity);
                break;
            case BlockMidiType::PitchBend:
                pitchBend(ev.channel, ev.value);
                break;
            case BlockMidiType::ControlChange:
                controlChange(ev.channel, ev.controller, ev.value);
                break;
            case BlockMidiType::ChannelPressure:
                channelPressure(ev.channel, ev.value);
                break;
            case BlockMidiType::PolyAftertouch:
                polyAftertouch(ev.channel, ev.note, ev.velocity);
                break;
        }
    }

    void Engine::setModRoutesLive(const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& routes) noexcept
    {
        // Unlike every other Live Parameter API setter above, there is no per-voice
        // copy to also update: process() already reads patch_.layerA.modRoutes fresh
        // every sample (see Voice::renderSample's liveModRoutes parameter), so this
        // one write is the entire update -- and it reaches already-sustaining voices
        // immediately, not just the next note-on.
        patch_.layerA.modRoutes = routes;
    }

    void Engine::setLayerGainLive(float gain) noexcept
    {
        patch_.layerA.gain = gain;
        const float newOutputGain = layerOutputGain(patch_.layerA, patch_.voiceSettings.masterGain);
        for (auto& v : voices_)
            v.outputGain = newOutputGain;
        for (auto& v : voicesB_)
            v.outputGain = layerOutputGain(patch_.layerB, patch_.voiceSettings.masterGain);
    }

    void Engine::setLayerPanLive(float pan) noexcept
    {
        patch_.layerA.pan = pan;
        for (auto& v : voices_)
            v.pan = pan;
    }

    void Engine::setUnisonLive(const patch::UnisonSettings& unison) noexcept
    {
        patch_.layerA.unison = unison;
    }

    void Engine::setMasterGainLive(float masterGain) noexcept
    {
        patch_.voiceSettings.masterGain = masterGain;
        for (auto& v : voices_)
            v.outputGain = layerOutputGain(patch_.layerA, masterGain);
        for (auto& v : voicesB_)
            v.outputGain = layerOutputGain(patch_.layerB, masterGain);
    }

    void Engine::setMasterDynamicsLive(const patch::MasterDynamics& params) noexcept
    {
        patch_.masterDynamics = params;
    }

    void Engine::setGenerativeLive(const modulation::GenerativeParams& params) noexcept
    {
        patch_.generative = params;
        generativeProcessor_.reset(params, patch_.seed);
    }

    void Engine::setUtilityPeaksLive(const modulation::PeaksUtilityParams& params) noexcept
    {
        patch_.utilityPeaks = params;
    }

    void Engine::setPortamentoLive(float portamentoSeconds) noexcept
    {
        patch_.voiceSettings.portamentoSeconds = portamentoSeconds;
    }

    void Engine::setInsertEffectLive(std::size_t slot, const effects::EffectSlotParams& params) noexcept
    {
        if (slot < patch_.layerA.insertEffects.size())
            patch_.layerA.insertEffects[slot] = params;
    }

    void Engine::setMasterEffectLive(std::size_t slot, const effects::EffectSlotParams& params) noexcept
    {
        if (slot < patch_.masterEffects.size())
            patch_.masterEffects[slot] = params;
    }

    void Engine::setArpeggiatorScalarLive(const sequencer::ArpeggiatorParams& params) noexcept
    {
        auto merged = patch_.arpeggiator; // preserves .steps -- see the header doc comment.
        merged.enabled = params.enabled;
        merged.mode = params.mode;
        merged.rateMode = params.rateMode;
        merged.rateHz = params.rateHz;
        merged.syncDivisionIndex = params.syncDivisionIndex;
        merged.octaveRange = params.octaveRange;
        merged.numSteps = params.numSteps;
        merged.swing = params.swing;
        merged.latch = params.latch;
        patch_.arpeggiator = merged;
        arpeggiator_.setLiveParams(merged);
    }

    void Engine::setArpStepLive(std::size_t stepIndex, const sequencer::ArpStep& step) noexcept
    {
        if (stepIndex >= sequencer::kMaxArpSteps)
            return;
        patch_.arpeggiator.steps[stepIndex] = step;
        auto merged = patch_.arpeggiator;
        arpeggiator_.setLiveParams(merged);
    }

    void Engine::process(core::StereoBlockView output, const BlockMidiEvent* blockMidi, std::size_t blockMidiCount,
                          const float* sidechainLeft, const float* sidechainRight) noexcept
    {
        const dsp::ScopedDenormalGuard denormalGuard;
        const bool sidechainConnected = sidechainLeft != nullptr;

        output.clear();
        const auto numFrames = output.numFrames();
        const auto left = output.left();
        const auto right = output.right();

        std::size_t nextMidiIndex = 0;

        for (std::size_t subBlockStart = 0; subBlockStart < numFrames; subBlockStart += kVoiceSumSubBlockSize)
        {
            const std::size_t subBlockEnd =
                std::min(subBlockStart + kVoiceSumSubBlockSize, numFrames);

            const bool masterBusModActive =
                modulation::ModMatrixExecutor::hasActiveMasterBusRoutes(patch_.layerA.modRoutes)
                || patch_.masterDynamics.enabled;
            // moddedMasterEffects always needs a mutable copy: Quasar macro koins (applied
            // below, once per sub-block) mutate the master bus unconditionally regardless of
            // masterBusModActive. moddedInsertEffects/B only need a copy when master-bus mod
            // is actually active -- otherwise they alias patch_'s live arrays directly further
            // down (no copy, no per-sample recompute; this used to run per-sample, see below).
            std::array<effects::EffectSlotParams, effects::kNumMasterSlots> moddedMasterEffects =
                patch_.masterEffects;
            std::array<effects::EffectSlotParams, effects::kNumLayerInsertSlots> moddedInsertEffectsStorage;
            std::array<effects::EffectSlotParams, effects::kNumLayerInsertSlots> moddedInsertEffectsBStorage;
            float masterGainMul = 1.0f;
            float masterDynamicsMixMod = 0.0f;
            float sidechainDepthMod = 0.0f;

            modulation::GenerativeOutputValues subBlockGenerative{};
            const bool generativeRoutesActive =
                modulation::ModMatrixExecutor::hasActiveGenerativeRoutes(patch_.layerA.modRoutes);
            if (generativeRoutesActive)
                subBlockGenerative = generativeProcessor_.processSample(patch_.generative, patch_.seed);

            const bool gateHigh = [&]() {
                for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
                    if (voices_[i].envelopeIsActive(0))
                        return true;
                return false;
            }();
            peaksUtilityValues_ = peaksUtilityProcessor_.processSample(patch_.utilityPeaks, gateHigh);

            if (masterBusModActive)
            {
                std::array<float, core::kNumLfosPerLayer> masterModLfoValues{};
                for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                    masterModLfoValues[i] = layerLfosA_[i].renderSample(patch_.layerA.lfos[i], bpm_);

                const auto modSources =
                    buildMasterBusModSources(patch_, masterModLfoValues, channelModWheel_[0], channelExpression_[0],
                                             sidechainLevel_, subBlockGenerative);
                const auto masterMod =
                    modulation::ModMatrixExecutor::applyMasterBus(patch_.layerA.modRoutes, modSources);
                applyMasterModToEffects(moddedMasterEffects, masterMod);
                moddedInsertEffectsStorage = patch_.layerA.insertEffects;
                moddedInsertEffectsBStorage = patch_.layerB.insertEffects;
                applyInsertModToEffects(moddedInsertEffectsStorage, masterMod);
                applyInsertModToEffects(moddedInsertEffectsBStorage, masterMod);
                masterGainMul = masterGainMultiplier(patch_.voiceSettings.masterGain, masterMod.masterGainOffset);
                masterDynamicsMixMod = masterMod.masterDynamicsMixOffset;
                sidechainDepthMod = masterMod.sidechainDepthOffset;
            }

            // moddedInsertEffects/B alias whichever array is actually live for this
            // sub-block: the modulated copy above when master-bus mod is active, or
            // patch_'s own arrays directly (no copy) otherwise.
            const auto& moddedInsertEffects =
                masterBusModActive ? moddedInsertEffectsStorage : patch_.layerA.insertEffects;
            const auto& moddedInsertEffectsB =
                masterBusModActive ? moddedInsertEffectsBStorage : patch_.layerB.insertEffects;

            // Quasar macro koins apply to the master bus every sub-block regardless of
            // masterBusModActive -- moved here from inside the per-sample loop below, where
            // it previously ran (and, in the masterBusModActive==false case, also re-copied
            // patch_.masterEffects) once per *sample* even though orbitMacro/spreadMacro are
            // control-rate values that can't change within a sub-block.
            {
                const float orbitMacro = 0.5f + patch_.macros[3].value * 0.5f;
                const float spreadMacro = 0.5f + patch_.macros[6].value * 0.5f;
                applyQuasarMacroKoinsToMasterEffects(moddedMasterEffects, orbitMacro, spreadMacro);
            }

            float subBlockSynthPeak = 0.0f;
            float subBlockMasterPeak = 0.0f;
            std::array<float, core::kNodesPerLayer> subBlockOperatorPeaks{};
            subBlockOperatorPeaks.fill(0.0f);

            for (std::size_t s = subBlockStart; s < subBlockEnd; ++s)
            {
                const float sidechainSampleL =
                    sidechainLeft != nullptr ? dsp::flushIfNotFinite(sidechainLeft[s]) : 0.0f;
                const float sidechainSampleR =
                    sidechainRight != nullptr ? dsp::flushIfNotFinite(sidechainRight[s])
                                              : (sidechainLeft != nullptr ? sidechainSampleL : 0.0f);

                while (nextMidiIndex < blockMidiCount && blockMidi[nextMidiIndex].sampleOffset == s)
                {
                    dispatchBlockMidiEvent(blockMidi[nextMidiIndex]);
                    ++nextMidiIndex;
                }

                core::FixedVector<sequencer::ArpEvent, 32> arpEvents;
                arpeggiator_.tick(sampleRate_, bpm_, arpEvents);
                for (const auto& ev : arpEvents)
                {
                    const int velocity7 = dsp::clamp(static_cast<int>(ev.velocityUnit * 127.0f), 0, 127);
                    if (ev.type == sequencer::ArpEventType::NoteOn)
                        triggerNoteOnDirect(ev.note, ev.channel, velocity7);
                    else
                        triggerNoteOffDirect(ev.note, ev.channel, velocity7);
                }

                // The shared, layer-wide LFO tick (LAYER/GLOBAL-scoped mod routes) --
                // computed once per sample, identical for every voice this sample, unlike
                // each voice's own independent per-voice LFOs.
                std::array<float, core::kNumLfosPerLayer> layerLfoValues{};
                const bool layerLfoRoutesActive =
                    modulation::ModMatrixExecutor::hasActiveLayerLfoRoutes(patch_.layerA.modRoutes);
                if (layerLfoRoutesActive)
                {
                    for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                        layerLfoValues[i] = layerLfosA_[i].renderSample(patch_.layerA.lfos[i], bpm_);
                }

                float sumL = 0.0f;
                float sumR = 0.0f;
                float kahanL = 0.0f;
                float kahanR = 0.0f;
                for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
                {
                    float vl = 0.0f, vr = 0.0f;
                    std::array<float, core::kNodesPerLayer> voiceOperatorPeaks{};
                    voices_[i].renderSample(compiledLayerA_, wavetableTablesA_, bpm_, layerLfoValues, subBlockGenerative,
                                             patch_.layerA.modRoutes, patch_.layerA.metaRoutes, qualityMode_,
                                             sidechainSampleL, sidechainSampleR, vl, vr, &voiceOperatorPeaks);
                    for (std::size_t op = 0; op < core::kNodesPerLayer; ++op)
                        subBlockOperatorPeaks[op] = std::max(subBlockOperatorPeaks[op], voiceOperatorPeaks[op]);
                    kahanAdd(sumL, kahanL, vl);
                    kahanAdd(sumR, kahanR, vr);
                }

                if (compiledLayerA_.externalOp0DirectOutput)
                {
                    const auto& op0 = patch_.layerA.operators[0];
                    const float extMono =
                        dsp::flushIfNotFinite((sidechainSampleL + sidechainSampleR) * 0.5f * op0.level);
                    if (std::abs(extMono) > 1.0e-8f)
                    {
                        const float panClamped = dsp::clamp(op0.pan, -1.0f, 1.0f);
                        const float panRad = (panClamped * 0.5f + 0.5f) * (dsp::kPi * 0.5f);
                        kahanAdd(sumL, kahanL, extMono * std::cos(panRad));
                        kahanAdd(sumR, kahanR, extMono * std::sin(panRad));
                    }
                }

                subBlockSynthPeak =
                    std::max(subBlockSynthPeak, std::max(std::abs(sumL), std::abs(sumR)));

                const float preInsertTapL = sumL;
                const float preInsertTapR = sumR;

                const auto applyMasterGainMul = [&]() {
                    if (masterBusModActive)
                    {
                        sumL *= masterGainMul;
                        sumR *= masterGainMul;
                    }
                };

                const auto processLayerAInserts = [&]() {
                    layerAInsertChain_.process(moddedInsertEffects, patch_.fxProcessOrder.insert, sumL, sumR,
                                               sidechainSampleL, sidechainSampleR, sidechainConnected, bpm_);
                };

                const auto renderAndMergeLayerB = [&]() {
                    if (!isStackModeActive())
                        return;

                    std::array<float, core::kNumLfosPerLayer> layerLfoValuesB{};
                    const bool layerBLfoRoutesActive =
                        modulation::ModMatrixExecutor::hasActiveLayerLfoRoutes(patch_.layerB.modRoutes);
                    if (layerBLfoRoutesActive)
                    {
                        for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                            layerLfoValuesB[i] = layerLfosB_[i].renderSample(patch_.layerB.lfos[i], bpm_);
                    }

                    float sumBL = 0.0f;
                    float sumBR = 0.0f;
                    float kahanBL = 0.0f;
                    float kahanBR = 0.0f;
                    for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                    {
                        float vl = 0.0f, vr = 0.0f;
                        std::array<float, core::kNodesPerLayer> voiceOperatorPeaks{};
                        voicesB_[i].renderSample(compiledLayerB_, wavetableTablesB_, bpm_, layerLfoValuesB,
                                                  subBlockGenerative, patch_.layerB.modRoutes, patch_.layerB.metaRoutes,
                                                  qualityMode_, sidechainSampleL, sidechainSampleR, vl, vr,
                                                  &voiceOperatorPeaks);
                        for (std::size_t op = 0; op < core::kNodesPerLayer; ++op)
                            subBlockOperatorPeaks[op] = std::max(subBlockOperatorPeaks[op], voiceOperatorPeaks[op]);
                        kahanAdd(sumBL, kahanBL, vl);
                        kahanAdd(sumBR, kahanBR, vr);
                    }

                    if (!fxInsertsPostFader_)
                        layerBInsertChain_.process(moddedInsertEffectsB, patch_.fxProcessOrder.insert, sumBL, sumBR,
                                                   sidechainSampleL, sidechainSampleR, sidechainConnected, bpm_);
                    kahanAdd(sumL, kahanL, sumBL);
                    kahanAdd(sumR, kahanR, sumBR);
                };

                if (fxInsertsPostFader_)
                {
                    renderAndMergeLayerB();
                    applyMasterGainMul();
                    processLayerAInserts();
                }
                else
                {
                    processLayerAInserts();
                    renderAndMergeLayerB();
                    applyMasterGainMul();
                }

                const float sendTapL = fxInsertsPostFader_ ? sumL : preInsertTapL;
                const float sendTapR = fxInsertsPostFader_ ? sumR : preInsertTapR;

                const auto& masterFxForSend = moddedMasterEffects; // always the mod+koin'd sub-block copy now

                float sendMixL = 0.0f;
                float sendMixR = 0.0f;
                if (fxSendA_ > 1.0e-5f && masterFxForSend.size() > 0)
                {
                    const float auxL = sendTapL * fxSendA_;
                    const float auxR = sendTapR * fxSendA_;
                    float wetL = 0.0f;
                    float wetR = 0.0f;
                    sendReturnA_.processStereo(auxL, auxR, sidechainSampleL, sidechainSampleR, masterFxForSend[0], wetL,
                                               wetR, sidechainConnected, bpm_);
                    sendMixL += wetL;
                    sendMixR += wetR;
                }
                if (fxSendB_ > 1.0e-5f && masterFxForSend.size() > 1)
                {
                    const float auxL = sendTapL * fxSendB_;
                    const float auxR = sendTapR * fxSendB_;
                    float wetL = 0.0f;
                    float wetR = 0.0f;
                    sendReturnB_.processStereo(auxL, auxR, sidechainSampleL, sidechainSampleR, masterFxForSend[1], wetL,
                                               wetR, sidechainConnected, bpm_);
                    sendMixL += wetL;
                    sendMixR += wetR;
                }

                if (patch_.masterDynamics.enabled)
                {
                    bool gateNow = false;
                    for (std::size_t vi = 0; vi < allocator_.getPolyphony(); ++vi)
                    {
                        if (voices_[vi].gateOn)
                        {
                            gateNow = true;
                            break;
                        }
                    }
                    if (!gateNow && isStackModeActive())
                    {
                        for (std::size_t vi = 0; vi < allocatorB_.getPolyphony(); ++vi)
                        {
                            if (voicesB_[vi].gateOn)
                            {
                                gateNow = true;
                                break;
                            }
                        }
                    }
                    const bool triggerGate = gateNow && !dynamicsGatePrev_;
                    dynamicsGatePrev_ = gateNow;

                    dynamics::MasterDynamicsParams dynParams{};
                    dynParams.enabled = patch_.masterDynamics.enabled;
                    dynParams.mode = static_cast<dynamics::MasterDynamicsMode>(patch_.masterDynamics.mode);
                    dynParams.thresholdDb = patch_.masterDynamics.thresholdDb;
                    dynParams.ratio = patch_.masterDynamics.ratio;
                    dynParams.attackMs = patch_.masterDynamics.attackMs;
                    dynParams.releaseMs = patch_.masterDynamics.releaseMs;
                    dynParams.sidechainGain = patch_.masterDynamics.sidechainGain;
                    dynParams.vactrolSlewMs = patch_.masterDynamics.vactrolSlewMs;
                    dynParams.makeupDb = patch_.masterDynamics.makeupDb;
                    dynParams.mix = patch_.masterDynamics.mix;

                    masterDynamicsProcessor_.processSample(sumL, sumR, sidechainSampleL, sidechainSampleR, triggerGate,
                                                             dynParams, masterDynamicsMixMod, sidechainDepthMod);
                }
                else
                {
                    dynamicsGatePrev_ = false;
                }

                // moddedMasterEffects is already mod-applied (if active) and Quasar-koin'd,
                // computed once per sub-block above -- no per-sample branch needed anymore.
                masterChain_.process(moddedMasterEffects, patch_.fxProcessOrder.master, sumL, sumR, sidechainSampleL,
                                     sidechainSampleR, sidechainConnected, bpm_);

                sumL += sendMixL;
                sumR += sendMixR;

                left[s] = sumL;
                right[s] = sumR;
                subBlockMasterPeak =
                    std::max(subBlockMasterPeak, std::max(std::abs(sumL), std::abs(sumR)));
            }

            updatePeakHold(synthBusPeak_, subBlockSynthPeak);
            updatePeakHold(masterOutPeak_, subBlockMasterPeak);
            for (std::size_t op = 0; op < core::kNodesPerLayer; ++op)
                updatePeakHold(operatorPeaks_[op], subBlockOperatorPeaks[op]);
        }
    }

    void Engine::updatePeakHold(std::atomic<float>& hold, float blockPeak) noexcept
    {
        const float prev = hold.load(std::memory_order_relaxed);
        const float decayed = prev * 0.86f;
        hold.store(std::max(blockPeak, decayed), std::memory_order_relaxed);
    }

    std::size_t Engine::countActiveVoices() const noexcept
    {
        const auto countPool = [](const voice::VoicePool& voices, std::size_t poly) -> std::size_t {
            std::size_t active = 0;
            for (std::size_t i = 0; i < poly; ++i)
            {
                if (voices[i].gateOn || voices[i].isSounding())
                    ++active;
            }
            return active;
        };

        std::size_t total = countPool(voices_, allocator_.getPolyphony());
        if (isStackModeActive())
            total += countPool(voicesB_, allocatorB_.getPolyphony());
        return total;
    }

} // namespace pw8::render
