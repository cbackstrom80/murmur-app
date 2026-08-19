#include "FilterWireframeView.h"

#include <cmath>

#include "../../theme/ObsidianFonts.h"
#include "../../visualizer/VisualizerGpu.h"
#include "../../visualizer/PreviewDraw.h"
#include "../../visualizer/PreviewSurface.h"
#include "../../visualizer/VisualPreviewCache.h"
#include "WireframeProjection.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                       float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        [[nodiscard]] float freqToNorm(float hz) noexcept
        {
            return (std::log10(juce::jlimit(20.0f, 20000.0f, hz)) - std::log10(20.0f))
                   / (std::log10(20000.0f) - std::log10(20.0f));
        }

        [[nodiscard]] float responseAt(float freqNorm, int mode, float cutoffNorm, float resonance) noexcept
        {
            const float q = 0.5f + resonance * 4.5f;
            const float d = freqNorm - cutoffNorm;
            const float bell = std::exp(-d * d * q * 18.0f) * resonance * 1.8f;

            switch (mode)
            {
                case 0:
                    return juce::jlimit(-1.0f, 1.0f, 1.0f - juce::jmax(0.0f, freqNorm - cutoffNorm) * (2.5f - resonance)
                                                     + bell);
                case 1:
                    return juce::jlimit(-1.0f, 1.0f, 1.0f - juce::jmax(0.0f, cutoffNorm - freqNorm) * (2.5f - resonance)
                                                     + bell);
                case 2:
                    return juce::jlimit(-1.0f, 1.0f, bell * 2.2f - std::abs(d) * 3.5f);
                case 3:
                    return juce::jlimit(-1.0f, 1.0f, 1.0f - bell * 2.5f - std::abs(d) * 0.5f);
                case 4:
                    return juce::jlimit(-1.0f, 1.0f, bell * 2.0f);
                default:
                    return 0.0f;
            }
        }

        [[nodiscard]] const char* filterModeName(int mode) noexcept
        {
            switch (mode)
            {
                case 0: return "LOWPASS";
                case 1: return "HIGHPASS";
                case 2: return "BANDPASS";
                case 3: return "NOTCH";
                case 4: return "PEAK";
                default: return "?";
            }
        }
    } // namespace

    FilterWireframeView::FilterWireframeView(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        startTimerHz(8);
    }

    FilterWireframeView::~FilterWireframeView() { stopTimer(); }

    void FilterWireframeView::attachVisualizerBus(murmur8::AudioVisualizerBus& bus)
    {
        visualizerBus_ = &bus;
        if (!murmur8::visualizerGpuEnabled())
            return;

        glPlot_ = std::make_unique<murmur8::MurmurVisualizerComponent>(bus);
        glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::Filter);
        addAndMakeVisible(*glPlot_);
        syncGlPreview();
        resized();
    }

    void FilterWireframeView::setParamIds(const juce::String& enabledId, const juce::String& modeId,
                                           const juce::String& cutoffId, const juce::String& resonanceId)
    {
        enabledId_ = enabledId;
        modeId_ = modeId;
        cutoffId_ = cutoffId;
        resonanceId_ = resonanceId;
        timerCallback();
    }

    void FilterWireframeView::syncGlPreview()
    {
        if (glPlot_ == nullptr)
            return;

        murmur8::FilterPreviewParams params;
        params.mode = mode_;
        params.cutoffNorm = freqToNorm(cutoffHz_);
        params.resonance = resonance_;
        glPlot_->setFilterPreviewParams(params);
        glPlot_->setVisible(enabled_);
    }

    void FilterWireframeView::timerCallback()
    {
        if (enabledId_.isEmpty())
            return;

        enabled_ = loadParam(apvts_, enabledId_) >= 0.5f;
        mode_ = static_cast<int>(loadParam(apvts_, modeId_) + 0.5f);
        cutoffHz_ = loadParam(apvts_, cutoffId_, 8000.0f);
        resonance_ = loadParam(apvts_, resonanceId_, 0.2f);

        setCaption(juce::String(filterModeName(mode_)) + (enabled_ ? "" : " · OFF"));
        setSubCaption(juce::String(static_cast<int>(cutoffHz_)) + " Hz · Q "
                      + juce::String(0.5f + resonance_ * 4.5f, 1));
        syncGlPreview();
        repaint();
    }

    void FilterWireframeView::resized()
    {
        WireframeCanvas::resized();
        if (glPlot_ != nullptr && !plotBounds_.isEmpty())
            glPlot_->setBounds(plotBounds_);
    }

    void FilterWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        plotBounds_ = bounds.toNearestInt();

        if (!enabled_)
        {
            if (glPlot_ != nullptr)
                glPlot_->setVisible(false);
            paintEmptyMessage(g, bounds, "Filter disabled");
            return;
        }

        if (glPlot_ != nullptr && murmur8::visualizerGpuEnabled())
        {
            glPlot_->setBounds(plotBounds_);
            glPlot_->setVisible(true);

            const float cutoffNorm = freqToNorm(cutoffHz_);
            const float markerX = bounds.getX() + cutoffNorm * bounds.getWidth();
            g.setColour(palette::kAccentWarm.withAlpha(0.85f));
            g.drawVerticalLine(static_cast<int>(markerX), bounds.getY(), bounds.getBottom());
            g.setFont(fonts::label(8.0f));
            g.drawText("cutoff", juce::Rectangle<float>(markerX + 3.0f, bounds.getY(), 40.0f, 12.0f),
                       juce::Justification::centredLeft);
            return;
        }

        const float cutoffNorm = freqToNorm(cutoffHz_);
        const auto key = preview::filterPreviewKey(mode_, cutoffNorm, resonance_);
        previewSurface_.setPlotBounds(bounds);
        previewSurface_.setDataKey(key);

        previewSurface_.paintBackground(g, [&](juce::Graphics& bg, juce::Rectangle<float> plot)
                                          {
                                              bg.setColour(palette::kBorder.withAlpha(0.22f));
                                              bg.drawHorizontalLine(static_cast<int>(plot.getCentreY()), plot.getX(),
                                                                    plot.getRight());
                                          });

        previewSurface_.paintData(g, [&](juce::Graphics& dg, juce::Rectangle<float> plot)
                                  {
                                      const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
                                          key, preview::kDefaultPolylinePoints,
                                          [&](int n, preview::PreviewPolyline& out)
                                          {
                                              out.xNorm.clear();
                                              out.yNorm.resize(static_cast<std::size_t>(n));
                                              for (int i = 0; i < n; ++i)
                                              {
                                                  const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                                                  out.yNorm[static_cast<std::size_t>(i)] =
                                                      responseAt(t, mode_, cutoffNorm, resonance_);
                                              }
                                          });
                                      preview::paintPolylineCurve(dg, plot, polyline);
                                  });

        previewSurface_.paintOverlay(g, [&](juce::Graphics& og, juce::Rectangle<float> plot)
                                     {
                                         const float markerX = plot.getX() + cutoffNorm * plot.getWidth();
                                         og.setColour(palette::kAccentWarm.withAlpha(0.85f));
                                         og.drawVerticalLine(static_cast<int>(markerX), plot.getY(), plot.getBottom());
                                         og.setFont(fonts::label(8.0f));
                                         og.drawText("cutoff",
                                                     juce::Rectangle<float>(markerX + 3.0f, plot.getY(), 40.0f, 12.0f),
                                                     juce::Justification::centredLeft);
                                     });
    }

} // namespace pw8::plugin::ui::wireframe
