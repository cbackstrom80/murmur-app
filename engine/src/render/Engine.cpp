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

        std::array<float, 8> macroValues{};
        for (std::size_t i = 0; i < macroValues.size(); ++i)
            macroValues[i] = patch_.macros[i].value;

        for (auto& v : voices_)
        {
            v.operatorParams = operatorParamsTemplateA_;
            v.filterParams = patch_.layerA.filter1;
            v.lfoParams = patch_.layerA.lfo1;
            v.modRoutes = patch_.layerA.modRoutes;
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
        v.lfoParams = patch_.layerA.lfo1;
        v.modRoutes = patch_.layerA.modRoutes;
        for (std::size_t i = 0; i < v.macroValues.size(); ++i)
            v.macroValues[i] = patch_.macros[i].value;

        const float freqHz = tuning_.noteToFrequency(static_cast<float>(note));
        const float velUnit = static_cast<float>(velocity7) / 127.0f;

        v.noteOn(note, channel, velUnit, freqHz, patch_.layerA.ampEnvelope, allocator_.nextAge(),
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

            float sumL = 0.0f;
            float sumR = 0.0f;
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            {
                float vl = 0.0f, vr = 0.0f;
                voices_[i].renderSample(compiledLayerA_, wavetableTablesA_, bpm_, vl, vr);
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
