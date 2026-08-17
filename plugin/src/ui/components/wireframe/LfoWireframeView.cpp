#include "LfoWireframeView.h"

#include "../../theme/ObsidianPalette.h"
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
    } // namespace

    LfoWireframeView::LfoWireframeView(juce::AudioProcessorValueTreeState& apvts, std::size_t lfoIndex)
        : apvts_(apvts), lfoIndex_(lfoIndex)
    {
        startTimerHz(12);
    }

    LfoWireframeView::~LfoWireframeView() { stopTimer(); }

    void LfoWireframeView::setLfoIndex(std::size_t lfoIndex)
    {
        lfoIndex_ = lfoIndex;
        repaint();
    }

    void LfoWireframeView::timerCallback()
    {
        const auto prefix = lfoParamId(lfoIndex_, "");
        waveform_ = static_cast<lfo::LfoWaveform>(static_cast<int>(loadParam(apvts_, prefix + "Waveform") + 0.5f));
        mode_ = static_cast<lfo::LfoMode>(static_cast<int>(loadParam(apvts_, prefix + "Mode") + 0.5f));
        rateHz_ = loadParam(apvts_, prefix + "RateHz", 2.0f);
        phaseOffset_ = loadParam(apvts_, prefix + "PhaseOffset");

        animPhase_ += rateHz_ * 0.012f;
        if (animPhase_ > 1.0f)
            animPhase_ -= 1.0f;

        setCaption(juce::String("LFO ") + juce::String(static_cast<int>(lfoIndex_ + 1)) + " · "
                   + lfoWaveformName(waveform_));
        setSubCaption(juce::String(lfoModeName(mode_)) + " · " + juce::String(rateHz_, 2) + " Hz");
        repaint();
    }

    void LfoWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);

        const int cycles = (mode_ == lfo::LfoMode::OneShot) ? 1 : 2;
        paintFlatWaveform(
            g, bounds, 96,
            [&](float t)
            {
                const float phase = std::fmod(t * static_cast<float>(cycles) + phaseOffset_ + animPhase_, 1.0f);
                switch (waveform_)
                {
                    case lfo::LfoWaveform::SampleHold:
                        return sampleLfoSampleHold(phase);
                    case lfo::LfoWaveform::SmoothRandom:
                        return sampleLfoSmoothRandom(phase, static_cast<int>(animPhase_ * 8.0f));
                    default:
                        return sampleLfoWaveform(waveform_, phase);
                }
            },
            true);

        if (mode_ == lfo::LfoMode::OneShot)
        {
            const float markerX = bounds.getX() + bounds.getWidth() / static_cast<float>(cycles);
            g.setColour(palette::kAccentWarm.withAlpha(0.75f));
            g.drawVerticalLine(static_cast<int>(markerX), bounds.getY(), bounds.getBottom());
        }
    }

} // namespace pw8::plugin::ui::wireframe
