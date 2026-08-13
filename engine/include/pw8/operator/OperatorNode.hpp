#pragma once

#include <cmath>
#include <cstdint>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/noise/NoiseSource.hpp"
#include "pw8/oscillator/AdditiveOscillator.hpp"
#include "pw8/oscillator/ClassicOscillator.hpp"
#include "pw8/oscillator/GranularOscillator.hpp"
#include "pw8/oscillator/PhaseShapeOscillator.hpp"
#include "pw8/oscillator/ResonatorOscillator.hpp"
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
        /// Wavetable frame position, 0..1 (used by the Wavetable engine, and
        /// reused by the Granular engine as its grain read-position control --
        /// same semantic shape, and it's what ModDestination::OperatorWavetablePosition
        /// already targets).
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

        /// Resonator engine fields -- see oscillator::ResonatorParams for the full
        /// per-field writeup. resonatorModeCount is stored as a float (rounded to
        /// int at the render() call site), matching every other discrete field's
        /// automation-friendly flat-struct convention.
        float resonatorStructure = 0.3f;
        float resonatorDecay = 0.5f;
        float resonatorDamping = 0.5f;
        float resonatorBrightness = 0.5f;
        float resonatorModeCount = 6.0f;

        /// Granular engine fields -- see oscillator::GranularParams for the full
        /// per-field writeup. Deliberately reuses wavetableFramePosition/level
        /// above rather than duplicating them (grains read from the same
        /// wavetableId-loaded data the Wavetable engine uses).
        float grainDensity = 20.0f;
        float grainSizeMs = 60.0f;
        float grainPositionJitter = 0.1f;
        float grainPitchJitter = 0.0f;

        /// Wavetable engine warp fields — see oscillator::WtWarpParams.
        float wtBend = 0.0f;
        float wtAsymmetry = 0.0f;
        float wtSyncRatio = 1.0f;
        float wtSyncAmount = 0.0f;
        float wtFormantShift = 0.0f;
    };

    struct OperatorState
    {
        oscillator::ClassicOscillator classicOsc;
        oscillator::WavetableOscillator waveOsc;
        oscillator::ClassicOscillator fmModulatorOsc; ///< Engine Type 3 (FM/PM) only.
        float fmModulatorLastOutput = 0.0f;            ///< For the modulator's own self-feedback.
        float fmModulatorPrevOutput = 0.0f;            ///< Previous sample for 2x OS on FM feedback.
        noise::NoiseSource noiseSource;
        oscillator::PhaseShapeOscillator phaseShapeOsc;
        oscillator::AdditiveOscillator additiveOsc;
        oscillator::ResonatorOscillator resonatorOsc;
        oscillator::GranularOscillator granularOsc;
        float lastOutput = 0.0f; ///< previous-sample output, used by Feedback edges.
        float prevLastOutput = 0.0f; ///< two samples ago, for 2x OS on graph feedback edges.

        /// True when this operator's primary phase-based oscillator wrapped its cycle
        /// on the most recent render() call -- used by algorithm-graph SYNC edges.
        [[nodiscard]] bool didWrapThisSample(algorithm::EngineType engine) const noexcept
        {
            switch (engine)
            {
                case algorithm::EngineType::Classic:
                    return classicOsc.didWrapThisSample();
                case algorithm::EngineType::Wavetable:
                    return waveOsc.didWrapThisSample();
                case algorithm::EngineType::FmPm:
                    return classicOsc.didWrapThisSample();
                case algorithm::EngineType::PhaseShape:
                    return phaseShapeOsc.didWrapThisSample();
                case algorithm::EngineType::Additive:
                    return additiveOsc.didWrapThisSample();
                default:
                    return false;
            }
        }

        void prepare(double sampleRate) noexcept
        {
            classicOsc.prepare(sampleRate);
            waveOsc.prepare(sampleRate);
            fmModulatorOsc.prepare(sampleRate);
            noiseSource.prepare(sampleRate);
            phaseShapeOsc.prepare(sampleRate);
            additiveOsc.prepare(sampleRate);
            resonatorOsc.prepare(sampleRate);
            granularOsc.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void reset(float initialPhase = 0.0f) noexcept
        {
            classicOsc.reset(initialPhase);
            waveOsc.reset(initialPhase);
            fmModulatorOsc.reset(initialPhase);
            fmModulatorLastOutput = 0.0f;
            fmModulatorPrevOutput = 0.0f;
            phaseShapeOsc.reset(initialPhase);
            additiveOsc.reset(initialPhase);
            // A fixed, non-randomized seed: this path also runs on a mid-note
            // algorithm-graph Sync edge (see AlgorithmExecutor), where re-triggering
            // the exciter burst is the whole point but a *new* random seed isn't --
            // seedResonator() below is the one real per-note-random reseed, called
            // once from Voice::noteOn().
            resonatorOsc.reset(0);
            // Same reasoning as resonatorOsc above -- a Sync edge restarts the grain
            // clock/pool deterministically but isn't meant to draw a *new* random
            // seed; seedGranular() below is the real per-note-random reseed.
            granularOsc.reset(0);
            lastOutput = 0.0f;
            prevLastOutput = 0.0f;
        }

        /// Reseeds this node's noise source. Separate from reset() (which only
        /// takes a phase) because the two are driven by different call sites:
        /// reset() also runs on mid-note algorithm-graph Sync edges (see
        /// AlgorithmExecutor), where noise has no "phase" to synchronize and
        /// reseeding would just restart its stream at an arbitrary point rather
        /// than doing anything meaningful -- so only Voice::noteOn calls this, once
        /// per note, with a dsp::DeterministicRng::deriveSeed()-derived seed.
        void seedNoise(std::uint64_t seed) noexcept { noiseSource.reset(seed); }

        /// Reseeds this node's resonator exciter with a real per-voice-per-note
        /// random seed and re-triggers its burst. Separate from reset() for the
        /// same reason seedNoise() (NoiseChaos engine) is separate from reset() --
        /// only Voice::noteOn() calls this, once per note, with a
        /// dsp::DeterministicRng::deriveSeed()-derived seed.
        void seedResonator(std::uint64_t seed) noexcept { resonatorOsc.reset(seed); }

        /// Reseeds this node's grain RNG with a real per-voice-per-note random
        /// seed. Separate from reset() for the same reason seedNoise()/
        /// seedResonator() are -- only Voice::noteOn() calls this, once per note,
        /// with a dsp::DeterministicRng::deriveSeed()-derived seed.
        void seedGranular(std::uint64_t seed) noexcept { granularOsc.reset(seed); }

        /// Renders one sample. `baseFrequencyHz` is the voice's key-tracked note
        /// frequency (before this operator's ratio/fixed override); `phaseMod` and
        /// `freqModHz` are this-sample modulation accumulated from algorithm-graph edges.
        /// `wavetableTable` may be nullptr (renders silence on the Wavetable engine) --
        /// when present, the appropriate band-limited mip is picked fresh every sample
        /// based on this sample's carrier frequency (see WavetableTable::viewForFrequency,
        /// a cheap linear scan, no allocation -- safe on the audio thread).
        [[nodiscard]] float render(const OperatorParams& params, const oscillator::WavetableTable* wavetableTable,
                                    float baseFrequencyHz, float phaseMod, float freqModHz,
                                    int nonlinearOsFactor = 1) noexcept
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
                    oscillator::WtWarpParams warp;
                    warp.bend = params.wtBend;
                    warp.asymmetry = params.wtAsymmetry;
                    warp.syncRatio = params.wtSyncRatio;
                    warp.syncAmount = params.wtSyncAmount;
                    warp.formantShift = params.wtFormantShift;
                    out = waveOsc.renderSample(view, params.wavetableFramePosition, phaseMod, warp);
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
                    // (tanhFeedback2x, flushIfNotFinite), applied here to the modulator
                    // instead of a whole graph edge.
                    const float feedbackPhaseMod = dsp::tanhFeedbackOs(
                        fmModulatorLastOutput, fmModulatorPrevOutput, params.fmModulatorFeedback, nonlinearOsFactor);

                    oscillator::ClassicOscillatorParams modulatorParams;
                    modulatorParams.waveform = params.fmModulatorWaveform;
                    const float modulatorOut = fmModulatorOsc.renderSample(modulatorParams, feedbackPhaseMod);
                    fmModulatorPrevOutput = fmModulatorLastOutput;
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
                    out = phaseShapeOsc.renderSample(shapeParams, phaseMod, nonlinearOsFactor);
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

                case algorithm::EngineType::Resonator:
                {
                    // Noise-excited resonator: no pitch envelope/phase concept the
                    // way an oscillator has -- carrierHz still sets each mode's
                    // tuned frequency, but phaseMod is unused (there's no phase to
                    // offset), matching NoiseChaos's own documented rationale for
                    // ignoring inputs that don't apply to this engine's design.
                    resonatorOsc.setFrequency(carrierHz);
                    oscillator::ResonatorParams resonatorParams;
                    resonatorParams.structure = params.resonatorStructure;
                    resonatorParams.decay = params.resonatorDecay;
                    resonatorParams.damping = params.resonatorDamping;
                    resonatorParams.brightness = params.resonatorBrightness;
                    resonatorParams.modeCount = static_cast<int>(params.resonatorModeCount + 0.5f);
                    out = resonatorOsc.renderSample(resonatorParams);
                    break;
                }

                case algorithm::EngineType::Granular:
                {
                    granularOsc.setFrequency(carrierHz);
                    oscillator::GranularParams granularParams;
                    granularParams.framePosition01 = params.wavetableFramePosition;
                    granularParams.densityHz = params.grainDensity;
                    granularParams.grainSizeMs = params.grainSizeMs;
                    granularParams.positionJitter = params.grainPositionJitter;
                    granularParams.pitchJitter = params.grainPitchJitter;
                    // Band-limited mip selected from the current carrier frequency so
                    // grain spawn reads the same anti-aliased table the Wavetable engine uses.
                    const oscillator::WavetableView sourceView =
                        wavetableTable != nullptr ? wavetableTable->viewForFrequency(carrierHz, sampleRate_)
                                                   : oscillator::WavetableView{};
                    out = granularOsc.renderSample(granularParams, sourceView);
                    break;
                }

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
