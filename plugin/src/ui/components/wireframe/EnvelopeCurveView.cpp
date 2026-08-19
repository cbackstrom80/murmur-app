#include "EnvelopeCurveView.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "../../visualizer/VisualizerGpu.h"
#include "../../visualizer/PreviewDraw.h"
#include "../../visualizer/PreviewSurface.h"
#include "../../visualizer/VisualPreviewCache.h"
#include "EnvelopePathBuilder.h"
#include "WireframeProjection.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] float loadEnvParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                          float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        [[nodiscard]] murmur8::EnvelopeCurveParams curveParamsFromEnvelope(const EnvelopePreviewParams& params)
        {
            constexpr float kPlateau = 0.35f;
            const float attack = juce::jmax(params.attackSeconds, 0.05f);
            const float decay = juce::jmax(params.decaySeconds, 0.05f);
            const float release = juce::jmax(params.releaseSeconds, 0.05f);
            const float total = attack + decay + kPlateau + release;

            murmur8::EnvelopeCurveParams out;
            out.attackNorm = attack / total;
            out.decayNorm = decay / total;
            out.sustain = params.sustainLevel;
            out.releaseNorm = release / total;
            return out;
        }
    } // namespace

    EnvelopeCurveView::EnvelopeCurveView(juce::AudioProcessorValueTreeState& apvts, std::size_t envIndex)
        : apvts_(apvts), envIndex_(envIndex)
    {
        setCaption("AMP ENVELOPE · DAHDSR");
        setSubCaption("Release tail shown after sustain plateau");
        startTimerHz(8);
        refreshParams();
    }

    EnvelopeCurveView::~EnvelopeCurveView() { stopTimer(); }

    void EnvelopeCurveView::attachVisualizerBus(murmur8::AudioVisualizerBus& bus)
    {
        visualizerBus_ = &bus;
        if (!murmur8::visualizerGpuEnabled())
            return;

        glPlot_ = std::make_unique<murmur8::MurmurVisualizerComponent>(bus);
        glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::EnvelopeCurve);
        addAndMakeVisible(*glPlot_);
        syncGlPreview();
        resized();
    }

    void EnvelopeCurveView::timerCallback()
    {
        refreshParams();
        syncGlPreview();
        repaint();
    }

    void EnvelopeCurveView::refreshParams()
    {
        const auto prefix = envelopeParamId(envIndex_, "");
        params_.delaySeconds = loadEnvParam(apvts_, prefix + "Delay");
        params_.attackSeconds = loadEnvParam(apvts_, prefix + "Attack", 0.005f);
        params_.holdSeconds = loadEnvParam(apvts_, prefix + "Hold");
        params_.decaySeconds = loadEnvParam(apvts_, prefix + "Decay", 0.2f);
        params_.sustainLevel = loadEnvParam(apvts_, prefix + "Sustain", 0.7f);
        params_.releaseSeconds = loadEnvParam(apvts_, prefix + "Release", 0.3f);
        params_.curveShape = loadEnvParam(apvts_, prefix + "Curve", 2.0f);
    }

    void EnvelopeCurveView::syncGlPreview()
    {
        if (glPlot_ == nullptr)
            return;

        glPlot_->setEnvelopeCurveParams(curveParamsFromEnvelope(params_));
    }

    void EnvelopeCurveView::resized()
    {
        WireframeCanvas::resized();
        if (glPlot_ != nullptr && !plotBounds_.isEmpty())
            glPlot_->setBounds(plotBounds_);
    }

    void EnvelopeCurveView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        plotBounds_ = bounds.toNearestInt();

        if (glPlot_ != nullptr && murmur8::visualizerGpuEnabled())
        {
            glPlot_->setBounds(plotBounds_);
            glPlot_->setVisible(true);

            juce::Path outline;
            juce::Path fillPath;
            float sustainEndX = bounds.getX();
            buildEnvelopePath(params_, bounds, outline, fillPath, sustainEndX);

            g.setColour(palette::kBorderBright.withAlpha(0.55f));
            g.drawHorizontalLine(static_cast<int>(bounds.getBottom() - bounds.getHeight() * 0.06f),
                                 bounds.getX(), bounds.getRight());

            const char* labels[] = {"D", "A", "H", "D", "S", "R"};
            const float stageStarts[] = {0.0f, 0.06f, 0.18f, 0.24f, 0.36f, 0.54f};
            const float markerY = bounds.getBottom() + 2.0f;
            g.setFont(fonts::label(8.5f));
            g.setColour(palette::kTextDim);
            for (int i = 0; i < 6; ++i)
            {
                const float x = bounds.getX() + stageStarts[i] * bounds.getWidth();
                g.drawText(labels[i], juce::Rectangle<float>(x - 8.0f, markerY, 16.0f, 12.0f),
                           juce::Justification::centred);
            }

            g.setColour(palette::kAccentWarm.withAlpha(0.85f));
            g.drawVerticalLine(static_cast<int>(sustainEndX), bounds.getY(), bounds.getBottom());
            g.setFont(fonts::label(8.0f));
            g.drawText("note off", juce::Rectangle<float>(sustainEndX + 3.0f, bounds.getY(), 48.0f, 12.0f),
                       juce::Justification::centredLeft);
            return;
        }

        const auto key = preview::envelopePreviewKey(params_.delaySeconds, params_.attackSeconds, params_.holdSeconds,
                                                      params_.decaySeconds, params_.sustainLevel, params_.releaseSeconds,
                                                      params_.curveShape);
        const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
            key, preview::kDefaultPolylinePoints,
            [&](int n, preview::PreviewPolyline& out) { preview::buildEnvelopePolyline(params_, n, out); });
        const float sustainEndNorm = polyline.sustainEndNorm;

        previewSurface_.setPlotBounds(bounds);
        previewSurface_.setDataKey(key);

        previewSurface_.paintBackground(g, [&](juce::Graphics& bg, juce::Rectangle<float> plot)
                                        {
                                            bg.setColour(palette::kBorderBright.withAlpha(0.55f));
                                            bg.drawHorizontalLine(static_cast<int>(plot.getBottom() - plot.getHeight() * 0.06f),
                                                                  plot.getX(), plot.getRight());
                                        });

        previewSurface_.paintData(g, [&](juce::Graphics& dg, juce::Rectangle<float> plot)
                                  {
                                      preview::PolylineDrawOptions opts;
                                      opts.yScale = 0.88f;
                                      preview::paintPolylineCurve(dg, plot, polyline, opts);
                                  });

        previewSurface_.paintOverlay(g, [sustainEndNorm](juce::Graphics& og, juce::Rectangle<float> plot)
                                    {
                                        const float markerY = plot.getBottom() + 2.0f;
                                        og.setFont(fonts::label(8.5f));
                                        og.setColour(palette::kTextDim);

                                        const char* labels[] = {"D", "A", "H", "D", "S", "R"};
                                        const float stageStarts[] = {0.0f, 0.06f, 0.18f, 0.24f, 0.36f, 0.54f};
                                        for (int i = 0; i < 6; ++i)
                                        {
                                            const float x = plot.getX() + stageStarts[i] * plot.getWidth();
                                            og.drawText(labels[i], juce::Rectangle<float>(x - 8.0f, markerY, 16.0f, 12.0f),
                                                        juce::Justification::centred);
                                        }

                                        const float sustainX = plot.getX() + sustainEndNorm * plot.getWidth();
                                        og.setColour(palette::kAccentWarm.withAlpha(0.85f));
                                        og.drawVerticalLine(static_cast<int>(sustainX), plot.getY(), plot.getBottom());
                                        og.setFont(fonts::label(8.0f));
                                        og.drawText("note off",
                                                    juce::Rectangle<float>(sustainX + 3.0f, plot.getY(), 48.0f, 12.0f),
                                                    juce::Justification::centredLeft);
                                    });
    }

} // namespace pw8::plugin::ui::wireframe
