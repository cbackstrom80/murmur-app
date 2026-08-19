#include "FxWireframeView.h"

#include "../../visualizer/FxCpuPreview.h"
#include "../../visualizer/PreviewDraw.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] const char* effectTypeName(int typeOrdinal) noexcept
        {
            switch (typeOrdinal)
            {
                case 0: return "BYPASS";
                case 1: return "SATURATION";
                case 2: return "CHORUS";
                case 3: return "TAPE DELAY";
                case 4: return "NODE DELAY";
                case 5: return "FREQ SHIFT ECHO";
                case 6: return "FRACTAL ECHO";
                case 7: return "REVERB";
                case 8: return "EQ";
                case 9: return "COMPRESSOR";
                case 10: return "LIMITER";
                case 11: return "VOCODER";
                case 12: return "CLOUDS";
                case 13: return "QUASAR";
                default: return "?";
            }
        }

        [[nodiscard]] float readParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix,
                                      const char* suffix, float fallback)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + suffix))
                return raw->load();
            return fallback;
        }
    } // namespace

    FxWireframeView::FxWireframeView(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        startTimerHz(15);
    }

    FxWireframeView::~FxWireframeView() { stopTimer(); }

    void FxWireframeView::attachVisualizerBus(murmur8::AudioVisualizerBus& bus)
    {
        visualizerBus_ = &bus;
        if (!murmur8::visualizerGpuEnabled())
            return;

        glPlot_ = std::make_unique<murmur8::MurmurVisualizerComponent>(bus);
        glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::FxPreview);
        addAndMakeVisible(*glPlot_);
        syncGlPreview();
        resized();
    }

    void FxWireframeView::bindToSlot(const juce::String& paramPrefix)
    {
        paramPrefix_ = paramPrefix;
        timerCallback();
    }

    int FxWireframeView::fxKindForEffectType() const
    {
        switch (effectType_)
        {
            case 1: return 1;
            case 2: return 2;
            case 3:
            case 4: return 3;
            case 5: return 5;
            case 6: return 6;
            case 7: return 7;
            case 8: return 4;
            case 9: return 9;
            case 10: return 10;
            case 11: return 11;
            case 12: return 12;
            default: return 0;
        }
    }

    void FxWireframeView::syncGlPreview()
    {
        if (glPlot_ == nullptr || paramPrefix_.isEmpty())
            return;

        if (effectType_ == 13)
        {
            glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::QuasarBinaural);
            murmur8::QuasarFieldParams q;
            q.qsr1AngleDeg = readParam(apvts_, paramPrefix_, "Qsr1AngleDeg", 30.0f);
            q.qsr1Distance = readParam(apvts_, paramPrefix_, "Qsr1Distance", 0.35f);
            q.qsr1Height = readParam(apvts_, paramPrefix_, "Qsr1Height", 0.0f);
            q.qsr2AngleDeg = readParam(apvts_, paramPrefix_, "Qsr2AngleDeg", 330.0f);
            q.qsr2Distance = readParam(apvts_, paramPrefix_, "Qsr2Distance", 0.35f);
            q.qsr2Height = readParam(apvts_, paramPrefix_, "Qsr2Height", 0.0f);
            q.crossfeed = readParam(apvts_, paramPrefix_, "QuasarCrossfeed", 0.0f);
            q.width = readParam(apvts_, paramPrefix_, "CntrLevel", 0.5f);
            q.mix = mix_;
            glPlot_->setQuasarFieldParams(q);
            return;
        }

        glPlot_->setMode(murmur8::MurmurVisualizerComponent::Mode::FxPreview);
        murmur8::FxPreviewParams params;
        params.fxKind = fxKindForEffectType();
        params.mix = mix_;
        params.paramA0 = readParam(apvts_, paramPrefix_, "SaturationDrive", 6.0f);
        params.paramA1 = readParam(apvts_, paramPrefix_, "ChorusRate", 0.5f);
        params.paramA2 = readParam(apvts_, paramPrefix_, "ChorusDepth", 4.0f);
        params.paramA3 = readParam(apvts_, paramPrefix_, "TapeDriftRate", 0.3f);
        params.paramB0 = readParam(apvts_, paramPrefix_, "ReverbDecaySeconds", 2.0f);
        params.paramB1 = readParam(apvts_, paramPrefix_, "ReverbSize", 1.0f);
        params.paramB2 = readParam(apvts_, paramPrefix_, "ReverbHighRatio", 0.6f);
        params.paramB3 = readParam(apvts_, paramPrefix_, "TapeDriftDepth", 1.5f);
        params.paramC0 = readParam(apvts_, paramPrefix_, "CompThresholdDb", -18.0f);
        params.paramC1 = readParam(apvts_, paramPrefix_, "CompRatio", 4.0f);
        glPlot_->setFxPreviewParams(params);
    }

    void FxWireframeView::timerCallback()
    {
        if (paramPrefix_.isEmpty())
            return;

        if (auto* typeRaw = apvts_.getRawParameterValue(paramPrefix_ + "Type"))
            effectType_ = static_cast<int>(typeRaw->load() + 0.5f);
        if (auto* mixRaw = apvts_.getRawParameterValue(paramPrefix_ + "Mix"))
            mix_ = mixRaw->load();

        setCaption(juce::String(effectTypeName(effectType_)));
        setSubCaption("Mix " + juce::String(mix_, 2));
        syncGlPreview();
        repaint();
    }

    void FxWireframeView::resized()
    {
        WireframeCanvas::resized();
        if (glPlot_ != nullptr && !plotBounds_.isEmpty())
            glPlot_->setBounds(plotBounds_);
    }

    void FxWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        plotBounds_ = bounds.toNearestInt();

        if (glPlot_ != nullptr && murmur8::visualizerGpuEnabled())
        {
            glPlot_->setBounds(plotBounds_);
            glPlot_->setVisible(true);
            return;
        }

        g.setColour(palette::kBackgroundBottom.withAlpha(0.45f));
        g.fillRoundedRectangle(bounds, 4.0f);
        preview::paintFxWireframePreview(g, bounds, apvts_, paramPrefix_, effectType_, mix_);
    }

} // namespace pw8::plugin::ui::wireframe
