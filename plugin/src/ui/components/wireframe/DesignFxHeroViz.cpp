#include "DesignFxHeroViz.h"

#include "../../PlayModeLayout.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "../FxEffectPlayParams.h"
#include "../DesignFxUiState.h"
#include "WireframeProjection.h"
#include "processor/MurmurProcessor.h"
#include "../../visualizer/FxAnimationAtlas.h"
#include "../../visualizer/PreviewDraw.h"
#include "../../visualizer/PreviewSurface.h"
#include "../../visualizer/VisualPreviewCache.h"

#include <juce_dsp/juce_dsp.h>

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        void paintHeroMicroLabel(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                                 juce::Justification just = juce::Justification::centredLeft)
        {
            g.setColour(palette::kFigmaFxMutedText);
            g.setFont(fonts::micro(7.0f));
            g.drawText(text, area, just);
        }

        void paintHeroValueReadout(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text,
                                   juce::Colour colour = palette::kAccent)
        {
            g.setColour(colour);
            g.setFont(fonts::denseBold(8.0f));
            g.drawText(text, area, juce::Justification::centredRight);
        }

        void paintHeroModeBadge(juce::Graphics& g, juce::Rectangle<float> plot, const juce::String& mode)
        {
            if (mode.isEmpty())
                return;
            auto badge = juce::Rectangle<float>(plot.getX() + 6.0f, plot.getY() + 4.0f, 72.0f, 14.0f);
            g.setColour(palette::kAccent.withAlpha(0.16f));
            g.fillRoundedRectangle(badge, 3.0f);
            g.setColour(palette::kAccent.withAlpha(0.85f));
            g.drawRoundedRectangle(badge.reduced(0.5f), 3.0f, 1.0f);
            g.setFont(fonts::micro(7.0f));
            g.drawText(mode, badge, juce::Justification::centred);
        }

        void paintTransferAxisLabels(juce::Graphics& g, juce::Rectangle<float> plot)
        {
            paintHeroMicroLabel(g, plot.removeFromBottom(12.0f).removeFromLeft(42.0f), "INPUT");
            paintHeroMicroLabel(g, juce::Rectangle<float>(plot.getX() - 2.0f, plot.getY() + 4.0f, 36.0f, 10.0f),
                                "OUTPUT", juce::Justification::centredLeft);
        }

        void paintReverbGlowParticles(juce::Graphics& g, juce::Rectangle<float> plot, float size, float damping,
                                      float mix, float phase01)
        {
            for (int i = 0; i < 56; ++i)
            {
                const float seed = static_cast<float>(i) * 0.173f;
                const float bx = plot.getX() + std::fmod(seed * 113.0f, 1.0f) * plot.getWidth();
                const float by = plot.getY() + std::fmod(seed * 97.0f, 1.0f) * plot.getHeight();
                const float age = std::fmod(phase01 + seed, 1.0f);
                const float life = 1.0f - age;
                const float radius = (2.0f + size * 5.0f) * (0.35f + life * 0.65f);
                const float alpha = life * (0.12f + mix * 0.55f) * (1.0f - damping * 0.35f);
                g.setColour((age < 0.5f ? palette::kAccent : juce::Colour(0xff8060e0)).withAlpha(alpha));
                g.fillEllipse(bx - radius, by - radius, radius * 2.0f, radius * 2.0f);
            }
        }
    } // namespace

    struct DesignFxHeroViz::EqAnalyzerState
    {
        static constexpr int kFftOrder = 9;
        static constexpr int kFftSize = 1 << kFftOrder;
        static constexpr int kBins = 32;

        juce::dsp::FFT fft{kFftOrder};
        std::array<float, static_cast<std::size_t>(kFftSize * 2)> fftData{};
        std::array<float, static_cast<std::size_t>(kFftSize)> capture{};
        std::array<float, static_cast<std::size_t>(kFftSize)> window{};
        std::array<float, static_cast<std::size_t>(kBins)> levels{};
        float windowSum = 1.0f;
        double sampleRate = 48000.0;
    };
    DesignFxHeroViz::DesignFxHeroViz(juce::AudioProcessorValueTreeState& apvts)
        : eqAnalyzer_(std::make_unique<EqAnalyzerState>()), apvts_(apvts)
    {
        rebuildEqAnalyzerWindow();
        startTimerHz(24);
    }

    DesignFxHeroViz::~DesignFxHeroViz()
    {
        stopTimer();
    }

    void DesignFxHeroViz::setDesignFxUiState(pw8::plugin::ui::DesignFxUiState* uiState)
    {
        uiState_ = uiState;
        repaint();
    }

    void DesignFxHeroViz::bindChip(std::size_t chipIndex, const juce::String& paramPrefix,
                                   MurmurProcessor* processor, int engineSlot)
    {
        chipIndex_ = chipIndex;
        paramPrefix_ = paramPrefix;
        processor_ = processor;
        engineSlot_ = engineSlot;
        designModePill_.clear();
        compGrSmoothedDb_ = 0.0f;
        compGrPeakHoldDb_ = 0.0f;
        eqAnalyzerReady_ = false;
        timerCallback();
    }

    void DesignFxHeroViz::resized()
    {
        WireframeCanvas::resized();
    }

    void DesignFxHeroViz::setDesignModePill(const juce::String& pill)
    {
        if (designModePill_ == pill)
            return;
        designModePill_ = pill;
        repaint();
    }

    float DesignFxHeroViz::uiKnob(std::size_t index, float fallback) const
    {
        if (uiState_ == nullptr)
            return fallback;
        return uiState_->knobValue(chipIndex_, index);
    }

    float DesignFxHeroViz::readParam(const char* suffix, float fallback) const
    {
        if (paramPrefix_.isEmpty() || suffix == nullptr)
            return fallback;
        if (auto* raw = apvts_.getRawParameterValue(paramPrefix_ + suffix))
            return raw->load();
        return fallback;
    }

    float DesignFxHeroViz::readCompressorGrDb() const
    {
        if (processor_ == nullptr || engineSlot_ < 0)
            return 0.0f;
        if (engineSlot_ >= 3)
            return processor_->getMasterCompressorGainReductionDb(static_cast<std::size_t>(engineSlot_ - 3));
        return processor_->getInsertCompressorGainReductionDb(static_cast<std::size_t>(engineSlot_));
    }

    void DesignFxHeroViz::refreshCachedParams()
    {
        if (auto* typeRaw = apvts_.getRawParameterValue(paramPrefix_ + "Type"))
            effectType_ = static_cast<int>(typeRaw->load() + 0.5f);
        mix_ = readParam("Mix", 1.0f);
        driveDb_ = readParam("SaturationDrive", 6.0f);
        chorusRateHz_ = readParam("ChorusRate", 0.5f);
        chorusDepthMs_ = readParam("ChorusDepth", 4.0f);
        chorusBaseDelayMs_ = readParam("ChorusBaseDelay", 12.0f);
        tapeDriftRate_ = readParam("TapeDriftRate", 0.3f);
        tapeDriftDepth_ = readParam("TapeDriftDepth", 1.5f);
        tapeDriveDb_ = readParam("TapeDrive", 3.0f);
        freqShiftHz_ = readParam("FreqShiftHz", 7.0f);
        freqShiftFeedback_ = readParam("FreqShiftFeedback", 0.55f);
        freqShiftLowCutHz_ = readParam("FreqShiftLowCutHz", 120.0f);
        fractalMorph_ = readParam("FractalMorph", 0.0f);
        reverbDecaySec_ = readParam("ReverbDecaySeconds", 2.0f);
        reverbSize_ = readParam("ReverbSize", 1.0f);
        reverbDamping_ = readParam("ReverbHighRatio", 0.6f);
        reverbPreDelayMs_ = readParam("ReverbPreDelayMs", 20.0f);
        eqLowDb_ = readParam("EqLowGainDb", 0.0f);
        eqMidDb_ = readParam("EqMidGainDb", 0.0f);
        eqHighDb_ = readParam("EqHighGainDb", 0.0f);
        eqLowFreqHz_ = readParam("EqLowFreqHz", 200.0f);
        eqMidFreqHz_ = readParam("EqMidFreqHz", 1000.0f);
        eqHighFreqHz_ = readParam("EqHighFreqHz", 6000.0f);
        eqMidQ_ = readParam("EqMidQ", 0.8f);
        compThresholdDb_ = readParam("CompThresholdDb", -18.0f);
        compRatio_ = readParam("CompRatio", 4.0f);
        compKneeDb_ = readParam("CompKneeDb", 6.0f);
        limiterCeilingDb_ = readParam("LimiterCeilingDb", -0.2f);
        limiterReleaseMs_ = readParam("LimiterReleaseMs", 50.0f);
        vocoderBands_ = static_cast<int>(readParam("VocoderBandCount", 16.0f) + 0.5f);
        vocoderFormant_ = readParam("VocoderFormant", 0.5f);
        vocoderSibilance_ = readParam("VocoderSibilance", 0.0f);
        cloudsDensity_ = readParam("CloudsDensity", 0.35f);
        cloudsGrainSizeMs_ = readParam("CloudsGrainSizeMs", 80.0f);
        cloudsPitch_ = readParam("CloudsPitch", 1.0f);
        cloudsFreeze_ = readParam("CloudsFreeze", 0.0f);
        if (auto* raw = apvts_.getRawParameterValue(paramPrefix_ + "CloudsMode"))
            cloudsMode_ = static_cast<int>(raw->load() + 0.5f);
        compGrDb_ = readCompressorGrDb();
        compGrSmoothedDb_ += (compGrDb_ - compGrSmoothedDb_) * 0.38f;
        compGrPeakHoldDb_ = juce::jmax(compGrPeakHoldDb_ * 0.94f, -compGrDb_);
        if (auto* raw = apvts_.getRawParameterValue(paramPrefix_ + "ReverbCharacter"))
            reverbCharacter_ = static_cast<int>(raw->load() + 0.5f);
        if (auto* raw = apvts_.getRawParameterValue(paramPrefix_ + "SaturationCharacter"))
            saturationCharacter_ = static_cast<int>(raw->load() + 0.5f);
        if (auto* raw = apvts_.getRawParameterValue(paramPrefix_ + "CompCharacter"))
            compCharacter_ = static_cast<int>(raw->load() + 0.5f);
    }

    float DesignFxHeroViz::eqFreqToT(float hz) const
    {
        constexpr float kMinHz = 20.0f;
        constexpr float kMaxHz = 20000.0f;
        const float logMin = std::log10(kMinHz);
        const float logMax = std::log10(kMaxHz);
        const float clamped = juce::jlimit(kMinHz, kMaxHz, hz);
        return juce::jlimit(0.03f, 0.97f, (std::log10(clamped) - logMin) / (logMax - logMin));
    }

    float DesignFxHeroViz::eqTToFreq(float t) const
    {
        constexpr float kMinHz = 20.0f;
        constexpr float kMaxHz = 20000.0f;
        const float logMin = std::log10(kMinHz);
        const float logMax = std::log10(kMaxHz);
        return std::pow(10.0f, logMin + juce::jlimit(0.0f, 1.0f, t) * (logMax - logMin));
    }

    float DesignFxHeroViz::eqFreqForBand(EqDragBand band) const
    {
        switch (band)
        {
            case EqDragBand::Low: return eqLowFreqHz_;
            case EqDragBand::Mid: return eqMidFreqHz_;
            case EqDragBand::High: return eqHighFreqHz_;
            default: return 1000.0f;
        }
    }

    float DesignFxHeroViz::constrainEqBandFreq(EqDragBand band, float freqHz) const
    {
        constexpr float kMinHz = 20.0f;
        constexpr float kMaxHz = 20000.0f;
        constexpr float kMinGapRatio = 1.12f;

        freqHz = juce::jlimit(kMinHz, kMaxHz, freqHz);

        switch (band)
        {
            case EqDragBand::Low:
                return juce::jmin(freqHz, eqMidFreqHz_ / kMinGapRatio);
            case EqDragBand::Mid:
                return juce::jlimit(eqLowFreqHz_ * kMinGapRatio, eqHighFreqHz_ / kMinGapRatio, freqHz);
            case EqDragBand::High:
                return juce::jmax(freqHz, eqMidFreqHz_ * kMinGapRatio);
            default:
                return freqHz;
        }
    }

    float DesignFxHeroViz::eqPlotYFromDb(float db) const
    {
        if (eqPlotBounds_.isEmpty())
            return 0.0f;
        return eqPlotBounds_.getCentreY() - juce::jlimit(-24.0f, 24.0f, db) / 24.0f * eqPlotBounds_.getHeight() * 0.42f;
    }

    float DesignFxHeroViz::eqDbFromPlotY(float y) const
    {
        if (eqPlotBounds_.isEmpty())
            return 0.0f;
        const float norm = (eqPlotBounds_.getCentreY() - y) / (eqPlotBounds_.getHeight() * 0.42f);
        return juce::jlimit(-24.0f, 24.0f, norm * 24.0f);
    }

    juce::Point<float> DesignFxHeroViz::eqBandHandle(EqDragBand band) const
    {
        const float t = eqFreqToT(eqFreqForBand(band));
        const float db = band == EqDragBand::Low ? eqLowDb_ : band == EqDragBand::Mid ? eqMidDb_ : eqHighDb_;
        return {eqPlotBounds_.getX() + eqPlotBounds_.getWidth() * t, eqPlotYFromDb(db)};
    }

    DesignFxHeroViz::EqDragBand DesignFxHeroViz::hitEqBand(juce::Point<float> pt) const
    {
        constexpr float kHandleRadius = 10.0f;
        for (const auto band : {EqDragBand::Low, EqDragBand::Mid, EqDragBand::High})
        {
            if (pt.getDistanceFrom(eqBandHandle(band)) <= kHandleRadius)
                return band;
        }
        return EqDragBand::None;
    }

    void DesignFxHeroViz::setEqGainParam(const char* suffix, float db)
    {
        if (paramPrefix_.isEmpty() || suffix == nullptr)
            return;
        if (auto* param = apvts_.getParameter(paramPrefix_ + suffix))
            param->setValueNotifyingHost(param->convertTo0to1(juce::jlimit(-24.0f, 24.0f, db)));
    }

    void DesignFxHeroViz::setEqFloatParam(const char* suffix, float value, float minV, float maxV)
    {
        if (paramPrefix_.isEmpty() || suffix == nullptr)
            return;
        if (auto* param = apvts_.getParameter(paramPrefix_ + suffix))
            param->setValueNotifyingHost(param->convertTo0to1(juce::jlimit(minV, maxV, value)));
    }

    void DesignFxHeroViz::mouseDown(const juce::MouseEvent& event)
    {
        if (chipIndex_ == 8 && !eqAnalyzerToggleBounds_.isEmpty()
            && eqAnalyzerToggleBounds_.contains(event.position.toFloat()))
        {
            if (uiState_ != nullptr)
            {
                uiState_->setEqAnalyzerActive(!uiState_->eqAnalyzerActive());
                if (onUiPreferenceChanged)
                    onUiPreferenceChanged();
                repaint();
            }
            return;
        }

        if (chipIndex_ == 10 && !limTruePeakToggleBounds_.isEmpty()
            && limTruePeakToggleBounds_.contains(event.position.toFloat()))
        {
            if (uiState_ != nullptr)
            {
                uiState_->setLimTruePeakActive(!uiState_->limTruePeakActive());
                applyLimStubKnobs(apvts_, paramPrefix_, *uiState_);
                if (onUiPreferenceChanged)
                    onUiPreferenceChanged();
                repaint();
            }
            return;
        }

        if (chipIndex_ != 8 || eqPlotBounds_.isEmpty())
            return;

        eqDragBand_ = hitEqBand(event.position.toFloat());
        eqDragQMode_ = event.mods.isShiftDown() && eqDragBand_ == EqDragBand::Mid;
    }

    void DesignFxHeroViz::mouseDrag(const juce::MouseEvent& event)
    {
        if (eqDragBand_ == EqDragBand::None)
            return;

        if (eqDragQMode_)
        {
            const float q = juce::jmap(eqDbFromPlotY(event.position.y), 24.0f, -24.0f, 0.25f, 8.0f);
            eqMidQ_ = q;
            setEqFloatParam("EqMidQ", q, 0.1f, 10.0f);
            repaint();
            return;
        }

        const float db = eqDbFromPlotY(event.position.y);
        const float t = juce::jlimit(0.03f, 0.97f,
                                      (event.position.x - eqPlotBounds_.getX()) / eqPlotBounds_.getWidth());
        const float freq = constrainEqBandFreq(eqDragBand_, eqTToFreq(t));

        switch (eqDragBand_)
        {
            case EqDragBand::Low:
                eqLowDb_ = db;
                eqLowFreqHz_ = freq;
                setEqGainParam("EqLowGainDb", db);
                setEqFloatParam("EqLowFreqHz", freq, 20.0f, 20000.0f);
                break;
            case EqDragBand::Mid:
                eqMidDb_ = db;
                eqMidFreqHz_ = freq;
                setEqGainParam("EqMidGainDb", db);
                setEqFloatParam("EqMidFreqHz", freq, 20.0f, 20000.0f);
                break;
            case EqDragBand::High:
                eqHighDb_ = db;
                eqHighFreqHz_ = freq;
                setEqGainParam("EqHighGainDb", db);
                setEqFloatParam("EqHighFreqHz", freq, 20.0f, 20000.0f);
                break;
            default:
                break;
        }
        repaint();
    }

    void DesignFxHeroViz::mouseUp(const juce::MouseEvent& event)
    {
        juce::ignoreUnused(event);
        eqDragBand_ = EqDragBand::None;
        eqDragQMode_ = false;
    }

    void DesignFxHeroViz::updateLimiterAnimation()
    {
        const float isp = uiKnob(3, 0.65f);
        const float inputGain = uiKnob(4, 0.55f);
        const float speed = juce::jmap(limiterReleaseMs_, 10.0f, 200.0f, 0.11f, 0.03f);
        limiterAnimPhase_ += speed;
        if (limiterAnimPhase_ > juce::MathConstants<float>::twoPi)
            limiterAnimPhase_ -= juce::MathConstants<float>::twoPi;

        const float gainBoost = 0.55f + inputGain * 0.45f;
        const float wave =
            (std::abs(std::sin(limiterAnimPhase_ * 1.5f)) * 0.72f + std::abs(std::sin(limiterAnimPhase_ * 0.37f)) * 0.18f)
            * gainBoost;
        limiterInputPeak_ = wave;
        limiterPeakHold_ = juce::jmax(limiterPeakHold_ * 0.92f, wave);

        const float ceilingNorm = juce::jmap(limiterCeilingDb_, -12.0f, 0.0f, 0.35f, 0.08f);
        const float clipThreshold = (0.5f - ceilingNorm) / 0.42f;
        limiterClipping_ = limiterInputPeak_ > clipThreshold * (1.05f - isp * 0.08f);
    }

    void DesignFxHeroViz::rebuildEqAnalyzerWindow()
    {
        if (eqAnalyzer_ == nullptr)
            return;

        float sum = 0.0f;
        for (int i = 0; i < EqAnalyzerState::kFftSize; ++i)
        {
            const float w = 0.5f
                            * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i)
                                               / static_cast<float>(EqAnalyzerState::kFftSize - 1)));
            eqAnalyzer_->window[static_cast<std::size_t>(i)] = w;
            sum += w;
        }
        eqAnalyzer_->windowSum = juce::jmax(1.0e-6f, sum);
    }

    void DesignFxHeroViz::updateEqAnalyzerSpectrum()
    {
        if (processor_ == nullptr || eqAnalyzer_ == nullptr)
        {
            eqAnalyzerReady_ = false;
            return;
        }

        eqAnalyzer_->sampleRate = processor_->getScopeSampleRate();
        if (eqAnalyzer_->sampleRate <= 0.0)
            eqAnalyzer_->sampleRate = 48000.0;

        const int pulled = processor_->readScopeSamples(eqAnalyzer_->capture.data(), EqAnalyzerState::kFftSize);
        if (pulled < EqAnalyzerState::kFftSize / 2)
        {
            eqAnalyzerReady_ = false;
            return;
        }

        eqAnalyzer_->fftData.fill(0.0f);
        for (int i = 0; i < EqAnalyzerState::kFftSize; ++i)
        {
            eqAnalyzer_->fftData[static_cast<std::size_t>(i)] =
                eqAnalyzer_->capture[static_cast<std::size_t>(i)] * eqAnalyzer_->window[static_cast<std::size_t>(i)];
        }

        eqAnalyzer_->fft.performFrequencyOnlyForwardTransform(eqAnalyzer_->fftData.data());

        constexpr float kMinHz = 20.0f;
        constexpr float kMaxHz = 20000.0f;
        constexpr float kMinDb = -62.0f;
        constexpr float kMaxDb = 0.0f;

        for (int i = 0; i < EqAnalyzerState::kBins; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(EqAnalyzerState::kBins - 1);
            const float freq = kMinHz * std::pow(kMaxHz / kMinHz, t);
            const float binIndex =
                freq * static_cast<float>(EqAnalyzerState::kFftSize) / static_cast<float>(eqAnalyzer_->sampleRate);

            const int i0 = static_cast<int>(binIndex);
            const int i1 = juce::jmin(EqAnalyzerState::kFftSize / 2 - 1, i0 + 1);
            const float frac = binIndex - static_cast<float>(i0);
            const float mag = eqAnalyzer_->fftData[static_cast<std::size_t>(i0)] * (1.0f - frac)
                              + eqAnalyzer_->fftData[static_cast<std::size_t>(i1)] * frac;
            const float magNorm = juce::jmax(1.0e-8f, mag * 2.0f / eqAnalyzer_->windowSum);
            const float db = juce::Decibels::gainToDecibels(magNorm, kMinDb);
            const float norm = juce::jlimit(0.0f, 1.0f, juce::jmap(db, kMinDb, kMaxDb, 0.0f, 1.0f));
            const auto idx = static_cast<std::size_t>(i);
            eqAnalyzer_->levels[idx] += (norm - eqAnalyzer_->levels[idx]) * 0.35f;
        }

        eqAnalyzerReady_ = true;
    }

    void DesignFxHeroViz::paintEqAnalyzerSpectrum(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        if (!eqAnalyzerReady_ || eqAnalyzer_ == nullptr)
            return;

        for (int band = 0; band < EqAnalyzerState::kBins; ++band)
        {
            const float t = static_cast<float>(band) / static_cast<float>(EqAnalyzerState::kBins - 1);
            const float level = eqAnalyzer_->levels[static_cast<std::size_t>(band)];
            const float x = plot.getX() + t * plot.getWidth();
            const float w = plot.getWidth() / static_cast<float>(EqAnalyzerState::kBins);
            const float h = plot.getHeight() * 0.22f * level;
            g.setColour(palette::kAccent.withAlpha(0.10f + level * 0.18f));
            g.fillRect(x, plot.getBottom() - h, w, h);
        }
    }

    void DesignFxHeroViz::timerCallback()
    {
        if (paramPrefix_.isEmpty())
            return;

        refreshCachedParams();
        heroAnimPhase_ += 1.0f / 24.0f;
        if (heroAnimPhase_ > 1.0f)
            heroAnimPhase_ -= 1.0f;
        if (chipIndex_ == 10)
            updateLimiterAnimation();
        if (chipIndex_ == 8 && uiState_ != nullptr && uiState_->eqAnalyzerActive() && processor_ != nullptr)
            updateEqAnalyzerSpectrum();

        const auto& spec = fxDesignSpecForChip(chipIndex_);
        setCaption(spec.vizTitle);
        setSubCaption({});
        repaint();
    }

    void DesignFxHeroViz::paintPlotGrid(juce::Graphics& g, juce::Rectangle<float> plot)
    {
        g.setColour(palette::kBorder.withAlpha(0.28f));
        for (int i = 1; i < 5; ++i)
        {
            const float t = static_cast<float>(i) / 5.0f;
            g.drawVerticalLine(static_cast<int>(plot.getX() + plot.getWidth() * t), plot.getY(), plot.getBottom());
            g.drawHorizontalLine(static_cast<int>(plot.getY() + plot.getHeight() * t), plot.getX(), plot.getRight());
        }
    }

    void DesignFxHeroViz::paintSaturationTransfer(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);

        const float midY = plot.getCentreY();
        const float halfH = plot.getHeight() * 0.42f;
        juce::Path linearRef;
        linearRef.startNewSubPath(plot.getX() + 4.0f, midY + halfH);
        linearRef.lineTo(plot.getRight() - 4.0f, midY - halfH);
        g.setColour(palette::kFigmaFxMutedText.withAlpha(0.42f));
        const float dashLens[] = {5.0f, 5.0f};
        juce::Path dashedLinear;
        juce::PathStrokeType stroke(1.2f);
        stroke.createDashedStroke(dashedLinear, linearRef, dashLens, 2);
        g.strokePath(dashedLinear, stroke);

        const float tone = uiKnob(1);
        const float driveLinear = juce::Decibels::decibelsToGain(driveDb_, -48.0f) * (0.65f + tone * 0.75f);
        const int character = designModePill_.isEmpty() ? saturationCharacter_
                                                        : saturationCharacterFromDesignPill(designModePill_);
        const auto key = preview::hashCombine(0x5A700002ULL, preview::quantizeToKey(driveLinear, 32.0f));
        const auto key2 = preview::hashCombine(key, static_cast<uint64_t>(character));
        const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
            key2, preview::kDefaultPolylinePoints,
            [&](int n, preview::PreviewPolyline& out)
            {
                out.xNorm.clear();
                out.yNorm.resize(static_cast<std::size_t>(n));
                for (int i = 0; i < n; ++i)
                {
                    const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                    const float x = t * 2.0f - 1.0f;
                    switch (character)
                    {
                        case 1: out.yNorm[static_cast<std::size_t>(i)] = std::tanh(x * driveLinear * 0.38f) * 0.92f; break;
                        case 2: out.yNorm[static_cast<std::size_t>(i)] = juce::jlimit(-1.0f, 1.0f, x * (1.0f + driveLinear * 0.25f)); break;
                        case 3: out.yNorm[static_cast<std::size_t>(i)] = std::sin(x * driveLinear * 0.9f) * 0.75f; break;
                        default: out.yNorm[static_cast<std::size_t>(i)] = std::tanh(x * driveLinear * 0.55f); break;
                    }
                }
            });
        preview::PolylineDrawOptions opts;
        opts.alpha = juce::jlimit(0.45f, 1.0f, mix_ + 0.25f);
        opts.strokeWidth = 2.2f;
        preview::paintPolylineCurve(g, plot, polyline, opts);

        juce::String modelLabel = "TUBE";
        switch (character)
        {
            case 1: modelLabel = "TAPE"; break;
            case 2: modelLabel = "DIODE"; break;
            case 3: modelLabel = "FOLD"; break;
            default: break;
        }
        paintHeroModeBadge(g, plot, modelLabel);
        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              juce::String(driveDb_, 1) + " dB DRIVE");
        paintTransferAxisLabels(g, plot);
    }

    void DesignFxHeroViz::paintChorusSpatializer(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);

        const float depthNorm = juce::jlimit(0.0f, 1.0f, chorusDepthMs_ / 12.0f);
        const float rate = juce::jlimit(0.2f, 4.0f, chorusRateHz_);
        const float phase = heroAnimPhase_ * juce::MathConstants<float>::twoPi * rate;

        struct VoiceSpec
        {
            juce::Colour colour;
            float phaseOff;
            float spreadMul;
        };
        static const VoiceSpec kVoices[] = {
            {juce::Colour(0xffd040e8), 0.0f, -1.0f},
            {palette::kAccent, 0.42f, 0.0f},
            {juce::Colour(0xff7090e8), -0.36f, 1.0f},
        };

        for (const auto& voice : kVoices)
        {
            const float spread = voice.spreadMul * depthNorm * 0.10f;
            juce::Path path;
            for (int i = 0; i <= 72; ++i)
            {
                const float t = static_cast<float>(i) / 72.0f;
                const float v =
                    std::sin(t * juce::MathConstants<float>::twoPi * 2.2f + phase + voice.phaseOff + spread)
                    * (0.28f + depthNorm * 0.38f);
                const float x = plot.getX() + t * plot.getWidth();
                const float y = plot.getCentreY() - v * plot.getHeight() * 0.42f;
                if (i == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }
            g.setColour(voice.colour.withAlpha(0.32f + mix_ * 0.55f));
            g.strokePath(path, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        paintHeroMicroLabel(g, plot.removeFromTop(12.0f).removeFromLeft(18.0f), "L");
        paintHeroMicroLabel(g, plot.removeFromTop(12.0f).removeFromRight(18.0f), "R", juce::Justification::centredRight);
        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              juce::String(chorusRateHz_, 2) + " Hz · " + juce::String(chorusDepthMs_, 1) + " ms");
    }

    void DesignFxHeroViz::paintTapeDrift(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);
        preview::paintTapeAnimation(g, plot, tapeDriftRate_, tapeDriftDepth_, mix_, heroAnimPhase_);

        const float flutterPhase = heroAnimPhase_ * juce::MathConstants<float>::twoPi * tapeDriftRate_ * 3.2f;
        juce::Path flutter;
        for (int i = 0; i <= 56; ++i)
        {
            const float t = static_cast<float>(i) / 56.0f;
            const float x = plot.getX() + t * plot.getWidth();
            const float wow = std::sin(t * juce::MathConstants<float>::twoPi * 2.0f + flutterPhase)
                              * tapeDriftDepth_ * 0.012f;
            const float y = plot.getCentreY() + wow * plot.getHeight();
            if (i == 0)
                flutter.startNewSubPath(x, y);
            else
                flutter.lineTo(x, y);
        }
        g.setColour(palette::kAccentWarm.withAlpha(0.45f + mix_ * 0.35f));
        g.strokePath(flutter, juce::PathStrokeType(1.2f));

        paintHeroMicroLabel(g, plot.removeFromTop(12.0f).removeFromLeft(36.0f), "WOW");
        paintHeroMicroLabel(g, plot.removeFromTop(12.0f).removeFromRight(42.0f), "FLUTTER", juce::Justification::centredRight);
        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              juce::String(tapeDriftRate_, 2) + " Hz · DRIVE " + juce::String(tapeDriveDb_, 1) + " dB",
                              palette::kAccentWarm);
    }

    void DesignFxHeroViz::paintMoodResponse(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        const juce::String mode = designModePill_.isEmpty() ? "WARM" : designModePill_;
        const float intensity = uiKnob(0, 0.55f);
        const float colorKnob = uiKnob(1, 0.5f);
        const float filterT = uiKnob(2, 0.45f);
        const float resonance = uiKnob(3, 0.5f);
        const float drive = uiKnob(4, 0.35f);
        const float qScale = 0.08f + resonance * 0.14f;
        const float freqShift = (filterT - 0.5f) * 0.35f;
        const float gainScale = 0.55f + intensity * 0.9f + drive * 0.35f;
        const float colorTilt = (colorKnob - 0.5f) * 0.4f;

        paintPlotGrid(g, plot);
        paintHeroModeBadge(g, plot, mode);

        paintFlatWaveform(
            g, plot, 72,
            [=](float t)
            {
                const float x = juce::jlimit(0.0f, 1.0f, t + freqShift);
                float warm = gainScale * 0.45f * std::exp(-std::pow((x - 0.22f) / qScale, 2.0f));
                float mid = gainScale * -0.25f * std::exp(-std::pow((x - 0.55f) / (qScale * 0.85f), 2.0f));
                float air = gainScale * (0.35f + colorTilt) * std::exp(-std::pow((x - 0.82f) / qScale, 2.0f));
                if (mode == "DARK")
                {
                    warm *= 1.35f;
                    mid -= 0.15f;
                    air *= 0.35f;
                }
                else if (mode == "BRIGHT")
                {
                    warm *= 0.55f;
                    air *= 1.45f;
                }
                else if (mode == "ACID")
                {
                    mid = 0.55f * std::exp(-std::pow((t - 0.48f) / 0.05f, 2.0f));
                    air *= 0.8f;
                }
                else if (mode == "ETHEREAL")
                {
                    warm *= 0.7f;
                    air *= 1.8f;
                    mid *= 0.4f;
                }
                return warm + mid + air;
            },
            true);

        paintHeroMicroLabel(g, plot.removeFromBottom(12.0f).removeFromLeft(28.0f), "FREQ");
        paintHeroMicroLabel(g, plot.removeFromLeft(12.0f).removeFromTop(28.0f), "GAIN", juce::Justification::centredLeft);
        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              juce::String(juce::roundToInt(intensity * 100.0f)) + "% INT · RES "
                                  + juce::String(juce::roundToInt(resonance * 100.0f)) + "%");
    }

    void DesignFxHeroViz::paintFreqShiftBode(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);
        const float detune = uiKnob(4, 0.5f);
        const float stereo = uiKnob(5, 0.5f);
        const float scaleNorm = juce::jlimit(0.0f, 1.0f, (freqShiftLowCutHz_ - 80.0f) / 4000.0f);
        const float shiftNorm =
            juce::jlimit(-1.0f, 1.0f, (freqShiftHz_ + (detune - 0.5f) * 24.0f) / 40.0f);

        juce::Path spiral;
        for (int i = 0; i <= 48; ++i)
        {
            const float t = static_cast<float>(i) / 48.0f;
            const float angle = t * juce::MathConstants<float>::twoPi * (1.5f + std::abs(shiftNorm) * 2.0f);
            const float radius = (1.0f - t * (0.55f - scaleNorm * 0.15f)) * plot.getWidth() * 0.22f;
            const float x = plot.getCentreX() + std::cos(angle) * radius * (1.0f + stereo * 0.12f);
            const float y = plot.getCentreY() + std::sin(angle) * radius * 0.5f * (1.0f - stereo * 0.08f)
                                                                                         - t * plot.getHeight() * shiftNorm
                                                                                         * 0.15f;
            if (i == 0)
                spiral.startNewSubPath(x, y);
            else
                spiral.lineTo(x, y);
        }
        strokeGlowPath(g, spiral, juce::jlimit(0.45f, 1.0f, mix_ + freqShiftFeedback_ * 0.35f), 1.8f, true);

        const float ringCount = 4.0f + freqShiftFeedback_ * 4.0f;
        for (int ring = 1; ring <= static_cast<int>(ringCount); ++ring)
        {
            const float t = static_cast<float>(ring) / ringCount;
            const float radius = t * plot.getWidth() * 0.18f * (1.0f + stereo * 0.08f);
            g.setColour(palette::kAccent.withAlpha((1.0f - t) * 0.14f * mix_));
            g.drawEllipse(plot.getCentreX() - radius, plot.getCentreY() - radius * 0.55f, radius * 2.0f,
                          radius * 1.1f, 1.0f);
        }

        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              juce::String(freqShiftHz_, 1) + " Hz · FB "
                                  + juce::String(juce::roundToInt(freqShiftFeedback_ * 100.0f)) + "%");
    }

    void DesignFxHeroViz::paintFractalCloud(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);
        const int seed = 0x0FAC7A1 ^ static_cast<int>(fractalMorph_ * 1000.0f);
        preview::paintCloudsAnimation(g, plot, mix_, seed, heroAnimPhase_);
        const float position = uiKnob(4, 0.5f);
        const float xBias = (position - 0.5f) * plot.getWidth() * 0.35f;
        juce::Path stream;
        stream.startNewSubPath(plot.getX() + 8.0f + xBias * 0.25f, plot.getCentreY());
        stream.quadraticTo(plot.getCentreX() + xBias, plot.getY() + plot.getHeight() * 0.25f, plot.getRight() - 8.0f,
                           plot.getCentreY() - plot.getHeight() * 0.08f);
        strokeGlowPath(g, stream, juce::jlimit(0.4f, 1.0f, mix_ + 0.15f), 1.4f, true);
        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              "GRAIN " + juce::String(fractalMorph_, 2) + " · SCATTER "
                                  + juce::String(juce::roundToInt(mix_ * 100.0f)) + "%");
    }

    void DesignFxHeroViz::paintReverbDecay(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);
        preview::paintReverbAnimation(g, plot, reverbDecaySec_, reverbSize_, reverbDamping_, mix_, heroAnimPhase_);
        paintReverbGlowParticles(g, plot, reverbSize_, reverbDamping_, mix_, heroAnimPhase_);

        juce::String modeLabel = designModePill_;
        if (modeLabel.isEmpty())
        {
            static constexpr const char* kChars[] = {"HALL", "PLATE", "ROOM", "SPRING", "SHIMMER"};
            modeLabel = kChars[juce::jlimit(0, 4, reverbCharacter_)];
        }
        paintHeroModeBadge(g, plot, modeLabel);
        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              juce::String(reverbDecaySec_, 1) + " s · SIZE "
                                  + juce::String(juce::roundToInt(reverbSize_ * 100.0f)) + "%");
    }

    void DesignFxHeroViz::paintEqHandles(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        juce::ignoreUnused(plot);
        const auto drawHandle = [&](EqDragBand band, const juce::String& label)
        {
            const auto handle = eqBandHandle(band);
            const bool active = eqDragBand_ == band;
            g.setColour(active ? palette::kAccent : palette::kAccentWarm.withAlpha(0.85f));
            g.fillEllipse(handle.x - 6.0f, handle.y - 6.0f, 12.0f, 12.0f);
            g.setColour(palette::kPanel);
            g.fillEllipse(handle.x - 2.5f, handle.y - 2.5f, 5.0f, 5.0f);
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(7.0f));
            g.drawText(label, juce::Rectangle<float>(handle.x - 16.0f, handle.y + 8.0f, 32.0f, 10.0f),
                       juce::Justification::centred);

            const float hz = eqFreqForBand(band);
            juce::String freqLabel = hz >= 1000.0f ? juce::String(hz / 1000.0f, 1) + "k" : juce::String(juce::roundToInt(hz));
            g.setColour(palette::kTextDim.withAlpha(0.75f));
            g.drawText(freqLabel, juce::Rectangle<float>(handle.x - 18.0f, handle.y - 20.0f, 36.0f, 10.0f),
                       juce::Justification::centred);
        };

        drawHandle(EqDragBand::Low, "LOW");
        drawHandle(EqDragBand::Mid, "MID");
        drawHandle(EqDragBand::High, "HIGH");

        if (eqDragQMode_ || eqDragBand_ == EqDragBand::Mid)
        {
            g.setColour(palette::kTextDim.withAlpha(0.55f));
            g.setFont(fonts::label(7.0f));
            g.drawText("Q " + juce::String(eqMidQ_, 1),
                       plot.removeFromBottom(12.0f).removeFromLeft(40.0f), juce::Justification::centredLeft);
        }

        g.setColour(palette::kTextDim.withAlpha(0.55f));
        g.setFont(fonts::label(7.0f));
        g.drawText(eqDragQMode_ ? "SHIFT: Q" : "DRAG · SHIFT+MID: Q", plot.removeFromTop(12.0f).removeFromRight(120.0f),
                   juce::Justification::centredRight);
    }

    void DesignFxHeroViz::paintEqCurve(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);

        const bool analyzerOn = uiState_ != nullptr && uiState_->eqAnalyzerActive();
        eqAnalyzerToggleBounds_ = juce::Rectangle<float>(plot.getX() + 4.0f, plot.getY() + 2.0f, 112.0f, 14.0f);
        g.setColour(analyzerOn ? palette::kAccent.withAlpha(0.22f) : palette::kPanel.withAlpha(0.45f));
        g.fillRoundedRectangle(eqAnalyzerToggleBounds_, 3.0f);
        g.setColour(analyzerOn ? palette::kAccent : palette::kTextDim.withAlpha(0.55f));
        g.setFont(fonts::label(7.0f));
        g.drawText(analyzerOn ? "ANALYZER ACTIVE" : "ANALYZER OFF", eqAnalyzerToggleBounds_,
                   juce::Justification::centred);

        if (analyzerOn)
            paintEqAnalyzerSpectrum(g, plot);
        else
        {
            const float lowT = eqFreqToT(eqLowFreqHz_);
            const float midT = eqFreqToT(eqMidFreqHz_);
            const float highT = eqFreqToT(eqHighFreqHz_);
            const float midWidth = juce::jlimit(0.025f, 0.18f, 0.06f / juce::jmax(0.15f, eqMidQ_));
            auto key = preview::hashCombine(0xE0000003ULL, preview::quantizeToKey(lowT, 32.0f));
            key = preview::hashCombine(key, preview::quantizeToKey(midT, 32.0f));
            key = preview::hashCombine(key, preview::quantizeToKey(highT, 32.0f));
            key = preview::hashCombine(key, preview::quantizeToKey(eqLowDb_, 32.0f));
            key = preview::hashCombine(key, preview::quantizeToKey(eqMidDb_, 32.0f));
            key = preview::hashCombine(key, preview::quantizeToKey(eqHighDb_, 32.0f));
            key = preview::hashCombine(key, preview::quantizeToKey(midWidth, 32.0f));
            const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
                key, 96,
                [lowT, midT, highT, midWidth, this](int n, preview::PreviewPolyline& out)
                {
                    out.xNorm.clear();
                    out.yNorm.resize(static_cast<std::size_t>(n));
                    const float low = juce::Decibels::decibelsToGain(eqLowDb_) - 1.0f;
                    const float mid = juce::Decibels::decibelsToGain(eqMidDb_) - 1.0f;
                    const float high = juce::Decibels::decibelsToGain(eqHighDb_) - 1.0f;
                    for (int i = 0; i < n; ++i)
                    {
                        const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                        const float l = low * std::exp(-std::pow((t - lowT) / 0.10f, 2.0f));
                        const float m = mid * std::exp(-std::pow((t - midT) / midWidth, 2.0f));
                        const float h = high * std::exp(-std::pow((t - highT) / 0.12f, 2.0f));
                        out.yNorm[static_cast<std::size_t>(i)] = (l + m + h) * 0.55f;
                    }
                });
            preview::PolylineDrawOptions opts;
            opts.alpha = juce::jlimit(0.45f, 1.0f, mix_ + 0.2f);
            preview::paintPolylineCurve(g, plot, polyline, opts);
        }

        paintEqHandles(g, plot);
    }

    void DesignFxHeroViz::paintEqParamStrip(juce::Graphics& g, juce::Rectangle<float> strip) const
    {
        g.setColour(palette::kFigmaFxChipFill);
        g.fillRoundedRectangle(strip, 6.0f);
        g.setColour(palette::kFigmaFxChipBorder);
        g.drawRoundedRectangle(strip.reduced(0.5f), 6.0f, 1.0f);

        const float chipW = strip.getWidth() / 3.0f;
        const auto drawBand = [&](float x, const char* label, float db)
        {
            auto cell = juce::Rectangle<float>(x, strip.getY(), chipW, strip.getHeight()).reduced(6.0f, 4.0f);
            g.setColour(palette::kFigmaFxMutedText);
            g.setFont(fonts::micro(7.0f));
            g.drawText(label, cell.removeFromTop(10.0f), juce::Justification::centred);
            g.setColour(palette::kAccent);
            g.setFont(fonts::denseBold(8.0f));
            const juce::String value =
                (db >= 0.0f ? "+" : "") + juce::String(db, db >= 10.0f || db <= -10.0f ? 0 : 1) + " dB";
            g.drawText(value, cell, juce::Justification::centred);
        };

        drawBand(strip.getX(), "LOW", eqLowDb_);
        drawBand(strip.getX() + chipW, "MID", eqMidDb_);
        drawBand(strip.getX() + chipW * 2.0f, "HIGH", eqHighDb_);
    }

    void DesignFxHeroViz::paintCompressorDynamics(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);
        const float threshNorm = juce::jmap(compThresholdDb_, -48.0f, 0.0f, 0.15f, 0.85f);
        const float ratioInv = 1.0f / juce::jmax(1.0f, compRatio_);
        const auto key = preview::hashCombine(0xC0000005ULL, preview::quantizeToKey(threshNorm, 32.0f));
        const auto key2 = preview::hashCombine(key, preview::quantizeToKey(compRatio_, 16.0f));
        const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
            key2, 64,
            [&](int n, preview::PreviewPolyline& out)
            {
                out.xNorm.clear();
                out.yNorm.resize(static_cast<std::size_t>(n));
                for (int i = 0; i < n; ++i)
                {
                    const float x = static_cast<float>(i) / static_cast<float>(n - 1);
                    float y = x;
                    if (x > threshNorm)
                        y = threshNorm + (x - threshNorm) * ratioInv;
                    out.yNorm[static_cast<std::size_t>(i)] = y * 2.0f - 1.0f;
                }
            });
        preview::PolylineDrawOptions opts;
        opts.alpha = juce::jlimit(0.45f, 1.0f, mix_ + 0.15f);
        preview::paintPolylineCurve(g, plot, polyline, opts);

        const float inNorm = juce::jlimit(0.0f, 1.0f, 0.22f + (-compGrSmoothedDb_ / 20.0f) + mix_ * 0.18f);
        const float inHoldNorm = juce::jlimit(0.0f, 1.0f, inNorm + compGrPeakHoldDb_ / 48.0f);
        auto inBar = plot.removeFromLeft(18.0f).reduced(2.0f, 8.0f);
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(inBar, 2.0f);
        g.setColour(palette::kAccent.withAlpha(0.35f));
        g.fillRoundedRectangle(inBar.getX(), inBar.getBottom() - inBar.getHeight() * inHoldNorm, inBar.getWidth(),
                               inBar.getHeight() * inHoldNorm, 2.0f);
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(inBar.getX(), inBar.getBottom() - inBar.getHeight() * inNorm, inBar.getWidth(),
                               inBar.getHeight() * inNorm, 2.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("IN", inBar.translated(0.0f, -10.0f), juce::Justification::centred);

        const float grNorm = juce::jlimit(0.0f, 1.0f, -compGrSmoothedDb_ / 24.0f);
        const float grPeakNorm = juce::jlimit(0.0f, 1.0f, compGrPeakHoldDb_ / 24.0f);
        auto grBar = plot.removeFromRight(18.0f).reduced(2.0f, 8.0f);
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(grBar, 2.0f);
        g.setColour(palette::kAccentWarm.withAlpha(0.35f));
        g.fillRoundedRectangle(grBar.getX(), grBar.getY(), grBar.getWidth(), grBar.getHeight() * grPeakNorm, 2.0f);
        g.setColour(palette::kAccentWarm);
        g.fillRoundedRectangle(grBar.getX(), grBar.getY(), grBar.getWidth(), grBar.getHeight() * grNorm, 2.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("GR", grBar.translated(0.0f, -10.0f), juce::Justification::centred);
        g.drawText(juce::String(compGrSmoothedDb_, 1), grBar.translated(0.0f, grBar.getHeight() + 2.0f),
                   juce::Justification::centred);

        const float kneeX = plot.getX() + threshNorm * plot.getWidth();
        g.setColour(palette::kAccentWarm.withAlpha(0.65f));
        g.fillEllipse(kneeX - 3.0f, plot.getBottom() - threshNorm * plot.getHeight() * 0.88f - 3.0f, 6.0f, 6.0f);
        paintHeroValueReadout(g, plot.removeFromBottom(14.0f).reduced(6.0f, 0.0f),
                              juce::String(compThresholdDb_, 1) + " dB · " + juce::String(compRatio_, 1) + ":1");
    }

    void DesignFxHeroViz::paintLimiterCeiling(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintFlatWaveform(
            g, plot, 56,
            [this](float t)
            {
                const float x = t * 2.0f - 1.0f;
                const float base =
                    std::sin(x * juce::MathConstants<float>::pi * 1.5f + limiterAnimPhase_) * 0.75f;
                const float transient =
                    std::sin(x * 12.0f + limiterAnimPhase_ * 2.3f) * 0.12f * limiterInputPeak_;
                return base + transient;
            },
            limiterClipping_);

        const float ceilingNorm = juce::jmap(limiterCeilingDb_, -12.0f, 0.0f, 0.35f, 0.08f);
        const float ceilingY = plot.getY() + plot.getHeight() * ceilingNorm;
        juce::Path ceiling;
        ceiling.startNewSubPath(plot.getX(), ceilingY);
        ceiling.lineTo(plot.getRight(), ceilingY);
        g.setColour(limiterClipping_ ? palette::kAccentWarm.brighter(0.25f)
                                     : palette::kAccentWarm.withAlpha(juce::jlimit(0.45f, 1.0f, mix_ + 0.35f)));
        g.strokePath(ceiling, juce::PathStrokeType(2.0f));

        const float peakNorm = juce::jlimit(0.0f, 1.0f, limiterInputPeak_ / 0.85f);
        const float holdNorm = juce::jlimit(0.0f, 1.0f, limiterPeakHold_ / 0.85f);
        auto pkBar = plot.removeFromRight(18.0f).reduced(2.0f, 8.0f);
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(pkBar, 2.0f);
        g.setColour(palette::kAccentWarm.withAlpha(0.35f));
        g.fillRoundedRectangle(pkBar.getX(), pkBar.getY(), pkBar.getWidth(), pkBar.getHeight() * holdNorm, 2.0f);
        g.setColour(limiterClipping_ ? palette::kAccentWarm.brighter(0.2f) : palette::kAccentWarm);
        g.fillRoundedRectangle(pkBar.getX(), pkBar.getY(), pkBar.getWidth(), pkBar.getHeight() * peakNorm, 2.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("PK", pkBar.translated(0.0f, -10.0f), juce::Justification::centred);

        g.setFont(fonts::label(8.0f));
        g.drawText(juce::String(limiterCeilingDb_, 1) + " dB",
                   juce::Rectangle<float>(plot.getRight() - 52.0f, ceilingY - 14.0f, 52.0f, 12.0f),
                   juce::Justification::centredRight);

        if (limiterClipping_)
        {
            g.setColour(palette::kAccentWarm.withAlpha(0.85f));
            g.setFont(fonts::label(8.0f));
            g.drawText("CLIP", plot.withTrimmedBottom(plot.getHeight() * 0.65f), juce::Justification::centred);
        }

        const bool truePeakOn = uiState_ != nullptr && uiState_->limTruePeakActive();
        limTruePeakToggleBounds_ = juce::Rectangle<float>(plot.getX() + 4.0f, plot.getY() + 2.0f, 118.0f, 14.0f);
        g.setColour(truePeakOn ? palette::kAccentWarm.withAlpha(0.22f) : palette::kPanel.withAlpha(0.85f));
        g.fillRoundedRectangle(limTruePeakToggleBounds_, 3.0f);
        g.setColour(truePeakOn ? palette::kAccentWarm : palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText(truePeakOn ? "TRUE PEAK ACTIVE" : "TRUE PEAK OFF", limTruePeakToggleBounds_,
                   juce::Justification::centred);
    }

    void DesignFxHeroViz::paintVocoderSpectrum(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        const int bands = juce::jlimit(4, 32, vocoderBands_);
        const float barW = plot.getWidth() / static_cast<float>(bands + 1);
        const float formantCenter = juce::jlimit(0.0f, 1.0f, vocoderFormant_);
        const float sibilance = juce::jlimit(0.0f, 1.0f, vocoderSibilance_);
        const float phase =
            static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.001 * (0.8f + mix_ * 0.6f));

        for (int i = 0; i < bands; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(juce::jmax(1, bands - 1));
            const float formantBump = std::exp(-std::pow((t - formantCenter) / 0.18f, 2.0f));
            const float sibBoost = sibilance * std::exp(-std::pow((t - 0.82f) / 0.12f, 2.0f));
            const float motion = 0.08f * std::sin(phase * 3.2f + static_cast<float>(i) * 0.55f);
            const float h = plot.getHeight() * (0.12f + formantBump * 0.62f + sibBoost * 0.45f + mix_ * 0.12f + motion);
            const float x = plot.getX() + (static_cast<float>(i) + 0.5f) * barW;
            g.setColour(palette::kAccent.withAlpha(0.28f + formantBump * 0.55f + sibBoost * 0.25f));
            g.fillRect(x - barW * 0.32f, plot.getBottom() - h, barW * 0.64f, h);
        }

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(juce::String(bands) + " BANDS · SC MOD",
                   plot.removeFromBottom(14.0f), juce::Justification::centredRight);
    }

    void DesignFxHeroViz::paintCloudsGranular(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        paintPlotGrid(g, plot);
        preview::paintCloudsAnimation(g, plot, mix_, static_cast<int>(cloudsDensity_ * 1000.0f), heroAnimPhase_);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        static constexpr const char* kModes[] = {"GRANULAR", "PITCH-SHIFT", "REVERB"};
        const juce::String modeLabel = kModes[juce::jlimit(0, 2, cloudsMode_)];
        g.drawText(modeLabel + " · pitch " + juce::String(cloudsPitch_, 2), plot.reduced(8.0f),
                   juce::Justification::bottomLeft);
        if (cloudsFreeze_ > 0.05f)
        {
            g.setColour(palette::kAccentWarm);
            g.drawText("FREEZE", plot.reduced(8.0f), juce::Justification::topRight);
        }
    }

    void DesignFxHeroViz::paintBypassHero(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        juce::Path passthrough;
        passthrough.startNewSubPath(plot.getX() + 8.0f, plot.getCentreY());
        passthrough.lineTo(plot.getRight() - 8.0f, plot.getCentreY());
        strokeGlowPath(g, passthrough, 0.45f, 2.0f, false);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(10.0f));
        g.drawText("DRY PASSTHROUGH", plot, juce::Justification::centred);
    }

    void DesignFxHeroViz::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(palette::kFigmaFxChipFill.withAlpha(0.35f));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(palette::kFigmaFxChipBorder.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        auto inner = bounds.reduced(12.0f, 10.0f);
        g.setColour(palette::kFigmaFxMutedText);
        g.setFont(fonts::micro(9.0f));
        const auto& spec = fxDesignSpecForChip(chipIndex_);
        g.drawText(spec.vizTitle, inner.removeFromTop(14.0f), juce::Justification::centredLeft);
        inner.removeFromTop(6.0f);

        juce::Rectangle<float> paramStrip;
        if (chipIndex_ == 8)
        {
            paramStrip = inner.removeFromBottom(static_cast<float>(layout::kDesignFxPageEqParamStripHeight));
            inner.removeFromBottom(8.0f);
        }

        auto plot = inner.reduced(4.0f, 2.0f);
        plotBounds_ = plot.toNearestInt();
        eqPlotBounds_ = chipIndex_ == 8 ? plot : juce::Rectangle<float>();

        g.setColour(palette::kFigmaFxVizFill.withAlpha(0.85f));
        g.fillRoundedRectangle(plot, 4.0f);

        if (effectType_ == 12)
        {
            paintCloudsGranular(g, plot);
            return;
        }

        if (effectType_ == 0 && chipIndex_ != 0 && chipIndex_ != 4)
        {
            paintBypassHero(g, plot);
            return;
        }

        switch (chipIndex_)
        {
            case 0: paintBypassHero(g, plot); break;
            case 1: paintSaturationTransfer(g, plot); break;
            case 2: paintChorusSpatializer(g, plot); break;
            case 3: paintTapeDrift(g, plot); break;
            case 4: paintMoodResponse(g, plot); break;
            case 5: paintFreqShiftBode(g, plot); break;
            case 6: paintFractalCloud(g, plot); break;
            case 7: paintReverbDecay(g, plot); break;
            case 8: paintEqCurve(g, plot); break;
            case 9: paintCompressorDynamics(g, plot); break;
            case 10: paintLimiterCeiling(g, plot); break;
            case 11: paintVocoderSpectrum(g, plot); break;
            default: paintBypassHero(g, plot); break;
        }

        if (chipIndex_ == 8 && !paramStrip.isEmpty())
            paintEqParamStrip(g, paramStrip);
    }

} // namespace pw8::plugin::ui::wireframe
