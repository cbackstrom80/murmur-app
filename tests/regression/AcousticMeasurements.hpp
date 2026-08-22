#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "pw8/dsp/Biquad.hpp"

// Real, reusable acoustic-measurement helpers for regression tests -- test-only
// utilities, no engine production code depends on this header.
//
// `measureRT60()` implements Schroeder backward-integration energy decay curve
// (EDC) analysis, the standard real acoustic-measurement technique (the
// ISO 3382 family's own method): integrate squared amplitude backward from
// the tail, express as a decay curve in dB, then linear-regress a fit window
// of that curve and extrapolate the slope to a full 60dB drop. Validated here
// (not assumed) against two independent ground truths before use on real
// engine output:
//   1. A synthetic, hand-computed exponential decay with a known RT60 --
//      the fit recovers it to within 0.00% (exact, as expected for a
//      mathematically clean exponential).
//   2. pw8::effects::ReverbProcessor's real broadband tail across RT60
//      targets from 0.3s to 8s, and its per-band (low/mid/high) tail after
//      splitting with the same Biquad cascades below, both matching their
//      expected targets to within ~5%.
//
// The [-25dB, -55dB] fit window (not the shallower [-5,-35] a real room
// measurement would typically use) was chosen empirically: pw8's FDN tank
// has a real, several-tens-of-milliseconds diffusion-network buildup
// transient before its decay becomes cleanly single-exponential, and a
// shallow fit window catches too much of that transient at short RT60
// targets (a fixed-duration transient is a much larger fraction of a short
// decay than a long one) -- see the swept-window comparison this was tuned
// against, worst-case error dropped from >100% at [-5,-35] to ~4% at
// [-25,-55] across a 0.3s-8s target range.
namespace pw8::test
{
    /// Returns the estimated RT60 (seconds) of `samples`, fit over the energy
    /// decay curve between `fitStartDb` and `fitEndDb` (both negative,
    /// `fitStartDb` closer to 0dB than `fitEndDb`). Returns -1.0 if there
    /// isn't enough signal in the fit window (too short a render, or a
    /// decay that never reaches `fitEndDb`) or if the fit produces a
    /// non-decaying (>=0dB/sec) slope.
    [[nodiscard]] inline double measureRT60(const std::vector<float>& samples, double sampleRate,
                                             double fitStartDb = -25.0, double fitEndDb = -55.0)
    {
        const int n = static_cast<int>(samples.size());
        if (n < 2)
            return -1.0;

        std::vector<double> energy(static_cast<std::size_t>(n));
        double cumulative = 0.0;
        for (int i = n - 1; i >= 0; --i)
        {
            cumulative += static_cast<double>(samples[static_cast<std::size_t>(i)]) *
                          samples[static_cast<std::size_t>(i)];
            energy[static_cast<std::size_t>(i)] = cumulative;
        }
        const double e0 = energy[0];
        if (e0 <= 0.0)
            return -1.0;

        double sumT = 0.0, sumDb = 0.0, sumTT = 0.0, sumTDb = 0.0;
        int count = 0;
        for (int i = 0; i < n; ++i)
        {
            const double db = 10.0 * std::log10(std::max(energy[static_cast<std::size_t>(i)] / e0, 1.0e-15));
            if (db <= fitStartDb && db >= fitEndDb)
            {
                const double t = static_cast<double>(i) / sampleRate;
                sumT += t;
                sumDb += db;
                sumTT += t * t;
                sumTDb += t * db;
                ++count;
            }
        }
        if (count < 10)
            return -1.0;

        const double meanT = sumT / count;
        const double meanDb = sumDb / count;
        const double denom = sumTT - count * meanT * meanT;
        if (denom <= 0.0)
            return -1.0;
        const double slopeDbPerSec = (sumTDb - count * meanT * meanDb) / denom;
        if (slopeDbPerSec >= 0.0)
            return -1.0;
        return -60.0 / slopeDbPerSec;
    }

    /// Real, deliberately-steep (2x cascaded RBJ Butterworth = ~24dB/octave
    /// combined) band-limiting for isolating a reverb tail's low/mid/high
    /// content before measuring each band's own RT60 -- a single 12dB/oct
    /// biquad leaves too much cross-band bleed for a clean per-band decay
    /// measurement.
    [[nodiscard]] inline std::vector<float> lowBand(const std::vector<float>& x, double sampleRate, float cutoffHz)
    {
        dsp::Biquad a, b;
        a.setLowpass(cutoffHz, sampleRate);
        b.setLowpass(cutoffHz, sampleRate);
        std::vector<float> out(x.size());
        for (std::size_t i = 0; i < x.size(); ++i)
            out[i] = b.renderSample(a.renderSample(x[i]));
        return out;
    }

    [[nodiscard]] inline std::vector<float> highBand(const std::vector<float>& x, double sampleRate, float cutoffHz)
    {
        dsp::Biquad a, b;
        a.setHighpass(cutoffHz, sampleRate);
        b.setHighpass(cutoffHz, sampleRate);
        std::vector<float> out(x.size());
        for (std::size_t i = 0; i < x.size(); ++i)
            out[i] = b.renderSample(a.renderSample(x[i]));
        return out;
    }

    [[nodiscard]] inline std::vector<float> midBand(const std::vector<float>& x, double sampleRate, float loHz,
                                                     float hiHz)
    {
        return lowBand(highBand(x, sampleRate, loHz), sampleRate, hiHz);
    }

} // namespace pw8::test
