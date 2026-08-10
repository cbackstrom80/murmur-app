#include "pw8/render/Renderer.hpp"

#include <algorithm>
#include <cmath>

#include "pw8/render/Engine.hpp"

namespace pw8::render
{
    namespace
    {
        void dispatchEvent(Engine& engine, const midi::MidiEvent& e) noexcept
        {
            switch (e.type)
            {
                case midi::EventType::NoteOn:
                    engine.noteOn(e.note, e.channel, e.velocity);
                    break;
                case midi::EventType::NoteOff:
                    engine.noteOff(e.note, e.channel, e.velocity);
                    break;
                case midi::EventType::PitchBend:
                    engine.pitchBend(e.channel, e.value);
                    break;
                case midi::EventType::ControlChange:
                    engine.controlChange(e.channel, e.controller, e.value);
                    break;
                case midi::EventType::ChannelPressure:
                    engine.channelPressure(e.channel, e.value);
                    break;
                case midi::EventType::PolyAftertouch:
                    engine.polyAftertouch(e.channel, e.note, e.velocity);
                    break;
                case midi::EventType::ProgramChange:
                    break; // Patch-per-program-change is PLANNED (Patchwork integration concern).
            }
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

            // Events are sorted ascending and nextEventIndex only moves forward, so any
            // event not yet dispatched is guaranteed to fall at or after this block's start.
            const double blockEndSeconds = static_cast<double>(framesRendered + framesThisBlock) / options.sampleRate;

            while (nextEventIndex < events.size() && events[nextEventIndex].timeSeconds < blockEndSeconds)
            {
                dispatchEvent(engine, events[nextEventIndex]);
                ++nextEventIndex;
            }

            core::StereoBlockView view(blockL.data(), blockR.data(), framesThisBlock);
            engine.process(view);

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
        Engine engine;
        engine.prepare(options.sampleRate);
        engine.loadPatch(patchToRender); // Compile failure already falls back to a safe graph.
        return renderWithEngine(engine, midi, options);
    }

} // namespace pw8::render
