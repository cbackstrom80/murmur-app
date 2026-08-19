#include "QuasarEngineCard.h"

#include "../../theme/FigmaKnobTokens.h"
#include "../../theme/ObsidianDraw.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        void styleSectionChip(juce::Label& label, const juce::String& text)
        {
            label.setText(text, juce::dontSendNotification);
            label.setFont(fonts::label(7.0f));
            label.setColour(juce::Label::textColourId, palette::kTextDim);
            label.setJustificationType(juce::Justification::centredLeft);
        }
    } // namespace

    QuasarEngineCard::QuasarEngineCard(MurmurProcessor& processor,
                                       juce::AudioProcessorValueTreeState& apvts)
        : processor_(processor), apvts_(apvts), panel_("BINAURAL ENGINE", juce::Colour(0xff00c8ff))
    {
        addAndMakeVisible(panel_);

        styleSectionChip(hrtfHeader_, "HRTF PROFILE");
        styleSectionChip(modeHeader_, "OUTPUT MODE");
        panel_.addAndMakeVisible(hrtfHeader_);
        panel_.addAndMakeVisible(modeHeader_);

        static constexpr const char* kHrtfLabels[] = {"KEMAR", "CIPIC", "CUSTOM"};
        static constexpr float kHrtfSizes[] = {0.85f, 1.0f, 1.25f};
        for (std::size_t i = 0; i < hrtfPills_.size(); ++i)
        {
            stylePill(hrtfPills_[i], kHrtfLabels[i]);
            hrtfPills_[i].onClick = [this, size = kHrtfSizes[i]] {
                setParam("Qsr1RoomSize", size);
                refreshPills();
            };
            panel_.addAndMakeVisible(hrtfPills_[i]);
        }

        static constexpr const char* kModeLabels[] = {"HEADPHONE", "SPEAKER", "MONITOR"};
        for (std::size_t i = 0; i < modePills_.size(); ++i)
        {
            stylePill(modePills_[i], kModeLabels[i]);
            modePills_[i].onClick = [this, i] {
                setIntParam("QuasarOutputMode", static_cast<int>(i));
                refreshPills();
            };
            panel_.addAndMakeVisible(modePills_[i]);
        }

        correlationLabel_.setFont(fonts::label(7.0f));
        correlationLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        correlationLabel_.setJustificationType(juce::Justification::centredLeft);
        correlationLabel_.setText("PHASE CORRELATION", juce::dontSendNotification);
        panel_.addAndMakeVisible(correlationLabel_);

        bindSlot(5);
    }

    juce::String QuasarEngineCard::slotParamPrefix() const
    {
        const std::size_t local = slotIndex_ >= 3 ? slotIndex_ - 3 : 0;
        return masterFxParamId(local, "");
    }

    float QuasarEngineCard::readParam(const juce::String& suffix, float fallback) const
    {
        if (auto* raw = apvts_.getRawParameterValue(slotParamPrefix() + suffix))
            return raw->load();
        return fallback;
    }

    void QuasarEngineCard::setParam(const juce::String& suffix, float value)
    {
        if (auto* param = apvts_.getParameter(slotParamPrefix() + suffix))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    void QuasarEngineCard::setIntParam(const juce::String& suffix, int value)
    {
        if (auto* param = apvts_.getParameter(slotParamPrefix() + suffix))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(value)));
    }

    void QuasarEngineCard::stylePill(juce::TextButton& btn, const juce::String& text)
    {
        btn.setButtonText(text);
        btn.setClickingTogglesState(false);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
    }

    void QuasarEngineCard::highlightPill(juce::TextButton& btn, bool active, juce::Colour accent)
    {
        btn.setColour(juce::TextButton::buttonColourId, active ? accent.withAlpha(0.12f) : juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, active ? accent : palette::kTextDim);
    }

    void QuasarEngineCard::bindSlot(std::size_t globalFxSlotIndex)
    {
        slotIndex_ = juce::jlimit<std::size_t>(3, 6, globalFxSlotIndex);
        monoBelowKnob_.reset();
        monoBelowKnob_ = std::make_unique<GlowKnob>(apvts_, slotParamPrefix() + "CntrHpfHz", "MONO BELOW");
        monoBelowKnob_->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        panel_.addAndMakeVisible(*monoBelowKnob_);
        refreshPills();
        resized();
    }

    void QuasarEngineCard::refresh() { refreshPills(); }

    void QuasarEngineCard::refreshPills()
    {
        const float roomSize = readParam("Qsr1RoomSize", 1.0f);
        highlightPill(hrtfPills_[0], std::abs(roomSize - 0.85f) < 0.06f, juce::Colour(0xff00c8ff));
        highlightPill(hrtfPills_[1], std::abs(roomSize - 1.0f) < 0.06f, juce::Colour(0xff00c8ff));
        highlightPill(hrtfPills_[2], std::abs(roomSize - 1.25f) < 0.06f, juce::Colour(0xff00c8ff));

        const int mode = static_cast<int>(readParam("QuasarOutputMode", 0.0f) + 0.5f);
        for (std::size_t i = 0; i < modePills_.size(); ++i)
            highlightPill(modePills_[i], static_cast<int>(i) == mode, juce::Colour(0xff7c4dff));
    }

    void QuasarEngineCard::updateCorrelationFromScope()
    {
        const int pulled =
            processor_.readScopeSamples(scopeLeft_.data(), static_cast<int>(scopeLeft_.size()));
        if (pulled < 8)
        {
            correlationDisplay_ = 1.0f - readParam("QuasarCrossfeed", 0.0f) * 0.5f;
            return;
        }

        for (int i = 0; i < pulled; ++i)
        {
            scopeRight_[static_cast<std::size_t>(i)] =
                (i + 1 < pulled) ? scopeLeft_[static_cast<std::size_t>(i + 1)] : scopeLeft_[static_cast<std::size_t>(i)];
        }

        double sumL = 0.0;
        double sumR = 0.0;
        double sumLR = 0.0;
        double sumL2 = 0.0;
        double sumR2 = 0.0;
        for (int i = 0; i < pulled; ++i)
        {
            const double l = scopeLeft_[static_cast<std::size_t>(i)];
            const double r = scopeRight_[static_cast<std::size_t>(i)];
            sumL += l;
            sumR += r;
            sumLR += l * r;
            sumL2 += l * l;
            sumR2 += r * r;
        }

        const double n = static_cast<double>(pulled);
        const double denom = std::sqrt((n * sumL2 - sumL * sumL) * (n * sumR2 - sumR * sumR));
        correlationDisplay_ = denom > 1.0e-9 ? static_cast<float>((n * sumLR - sumL * sumR) / denom) : 1.0f;
        correlationDisplay_ = juce::jlimit(-1.0f, 1.0f, correlationDisplay_);
    }

    void QuasarEngineCard::tick()
    {
        updateCorrelationFromScope();
        repaint(correlationBounds_);
    }

    void QuasarEngineCard::paintOverChildren(juce::Graphics& g)
    {
        if (correlationBounds_.isEmpty())
            return;

        auto bar = correlationBounds_.toFloat().reduced(0.0f, 2.0f);
        draw::fillRecessedRoundedRect(g, bar, 4.0f);

        const float midX = bar.getCentreX();
        g.setColour(palette::kBorder.withAlpha(0.6f));
        g.drawVerticalLine(static_cast<int>(midX), bar.getY() + 2.0f, bar.getBottom() - 2.0f);

        const float fillW = bar.getWidth() * 0.5f * std::abs(correlationDisplay_);
        juce::Rectangle<float> fill;
        if (correlationDisplay_ >= 0.0f)
            fill = {midX, bar.getY() + 3.0f, fillW, bar.getHeight() - 6.0f};
        else
            fill = {midX - fillW, bar.getY() + 3.0f, fillW, bar.getHeight() - 6.0f};

        g.setColour(correlationDisplay_ >= 0.0f ? juce::Colour(0xff00c8ff).withAlpha(0.85f)
                                                : juce::Colour(0xffe040fb).withAlpha(0.85f));
        g.fillRoundedRectangle(fill, 2.0f);

        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextSecondary);
        g.drawText(juce::String(correlationDisplay_, 2), bar, juce::Justification::centred, true);
    }

    void QuasarEngineCard::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds().reduced(8, 6);

        auto topRow = content.removeFromTop(52);
        hrtfHeader_.setBounds(topRow.removeFromTop(12));
        const int chipW = juce::jmax(52, (topRow.getWidth() - 8) / 3);
        for (auto& pill : hrtfPills_)
        {
            pill.setBounds(topRow.removeFromLeft(chipW).reduced(0, 2));
            topRow.removeFromLeft(4);
        }

        content.removeFromTop(8);
        auto modeRow = content.removeFromTop(52);
        modeHeader_.setBounds(modeRow.removeFromTop(12));
        for (auto& pill : modePills_)
        {
            pill.setBounds(modeRow.removeFromLeft(chipW).reduced(0, 2));
            modeRow.removeFromLeft(4);
        }

        content.removeFromTop(8);
        auto bottom = content;
        if (monoBelowKnob_ != nullptr)
            monoBelowKnob_->setBounds(bottom.removeFromLeft(88).removeFromTop(88));
        bottom.removeFromLeft(12);
        correlationLabel_.setBounds(bottom.removeFromTop(12));
        correlationBounds_ = bottom.removeFromTop(18);
    }

} // namespace pw8::plugin::ui
