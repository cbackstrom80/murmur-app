#pragma once

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/oscillator/ClassicOscillator.hpp"
#include "pw8/oscillator/WavetableOscillator.hpp"
#include "pw8/oscillator/WavetableTable.hpp"

// A single node ("operator") inside a layer's 8-node algorithm graph.
// Holds per-voice DSP state for whichever engine type it's configured as.
// No virtual dispatch: `render()` switches on `engine`, resolved once per patch load,
// not per call site -- the switch itself is a handful of predictable branches the
// compiler resolves well, not a vtable indirection.

namespace pw8::op
{
    struct OperatorParams
    {
        algorithm::EngineType engine = algorithm::EngineType::Classic;
        oscillator::ClassicOscillatorParams classic{};
        /// Wavetable frame position, 0..1 (only used when engine == Wavetable).
        float wavetableFramePosition = 0.0f;
        /// Frequency ratio relative to the voice's base (note) frequency, for ratio-mode
        /// operators. Fixed-Hz mode is expressed by setting keyTrack = false and using
        /// fixedFrequencyHz directly.
        float frequencyRatio = 1.0f;
        float fixedFrequencyHz = 440.0f;
        bool keyTrack = true;
        /// Output level of this node before it's summed into any output bus or consumed
        /// by AUDIO edges downstream.
        float level = 1.0f;
    };

    struct OperatorState
    {
        oscillator::ClassicOscillator classicOsc;
        oscillator::WavetableOscillator waveOsc;
        float lastOutput = 0.0f; ///< previous-sample output, used by Feedback edges.

        void prepare(double sampleRate) noexcept
        {
            classicOsc.prepare(sampleRate);
            waveOsc.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void reset(float initialPhase = 0.0f) noexcept
        {
            classicOsc.reset(initialPhase);
            waveOsc.reset(initialPhase);
            lastOutput = 0.0f;
        }

        /// Renders one sample. `baseFrequencyHz` is the voice's key-tracked note
        /// frequency (before this operator's ratio/fixed override); `phaseMod` and
        /// `freqModHz` are this-sample modulation accumulated from algorithm-graph edges.
        /// `wavetableTable` may be nullptr (renders silence on the Wavetable engine) --
        /// when present, the appropriate band-limited mip is picked fresh every sample
        /// based on this sample's carrier frequency (see WavetableTable::viewForFrequency,
        /// a cheap linear scan, no allocation -- safe on the audio thread).
        [[nodiscard]] float render(const OperatorParams& params, const oscillator::WavetableTable* wavetableTable,
                                    float baseFrequencyHz, float phaseMod, float freqModHz) noexcept
        {
            const float carrierHz = (params.keyTrack ? baseFrequencyHz * params.frequencyRatio
                                                       : params.fixedFrequencyHz) +
                                     freqModHz;

            float out = 0.0f;
            switch (params.engine)
            {
                case algorithm::EngineType::Classic:
                    classicOsc.setFrequency(carrierHz);
                    out = classicOsc.renderSample(params.classic, phaseMod);
                    break;

                case algorithm::EngineType::Wavetable:
                {
                    waveOsc.setFrequency(carrierHz);
                    const oscillator::WavetableView view =
                        wavetableTable != nullptr ? wavetableTable->viewForFrequency(carrierHz, sampleRate_)
                                                   : oscillator::WavetableView{};
                    out = waveOsc.renderSample(view, params.wavetableFramePosition, phaseMod);
                    break;
                }

                // Engine types 3-8 (FM/PM, Additive, Phase/Shape, Granular, Noise, Resonator)
                // are architected (see algorithm::EngineType, docs/ROADMAP.md Phase 10) but not
                // yet implemented -- they intentionally render silence rather than guess.
                default:
                    out = 0.0f;
                    break;
            }

            out *= params.level;
            lastOutput = out;
            return out;
        }

    private:
        double sampleRate_ = 48000.0;
    };

} // namespace pw8::op
