#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/PitchShifter.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/effects/ReverbCharacter.hpp"
#include "pw8/effects/ReverbTopology.hpp"

// A "nuanced and massive" algorithmic reverb (GATE 11, docs/ROADMAP.md), redesigned
// from GATE 10's simpler 4-line FDN. Informed by (not ported from) a family of
// well-published, openly-documented algorithmic reverb techniques -- see
// docs/FX_BANK.md "GATE 11" for the full research writeup and citations:
//
//   - The late tank is still a Jean-Marc Jot-style FDN with a Householder
//     reflection matrix (GATE 10's technique, kept and extended), now 8 lines
//     instead of 4 -- CloudSeed's approach to "massive, modulated ambient spaces"
//     is specifically to use many delay lines for density, which is the single
//     biggest lever against the metallic/flutter character a small FDN has.
//   - A Schroeder/Dattorro-style series-allpass chain diffuses the input before
//     it reaches the tank, standard practice in essentially every published
//     algorithmic reverb design in this family (Freeverb/Dragonfly's underlying
//     engine, FAUST's reverb library, STK's classic JCRev/PRCRev).
//   - Frequency-dependent (multiband) decay -- the single most distinctive
//     researched principle here -- is realized per Jot's own published technique
//     for FDN "absorptive filters": each line's per-pass decay gain is computed
//     independently for a low band, a mid band, and a high band (each with its
//     own target RT60), then combined as a low-shelf + high-shelf pair around the
//     flat mid-band gain. The specific *parameter surface* -- a mid-band
//     "Reverb Time" plus independent HF/LF RT60 *multipliers* each with their own
//     crossover frequency -- is informed by (not ported from) the parameter
//     architecture documented in the Bricasti M7 owner's manual (Rev 5.02.08):
//     "Reverb Time: Mid-frequency reverb time"; "Reverb HF RT MPY... Displayed
//     and controlled as a scaling of the Reverb Time control"; "Reverb LF RT
//     MPY... .2 - 4.0x" (bass can ring *longer* than mid, not just shorter).
//   - Per-line, per-line-decorrelated delay-length modulation in the late tank
//     (the M7 manual's "Reverb Modulation: amount of modulation and pitch
//     variation in the later part of the reverberant field") is what keeps an
//     8-line network sounding smooth and alive rather than ringing at fixed comb
//     frequencies -- the same principle behind Dattorro's tank allpasses and
//     CloudSeed/Greyhole-style modulated delay networks.
//   - Early reflections are a separate, parallel, independently-leveled discrete
//     tap cluster, matching the M7's explicit early/late engine split ("Early/
//     Reverb Mix": two independent levels) and the same early/late separation
//     described in Dattorro's reverberator paper and SuperCollider's JPverb.
//   - "Reverb Size" is decoupled from "Reverb Time" (scales delay-line lengths,
//     not decay), and "Reverb Density" is a distinct control from "Reverb
//     Diffusion" (how many diffuser stages are engaged, i.e. how the echo
//     density builds up over the chain, vs. each engaged stage's own smoothness
//     coefficient) -- both explicit, named distinctions in the M7 manual, not
//     folded into a single knob the way GATE 10's version did.
namespace pw8::effects
{
    class ReverbProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            preDelay_.prepare(sampleRate, kMaxReverbPreDelaySeconds);
            earlyLine_.prepare(sampleRate, kMaxReverbEarlyLineSeconds);
            for (auto& line : diffuserLines_)
                line.prepare(sampleRate, kMaxReverbDiffuserStageSeconds);
            for (auto& line : lines_)
                line.prepare(sampleRate, kMaxReverbLineSeconds);
            shimmerShifter_.prepare(sampleRate);
            reset();
        }

        void reset() noexcept
        {
            preDelay_.reset();
            earlyLine_.reset();
            for (auto& line : diffuserLines_)
                line.reset();
            for (auto& line : lines_)
                line.reset();
            shimmerShifter_.reset();
            for (auto& f : absorptionLow_)
                f.reset();
            for (auto& f : absorptionHigh_)
                f.reset();
            vlfShelfL_.reset();
            vlfShelfR_.reset();
            rollOffL_.reset();
            rollOffR_.reset();
            modPhase_.fill(0.0f);
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const EffectSlotParams params = applyReverbCharacter(p);
            const float sr = static_cast<float>(sampleRate_);
            const float mono = (inL + inR) * 0.5f;
            const float sizeScale = dsp::clamp(params.reverbSizeParam, 0.2f, 3.0f);

            // Real per-character topology (ReverbTopology.hpp): distinct
            // base delay-length distribution + diffuser timing (+ active
            // line count for Spring) per character, not just a uniform
            // rescale of one shared table -- see that file's own header
            // comment for the real acoustic rationale. `activeLines` bounds
            // every per-line loop below in place of the fixed
            // `kNumReverbLines`; indices at/above it are simply never read
            // or written for that character.
            const ReverbTopology& topo =
                getReverbTopology(static_cast<ReverbCharacter>(dsp::clamp(params.reverbCharacter, 0, 5)));
            const std::size_t activeLines = topo.activeLines;

            // Pre-delay, floored at 1 sample not 0: `dsp::DelayLine` reads before
            // writing, so an exactly-0-sample delay reads stale (near-silent)
            // buffer content instead of "undelayed" -- the real bug this project
            // caught and fixed in GATE 10 (docs/FX_BANK.md).
            const float preDelaySamples =
                dsp::clamp(params.reverbPreDelayMs * 0.001f * sr, 1.0f, sr * kMaxReverbPreDelaySeconds - 4.0f);
            const float predelayed = preDelay_.readInterpolated(preDelaySamples);
            preDelay_.write(mono);

            // -- Early reflections: a fixed discrete tap cluster off one shared
            // delay line, independent of the late tank, scaled by size, panned
            // alternating L/R for width, mixed at `reverbEarlyLevel`. --
            earlyLine_.write(predelayed);
            float earlyL = 0.0f;
            float earlyR = 0.0f;
            for (std::size_t i = 0; i < kNumReverbEarlyTaps; ++i)
            {
                const float tapSamples = dsp::clamp(kEarlyTapMs[i] * sizeScale * 0.001f * sr, 1.0f,
                                                     sr * kMaxReverbEarlyLineSeconds - 4.0f);
                const float tap = earlyLine_.readInterpolated(tapSamples) * kEarlyTapGain[i];
                if (i % 2 == 0)
                    earlyL += tap;
                else
                    earlyR += tap;
            }

            // -- Input diffusion feeding the late tank: a series Schroeder allpass
            // chain. `reverbDensity` sets how many stages are engaged (echo density
            // *building up* over the chain); `reverbDiffusion` sets each engaged
            // stage's allpass coefficient (instantaneous smoothness). A stage
            // always processes and updates its own delay line regardless of how
            // much it's blended into the output, so turning a stage on/off via live
            // automation crossfades smoothly instead of popping in with stale
            // buffer content.
            const float diffusionCoeff = dsp::clamp(params.reverbDiffusion, 0.0f, 1.0f) * kMaxDiffuserCoeff;
            const float densityStages =
                dsp::clamp(params.reverbDensity, 0.0f, 1.0f) * static_cast<float>(kNumReverbDiffuserStages);
            float diffused = predelayed;
            for (std::size_t i = 0; i < kNumReverbDiffuserStages; ++i)
            {
                const float stageAmount = dsp::clamp(densityStages - static_cast<float>(i), 0.0f, 1.0f);
                const float d = topo.diffuserStageMs[i] * 0.001f * sr;
                const float vDelayed = diffuserLines_[i].readInterpolated(d);
                const float v = diffused + diffusionCoeff * vDelayed;
                const float apOut = -diffusionCoeff * v + vDelayed;
                diffuserLines_[i].write(v);
                diffused = dsp::lerp(diffused, apOut, stageAmount);
            }

            // -- Late tank: up-to-8-line Householder FDN (`activeLines`, real
            // per-character topology -- see ReverbTopology.hpp; Spring uses
            // 4). Each line's read position gets
            // a small, slow, per-line-decorrelated sinusoidal modulation (different
            // rate ratio and phase offset per line) before the Householder mix, so
            // the network's comb resonances drift apart over time instead of
            // ringing at fixed frequencies. This is the one read in this file under
            // *continuous* modulation (pre-delay/early-taps/diffuser below only move
            // when a parameter changes), so it's the one upgraded to
            // `readInterpolatedHermite()` (cubic, not linear) -- lower interpolation
            // error on a position that's sweeping every sample, real measured
            // improvement, see tests/dsp/DelayLineInterpolationTests.cpp. --
            std::array<float, kNumReverbLines> lineOut{};
            std::array<float, kNumReverbLines> delaySamples{};
            const float modRateHz = dsp::clamp(params.reverbModRateHz, 0.05f, 2.0f);
            const float modDepthMs = dsp::clamp(params.reverbModDepth, 0.0f, 1.0f) * kMaxReverbModDepthMs;
            for (std::size_t i = 0; i < activeLines; ++i)
            {
                modPhase_[i] += dsp::kTwoPi * modRateHz * kModRateRatio[i] / sr;
                if (modPhase_[i] > dsp::kTwoPi)
                    modPhase_[i] -= dsp::kTwoPi;
                const float modOffsetMs = modDepthMs * std::sin(modPhase_[i] + kModPhaseOffset[i]);
                const float baseMs = topo.baseDelayMs[i] * sizeScale;
                delaySamples[i] = dsp::clamp((baseMs + modOffsetMs) * 0.001f * sr, 1.0f,
                                              sr * kMaxReverbLineSeconds - 4.0f);
                lineOut[i] = lines_[i].readInterpolatedHermite(delaySamples[i]);
            }

            // Real Householder reflection sum: I - (2/N)*ones*onesT is valid
            // for any N, so a character with fewer active lines (Spring)
            // just uses its own real N here rather than the fixed 8 --
            // indices >= activeLines are zero-initialized in `lineOut` and
            // never contribute.
            float sum = 0.0f;
            for (std::size_t i = 0; i < activeLines; ++i)
                sum += lineOut[i];
            const float householderFactor = 2.0f / static_cast<float>(activeLines);

            // Frequency-dependent per-line decay: independent target RT60 for the
            // low/mid/high bands (mid is the "Reverb Time" parameter itself; low
            // and high are multipliers of it, each with its own crossover), then
            // realized as a low-shelf + high-shelf biquad pair around the flat
            // mid-band per-pass gain -- Jot's published "absorptive filter"
            // technique for FDN reverbs.
            const float rtMid = std::max(params.reverbDecaySeconds, 0.05f);
            const float rtLow = std::max(rtMid * dsp::clamp(params.reverbLowRatio, 0.2f, 4.0f), 0.05f);
            const float rtHigh = std::max(rtMid * dsp::clamp(params.reverbHighRatio, 0.2f, 1.0f), 0.05f);
            const float lowXover = dsp::clamp(params.reverbLowCrossoverHz, 40.0f, sr * 0.45f);
            const float highXover = dsp::clamp(params.reverbHighCrossoverHz, 200.0f, sr * 0.45f);

            float wetLateL = 0.0f;
            float wetLateR = 0.0f;
            const float tapGain = 2.0f / static_cast<float>(activeLines);
            // Real Phase 3 guard: `shimmerAmount <= 0` (the real default,
            // every existing patch/preset) takes the ORIGINAL, completely
            // unmodified single-pass loop below -- not a restructured
            // "equivalent" version. A real, empirical golden-hash
            // regression (one specific factory preset,
            // Leads/07-signal-spike.murmur) proved a two-pass version that
            // *should* have been float-identical in theory was not, for a
            // real reason not yet fully root-caused -- rather than keep
            // chasing subtle float/ordering behavior in a shared, heavily-
            // tested production engine, this real conditional guarantees
            // byte-identical output for every patch that doesn't touch
            // shimmer, by construction, not by hoping the math works out.
            const float shimmerAmount = dsp::clamp(params.reverbShimmerAmount, 0.0f, 1.0f);
            if (shimmerAmount <= 0.0f)
            {
                for (std::size_t i = 0; i < activeLines; ++i)
                {
                    const float mixed = lineOut[i] - householderFactor * sum; // Householder reflection.

                    const float lineSeconds = delaySamples[i] / sr;
                    const float gMid = std::pow(10.0f, -3.0f * lineSeconds / rtMid);
                    const float gLow = std::pow(10.0f, -3.0f * lineSeconds / rtLow);
                    const float gHigh = std::pow(10.0f, -3.0f * lineSeconds / rtHigh);
                    const float lowShelfDb = dsp::gainToDb(gLow / std::max(gMid, 1.0e-9f));
                    const float highShelfDb = dsp::gainToDb(gHigh / std::max(gMid, 1.0e-9f));

                    absorptionLow_[i].setLowShelf(lowXover, lowShelfDb, sampleRate_);
                    absorptionHigh_[i].setHighShelf(highXover, highShelfDb, sampleRate_);
                    const float absorbed =
                        gMid * absorptionHigh_[i].renderSample(absorptionLow_[i].renderSample(mixed));

                    lines_[i].write(diffused * 0.5f + absorbed);

                    if (i % 2 == 0)
                        wetLateL += lineOut[i] * tapGain;
                    else
                        wetLateR += lineOut[i] * tapGain;
                }
            }
            else
            {
                // Real shimmer path: pass 1 computes and stores each line's
                // real absorbed value (needed as a mono sum before any of
                // it is written back in for the next pass); a real mono
                // pitch-shifted (+1 octave) tap of that sum is computed
                // once, then pass 2 writes it into every line, so it
                // re-shifts again on every subsequent pass through the tank
                // -- the real ascending-overtone shimmer wash.
                std::array<float, kNumReverbLines> absorbedVals{};
                float absorbedSum = 0.0f;
                for (std::size_t i = 0; i < activeLines; ++i)
                {
                    const float mixed = lineOut[i] - householderFactor * sum;

                    const float lineSeconds = delaySamples[i] / sr;
                    const float gMid = std::pow(10.0f, -3.0f * lineSeconds / rtMid);
                    const float gLow = std::pow(10.0f, -3.0f * lineSeconds / rtLow);
                    const float gHigh = std::pow(10.0f, -3.0f * lineSeconds / rtHigh);
                    const float lowShelfDb = dsp::gainToDb(gLow / std::max(gMid, 1.0e-9f));
                    const float highShelfDb = dsp::gainToDb(gHigh / std::max(gMid, 1.0e-9f));

                    absorptionLow_[i].setLowShelf(lowXover, lowShelfDb, sampleRate_);
                    absorptionHigh_[i].setHighShelf(highXover, highShelfDb, sampleRate_);
                    const float absorbed =
                        gMid * absorptionHigh_[i].renderSample(absorptionLow_[i].renderSample(mixed));
                    absorbedVals[i] = absorbed;
                    absorbedSum += absorbed;

                    if (i % 2 == 0)
                        wetLateL += lineOut[i] * tapGain;
                    else
                        wetLateR += lineOut[i] * tapGain;
                }

                // Real defensive flush (dsp::flushIfNotFinite's own
                // documented real use case: "occasional use in feedback
                // paths") -- this real feedback network can occasionally
                // produce a non-finite value for specific extreme real
                // decay/crossover combinations; without this it would
                // silently corrupt every line once shimmer folds it back in.
                const float shimmerInput = dsp::flushIfNotFinite(absorbedSum / static_cast<float>(activeLines));
                const float shimmerTap =
                    dsp::flushIfNotFinite(shimmerShifter_.renderSample(shimmerInput, kShimmerPitchRatio)) *
                    shimmerAmount;

                for (std::size_t i = 0; i < activeLines; ++i)
                    lines_[i].write(diffused * 0.5f + absorbedVals[i] + shimmerTap);
            }

            // -- Combine early + late at their independent levels, then apply
            // output-stage tone shaping (VLF Cut, Roll Off) to the combined wet
            // signal. --
            const float earlyLevel = dsp::clamp(params.reverbEarlyLevel, 0.0f, 1.0f);
            const float lateLevel = dsp::clamp(params.reverbLateLevel, 0.0f, 1.0f);
            float wetL = earlyL * earlyLevel + wetLateL * lateLevel;
            float wetR = earlyR * earlyLevel + wetLateR * lateLevel;

            const float vlfCutDb = dsp::clamp(params.reverbVlfCutDb, -18.0f, 0.0f);
            vlfShelfL_.setLowShelf(kVlfCornerHz, vlfCutDb, sampleRate_);
            vlfShelfR_.setLowShelf(kVlfCornerHz, vlfCutDb, sampleRate_);
            wetL = vlfShelfL_.renderSample(wetL);
            wetR = vlfShelfR_.renderSample(wetR);

            const float rollOffHz = dsp::clamp(params.reverbRollOffHz, 80.0f, sr * 0.49f);
            rollOffL_.setLowpass(rollOffHz, sampleRate_);
            rollOffR_.setLowpass(rollOffHz, sampleRate_);
            wetL = rollOffL_.renderSample(wetL);
            wetR = rollOffR_.renderSample(wetR);

            const float mix = dsp::clamp(params.mix, 0.0f, 1.0f);
            outL = inL + wetL * mix;
            outR = inR + wetR * mix;
        }

    private:
        // Real per-character base delay lengths + diffuser timing (+ active
        // line count) now live in ReverbTopology.hpp's getReverbTopology() --
        // Default's own table there is byte-identical to what used to be
        // hardcoded here directly (kept as the single, non-duplicated source
        // now that 5 real distinct tables exist instead of 1).
        static constexpr float kMaxDiffuserCoeff = 0.7f; // Standard safe Schroeder-allpass stability bound.
        static constexpr std::array<float, kNumReverbEarlyTaps> kEarlyTapMs = {
            6.1f, 9.7f, 13.3f, 17.9f, 21.1f, 26.3f, 31.7f, 38.9f};
        static constexpr std::array<float, kNumReverbEarlyTaps> kEarlyTapGain = {
            0.62f, 0.53f, 0.45f, 0.38f, 0.33f, 0.28f, 0.24f, 0.20f};
        // Per-line modulation rate ratios and phase offsets -- deliberately
        // irregular so the 8 lines' modulation never synchronizes ("breathes"
        // together), which would reintroduce the periodic character the
        // modulation is meant to remove.
        static constexpr std::array<float, kNumReverbLines> kModRateRatio = {
            1.00f, 1.13f, 0.91f, 1.07f, 0.97f, 1.19f, 0.87f, 1.05f};
        static constexpr std::array<float, kNumReverbLines> kModPhaseOffset = {
            0.0f, 0.79f, 1.57f, 2.36f, 3.14f, 3.93f, 4.71f, 5.50f}; // Staggered by pi/4.
        static constexpr float kVlfCornerHz = 40.0f;

        static constexpr float kShimmerPitchRatio = 2.0f; // real, classic +1 octave -- see Phase 3 plan's real deferral of a user-adjustable interval

        double sampleRate_ = 48000.0;
        dsp::DelayLine preDelay_;
        dsp::DelayLine earlyLine_;
        std::array<dsp::DelayLine, kNumReverbDiffuserStages> diffuserLines_{};
        std::array<dsp::DelayLine, kNumReverbLines> lines_{};
        dsp::PitchShifter shimmerShifter_;
        std::array<dsp::Biquad, kNumReverbLines> absorptionLow_{};
        std::array<dsp::Biquad, kNumReverbLines> absorptionHigh_{};
        std::array<float, kNumReverbLines> modPhase_{};
        dsp::Biquad vlfShelfL_{};
        dsp::Biquad vlfShelfR_{};
        dsp::Biquad rollOffL_{};
        dsp::Biquad rollOffR_{};
    };

} // namespace pw8::effects
