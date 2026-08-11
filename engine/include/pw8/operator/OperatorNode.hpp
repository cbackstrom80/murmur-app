#pragma once

#include <cstdint>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/oscillator/ClassicOscillator.hpp"
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

        /// Resonator engine fields -- see oscillator::ResonatorParams for the full
        /// per-field writeup. resonatorModeCount is stored as a float (rounded to
        /// int at the render() call site), matching every other discrete field's
        /// automation-friendly flat-struct convention.
        float resonatorStructure = 0.3f;
        float resonatorDecay = 0.5f;
        float resonatorDamping = 0.5f;
        float resonatorBrightness = 0.5f;
        float resonatorModeCount = 6.0f;
    };

    struct OperatorState
    {
        oscillator::ClassicOscillator classicOsc;
        oscillator::WavetableOscillator waveOsc;
        oscillator::ResonatorOscillator resonatorOsc;
        float lastOutput = 0.0f; ///< previous-sample output, used by Feedback edges.

        void prepare(double sampleRate) noexcept
        {
            classicOsc.prepare(sampleRate);
            waveOsc.prepare(sampleRate);
            resonatorOsc.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void reset(float initialPhase = 0.0f) noexcept
        {
            classicOsc.reset(initialPhase);
            waveOsc.reset(initialPhase);
            // A fixed, non-randomized seed: this path also runs on a mid-note
            // algorithm-graph Sync edge (see AlgorithmExecutor), where re-triggering
            // the exciter burst is the whole point but a *new* random seed isn't --
            // seedResonator() below is the one real per-note-random reseed, called
            // once from Voice::noteOn().
            resonatorOsc.reset(0);
            lastOutput = 0.0f;
        }

        /// Reseeds this node's resonator exciter with a real per-voice-per-note
        /// random seed and re-triggers its burst. Separate from reset() for the
        /// same reason OperatorState::seedNoise() (NoiseChaos engine) is separate
        /// from reset() -- only Voice::noteOn() calls this, once per note, with a
        /// dsp::DeterministicRng::deriveSeed()-derived seed.
        void seedResonator(std::uint64_t seed) noexcept { resonatorOsc.reset(seed); }

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

                // Engine types 3, 4, 5, 6, 7 (FM/PM, Additive, Phase/Shape, Granular,
                // Noise) are architected (see algorithm::EngineType, docs/ROADMAP.md
                // Phase 10) but not yet implemented -- they intentionally render
                // silence rather than guess.
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
