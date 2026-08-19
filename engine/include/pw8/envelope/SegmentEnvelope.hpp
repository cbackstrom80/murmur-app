#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <string>

#include "pw8/dsp/Math.hpp"
#include "pw8/envelope/DahdsrEnvelope.hpp"
#include "pw8/modulation/MorphEasing.hpp"

// Stages-inspired segment envelope chain (Track D — docs/MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md §8).
// Header-only executor: up to 6 segments per slot with loop markers and per-segment easing.
namespace pw8::envelope
{
    enum class SegmentType : std::uint8_t
    {
        Ramp = 0,
        Hold,
        Step,
        Loop,
    };

    inline constexpr std::size_t kMaxSegmentEnvelopeSegments = 6;

    struct SegmentEnvelopeSegment
    {
        SegmentType type = SegmentType::Ramp;
        float durationMs = 100.0f;
        float level = 1.0f;
        std::string shape; ///< Morph easing name; empty = linear
    };

    struct SegmentEnvelopeChain
    {
        std::array<SegmentEnvelopeSegment, kMaxSegmentEnvelopeSegments> segments{};
        std::size_t segmentCount = 0;
        std::size_t loopStart = 0;
        std::size_t loopEnd = 0;

        [[nodiscard]] bool isActive() const noexcept { return segmentCount > 0; }
    };

    [[nodiscard]] inline SegmentType parseSegmentType(const std::string& s) noexcept
    {
        if (s == "hold")
            return SegmentType::Hold;
        if (s == "step")
            return SegmentType::Step;
        if (s == "loop")
            return SegmentType::Loop;
        return SegmentType::Ramp;
    }

    [[nodiscard]] inline const char* segmentTypeToString(SegmentType type) noexcept
    {
        switch (type)
        {
            case SegmentType::Hold: return "hold";
            case SegmentType::Step: return "step";
            case SegmentType::Loop: return "loop";
            case SegmentType::Ramp:
            default: return "ramp";
        }
    }

    [[nodiscard]] inline const char* segmentTypeLabel(SegmentType type) noexcept
    {
        switch (type)
        {
            case SegmentType::Hold: return "HLD";
            case SegmentType::Step: return "STP";
            case SegmentType::Loop: return "LOP";
            case SegmentType::Ramp:
            default: return "RMP";
        }
    }

    [[nodiscard]] inline SegmentType cycleSegmentType(SegmentType type) noexcept
    {
        return static_cast<SegmentType>((static_cast<std::uint8_t>(type) + 1) % 4);
    }

    class SegmentEnvelope
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        }

        void noteOn(const SegmentEnvelopeChain& chain, float releaseSeconds) noexcept
        {
            chain_ = chain;
            releaseSeconds_ = releaseSeconds > 0.0f ? releaseSeconds : 0.3f;
            gateOn_ = true;
            inRelease_ = false;
            segmentIndex_ = 0;
            samplesIntoSegment_ = 0;
            startLevel_ = 0.0f;
            currentLevel_ = 0.0f;
            stageLengthSamples_ = segmentLengthSamples(0);
            if (chain_.segmentCount > 0 && chain_.segments[0].type == SegmentType::Step)
                currentLevel_ = chain_.segments[0].level;
        }

        void noteOff() noexcept
        {
            if (!isActive())
                return;
            gateOn_ = false;
            if (currentLevel_ <= 0.0f)
            {
                reset();
                return;
            }
            inRelease_ = true;
            releaseStartLevel_ = currentLevel_;
            samplesIntoSegment_ = 0;
            stageLengthSamples_ = secondsToSamples(releaseSeconds_);
        }

        void reset() noexcept
        {
            gateOn_ = false;
            inRelease_ = false;
            segmentIndex_ = 0;
            samplesIntoSegment_ = 0;
            currentLevel_ = 0.0f;
            chain_ = {};
        }

        [[nodiscard]] float renderSample() noexcept
        {
            if (!isActive())
            {
                currentLevel_ = 0.0f;
                return 0.0f;
            }

            if (inRelease_)
            {
                const float t = progress();
                currentLevel_ = dsp::lerp(releaseStartLevel_, 0.0f, t);
                if (advanceSample())
                {
                    reset();
                    currentLevel_ = 0.0f;
                }
                return currentLevel_;
            }

            if (chain_.segmentCount == 0)
            {
                reset();
                return 0.0f;
            }

            const auto& seg = chain_.segments[segmentIndex_];
            switch (seg.type)
            {
                case SegmentType::Ramp:
                {
                    const float t = progress();
                    const float shaped = modulation::applyMorphEasing(t, seg.shape);
                    currentLevel_ = dsp::lerp(startLevel_, seg.level, shaped);
                    break;
                }
                case SegmentType::Hold:
                    currentLevel_ = seg.level;
                    break;
                case SegmentType::Step:
                    currentLevel_ = seg.level;
                    break;
                case SegmentType::Loop:
                    currentLevel_ = seg.level;
                    break;
            }

            if (advanceSample())
                advanceSegment();
            return currentLevel_;
        }

        [[nodiscard]] bool isActive() const noexcept
        {
            return gateOn_ || inRelease_ || currentLevel_ > 0.0f;
        }

        [[nodiscard]] float getCurrentLevel() const noexcept { return currentLevel_; }
        [[nodiscard]] std::size_t getSegmentIndex() const noexcept { return segmentIndex_; }

    private:
        [[nodiscard]] std::int64_t secondsToSamples(float seconds) const noexcept
        {
            const auto n = static_cast<std::int64_t>(seconds * static_cast<float>(sampleRate_));
            return n > 0 ? n : 1;
        }

        [[nodiscard]] std::int64_t segmentLengthSamples(std::size_t index) const noexcept
        {
            if (index >= chain_.segmentCount)
                return 1;
            const float ms = dsp::clamp(chain_.segments[index].durationMs, 1.0f, 60000.0f);
            return secondsToSamples(ms * 0.001f);
        }

        [[nodiscard]] float progress() const noexcept
        {
            if (stageLengthSamples_ <= 0)
                return 1.0f;
            return dsp::clamp(static_cast<float>(samplesIntoSegment_) / static_cast<float>(stageLengthSamples_), 0.0f,
                              1.0f);
        }

        [[nodiscard]] bool advanceSample() noexcept
        {
            ++samplesIntoSegment_;
            return samplesIntoSegment_ >= stageLengthSamples_;
        }

        void advanceSegment() noexcept
        {
            samplesIntoSegment_ = 0;

            if (segmentIndex_ >= chain_.segmentCount)
            {
                if (gateOn_)
                {
                    currentLevel_ = chain_.segments[chain_.segmentCount - 1].level;
                    stageLengthSamples_ = std::numeric_limits<std::int64_t>::max() / 2;
                }
                else
                    noteOff();
                return;
            }

            const auto finishedType = chain_.segments[segmentIndex_].type;
            const bool atLoopEnd = chain_.loopEnd < chain_.segmentCount && segmentIndex_ == chain_.loopEnd;
            const bool shouldLoop =
                gateOn_ && (finishedType == SegmentType::Loop || atLoopEnd) && chain_.loopEnd >= chain_.loopStart;

            if (shouldLoop)
            {
                segmentIndex_ = chain_.loopStart;
            }
            else
            {
                ++segmentIndex_;
                if (segmentIndex_ >= chain_.segmentCount)
                {
                    if (gateOn_)
                    {
                        segmentIndex_ = chain_.segmentCount - 1;
                        currentLevel_ = chain_.segments[segmentIndex_].level;
                        stageLengthSamples_ = std::numeric_limits<std::int64_t>::max() / 2;
                        return;
                    }
                    noteOff();
                    return;
                }
            }

            startLevel_ = currentLevel_;
            stageLengthSamples_ = segmentLengthSamples(segmentIndex_);
            if (chain_.segments[segmentIndex_].type == SegmentType::Step)
                currentLevel_ = chain_.segments[segmentIndex_].level;
        }

        double sampleRate_ = 48000.0;
        SegmentEnvelopeChain chain_{};
        float releaseSeconds_ = 0.3f;
        bool gateOn_ = false;
        bool inRelease_ = false;
        std::size_t segmentIndex_ = 0;
        std::int64_t samplesIntoSegment_ = 0;
        std::int64_t stageLengthSamples_ = 1;
        float startLevel_ = 0.0f;
        float releaseStartLevel_ = 0.0f;
        float currentLevel_ = 0.0f;
    };

} // namespace pw8::envelope
