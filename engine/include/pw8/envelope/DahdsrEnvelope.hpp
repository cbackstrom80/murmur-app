#pragma once

#include <cmath>
#include <cstdint>

#include "pw8/dsp/Math.hpp"

// High-quality DAHDSR envelope generator: Delay, Attack, Hold, Decay, Sustain, Release.
// Per-sample, no allocation, no branching on anything but its own stage enum.
// Stage curves default to a musical exponential-ish shape (adjustable per-stage via
// `curveShape`, 0 = linear, positive = exponential-feeling, matching common synth UX).

namespace pw8::envelope
{
    enum class Stage : std::uint8_t
    {
        Idle = 0,
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release
    };

    struct DahdsrParams
    {
        float delaySeconds = 0.0f;
        float attackSeconds = 0.005f;
        float holdSeconds = 0.0f;
        float decaySeconds = 0.2f;
        float sustainLevel = 0.7f; // 0..1
        float releaseSeconds = 0.3f;

        /// Per-stage curve shape: 0 = linear, >0 bows the curve like a typical analog-style
        /// envelope (higher = more exponential feel). Kept simple/uniform for MVP; a future
        /// pass can expose per-stage curves independently.
        float curveShape = 2.0f;

        /// If true, a retrigger while a voice is already sounding restarts from Delay at
        /// the current level (click-free) rather than jumping straight back to 0.
        bool legato = false;
    };

    class DahdsrEnvelope
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        }

        void noteOn(const DahdsrParams& params) noexcept
        {
            params_ = params;
            if (!params.legato || stage_ == Stage::Idle)
                startLevel_ = 0.0f;
            else
                startLevel_ = currentLevel_;

            samplesIntoStage_ = 0;
            stage_ = params.delaySeconds > 0.0f ? Stage::Delay : Stage::Attack;
            if (stage_ == Stage::Attack)
                stageLengthSamples_ = secondsToSamples(params.attackSeconds);
            else
                stageLengthSamples_ = secondsToSamples(params.delaySeconds);
        }

        void noteOff() noexcept
        {
            if (stage_ == Stage::Idle)
                return;
            stage_ = Stage::Release;
            samplesIntoStage_ = 0;
            releaseStartLevel_ = currentLevel_;
            stageLengthSamples_ = secondsToSamples(params_.releaseSeconds);
        }

        /// Immediately silence the envelope (e.g. hard voice steal with no crossfade available).
        void reset() noexcept
        {
            stage_ = Stage::Idle;
            currentLevel_ = 0.0f;
            samplesIntoStage_ = 0;
        }

        [[nodiscard]] float renderSample() noexcept
        {
            switch (stage_)
            {
                case Stage::Idle:
                    currentLevel_ = 0.0f;
                    break;

                case Stage::Delay:
                    currentLevel_ = startLevel_;
                    advanceStageOrTransition(Stage::Attack, params_.attackSeconds);
                    break;

                case Stage::Attack:
                {
                    const float t = progress();
                    currentLevel_ = dsp::lerp(startLevel_, 1.0f, shapeUp(t));
                    advanceStageOrTransition(Stage::Hold, params_.holdSeconds);
                    break;
                }

                case Stage::Hold:
                    currentLevel_ = 1.0f;
                    advanceStageOrTransition(Stage::Decay, params_.decaySeconds);
                    break;

                case Stage::Decay:
                {
                    const float t = progress();
                    currentLevel_ = dsp::lerp(1.0f, params_.sustainLevel, shapeDown(t));
                    if (advanceSample())
                    {
                        stage_ = Stage::Sustain;
                        samplesIntoStage_ = 0;
                    }
                    break;
                }

                case Stage::Sustain:
                    currentLevel_ = params_.sustainLevel;
                    break;

                case Stage::Release:
                {
                    const float t = progress();
                    currentLevel_ = dsp::lerp(releaseStartLevel_, 0.0f, shapeDown(t));
                    if (advanceSample())
                    {
                        stage_ = Stage::Idle;
                        currentLevel_ = 0.0f;
                    }
                    break;
                }
            }

            return currentLevel_;
        }

        [[nodiscard]] Stage getStage() const noexcept { return stage_; }
        [[nodiscard]] bool isActive() const noexcept { return stage_ != Stage::Idle; }
        [[nodiscard]] bool isReleasing() const noexcept { return stage_ == Stage::Release; }
        [[nodiscard]] float getCurrentLevel() const noexcept { return currentLevel_; }

    private:
        [[nodiscard]] std::int64_t secondsToSamples(float seconds) const noexcept
        {
            const auto n = static_cast<std::int64_t>(seconds * static_cast<float>(sampleRate_));
            return n > 0 ? n : 1;
        }

        [[nodiscard]] float progress() const noexcept
        {
            if (stageLengthSamples_ <= 0)
                return 1.0f;
            return dsp::clamp(static_cast<float>(samplesIntoStage_) / static_cast<float>(stageLengthSamples_), 0.0f, 1.0f);
        }

        // Returns true when the stage duration has elapsed this call.
        [[nodiscard]] bool advanceSample() noexcept
        {
            ++samplesIntoStage_;
            return samplesIntoStage_ >= stageLengthSamples_;
        }

        void advanceStageOrTransition(Stage next, float nextLengthSeconds) noexcept
        {
            if (advanceSample())
            {
                stage_ = next;
                samplesIntoStage_ = 0;
                stageLengthSamples_ = secondsToSamples(nextLengthSeconds);
            }
        }

        [[nodiscard]] float shapeUp(float t) const noexcept
        {
            if (params_.curveShape <= 0.0f)
                return t;
            // Concave-up-feeling attack curve: fast start, easing into the target.
            return 1.0f - std::pow(1.0f - t, 1.0f + params_.curveShape);
        }

        [[nodiscard]] float shapeDown(float t) const noexcept
        {
            if (params_.curveShape <= 0.0f)
                return t;
            // Exponential-feeling decay/release curve.
            return 1.0f - std::pow(1.0f - t, 1.0f / (1.0f + params_.curveShape));
        }

        double sampleRate_ = 48000.0;
        DahdsrParams params_{};
        Stage stage_ = Stage::Idle;
        std::int64_t samplesIntoStage_ = 0;
        std::int64_t stageLengthSamples_ = 1;
        float startLevel_ = 0.0f;
        float releaseStartLevel_ = 0.0f;
        float currentLevel_ = 0.0f;
    };

} // namespace pw8::envelope
