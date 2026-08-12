#include "pw8/render/Engine.hpp"

#include "pw8/content/ContentPaths.hpp"
#include "pw8/dsp/Denormal.hpp"
#include "pw8/dsp/Math.hpp"
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
        return out;
    }

    void Engine::loadLayerResources(const patch::LayerPatch& layer, algorithm::CompiledAlgorithm& compiledOut,
                                       std::array<op::OperatorParams, core::kNodesPerLayer>& templatesOut,
                                       std::array<std::optional<oscillator::WavetableTable>, core::kNodesPerLayer>& storageOut,
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

        for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
        {
            storageOut[i].reset();
            const auto& op = layer.operators[i];
            if ((op.engine == algorithm::EngineType::Wavetable || op.engine == algorithm::EngineType::Granular) &&
                !op.wavetableId.empty())
            {
                const auto resolved = content::resolveWavetablePath(op.wavetableId);
                const auto& pathToLoad = resolved.has_value() ? *resolved : op.wavetableId;
                auto loadResult = oscillator::loadWavetableFromFile(pathToLoad);
                if (loadResult.ok)
                    storageOut[i] = std::move(loadResult.table);
            }
            tablesOut[i] = storageOut[i].has_value() ? &*storageOut[i] : nullptr;
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
            v.lfoParams = layer.lfos;
            v.macroValues = macroValues;
        }
    }

    bool Engine::loadPatch(const patch::Patch& patchToLoad) noexcept
    {
        patch_ = patchToLoad;

        // Only SingleA and Stack are rendered today; other layer modes stay clamped.
        if (patch_.layerMode != patch::LayerMode::SingleA && patch_.layerMode != patch::LayerMode::Stack)
            patch_.layerMode = patch::LayerMode::SingleA;

        const bool stackActive = patch_.layerMode == patch::LayerMode::Stack;

        algorithm::CompileStatus statusA = algorithm::CompileStatus::Ok;
        loadLayerResources(patch_.layerA, compiledLayerA_, operatorParamsTemplateA_, wavetableStorageA_,
                           wavetableTablesA_, layerLfosA_, voices_, statusA);

        algorithm::CompileStatus statusB = algorithm::CompileStatus::Ok;
        if (stackActive)
        {
            loadLayerResources(patch_.layerB, compiledLayerB_, operatorParamsTemplateB_, wavetableStorageB_,
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
        const float gainScale = unison.blend / static_cast<float>(unisonVoices);

        for (int u = 0; u < unisonVoices; ++u)
        {
            const auto idx = allocator.allocate(voices);
            auto& v = voices[idx];
            v.id = static_cast<std::uint32_t>(idx);
            v.operatorParams = templates;
            v.filterParams = layer.filter1;
            v.lfoParams = layer.lfos;
            for (std::size_t i = 0; i < v.macroValues.size(); ++i)
                v.macroValues[i] = patch_.macros[i].value;

            float detuneCents = 0.0f;
            float unisonPan = 0.0f;
            unisonSpread(unison, u, unisonVoices, detuneCents, unisonPan);

            const float detunedHz = baseFreqHz * centsToRatio(detuneCents);
            v.noteOn(note, channel, velUnit, detunedHz, layer.envelopes, allocator.nextAge(), ++noteGenerationCounter_,
                     patch_.seed ^ static_cast<std::uint64_t>(u + 1));
            v.outputGain = layer.gain * patch_.voiceSettings.masterGain * gainScale;
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

        triggerLayerNoteOn(patch_.layerA, patch_.layerA.unison, operatorParamsTemplateA_, voices_, allocator_, note,
                           channel, velocity7);

        if (isStackModeActive())
            triggerLayerNoteOn(patch_.layerB, patch_.layerB.unison, operatorParamsTemplateB_, voicesB_, allocatorB_,
                               note, channel, velocity7);
    }

    void Engine::triggerNoteOffDirect(int note, int channel, int velocity7) noexcept
    {
        const float relVel = static_cast<float>(velocity7) / 127.0f;
        allocator_.release(voices_, note, channel, relVel);
        if (isStackModeActive())
            allocatorB_.release(voicesB_, note, channel, relVel);
    }

    void Engine::pitchBend(int channel, int value14) noexcept
    {
        const float normalized = (static_cast<float>(value14) - 8192.0f) / 8192.0f; // -1..~1
        const float semitones = normalized * kPitchBendRangeSemitones;
        for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                voices_[i].expression.pitchBendSemitones = semitones;
        if (isStackModeActive())
        {
            for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                if (voicesB_[i].gateOn && voicesB_[i].midiChannel == channel)
                    voicesB_[i].expression.pitchBendSemitones = semitones;
        }
    }

    void Engine::controlChange(int channel, int controller, int value7) noexcept
    {
        // Sustain pedal and full CC routing through the mod matrix are PLANNED
        // (docs/MODULATION.md). Only CC74 (MPE slide/timbre) is captured today since
        // NoteExpression already has a field for it and it costs nothing extra to wire.
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

    void Engine::allNotesOff() noexcept
    {
        allocator_.releaseAll(voices_, 0.5f);
        if (isStackModeActive())
            allocatorB_.releaseAll(voicesB_, 0.5f);
    }

    void Engine::setMacroValue(std::size_t index, float value) noexcept
    {
        if (index >= patch_.macros.size())
            return;

        patch_.macros[index].value = value;
        for (auto& v : voices_)
            if (index < v.macroValues.size())
                v.macroValues[index] = value;
        for (auto& v : voicesB_)
            if (index < v.macroValues.size())
                v.macroValues[index] = value;
    }

    void Engine::setFilterLive(const filter::FilterParams& params) noexcept
    {
        patch_.layerA.filter1 = params;
        for (auto& v : voices_)
            v.filterParams = params;
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
        stored.engine = params.engine;
        stored.classicWaveform = params.classic.waveform;
        stored.classicMorph = params.classic.morph;
        stored.pulseWidth = params.classic.pulseWidth;
        stored.wavetableFramePosition = params.wavetableFramePosition;
        stored.frequencyRatio = params.frequencyRatio;
        stored.fixedFrequencyHz = params.fixedFrequencyHz;
        stored.keyTrack = params.keyTrack;
        stored.level = params.level;
    }

    void Engine::setEnvelopeLive(std::size_t envIndex, const envelope::DahdsrParams& params) noexcept
    {
        if (envIndex >= core::kNumEnvelopesPerLayer)
            return;
        patch_.layerA.envelopes[envIndex] = params;
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
        const float newOutputGain = gain * patch_.voiceSettings.masterGain;
        for (auto& v : voices_)
            v.outputGain = newOutputGain;
    }

    void Engine::setLayerPanLive(float pan) noexcept
    {
        patch_.layerA.pan = pan;
        for (auto& v : voices_)
            v.pan = pan;
    }

    void Engine::setMasterGainLive(float masterGain) noexcept
    {
        patch_.voiceSettings.masterGain = masterGain;
        const float newOutputGain = patch_.layerA.gain * masterGain;
        for (auto& v : voices_)
            v.outputGain = newOutputGain;
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
        merged.latch = params.latch;
        patch_.arpeggiator = merged;
        arpeggiator_.setLiveParams(merged);
    }

    void Engine::process(core::StereoBlockView output) noexcept
    {
        const dsp::ScopedDenormalGuard denormalGuard;

        output.clear();
        const auto numFrames = output.numFrames();
        const auto left = output.left();
        const auto right = output.right();

        for (std::size_t s = 0; s < numFrames; ++s)
        {
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
            for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                layerLfoValues[i] = layerLfosA_[i].renderSample(patch_.layerA.lfos[i], bpm_);

            float sumL = 0.0f;
            float sumR = 0.0f;
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            {
                float vl = 0.0f, vr = 0.0f;
                voices_[i].renderSample(compiledLayerA_, wavetableTablesA_, bpm_, layerLfoValues,
                                         patch_.layerA.modRoutes, vl, vr);
                sumL += vl;
                sumR += vr;
            }

            layerAInsertChain_.process(patch_.layerA.insertEffects, sumL, sumR);

            if (isStackModeActive())
            {
                std::array<float, core::kNumLfosPerLayer> layerLfoValuesB{};
                for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                    layerLfoValuesB[i] = layerLfosB_[i].renderSample(patch_.layerB.lfos[i], bpm_);

                float sumBL = 0.0f;
                float sumBR = 0.0f;
                for (std::size_t i = 0; i < allocatorB_.getPolyphony(); ++i)
                {
                    float vl = 0.0f, vr = 0.0f;
                    voicesB_[i].renderSample(compiledLayerB_, wavetableTablesB_, bpm_, layerLfoValuesB,
                                              patch_.layerB.modRoutes, vl, vr);
                    sumBL += vl;
                    sumBR += vr;
                }

                layerBInsertChain_.process(patch_.layerB.insertEffects, sumBL, sumBR);
                sumL += sumBL;
                sumR += sumBR;
            }

            masterChain_.process(patch_.masterEffects, sumL, sumR);

            left[s] = sumL;
            right[s] = sumR;
        }
    }

} // namespace pw8::render
