#include "pw8/render/Engine.hpp"

#include "pw8/dsp/Denormal.hpp"
#include "pw8/oscillator/WavetableTableLoader.hpp"

namespace pw8::render
{
    void Engine::prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        for (auto& v : voices_)
            v.prepare(sampleRate_);
        for (auto& l : layerLfosA_)
            l.prepare(sampleRate_);
        arpeggiator_.prepare(sampleRate_);
        layerAInsertChain_.prepare(sampleRate_);
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
        out.grainDensity = p.grainDensity;
        out.grainSizeMs = p.grainSizeMs;
        out.grainPositionJitter = p.grainPositionJitter;
        out.grainPitchJitter = p.grainPitchJitter;
        return out;
    }

    bool Engine::loadPatch(const patch::Patch& patchToLoad) noexcept
    {
        patch_ = patchToLoad;

        algorithm::CompiledAlgorithm compiled;
        const auto status = algorithm::AlgorithmGraphCompiler::compile(patch_.layerA.algorithm, compiled);
        lastCompileStatus_ = status;

        if (status == algorithm::CompileStatus::Ok)
        {
            compiledLayerA_ = compiled;
        }
        else
        {
            // Never leave the audio thread without a valid compiled algorithm: fall back
            // to a known-safe default (single sine carrier) rather than propagating an
            // invalid graph. See docs/ALGORITHM_GRAPH.md "Graph Validation".
            algorithm::CompiledAlgorithm fallback;
            [[maybe_unused]] const auto fallbackStatus =
                algorithm::AlgorithmGraphCompiler::compile(algorithm::AlgorithmGraphDefinition::makeDefaultParallel8(), fallback);
            compiledLayerA_ = fallback;
        }

        for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
            operatorParamsTemplateA_[i] = toOperatorParams(patch_.layerA.operators[i]);

        // Load referenced wavetable content. `wavetableId` is currently treated as a
        // filesystem path (relative to the process's working directory, or absolute) --
        // see docs/PATCH_FORMAT.md "Wavetable Resource Resolution" for why this is
        // PARTIAL rather than a full content-addressed resolution system. A missing or
        // malformed file just leaves that operator silent (documented WavetableOscillator
        // behavior for an invalid view), not a load failure for the whole patch.
        for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
        {
            wavetableStorageA_[i].reset();
            const auto& op = patch_.layerA.operators[i];
            if (op.engine == algorithm::EngineType::Wavetable && !op.wavetableId.empty())
            {
                auto loadResult = oscillator::loadWavetableFromFile(op.wavetableId);
                if (loadResult.ok)
                    wavetableStorageA_[i] = std::move(loadResult.table);
            }
            wavetableTablesA_[i] = wavetableStorageA_[i].has_value() ? &*wavetableStorageA_[i] : nullptr;
        }

        allocator_.configure(patch_.voiceSettings.polyphony);
        tuning_.setA4(patch_.voiceSettings.a4Hz);
        arpeggiator_.configure(patch_.arpeggiator, patch_.seed);

        // Clear delay/chorus tails on patch load rather than letting a previous
        // patch's effect state bleed into the newly loaded one -- keeps rendering
        // deterministic from a fresh loadPatch() regardless of what was loaded before.
        layerAInsertChain_.reset();
        masterChain_.reset();

        // The shared, layer-wide LFO bank (LAYER/GLOBAL-scoped mod routes, see
        // ModScope's doc comment in ModMatrixTypes.hpp): initialized once here rather
        // than per-note, since there's no single "note-on" for a layer-wide
        // modulator. Deterministically seeded from the patch seed + LFO index.
        for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
            layerLfosA_[i].noteOn(patch_.layerA.lfos[i],
                                   dsp::DeterministicRng::deriveSeed(patch_.seed, static_cast<std::uint32_t>(i), patch_.seed));

        std::array<float, 8> macroValues{};
        for (std::size_t i = 0; i < macroValues.size(); ++i)
            macroValues[i] = patch_.macros[i].value;

        for (auto& v : voices_)
        {
            v.operatorParams = operatorParamsTemplateA_;
            v.filterParams = patch_.layerA.filter1;
            v.lfoParams = patch_.layerA.lfos;
            v.macroValues = macroValues;
        }

        return status == algorithm::CompileStatus::Ok;
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

        const auto idx = allocator_.allocate(voices_);
        auto& v = voices_[idx];
        v.id = static_cast<std::uint32_t>(idx);
        v.operatorParams = operatorParamsTemplateA_;
        v.filterParams = patch_.layerA.filter1;
        v.lfoParams = patch_.layerA.lfos;
        for (std::size_t i = 0; i < v.macroValues.size(); ++i)
            v.macroValues[i] = patch_.macros[i].value;

        const float freqHz = tuning_.noteToFrequency(static_cast<float>(note));
        const float velUnit = static_cast<float>(velocity7) / 127.0f;

        v.noteOn(note, channel, velUnit, freqHz, patch_.layerA.envelopes, allocator_.nextAge(),
                 ++noteGenerationCounter_, patch_.seed);
        v.outputGain = patch_.layerA.gain * patch_.voiceSettings.masterGain;
        v.pan = patch_.layerA.pan;
    }

    void Engine::triggerNoteOffDirect(int note, int channel, int velocity7) noexcept
    {
        const float relVel = static_cast<float>(velocity7) / 127.0f;
        allocator_.release(voices_, note, channel, relVel);
    }

    void Engine::pitchBend(int channel, int value14) noexcept
    {
        const float normalized = (static_cast<float>(value14) - 8192.0f) / 8192.0f; // -1..~1
        const float semitones = normalized * kPitchBendRangeSemitones;
        for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                voices_[i].expression.pitchBendSemitones = semitones;
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
        }
    }

    void Engine::channelPressure(int channel, int value7) noexcept
    {
        const float v = static_cast<float>(value7) / 127.0f;
        for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            if (voices_[i].gateOn && voices_[i].midiChannel == channel)
                voices_[i].expression.channelPressure = v;
    }

    void Engine::polyAftertouch(int channel, int note, int value7) noexcept
    {
        const float v = static_cast<float>(value7) / 127.0f;
        for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            if (voices_[i].gateOn && voices_[i].midiChannel == channel && voices_[i].noteNumber == note)
                voices_[i].expression.polyAftertouch = v;
    }

    void Engine::allNotesOff() noexcept
    {
        allocator_.releaseAll(voices_, 0.5f);
    }

    void Engine::setMacroValue(std::size_t index, float value) noexcept
    {
        if (index >= patch_.macros.size())
            return;

        patch_.macros[index].value = value;
        for (auto& v : voices_)
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

            // Layer A's 3 insert slots, then the engine-wide 4 master slots (see
            // docs/FX_BANK.md). Layer B isn't voiced yet (Phase 8), so there is
            // nothing to sum in or run its own insert chain on in this pass.
            layerAInsertChain_.process(patch_.layerA.insertEffects, sumL, sumR);
            masterChain_.process(patch_.masterEffects, sumL, sumR);

            left[s] = sumL;
            right[s] = sumR;
        }
    }

} // namespace pw8::render
