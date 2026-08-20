#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "pw8/algorithm/AlgorithmExecutor.hpp"
#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/dsp/Smoother.hpp"
#include "pw8/envelope/DahdsrEnvelope.hpp"
#include "pw8/envelope/SegmentEnvelope.hpp"
#include "pw8/filter/CharacterFilter.hpp"
#include "pw8/filter/FilterRouting.hpp"
#include "pw8/filter/StateVariableFilter.hpp"
#include "pw8/lfo/Lfo.hpp"
#include "pw8/modulation/GenerativeSources.hpp"
#include "pw8/modulation/ModMatrixExecutor.hpp"
#include "pw8/operator/OperatorNode.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/render/RenderTypes.hpp"

// A single polyphonic voice: 8 operator nodes routed through a (shared, precompiled)
// algorithm graph, 8 envelopes, 8 LFOs, a mod matrix, Filter 1, and the per-note
// state needed for MPE and voice allocation.
//
// Signal chain per sample: 8 LFOs + 8 envelopes render (mod sources) -> mod matrix
// -> (operator-level-modulated) algorithm graph (each engine optionally filtered)
// -> global filter (optional) -> amplitude envelope (envelopes[0]) VCA -> pan -> output.
//
// envelopes[0]/lfos[0] are conventionally "the" amp envelope / LFO1 (envelopes[0]
// is the only one wired to the VCA and to voice lifetime -- see isFree()); all 8 of
// each are otherwise fully general-purpose mod matrix sources (docs/MODULATION.md
// "8 envelopes / 8 LFOs", reached in the GATE 5 pass, docs/ROADMAP.md). LAYER/GLOBAL
// mod scope is implemented for LFO sources only (see ModScope's doc comment in
// ModMatrixTypes.hpp) -- render::Engine ticks a separate, shared bank of 8 LFOs
// once per sample and passes the result into every voice's renderSample() call, so
// LAYER-scoped LFO routes read the same value on every voice this sample rather
// than each voice's own independent phase.

namespace pw8::voice
{
    /// Per-note expressive state. Populated from MIDI/MPE at note-on and updated by
    /// subsequent per-channel messages. pitchBendSemitones/mpePitch directly affect
    /// pitch; channelPressure/polyAftertouch/mpeSlide/modWheel are mod matrix sources
    /// (ModSource::ChannelPressure/PolyAftertouch/MpeSlide/ModWheel).
    struct NoteExpression
    {
        float pitchBendSemitones = 0.0f;
        float channelPressure = 0.0f; ///< 0..1
        float polyAftertouch = 0.0f;  ///< 0..1
        float mpeSlide = 0.0f;        ///< 0..1, MPE "timbre"/slide (CC74) when in MPE mode
        float modWheel = 0.0f;        ///< 0..1, MIDI CC1 (channel-wide, latched per channel)
        float expression = 0.0f;      ///< 0..1, MIDI CC11 (channel-wide, latched per channel)
        float mpePitch = 0.0f;        ///< semitones, MPE per-note pitch bend
    };

    class Voice
    {
    public:
        std::array<envelope::DahdsrEnvelope, core::kNumEnvelopesPerLayer> envelopes{};
        std::array<envelope::SegmentEnvelope, core::kNumEnvelopesPerLayer> segmentEnvelopes_{};
        std::array<envelope::SegmentEnvelopeChain, core::kNumEnvelopesPerLayer> segmentEnvelopeChains{};

        [[nodiscard]] bool usesSegmentEnvelope(std::size_t index) const noexcept
        {
            return index < core::kNumEnvelopesPerLayer && segmentEnvelopeChains[index].isActive();
        }

        [[nodiscard]] bool envelopeIsActive(std::size_t index) const noexcept
        {
            if (usesSegmentEnvelope(index))
                return segmentEnvelopes_[index].isActive();
            return envelopes[index].isActive();
        }

        [[nodiscard]] float envelopeLevel(std::size_t index) const noexcept
        {
            if (usesSegmentEnvelope(index))
                return segmentEnvelopes_[index].getCurrentLevel();
            return envelopes[index].getCurrentLevel();
        }

        [[nodiscard]] float renderEnvelopeSample(std::size_t index) noexcept
        {
            if (usesSegmentEnvelope(index))
                return segmentEnvelopes_[index].renderSample();
            return envelopes[index].renderSample();
        }

        void startEnvelope(std::size_t index, const envelope::DahdsrParams& params) noexcept
        {
            if (usesSegmentEnvelope(index))
                segmentEnvelopes_[index].noteOn(segmentEnvelopeChains[index], params.releaseSeconds);
            else
                envelopes[index].noteOn(params);
        }

        void envelopeNoteOff(std::size_t index) noexcept
        {
            if (usesSegmentEnvelope(index))
                segmentEnvelopes_[index].noteOff();
            else
                envelopes[index].noteOff();
        }

        void prepare(double sampleRate) noexcept
        {
            for (auto& s : operatorStates)
                s.prepare(sampleRate);
            for (auto& e : envelopes)
                e.prepare(sampleRate);
            for (auto& e : segmentEnvelopes_)
                e.prepare(sampleRate);
            for (auto& l : lfos)
                l.prepare(sampleRate);
            filter1.prepare(sampleRate);
            filter2.prepare(sampleRate);
            for (auto& f : operatorFilters_)
                f.prepare(sampleRate);
            sampleRate_ = sampleRate;
            stealFadeSamples_ = static_cast<int>(sampleRate * 0.008); // ~8 ms steal crossfade
            if (stealFadeSamples_ < 64)
                stealFadeSamples_ = 64;
            else if (stealFadeSamples_ > 512)
                stealFadeSamples_ = 512;
            stealGainRamp_.setLengthSamples(stealFadeSamples_);
        }

        void noteOn(int note, int channel, float velocityUnit, float baseFreqHz,
                    const std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer>& envParams,
                    std::uint64_t ageCounter, std::uint64_t noteGenerationId, std::uint64_t voiceSeed,
                    float portamentoSeconds = 0.0f) noexcept
        {
            applyNoteOnInternal(note, channel, velocityUnit, baseFreqHz, envParams, ageCounter, noteGenerationId,
                                voiceSeed, portamentoSeconds);
            stealPhase_ = StealPhase::None;
            pendingNoteOn_.valid = false;
        }

        /// When the allocator had to take an actively-gated voice, fade the old note out,
        /// retrigger cleanly at zero gain, then fade the new note in (~8 ms by default).
        void beginStealCrossfadeNoteOn(int note, int channel, float velocityUnit, float baseFreqHz,
                                       const std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer>& envParams,
                                       std::uint64_t ageCounter, std::uint64_t noteGenerationId,
                                       std::uint64_t voiceSeed, float portamentoSeconds = 0.0f) noexcept
        {
            // Released voices still in their amp tail have gateOn=false but are audibly
            // active -- crossfade them too, otherwise filter/envelope retrigger clicks.
            if (!envelopeIsActive(0))
            {
                noteOn(note, channel, velocityUnit, baseFreqHz, envParams, ageCounter, noteGenerationId, voiceSeed,
                       portamentoSeconds);
                return;
            }

            pendingNoteOn_ = PendingNoteOn{ note,          channel,     velocityUnit, baseFreqHz,
                                              envParams,     ageCounter,  noteGenerationId,
                                              voiceSeed,     portamentoSeconds, true };
            stealPhase_ = StealPhase::FadeOut;
            stealGainRamp_.setLengthSamples(stealFadeSamples_);
            stealGainRamp_.startFromCurrent(1.0f, 0.0f);
        }

        void noteOff(float releaseVelocityUnit) noexcept
        {
            releaseVelocity = dsp::clamp(releaseVelocityUnit, 0.0f, 1.0f);
            gateOn = false;
            for (std::size_t i = 0; i < core::kNumEnvelopesPerLayer; ++i)
                envelopeNoteOff(i);
        }

        /// Immediately silences the voice with no release tail -- used at the end of a
        /// steal fade-out (gain already at zero) before retriggering the voice.
        void hardKill() noexcept
        {
            stealPhase_ = StealPhase::None;
            pendingNoteOn_.valid = false;
            for (auto& e : envelopes)
                e.reset();
            for (auto& e : segmentEnvelopes_)
                e.reset();
            noteNumber = -1;
            gateOn = false;
        }

        [[nodiscard]] bool isFree() const noexcept { return noteNumber < 0 && !envelopeIsActive(0); }
        [[nodiscard]] bool isReleased() const noexcept { return !gateOn; }
        [[nodiscard]] bool isSounding() const noexcept { return envelopeIsActive(0); }
        [[nodiscard]] float amplitudeEstimate() const noexcept { return envelopeLevel(0) * velocity; }

        /// Renders one stereo sample. Returns silence and frees the voice once its
        /// amp envelope (envelopes[0]) has fully finished its release. `layerLfoValues`
        /// is this sample's shared, layer-wide LFO tick (see class doc comment) --
        /// computed once per sample by the caller (render::Engine), not per-voice.
        /// `liveModRoutes` is read fresh every sample from the caller (render::Engine's
        /// patch_.layerA.modRoutes), the same "no per-voice frozen copy" pattern
        /// `layerLfoValues` already uses -- unlike the envelope live-update exception
        /// documented on Engine::setEnvelopeLive(), a mod-route change (add/remove/
        /// retarget/reamount) takes effect on an already-sustaining voice the very
        /// next sample, not just the next note-on. This is what makes drag-to-modulate
        /// (docs/UI.md) feel immediate rather than needing a fresh keypress.
        void renderSample(const algorithm::CompiledAlgorithm& compiled,
                           const std::array<const oscillator::WavetableTable*, core::kNodesPerLayer>& wavetableTables,
                           float bpm, const std::array<float, core::kNumLfosPerLayer>& layerLfoValues,
                           const modulation::GenerativeOutputValues& generativeValues,
                           const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& liveModRoutes,
                           const core::FixedVector<patch::MetaModRoute, 8>& liveMetaRoutes,
                           render::QualityMode qualityMode, float externalSampleL, float externalSampleR,
                           float& outLeft, float& outRight,
                           std::array<float, core::kNodesPerLayer>* operatorPeakScratch = nullptr) noexcept
        {
            if (noteNumber < 0 && !envelopeIsActive(0))
            {
                outLeft = 0.0f;
                outRight = 0.0f;
                return;
            }

            const float pitchBendHz = currentFrequencyHz_ *
                                       (std::pow(2.0f, (expression.pitchBendSemitones + expression.mpePitch) / 12.0f) - 1.0f);
            const float effectiveFreq = currentFrequencyHz_ + pitchBendHz;

            // Mod sources that don't depend on this sample's algorithm-graph output are
            // rendered first, so their values can modulate the operators that ARE about
            // to run this sample (e.g. LFO -> operator level tremolo).
            modulation::ModSourceValues sourceValues;
            const bool voiceLfoRoutesActive = modulation::ModMatrixExecutor::hasActiveVoiceLfoRoutes(liveModRoutes);
            if (voiceLfoRoutesActive)
            {
                for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                    sourceValues.voiceLfos[i] = lfos[i].renderSample(lfoParams[i], bpm);
            }
            sourceValues.layerLfos = layerLfoValues;
            for (std::size_t i = 0; i < core::kNumEnvelopesPerLayer; ++i)
                sourceValues.envelopes[i] = renderEnvelopeSample(i);
            const float env = sourceValues.envelopes[0]; // envelopes[0] is the amp envelope -- drives the VCA below.
            sourceValues.velocity = velocity;
            sourceValues.channelPressure = expression.channelPressure;
            sourceValues.polyAftertouch = expression.polyAftertouch;
            sourceValues.mpeSlide = expression.mpeSlide;
            sourceValues.modWheel = expression.modWheel;
            sourceValues.expression = expression.expression;
            sourceValues.macros = macroValues;
            sourceValues.randomT = generativeValues.randomT;
            sourceValues.randomX = generativeValues.randomX;
            sourceValues.randomOutputs = generativeValues.outputs;

            const auto modOut = modulation::ModMatrixExecutor::hasAudioRateModRoutes(liveModRoutes)
                                    ? modulation::ModMatrixExecutor::apply(liveModRoutes, sourceValues, liveMetaRoutes)
                                    : modulation::ModMatrixExecutor::applyControlRateOnly(liveModRoutes, sourceValues,
                                                                                          liveMetaRoutes);

            const int nonlinearOsFactor = render::nonlinearOversamplingFactor(qualityMode);

            // Operator-level modulation needs a per-sample-modified params copy -- but
            // only when at least one active route actually targets an Operator* field
            // (see ModMatrixExecutor::hasActiveOperatorParamRoutes's doc comment for the
            // exact field list). The common case (no such routes) skips the copy+clamp
            // loop entirely and reads operatorParams directly downstream instead --
            // safe because AlgorithmExecutor::processSample never mutates through its
            // params reference despite taking it non-const (confirmed: every use is a
            // read, passed further down into OperatorState::render(const OperatorParams&)).
            const bool operatorParamRoutesActive =
                modulation::ModMatrixExecutor::hasActiveOperatorParamRoutes(liveModRoutes);
            std::array<op::OperatorParams, core::kNodesPerLayer> modulatedParamsStorage;
            if (operatorParamRoutesActive)
            {
                modulatedParamsStorage = operatorParams;
                auto& modulatedParams = modulatedParamsStorage;
                for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
                {
                    modulatedParams[i].level *= modOut.operatorLevelMultiplier[i];
                    modulatedParams[i].wavetableFramePosition = dsp::clamp(
                        modulatedParams[i].wavetableFramePosition + modOut.operatorWavetablePositionOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].wtBend = dsp::clamp(
                        modulatedParams[i].wtBend + modOut.operatorWavetableBendOffset[i], -1.0f, 1.0f);
                    modulatedParams[i].wtAsymmetry = dsp::clamp(
                        modulatedParams[i].wtAsymmetry + modOut.operatorWavetableAsymmetryOffset[i], -1.0f, 1.0f);
                    modulatedParams[i].wtSyncRatio = dsp::clamp(
                        modulatedParams[i].wtSyncRatio + modOut.operatorWavetableSyncRatioOffset[i], 1.0f, 16.0f);
                    modulatedParams[i].wtSyncAmount = dsp::clamp(
                        modulatedParams[i].wtSyncAmount + modOut.operatorWavetableSyncAmountOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].wtFormantShift = dsp::clamp(
                        modulatedParams[i].wtFormantShift + modOut.operatorWavetableFormantOffset[i], -1.0f, 1.0f);
                    modulatedParams[i].fmModulatorRatio = dsp::clamp(
                        modulatedParams[i].fmModulatorRatio + modOut.operatorFmModulatorRatioOffset[i], 0.001f, 32.0f);
                    modulatedParams[i].fmModulatorIndex = dsp::clamp(
                        modulatedParams[i].fmModulatorIndex + modOut.operatorFmModulatorIndexOffset[i], 0.0f, 2.0f);
                    modulatedParams[i].fmModulatorFeedback = dsp::clamp(
                        modulatedParams[i].fmModulatorFeedback + modOut.operatorFmModulatorFeedbackOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].frequencyRatio = dsp::clamp(
                        modulatedParams[i].frequencyRatio + modOut.operatorFreqRatioOffset[i], 0.001f, 32.0f);
                    modulatedParams[i].phaseBend = dsp::clamp(
                        modulatedParams[i].phaseBend + modOut.operatorPhaseBendOffset[i], -1.0f, 1.0f);
                    modulatedParams[i].phaseFold = dsp::clamp(
                        modulatedParams[i].phaseFold + modOut.operatorPhaseFoldOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].phaseAsymmetry = dsp::clamp(
                        modulatedParams[i].phaseAsymmetry + modOut.operatorPhaseAsymmetryOffset[i], -1.0f, 1.0f);
                    modulatedParams[i].additivePartialCount = dsp::clamp(
                        modulatedParams[i].additivePartialCount + modOut.operatorAdditivePartialCountOffset[i], 1.0f, 64.0f);
                    modulatedParams[i].additiveTilt = dsp::clamp(
                        modulatedParams[i].additiveTilt + modOut.operatorAdditiveTiltOffset[i], -1.0f, 1.0f);
                    modulatedParams[i].additiveOddEven = dsp::clamp(
                        modulatedParams[i].additiveOddEven + modOut.operatorAdditiveOddEvenOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].additiveStretch = dsp::clamp(
                        modulatedParams[i].additiveStretch + modOut.operatorAdditiveStretchOffset[i], -1.0f, 1.0f);
                    modulatedParams[i].resonatorStructure = dsp::clamp(
                        modulatedParams[i].resonatorStructure + modOut.operatorResonatorStructureOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].resonatorDecay = dsp::clamp(
                        modulatedParams[i].resonatorDecay + modOut.operatorResonatorDecayOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].resonatorDamping = dsp::clamp(
                        modulatedParams[i].resonatorDamping + modOut.operatorResonatorDampingOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].resonatorBrightness = dsp::clamp(
                        modulatedParams[i].resonatorBrightness + modOut.operatorResonatorBrightnessOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].resonatorModeCount = dsp::clamp(
                        modulatedParams[i].resonatorModeCount + modOut.operatorResonatorModeCountOffset[i], 2.0f, 8.0f);
                    modulatedParams[i].grainDensity = dsp::clamp(
                        modulatedParams[i].grainDensity + modOut.operatorGrainDensityOffset[i], 0.5f, 200.0f);
                    modulatedParams[i].grainSizeMs = dsp::clamp(
                        modulatedParams[i].grainSizeMs + modOut.operatorGrainSizeMsOffset[i], 1.0f, 500.0f);
                    modulatedParams[i].grainPositionJitter = dsp::clamp(
                        modulatedParams[i].grainPositionJitter + modOut.operatorGrainPositionJitterOffset[i], 0.0f, 1.0f);
                    modulatedParams[i].grainPitchJitter = dsp::clamp(
                        modulatedParams[i].grainPitchJitter + modOut.operatorGrainPitchJitterOffset[i], 0.0f, 1.0f);
                }
            }
            auto& paramsForRender = operatorParamRoutesActive ? modulatedParamsStorage : operatorParams;

            const float raw = executor.processSample(compiled, paramsForRender, operatorStates, wavetableTables,
                                                      effectiveFreq, operatorFilterParams_, operatorFilters_,
                                                      modOut.operatorFilterCutoffSemitones,
                                                      modOut.operatorFilterResonanceOffset, externalSampleL,
                                                      externalSampleR, nonlinearOsFactor, operatorPeakScratch);

            const float voiceGain = std::abs(env * velocity * outputGain * stealOutputGain_);
            if (operatorPeakScratch != nullptr && voiceGain > 0.0f)
            {
                for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
                    (*operatorPeakScratch)[i] *= voiceGain;
            }

            float filtered = raw;

            float f1ModulatedCutoffHz = filterParams.cutoffHz;
            if (filterParams.enabled)
            {
                const float keyTrackFactor = dsp::filterKeyTrackFactor(effectiveFreq, filterParams.keyTrack);
                // modOut.filterCutoffSemitones is an exact 0.0f whenever no active route
                // targets FilterCutoff (ModOutputs default-constructs to 0 and only active
                // routes write non-default) -- skip the pow() call in the common case.
                const float cutoffModMultiplier = modOut.filterCutoffSemitones != 0.0f
                                                       ? std::pow(2.0f, modOut.filterCutoffSemitones / 12.0f)
                                                       : 1.0f;
                f1ModulatedCutoffHz = filterParams.cutoffHz * keyTrackFactor * cutoffModMultiplier;
            }

            const auto renderGlobalF1 = [&](float in) -> float {
                if (!filterParams.enabled)
                    return in;
                const float resonance =
                    dsp::clamp(filterParams.resonance + modOut.filterResonanceOffset, 0.0f, 1.0f);
                const float modeMorph =
                    dsp::clamp(filterParams.modeMorph + modOut.filterModeMorphOffset, 0.0f, 1.0f);
                return filter1.renderSample(in, filterParams.mode, modeMorph, f1ModulatedCutoffHz, resonance);
            };

            const auto renderGlobalF2 = [&](float in) -> float {
                if (!filter2Params.enabled)
                    return in;
                const float cutoffHz = filter::computeFilter2CutoffHz(filterParams.enabled, f1ModulatedCutoffHz,
                                                                      filter2Params, effectiveFreq);
                const float resonance = dsp::clamp(filter2Params.resonance, 0.0f, 1.0f);
                const float drive = dsp::clamp(filter2Params.drive + modOut.filterDriveOffset, 0.0f, 1.0f);
                return filter2.renderSample(in, cutoffHz, resonance, drive);
            };

            const float routingMorph =
                dsp::clamp(filterRouting + modOut.filterRoutingOffset, 0.0f, 1.0f);
            filtered = filter::applyDualFilterRouting(raw, routingMorph, filterParams.enabled,
                                                      filter2Params.enabled, renderGlobalF1, renderGlobalF2);

            const float amp = dsp::clamp(filtered * env * velocity * outputGain * stealOutputGain_, -16.0f, 16.0f);

            const float panClamped = dsp::clamp(pan + modOut.panOffset, -1.0f, 1.0f);
            if (panClamped != lastPanClamped_)
            {
                const float panRad = (panClamped * 0.5f + 0.5f) * (dsp::kPi * 0.5f);
                cosPanCached_ = std::cos(panRad);
                sinPanCached_ = std::sin(panRad);
                lastPanClamped_ = panClamped;
            }
            outLeft = amp * cosPanCached_;
            outRight = amp * sinPanCached_;

            advanceStealCrossfade();
            advancePortamentoGlide();

            if (!envelopeIsActive(0))
                noteNumber = -1; // fully released -- free for reuse next allocation pass.
        }

        enum class StealPhase : std::uint8_t
        {
            None = 0,
            FadeOut,
            FadeIn,
        };

        struct PendingNoteOn
        {
            int note = -1;
            int channel = 0;
            float velocityUnit = 0.0f;
            float baseFreqHz = 440.0f;
            std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer> envParams{};
            std::uint64_t ageCounter = 0;
            std::uint64_t noteGenerationId = 0;
            std::uint64_t voiceSeed = 0;
            float portamentoSeconds = 0.0f;
            bool valid = false;
        };

        [[nodiscard]] float computePortamentoCoeff(float portamentoSeconds) const noexcept
        {
            if (portamentoSeconds <= 0.0f || sampleRate_ <= 0.0)
                return 0.0f;
            const float tau = static_cast<float>(portamentoSeconds * sampleRate_);
            return tau > 1.0f ? 1.0f - std::exp(-1.0f / tau) : 1.0f;
        }

        void advancePortamentoGlide() noexcept
        {
            if (portamentoCoeff_ <= 0.0f)
                return;
            currentFrequencyHz_ += (glideTargetHz_ - currentFrequencyHz_) * portamentoCoeff_;
            if (std::abs(glideTargetHz_ - currentFrequencyHz_) < 0.01f)
            {
                currentFrequencyHz_ = glideTargetHz_;
                portamentoCoeff_ = 0.0f;
            }
        }

        void applyNoteOnInternal(int note, int channel, float velocityUnit, float baseFreqHz,
                                 const std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer>& envParams,
                                 std::uint64_t ageCounter, std::uint64_t noteGenerationId,
                                 std::uint64_t voiceSeed, float portamentoSeconds) noexcept
        {
            const bool legatoGlide = portamentoSeconds > 0.0f && gateOn && envelopeIsActive(0) &&
                                     note != noteNumber && baseFreqHz != currentFrequencyHz_;

            noteNumber = note;
            midiChannel = channel;
            velocity = dsp::clamp(velocityUnit, 0.0f, 1.0f);
            gateOn = true;
            sustainPendingRelease = false;
            age = ageCounter;
            noteGenId = noteGenerationId;

            if (legatoGlide)
            {
                glideTargetHz_ = baseFreqHz;
                portamentoCoeff_ = computePortamentoCoeff(portamentoSeconds);
                auto legatoParams = envParams;
                legatoParams[0].legato = true;
                startEnvelope(0, legatoParams[0]);
                return;
            }

            baseFrequencyHz = baseFreqHz;
            currentFrequencyHz_ = baseFreqHz;
            glideTargetHz_ = baseFreqHz;
            portamentoCoeff_ = 0.0f;
            expression = NoteExpression{};

            // Deterministic, seeded per-voice phase variation (see docs/DSP_ENGINE.md
            // "Voice Variation"): small, controlled, reproducible -- not free-running RNG.
            // The same seeded stream that randomizes operator phase also seeds each of
            // the 8 LFOs (consumed in order, one nextU64() per LFO) so every LFO gets a
            // decorrelated-but-reproducible seed rather than 8 copies of the same one.
            const auto seed = dsp::DeterministicRng::deriveSeed(voiceSeed, id, voiceSeed ^ noteGenerationId);
            dsp::DeterministicRng rng(seed);
            for (auto& s : operatorStates)
            {
                s.reset(rng.nextFloat());
                s.seedNoise(rng.nextU64());
                s.seedResonator(rng.nextU64());
                s.seedGranular(rng.nextU64());
            }

            for (std::size_t i = 0; i < core::kNumEnvelopesPerLayer; ++i)
                startEnvelope(i, envParams[i]);
            filter1.reset();
            filter2.reset();
            for (auto& f : operatorFilters_)
                f.reset();
            for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                lfos[i].noteOn(lfoParams[i], rng.nextU64());
        }

        void commitPendingNoteOn() noexcept
        {
            if (!pendingNoteOn_.valid)
                return;

            const auto pending = pendingNoteOn_;
            pendingNoteOn_.valid = false;
            hardKill(); // amp tail already faded to zero -- clear filter/envelope state before retrigger.
            applyNoteOnInternal(pending.note, pending.channel, pending.velocityUnit, pending.baseFreqHz,
                                pending.envParams, pending.ageCounter, pending.noteGenerationId, pending.voiceSeed,
                                pending.portamentoSeconds);
            stealPhase_ = StealPhase::FadeIn;
            stealGainRamp_.setLengthSamples(stealFadeSamples_);
            stealGainRamp_.startFromCurrent(0.0f, 1.0f);
            stealOutputGain_ = 0.0f;
        }

        void advanceStealCrossfade() noexcept
        {
            switch (stealPhase_)
            {
                case StealPhase::None:
                    stealOutputGain_ = 1.0f;
                    break;
                case StealPhase::FadeOut:
                    stealOutputGain_ = stealGainRamp_.getNext();
                    if (stealGainRamp_.isFinished())
                        commitPendingNoteOn();
                    break;
                case StealPhase::FadeIn:
                    stealOutputGain_ = stealGainRamp_.getNext();
                    if (stealGainRamp_.isFinished())
                        stealPhase_ = StealPhase::None;
                    break;
            }
        }

        std::uint32_t id = 0;
        int noteNumber = -1; ///< -1 == free.
        int midiChannel = 0;
        float velocity = 0.0f;
        float releaseVelocity = 0.0f;
        bool gateOn = false;
        bool sustainPendingRelease = false;
        std::uint64_t age = 0;
        std::uint64_t noteGenId = 0;

        float baseFrequencyHz = 440.0f;
        float currentFrequencyHz_ = 440.0f;
        float glideTargetHz_ = 440.0f;
        float portamentoCoeff_ = 0.0f;
        float pan = 0.0f;
        float outputGain = 1.0f;

        // Equal-power pan cos/sin cache -- renderSample() recomputes cosPanCached_/
        // sinPanCached_ only when panClamped actually differs from lastPanClamped_
        // (exact float equality, not epsilon: an epsilon-based cache could let a
        // genuinely slowly-modulated pan silently stick stale between samples whose
        // delta is below the epsilon; exact equality only skips the true static
        // case, which is the common one -- a sustained note with no pan modulation).
        // -2.0f is out of panClamped's real [-1,1] range, so the first real sample
        // always misses the cache and computes for real.
        float lastPanClamped_ = -2.0f;
        float cosPanCached_ = 1.0f;
        float sinPanCached_ = 0.0f;

        NoteExpression expression{};

        std::array<op::OperatorParams, core::kNodesPerLayer> operatorParams{};
        std::array<op::OperatorState, core::kNodesPerLayer> operatorStates{};
        algorithm::AlgorithmExecutor executor{};

        filter::FilterParams filterParams{};
        filter::StateVariableFilter filter1{};

        filter::CharacterFilterParams filter2Params{};
        filter::CharacterFilter filter2{};
        float filterRouting = 0.0f;

        std::array<filter::FilterParams, core::kNodesPerLayer> operatorFilterParams_{};
        std::array<filter::StateVariableFilter, core::kNodesPerLayer> operatorFilters_{};

        std::array<lfo::LfoParams, core::kNumLfosPerLayer> lfoParams{};
        std::array<lfo::Lfo, core::kNumLfosPerLayer> lfos{};

        std::array<float, 8> macroValues{};
        float morphPosition = 0.0f; ///< frozen at note-on when dissemination enabled

    private:
        double sampleRate_ = 48000.0;
        StealPhase stealPhase_ = StealPhase::None;
        float stealOutputGain_ = 1.0f;
        int stealFadeSamples_ = 384;
        dsp::LinearRamp stealGainRamp_{};
        PendingNoteOn pendingNoteOn_{};
    };

} // namespace pw8::voice
