#include "pw8/render/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "pw8/render/BlockMidi.hpp"
#include "pw8/render/Engine.hpp"

namespace pw8::render
{
    namespace
    {
        BlockMidiEvent toBlockEvent(const midi::MidiEvent& e) noexcept
        {
            BlockMidiEvent out{};
            out.channel = e.channel;
            out.note = e.note;
            out.velocity = e.velocity;
            out.controller = e.controller;
            out.value = e.value;
            switch (e.type)
            {
                case midi::EventType::NoteOn: out.type = BlockMidiType::NoteOn; break;
                case midi::EventType::NoteOff: out.type = BlockMidiType::NoteOff; break;
                case midi::EventType::PitchBend: out.type = BlockMidiType::PitchBend; break;
                case midi::EventType::ControlChange: out.type = BlockMidiType::ControlChange; break;
                case midi::EventType::ChannelPressure: out.type = BlockMidiType::ChannelPressure; break;
                case midi::EventType::PolyAftertouch: out.type = BlockMidiType::PolyAftertouch; break;
                case midi::EventType::ProgramChange: break;
            }
            return out;
        }

        RenderMetrics computeMetrics(const std::vector<float>& interleaved, double sampleRate) noexcept
        {
            RenderMetrics m;
            const std::size_t numFrames = interleaved.size() / 2;
            if (numFrames == 0)
                return m;

            double sumSqL = 0.0, sumSqR = 0.0, sumL = 0.0, sumR = 0.0;
            float peak = 0.0f;

            for (std::size_t i = 0; i < numFrames; ++i)
            {
                const float l = interleaved[i * 2];
                const float r = interleaved[i * 2 + 1];

                if (!std::isfinite(l) || !std::isfinite(r))
                    m.containsNaNOrInf = true;

                peak = std::max({peak, std::abs(l), std::abs(r)});
                sumSqL += static_cast<double>(l) * l;
                sumSqR += static_cast<double>(r) * r;
                sumL += l;
                sumR += r;
            }

            m.peak = peak;
            m.rms = static_cast<float>(std::sqrt((sumSqL + sumSqR) / (2.0 * static_cast<double>(numFrames))));
            m.dcOffsetLeft = static_cast<float>(sumL / static_cast<double>(numFrames));
            m.dcOffsetRight = static_cast<float>(sumR / static_cast<double>(numFrames));
            m.durationSeconds = static_cast<double>(numFrames) / sampleRate;

            const double rmsL = std::sqrt(sumSqL / static_cast<double>(numFrames));
            const double rmsR = std::sqrt(sumSqR / static_cast<double>(numFrames));
            const double total = rmsL + rmsR;
            m.leftRightBalance = total > 1.0e-12 ? static_cast<float>((rmsR - rmsL) / total) : 0.0f;

            return m;
        }
    } // namespace

    RenderResult renderWithEngine(Engine& engine, const midi::MidiSequence& midi, const RenderOptions& options) noexcept
    {
        RenderResult result;
        result.sampleRate = options.sampleRate;

        if (options.sampleRate < 8000.0 || options.sampleRate > 384000.0)
        {
            result.error = "sampleRate out of supported range";
            return result;
        }
        if (options.blockSize <= 0 || options.blockSize > 65536)
        {
            result.error = "blockSize out of supported range";
            return result;
        }
        if (std::abs(engine.getSampleRate() - options.sampleRate) > 0.5)
        {
            result.error = "engine was prepared at a different sample rate than options.sampleRate";
            return result;
        }

        engine.setTempo(static_cast<float>(options.bpm));
        engine.setQualityMode(options.quality);

        double totalSeconds = options.durationSecondsOverride;
        if (totalSeconds < 0.0)
            totalSeconds = midi.durationSeconds() + options.releaseTailSeconds;
        totalSeconds = std::max(0.0, std::min(totalSeconds, 3600.0)); // 1-hour sanity ceiling.

        const auto totalFrames = static_cast<std::size_t>(totalSeconds * options.sampleRate);

        result.interleavedStereo.assign(totalFrames * 2, 0.0f);

        std::vector<float> blockL(static_cast<std::size_t>(options.blockSize));
        std::vector<float> blockR(static_cast<std::size_t>(options.blockSize));

        std::size_t nextEventIndex = 0;
        const auto& events = midi.events;

        std::size_t framesRendered = 0;
        while (framesRendered < totalFrames)
        {
            const std::size_t framesThisBlock =
                std::min(static_cast<std::size_t>(options.blockSize), totalFrames - framesRendered);

            std::vector<BlockMidiEvent> blockEvents;
            blockEvents.reserve(32);
            while (nextEventIndex < events.size())
            {
                const auto& ev = events[nextEventIndex];
                const auto eventFrame = static_cast<std::size_t>(ev.timeSeconds * options.sampleRate + 0.5);
                if (eventFrame >= framesRendered + framesThisBlock)
                    break;
                if (ev.type == midi::EventType::ProgramChange)
                {
                    ++nextEventIndex;
                    continue;
                }
                auto blockEv = toBlockEvent(ev);
                blockEv.sampleOffset = eventFrame - framesRendered;
                blockEvents.push_back(blockEv);
                ++nextEventIndex;
            }

            core::StereoBlockView view(blockL.data(), blockR.data(), framesThisBlock);
            const float* sidechainLeft = nullptr;
            const float* sidechainRight = nullptr;
            std::vector<float> sidechainBlockL;
            std::vector<float> sidechainBlockR;
            if (options.simulateSidechain && framesThisBlock > 0)
            {
                sidechainBlockL.resize(framesThisBlock);
                sidechainBlockR.resize(framesThisBlock);
                for (std::size_t i = 0; i < framesThisBlock; ++i)
                {
                    const double t =
                        static_cast<double>(framesRendered + i) / options.sampleRate;
                    const float mod = options.sidechainModulatorAmp *
                                      std::sin(2.0 * M_PI * static_cast<double>(options.sidechainModulatorHz) * t);
                    sidechainBlockL[i] = mod;
                    sidechainBlockR[i] = mod;
                }
                sidechainLeft = sidechainBlockL.data();
                sidechainRight = sidechainBlockR.data();
            }
            engine.process(view, blockEvents.empty() ? nullptr : blockEvents.data(), blockEvents.size(),
                           sidechainLeft, sidechainRight);

            for (std::size_t i = 0; i < framesThisBlock; ++i)
            {
                result.interleavedStereo[(framesRendered + i) * 2] = blockL[i];
                result.interleavedStereo[(framesRendered + i) * 2 + 1] = blockR[i];
            }

            framesRendered += framesThisBlock;
        }

        result.metrics = computeMetrics(result.interleavedStereo, options.sampleRate);
        result.ok = true;
        return result;
    }

    RenderResult render(const patch::Patch& patchToRender, const midi::MidiSequence& midi,
                         const RenderOptions& options) noexcept
    {
        // Engine must be heap-allocated, not stack-allocated -- sizeof(Engine) is
        // ~19MB (fixed-size voice/wavetable/effect-chain storage), well past the
        // default 8MB thread stack on stock Linux (confirmed via a real crash on a
        // bare Ubuntu 24.04 box: instant SIGSEGV on function entry, before a single
        // line of render() ran). The JUCE plugin has always heap-allocated every
        // Engine instance via std::make_unique (see MurmurProcessor.cpp) for exactly
        // this reason -- this free function was the one place that didn't.
        auto engine = std::make_unique<Engine>();
        engine->prepare(options.sampleRate);
        engine->loadPatch(patchToRender); // Compile failure already falls back to a safe graph.
        return renderWithEngine(*engine, midi, options);
    }

} // namespace pw8::render
