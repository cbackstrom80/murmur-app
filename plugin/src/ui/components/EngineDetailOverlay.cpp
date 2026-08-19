#include "EngineDetailOverlay.h"

#include "../PerformanceMetricsUi.h"
#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../theme/FigmaKnobTokens.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"
#include "state/PluginState.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        using layout::kEngineDeepEditorAmpColumnHeight;
        using layout::kEngineDeepEditorBottomBarHeight;
        using layout::kEngineDeepEditorColumnGap;
        using layout::kEngineDeepEditorColumnWidth;
        using layout::kEngineDeepEditorEngineTabGap;
        using layout::kEngineDeepEditorEngineTabHeight;
        using layout::kEngineDeepEditorFilterColumnHeight;
        using layout::kEngineDeepEditorFrameHeight;
        using layout::kEngineDeepEditorFrameWidth;
        using layout::kEngineDeepEditorHeaderHeight;
        using layout::kEngineDeepEditorMainContentHeight;
        using layout::kEngineDeepEditorOuterMargin;
        using layout::kEngineDeepEditorOscColumnHeight;
        using layout::kEngineDeepEditorStateButtonHeight;

        void styleEngineTab(juce::TextButton& btn)
        {
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(9201);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::buttonOnColourId, palette::kAccent.withAlpha(0.22f));
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn.setColour(juce::TextButton::textColourOnId, palette::kAccent);
        }

        void styleStateButton(juce::TextButton& btn, juce::Colour onColour = palette::kAccent.withAlpha(0.28f))
        {
            btn.setClickingTogglesState(true);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::buttonOnColourId, onColour);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
            btn.setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
        }

        juce::String shortModSourceLabel(modulation::ModSource source)
        {
            const auto full = modSourceLabel(source);
            if (full.startsWith("LFO "))
                return "LFO" + full.substring(4).trim();
            if (full.startsWith("ENV "))
                return full.substring(4).trim();
            if (full == "MOD WHEEL")
                return "MWHL";
            if (full == "VELOCITY")
                return "VEL";
            if (full == "EXPRESSION")
                return "EXPR";
            if (full.startsWith("MACRO "))
                return "M" + full.substring(6).trim();
            return full.replace(" ", "");
        }

        juce::String shortModDestinationLabel(modulation::ModDestination destination, std::uint8_t targetIndex)
        {
            using modulation::ModDestination;
            switch (destination)
            {
                case ModDestination::FilterCutoff:
                case ModDestination::OperatorFilterCutoff:
                    return "CUTOFF";
                case ModDestination::FilterResonance:
                case ModDestination::OperatorFilterResonance:
                    return "RES";
                case ModDestination::OperatorLevel:
                    return "LEVEL";
                case ModDestination::Pan:
                    return "PAN";
                case ModDestination::OperatorWavetablePosition:
                    return "WT POS";
                case ModDestination::OperatorWavetableBend:
                    return "PWM";
                case ModDestination::OperatorWavetableAsymmetry:
                    return "ASYM";
                case ModDestination::OperatorWavetableSyncRatio:
                    return "SYNC";
                case ModDestination::OperatorWavetableFormant:
                    return "FORM";
                case ModDestination::OperatorWavetableSyncAmount:
                    return "SYNC AMT";
                case ModDestination::MasterGain:
                    return "GAIN";
                default:
                    break;
            }
            juce::ignoreUnused(targetIndex);
            return modDestinationLabel(destination, targetIndex).replace(" ", "");
        }

        juce::String formatAmpRouteRow(const modulation::ModRoute& route)
        {
            const auto source = shortModSourceLabel(route.source);
            const auto dest = shortModDestinationLabel(route.destination, route.targetIndex);
            const auto depth = formatModRouteAmount(route.destination, route.amount);
            return source + "  →  " + dest + "  " + depth;
        }
    } // namespace

    EngineDetailOverlay::EngineDetailOverlay(PatchworkEightProcessor& processor,
                                             ModAssignmentController& modAssignmentController)
        : processor_(processor),
          operatorPanel_(processor, modAssignmentController),
          filterPanel_(processor, modAssignmentController),
          adsrMini_(std::make_unique<EngineAdsrMini>(processor, 0))
    {
        setVisible(false);
        addAndMakeVisible(contentHost_);

        brandTitle_.setText("MURMUR 8-ENGINE", juce::dontSendNotification);
        brandTitle_.setFont(fonts::title(14.0f));
        brandTitle_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        contentHost_.addAndMakeVisible(brandTitle_);

        brandSubtitle_.setFont(fonts::label(9.0f));
        brandSubtitle_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        contentHost_.addAndMakeVisible(brandSubtitle_);

        for (int i = 0; i < static_cast<int>(engineTabButtons_.size()); ++i)
        {
            auto& btn = engineTabButtons_[static_cast<std::size_t>(i)];
            btn.setButtonText("ENG " + juce::String(i + 1));
            styleEngineTab(btn);
            btn.onClick = [this, i] { rebindEngine(i); };
            contentHost_.addAndMakeVisible(btn);
        }

        styleStateButton(onButton_);
        styleStateButton(soloButton_, palette::kAccentWarm.withAlpha(0.28f));
        styleStateButton(muteButton_, juce::Colour(0xff553333).withAlpha(0.55f));
        contentHost_.addAndMakeVisible(onButton_);
        contentHost_.addAndMakeVisible(soloButton_);
        contentHost_.addAndMakeVisible(muteButton_);

        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        closeButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        contentHost_.addAndMakeVisible(closeButton_);

        nextEngineButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        nextEngineButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        nextEngineButton_.onClick = [this] { rebindEngine((engineIndex_ + 1) % 8); };
        contentHost_.addAndMakeVisible(nextEngineButton_);

        contentHost_.addAndMakeVisible(oscColumn_);
        contentHost_.addAndMakeVisible(filterColumn_);
        contentHost_.addAndMakeVisible(ampColumn_);

        oscColumn_.addAndMakeVisible(operatorPanel_);
        filterColumn_.addAndMakeVisible(filterPanel_);
        filterColumn_.addAndMakeVisible(*adsrMini_);

        filterPanel_.setScope(FilterPanelScope::Engine, 0);

        rebuildAmpKnobs();
        startTimerHz(8);
    }

    void EngineDetailOverlay::showForEngine(int engineIndex)
    {
        rebindEngine(engineIndex);
        setVisible(true);
    }

    void EngineDetailOverlay::dismiss() { setVisible(false); }

    void EngineDetailOverlay::rebindEngine(int engineIndex)
    {
        engineIndex_ = juce::jlimit(0, 7, engineIndex);
        brandSubtitle_.setText("ENGINE " + juce::String(engineIndex_ + 1).paddedLeft('0', 2) + " — DEEP EDITOR",
                               juce::dontSendNotification);

        for (int i = 0; i < static_cast<int>(engineTabButtons_.size()); ++i)
            engineTabButtons_[static_cast<std::size_t>(i)].setToggleState(i == engineIndex_, juce::dontSendNotification);

        operatorPanel_.showNode(engineIndex_);
        filterPanel_.setScope(FilterPanelScope::Engine, engineIndex_);
        adsrMini_ = std::make_unique<EngineAdsrMini>(processor_, engineIndex_);
        filterColumn_.removeAllChildren();
        filterColumn_.addAndMakeVisible(filterPanel_);
        filterColumn_.addAndMakeVisible(*adsrMini_);
        rebuildMixAttachments();
        rebuildAmpKnobs();
        layoutDesignSurface();
        repaint();
    }

    void EngineDetailOverlay::rebuildAmpKnobs()
    {
        ampColumn_.removeAllChildren();
        auto& apvts = processor_.apvts;
        const auto idx = static_cast<std::size_t>(engineIndex_);

        levelKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(idx, "Level"), "Level", nullptr,
                                                palette::kAccentWarm);
        panKnob_ = std::make_unique<GlowKnob>(
            apvts, juce::String(kLayerPanId), "Pan",
            [](float v) { return juce::String(juce::roundToInt(v * 100.0f)); }, palette::kAccentWarm);

        unisonDetuneKnob_ = std::make_unique<GlowKnob>(
            apvts, kUnisonDetuneId, "Detune",
            [](float cents) { return juce::String(juce::roundToInt(cents * 0.5f)) + "%"; }, palette::kAccent);
        unisonSpreadKnob_ = std::make_unique<GlowKnob>(
            apvts, kUnisonSpreadId, "Spread",
            [](float spread) { return juce::String(juce::roundToInt(spread * 100.0f)) + "%"; }, palette::kAccent);

        if (levelKnob_ != nullptr)
        {
            levelKnob_->applyFigmaContext(figma::KnobContext::DesktopMacroStandard);
            ampColumn_.addAndMakeVisible(*levelKnob_);
        }
        if (panKnob_ != nullptr)
        {
            panKnob_->applyFigmaContext(figma::KnobContext::DesktopMacroStandard);
            ampColumn_.addAndMakeVisible(*panKnob_);
        }
        if (unisonDetuneKnob_ != nullptr)
        {
            unisonDetuneKnob_->applyFigmaContext(figma::KnobContext::DashboardFilter);
            ampColumn_.addAndMakeVisible(*unisonDetuneKnob_);
        }
        if (unisonSpreadKnob_ != nullptr)
        {
            unisonSpreadKnob_->applyFigmaContext(figma::KnobContext::DashboardFilter);
            ampColumn_.addAndMakeVisible(*unisonSpreadKnob_);
        }
    }

    void EngineDetailOverlay::rebuildMixAttachments()
    {
        auto& apvts = processor_.apvts;
        const auto idx = static_cast<std::size_t>(engineIndex_);
        onAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, operatorMixParamId(idx, "MixEnabled"), onButton_);
        soloAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, operatorMixParamId(idx, "MixSolo"), soloButton_);
        muteAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, operatorMixParamId(idx, "MixMute"), muteButton_);
    }

    void EngineDetailOverlay::layoutDesignSurface()
    {
        auto bounds = juce::Rectangle<int>(0, 0, kEngineDeepEditorFrameWidth, kEngineDeepEditorFrameHeight);
        bounds = bounds.reduced(kEngineDeepEditorOuterMargin);

        auto header = bounds.removeFromTop(kEngineDeepEditorHeaderHeight);
        auto brandArea = header.removeFromLeft(220);
        brandTitle_.setBounds(brandArea.removeFromTop(20));
        brandSubtitle_.setBounds(brandArea.removeFromTop(14));

        const int tabTotalW = 462;
        auto tabRow = header.withSizeKeepingCentre(tabTotalW, kEngineDeepEditorEngineTabHeight);
        static constexpr std::array<int, 8> kTabWidths{53, 54, 54, 55, 54, 55, 54, 55};
        for (std::size_t i = 0; i < engineTabButtons_.size(); ++i)
        {
            engineTabButtons_[i].setBounds(tabRow.removeFromLeft(kTabWidths[i]));
            if (i + 1 < engineTabButtons_.size())
                tabRow.removeFromLeft(kEngineDeepEditorEngineTabGap);
        }

        auto stateRow = header.removeFromRight(139);
        onButton_.setBounds(stateRow.removeFromLeft(34).withHeight(kEngineDeepEditorStateButtonHeight));
        stateRow.removeFromLeft(6);
        soloButton_.setBounds(stateRow.removeFromLeft(46).withHeight(kEngineDeepEditorStateButtonHeight));
        stateRow.removeFromLeft(6);
        muteButton_.setBounds(stateRow.removeFromLeft(47).withHeight(kEngineDeepEditorStateButtonHeight));

        closeButton_.setBounds(juce::Rectangle<int>(kEngineDeepEditorFrameWidth - kEngineDeepEditorOuterMargin - 96,
                                                    kEngineDeepEditorOuterMargin + 8, 96, 24));

        bounds.removeFromTop(kEngineDeepEditorOuterMargin - 8);
        footerBounds_ = bounds.removeFromBottom(kEngineDeepEditorBottomBarHeight);
        auto mainRow = bounds.removeFromTop(kEngineDeepEditorMainContentHeight);

        oscColumn_.setBounds(mainRow.removeFromLeft(kEngineDeepEditorColumnWidth).withHeight(kEngineDeepEditorOscColumnHeight));
        mainRow.removeFromLeft(kEngineDeepEditorColumnGap);
        filterColumn_.setBounds(mainRow.removeFromLeft(kEngineDeepEditorColumnWidth).withHeight(kEngineDeepEditorFilterColumnHeight));
        mainRow.removeFromLeft(kEngineDeepEditorColumnGap);
        ampColumn_.setBounds(mainRow.removeFromLeft(kEngineDeepEditorColumnWidth).withHeight(kEngineDeepEditorAmpColumnHeight));

        oscHeadingBounds_ = oscColumn_.getBounds().reduced(16, 16).removeFromTop(35);
        filterHeadingBounds_ = filterColumn_.getBounds().reduced(16, 16).removeFromTop(35);
        ampHeadingBounds_ = ampColumn_.getBounds().reduced(16, 16).removeFromTop(35);
        ampPaintBounds_ = ampColumn_.getBounds().reduced(16, 16);

        {
            auto ampBounds = ampColumn_.getLocalBounds().reduced(16);
            ampBounds.removeFromTop(35 + 8);
            auto levelBlock = ampBounds.removeFromTop(147);
            ampKnobRowBounds_ = levelBlock.removeFromRight(132).reduced(0, 8);
            if (levelKnob_ != nullptr)
                levelKnob_->setBounds(ampKnobRowBounds_.removeFromLeft(56));
            ampKnobRowBounds_.removeFromLeft(8);
            if (panKnob_ != nullptr)
                panKnob_->setBounds(ampKnobRowBounds_.removeFromLeft(56));

            ampBounds.removeFromTop(16);
            auto unisonKnobRow = ampBounds.removeFromTop(103).removeFromBottom(40).reduced(10, 0);
            if (unisonDetuneKnob_ != nullptr)
                unisonDetuneKnob_->setBounds(unisonKnobRow.removeFromLeft(unisonKnobRow.getWidth() / 2).reduced(2, 0));
            if (unisonSpreadKnob_ != nullptr)
                unisonSpreadKnob_->setBounds(unisonKnobRow.reduced(2, 0));

            ampBounds.removeFromTop(103 - 40);
            ampRoutingBounds_ = ampBounds;
        }

        {
            auto oscBounds = oscColumn_.getLocalBounds().reduced(16);
            oscBounds.removeFromTop(35 + 16);
            operatorPanel_.setBounds(oscBounds);
        }
        {
            auto filterBounds = filterColumn_.getLocalBounds().reduced(16);
            filterBounds.removeFromTop(35 + 16);
            const int adsrHeight = 120;
            if (adsrMini_ != nullptr)
                adsrMini_->setBounds(filterBounds.removeFromBottom(adsrHeight));
            filterBounds.removeFromBottom(8);
            filterPanel_.setBounds(filterBounds);
        }

        nextEngineButton_.setBounds(footerBounds_.removeFromRight(96).withHeight(24).withY(footerBounds_.getY() + 12));
    }

    void EngineDetailOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop.withAlpha(0.92f));
    }

    void EngineDetailOverlay::paintOverChildren(juce::Graphics& g)
    {
        g.saveState();
        g.addTransform(contentHost_.getTransform());

        auto panel = juce::Rectangle<float>(static_cast<float>(kEngineDeepEditorOuterMargin),
                                            static_cast<float>(kEngineDeepEditorOuterMargin),
                                            static_cast<float>(kEngineDeepEditorFrameWidth - kEngineDeepEditorOuterMargin * 2),
                                            static_cast<float>(kEngineDeepEditorFrameHeight - kEngineDeepEditorOuterMargin * 2));
        draw::fillRecessedRoundedRect(g, panel, 10.0f);
        draw::strokeGlowPath(g, draw::roundedRectPath(panel, 10.0f), 0.35f, 1.0f, false);

        paintSectionHeading(g, oscHeadingBounds_, "OSCILLATOR SECTION", "ANALOG / WAVEFORM OSCILLATOR");
        paintSectionHeading(g, filterHeadingBounds_, "FILTER SECTION", "SURFACE & RESONANT FILTER");
        paintSectionHeading(g, ampHeadingBounds_, "AMPLIFIER SECTION", "LEVEL, UNISON & OUTPUT");

        for (const auto* col : {&oscColumn_, &filterColumn_, &ampColumn_})
        {
            g.setColour(palette::kBorder.withAlpha(0.35f));
            g.drawRoundedRectangle(col->getBounds().toFloat().reduced(0.5f), 8.0f, 1.0f);
        }

        paintAmpColumn(g, ampPaintBounds_);
        paintFooter(g, footerBounds_);

        g.restoreState();
    }

    void EngineDetailOverlay::paintSectionHeading(juce::Graphics& g, juce::Rectangle<int> bounds,
                                                  const juce::String& kicker, const juce::String& title) const
    {
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(kicker, bounds.removeFromTop(10), juce::Justification::centredLeft, true);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(12.0f));
        g.drawText(title, bounds, juce::Justification::centredLeft, true);
    }

    void EngineDetailOverlay::paintAmpColumn(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        auto area = bounds;
        area.removeFromTop(35 + 8);

        auto levelBlock = area.removeFromTop(147);
        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(levelBlock.toFloat(), 6.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("LEVEL", levelBlock.getX() + 12, levelBlock.getY() + 8, 40, 10, juce::Justification::centredLeft);

        const auto faderTrack = juce::Rectangle<float>(static_cast<float>(levelBlock.getX() + 15),
                                                       static_cast<float>(levelBlock.getY() + 22), 16.0f, 100.0f);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(faderTrack, 4.0f);

        const float levelNorm = [&]() {
            if (auto* raw = processor_.apvts.getRawParameterValue(
                    operatorParamId(static_cast<std::size_t>(engineIndex_), "Level")))
                return juce::jlimit(0.0f, 1.0f, raw->load() / 4.0f);
            return 0.75f;
        }();
        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.fillRoundedRectangle(faderTrack.withTrimmedTop(faderTrack.getHeight() * (1.0f - levelNorm)), 4.0f);

        const auto meterL = juce::Rectangle<float>(static_cast<float>(levelBlock.getX() + 63),
                                                   static_cast<float>(levelBlock.getY() + 15), 6.0f, 132.0f);
        const auto meterR = meterL.translated(10.0f, 0.0f);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(meterL, 2.0f);
        g.fillRoundedRectangle(meterR, 2.0f);
        g.setColour(palette::kAccent.withAlpha(0.75f));
        g.fillRoundedRectangle(meterL.withTrimmedTop(meterL.getHeight() * (1.0f - levelNorm * 0.9f)), 2.0f);
        g.fillRoundedRectangle(meterR.withTrimmedTop(meterR.getHeight() * (1.0f - levelNorm * 0.85f)), 2.0f);

        const auto& unison = processor_.getCurrentPatch().layerA.unison;
        const int unisonVoices = [&]() {
            if (auto* raw = processor_.apvts.getRawParameterValue(kUnisonVoicesId))
                return juce::jmax(1, static_cast<int>(raw->load() + 0.5f));
            return juce::jmax(1, unison.voices);
        }();
        const float detuneCents = [&]() {
            if (auto* raw = processor_.apvts.getRawParameterValue(kUnisonDetuneId))
                return raw->load();
            return unison.detuneCents;
        }();
        const float spread = [&]() {
            if (auto* raw = processor_.apvts.getRawParameterValue(kUnisonSpreadId))
                return raw->load();
            return unison.spread;
        }();

        area.removeFromTop(16);
        auto unisonCard = area.removeFromTop(103);
        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(unisonCard.toFloat(), 6.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("OSC UNISON ENGINE", unisonCard.getX() + 10, unisonCard.getY() + 14, 120, 10,
                   juce::Justification::centredLeft);

        static constexpr std::array<int, 4> kVoiceChoices{1, 4, 8, 16};
        int voiceX = unisonCard.getX() + 10;
        const int voiceY = unisonCard.getY() + 34;
        for (std::size_t i = 0; i < kVoiceChoices.size(); ++i)
        {
            const int voices = kVoiceChoices[i];
            const bool active = unisonVoices == voices;
            auto pill = juce::Rectangle<int>(voiceX, voiceY, 18, 14);
            unisonVoicePillBounds_[i] = pill;
            g.setColour(active ? palette::kAccent.withAlpha(0.22f) : palette::kBackgroundBottom);
            g.fillRoundedRectangle(pill.toFloat(), 3.0f);
            g.setColour(active ? palette::kAccent : palette::kTextDim);
            g.setFont(fonts::label(8.0f));
            g.drawText(juce::String(voices), pill, juce::Justification::centred, false);
            voiceX += 22;
        }

        const juce::String detuneText =
            juce::String(juce::roundToInt(juce::jlimit(0.0f, 100.0f, detuneCents * 0.5f))) + "% DETUNE";
        const juce::String spreadText = juce::String(juce::roundToInt(spread * 100.0f)) + "% SPREAD";
        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(8.0f));
        g.drawText(detuneText, unisonCard.getX() + 10, unisonCard.getY() + 58, 90, 10, juce::Justification::centredLeft);
        g.drawText(spreadText, unisonCard.getX() + 110, unisonCard.getY() + 58, 90, 10, juce::Justification::centredLeft);

        const juce::String modeLabel = [&]() {
            switch (unison.mode)
            {
                case patch::UnisonMode::Full: return "FULL MODE";
                case patch::UnisonMode::Operator: return "OPERATOR MODE";
                case patch::UnisonMode::Stereo: return "STEREO MODE";
                case patch::UnisonMode::Hyper: return "HYPER MODE";
                case patch::UnisonMode::Harmonic: return "HARMONIC MODE";
                case patch::UnisonMode::Off:
                default: return "CLASSIC MODE";
            }
        }();
        auto modePill = juce::Rectangle<int>(unisonCard.getX() + 10, unisonCard.getY() + 78, 88, 14);
        g.setColour(palette::kAccent.withAlpha(0.18f));
        g.fillRoundedRectangle(modePill.toFloat(), 3.0f);
        g.setColour(palette::kAccent);
        g.drawText(modeLabel, modePill, juce::Justification::centred, true);

        area.removeFromTop(16);
        g.setColour(palette::kTextDim);
        g.drawText("ROUTING SECTION", area.removeFromTop(10), juce::Justification::centredLeft, true);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(12.0f));
        g.drawText("MOD ASSIGNMENT MATRIX", area.removeFromTop(18), juce::Justification::centredLeft, true);

        area.removeFromTop(8);
        juce::StringArray routes;
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;
            routes.add(formatAmpRouteRow(route));
            if (routes.size() >= 4)
                break;
        }
        if (routes.isEmpty())
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(9.0f));
            g.drawText("No active mod routes", area.removeFromTop(23).reduced(10, 0),
                       juce::Justification::centredLeft, true);
            return;
        }

        for (const auto& route : routes)
        {
            auto row = area.removeFromTop(23);
            g.setColour(palette::kPanelRaised);
            g.fillRoundedRectangle(row.toFloat(), 4.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(9.0f));
            g.drawText(route, row.reduced(10, 0), juce::Justification::centredLeft, true);
            area.removeFromTop(4);
        }
    }

    void EngineDetailOverlay::paintFooter(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawHorizontalLine(bounds.getY(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

        const auto metrics = readPerformanceMetrics(processor_);
        const auto& patch = processor_.getCurrentPatch();
        const juce::String presetName = patch.metadata.name.empty() ? juce::String("INIT PATCH")
                                                                    : juce::String(patch.metadata.name);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        g.drawText("ACTIVE PRESET:", bounds.getX() + 16, bounds.getY() + 14, 70, 10, juce::Justification::centredLeft);

        auto badge = juce::Rectangle<float>(static_cast<float>(bounds.getX() + 90), static_cast<float>(bounds.getY() + 12),
                                            108.0f, 19.0f);
        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(palette::kAccent);
        g.setFont(fonts::label(10.0f));
        g.drawText(presetName.toUpperCase(), badge.toNearestInt(), juce::Justification::centred, true);

        const int centreX = bounds.getCentreX() - 140;
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        g.drawText("ENGINE CPU", centreX, bounds.getY() + 14, 50, 10, juce::Justification::centredLeft);
        const auto cpuTrack = juce::Rectangle<float>(static_cast<float>(centreX + 55), static_cast<float>(bounds.getY() + 17),
                                                   50.0f, 4.0f);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(cpuTrack, 2.0f);
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(cpuTrack.withWidth(cpuTrack.getWidth() * cpuBarFillRatio(metrics.cpuPercent)), 2.0f);
        g.drawText(formatCpuPercent(metrics.cpuPercent), centreX + 111, bounds.getY() + 14, 24, 10,
                   juce::Justification::centredLeft);

        g.setColour(palette::kTextDim);
        g.drawText("VOICES: " + formatVoiceCount(metrics.activeVoices, metrics.maxVoices), centreX + 144,
                   bounds.getY() + 14, 80, 10, juce::Justification::centredLeft);

        g.setColour(palette::kAccent);
        g.fillEllipse(static_cast<float>(centreX + 230), static_cast<float>(bounds.getY() + 16), 6.0f, 6.0f);
        g.setColour(palette::kTextDim);
        g.drawText("MIDI ACTIVE", centreX + 242, bounds.getY() + 14, 60, 10, juce::Justification::centredLeft);
    }

    void EngineDetailOverlay::resized()
    {
        const float scale =
            juce::jmin(getWidth() / static_cast<float>(kEngineDeepEditorFrameWidth),
                       getHeight() / static_cast<float>(kEngineDeepEditorFrameHeight));
        contentHost_.setBounds(0, 0, kEngineDeepEditorFrameWidth, kEngineDeepEditorFrameHeight);
        contentHost_.setTransform(
            juce::AffineTransform::scale(scale)
                .translated((getWidth() - kEngineDeepEditorFrameWidth * scale) * 0.5f,
                            (getHeight() - kEngineDeepEditorFrameHeight * scale) * 0.5f));
        layoutDesignSurface();
    }

    void EngineDetailOverlay::mouseDown(const juce::MouseEvent& event)
    {
        const juce::Point<int> local = contentHost_.getLocalPoint(this, event.getPosition());
        static constexpr std::array<int, 4> kVoiceChoices{1, 4, 8, 16};
        for (std::size_t i = 0; i < unisonVoicePillBounds_.size(); ++i)
        {
            if (!unisonVoicePillBounds_[i].contains(local))
                continue;
            if (auto* param = processor_.apvts.getParameter(kUnisonVoicesId))
                param->setValueNotifyingHost(
                    param->convertTo0to1(static_cast<float>(kVoiceChoices[i])));
            repaint();
            return;
        }
    }

    void EngineDetailOverlay::timerCallback()
    {
        if (isVisible())
            repaint();
    }

    bool EngineDetailOverlay::keyPressed(const juce::KeyPress& key)
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
