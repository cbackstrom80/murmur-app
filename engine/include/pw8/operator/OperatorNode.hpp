#pragma once

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/noise/NoiseSource.hpp"
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

        /// NoiseChaos engine fields (see noise::NoiseVariant). Stored as a float
        /// (not the enum type), matching every other discrete field's
        /// automation-friendly flat-struct convention -- render() rounds it back to
        /// an enum ordinal.
        float noiseVariant = 0.0f; // noise::NoiseVariant::White
        float noiseRate = 200.0f;
    };

    struct OperatorState
    {
        oscillator::ClassicOscillator classicOsc;
        oscillator::WavetableOscillator waveOsc;
        noise::NoiseSource noiseSource;
        float lastOutput = 0.0f; ///< previous-sample output, used by Feedback edges.

        void prepare(double sampleRate) noexcept
        {
            classicOsc.prepare(sampleRate);
            waveOsc.prepare(sampleRate);
            noiseSource.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void reset(float initialPhase = 0.0f) noexcept
        {
            classicOsc.reset(initialPhase);
            waveOsc.reset(initialPhase);
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

                // Engine types 4, 5, 6, 8 (Additive, Phase/Shape, Granular, Resonator)
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
