#pragma once

#include <cmath>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/noise/NoiseSource.hpp"
#include "pw8/oscillator/AdditiveOscillator.hpp"
#include "pw8/oscillator/ClassicOscillator.hpp"
#include "pw8/oscillator/PhaseShapeOscillator.hpp"
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

        /// NoiseChaos engine fields (see noise::NoiseVariant). Stored as a float
        /// (not the enum type), matching every other discrete field's
        /// automation-friendly flat-struct convention -- render() rounds it back to
        /// an enum ordinal.
        float noiseVariant = 0.0f; // noise::NoiseVariant::White
        float noiseRate = 200.0f;

        /// PhaseShape engine fields -- see oscillator::PhaseShapeParams for the full
        /// per-field writeup (CZ-style phase distortion + post-gen wavefold).
        float phaseBend = 0.0f;
        float phaseFold = 0.0f;
        float phaseAsymmetry = 0.0f;
        float phaseShape = 0.0f;

        /// Additive engine fields -- see oscillator::AdditiveParams for the full
        /// per-field writeup. additivePartialCount is stored as a float (rounded to
        /// int at the render() call site), matching every other discrete field's
        /// automation-friendly flat-struct convention.
        float additivePartialCount = 32.0f;
        float additiveTilt = 0.0f;
        float additiveOddEven = 0.5f;
        float additiveStretch = 0.0f;
    };

    struct OperatorState
    {
        oscillator::ClassicOscillator classicOsc;
        oscillator::WavetableOscillator waveOsc;
        oscillator::ClassicOscillator fmModulatorOsc; ///< Engine Type 3 (FM/PM) only.
        float fmModulatorLastOutput = 0.0f;            ///< For the modulator's own self-feedback.
        noise::NoiseSource noiseSource;
        oscillator::PhaseShapeOscillator phaseShapeOsc;
        oscillator::AdditiveOscillator additiveOsc;
        float lastOutput = 0.0f; ///< previous-sample output, used by Feedback edges.

        void prepare(double sampleRate) noexcept
        {
            classicOsc.prepare(sampleRate);
            waveOsc.prepare(sampleRate);
            fmModulatorOsc.prepare(sampleRate);
            noiseSource.prepare(sampleRate);
            phaseShapeOsc.prepare(sampleRate);
            additiveOsc.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void reset(float initialPhase = 0.0f) noexcept
        {
            classicOsc.reset(initialPhase);
            waveOsc.reset(initialPhase);
            fmModulatorOsc.reset(initialPhase);
            fmModulatorLastOutput = 0.0f;
            phaseShapeOsc.reset(initialPhase);
            additiveOsc.reset(initialPhase);
            lastOutput = 0.0f;
        }

        /// Reseeds this node's noise source. Separate from reset() (which only
        /// takes a phase) because the two are driven by different call sites:
        /// reset() also runs on mid-note algorithm-graph Sync edges (see
        /// AlgorithmExecutor), where noise has no "phase" to synchronize and
        /// reseeding would just restart its stream at an arbitrary point rather
        /// than doing anything meaningful -- so only Voice::noteOn calls this, once
        /// per note, with a dsp::DeterministicRng::deriveSeed()-derived seed.
        void seedNoise(std::uint64_t seed) noexcept { noiseSource.reset(seed); }

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

                case algorithm::EngineType::NoiseChaos:
                {
                    // Noise has no pitch of its own -- carrierHz (and phaseMod) are
                    // computed above for every engine uniformly but deliberately
                    // unused here; freqModHz still reaches the operator (through
                    // carrierHz's computation above) for API consistency, but noise
                    // has nothing to key it off, matching this engine's own design
                    // (docs/DSP_ENGINE.md "Noise/Chaos" -- rate-driven, not pitch-driven).
                    noise::NoiseSourceParams noiseParams;
                    noiseParams.variant = static_cast<noise::NoiseVariant>(
                        dsp::clamp(static_cast<int>(params.noiseVariant + 0.5f), 0, 6));
                    noiseParams.rateHz = params.noiseRate;
                    out = noiseSource.renderSample(noiseParams);
                    break;
                }

                case algorithm::EngineType::PhaseShape:
                {
                    phaseShapeOsc.setFrequency(carrierHz);
                    oscillator::PhaseShapeParams shapeParams;
                    shapeParams.phaseBend = params.phaseBend;
                    shapeParams.phaseFold = params.phaseFold;
                    shapeParams.phaseAsymmetry = params.phaseAsymmetry;
                    shapeParams.phaseShape = params.phaseShape;
                    out = phaseShapeOsc.renderSample(shapeParams, phaseMod);
                    break;
                }

                case algorithm::EngineType::Additive:
                {
                    additiveOsc.setFrequency(carrierHz);
                    oscillator::AdditiveParams additiveParams;
                    additiveParams.partialCount = static_cast<int>(params.additivePartialCount + 0.5f);
                    additiveParams.tilt = params.additiveTilt;
                    additiveParams.oddEven = params.additiveOddEven;
                    additiveParams.stretch = params.additiveStretch;
                    out = additiveOsc.renderSample(additiveParams, phaseMod);
                    break;
                }

                // Engine types 6, 8 (Granular, Resonator) are architected
                // (see algorithm::EngineType, docs/ROADMAP.md Phase 10) but not
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
