#include "pw8/render/Engine.hpp"

#include "pw8/dsp/Denormal.hpp"

namespace pw8::render
{
    void Engine::prepare(double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        for (auto& v : voices_)
            v.prepare(sampleRate_);
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

        for (auto& wt : wavetablesA_)
            wt = oscillator::WavetableView{}; // Wavetable content loading is PLANNED; empty view renders silence.

        allocator_.configure(patch_.voiceSettings.polyphony);
        tuning_.setA4(patch_.voiceSettings.a4Hz);

        for (auto& v : voices_)
            v.operatorParams = operatorParamsTemplateA_;

        return status == algorithm::CompileStatus::Ok;
    }

    void Engine::noteOn(int note, int channel, int velocity7) noexcept
    {
        if (velocity7 <= 0)
        {
            noteOff(note, channel, 0);
            return;
        }

        const auto idx = allocator_.allocate(voices_);
        auto& v = voices_[idx];
        v.id = static_cast<std::uint32_t>(idx);
        v.operatorParams = operatorParamsTemplateA_;

        const float freqHz = tuning_.noteToFrequency(static_cast<float>(note));
        const float velUnit = static_cast<float>(velocity7) / 127.0f;

        v.noteOn(note, channel, velUnit, freqHz, patch_.layerA.ampEnvelope, allocator_.nextAge(),
                 ++noteGenerationCounter_, patch_.seed);
        v.outputGain = patch_.layerA.gain * patch_.voiceSettings.masterGain;
        v.pan = patch_.layerA.pan;
    }

    void Engine::noteOff(int note, int channel, int velocity7) noexcept
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

    void Engine::process(core::StereoBlockView output) noexcept
    {
        const dsp::ScopedDenormalGuard denormalGuard;

        output.clear();
        const auto numFrames = output.numFrames();
        const auto left = output.left();
        const auto right = output.right();

        for (std::size_t s = 0; s < numFrames; ++s)
        {
            float sumL = 0.0f;
            float sumR = 0.0f;
            for (std::size_t i = 0; i < allocator_.getPolyphony(); ++i)
            {
                float vl = 0.0f, vr = 0.0f;
                voices_[i].renderSample(compiledLayerA_, wavetablesA_, vl, vr);
                sumL += vl;
                sumR += vr;
            }
            left[s] = sumL;
            right[s] = sumR;
        }
    }

} // namespace pw8::render
