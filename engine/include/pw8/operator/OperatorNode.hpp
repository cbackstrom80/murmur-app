#pragma once

#include <cmath>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/dsp/Math.hpp"
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

        // Engine Type 3 (FM/PM) only -- a self-contained 2-operator FM voice inside
        // this one node, distinct from the graph-level PM/FM edges (which modulate
        // BETWEEN separate nodes). Carrier reuses `classic` above; these describe the
        // internal modulator.
        float fmModulatorRatio = 1.0f;      ///< Modulator frequency / carrier frequency.
        float fmModulatorIndex = 0.5f;      ///< Modulation depth, in the same "cycles"
                                              ///< units AlgorithmExecutor's PhaseMod edges
                                              ///< already use (finalOut * amount) -- not
                                              ///< radians, so an index of 1.0 here reads
                                              ///< the same as a graph-level PM edge amount
                                              ///< of 1.0 would.
        float fmModulatorFeedback = 0.0f;   ///< 0..1, modulator self-feedback depth.
        oscillator::ClassicWaveform fmModulatorWaveform = oscillator::ClassicWaveform::Sine;
    };

    struct OperatorState
    {
        oscillator::ClassicOscillator classicOsc;
        oscillator::WavetableOscillator waveOsc;
        oscillator::ClassicOscillator fmModulatorOsc; ///< Engine Type 3 (FM/PM) only.
        float fmModulatorLastOutput = 0.0f;            ///< For the modulator's own self-feedback.
        float lastOutput = 0.0f; ///< previous-sample output, used by Feedback edges.

        void prepare(double sampleRate) noexcept
        {
            classicOsc.prepare(sampleRate);
            waveOsc.prepare(sampleRate);
            fmModulatorOsc.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void reset(float initialPhase = 0.0f) noexcept
        {
            classicOsc.reset(initialPhase);
            waveOsc.reset(initialPhase);
            fmModulatorOsc.reset(initialPhase);
            fmModulatorLastOutput = 0.0f;
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

                case algorithm::EngineType::FmPm:
                {
                    // Self-contained 2-operator FM/PM voice in one node -- distinct from
                    // the graph-level PM/FM edges (which modulate BETWEEN separate
                    // nodes). Carrier reuses params.classic; the modulator is a second
                    // internal oscillator phase-modulating it.
                    const float modulatorHz = carrierHz * params.fmModulatorRatio;
                    fmModulatorOsc.setFrequency(modulatorHz);

                    // One-sample-delayed self-feedback, soft-saturated -- the exact same
                    // technique AlgorithmExecutor.hpp's Feedback edges already use
                    // (std::tanh(prev * amount), flushIfNotFinite), applied here to the
                    // modulator instead of a whole graph edge.
                    const float feedbackPhaseMod =
                        dsp::flushIfNotFinite(std::tanh(fmModulatorLastOutput * params.fmModulatorFeedback));

                    oscillator::ClassicOscillatorParams modulatorParams;
                    modulatorParams.waveform = params.fmModulatorWaveform;
                    const float modulatorOut = fmModulatorOsc.renderSample(modulatorParams, feedbackPhaseMod);
                    fmModulatorLastOutput = modulatorOut;

                    // Modulator output phase-modulates the carrier. Same units/convention
                    // as a graph-level PhaseMod edge (finalOut * amount, in cycles) -- see
                    // OperatorParams::fmModulatorIndex's doc comment.
                    const float phaseModFromModulator = modulatorOut * params.fmModulatorIndex;

                    classicOsc.setFrequency(carrierHz);
                    out = classicOsc.renderSample(params.classic, phaseMod + phaseModFromModulator);
                    break;
                }

                // Engine types 4-8 (Additive, Phase/Shape, Granular, Noise, Resonator)
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
