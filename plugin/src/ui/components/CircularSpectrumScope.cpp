#include "CircularSpectrumScope.h"

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr float kAttack = 0.60f;
        constexpr float kRelease = 0.095f;
        constexpr float kDisplayLerp = 0.35f;
        constexpr float kPeakDecay = 0.970f;
        constexpr float kRmsSmooth = 0.05f;
        constexpr float kPulseDecay = 0.945f;
        constexpr float kBeatThreshold = 0.026f;
        constexpr float kMaxPulseGlow = 0.32f;

        [[nodiscard]] float compressDisplayLevel(float norm) noexcept
        {
            return std::sqrt(juce::jlimit(0.0f, 1.0f, norm));
        }

        [[nodiscard]] float asymmetricSmooth(float current, float target) noexcept
        {
            const float coeff = target > current ? kAttack : kRelease;
            return current + (target - current) * coeff;
        }

        [[nodiscard]] float freqForScopeBin(int bin, int numBins) noexcept
        {
            if (numBins <= 1)
                return 20.0f;

            const float t = static_cast<float>(bin) / static_cast<float>(numBins - 1);
            return 20.0f * std::pow(20000.0f / 20.0f, t);
        }

        [[nodiscard]] float angleForBin(int bin, int numBins) noexcept
        {
            const float t = static_cast<float>(bin) / static_cast<float>(numBins);
            return -juce::MathConstants<float>::halfPi + t * juce::MathConstants<float>::twoPi;
        }
    } // namespace

    CircularSpectrumScope::CircularSpectrumScope(PatchworkEightProcessor& processor) : processor_(processor)
    {
        rebuildWindow();
        startTimerHz(45);
    }

    CircularSpectrumScope::~CircularSpectrumScope()
    {
        stopTimer();
    }

    void CircularSpectrumScope::rebuildWindow() noexcept
    {
        float sum = 0.0f;
        for (int i = 0; i < kFftSize; ++i)
        {
            const float w = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i) /
                                                     static_cast<float>(kFftSize - 1)));
            window_[static_cast<std::size_t>(i)] = w;
            sum += w;
        }
        windowSum_ = juce::jmax(1.0e-6f, sum);
    }

    float CircularSpectrumScope::binMagnitudeDb(float binIndex) const noexcept
    {
        const int i0 = static_cast<int>(binIndex);
        const int i1 = juce::jmin(kFftSize / 2 - 1, i0 + 1);
        const float frac = binIndex - static_cast<float>(i0);

        const auto magAt = [this](int idx) {
            return fftData_[static_cast<std::size_t>(idx)];
        };

        const float mag = magAt(i0) * (1.0f - frac) + magAt(i1) * frac;
        const float magNorm = juce::jmax(1.0e-8f, mag * 2.0f / windowSum_);
        return juce::Decibels::gainToDecibels(magNorm, kMinDb);
    }

    void CircularSpectrumScope::setViewMode(ScopeViewMode mode)
    {
        if (viewMode_ == mode)
            return;

        viewMode_ = mode;
        hasFreshData_ = false;
        vu_.reset();
        repaint();
    }

    bool CircularSpectrumScope::pullAndAnalyseFft() noexcept
    {
        sampleRate_ = processor_.getScopeSampleRate();
        if (sampleRate_ <= 0.0)
            sampleRate_ = 48000.0;

        const int pulled = processor_.readScopeSamples(captureBuffer_.data(), kFftSize);
        if (pulled < kFftSize / 2)
            return false;

        float sumSq = 0.0f;
        for (int i = 0; i < pulled; ++i)
        {
            const float s = captureBuffer_[static_cast<std::size_t>(i)];
            sumSq += s * s;
        }
        const float frameRms = std::sqrt(sumSq / static_cast<float>(juce::jmax(1, pulled)));
        runningRms_ = runningRms_ * (1.0f - kRmsSmooth) + frameRms * kRmsSmooth;
        rmsEnvelope_ = juce::jmax(rmsEnvelope_ * 0.997f, runningRms_);

        if (runningRms_ > rmsEnvelope_ * 0.88f + kBeatThreshold)
            pulseGlow_ = juce::jmin(kMaxPulseGlow, pulseGlow_ + 0.16f);
        pulseGlow_ *= kPulseDecay;

        fftData_.fill(0.0f);
        for (int i = 0; i < kFftSize; ++i)
            fftData_[static_cast<std::size_t>(i)] = captureBuffer_[static_cast<std::size_t>(i)] * window_[static_cast<std::size_t>(i)];

        fft_.performFrequencyOnlyForwardTransform(fftData_.data());

        for (int i = 0; i < kScopeBins; ++i)
        {
            const float freq = freqForScopeBin(i, kScopeBins);
            const float binIndex = freq * static_cast<float>(kFftSize) / static_cast<float>(sampleRate_);
            const float db = binMagnitudeDb(binIndex);
            const float norm = compressDisplayLevel(juce::jmap(db, kMinDb, kMaxDb, 0.0f, 1.0f));

            const auto idx = static_cast<std::size_t>(i);
            targetLevels_[idx] = norm;
            smoothed_[idx] = asymmetricSmooth(smoothed_[idx], norm);
            displayLevels_[idx] += (smoothed_[idx] - displayLevels_[idx]) * kDisplayLerp;

            const float prevPeak = peakHold_[idx];
            if (displayLevels_[idx] >= prevPeak)
                peakHold_[idx] = displayLevels_[idx];
            else
                peakHold_[idx] = juce::jmax(displayLevels_[idx], prevPeak * kPeakDecay);
        }

        hasFreshData_ = true;
        return true;
    }

    bool CircularSpectrumScope::pullAndAnalyseVu() noexcept
    {
        const int pulled = processor_.readScopeSamples(vuCaptureBuffer_.data(), kVuCaptureSize);
        if (pulled < 32)
            return false;

        const auto [rms, peak] = scope::measureMonoBlock(vuCaptureBuffer_.data(), pulled);
        vu_.processFrame(rms, peak);

        if (vu_.rmsNorm() > 0.02f)
            pulseGlow_ = juce::jmin(kMaxPulseGlow, pulseGlow_ + 0.08f);
        pulseGlow_ *= kPulseDecay;

        hasFreshData_ = true;
        return true;
    }

    juce::Path CircularSpectrumScope::buildOutlinePath(juce::Point<float> centre, float innerR, float outerR) const
    {
        const float span = outerR - innerR;
        juce::Path outlinePath;

        for (int i = 0; i < kScopeBins; ++i)
        {
            const auto idx = static_cast<std::size_t>(i);
            const float level = displayLevels_[idx];
            const float angle = angleForBin(i, kScopeBins);
            const float r = innerR + span * level;
            const float x = centre.x + std::cos(angle) * r;
            const float y = centre.y + std::sin(angle) * r;

            if (i == 0)
                outlinePath.startNewSubPath(x, y);
            else
                outlinePath.lineTo(x, y);
        }

        outlinePath.closeSubPath();
        return outlinePath;
    }

    void CircularSpectrumScope::paintPeakHold(juce::Graphics& g, juce::Point<float> centre, float innerR,
                                              float outerR) const
    {
        if (!hasFreshData_)
            return;

        const float span = outerR - innerR;
        g.setColour(palette::kAccent.withAlpha(0.55f + pulseGlow_ * 0.08f));

        for (int i = 0; i < kScopeBins; ++i)
        {
            const auto idx = static_cast<std::size_t>(i);
            if (peakHold_[idx] <= displayLevels_[idx] + 0.018f)
                continue;

            const float angle = angleForBin(i, kScopeBins);
            const float r = innerR + span * peakHold_[idx];
            const float x = centre.x + std::cos(angle) * r;
            const float y = centre.y + std::sin(angle) * r;
            g.fillEllipse(x - 1.5f, y - 1.5f, 3.0f, 3.0f);
        }
    }

    void CircularSpectrumScope::paintSpectrumRing(juce::Graphics& g, juce::Point<float> centre, float innerR,
                                                   float outerR) const
    {
        if (!hasFreshData_)
            return;

        const auto outlinePath = buildOutlinePath(centre, innerR, outerR);
        draw::strokeGlowPath(g, outlinePath, 0.88f + pulseGlow_ * 0.10f, 2.0f, true);
    }

    void CircularSpectrumScope::paintVuArc(juce::Graphics& g, juce::Point<float> centre, float innerR,
                                           float outerR) const
    {
        if (!hasFreshData_)
            return;

        constexpr float kStartAngle = -2.35f;
        constexpr float kEndAngle = 2.35f;
        const float span = kEndAngle - kStartAngle;
        const float arcRadius = (innerR + outerR) * 0.5f;
        const float stroke = juce::jmax(4.0f, (outerR - innerR) * 0.55f);

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, kStartAngle, kEndAngle, true);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.strokePath(track, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float levelAngle = kStartAngle + span * vu_.rmsNorm();
        if (levelAngle > kStartAngle + 0.02f)
        {
            juce::Path levelArc;
            levelArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, kStartAngle, levelAngle, true);
            draw::strokeGlowPath(g, levelArc, 0.88f + pulseGlow_ * 0.10f, stroke * 0.55f, true);
        }

        const float peakAngle = kStartAngle + span * vu_.peakHoldNorm();
        const float px = centre.x + std::cos(peakAngle) * arcRadius;
        const float py = centre.y + std::sin(peakAngle) * arcRadius;
        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.fillEllipse(px - 2.5f, py - 2.5f, 5.0f, 5.0f);

        g.setFont(fonts::label(fonts::kCaptionSize));
        g.setColour(palette::kTextSecondary);
        g.drawText("VU", juce::Rectangle<float>(centre.x - 12.0f, centre.y + innerR * 0.15f, 24.0f, 12.0f),
                   juce::Justification::centred, false);
    }

    void CircularSpectrumScope::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        if (bounds.isEmpty())
            return;

        draw::fillRecessedRoundedRect(g, bounds, bounds.getWidth() * 0.5f);

        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 6.0f;
        const auto centre = bounds.getCentre();
        const float innerR = radius * 0.42f;
        const float outerR = radius * 0.92f;

        g.setColour(palette::kBackgroundBottom.withAlpha(0.92f));
        g.fillEllipse(centre.x - innerR + 2.0f, centre.y - innerR + 2.0f, (innerR - 2.0f) * 2.0f,
                      (innerR - 2.0f) * 2.0f);

        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawEllipse(centre.x - innerR, centre.y - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

        if (viewMode_ == ScopeViewMode::Fft)
        {
            paintSpectrumRing(g, centre, innerR, outerR);
            paintPeakHold(g, centre, innerR, outerR);
        }
        else
        {
            paintVuArc(g, centre, innerR, outerR);
        }

        g.setColour(branding::glowColour().withAlpha(0.06f + pulseGlow_ * 0.05f));
        g.drawEllipse(centre.x - outerR - 0.5f, centre.y - outerR - 0.5f, (outerR + 0.5f) * 2.0f,
                      (outerR + 0.5f) * 2.0f, 1.0f + pulseGlow_ * 0.2f);
    }

    void CircularSpectrumScope::timerCallback()
    {
        const bool updated = viewMode_ == ScopeViewMode::Fft ? pullAndAnalyseFft() : pullAndAnalyseVu();
        if (updated)
            repaint();
    }

} // namespace pw8::plugin::ui
