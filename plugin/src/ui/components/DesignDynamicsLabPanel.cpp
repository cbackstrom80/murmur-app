#include "DesignDynamicsLabPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        static constexpr const char* kModeLabels[] = {"ENV", "VACT", "FOLL", "COMP"};

        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float fallback)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }
    }

    DesignDynamicsLabPanel::DesignDynamicsLabPanel(MurmurProcessor& processor) : processor_(processor)
    {
        titleLabel_.setText("DYNAMICS LAB", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("Streams-style master bus dynamics", juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(10.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(subtitleLabel_);

        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        enableButton_.setClickingTogglesState(true);
        addAndMakeVisible(enableButton_);
        enabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor_.apvts, kMasterDynamicsEnabledId, enableButton_);

        for (std::size_t i = 0; i < modePills_.size(); ++i)
        {
            modePills_[i].setButtonText(kModeLabels[i]);
            modePills_[i].onClick = [this, i] { setDynamicsMode(static_cast<int>(i)); };
            addAndMakeVisible(modePills_[i]);
        }

        thresholdKnob_ = std::make_unique<GlowKnob>(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "ThresholdDb",
                                                    "THR", nullptr, palette::kAccent);
        ratioKnob_ = std::make_unique<GlowKnob>(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "Ratio", "RATIO",
                                                nullptr, palette::kAccent);
        attackKnob_ = std::make_unique<GlowKnob>(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "AttackMs",
                                                 "ATK", nullptr, palette::kAccentWarm);
        releaseKnob_ = std::make_unique<GlowKnob>(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "ReleaseMs",
                                                  "REL", nullptr, palette::kAccentWarm);
        sidechainKnob_ = std::make_unique<GlowKnob>(processor_.apvts,
                                                    juce::String(kMasterDynamicsIdPrefix) + "SidechainGain", "SC",
                                                    nullptr, palette::kFigmaTeal);
        mixKnob_ = std::make_unique<GlowKnob>(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "Mix", "MIX",
                                              nullptr, palette::kAccent);

        for (auto* k :
             {thresholdKnob_.get(), ratioKnob_.get(), attackKnob_.get(), releaseKnob_.get(), sidechainKnob_.get(),
              mixKnob_.get()})
        {
            k->applyFigmaContext(figma::KnobContext::DesignLabGrid);
            addAndMakeVisible(*k);
        }

        openPlayOutputButton_.onClick = [this] {
            if (onOpenPlayOutput)
                onOpenPlayOutput();
        };
        addAndMakeVisible(openPlayOutputButton_);

        footerHint_.setText("Voice sum" + juce::String(fonts::kArrow) + "Dynamics" + juce::String(fonts::kArrow)
                                + "Master FX" + juce::String(fonts::kArrow) + "Output",
                            juce::dontSendNotification);
        footerHint_.setFont(fonts::micro(9.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        addAndMakeVisible(heroPanel_);
        addAndMakeVisible(signalPanel_);

        refreshModePills();
        startTimerHz(20);
    }

    DesignDynamicsLabPanel::~DesignDynamicsLabPanel() { stopTimer(); }

    void DesignDynamicsLabPanel::showOverlay() { setVisible(true); }

    void DesignDynamicsLabPanel::dismiss() { setVisible(false); }

    void DesignDynamicsLabPanel::setEmbeddedInDesignMode(bool embedded)
    {
        embeddedInDesignMode_ = embedded;
        backButton_.setVisible(!embedded);
    }

    void DesignDynamicsLabPanel::refreshFromPatch() { refreshModePills(); }

    void DesignDynamicsLabPanel::setDynamicsMode(int modeIndex)
    {
        if (auto* param = processor_.apvts.getParameter(kMasterDynamicsModeId))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(juce::jlimit(0, 3, modeIndex))));
        refreshModePills();
    }

    void DesignDynamicsLabPanel::styleModePill(juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId, active ? palette::kAccent.withAlpha(0.22f) : juce::Colour(0xff12141a));
        btn.setColour(juce::TextButton::textColourOffId, active ? palette::kAccent : palette::kTextDim);
    }

    void DesignDynamicsLabPanel::refreshModePills()
    {
        int mode = 0;
        if (auto* raw = processor_.apvts.getRawParameterValue(kMasterDynamicsModeId))
            mode = static_cast<int>(raw->load());
        for (std::size_t i = 0; i < modePills_.size(); ++i)
            styleModePill(modePills_[i], static_cast<int>(i) == mode);
    }

    void DesignDynamicsLabPanel::timerCallback()
    {
        grDb_ = processor_.getMasterDynamicsGainReductionDb();
        sidechainEnv_ = processor_.getMasterDynamicsSidechainEnvelope();
        refreshModePills();
        repaint(transferPlotBounds_);
        repaint(signalPlotBounds_);
    }

    void DesignDynamicsLabPanel::paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        draw::fillRecessedRoundedRect(g, bounds.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.65f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
    }

    void DesignDynamicsLabPanel::paintTransferCurve(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        if (bounds.isEmpty())
            return;

        paintPanelCard(g, bounds);
        auto plot = bounds.reduced(12, 10).toFloat();

        const int mode = static_cast<int>(loadParam(processor_.apvts, kMasterDynamicsModeId, 0.0f));
        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextDim);
        g.drawText(kModeLabels[juce::jlimit(0, 3, mode)], plot.removeFromTop(12.0f), juce::Justification::centredLeft,
                   true);

        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawLine(plot.getX(), plot.getBottom(), plot.getRight(), plot.getBottom(), 1.0f);
        g.drawLine(plot.getX(), plot.getY(), plot.getX(), plot.getBottom(), 1.0f);

        const float sidechainGain =
            loadParam(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "SidechainGain", 1.0f);
        const float thresholdDb =
            loadParam(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "ThresholdDb", -12.0f);
        const float ratio = loadParam(processor_.apvts, juce::String(kMasterDynamicsIdPrefix) + "Ratio", 4.0f);

        juce::Path curve;
        const float w = plot.getWidth();
        const float h = plot.getHeight();
        for (int i = 0; i <= 64; ++i)
        {
            const float t = static_cast<float>(i) / 64.0f;
            float yNorm = 0.0f;
            switch (mode)
            {
                case 0:
                case 1:
                    yNorm = std::exp(-4.0f * (1.0f - t));
                    break;
                case 2:
                    yNorm = 1.0f - t * sidechainGain * 0.5f;
                    break;
                case 3:
                default:
                {
                    const float inputDb = juce::jmap(t, -48.0f, 0.0f);
                    float outputDb = inputDb;
                    const float overshoot = inputDb - thresholdDb;
                    constexpr float kneeDb = 6.0f;
                    float reductionDb = 0.0f;
                    if (kneeDb > 0.0f && std::abs(overshoot) < kneeDb / 2.0f)
                    {
                        const float x = overshoot + kneeDb / 2.0f;
                        reductionDb = (1.0f / std::max(ratio, 1.0f) - 1.0f) * (x * x) / (2.0f * kneeDb);
                    }
                    else if (overshoot > 0.0f)
                        reductionDb = (1.0f / std::max(ratio, 1.0f) - 1.0f) * overshoot;
                    outputDb = inputDb + reductionDb;
                    yNorm = juce::jmap(outputDb, -48.0f, 0.0f, 0.0f, 1.0f);
                    break;
                }
            }
            const float x = plot.getX() + t * w;
            const float y = plot.getBottom() - juce::jlimit(0.0f, 1.0f, yNorm) * h;
            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }

        g.setColour(palette::kAccent.withAlpha(0.9f));
        g.strokePath(curve, juce::PathStrokeType(2.0f));

        if (mode == 3)
        {
            const float thrX = plot.getX() + juce::jmap(thresholdDb, -48.0f, 0.0f, 0.0f, 1.0f) * w;
            g.setColour(palette::kAccentWarm.withAlpha(0.55f));
            g.drawVerticalLine(static_cast<int>(thrX), static_cast<int>(plot.getY()), static_cast<int>(plot.getBottom()));
        }

        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextDim);
        g.drawText("GR " + juce::String(grDb_, 1) + " dB", plot.removeFromTop(12.0f), juce::Justification::centredRight,
                   true);
    }

    void DesignDynamicsLabPanel::paintSignalDiagram(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        if (bounds.isEmpty())
            return;

        paintPanelCard(g, bounds);
        auto row = bounds.reduced(10, 8).toFloat();
        const float boxW = (row.getWidth() - 48.0f) / 4.0f;
        const char* labels[] = {"VOICES", "DYNAMICS", "MASTER FX", "OUT"};
        for (int i = 0; i < 4; ++i)
        {
            auto box = row.removeFromLeft(boxW);
            g.setColour(i == 1 ? palette::kAccent.withAlpha(0.18f) : juce::Colour(0xff101218));
            g.fillRoundedRectangle(box.reduced(0, 6), 4.0f);
            g.setColour(i == 1 ? palette::kAccent : palette::kBorder);
            g.drawRoundedRectangle(box.reduced(0, 6), 4.0f, 1.0f);
            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextSecondary);
            g.drawText(labels[i], box.reduced(0, 6), juce::Justification::centred, true);
            if (i < 3)
            {
                auto arrow = row.removeFromLeft(16.0f);
                g.setColour(palette::kTextDim);
                g.drawText(fonts::kArrow, arrow, juce::Justification::centred, true);
            }
        }

        g.setFont(fonts::label(8.5f));
        g.setColour(palette::kFigmaTeal.withAlpha(0.85f));
        g.drawText("SC env " + juce::String(sidechainEnv_, 2), bounds.reduced(10, 4).removeFromBottom(12.0f),
                   juce::Justification::centredLeft, true);
    }

    bool DesignDynamicsLabPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

    void DesignDynamicsLabPanel::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colour(0xff040508));
    }

    void DesignDynamicsLabPanel::paintOverChildren(juce::Graphics& g)
    {
        paintTransferCurve(g, transferPlotBounds_);
        paintSignalDiagram(g, signalPlotBounds_);
    }

    void DesignDynamicsLabPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDesignDynamicsLabPageOuterMargin);
        auto header = bounds.removeFromTop(layout::kDesignDynamicsLabHeaderHeight);
        backButton_.setBounds(header.removeFromLeft(96).reduced(0, 8));
        titleLabel_.setBounds(header.removeFromTop(18));
        subtitleLabel_.setBounds(header.removeFromTop(14));
        enableButton_.setBounds(header.removeFromRight(72).reduced(0, 4));
        bounds.removeFromTop(layout::kDesignDynamicsLabPageSectionGap);

        auto footer = bounds.removeFromBottom(layout::kDesignDynamicsLabFooterHeight);
        openPlayOutputButton_.setBounds(footer.removeFromRight(180).reduced(0, 6));
        footerHint_.setBounds(footer);

        auto modeRow = bounds.removeFromTop(24);
        bounds.removeFromTop(8);
        const int pillW = (modeRow.getWidth() - 6) / 4;
        for (std::size_t i = 0; i < modePills_.size(); ++i)
        {
            modePills_[i].setBounds(modeRow.removeFromLeft(pillW).reduced(1, 0));
            modeRow.removeFromLeft(2);
        }

        auto knobs = bounds.removeFromBottom(88);
        bounds.removeFromBottom(8);
        const int knobSlot = knobs.getWidth() / 6;
        thresholdKnob_->setBounds(knobs.removeFromLeft(knobSlot).reduced(2));
        ratioKnob_->setBounds(knobs.removeFromLeft(knobSlot).reduced(2));
        attackKnob_->setBounds(knobs.removeFromLeft(knobSlot).reduced(2));
        releaseKnob_->setBounds(knobs.removeFromLeft(knobSlot).reduced(2));
        sidechainKnob_->setBounds(knobs.removeFromLeft(knobSlot).reduced(2));
        mixKnob_->setBounds(knobs.removeFromLeft(knobSlot).reduced(2));

        signalBounds_ = bounds.removeFromBottom(layout::kDesignDynamicsLabSignalHeight);
        bounds.removeFromBottom(layout::kDesignDynamicsLabPageSectionGap);
        transferBounds_ = bounds;

        heroPanel_.setBounds(transferBounds_);
        signalPanel_.setBounds(signalBounds_);
        transferPlotBounds_ = getLocalArea(&heroPanel_, heroPanel_.getContentBounds()).reduced(4, 2);
        signalPlotBounds_ = getLocalArea(&signalPanel_, signalPanel_.getContentBounds()).reduced(4, 2);
    }

} // namespace pw8::plugin::ui
