#include "HeaderSpectrumScope.h"

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr float kMinHz = 20.0f;
        constexpr float kMaxHz = 20000.0f;
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
                return kMinHz;

            const float t = static_cast<float>(bin) / static_cast<float>(numBins - 1);
            return kMinHz * std::pow(kMaxHz / kMinHz, t);
        }

        [[nodiscard]] float xForFrequency(float freq, juce::Rectangle<float> bounds) noexcept
        {
            const float logMin = std::log10(kMinHz);
            const float logMax = std::log10(kMaxHz);
            const float logF = std::log10(juce::jlimit(kMinHz, kMaxHz, freq));
            const float norm = (logF - logMin) / (logMax - logMin);
            return bounds.getX() + norm * bounds.getWidth();
        }

        [[nodiscard]] float yForLevel(float level, juce::Rectangle<float> bounds) noexcept
        {
            return bounds.getBottom() - juce::jlimit(0.0f, 1.0f, level) * bounds.getHeight();
        }
    } // namespace

    HeaderSpectrumScope::HeaderSpectrumScope(PatchworkEightProcessor& processor) : processor_(processor)
    {
        rebuildWindow();
        startTimerHz(45);
    }

    HeaderSpectrumScope::~HeaderSpectrumScope()
    {
        stopTimer();
    }

    void HeaderSpectrumScope::rebuildWindow() noexcept
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

    float HeaderSpectrumScope::binMagnitudeDb(float binIndex) const noexcept
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

    void HeaderSpectrumScope::setViewMode(ScopeViewMode mode)
    {
        if (viewMode_ == mode)
            return;

        viewMode_ = mode;
        hasFreshData_ = false;
        vu_.reset();
        repaint();
    }

    bool HeaderSpectrumScope::pullAndAnalyseFft() noexcept
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

    bool HeaderSpectrumScope::pullAndAnalyseVu() noexcept
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

    juce::Rectangle<float> HeaderSpectrumScope::graphBounds() const noexcept
    {
        return getLocalBounds().toFloat().reduced(2.0f, 3.0f);
    }

    juce::Path HeaderSpectrumScope::buildOutlinePath(juce::Rectangle<float> bounds) const
    {
        juce::Path outlinePath;
        outlinePath.startNewSubPath(bounds.getX(), yForLevel(displayLevels_[0], bounds));

        for (int i = 1; i < kScopeBins; ++i)
        {
            const float freq = freqForScopeBin(i, kScopeBins);
            const float x = xForFrequency(freq, bounds);
            const auto idx = static_cast<std::size_t>(i);
            outlinePath.lineTo(x, yForLevel(displayLevels_[idx], bounds));
        }

        return outlinePath;
    }

    void HeaderSpectrumScope::paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        g.setColour(palette::kBorder.withAlpha(0.45f));
        for (const float freq : {100.0f, 1000.0f, 10000.0f})
        {
            const float x = xForFrequency(freq, bounds);
            g.drawVerticalLine(static_cast<int>(x), bounds.getY() + 2.0f, bounds.getBottom() - 10.0f);
        }

        g.setColour(palette::kBorder.withAlpha(0.28f));
        for (int i = 1; i < 4; ++i)
        {
            const float y = bounds.getY() + bounds.getHeight() * static_cast<float>(i) / 4.0f;
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
        }

        g.setFont(fonts::label(fonts::kCaptionSize));
        g.setColour(palette::kTextSecondary);
        g.drawText("100", juce::Rectangle<float>(xForFrequency(100.0f, bounds) - 10.0f, bounds.getBottom() - 10.0f, 24.0f, 10.0f),
                   juce::Justification::centred);
        g.drawText("1k", juce::Rectangle<float>(xForFrequency(1000.0f, bounds) - 10.0f, bounds.getBottom() - 10.0f, 24.0f, 10.0f),
                   juce::Justification::centred);
        g.drawText("10k", juce::Rectangle<float>(xForFrequency(10000.0f, bounds) - 12.0f, bounds.getBottom() - 10.0f, 28.0f, 10.0f),
                   juce::Justification::centred);
    }

    void HeaderSpectrumScope::paintPeakHold(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        g.setColour(palette::kAccent.withAlpha(0.55f + pulseGlow_ * 0.08f));
        for (int i = 0; i < kScopeBins; ++i)
        {
            const auto idx = static_cast<std::size_t>(i);
            if (peakHold_[idx] <= displayLevels_[idx] + 0.012f)
                continue;

            const float freq = freqForScopeBin(i, kScopeBins);
            const float x = xForFrequency(freq, bounds);
            const float yPeak = yForLevel(peakHold_[idx], bounds);
            g.fillEllipse(x - 1.5f, yPeak - 3.0f, 3.0f, 3.0f);
        }
    }

    void HeaderSpectrumScope::paintSpectrum(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        if (!hasFreshData_)
            return;

        const auto outlinePath = buildOutlinePath(bounds);
        draw::strokeGlowPath(g, outlinePath, 0.88f + pulseGlow_ * 0.10f, 2.0f, true);
        paintPeakHold(g, bounds);
    }

    void HeaderSpectrumScope::paintVuMeter(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        if (!hasFreshData_)
            return;

        const auto meterArea = bounds.reduced(8.0f, 10.0f).withTrimmedTop(bounds.getHeight() * 0.28f);
        const float barHeight = juce::jmin(18.0f, meterArea.getHeight() * 0.42f);
        const auto bar = juce::Rectangle<float>(meterArea.getX(), meterArea.getCentreY() - barHeight * 0.5f,
                                                meterArea.getWidth(), barHeight);

        g.setColour(palette::kBackgroundBottom.withAlpha(0.88f));
        g.fillRoundedRectangle(bar, barHeight * 0.35f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bar, barHeight * 0.35f, 1.0f);

        const float fillWidth = bar.getWidth() * vu_.rmsNorm();
        if (fillWidth > 1.0f)
        {
            auto fill = bar.withWidth(fillWidth).reduced(1.5f, 1.5f);
            g.setColour(palette::kAccent.withAlpha(0.22f + pulseGlow_ * 0.08f));
            g.fillRoundedRectangle(fill, fill.getHeight() * 0.35f);

            juce::Path fillOutline;
            fillOutline.addRoundedRectangle(fill, fill.getHeight() * 0.35f);
            draw::strokeGlowPath(g, fillOutline, 0.82f + pulseGlow_ * 0.08f, 1.4f, true);
        }

        const float peakX = bar.getX() + bar.getWidth() * vu_.peakHoldNorm();
        g.setColour(palette::kAccentWarm.withAlpha(0.75f));
        g.fillRect(peakX - 1.0f, bar.getY() + 2.0f, 2.0f, bar.getHeight() - 4.0f);

        g.setFont(fonts::label(fonts::kCaptionSize));
        g.setColour(palette::kTextSecondary);
        g.drawText("L", bar.translated(-14.0f, 0.0f), juce::Justification::centredRight, false);
        g.drawText("R", bar.withTrimmedLeft(bar.getWidth() + 4.0f), juce::Justification::centredLeft, false);
    }

    void HeaderSpectrumScope::paint(juce::Graphics& g)
    {
        const auto bounds = graphBounds();
        if (bounds.isEmpty())
            return;

        draw::fillRecessedRoundedRect(g, bounds, 5.0f);

        g.setColour(palette::kTopHighlight.withAlpha(0.04f + pulseGlow_ * 0.02f));
        g.fillRoundedRectangle(bounds.reduced(1.0f), 4.0f);

        if (viewMode_ == ScopeViewMode::Fft)
        {
            paintGrid(g, bounds);
            paintSpectrum(g, bounds);
        }
        else
        {
            paintVuMeter(g, bounds);
        }

        g.setColour(branding::glowColour().withAlpha(0.06f + pulseGlow_ * 0.05f));
        g.drawRoundedRectangle(bounds.expanded(0.5f), 5.5f, 1.0f + pulseGlow_ * 0.2f);
    }

    void HeaderSpectrumScope::timerCallback()
    {
        const bool updated = viewMode_ == ScopeViewMode::Fft ? pullAndAnalyseFft() : pullAndAnalyseVu();
        if (updated)
            repaint();
    }

} // namespace pw8::plugin::ui
