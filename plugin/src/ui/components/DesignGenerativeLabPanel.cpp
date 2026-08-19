#include "DesignGenerativeLabPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    DesignGenerativeLabPanel::DesignGenerativeLabPanel(MurmurProcessor& processor) : processor_(processor)
    {
        titleLabel_.setText("GENERATIVE LAB", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("Marbles-style T/X random, deja-vu, seed locker", juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(10.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(subtitleLabel_);

        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        dejaVuButton_.setClickingTogglesState(true);
        seedLockButton_.setClickingTogglesState(true);
        addAndMakeVisible(dejaVuButton_);
        addAndMakeVisible(seedLockButton_);
        dejaVuAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor_.apvts, kGenerativeDejaVuId, dejaVuButton_);
        seedLockAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor_.apvts, kGenerativeSeedLockedId, seedLockButton_);

        const juce::String prefix = kGenerativeIdPrefix;
        tRateKnob_ = std::make_unique<GlowKnob>(processor_.apvts, prefix + "ClockTRateHz", "T RATE", nullptr,
                                                palette::kFigmaTeal);
        xRateKnob_ = std::make_unique<GlowKnob>(processor_.apvts, prefix + "ClockXRateHz", "X RATE", nullptr,
                                                palette::kAccentWarm);
        correlationKnob_ = std::make_unique<GlowKnob>(processor_.apvts, prefix + "Correlation", "CORR", nullptr,
                                                      palette::kMurmurViolet);

        const char* stream0Suffix[] = {"Stream0Spread", "Stream0Bias", "Stream0LagMs"};
        const char* stream0Labels[] = {"SPREAD", "BIAS", "LAG"};
        for (std::size_t i = 0; i < stream0Knobs_.size(); ++i)
        {
            stream0Knobs_[i] = std::make_unique<GlowKnob>(processor_.apvts, prefix + stream0Suffix[i], stream0Labels[i],
                                                          nullptr, palette::kAccent);
            stream0Knobs_[i]->applyFigmaContext(figma::KnobContext::DesignLabGrid);
            addAndMakeVisible(*stream0Knobs_[i]);
        }

        const char* stream1Suffix[] = {"Stream1Spread", "Stream1Bias", "Stream1LagMs"};
        const char* stream1Labels[] = {"SPREAD", "BIAS", "LAG"};
        for (std::size_t i = 0; i < stream1Knobs_.size(); ++i)
        {
            stream1Knobs_[i] = std::make_unique<GlowKnob>(processor_.apvts, prefix + stream1Suffix[i], stream1Labels[i],
                                                          nullptr, palette::kAccentWarm);
            stream1Knobs_[i]->applyFigmaContext(figma::KnobContext::DesignLabGrid);
            addAndMakeVisible(*stream1Knobs_[i]);
        }

        for (auto* k : {tRateKnob_.get(), xRateKnob_.get(), correlationKnob_.get()})
        {
            k->applyFigmaContext(figma::KnobContext::DesignLabGrid);
            addAndMakeVisible(*k);
        }

        openModMatrixButton_.onClick = [this] {
            if (onOpenModMatrix)
                onOpenModMatrix();
        };
        addAndMakeVisible(openModMatrixButton_);

        footerHint_.setText("T = slow clock" + juce::String(fonts::kSep) + "X = fast clock" + juce::String(fonts::kSep)
                                + "route RANDOM sources in MOD MATRIX",
                            juce::dontSendNotification);
        footerHint_.setFont(fonts::micro(9.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        addAndMakeVisible(clockHeroPanel_);
        addAndMakeVisible(routingPanel_);

        startTimerHz(20);
    }

    DesignGenerativeLabPanel::~DesignGenerativeLabPanel() { stopTimer(); }

    void DesignGenerativeLabPanel::showOverlay() { setVisible(true); }

    void DesignGenerativeLabPanel::dismiss() { setVisible(false); }

    void DesignGenerativeLabPanel::setEmbeddedInDesignMode(bool embedded)
    {
        embeddedInDesignMode_ = embedded;
        backButton_.setVisible(!embedded);
    }

    void DesignGenerativeLabPanel::refreshFromPatch() { repaint(); }

    void DesignGenerativeLabPanel::timerCallback()
    {
        if (auto* t = processor_.apvts.getRawParameterValue(juce::String(kGenerativeIdPrefix) + "ClockTRateHz"))
            vizT_ += t->load() * 0.02f;
        if (auto* x = processor_.apvts.getRawParameterValue(juce::String(kGenerativeIdPrefix) + "ClockXRateHz"))
            vizX_ += x->load() * 0.08f;
        if (vizT_ > 1.0f)
            vizT_ -= 1.0f;
        if (vizX_ > 1.0f)
            vizX_ -= 1.0f;
        repaint(clockHeroBounds_);
        repaint(routingBounds_);
    }

    void DesignGenerativeLabPanel::paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        draw::fillRecessedRoundedRect(g, bounds.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
    }

    void DesignGenerativeLabPanel::paintClockHero(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        if (bounds.isEmpty())
            return;

        paintPanelCard(g, bounds);
        auto plot = bounds.reduced(14, 12).toFloat();

        const float tRate =
            processor_.apvts.getRawParameterValue(juce::String(kGenerativeIdPrefix) + "ClockTRateHz") != nullptr
                ? processor_.apvts.getRawParameterValue(juce::String(kGenerativeIdPrefix) + "ClockTRateHz")->load()
                : 1.0f;
        const float xRate =
            processor_.apvts.getRawParameterValue(juce::String(kGenerativeIdPrefix) + "ClockXRateHz") != nullptr
                ? processor_.apvts.getRawParameterValue(juce::String(kGenerativeIdPrefix) + "ClockXRateHz")->load()
                : 4.0f;

        auto header = plot.removeFromTop(12.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::micro(9.0f));
        g.drawText("T / X STOCHASTIC CLOCKS", header, juce::Justification::centredLeft);

        auto rateRow = plot.removeFromTop(10.0f);
        g.setColour(palette::kTextSecondary);
        g.drawText("T " + juce::String(tRate, 2) + " Hz", rateRow.removeFromLeft(rateRow.getWidth() * 0.5f),
                   juce::Justification::centredLeft);
        g.drawText("X " + juce::String(xRate, 2) + " Hz", rateRow, juce::Justification::centredRight);
        plot.removeFromTop(4.0f);

        const float tMidY = plot.getCentreY() - 14.0f;
        const float xMidY = plot.getCentreY() + 14.0f;

        auto drawTrack = [&](float midY, juce::Colour colour, float phase, int tickCount) {
            g.setColour(palette::kBorder.withAlpha(0.35f));
            g.drawHorizontalLine(static_cast<int>(midY), plot.getX(), plot.getRight());
            for (int i = 0; i <= tickCount; ++i)
            {
                const float x = plot.getX() + (static_cast<float>(i) / static_cast<float>(tickCount)) * plot.getWidth();
                g.drawVerticalLine(static_cast<int>(x), static_cast<int>(midY - 3.0f), static_cast<int>(midY + 3.0f));
            }

            juce::Path pulse;
            const int steps = 48;
            for (int i = 0; i <= steps; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(steps);
                const float wave = 0.5f + 0.5f * std::sin((t + phase) * juce::MathConstants<float>::twoPi * 2.0f);
                const float x = plot.getX() + t * plot.getWidth();
                const float y = midY - wave * 8.0f;
                if (i == 0)
                    pulse.startNewSubPath(x, y);
                else
                    pulse.lineTo(x, y);
            }
            g.setColour(colour.withAlpha(0.35f));
            g.strokePath(pulse, juce::PathStrokeType(1.5f));

            const float headX = plot.getX() + phase * plot.getWidth();
            g.setColour(colour.withAlpha(0.25f));
            g.fillEllipse(headX - 7.0f, midY - 7.0f, 14.0f, 14.0f);
            g.setColour(colour);
            g.fillEllipse(headX - 3.5f, midY - 3.5f, 7.0f, 7.0f);
        };

        drawTrack(tMidY, palette::kFigmaTeal, vizT_, 4);
        drawTrack(xMidY, palette::kAccentWarm, vizX_, 8);
    }

    void DesignGenerativeLabPanel::paintRoutingDiagram(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        if (bounds.isEmpty())
            return;

        paintPanelCard(g, bounds);
        auto inner = bounds.reduced(12, 10);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::micro(9.0f));
        g.drawText("6 OUTPUT BUS" + juce::String(fonts::kSep) + "ROUTE IN MOD MATRIX", inner.removeFromTop(12),
                   juce::Justification::centredLeft);
        inner.removeFromTop(8);

        static constexpr const char* kBusLabels[] = {"T", "X", "S1", "S2", "S3", "S4"};
        const int chipW = juce::jmax(36, (inner.getWidth() - 5 * 6) / 6);
        int x = inner.getX();
        const int chipY = inner.getY();
        for (int i = 0; i < 6; ++i)
        {
            auto chip = juce::Rectangle<int>(x, chipY, chipW, 22);
            const bool accent = i < 2;
            g.setColour(accent ? palette::kFigmaTeal.withAlpha(0.12f) : palette::kPanelRaised);
            g.fillRoundedRectangle(chip.toFloat(), 4.0f);
            g.setColour(accent ? palette::kFigmaTeal.withAlpha(0.85f) : palette::kAccent.withAlpha(0.55f));
            g.drawRoundedRectangle(chip.toFloat(), 4.0f, 1.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::micro(8.5f));
            g.drawText(kBusLabels[i], chip, juce::Justification::centred);
            if (i < 5)
            {
                g.setColour(palette::kTextDim);
                g.setFont(fonts::micro(7.5f));
                g.drawText(fonts::kArrow, juce::Rectangle<int>(x + chipW, chipY, 6, 22), juce::Justification::centred);
            }
            x += chipW + 6;
        }

        auto hintRow = inner.withTrimmedTop(30);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::micro(8.0f));
        g.drawText("Stream outputs feed RANDOM mod sources (DEJA-VU, T, X clocks)", hintRow.removeFromTop(12),
                   juce::Justification::centredLeft);
    }

    void DesignGenerativeLabPanel::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop);
    }

    void DesignGenerativeLabPanel::paintOverChildren(juce::Graphics& g)
    {
        paintClockHero(g, clockHeroBounds_);
        paintRoutingDiagram(g, routingBounds_);
    }

    void DesignGenerativeLabPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDesignGenerativeLabPageOuterMargin);
        auto header = bounds.removeFromTop(layout::kDesignGenerativeLabHeaderHeight);
        backButton_.setBounds(header.removeFromLeft(90).reduced(0, 8));
        titleLabel_.setBounds(header.removeFromTop(18));
        subtitleLabel_.setBounds(header.removeFromTop(14));

        bounds.removeFromBottom(layout::kDesignGenerativeLabFooterHeight);
        footerHint_.setBounds(getLocalBounds().removeFromBottom(layout::kDesignGenerativeLabFooterHeight).reduced(
            layout::kDesignGenerativeLabPageOuterMargin, 8));
        openModMatrixButton_.setBounds(footerHint_.getBounds().removeFromRight(160).reduced(0, 4));

        auto topRow = bounds.removeFromTop(36);
        dejaVuButton_.setBounds(topRow.removeFromLeft(90).reduced(0, 4));
        seedLockButton_.setBounds(topRow.removeFromLeft(90).reduced(4, 4));

        auto knobRow = bounds.removeFromTop(88);
        const int knobW = layout::kDesignGenerativeLabKnobSize + 12;
        tRateKnob_->setBounds(knobRow.removeFromLeft(knobW));
        xRateKnob_->setBounds(knobRow.removeFromLeft(knobW));
        correlationKnob_->setBounds(knobRow.removeFromLeft(knobW));

        bounds.removeFromTop(layout::kDesignGenerativeLabPageSectionGap);
        auto hero = bounds.removeFromTop(120);
        clockHeroPanel_.setBounds(hero);
        clockHeroBounds_ = hero.reduced(8, 24);

        bounds.removeFromTop(layout::kDesignGenerativeLabPageSectionGap);
        auto streamRow = bounds.removeFromTop(100);
        const int third = streamRow.getWidth() / 6;
        for (auto& k : stream0Knobs_)
            k->setBounds(streamRow.removeFromLeft(third).reduced(2));
        for (auto& k : stream1Knobs_)
            k->setBounds(streamRow.removeFromLeft(third).reduced(2));

        routingPanel_.setBounds(bounds);
        routingBounds_ = bounds.reduced(8, 24);
    }

    bool DesignGenerativeLabPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
