#include "LfoWireframeView.h"

#include "../../visualizer/PreviewSurface.h"
#include "../../visualizer/VisualizerGpu.h"
#include "../../visualizer/PreviewDraw.h"
#include "../../visualizer/VisualPreviewCache.h"
#include "state/PluginState.h"

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

        [[nodiscard]] const char* lfoWaveformName(lfo::LfoWaveform wf) noexcept
        {
            switch (wf)
            {
                case lfo::LfoWaveform::Sine: return "SINE";
                case lfo::LfoWaveform::Triangle: return "TRIANGLE";
                case lfo::LfoWaveform::Saw: return "SAW";
                case lfo::LfoWaveform::Square: return "SQUARE";
                case lfo::LfoWaveform::SampleHold: return "S & H";
                case lfo::LfoWaveform::SmoothRandom: return "S.RANDOM";
            }
            return "?";
        }

        [[nodiscard]] const char* lfoModeName(lfo::LfoMode mode) noexcept
        {
            switch (mode)
            {
                case lfo::LfoMode::Free: return "FREE";
                case lfo::LfoMode::Retrigger: return "RETRIGGER";
                case lfo::LfoMode::OneShot: return "ONE SHOT";
                case lfo::LfoMode::TempoSync: return "TEMPO SYNC";
            }
            return "?";
        }

        [[nodiscard]] float lfoWaveformToShaderIndex(lfo::LfoWaveform wf) noexcept
        {
            switch (wf)
            {
                case lfo::LfoWaveform::Sine: return 0.0f;
                case lfo::LfoWaveform::Triangle: return 1.0f;
                case lfo::LfoWaveform::Saw: return 2.0f;
                case lfo::LfoWaveform::Square: return 3.0f;
                case lfo::LfoWaveform::SampleHold: return 4.0f;
                case lfo::LfoWaveform::SmoothRandom: return 4.5f;
            }
            return 0.0f;
        }
    } // namespace

    LfoWireframeView::LfoWireframeView(juce::AudioProcessorValueTreeState& apvts, std::size_t lfoIndex)
        : apvts_(apvts), lfoIndex_(lfoIndex)
    {
        startTimerHz(12);
    }

    LfoWireframeView::~LfoWireframeView() { stopTimer(); }

    void LfoWireframeView::attachVisualizerBus(murmur8::AudioVisualizerBus& bus)
    {
        visualizerBus_ = &bus;
        if (!murmur8::visualizerGpuEnabled())
            return;

        glPlot_ = std::make_unique<murmur8::MurmurVisualizerComponent>(bus);
        glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::Lfo);
        addAndMakeVisible(*glPlot_);
        syncGlPreview();
        resized();
    }

    void LfoWireframeView::setLfoIndex(std::size_t lfoIndex)
    {
        lfoIndex_ = lfoIndex;
        repaint();
    }

    void LfoWireframeView::syncGlPreview()
    {
        if (glPlot_ == nullptr)
            return;

        murmur8::LfoPreviewParams params;
        params.waveform = lfoWaveformToShaderIndex(waveform_);
        params.rateHz = rateHz_;
        params.phase = phaseOffset_ * juce::MathConstants<float>::twoPi;
        glPlot_->setLfoPreviewParams(params);
    }

    void LfoWireframeView::timerCallback()
    {
        const auto prefix = lfoParamId(lfoIndex_, "");
        waveform_ = static_cast<lfo::LfoWaveform>(static_cast<int>(loadParam(apvts_, prefix + "Waveform") + 0.5f));
        mode_ = static_cast<lfo::LfoMode>(static_cast<int>(loadParam(apvts_, prefix + "Mode") + 0.5f));
        rateHz_ = loadParam(apvts_, prefix + "RateHz", 2.0f);
        phaseOffset_ = loadParam(apvts_, prefix + "PhaseOffset");

        setCaption(juce::String("LFO ") + juce::String(static_cast<int>(lfoIndex_ + 1)) + " · "
                   + lfoWaveformName(waveform_));
        setSubCaption(juce::String(lfoModeName(mode_)) + " · " + juce::String(rateHz_, 2) + " Hz");
        syncGlPreview();
        repaint();
    }

    void LfoWireframeView::resized()
    {
        WireframeCanvas::resized();
        if (glPlot_ != nullptr && !plotBounds_.isEmpty())
            glPlot_->setBounds(plotBounds_);
    }

    void LfoWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        plotBounds_ = bounds.toNearestInt();

        if (glPlot_ != nullptr && murmur8::visualizerGpuEnabled())
        {
            glPlot_->setBounds(plotBounds_);
            glPlot_->setVisible(true);

            if (mode_ == lfo::LfoMode::OneShot)
            {
                const float markerX = bounds.getX() + bounds.getWidth() * 0.5f;
                g.setColour(palette::kAccentWarm.withAlpha(0.75f));
                g.drawVerticalLine(static_cast<int>(markerX), bounds.getY(), bounds.getBottom());
            }
            return;
        }

        const int cycles = (mode_ == lfo::LfoMode::OneShot) ? 1 : 2;
        const auto key = preview::lfoPreviewKey(static_cast<int>(waveform_), cycles, phaseOffset_);
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
                                                  const float phase =
                                                      std::fmod(t * static_cast<float>(cycles) + phaseOffset_, 1.0f);
                                                  switch (waveform_)
                                                  {
                                                      case lfo::LfoWaveform::SampleHold:
                                                          out.yNorm[static_cast<std::size_t>(i)] =
                                                              sampleLfoSampleHold(phase);
                                                          break;
                                                      case lfo::LfoWaveform::SmoothRandom:
                                                          out.yNorm[static_cast<std::size_t>(i)] =
                                                              sampleLfoSmoothRandom(phase,
                                                                                    static_cast<int>(phaseOffset_ * 8.0f));
                                                          break;
                                                      default:
                                                          out.yNorm[static_cast<std::size_t>(i)] =
                                                              sampleLfoWaveform(waveform_, phase);
                                                          break;
                                                  }
                                              }
                                          });
                                      preview::paintPolylineCurve(dg, plot, polyline);
                                  });

        previewSurface_.paintOverlay(g, [&](juce::Graphics& og, juce::Rectangle<float> plot)
                                     {
                                         if (mode_ == lfo::LfoMode::OneShot)
                                         {
                                             const float markerX = plot.getX() + plot.getWidth() * 0.5f;
                                             og.setColour(palette::kAccentWarm.withAlpha(0.75f));
                                             og.drawVerticalLine(static_cast<int>(markerX), plot.getY(), plot.getBottom());
                                         }
                                     });
    }

} // namespace pw8::plugin::ui::wireframe
