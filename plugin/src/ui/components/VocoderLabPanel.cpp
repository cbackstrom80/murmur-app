#include "VocoderLabPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] int readEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        void setEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, int typeOrdinal)
        {
            if (auto* param = apvts.getParameter(prefix + "Type"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(typeOrdinal)));
        }

        [[nodiscard]] juce::String slotShortLabel(std::size_t index)
        {
            if (index < 3)
                return "I" + juce::String(static_cast<int>(index + 1));
            return "M" + juce::String(static_cast<int>(index - 2));
        }

        void paintSignalNode(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text,
                             juce::Colour border, juce::Colour fill, bool dashed = false)
        {
            g.setColour(fill);
            g.fillRoundedRectangle(bounds, 4.0f);
            g.setColour(border);
            if (dashed)
            {
                const float dash[] = {4.0f, 3.0f};
                g.drawDashedLine(juce::Line<float>(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY()),
                                 dash, 2, 1.0f);
                g.drawDashedLine(
                    juce::Line<float>(bounds.getRight(), bounds.getY(), bounds.getRight(), bounds.getBottom()), dash, 2,
                    1.0f);
                g.drawDashedLine(
                    juce::Line<float>(bounds.getRight(), bounds.getBottom(), bounds.getX(), bounds.getBottom()), dash, 2,
                    1.0f);
                g.drawDashedLine(juce::Line<float>(bounds.getX(), bounds.getBottom(), bounds.getX(), bounds.getY()),
                                 dash, 2, 1.0f);
            }
            else
                g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

            g.setColour(palette::kTextPrimary);
            g.setFont(fonts::label(8.0f));
            g.drawText(text, bounds.reduced(6.0f, 4.0f), juce::Justification::centred);
        }

        void paintFlowArrow(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, bool longArrow = false)
        {
            g.setColour(colour.withAlpha(0.85f));
            const float midY = bounds.getCentreY();
            const float x0 = bounds.getX();
            const float x1 = bounds.getRight();
            g.drawLine(x0, midY, x1 - 4.0f, midY, longArrow ? 1.2f : 1.0f);
            juce::Path head;
            head.addTriangle(x1 - 5.0f, midY - 3.0f, x1, midY, x1 - 5.0f, midY + 3.0f);
            g.fillPath(head);
        }
    } // namespace

    VocoderLabPanel::VocoderLabPanel(MurmurProcessor& processor)
        : processor_(processor), apvts_(processor.apvts), enableButton_("BYPASS OFF")
    {
        addAndMakeVisible(backButton_);
        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };

        titleLabel_.setText("VOCODER LAB", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        specBadge_.setText("UX-09", juce::dontSendNotification);
        specBadge_.setFont(fonts::label(9.0f));
        specBadge_.setColour(juce::Label::textColourId, palette::kAccent);
        specBadge_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(specBadge_);

        for (std::size_t i = 0; i < 7; ++i)
            slotCombo_.addItem(slotShortLabel(i) + " (" + (i < 3 ? "insert" : "master") + ")", static_cast<int>(i + 1));

        slotCombo_.onChange = [this] { bindSlot(static_cast<std::size_t>(slotCombo_.getSelectedId() - 1)); };
        addAndMakeVisible(slotCombo_);

        sidechainLabel_.setFont(fonts::label(10.0f));
        sidechainLabel_.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(sidechainLabel_);

        enableButton_.setClickingTogglesState(false);
        enableButton_.onClick = [this] {
            const auto prefix = slotParamPrefix(slotIndex_);
            const int type = readEffectType(apvts_, prefix);
            setEffectType(apvts_, prefix, type == 0 ? 11 : 0);
            enableButton_.setToggleState(type == 0, juce::dontSendNotification);
        };
        addAndMakeVisible(enableButton_);

        openFxChainButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        openFxChainButton_.setColour(juce::TextButton::textColourOffId, palette::kTextPrimary);
        openFxChainButton_.onClick = [this] {
            if (onOpenFxChain)
                onOpenFxChain();
        };
        addAndMakeVisible(openFxChainButton_);

        static constexpr float kSeedCarrier[] = {0.20f, 0.42f, 0.67f, 1.20f, 1.35f, 1.45f, 1.10f, 0.76f,
                                                 0.95f, 1.24f, 1.55f, 1.30f, 0.95f, 0.67f, 0.38f, 0.18f};
        static constexpr float kSeedModulator[] = {0.34f, 0.56f, 0.89f, 1.45f, 1.72f, 1.60f, 1.30f, 0.95f,
                                                   0.80f, 1.14f, 1.40f, 1.52f, 1.20f, 0.86f, 0.52f, 0.28f};
        for (std::size_t i = 0; i < layout::kDesignVocoderBandCount; ++i)
        {
            carrierBandHeights_[i] = kSeedCarrier[i];
            modulatorBandHeights_[i] = kSeedModulator[i];
        }

        bindSlot(2);
        rebuildFftWindow();
        startTimerHz(20);
    }

    VocoderLabPanel::~VocoderLabPanel() { stopTimer(); }

    juce::String VocoderLabPanel::slotParamPrefix(std::size_t slotIndex) const
    {
        if (slotIndex < 3)
            return insertFxParamId(slotIndex, "");
        return masterFxParamId(slotIndex - 3, "");
    }

    juce::String VocoderLabPanel::slotDisplayLabel() const
    {
        return slotShortLabel(slotIndex_) + " INSERT DIRECT SIGNAL PATH";
    }

    void VocoderLabPanel::bindSlot(std::size_t slotIndex)
    {
        slotIndex_ = juce::jlimit<std::size_t>(0, 6, slotIndex);
        slotCombo_.setSelectedId(static_cast<int>(slotIndex_ + 1), juce::dontSendNotification);

        const auto prefix = slotParamPrefix(slotIndex_);
        if (readEffectType(apvts_, prefix) != 11)
            setEffectType(apvts_, prefix, 11);

        enableButton_.setToggleState(readEffectType(apvts_, prefix) != 0, juce::dontSendNotification);
        rebuildKnobs();
    }

    void VocoderLabPanel::rebuildKnobs()
    {
        mixKnob_.reset();
        paramKnobs_.clear();

        const auto prefix = slotParamPrefix(slotIndex_);
        mixKnob_ = std::make_unique<GlowKnob>(apvts_, prefix + "Mix", "VOCODER MIX");
        mixKnob_->applyFigmaContext(figma::KnobContext::DesignVocoder);
        addAndMakeVisible(*mixKnob_);

        static constexpr const char* kFields[] = {"VocoderBandCount", "VocoderFormant", "VocoderSibilance",
                                                  "VocoderScGainDb", "VocoderReleaseMs"};
        static constexpr const char* kLabels[] = {"ACTIVE BANDS", "FORMANT SHIFT", "SIBILANCE", "SIDECHAIN GAIN",
                                                  "RELEASE"};

        for (std::size_t i = 0; i < 5; ++i)
        {
            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + kFields[i], kLabels[i]);
            knob->applyFigmaContext(figma::KnobContext::DesignVocoder);
            addAndMakeVisible(*knob);
            paramKnobs_.push_back(std::move(knob));
        }

        resized();
    }

    void VocoderLabPanel::showForFxSlot(std::size_t slotIndex)
    {
        bindSlot(slotIndex);
        setVisible(true);
    }

    void VocoderLabPanel::dismiss() { setVisible(false); }

    void VocoderLabPanel::setEmbeddedInDesignMode(bool embedded)
    {
        if (embeddedInDesignMode_ == embedded)
            return;

        embeddedInDesignMode_ = embedded;
        backButton_.setButtonText(embedded ? "← DESIGN" : "← PLAY BOARD");
        specBadge_.setVisible(!embedded);
        titleLabel_.setVisible(!embedded);
        slotCombo_.setVisible(!embedded);
        sidechainLabel_.setVisible(!embedded);
        enableButton_.setVisible(!embedded);
        resized();
    }

    void VocoderLabPanel::rebuildFftWindow() noexcept
    {
        float sum = 0.0f;
        for (int i = 0; i < kFftSize; ++i)
        {
            const float w = 0.5f
                            * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i)
                                               / static_cast<float>(kFftSize - 1)));
            fftWindow_[static_cast<std::size_t>(i)] = w;
            sum += w;
        }
        fftWindowSum_ = juce::jmax(1.0e-6f, sum);
    }

    void VocoderLabPanel::updateBandSpectrumFromScope() noexcept
    {
        const int pulled = processor_.readScopeSamples(fftCapture_.data(), kFftSize);
        if (pulled < kFftSize / 2)
        {
            fftReady_ = false;
            return;
        }

        fftData_.fill(0.0f);
        for (int i = 0; i < kFftSize; ++i)
        {
            fftData_[static_cast<std::size_t>(i)] =
                fftCapture_[static_cast<std::size_t>(i)] * fftWindow_[static_cast<std::size_t>(i)];
        }

        fft_.performFrequencyOnlyForwardTransform(fftData_.data());

        const double sampleRate = processor_.getScopeSampleRate() > 0.0 ? processor_.getScopeSampleRate() : 48000.0;
        constexpr float kMinHz = 80.0f;
        constexpr float kMaxHz = 12000.0f;
        constexpr float kMinDb = -62.0f;
        constexpr float kMaxDb = 0.0f;

        std::array<float, layout::kDesignVocoderBandCount> nextCarrier{};
        for (std::size_t band = 0; band < layout::kDesignVocoderBandCount; ++band)
        {
            const float t = static_cast<float>(band) / static_cast<float>(layout::kDesignVocoderBandCount - 1);
            const float freq = kMinHz * std::pow(kMaxHz / kMinHz, t);
            const float binIndex = static_cast<float>(freq * static_cast<float>(kFftSize) / static_cast<float>(sampleRate));
            const int i0 = juce::jlimit(0, kFftSize / 2 - 1, static_cast<int>(binIndex));
            const int i1 = juce::jmin(kFftSize / 2 - 1, i0 + 1);
            const float frac = binIndex - static_cast<float>(i0);
            const float mag = fftData_[static_cast<std::size_t>(i0)] * (1.0f - frac)
                              + fftData_[static_cast<std::size_t>(i1)] * frac;
            const float magNorm = juce::jmax(1.0e-8f, mag * 2.0f / fftWindowSum_);
            const float db = juce::Decibels::gainToDecibels(magNorm, kMinDb);
            nextCarrier[band] = juce::jlimit(0.08f, 1.0f, juce::jmap(db, kMinDb, kMaxDb, 0.08f, 1.0f));
        }

        const float scLevel = processor_.getSidechainLevel();
        const bool scActive = processor_.getSidechainActive();

        for (std::size_t i = 0; i < layout::kDesignVocoderBandCount; ++i)
        {
            carrierBandHeights_[i] =
                juce::jlimit(0.08f, 1.0f, carrierBandHeights_[i] * 0.55f + nextCarrier[i] * 0.45f);
            const float t = static_cast<float>(i) / static_cast<float>(layout::kDesignVocoderBandCount - 1);
            const float bandWeight = 0.35f + 0.65f * std::sin(t * juce::MathConstants<float>::pi);
            const float modTarget = scActive ? scLevel * bandWeight : 0.08f;
            modulatorBandHeights_[i] = juce::jlimit(
                0.08f, 1.0f, modulatorBandHeights_[i] * 0.55f + modTarget * 0.45f);
        }

        fftReady_ = true;
    }

    void VocoderLabPanel::timerCallback()
    {
        animPhase_ += 0.04f;
        if (animPhase_ > juce::MathConstants<float>::twoPi)
            animPhase_ -= juce::MathConstants<float>::twoPi;

        updateBandSpectrumFromScope();

        const bool scActive = processor_.getSidechainActive();
        const float scLevel = processor_.getSidechainLevel();
        sidechainLabel_.setText(scActive ? "SC ● LIVE  " + juce::String(juce::roundToInt(scLevel * 100.0f)) + "%"
                                         : "SC ○ SILENT",
                                juce::dontSendNotification);
        sidechainLabel_.setColour(juce::Label::textColourId, scActive ? palette::kAccent : palette::kTextDim);

        enableButton_.setToggleState(readEffectType(apvts_, slotParamPrefix(slotIndex_)) != 0,
                                     juce::dontSendNotification);

        if (!fftReady_)
        {
            for (std::size_t i = 0; i < layout::kDesignVocoderBandCount; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(layout::kDesignVocoderBandCount - 1);
                const float wobble = 0.08f * std::sin(animPhase_ + t * 4.2f);
                const float bandWeight = 0.35f + 0.65f * std::sin(t * juce::MathConstants<float>::pi);
                const float scTarget = scActive ? scLevel * bandWeight : 0.12f;
                carrierBandHeights_[i] =
                    juce::jlimit(0.08f, 1.0f, carrierBandHeights_[i] * 0.92f + (0.28f + scTarget + wobble) * 0.08f);
                const float modTarget = scActive ? scLevel * (0.4f + 0.6f * bandWeight) : 0.08f;
                modulatorBandHeights_[i] = juce::jlimit(
                    0.08f, 1.0f,
                    modulatorBandHeights_[i] * 0.92f
                        + (modTarget + 0.12f * std::sin(animPhase_ * 0.8f + t * 3.1f)) * 0.08f);
            }
        }

        repaint();
    }

    void VocoderLabPanel::paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8.0f, 1.0f);
    }

    void VocoderLabPanel::paintSignalDiagram(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        paintPanelCard(g, bounds);

        auto area = bounds.reduced(layout::kDesignVocoderSignalDiagramPadding);
        auto header = area.removeFromTop(14);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(9.0f));
        g.drawText("ROUTING SIGNAL FLOW", header, juce::Justification::centredLeft);

        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kAccent);
        g.drawText("CARRIER = SYNTH AUDIO AT THIS SLOT", header.removeFromRight(header.getWidth() / 2),
                   juce::Justification::centredRight);
        g.setColour(palette::kMurmurViolet);
        g.drawText("MODULATOR = EXTERNAL BUS (OR SELF)", header, juce::Justification::centredRight);

        area.removeFromTop(4);
        auto canvas = area.withHeight(layout::kDesignVocoderSignalCanvasHeight);
        const auto slotLabel = slotShortLabel(slotIndex_) + " EFFECT";

        const float nodeH = 22.0f;
        const float carrierY = canvas.getY() + 6.0f;
        const float modY = canvas.getY() + 36.0f;

        float x = static_cast<float>(canvas.getX()) + 12.0f;
        const auto drawCarrierChain = [&](const juce::String& label, bool highlight) {
            const float w = juce::jmax(56.0f, fonts::label(8.0f).getStringWidthFloat(label) + 20.0f);
            paintSignalNode(g, {x, carrierY, w, nodeH}, label,
                            highlight ? palette::kAccent : palette::kBorder,
                            highlight ? palette::kAccent.withAlpha(0.10f) : palette::kBackgroundBottom);
            x += w + 16.0f;
            paintFlowArrow(g, {x - 12.0f, carrierY + 7.0f, 12.0f, 8.0f}, palette::kAccent);
        };

        drawCarrierChain("8 ENGINES", false);
        drawCarrierChain("SUM", false);
        drawCarrierChain(slotLabel, true);
        drawCarrierChain("CARRIER IN", false);
        paintFlowArrow(g, {x - 8.0f, carrierY + 7.0f, 60.0f, 8.0f}, palette::kAccent, true);

        x = static_cast<float>(canvas.getX()) + 12.0f;
        const auto drawModChain = [&](const juce::String& label, bool highlight, bool dashed) {
            const float w = juce::jmax(64.0f, fonts::label(8.0f).getStringWidthFloat(label) + 20.0f);
            paintSignalNode(g, {x, modY, w, nodeH}, label,
                            highlight ? palette::kAccent : palette::kBorder,
                            highlight ? palette::kAccent.withAlpha(0.10f) : palette::kBackgroundBottom, dashed);
            x += w + 16.0f;
            paintFlowArrow(g, {x - 12.0f, modY + 7.0f, 12.0f, 8.0f}, palette::kMurmurViolet);
        };

        drawModChain("AU SIDECHAIN", false, true);
        drawModChain("VOCAL BUS", false, true);
        drawModChain("MODULATOR IN", true, true);
        paintFlowArrow(g, {x - 8.0f, modY + 7.0f, 44.0f, 8.0f}, palette::kMurmurViolet, true);

        const float coreW = 96.0f;
        const float coreX = static_cast<float>(canvas.getRight()) - 188.0f;
        auto core = juce::Rectangle<float>(coreX, canvas.getY() + 4.0f, coreW, 52.0f);
        paintSignalNode(g, core, "VOCODER CORE", palette::kBorder, palette::kBackgroundBottom);
        g.setColour(palette::kAccent);
        g.setFont(fonts::label(7.0f));
        g.drawText("32-BAND MATCH", core.removeFromBottom(16.0f), juce::Justification::centred);

        paintFlowArrow(g, {core.getRight(), core.getCentreY() - 4.0f, 24.0f, 8.0f}, palette::kAccent);

        auto output = juce::Rectangle<float>(static_cast<float>(canvas.getRight()) - 72.0f, canvas.getY() + 14.0f, 60.0f,
                                             28.0f);
        g.setColour(palette::kAccent.withAlpha(0.22f));
        g.fillRoundedRectangle(output, 4.0f);
        g.setColour(palette::kAccent);
        g.drawRoundedRectangle(output, 4.0f, 1.0f);
        g.setFont(fonts::label(9.0f));
        g.drawText("OUTPUT", output, juce::Justification::centred);
    }

    void VocoderLabPanel::paintBandAnalyzer(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        paintPanelCard(g, bounds);

        auto area = bounds.reduced(layout::kDesignVocoderCardPadding);
        auto header = area.removeFromTop(14);
        g.setColour(palette::kAccent);
        g.fillEllipse(static_cast<float>(header.getX()), static_cast<float>(header.getCentreY()) - 3.0f, 6.0f, 6.0f);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(10.0f));
        g.drawText("16-BAND SPECTRUM ANALYSIS", header.withTrimmedLeft(14), juce::Justification::centredLeft);

        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kAccent);
        g.drawText("● CARRIER ENERGY", header.removeFromRight(100), juce::Justification::centredRight);
        g.setColour(palette::kAccentWarm);
        g.drawText("● MODULATOR ENV", header, juce::Justification::centredRight);

        area.removeFromTop(8);
        auto graph = area;
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(graph.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(graph.toFloat().reduced(0.5f), 6.0f, 1.0f);

        auto plot = graph.reduced(16, 12);
        plot.removeFromBottom(14);

        for (int i = 1; i < 4; ++i)
        {
            const float y = static_cast<float>(plot.getY()) + static_cast<float>(plot.getHeight()) * static_cast<float>(i)
                            / 4.0f;
            g.setColour(palette::kBorder.withAlpha(0.35f));
            g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(plot.getX()),
                                 static_cast<float>(plot.getRight()));
        }

        const float maxBarH = static_cast<float>(plot.getHeight());
        const float bandGap = 2.0f;
        const float bandW = (static_cast<float>(plot.getWidth()) - bandGap * static_cast<float>(layout::kDesignVocoderBandCount - 1))
                            / static_cast<float>(layout::kDesignVocoderBandCount);

        for (std::size_t i = 0; i < layout::kDesignVocoderBandCount; ++i)
        {
            const float x = static_cast<float>(plot.getX()) + static_cast<float>(i) * (bandW + bandGap);
            const float modH = modulatorBandHeights_[i] * maxBarH;
            const float carH = carrierBandHeights_[i] * maxBarH;
            const float barW = juce::jmin(10.0f, (bandW - 2.0f) * 0.5f);

            g.setColour(juce::Colour(0xff40321f));
            g.fillRect(x, static_cast<float>(plot.getBottom()) - modH, barW, modH);
            g.setColour(palette::kAccentWarm);
            g.drawRect(x, static_cast<float>(plot.getBottom()) - modH, barW, modH, 1.0f);

            g.setColour(palette::kAccent.withAlpha(0.22f));
            g.fillRect(x + barW + 2.0f, static_cast<float>(plot.getBottom()) - carH, barW, carH);
            g.setColour(palette::kAccent);
            g.drawRect(x + barW + 2.0f, static_cast<float>(plot.getBottom()) - carH, barW, carH, 1.0f);
        }

        static constexpr const char* kFreqLabels[] = {"100Hz", "500Hz", "1kHz", "3kHz", "5kHz", "8kHz", "10kHz"};
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        auto tickRow = graph.removeFromBottom(14).reduced(18, 0);
        for (int i = 0; i < 7; ++i)
        {
            const auto slice = tickRow.withWidth(tickRow.getWidth() / 7);
            g.drawText(kFreqLabels[i], slice.withX(tickRow.getX() + i * slice.getWidth()), juce::Justification::centred);
        }
    }

    void VocoderLabPanel::paint(juce::Graphics& g)
    {
        g.fillAll(embeddedInDesignMode_ ? palette::kBackgroundTop : palette::kBackgroundTop.withAlpha(0.94f));

        if (!embeddedInDesignMode_)
        {
            auto badge = specBadge_.getBounds().toFloat().expanded(4.0f, 2.0f);
            if (!badge.isEmpty())
            {
                g.setColour(palette::kAccent.withAlpha(0.13f));
                g.fillRoundedRectangle(badge, 4.0f);
                g.setColour(palette::kAccent);
                g.drawRoundedRectangle(badge, 4.0f, 1.0f);
            }
        }

        if (!signalDiagramBounds_.isEmpty())
            paintSignalDiagram(g, signalDiagramBounds_);
        if (!bandVizBounds_.isEmpty())
            paintBandAnalyzer(g, bandVizBounds_);
        if (!controlsCardBounds_.isEmpty())
        {
            paintPanelCard(g, controlsCardBounds_);

            auto header = controlsCardBounds_.reduced(layout::kDesignVocoderControlsPadding).removeFromTop(14);
            g.setColour(palette::kMurmurViolet);
            g.fillEllipse(static_cast<float>(header.getX()), static_cast<float>(header.getCentreY()) - 3.0f, 6.0f, 6.0f);
            g.setColour(palette::kTextPrimary);
            g.setFont(fonts::label(11.0f));
            g.drawText("VOCODER ENGINE CONFIGURATION", header.withTrimmedLeft(14), juce::Justification::centredLeft);
        }

        if (!chainRoutingBounds_.isEmpty())
        {
            g.setColour(palette::kBackgroundBottom);
            g.fillRoundedRectangle(chainRoutingBounds_.toFloat(), 6.0f);
            g.setColour(palette::kBorder.withAlpha(0.45f));
            g.drawRoundedRectangle(chainRoutingBounds_.toFloat().reduced(0.5f), 6.0f, 1.0f);

            auto row = chainRoutingBounds_.reduced(10, 0);
            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextDim);
            g.drawText("CHAIN ROUTING:", row.removeFromLeft(88), juce::Justification::centredLeft);
            g.setColour(palette::kAccent);
            g.drawText(slotDisplayLabel(), row.removeFromLeft(row.getWidth() - 140), juce::Justification::centredLeft);
        }

        if (!footerBounds_.isEmpty())
        {
            paintPanelCard(g, footerBounds_);
            auto row = footerBounds_.reduced(16, 0);
            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextDim);
            g.drawText("Logic: Side Chain menu → Vocal BUS · Carrier = mix at slot point", row.removeFromLeft(row.getWidth() / 2),
                       juce::Justification::centredLeft);
            g.drawText("MURMUR VOCODER ENGINE VST3 · © 2026 MURMUR AUDIO", row, juce::Justification::centredRight);
        }
    }

    void VocoderLabPanel::resized()
    {
        auto bounds = getLocalBounds();
        if (embeddedInDesignMode_)
            bounds = bounds.reduced(layout::kDesignVocoderPageOuterMargin);
        else
            bounds = bounds.reduced(16);

        const int headerHeight = embeddedInDesignMode_ ? layout::kDesignLabPanelHeaderHeight : 36;
        auto header = bounds.removeFromTop(headerHeight);
        backButton_.setBounds(header.removeFromLeft(120));
        if (!embeddedInDesignMode_)
        {
            header.removeFromLeft(8);
            specBadge_.setBounds(header.removeFromLeft(36).withSizeKeepingCentre(36, 18));
            header.removeFromLeft(8);
            titleLabel_.setBounds(header.removeFromLeft(140));
            enableButton_.setBounds(header.removeFromRight(90).withSizeKeepingCentre(90, 28));
            header.removeFromRight(8);
            sidechainLabel_.setBounds(header.removeFromRight(140));
            header.removeFromRight(8);
            slotCombo_.setBounds(header.removeFromRight(120).withSizeKeepingCentre(120, 26));
        }

        bounds.removeFromTop(layout::kDesignVocoderPageSectionGap);
        footerBounds_ = bounds.removeFromBottom(layout::kDesignVocoderFooterHeight);
        bounds.removeFromBottom(layout::kDesignVocoderPageSectionGap);

        signalDiagramBounds_ = bounds.removeFromTop(layout::kDesignVocoderSignalDiagramHeight);
        bounds.removeFromTop(layout::kDesignVocoderPageSectionGap);

        auto controls = bounds.removeFromRight(juce::jmin(layout::kDesignVocoderControlsPanelWidth, bounds.getWidth() / 2 + 120));
        bounds.removeFromRight(layout::kDesignVocoderPageSectionGap);
        bandVizBounds_ = bounds;

        controlsCardBounds_ = controls;
        auto controlsInner = controls.reduced(layout::kDesignVocoderControlsPadding);
        controlsInner.removeFromTop(22);
        chainRoutingBounds_ = controlsInner.removeFromBottom(layout::kDesignVocoderChainRoutingBarHeight);
        controlsInner.removeFromBottom(12);

        openFxChainButton_.setBounds(chainRoutingBounds_.removeFromRight(150).reduced(4, 8));

        const int knobW = controlsInner.getWidth() / 3;
        const int knobBlockH = controlsInner.getHeight() / 2;
        auto placeKnob = [&](GlowKnob* knob, int col, int row) {
            if (knob == nullptr)
                return;
            knob->applyFigmaContext(figma::KnobContext::DesignVocoder);
            auto cell = controlsInner.withPosition(controlsInner.getX() + col * knobW, controlsInner.getY() + row * knobBlockH)
                            .withSize(knobW, knobBlockH);
            knob->setBounds(cell.withSizeKeepingCentre(knobW, knobBlockH).reduced(8));
        };

        placeKnob(mixKnob_.get(), 0, 0);
        for (std::size_t i = 0; i < paramKnobs_.size(); ++i)
            placeKnob(paramKnobs_[i].get(), static_cast<int>((i + 1) % 3), static_cast<int>((i + 1) / 3));
    }

    bool VocoderLabPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
