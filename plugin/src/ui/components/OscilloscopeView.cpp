#include "OscilloscopeView.h"

#include "../PerformanceMetricsUi.h"
#include "../PlayModeLayout.h"
#include "../ScopeVuMeter.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../visualizer/MurmurVisualizerComponent.h"
#include "../visualizer/PreviewDraw.h"
#include "../visualizer/VisualizerGpu.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] juce::Rectangle<float> desktopPlayWavePlotBounds(juce::Rectangle<float> bounds)
        {
            auto plot = bounds;
            plot.removeFromTop(static_cast<float>(layout::kDesktopPlayModeScopeMetadataHeight));
            plot.removeFromBottom(static_cast<float>(layout::kDesktopPlayModeScopePlotPaddingBottom));
            return plot.reduced(static_cast<float>(layout::kDesktopPlayModeScopeWavePaddingX), 4.0f);
        }
    } // namespace

    OscilloscopeView::OscilloscopeView(PatchworkEightProcessor& processor) : processor_(processor)
    {
        if (murmur8::visualizerGpuEnabled())
        {
            glPlot_ = std::make_unique<murmur8::MurmurVisualizerComponent>(processor_.getVisualizerBus());
            glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::Waveform);
            addAndMakeVisible(*glPlot_);
        }
        startTimerHz(30);
    }

    OscilloscopeView::~OscilloscopeView()
    {
        stopTimer();
    }

    void OscilloscopeView::setDesktopPlayModeLayout(bool desktopPlayMode)
    {
        desktopPlayMode_ = desktopPlayMode;
        if (desktopPlayMode)
        {
            compactLayout_ = false;
            ipadPlayLayout_ = false;
        }
        if (glPlot_ != nullptr)
        {
            glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::Waveform);
            glPlot_->setVisible(true);
        }
        resized();
        repaint();
    }

    void OscilloscopeView::setIpadPlayLayout(bool ipadPlayLayout)
    {
        ipadPlayLayout_ = ipadPlayLayout;
        if (ipadPlayLayout)
        {
            compactLayout_ = false;
            desktopPlayMode_ = false;
        }
        repaint();
    }

    void OscilloscopeView::setCompactLayout(bool compactLayout)
    {
        compactLayout_ = compactLayout;
        if (compactLayout)
            desktopPlayMode_ = false;
        repaint();
    }

    void OscilloscopeView::timerCallback()
    {
        const int pulled = processor_.readScopeSamples(capture_.data(), kCaptureSize);
        if (pulled > 8)
        {
            captureCount_ = pulled;
            hasData_ = true;

            float peak = 0.0f;
            for (int i = 0; i < pulled; ++i)
                peak = juce::jmax(peak, std::abs(capture_[static_cast<std::size_t>(i)]));
            lastPeakLinear_ = peak;

            leftVu_.processFrame(processor_.getScopeLeftPeakLinear(), processor_.getScopeLeftPeakLinear());
            rightVu_.processFrame(processor_.getScopeRightPeakLinear(), processor_.getScopeRightPeakLinear());

            if (ipadPlayLayout_ && scopeDisplayMode_ == ScopeDisplayMode::Spectrum)
            {
                std::array<float, murmur8::AudioVisualizerBus::fftBinCount> fftBins {};
                for (int i = 0; i < murmur8::AudioVisualizerBus::fftBinCount; ++i)
                {
                    const float t = static_cast<float>(i) / static_cast<float>(murmur8::AudioVisualizerBus::fftBinCount - 1);
                    const int sampleIdx = juce::jlimit(0, pulled - 1, static_cast<int>(t * static_cast<float>(pulled - 1)));
                    fftBins[static_cast<std::size_t>(i)] =
                        juce::jlimit(0.0f, 1.0f, std::abs(capture_[static_cast<std::size_t>(sampleIdx)]) * 2.5f);
                }
                processor_.getVisualizerBus().pushFFTFrame(fftBins.data(), murmur8::AudioVisualizerBus::fftBinCount);
            }

            repaint();
        }
    }

    void OscilloscopeView::paintWaveform(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        preview::paintWaveformFromBus(g, plot, processor_.getVisualizerBus());

        if (hasData_ && captureCount_ >= 8)
        {
            float busPeak = 0.0f;
            constexpr int kCols = murmur8::AudioVisualizerBus::waveformColumns;
            const auto& ring = processor_.getVisualizerBus().getWaveform();
            for (int i = 0; i < kCols; ++i)
            {
                busPeak = juce::jmax(busPeak, std::abs(ring.mins[static_cast<std::size_t>(i)].load(std::memory_order_relaxed)),
                                    std::abs(ring.maxs[static_cast<std::size_t>(i)].load(std::memory_order_relaxed)));
            }
            if (busPeak >= 0.0005f)
                return;
        }

        if (!hasData_ || captureCount_ < 8)
            return;

        juce::Path wave;
        const float midY = plot.getCentreY();
        const float halfH = plot.getHeight() * 0.42f;
        const float xStep = plot.getWidth() / static_cast<float>(captureCount_ - 1);

        for (int i = 0; i < captureCount_; ++i)
        {
            const float x = plot.getX() + static_cast<float>(i) * xStep;
            const float y = midY - juce::jlimit(-1.0f, 1.0f, capture_[static_cast<std::size_t>(i)]) * halfH;
            if (i == 0)
                wave.startNewSubPath(x, y);
            else
                wave.lineTo(x, y);
        }

        draw::strokeGlowPath(g, wave, 0.55f, 1.8f, true);
        g.setColour(palette::kAccent.withAlpha(0.9f));
        g.strokePath(wave, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void OscilloscopeView::positionGlPlot(juce::Rectangle<float> plot, murmur8::MurmurVisualizerComponent::Mode mode)
    {
        if (glPlot_ == nullptr || plot.isEmpty())
            return;

        glPlot_->setMode(mode);
        glPlot_->setBounds(plot.toNearestInt());
        glPlot_->setVisible(hasData_ || mode == murmur8::MurmurVisualizerComponent::Mode::Spectral);
        glPlot_->toFront(false);
    }

    void OscilloscopeView::paintCompactMode(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        auto inner = bounds.reduced(static_cast<float>(layout::kCompactScopePanelPadding));
        auto header = inner.removeFromTop(static_cast<float>(layout::kCompactScopeHeaderHeight));
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextSecondary);
        g.drawText("WAVEFORM", header, juce::Justification::centredLeft, true);

        const bool signalActive = hasData_ && lastPeakLinear_ > 0.001f;
        g.setColour(signalActive ? palette::kAccent : palette::kTextDim);
        g.fillEllipse(header.getRight() - 36.0f, header.getCentreY() - 2.5f, 5.0f, 5.0f);
        g.setColour(palette::kTextSecondary);
        g.drawText("IN", header.getRight() - 28.0f, header.getY(), 24.0f, header.getHeight(),
                   juce::Justification::centredLeft, true);

        inner.removeFromTop(static_cast<float>(layout::kCompactScopeHeaderGap));
        auto osc = inner.removeFromTop(static_cast<float>(layout::kCompactScopeOscHeight));
        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(osc, 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(osc.reduced(0.5f), 6.0f, 1.0f);

        auto plot = osc.reduced(10.0f, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.22f));
        for (int i = 1; i < 5; ++i)
        {
            const float y = plot.getY() + plot.getHeight() * static_cast<float>(i) / 5.0f;
            g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
        }
        for (int i = 1; i < 9; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * static_cast<float>(i) / 9.0f;
            g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
        }

        positionGlPlot(plot, murmur8::MurmurVisualizerComponent::Mode::Waveform);
        if (glPlot_ == nullptr || !murmur8::visualizerGpuEnabled())
            paintWaveform(g, plot);
    }

    void OscilloscopeView::paintDesktopPlayMode(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(juce::Colour(0xff050608));
        g.fillRoundedRectangle(bounds, static_cast<float>(layout::kDesktopPlayModeOscilloscopeCornerRadius));

        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds.reduced(0.75f), static_cast<float>(layout::kDesktopPlayModeOscilloscopeCornerRadius),
                               1.5f);

        const float metaY = bounds.getY() + static_cast<float>(layout::kDesktopPlayModeScopeMetadataInsetY);
        const float metaX = bounds.getX() + static_cast<float>(layout::kDesktopPlayModeScopeMetadataInsetX);
        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextSecondary);

        const auto metrics = readPerformanceMetrics(processor_);
        g.drawText("VOICES: " + formatVoiceCount(metrics.activeVoices, metrics.maxVoices),
                   metaX, metaY, 120.0f, 12.0f, juce::Justification::centredLeft, true);
        g.drawText("CPU LOAD: " + formatCpuPercent(metrics.cpuPercent), metaX + 87.0f, metaY, 80.0f, 12.0f,
                   juce::Justification::centredLeft, true);

        const float rightMetaX = bounds.getRight() - 195.0f;
        const bool signalActive = hasData_ && lastPeakLinear_ > 0.005f;
        g.setColour(signalActive ? palette::kAccent : palette::kTextDim.withAlpha(0.45f));
        g.fillEllipse(rightMetaX, metaY + 3.0f, 6.0f, 6.0f);
        g.setColour(palette::kTextSecondary);
        g.drawText(signalActive ? "SIGNAL ACTIVE" : "SIGNAL IDLE", rightMetaX + 12.0f, metaY, 88.0f, 12.0f,
                   juce::Justification::centredLeft, true);

        constexpr float kStereoMeterWidth = 32.0f;
        constexpr float kStereoMeterHeight = 6.0f;
        const float meterY = metaY + 1.5f;
        const float leftFill = kStereoMeterWidth * leftVu_.rmsNorm();
        const float rightFill = kStereoMeterWidth * rightVu_.rmsNorm();

        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(rightMetaX + 96.0f, meterY, kStereoMeterWidth, kStereoMeterHeight, 2.0f);
        if (leftFill > 0.5f)
        {
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(rightMetaX + 96.0f, meterY, leftFill, kStereoMeterHeight, 2.0f);
        }
        g.setFont(fonts::label(7.0f));
        g.drawText("L", rightMetaX + 131.0f, meterY - 1.0f, 8.0f, 9.0f, juce::Justification::centredLeft, true);

        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(rightMetaX + 139.0f, meterY, kStereoMeterWidth, kStereoMeterHeight, 2.0f);
        if (rightFill > 0.5f)
        {
            g.setColour(palette::kAccent.withAlpha(0.85f));
            g.fillRoundedRectangle(rightMetaX + 139.0f, meterY, rightFill, kStereoMeterHeight, 2.0f);
        }
        g.drawText("R", rightMetaX + 174.0f, meterY - 1.0f, 8.0f, 9.0f, juce::Justification::centredLeft, true);

        g.setColour(palette::kBorder.withAlpha(0.35f));
        const auto plot = desktopPlayWavePlotBounds(bounds);
        for (int i = 1; i < 4; ++i)
        {
            const float y = plot.getY() + plot.getHeight() * static_cast<float>(i) / 4.0f;
            g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
        }
        for (int i = 1; i < 8; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * static_cast<float>(i) / 8.0f;
            g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
        }

        positionGlPlot(plot, murmur8::MurmurVisualizerComponent::Mode::Waveform);
        if (glPlot_ == nullptr || !murmur8::visualizerGpuEnabled())
            paintWaveform(g, plot);
    }

    void OscilloscopeView::resized()
    {
        if (glPlot_ == nullptr)
            return;

        auto bounds = getLocalBounds().toFloat();

        if (desktopPlayMode_)
        {
            glPlot_->setBounds(desktopPlayWavePlotBounds(bounds).toNearestInt());
            return;
        }

        if (compactLayout_)
        {
            auto inner = bounds.reduced(static_cast<float>(layout::kCompactScopePanelPadding));
            inner.removeFromTop(static_cast<float>(layout::kCompactScopeHeaderHeight));
            inner.removeFromTop(static_cast<float>(layout::kCompactScopeHeaderGap));
            auto osc = inner.removeFromTop(static_cast<float>(layout::kCompactScopeOscHeight));
            glPlot_->setBounds(osc.reduced(10.0f, 8.0f).toNearestInt());
            return;
        }

        if (ipadPlayLayout_)
        {
            auto plot = bounds.reduced(14.0f, 10.0f);
            plot.removeFromTop(34.0f);
            glPlot_->setBounds(plot.toNearestInt());
        }
    }

    void OscilloscopeView::paintScopeModePills(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        static constexpr const char* kLabels[] = {"LIVE", "SPECTRUM", "VECTOR"};
        int x = static_cast<int>(bounds.getX());
        const int y = static_cast<int>(bounds.getY());
        const int pillH = 18;
        const int gap = 6;

        for (int i = 0; i < 3; ++i)
        {
            g.setFont(fonts::label(7.0f));
            const int pillW = 52;
            const auto pill = juce::Rectangle<int>(x, y, pillW, pillH);
            const bool active = static_cast<int>(scopeDisplayMode_) == i;
            g.setColour(active ? palette::kAccent.withAlpha(0.18f) : juce::Colour(0xff0a0b0e));
            g.fillRoundedRectangle(pill.toFloat(), 4.0f);
            g.setColour(active ? palette::kAccent : palette::kBorder.withAlpha(0.75f));
            g.drawRoundedRectangle(pill.toFloat().reduced(0.5f), 4.0f, 1.0f);
            g.setColour(active ? palette::kAccent : palette::kTextDim);
            g.drawText(kLabels[i], pill, juce::Justification::centred, true);
            x += pillW + gap;
        }
    }

    int OscilloscopeView::scopeModePillAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < 3; ++i)
        {
            if (scopeModePillBounds_[static_cast<std::size_t>(i)].contains(pos))
                return i;
        }
        return -1;
    }

    void OscilloscopeView::mouseDown(const juce::MouseEvent& event)
    {
        if (!ipadPlayLayout_)
            return;

        const int pill = scopeModePillAt(event.getPosition());
        if (pill < 0)
            return;

        scopeDisplayMode_ = static_cast<ScopeDisplayMode>(pill);
        if (glPlot_ != nullptr)
        {
            if (scopeDisplayMode_ == ScopeDisplayMode::Live)
                glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::Waveform);
            else if (scopeDisplayMode_ == ScopeDisplayMode::Spectrum)
                glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::Spectral);
        }
        repaint();
    }

    void OscilloscopeView::paintIpadPlayMode(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(juce::Colour(0xff050608));
        g.fillRoundedRectangle(bounds, static_cast<float>(layout::kDesktopPlayModeOscilloscopeCornerRadius));
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds.reduced(0.75f), static_cast<float>(layout::kDesktopPlayModeOscilloscopeCornerRadius),
                               1.5f);

        const float metaX = bounds.getX() + 14.0f;
        const float metaY = bounds.getY() + 12.0f;
        g.setFont(fonts::label(layout::kIpadPlayLabelSize));
        g.setColour(palette::kTextSecondary);
        g.drawText("OSCILLOSCOPE", metaX, metaY, 100.0f, 12.0f, juce::Justification::centredLeft, true);

        const auto metrics = readPerformanceMetrics(processor_);
        g.drawText("VOICES " + formatVoiceCount(metrics.activeVoices, metrics.maxVoices), metaX + 110.0f, metaY, 80.0f,
                   12.0f, juce::Justification::centredLeft, true);

        const float bpm = processor_.getHostBpm();
        g.setColour(palette::kFigmaTeal);
        g.drawText(juce::String(bpm, 1) + " BPM", bounds.getRight() - 92.0f, metaY, 72.0f, 12.0f,
                   juce::Justification::centredRight, true);
        g.setColour(palette::kTextSecondary);

        auto pillRow = juce::Rectangle<float>(bounds.getRight() - 190.0f, metaY - 2.0f, 176.0f, 18.0f);
        paintScopeModePills(g, pillRow);

        int pillX = static_cast<int>(pillRow.getX());
        for (int i = 0; i < 3; ++i)
        {
            scopeModePillBounds_[static_cast<std::size_t>(i)] = {pillX, static_cast<int>(pillRow.getY()), 52, 18};
            pillX += 58;
        }

        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(bounds.getY() + 38.0f), bounds.getX() + 8.0f, bounds.getRight() - 8.0f);

        auto plot = bounds.reduced(14.0f, 10.0f);
        plot.removeFromTop(34.0f);

        if (scopeDisplayMode_ == ScopeDisplayMode::Live)
        {
            positionGlPlot(plot, murmur8::MurmurVisualizerComponent::Mode::Waveform);
            if (glPlot_ == nullptr || !murmur8::visualizerGpuEnabled())
                paintWaveform(g, plot);
            return;
        }

        g.setColour(palette::kBorder.withAlpha(0.25f));
        for (int i = 1; i < 8; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * static_cast<float>(i) / 8.0f;
            g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
        }

        if (scopeDisplayMode_ == ScopeDisplayMode::Spectrum)
        {
            positionGlPlot(plot, murmur8::MurmurVisualizerComponent::Mode::Spectral);
            if (glPlot_ == nullptr || !murmur8::visualizerGpuEnabled())
                preview::paintSpectrumFromBus(g, plot, processor_.getVisualizerBus());
            return;
        }

        const auto centre = plot.getCentre();
        const float radius = juce::jmin(plot.getWidth(), plot.getHeight()) * 0.38f;
        g.setColour(palette::kBorder.withAlpha(0.35f));
        for (int ring = 1; ring <= 3; ++ring)
            g.drawEllipse(centre.x - radius * static_cast<float>(ring) / 3.0f,
                          centre.y - radius * static_cast<float>(ring) / 3.0f,
                          radius * 2.0f * static_cast<float>(ring) / 3.0f,
                          radius * 2.0f * static_cast<float>(ring) / 3.0f, 1.0f);

        juce::Path radar;
        const float angle = juce::Time::getMillisecondCounterHiRes() * 0.001f;
        for (int i = 0; i < 64; ++i)
        {
            const float a = angle + static_cast<float>(i) / 64.0f * juce::MathConstants<float>::twoPi;
            const float r = radius * (0.35f + 0.55f * std::abs(std::sin(a * 3.0f + lastPeakLinear_ * 5.0f)));
            const float x = centre.x + std::cos(a) * r;
            const float y = centre.y + std::sin(a) * r;
            if (i == 0)
                radar.startNewSubPath(x, y);
            else
                radar.lineTo(x, y);
        }
        radar.closeSubPath();
        g.setColour(palette::kAccent.withAlpha(0.25f));
        g.fillPath(radar);
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.strokePath(radar, juce::PathStrokeType(1.2f));
    }

    void OscilloscopeView::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();

        if (compactLayout_)
        {
            paintCompactMode(g, bounds);
            return;
        }

        if (ipadPlayLayout_)
        {
            paintIpadPlayMode(g, bounds);
            return;
        }

        if (desktopPlayMode_)
        {
            paintDesktopPlayMode(g, bounds);
            return;
        }

        bounds = bounds.reduced(2.0f);
        draw::fillRecessedRoundedRect(g, bounds, 6.0f);

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(fonts::kCaptionSize));
        g.drawText("SCOPE", bounds.removeFromTop(14.0f).reduced(6.0f, 0.0f), juce::Justification::centredLeft);

        auto plot = bounds.reduced(6.0f, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(plot.getCentreY()), plot.getX(), plot.getRight());

        if (!hasData_ || captureCount_ < 8)
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(fonts::kCaptionSize));
            g.drawText("Play notes to see waveform", plot, juce::Justification::centred);
            if (glPlot_ != nullptr)
                glPlot_->setVisible(false);
            return;
        }

        positionGlPlot(plot, murmur8::MurmurVisualizerComponent::Mode::Waveform);
        if (glPlot_ == nullptr || !murmur8::visualizerGpuEnabled())
            paintWaveform(g, plot);
    }

} // namespace pw8::plugin::ui
