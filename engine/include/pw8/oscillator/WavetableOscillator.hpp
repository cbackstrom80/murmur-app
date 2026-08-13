#pragma once

#include <cstddef>

#include "pw8/dsp/Math.hpp"

// Engine Type 2 -- Wavetable (MINIMAL / PARTIAL implementation, see docs/DSP_ENGINE.md).
//
// Status: PARTIAL. This is a real, working wavetable reader (multi-frame, linearly
// interpolated across both sample position and frame position) but it is NOT yet
// mip-mapped/band-limited -- at high fundamental frequencies against a table with
// significant high-harmonic content, this will alias. Mipmapping is tracked as a
// PLANNED follow-up (Phase 2 completion) once the core architecture proves out; the
// table storage layout below was chosen specifically so mip levels can be added as
// extra `WavetableView`s per octave without changing the oscillator's read logic.
//
// The oscillator itself owns no sample storage (no realtime allocation): callers
// point it at a `WavetableView` that is prepared off the audio thread (see
// tools/wavetable_builder and pw8::patch::WavetableTable).

namespace pw8::oscillator
{
    /// Non-owning view over a multi-frame wavetable: `numFrames` frames of
    /// `samplesPerFrame` samples each, laid out frame-major and contiguous.
    struct WavetableView
    {
        const float* samples = nullptr;
        int numFrames = 0;
        int samplesPerFrame = 0;

        [[nodiscard]] bool isValid() const noexcept
        {
            return samples != nullptr && numFrames > 0 && samplesPerFrame > 1;
        }

        [[nodiscard]] float sampleAt(int frame, int index) const noexcept
        {
            return samples[static_cast<std::size_t>(frame) * static_cast<std::size_t>(samplesPerFrame) +
                            static_cast<std::size_t>(index)];
        }
    };

    class WavetableOscillator
    {
    public:
        void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0; }

        void reset(float initialPhase = 0.0f) noexcept { phase_ = dsp::wrapPhase(initialPhase); }

        void setFrequency(float hz) noexcept { frequencyHz_ = hz; }

        /// `framePosition01` selects a (possibly fractional) frame across the table, 0..1.
        [[nodiscard]] float renderSample(const WavetableView& table, float framePosition01,
                                          float externalPhaseModulation = 0.0f) noexcept
        {
            if (!table.isValid())
            {
                phase_ = dsp::wrapPhase(phase_ + static_cast<float>(frequencyHz_ / sampleRate_));
                return 0.0f;
            }

            const float readPhase = dsp::wrapPhase(phase_ + externalPhaseModulation);
            const float out = readTable(table, dsp::clamp(framePosition01, 0.0f, 1.0f), readPhase);

            const float dt = static_cast<float>(frequencyHz_ / sampleRate_);
            const float phaseBefore = phase_;
            phase_ = dsp::wrapPhase(phase_ + dt);
            didWrapThisSample_ = phase_ < phaseBefore;
            return out;
        }

        [[nodiscard]] bool didWrapThisSample() const noexcept { return didWrapThisSample_; }

    private:
        [[nodiscard]] static float readTable(const WavetableView& table, float framePosition01, float phase) noexcept
        {
            const float frameF = framePosition01 * static_cast<float>(table.numFrames - 1);
            const int frame0 = static_cast<int>(frameF);
            const int frame1 = dsp::clamp(frame0 + 1, 0, table.numFrames - 1);
            const float frameFrac = frameF - static_cast<float>(frame0);

            const float sampleF = phase * static_cast<float>(table.samplesPerFrame);
            const int idx0 = static_cast<int>(sampleF) % table.samplesPerFrame;
            const int idx1 = (idx0 + 1) % table.samplesPerFrame;
            const float sampleFrac = sampleF - std::floor(sampleF);

            const float s0 = dsp::lerp(table.sampleAt(frame0, idx0), table.sampleAt(frame0, idx1), sampleFrac);
            const float s1 = dsp::lerp(table.sampleAt(frame1, idx0), table.sampleAt(frame1, idx1), sampleFrac);
            return dsp::lerp(s0, s1, frameFrac);
        }

        double sampleRate_ = 48000.0;
        float frequencyHz_ = 440.0f;
        float phase_ = 0.0f;
        bool didWrapThisSample_ = false;
    };

} // namespace pw8::oscillator
