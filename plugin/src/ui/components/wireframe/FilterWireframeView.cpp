#include "FilterWireframeView.h"

#include <cmath>

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
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
                case 0: // lowpass
                    return juce::jlimit(-1.0f, 1.0f, 1.0f - juce::jmax(0.0f, freqNorm - cutoffNorm) * (2.5f - resonance)
                                                     + bell);
                case 1: // highpass
                    return juce::jlimit(-1.0f, 1.0f, 1.0f - juce::jmax(0.0f, cutoffNorm - freqNorm) * (2.5f - resonance)
                                                     + bell);
                case 2: // bandpass
                    return juce::jlimit(-1.0f, 1.0f, bell * 2.2f - std::abs(d) * 3.5f);
                case 3: // notch
                    return juce::jlimit(-1.0f, 1.0f, 1.0f - bell * 2.5f - std::abs(d) * 0.5f);
                case 4: // peak
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

    void FilterWireframeView::setParamIds(const juce::String& enabledId, const juce::String& modeId,
                                           const juce::String& cutoffId, const juce::String& resonanceId)
    {
        enabledId_ = enabledId;
        modeId_ = modeId;
        cutoffId_ = cutoffId;
        resonanceId_ = resonanceId;
        timerCallback();
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
        repaint();
    }

    void FilterWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);

        if (!enabled_)
        {
            paintEmptyMessage(g, bounds, "Filter disabled");
            return;
        }

        const float cutoffNorm = freqToNorm(cutoffHz_);
        paintFlatWaveform(g, bounds, 64,
                          [&](float t)
                          {
                              return responseAt(t, mode_, cutoffNorm, resonance_);
                          },
                          true);

        const float markerX = bounds.getX() + cutoffNorm * bounds.getWidth();
        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.drawVerticalLine(static_cast<int>(markerX), bounds.getY(), bounds.getBottom());
        g.setFont(fonts::label(8.0f));
        g.drawText("cutoff", juce::Rectangle<float>(markerX + 3.0f, bounds.getY(), 40.0f, 12.0f),
                   juce::Justification::centredLeft);
    }

} // namespace pw8::plugin::ui::wireframe
