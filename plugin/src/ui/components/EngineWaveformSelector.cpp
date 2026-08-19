#include "EngineWaveformSelector.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"
#include "wireframe/WireframeProjection.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] const char* waveformLabel(int ordinal) noexcept
        {
            switch (ordinal)
            {
                case 0: return "SIN";
                case 1: return "TRI";
                case 2: return "SAW";
                default: return "SQR";
            }
        }

        [[nodiscard]] bool loadBool(juce::AudioProcessorValueTreeState& apvts, const juce::String& id) noexcept
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load() >= 0.5f;
            return false;
        }
    } // namespace

    EngineWaveformSelector::EngineWaveformSelector(MurmurProcessor& processor, int engineIndex)
        : processor_(processor), engineIndex_(engineIndex)
    {
        startTimerHz(30);
        refreshPreviews();
    }

    bool EngineWaveformSelector::isEngineLive() const
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (!loadBool(processor_.apvts, operatorMixParamId(idx, "MixEnabled")))
            return false;
        if (loadBool(processor_.apvts, operatorMixParamId(idx, "MixMute")))
            return false;

        bool anySolo = false;
        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            if (loadBool(processor_.apvts, operatorMixParamId(op, "MixSolo")))
            {
                anySolo = true;
                break;
            }
        }
        if (anySolo && !loadBool(processor_.apvts, operatorMixParamId(idx, "MixSolo")))
            return false;
        return true;
    }

    void EngineWaveformSelector::refreshPreviews()
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (auto* raw = processor_.apvts.getRawParameterValue(operatorParamId(idx, "Waveform")))
            activeWaveform_ = juce::jlimit(0, 3, static_cast<int>(raw->load() + 0.5f));

        float pulseWidth = 0.5f;
        if (auto* pw = processor_.apvts.getRawParameterValue(operatorParamId(idx, "PulseWidth")))
            pulseWidth = pw->load();

        for (int w = 0; w < 4; ++w)
        {
            wireframe::ClassicPreviewParams p;
            p.waveform = wireframe::OscPreviewSampler::waveformFromOrdinal(w);
            p.pulseWidth = pulseWidth;
            wireframe::OscPreviewSampler::sampleClassicCycle(p, previews_[static_cast<std::size_t>(w)]);
        }
    }

    void EngineWaveformSelector::advanceAnimation()
    {
        engineLive_ = isEngineLive();

        if (!engineLive_)
        {
            motionGain_ = 1.0f;
            return;
        }

        const auto idx = static_cast<std::size_t>(engineIndex_);
        float ratio = 1.0f;
        if (auto* raw = processor_.apvts.getRawParameterValue(operatorParamId(idx, "FreqRatio")))
            ratio = juce::jlimit(0.25f, 8.0f, raw->load());

        animPhase_ += 0.014f * ratio;
        if (animPhase_ >= 1.0f)
            animPhase_ -= 1.0f;

        std::array<float, 256> scope{};
        const int pulled = processor_.readScopeSamples(scope.data(), static_cast<int>(scope.size()));
        if (pulled > 8)
        {
            double sumSq = 0.0;
            for (int i = 0; i < pulled; ++i)
                sumSq += static_cast<double>(scope[static_cast<std::size_t>(i)])
                         * static_cast<double>(scope[static_cast<std::size_t>(i)]);
            const float rms = static_cast<float>(std::sqrt(sumSq / static_cast<double>(pulled)));
            motionGain_ = juce::jlimit(0.82f, 1.18f, 0.88f + rms * 2.4f);
        }
        else
            motionGain_ = 0.92f;
    }

    float EngineWaveformSelector::previewSample(const std::array<float, wireframe::kPreviewPoints>& buf, float t,
                                                float phaseOffset, float amp) const
    {
        float u = t + phaseOffset;
        u -= std::floor(u);
        const float f = u * static_cast<float>(wireframe::kPreviewPoints - 1);
        const int i0 = juce::jlimit(0, wireframe::kPreviewPoints - 1, static_cast<int>(f));
        const int i1 = (i0 + 1) % wireframe::kPreviewPoints;
        const float frac = f - static_cast<float>(i0);
        const float sample = buf[static_cast<std::size_t>(i0)] * (1.0f - frac) + buf[static_cast<std::size_t>(i1)] * frac;
        return juce::jlimit(-1.2f, 1.2f, sample * amp);
    }

    void EngineWaveformSelector::timerCallback()
    {
        if (++previewRefreshCounter_ >= 3)
        {
            previewRefreshCounter_ = 0;
            refreshPreviews();
        }
        advanceAnimation();
        repaint();
    }

    void EngineWaveformSelector::setWaveformOrdinal(int ordinal)
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (auto* param = processor_.apvts.getParameter(operatorParamId(idx, "Waveform")))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(juce::jlimit(0, 3, ordinal))));
        refreshPreviews();
    }

    juce::Rectangle<float> EngineWaveformSelector::cellBounds(int index) const
    {
        if (index >= 0 && index < 4)
            return cellLayout_[static_cast<std::size_t>(index)].toFloat();
        return {};
    }

    int EngineWaveformSelector::cellIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < 4; ++i)
        {
            if (cellLayout_[static_cast<std::size_t>(i)].contains(pos))
                return i;
        }
        return -1;
    }

    void EngineWaveformSelector::mouseDown(const juce::MouseEvent& event)
    {
        if (const int cell = cellIndexAt(event.getPosition()); cell >= 0)
            setWaveformOrdinal(cell);
    }

    void EngineWaveformSelector::paint(juce::Graphics& g)
    {
        for (int i = 0; i < 4; ++i)
        {
            const auto cell = cellBounds(i).reduced(1.5f);
            const bool selected = i == activeWaveform_;
            const float cellPhase = engineLive_ ? animPhase_ * (selected ? 1.0f : 0.35f) : 0.0f;
            const float cellAmp = engineLive_ ? (selected ? motionGain_ : 0.78f) : 1.0f;

            draw::fillRecessedRoundedRect(g, cell, 4.0f);
            g.setColour(selected ? palette::kAccent.withAlpha(0.22f) : palette::kPanel.withAlpha(0.45f));
            g.fillRoundedRectangle(cell, 4.0f);

            if (selected)
                draw::strokeGlowPath(g, draw::roundedRectPath(cell.reduced(0.5f), 4.0f), engineLive_ ? 1.0f : 0.95f, 1.4f,
                                     true);
            else
            {
                g.setColour(palette::kBorder.withAlpha(0.55f));
                g.drawRoundedRectangle(cell, 4.0f, 1.0f);
            }

            const auto waveArea = cell.reduced(4.0f, 8.0f);
            const auto& samples = previews_[static_cast<std::size_t>(i)];
            wireframe::paintFlatWaveform(
                g, waveArea, wireframe::kPreviewPoints,
                [&](float t) { return previewSample(samples, t, cellPhase, cellAmp); },
                selected && engineLive_);

            g.setColour(selected ? palette::kAccent : palette::kTextDim);
            g.setFont(juce::Font(juce::FontOptions(8.0f)));
            const auto labelArea = cell.withTrimmedBottom(0.0f).withHeight(10.0f).withBottom(cell.getBottom());
            g.drawText(waveformLabel(i), labelArea, juce::Justification::centred);
        }
    }

    void EngineWaveformSelector::resized()
    {
        auto area = getLocalBounds().reduced(1);
        const int halfW = area.getWidth() / 2;
        const int halfH = area.getHeight() / 2;
        cellLayout_[0] = {area.getX(), area.getY(), halfW, halfH};
        cellLayout_[1] = {area.getX() + halfW, area.getY(), halfW, halfH};
        cellLayout_[2] = {area.getX(), area.getY() + halfH, halfW, halfH};
        cellLayout_[3] = {area.getX() + halfW, area.getY() + halfH, halfW, halfH};
    }

} // namespace pw8::plugin::ui
