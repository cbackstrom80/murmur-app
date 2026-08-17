#include "EngineCard.h"

#include <cmath>

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        using layout::kDesignModeV2CardHeaderHeight;
        using layout::kDesignModeV2CardPadding;
        using layout::kDesignModeV2CardRowGap;
        using layout::kDesignModeV2ContextVisualizerHeight;
        using layout::kDesignModeV2EnvelopeHeight;
        using layout::kDesignModeV2EnvelopeWidth;
        using layout::kDesignModeV2EnvelopeYOffset;
        using layout::kDesignModeV2KnobsEnvelopeRowHeight;
        using layout::kDesignModeV2LevelRowHeight;
        using layout::kDesignModeV2OscillatorPickerHeight;
        using layout::kDesignModeV2TypeStripHeight;
        using layout::kEngineCardCornerRadius;
        using layout::kEngineCardFilterEnvGap;
        using layout::kEngineCardFilterEnvRowHeight;
        using layout::kEngineCardFilterModePadding;
        using layout::kEngineCardFilterModePillHeight;
        using layout::kEngineCardFilterModeRowHeight;
        using layout::kEngineCardHeaderHeight;
        using layout::kEngineCardKnobDialSize;
        using layout::kEngineCardKnobGap;
        using layout::kEngineCardKnobHeight;
        using layout::kEngineCardKnobWidth;
        using layout::kEngineCardLedSize;
        using layout::kEngineCardLedTitleGap;
        using layout::kEngineCardLevelCaptionGap;
        using layout::kEngineCardLevelCaptionWidth;
        using layout::kEngineCardLevelRowHeight;
        using layout::kEngineCardLevelSliderHeight;
        using layout::kEngineCardLevelValueGap;
        using layout::kEngineCardLevelValueWidth;
        using layout::kEngineCardMixButtonGap;
        using layout::kEngineCardMixControlsWidth;
        using layout::kEngineCardMuteButtonWidth;
        using layout::kEngineCardOnButtonHeight;
        using layout::kEngineCardOnButtonWidth;
        using layout::kEngineCardOscPickerHeight;
        using layout::kEngineCardOscPickerWidth;
        using layout::kEngineCardPadding;
        using layout::kEngineCardPitchKnobsGap;
        using layout::kEngineCardPitchKnobsYOffset;
        using layout::kEngineCardPitchRowHeight;
        using layout::kEngineCardPlayBoardHeaderHeight;
        using layout::kEngineCardPlayBoardKnobGap;
        using layout::kEngineCardPlayBoardKnobSize;
        using layout::kEngineCardPlayBoardLevelRowHeight;
        using layout::kEngineCardPlayBoardMiddleRowHeight;
        using layout::kEngineCardPlayBoardOscStubWidth;
        using layout::kEngineCardPlayBoardRowGap;
        using layout::kEngineCardPlayBoardTypeBadgeFontSize;
        using layout::kEngineCardRowGap;
        using layout::kEngineCardSoloButtonWidth;
        using layout::kEngineCardTitleFontSize;
        using layout::kEngineCardEnvelopeHeight;
        using layout::kEngineCardEnvelopeWidth;
        using layout::kEngineCardEnvelopeYOffset;

        [[nodiscard]] const char* engineTypeLabel(algorithm::EngineType engine) noexcept
        {
            switch (engine)
            {
                case algorithm::EngineType::Classic: return "CLS";
                case algorithm::EngineType::Wavetable: return "WT";
                case algorithm::EngineType::FmPm: return "FM";
                case algorithm::EngineType::Additive: return "ADD";
                case algorithm::EngineType::PhaseShape: return "PHS";
                case algorithm::EngineType::Granular: return "GRN";
                case algorithm::EngineType::NoiseChaos: return "NSE";
                case algorithm::EngineType::Resonator: return "RES";
                case algorithm::EngineType::External: return "EXT";
            }
            return "?";
        }

        void styleMixButton(juce::TextButton& btn, juce::Colour onColour = palette::kAccent.withAlpha(0.35f))
        {
            btn.setClickingTogglesState(true);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
            btn.setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
            btn.setColour(juce::TextButton::buttonOnColourId, onColour);
        }
    } // namespace

    EngineCard::EngineCard(PatchworkEightProcessor& processor, int engineIndex)
        : processor_(processor),
          engineIndex_(engineIndex),
          oscillatorPicker_(processor_, engineIndex),
          adsrMini_(processor_, engineIndex)
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);

        titleLabel_.setText("ENGINE " + juce::String(engineIndex_ + 1).paddedLeft('0', 2), juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(static_cast<float>(kEngineCardTitleFontSize)));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        titleLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(titleLabel_);

        engineTypeBadge_.setFont(fonts::label(static_cast<float>(kEngineCardPlayBoardTypeBadgeFontSize)));
        engineTypeBadge_.setColour(juce::Label::textColourId, palette::kTextDim);
        engineTypeBadge_.setJustificationType(juce::Justification::centredRight);
        engineTypeBadge_.setVisible(false);
        addAndMakeVisible(engineTypeBadge_);

        styleMixButton(onButton_, palette::kAccent.withAlpha(0.28f));
        styleMixButton(soloButton_);
        styleMixButton(muteButton_, palette::kAccentWarm.withAlpha(0.35f));
        for (auto* btn : {&onButton_, &soloButton_, &muteButton_})
            addAndMakeVisible(*btn);

        auto& apvts = processor_.apvts;
        onAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, operatorMixParamId(idx, "MixEnabled"), onButton_);
        muteAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, operatorMixParamId(idx, "MixMute"), muteButton_);
        soloAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, operatorMixParamId(idx, "MixSolo"), soloButton_);

        addAndMakeVisible(oscillatorPicker_);
        oscillatorPicker_.onWavetableLabRequested = [this] {
            if (onWavetableLabRequested)
                onWavetableLabRequested(engineIndex_);
        };
        addAndMakeVisible(adsrMini_);

        coarseKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(idx, "FrequencyRatio"), "COARSE");
        fineKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(idx, "PhaseBend"), "FINE");
        cutoffKnob_ = std::make_unique<GlowKnob>(apvts, operatorFilterParamId(idx, "CutoffHz"), "CUT");
        resKnob_ = std::make_unique<GlowKnob>(apvts, operatorFilterParamId(idx, "Resonance"), "RESO");

        for (auto* knob : {coarseKnob_.get(), fineKnob_.get(), cutoffKnob_.get(), resKnob_.get()})
        {
            knob->setHeaderCompactMode(true);
            knob->setMaxDialDiameter(kEngineCardKnobDialSize);
            addAndMakeVisible(*knob);
        }

        levelCaption_.setFont(fonts::label(7.0f));
        levelCaption_.setColour(juce::Label::textColourId, palette::kTextDim);
        levelCaption_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(levelCaption_);

        levelSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        levelSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        levelSlider_.setRange(0.0, 4.0, 0.001);
        levelSlider_.setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        levelSlider_.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
        levelSlider_.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(levelSlider_);
        levelAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, operatorParamId(idx, "Level"), levelSlider_);

        levelValueLabel_.setFont(fonts::value(7.0f));
        levelValueLabel_.setColour(juce::Label::textColourId, palette::kAccentWarm);
        levelValueLabel_.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(levelValueLabel_);

        for (std::size_t i = 0; i < filterModeButtons_.size(); ++i)
        {
            auto& btn = filterModeButtons_[i];
            btn.setClickingTogglesState(false);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanel);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
            btn.onClick = [this, mode = static_cast<int>(i)] { setFilterMode(mode); };
            addAndMakeVisible(btn);
        }

        startTimerHz(10);
        refreshLevelLabel();
    }

    EngineCard::~EngineCard() { stopTimer(); }

    void EngineCard::setPlayBoardCompactMode(bool compact)
    {
        if (playBoardCompactMode_ == compact)
            return;

        playBoardCompactMode_ = compact;
        if (compact)
            designModeV2Layout_ = false;
        oscillatorPicker_.setPlayBoardCompactMode(compact);
        oscillatorPicker_.setDesignModeV2Layout(false);
        applyPlayBoardCompactVisibility();
        applyDesignModeV2Visibility();
        resized();
        repaint();
    }

    void EngineCard::setDesignModeV2Layout(bool designMode)
    {
        if (designModeV2Layout_ == designMode)
            return;

        designModeV2Layout_ = designMode;
        if (designMode)
            playBoardCompactMode_ = false;
        oscillatorPicker_.setPlayBoardCompactMode(false);
        oscillatorPicker_.setDesignModeV2Layout(designMode);
        applyPlayBoardCompactVisibility();
        applyDesignModeV2Visibility();
        resized();
        repaint();
    }

    void EngineCard::refreshDesignModeV2ControlGroups()
    {
        if (!designModeV2Layout_)
            return;

        algorithm::EngineType engine = algorithm::EngineType::Classic;
        if (auto* raw = processor_.apvts.getRawParameterValue(
                operatorParamId(static_cast<std::size_t>(engineIndex_), "Engine")))
            engine = static_cast<algorithm::EngineType>(static_cast<int>(raw->load() + 0.5f));

        const bool showPitch =
            engine != algorithm::EngineType::NoiseChaos && engine != algorithm::EngineType::External;
        const bool showFilter = engine != algorithm::EngineType::FmPm;

        if (showPitch == designShowPitchKnobs_ && showFilter == designShowFilterKnobs_)
            return;

        designShowPitchKnobs_ = showPitch;
        designShowFilterKnobs_ = showFilter;
        coarseKnob_->setVisible(showPitch);
        fineKnob_->setVisible(showPitch);
        cutoffKnob_->setVisible(showFilter);
        resKnob_->setVisible(showFilter);
        resized();
    }

    void EngineCard::applyDesignModeV2Visibility()
    {
        if (!designModeV2Layout_)
            return;

        onButton_.setVisible(true);
        soloButton_.setVisible(true);
        muteButton_.setVisible(true);
        adsrMini_.setVisible(true);
        engineTypeBadge_.setVisible(false);

        for (auto& btn : filterModeButtons_)
            btn.setVisible(false);

        refreshDesignModeV2ControlGroups();
    }

    void EngineCard::applyPlayBoardCompactVisibility()
    {
        if (designModeV2Layout_)
            return;

        onButton_.setVisible(!playBoardCompactMode_);
        soloButton_.setVisible(!playBoardCompactMode_);
        muteButton_.setVisible(!playBoardCompactMode_);
        cutoffKnob_->setVisible(!playBoardCompactMode_);
        resKnob_->setVisible(!playBoardCompactMode_);
        adsrMini_.setVisible(!playBoardCompactMode_);
        coarseKnob_->setVisible(!playBoardCompactMode_);
        fineKnob_->setVisible(!playBoardCompactMode_);
        engineTypeBadge_.setVisible(playBoardCompactMode_);

        for (auto& btn : filterModeButtons_)
            btn.setVisible(!playBoardCompactMode_);
    }

    void EngineCard::refreshEngineTypeBadge()
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        algorithm::EngineType engine = algorithm::EngineType::Classic;
        if (auto* raw = processor_.apvts.getRawParameterValue(operatorParamId(idx, "Engine")))
            engine = static_cast<algorithm::EngineType>(static_cast<int>(raw->load() + 0.5f));

        engineTypeBadge_.setText(engineTypeLabel(engine), juce::dontSendNotification);
    }

    void EngineCard::setFilterMode(int modeOrdinal)
    {
        if (auto* param = processor_.apvts.getParameter(
                operatorFilterParamId(static_cast<std::size_t>(engineIndex_), "FilterMode")))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(modeOrdinal)));
        repaint();
    }

    void EngineCard::refreshLevelLabel()
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        float level = 1.0f;
        if (auto* raw = processor_.apvts.getRawParameterValue(operatorParamId(idx, "Level")))
            level = raw->load();
        const int pct = juce::jlimit(0, 400, static_cast<int>(level / 4.0f * 100.0f + 0.5f));
        levelValueLabel_.setText(juce::String(pct) + "%", juce::dontSendNotification);
    }

    void EngineCard::refreshMixButtonStates()
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (auto* enabled = processor_.apvts.getRawParameterValue(operatorMixParamId(idx, "MixEnabled")))
            ledActive_ = enabled->load() >= 0.5f;

        int filterMode = 0;
        if (auto* mode = processor_.apvts.getRawParameterValue(operatorFilterParamId(idx, "FilterMode")))
            filterMode = static_cast<int>(mode->load() + 0.5f);
        for (std::size_t i = 0; i < filterModeButtons_.size(); ++i)
        {
            const bool on = static_cast<int>(i) == filterMode;
            filterModeButtons_[i].setColour(juce::TextButton::buttonColourId,
                                            on ? palette::kAccent.withAlpha(0.28f) : palette::kPanel);
            filterModeButtons_[i].setColour(juce::TextButton::textColourOffId,
                                            on ? palette::kTextPrimary : palette::kTextDim);
        }
        refreshLevelLabel();
        refreshEngineTypeBadge();
        if (designModeV2Layout_)
            refreshDesignModeV2ControlGroups();
        setAlpha(ledActive_ ? 1.0f : 0.35f);
        repaint();
    }

    void EngineCard::timerCallback()
    {
        ledPulsePhase_ += 0.11f;
        if (ledPulsePhase_ > juce::MathConstants<float>::twoPi)
            ledPulsePhase_ -= juce::MathConstants<float>::twoPi;

        const auto idx = static_cast<std::size_t>(engineIndex_);
        const float peakLinear = processor_.getOperatorPeakLinear(idx);
        const float peakNorm = juce::jlimit(0.0f, 1.0f, peakLinear / 4.0f);
        livePeakVu_.processFrame(peakNorm * 0.707f, peakNorm);
        livePeakNorm_ = livePeakVu_.rmsNorm();

        refreshMixButtonStates();

        if (playBoardCompactMode_)
        {
            if (ledActive_)
                repaint(getLocalBounds().withHeight(kEngineCardPlayBoardHeaderHeight + 2));
            if (livePeakNorm_ > 0.01f && !levelSliderBounds_.isEmpty())
                repaint(levelSliderBounds_.expanded(1));
            if (!playBoardKnobStubBounds_[0].isEmpty())
            {
                auto knobArea = playBoardKnobStubBounds_[0].getUnion(playBoardKnobStubBounds_[1]);
                repaint(knobArea.expanded(1));
            }
        }
    }

    void EngineCard::mouseDoubleClick(const juce::MouseEvent& event)
    {
        juce::Component::mouseDoubleClick(event);
        if (onDoubleClicked)
            onDoubleClicked(engineIndex_);
    }

    void EngineCard::paintLevelRow(juce::Graphics& g, juce::Rectangle<int> /*rowBounds*/)
    {
        if (levelSliderBounds_.isEmpty())
            return;

        auto track = levelSliderBounds_.toFloat();
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(track, 3.0f);
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(track.reduced(0.5f), 3.0f, 1.0f);

        if (playBoardCompactMode_ && ledActive_ && livePeakNorm_ > 0.01f)
        {
            auto liveFill = track.reduced(1.0f);
            const float liveNorm = juce::jlimit(0.0f, 1.0f, livePeakNorm_);
            liveFill.setWidth(liveFill.getWidth() * liveNorm);
            g.setColour(palette::kAccent.withAlpha(0.5f));
            g.fillRoundedRectangle(liveFill, 2.0f);
            if (liveNorm > 0.75f)
            {
                g.setColour(palette::kAccent.withAlpha(0.25f));
                g.fillRoundedRectangle(liveFill.expanded(0.0f, 1.0f), 2.5f);
            }
        }

        const float norm = static_cast<float>(levelSlider_.valueToProportionOfLength(levelSlider_.getValue()));
        auto fill = track.reduced(1.0f);
        const float fillW = juce::jmax(0.0f, fill.getWidth() * norm);
        fill.setWidth(fillW);
        g.setColour(palette::kAccentWarm);
        g.fillRoundedRectangle(fill, 2.0f);

        if (playBoardCompactMode_ && ledActive_ && fillW > 6.0f)
        {
            auto tail = fill;
            tail.setX(tail.getRight() - juce::jmin(10.0f, fillW * 0.35f));
            tail.setWidth(juce::jmin(10.0f, fillW * 0.35f));
            g.setGradientFill(juce::ColourGradient(palette::kAccentWarm.withAlpha(0.0f), tail.getX(), tail.getCentreY(),
                                                   palette::kAccentWarm.withAlpha(0.85f), tail.getRight(),
                                                   tail.getCentreY(), false));
            g.fillRoundedRectangle(tail, 2.0f);
        }
    }

    void EngineCard::paintPlayBoardKnobStubs(juce::Graphics& g) const
    {
        if (playBoardKnobStubBounds_[0].isEmpty())
            return;

        const auto idx = static_cast<std::size_t>(engineIndex_);
        const auto readNorm = [this, idx](const char* suffix) -> float {
            if (auto* raw = processor_.apvts.getRawParameterValue(operatorParamId(idx, suffix)))
                return raw->load();
            return 0.5f;
        };

        const float knobNorms[] = {readNorm("FrequencyRatio"), readNorm("PhaseBend")};
        for (std::size_t i = 0; i < playBoardKnobStubBounds_.size(); ++i)
        {
            const auto bounds = playBoardKnobStubBounds_[i];
            const auto dial = bounds.toFloat();
            g.setColour(palette::kPanelRaised);
            g.fillEllipse(dial);
            g.setColour(palette::kBorder.withAlpha(0.85f));
            g.drawEllipse(dial.reduced(0.5f), 1.0f);
            g.setColour(palette::kBorder.withAlpha(0.35f));
            g.drawEllipse(dial.reduced(3.0f), 0.8f);

            const float norm = juce::jlimit(0.0f, 1.0f, knobNorms[i]);
            const float angle = juce::MathConstants<float>::pi * 1.25f
                                + (norm * 2.0f - 1.0f) * juce::MathConstants<float>::halfPi;
            const float r = dial.getWidth() * 0.38f;
            g.setColour(palette::kAccent.withAlpha(0.75f));
            g.drawLine(dial.getCentreX() + std::cos(angle) * 2.0f, dial.getCentreY() + std::sin(angle) * 2.0f,
                       dial.getCentreX() + std::cos(angle) * r, dial.getCentreY() + std::sin(angle) * r, 1.2f);
        }
    }

    void EngineCard::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, static_cast<float>(kEngineCardCornerRadius));
        g.setColour(palette::kBorderBright.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds, static_cast<float>(kEngineCardCornerRadius), 1.0f);

        const int headerHeight = playBoardCompactMode_
                                     ? kEngineCardPlayBoardHeaderHeight
                                     : (designModeV2Layout_ ? kDesignModeV2CardHeaderHeight : kEngineCardHeaderHeight);
        auto header = bounds.removeFromTop(static_cast<float>(headerHeight));
        header = header.reduced(static_cast<float>(designModeV2Layout_ ? kDesignModeV2CardPadding : kEngineCardPadding),
                                0.0f);
        const float ledR = static_cast<float>(kEngineCardLedSize) * 0.5f;
        const float ledX = header.getX();
        const float ledY = header.getCentreY() - ledR;
        g.setColour(ledActive_ ? palette::kAccent.withAlpha(0.95f) : palette::kTextDim.withAlpha(0.35f));
        g.fillEllipse(ledX, ledY, static_cast<float>(kEngineCardLedSize), static_cast<float>(kEngineCardLedSize));
        if (ledActive_)
        {
            const float pulse = playBoardCompactMode_
                                    ? 0.22f + 0.18f * (0.5f + 0.5f * std::sin(ledPulsePhase_))
                                    : 0.3f;
            g.setColour(palette::kAccent.withAlpha(pulse));
            g.fillEllipse(ledX - 1.5f, ledY - 1.5f, static_cast<float>(kEngineCardLedSize) + 3.0f,
                         static_cast<float>(kEngineCardLedSize) + 3.0f);
        }

        if (playBoardCompactMode_)
            paintPlayBoardKnobStubs(g);

        auto levelRow = getLocalBounds().reduced(
            designModeV2Layout_ ? kDesignModeV2CardPadding : kEngineCardPadding);
        if (playBoardCompactMode_)
        {
            levelRow.removeFromTop(kEngineCardPlayBoardHeaderHeight + kEngineCardPlayBoardRowGap
                                   + kEngineCardPlayBoardMiddleRowHeight + kEngineCardPlayBoardRowGap);
            paintLevelRow(g, levelRow.removeFromTop(kEngineCardPlayBoardLevelRowHeight));
            return;
        }

        if (designModeV2Layout_)
        {
            levelRow.removeFromTop(kDesignModeV2CardHeaderHeight + kDesignModeV2CardRowGap + kDesignModeV2OscillatorPickerHeight
                                   + kDesignModeV2CardRowGap + kDesignModeV2KnobsEnvelopeRowHeight + kDesignModeV2CardRowGap);
            paintLevelRow(g, levelRow.removeFromTop(kDesignModeV2LevelRowHeight));
            return;
        }

        levelRow.removeFromTop(kEngineCardHeaderHeight + kEngineCardRowGap + kEngineCardPitchRowHeight + kEngineCardRowGap
                               + kEngineCardFilterEnvRowHeight + kEngineCardRowGap);
        paintLevelRow(g, levelRow.removeFromTop(kEngineCardLevelRowHeight));
    }

    void EngineCard::resized()
    {
        auto bounds = getLocalBounds().reduced(
            designModeV2Layout_ ? kDesignModeV2CardPadding : kEngineCardPadding);

        if (playBoardCompactMode_)
        {
            auto header = bounds.removeFromTop(kEngineCardPlayBoardHeaderHeight);
            engineTypeBadge_.setBounds(header.removeFromRight(20));
            titleLabel_.setBounds(header.withTrimmedLeft(kEngineCardLedSize + kEngineCardLedTitleGap));

            bounds.removeFromTop(kEngineCardPlayBoardRowGap);

            auto middleRow = bounds.removeFromTop(kEngineCardPlayBoardMiddleRowHeight);
            oscillatorPicker_.setBounds(middleRow.removeFromLeft(kEngineCardPlayBoardOscStubWidth));
            middleRow.removeFromLeft(8);
            auto knobArea = middleRow.withSizeKeepingCentre(
                kEngineCardPlayBoardKnobSize * 2 + kEngineCardPlayBoardKnobGap, kEngineCardPlayBoardKnobSize);
            playBoardKnobStubBounds_[0] = knobArea.removeFromLeft(kEngineCardPlayBoardKnobSize);
            knobArea.removeFromLeft(kEngineCardPlayBoardKnobGap);
            playBoardKnobStubBounds_[1] = knobArea.removeFromLeft(kEngineCardPlayBoardKnobSize);

            bounds.removeFromTop(kEngineCardPlayBoardRowGap);

            auto levelRow = bounds.removeFromTop(kEngineCardPlayBoardLevelRowHeight);
            levelCaption_.setBounds(levelRow.removeFromLeft(kEngineCardLevelCaptionWidth));
            levelRow.removeFromLeft(kEngineCardLevelCaptionGap);
            levelValueLabel_.setBounds(levelRow.removeFromRight(kEngineCardLevelValueWidth));
            levelRow.removeFromRight(kEngineCardLevelValueGap);
            levelSliderBounds_ = levelRow;
            levelSlider_.setBounds(levelSliderBounds_);
            return;
        }

        if (designModeV2Layout_)
        {
            playBoardKnobStubBounds_[0] = {};
            playBoardKnobStubBounds_[1] = {};

            auto header = bounds.removeFromTop(kDesignModeV2CardHeaderHeight);
            titleLabel_.setBounds(header.withTrimmedLeft(kEngineCardLedSize + kEngineCardLedTitleGap).withTrimmedRight(64));

            auto mixRow = header.removeFromRight(64);
            muteButton_.setBounds(mixRow.removeFromRight(17).withHeight(14));
            mixRow.removeFromRight(4);
            soloButton_.setBounds(mixRow.removeFromRight(17).withHeight(14));
            mixRow.removeFromRight(4);
            onButton_.setBounds(mixRow.removeFromLeft(22).withHeight(14));

            bounds.removeFromTop(kDesignModeV2CardRowGap);

            oscillatorPicker_.setBounds(bounds.removeFromTop(kDesignModeV2OscillatorPickerHeight));

            bounds.removeFromTop(kDesignModeV2CardRowGap);

            auto knobsEnvRow = bounds.removeFromTop(kDesignModeV2KnobsEnvelopeRowHeight);
            auto knobStrip = knobsEnvRow.removeFromLeft(203);
            constexpr int kDesignKnobW = 44;
            constexpr int kDesignKnobH = 40;
            constexpr int kDesignKnobGap = 4;
            auto placeDesignKnob = [&](GlowKnob* knob) {
                if (knob != nullptr && knob->isVisible())
                {
                    knob->setBounds(knobStrip.removeFromLeft(kDesignKnobW).withHeight(kDesignKnobH));
                    knobStrip.removeFromLeft(kDesignKnobGap);
                }
            };
            placeDesignKnob(coarseKnob_.get());
            placeDesignKnob(fineKnob_.get());
            placeDesignKnob(cutoffKnob_.get());
            placeDesignKnob(resKnob_.get());

            knobsEnvRow.removeFromLeft(8);
            adsrMini_.setBounds(knobsEnvRow.withSizeKeepingCentre(kDesignModeV2EnvelopeWidth, kDesignModeV2EnvelopeHeight)
                                    .withY(knobsEnvRow.getY() + kDesignModeV2EnvelopeYOffset));

            bounds.removeFromTop(kDesignModeV2CardRowGap);

            auto levelRow = bounds.removeFromTop(kDesignModeV2LevelRowHeight);
            levelCaption_.setBounds(levelRow.removeFromLeft(kEngineCardLevelCaptionWidth));
            levelRow.removeFromLeft(kEngineCardLevelCaptionGap);
            levelValueLabel_.setBounds(levelRow.removeFromRight(24));
            levelRow.removeFromRight(8);
            levelSliderBounds_ = levelRow;
            levelSlider_.setBounds(levelSliderBounds_);
            return;
        }

        playBoardKnobStubBounds_[0] = {};
        playBoardKnobStubBounds_[1] = {};

        auto header = bounds.removeFromTop(kEngineCardHeaderHeight);
        auto labelArea = header;
        labelArea.removeFromRight(kEngineCardMixControlsWidth);
        titleLabel_.setBounds(labelArea.withTrimmedLeft(kEngineCardLedSize + kEngineCardLedTitleGap));

        auto mixRow = header.removeFromRight(kEngineCardMixControlsWidth);
        muteButton_.setBounds(mixRow.removeFromRight(kEngineCardMuteButtonWidth));
        mixRow.removeFromRight(kEngineCardMixButtonGap);
        soloButton_.setBounds(mixRow.removeFromRight(kEngineCardSoloButtonWidth));
        mixRow.removeFromRight(kEngineCardMixButtonGap);
        onButton_.setBounds(mixRow.removeFromLeft(kEngineCardOnButtonWidth).withHeight(kEngineCardOnButtonHeight));

        bounds.removeFromTop(kEngineCardRowGap);

        auto pitchRow = bounds.removeFromTop(kEngineCardPitchRowHeight);
        oscillatorPicker_.setBounds(pitchRow.removeFromLeft(kEngineCardOscPickerWidth).withHeight(kEngineCardOscPickerHeight));
        pitchRow.removeFromLeft(kEngineCardPitchKnobsGap);
        auto pitchKnobs = pitchRow.withTrimmedTop(kEngineCardPitchKnobsYOffset).withHeight(kEngineCardKnobHeight);
        coarseKnob_->setBounds(pitchKnobs.removeFromLeft(kEngineCardKnobWidth));
        pitchKnobs.removeFromLeft(kEngineCardKnobGap);
        fineKnob_->setBounds(pitchKnobs.removeFromLeft(kEngineCardKnobWidth));

        bounds.removeFromTop(kEngineCardRowGap);

        auto filterEnvRow = bounds.removeFromTop(kEngineCardFilterEnvRowHeight);
        auto filterGroup = filterEnvRow.removeFromLeft(filterEnvRow.getWidth() - kEngineCardEnvelopeWidth - kEngineCardFilterEnvGap);
        auto filterKnobs = filterGroup.removeFromTop(kEngineCardKnobHeight);
        cutoffKnob_->setBounds(filterKnobs.removeFromLeft(kEngineCardKnobWidth));
        filterKnobs.removeFromLeft(kEngineCardKnobGap);
        resKnob_->setBounds(filterKnobs.removeFromLeft(kEngineCardKnobWidth));

        filterGroup.removeFromTop(4);
        auto modeRow = filterGroup.removeFromTop(kEngineCardFilterModeRowHeight).reduced(kEngineCardFilterModePadding, 0);
        const int modeW = modeRow.getWidth() / static_cast<int>(filterModeButtons_.size());
        for (auto& btn : filterModeButtons_)
            btn.setBounds(modeRow.removeFromLeft(modeW).withHeight(kEngineCardFilterModePillHeight).reduced(0, 0));

        filterEnvRow.removeFromLeft(kEngineCardFilterEnvGap);
        adsrMini_.setBounds(filterEnvRow.removeFromRight(kEngineCardEnvelopeWidth)
                                .withTrimmedTop(kEngineCardEnvelopeYOffset)
                                .withHeight(kEngineCardEnvelopeHeight));

        bounds.removeFromTop(kEngineCardRowGap);

        auto levelRow = bounds.removeFromTop(kEngineCardLevelRowHeight);
        levelCaption_.setBounds(levelRow.removeFromLeft(kEngineCardLevelCaptionWidth));
        levelRow.removeFromLeft(kEngineCardLevelCaptionGap);
        levelValueLabel_.setBounds(levelRow.removeFromRight(kEngineCardLevelValueWidth));
        levelRow.removeFromRight(kEngineCardLevelValueGap);
        levelSliderBounds_ = levelRow;
        levelSlider_.setBounds(levelSliderBounds_);
    }

} // namespace pw8::plugin::ui
