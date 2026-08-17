#include "VstBottomBar.h"

#include "../PerformanceMetricsUi.h"
#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "LabLauncherChip.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    VstBottomBar::VstBottomBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        statusLabel_.setFont(fonts::label(8.0f));
        statusLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        statusLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(statusLabel_);

        voicesLabel_.setFont(fonts::label(8.0f));
        voicesLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        voicesLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(voicesLabel_);

        vocoderChip_ = std::make_unique<LabLauncherChip>();
        vocoderChip_->setLabel("VOCODER");
        vocoderChip_->setIcon(LabLauncherIcon::Vocoder);
        vocoderChip_->setHighlighted(true);
        vocoderChip_->onClick = [this] {
            if (onVocoderLabRequested)
                onVocoderLabRequested();
        };
        addAndMakeVisible(*vocoderChip_);

        lfoChip_ = std::make_unique<LabLauncherChip>();
        lfoChip_->setLabel("LFO 1·2");
        lfoChip_->setAccentColour(juce::Colour(0xff9f80ff));
        lfoChip_->setIcon(LabLauncherIcon::Lfo);
        lfoChip_->onClick = [this] {
            if (onLfoLabRequested)
                onLfoLabRequested();
        };
        addAndMakeVisible(*lfoChip_);

        modChip_ = std::make_unique<LabLauncherChip>();
        modChip_->setLabel("MOD");
        modChip_->setAccentColour(palette::kMurmurViolet);
        modChip_->setIcon(LabLauncherIcon::Mod);
        modChip_->onClick = [this] {
            if (onModMatrixRequested)
                onModMatrixRequested();
        };
        addAndMakeVisible(*modChip_);

        panicButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        panicButton_.setColour(juce::TextButton::textColourOffId, palette::kAccentWarm);
        panicButton_.onClick = [this] { processor_.panicAllNotes(); };
        addAndMakeVisible(panicButton_);

        latchButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        latchButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        latchButton_.setVisible(false);
        addChildComponent(latchButton_);

        sustainLabel_.setFont(fonts::label(9.0f));
        sustainLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        sustainLabel_.setVisible(false);
        addChildComponent(sustainLabel_);

        midiLabel_.setFont(fonts::label(9.0f));
        midiLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        midiLabel_.setVisible(false);
        addChildComponent(midiLabel_);

        routeLabel_.setFont(fonts::label(9.0f));
        routeLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        routeLabel_.setJustificationType(juce::Justification::centred);
        routeLabel_.setVisible(false);
        addChildComponent(routeLabel_);

        cpuLabel_.setFont(fonts::label(8.0f));
        cpuLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        cpuLabel_.setJustificationType(juce::Justification::centredLeft);
        cpuLabel_.setVisible(false);
        addChildComponent(cpuLabel_);

        fxLoadLabel_.setFont(fonts::label(8.0f));
        fxLoadLabel_.setColour(juce::Label::textColourId, palette::kAccentWarm);
        fxLoadLabel_.setJustificationType(juce::Justification::centredLeft);
        fxLoadLabel_.setVisible(false);
        addChildComponent(fxLoadLabel_);

        modSourcesLabel_.setFont(fonts::label(8.0f));
        modSourcesLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        modSourcesLabel_.setText("MOD SOURCES:", juce::dontSendNotification);
        modSourcesLabel_.setVisible(false);
        addChildComponent(modSourcesLabel_);

        for (auto& chip : modSourceChips_)
        {
            chip = std::make_unique<juce::Label>();
            chip->setVisible(false);
            addChildComponent(*chip);
        }

        startTimerHz(8);
    }

    VstBottomBar::~VstBottomBar() { stopTimer(); }

    void VstBottomBar::setDesignModeV2Layout(bool designMode)
    {
        designModeV2Layout_ = designMode;
        if (designMode)
        {
            playBoardMode_ = false;
            desktopPlayMode_ = false;
            vocoderChip_->setVisible(true);
            lfoChip_->setVisible(true);
            modChip_->setVisible(true);
        }

        statusLabel_.setVisible(!designMode);
        if (!designMode)
        {
            vocoderChip_->setVisible(true);
            lfoChip_->setVisible(true);
            modChip_->setVisible(true);
        }
        latchButton_.setVisible(false);
        sustainLabel_.setVisible(false);
        routeLabel_.setVisible(false);

        cpuLabel_.setVisible(designMode);
        fxLoadLabel_.setVisible(designMode);
        voicesLabel_.setVisible(true);
        modSourcesLabel_.setVisible(designMode);
        for (auto& chip : modSourceChips_)
            chip->setVisible(false);
        panicButton_.setVisible(true);
        if (designMode)
            panicButton_.setButtonText("PANIC RESET");

        resized();
    }

    void VstBottomBar::setPlayBoardMode(bool playBoardMode)
    {
        playBoardMode_ = playBoardMode;
        if (playBoardMode)
        {
            desktopPlayMode_ = false;
            designModeV2Layout_ = false;
        }
        modChip_->setVisible(!playBoardMode_ && !desktopPlayMode_);
        panicButton_.setVisible(!playBoardMode_ && !desktopPlayMode_);
        latchButton_.setVisible(desktopPlayMode_);
        sustainLabel_.setVisible(desktopPlayMode_);
        midiLabel_.setVisible(desktopPlayMode_);
        routeLabel_.setVisible(desktopPlayMode_);
        vocoderChip_->setVisible(!desktopPlayMode_);
        lfoChip_->setVisible(!desktopPlayMode_);
        resized();
    }

    void VstBottomBar::setDesktopPlayMode(bool desktopPlayMode)
    {
        desktopPlayMode_ = desktopPlayMode;
        if (desktopPlayMode)
        {
            playBoardMode_ = false;
            designModeV2Layout_ = false;
        }
        modChip_->setVisible(!playBoardMode_ && !desktopPlayMode_);
        panicButton_.setVisible(desktopPlayMode_ || (!playBoardMode_ && !desktopPlayMode_));
        latchButton_.setVisible(desktopPlayMode_);
        sustainLabel_.setVisible(desktopPlayMode_);
        midiLabel_.setVisible(desktopPlayMode_);
        routeLabel_.setVisible(desktopPlayMode_);
        vocoderChip_->setVisible(!desktopPlayMode_);
        lfoChip_->setVisible(!desktopPlayMode_);
        if (desktopPlayMode_)
        {
            panicButton_.setButtonText("PANIC RESET");
            latchButton_.setButtonText("HOLD / LATCH [ON]");
        }
        else
        {
            panicButton_.setButtonText("PANIC");
        }
        resized();
    }

    void VstBottomBar::timerCallback()
    {
        const auto metrics = readPerformanceMetrics(processor_);
        const bool sc = processor_.getSidechainActive();
        juce::String status = "MURMUR v" + juce::String(JucePlugin_VersionString);
        status += sc ? " · Side Chain active · Master summed"
                     : (playBoardMode_ ? " · 8 ENGINES · V/L labs · MIDI IN"
                                       : " · 8 ENGINES · V/L/M labs · MIDI IN");
        statusLabel_.setText(status, juce::dontSendNotification);
        const juce::String voiceText = formatVoiceCount(metrics.activeVoices, metrics.maxVoices);
        voicesLabel_.setText(designModeV2Layout_ ? voiceText
                                                 : "VOICES: " + voiceText,
                             juce::dontSendNotification);

        cpuLoadPercent_ = metrics.cpuPercent;
        fxLoadPercent_ = estimateFxLoadPercent(processor_.apvts);

        if (designModeV2Layout_)
        {
            cpuLabel_.setText(formatCpuPercent(metrics.cpuPercent), juce::dontSendNotification);
            fxLoadLabel_.setText(formatFxLoadPercent(fxLoadPercent_), juce::dontSendNotification);
            midiLabel_.setVisible(true);
            midiLabel_.setText("MIDI IN", juce::dontSendNotification);
            repaint(cpuBarBounds_);
            repaint(fxLoadBarBounds_);
            repaint();
            return;
        }

        if (desktopPlayMode_)
        {
            sustainLabel_.setText("SUSTAIN", juce::dontSendNotification);
            midiLabel_.setText("MIDI IN [CH 1]", juce::dontSendNotification);
            routeLabel_.setText("MAIN OUT", juce::dontSendNotification);
            repaint();
        }
    }

    bool VstBottomBar::modSourceActive(modulation::ModSource source) const
    {
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive() && route.source == source)
                return true;
        }
        return false;
    }

    void VstBottomBar::paintDesignModeModChips(juce::Graphics& g) const
    {
        static const struct
        {
            const char* label;
            juce::Colour colour;
            modulation::ModSource source;
        } kChips[] = {
            {"LFO1", palette::kAccent, modulation::ModSource::Lfo1},
            {"LFO2", juce::Colour(0xff7f9fe0), modulation::ModSource::Lfo2},
            {"ENV1", juce::Colour(0xff8f7fe0), modulation::ModSource::Env1},
            {"ENV2", juce::Colour(0xffe0c17f), modulation::ModSource::Env2},
        };

        g.setFont(fonts::label(7.0f));
        for (std::size_t i = 0; i < modChipBounds_.size(); ++i)
        {
            const auto chip = modChipBounds_[i].toFloat();
            if (chip.isEmpty())
                continue;

            const auto& spec = kChips[i];
            const bool active = modSourceActive(spec.source);
            g.setColour(spec.colour.withAlpha(active ? 0.16f : 0.08f));
            g.fillRoundedRectangle(chip, 4.0f);
            g.setColour(spec.colour.withAlpha(active ? 1.0f : 0.82f));
            g.drawRoundedRectangle(chip.reduced(0.5f), 4.0f, 1.0f);
            g.setColour(active ? palette::kTextPrimary : palette::kTextDim);
            g.drawText(spec.label, chip, juce::Justification::centred);
        }
    }

    void VstBottomBar::paintOverChildren(juce::Graphics& g)
    {
        if (!designModeV2Layout_)
            return;

        paintDesignModeModChips(g);

        if (!cpuBarBounds_.isEmpty())
        {
            auto barArea = cpuBarBounds_.toFloat();
            g.setColour(juce::Colour(0xff0d0f14));
            g.fillRoundedRectangle(barArea, 2.0f);
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(barArea.getX(), barArea.getY(),
                                   barArea.getWidth() * cpuBarFillRatio(cpuLoadPercent_), barArea.getHeight(), 2.0f);
        }

        if (!fxLoadBarBounds_.isEmpty())
        {
            auto barArea = fxLoadBarBounds_.toFloat();
            g.setColour(juce::Colour(0xff0d0f14));
            g.fillRoundedRectangle(barArea, 2.0f);
            g.setColour(palette::kAccentWarm);
            g.fillRoundedRectangle(barArea.getX(), barArea.getY(),
                                   barArea.getWidth() * cpuBarFillRatio(fxLoadPercent_), barArea.getHeight(), 2.0f);
        }

        auto midiArea = midiLabel_.getBounds();
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.fillEllipse(static_cast<float>(midiArea.getX() - 12), static_cast<float>(midiArea.getCentreY() - 3), 6.0f,
                      6.0f);
    }

    void VstBottomBar::mouseDown(const juce::MouseEvent& event)
    {
        if (!designModeV2Layout_)
            return;

        for (std::size_t i = 0; i < 2; ++i)
        {
            if (modChipBounds_[i].contains(event.getPosition()))
            {
                if (onLfoLabRequested)
                    onLfoLabRequested();
                return;
            }
        }
    }

    void VstBottomBar::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const float radius = desktopPlayMode_ ? 10.0f : (designModeV2Layout_ ? 8.0f : 6.0f);
        g.setColour(palette::kPanelRaised.withAlpha(0.92f));
        g.fillRoundedRectangle(bounds, radius);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);

        if (designModeV2Layout_)
        {
            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextDim);
            const int padX = layout::kDesignModeV2StatusBarPaddingX;
            const int y = static_cast<int>(bounds.getY()) + 15;
            g.drawText("CPU", padX, y, 15, 10, juce::Justification::centredLeft, true);
            g.drawText("FX", padX + 89, y, 10, 10, juce::Justification::centredLeft, true);
            g.drawText("VOICES:", padX + 170, y, 34, 10, juce::Justification::centredLeft, true);
            return;
        }

        if (!desktopPlayMode_)
            return;

        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextSecondary);
        const int padX = layout::kDesktopPlayModeBottomBarPaddingX;
        const int indicatorY = static_cast<int>(bounds.getY()) + 17;
        g.drawText("ENGINES", padX, indicatorY + 9, 40, 12, juce::Justification::centredLeft, true);

        static constexpr const char* kEngineLabels[] = {"SIN", "SAW", "TRI", "SQR", "NOI", "PAD", "FMB", "ORG"};
        int x = padX + 56;
        for (int i = 0; i < 8; ++i)
        {
            const auto& op = processor_.getCurrentPatch().layerA.operators[static_cast<std::size_t>(i)];
            const bool active = op.mixEnabled && !op.mixMute;

            const auto ledBox = juce::Rectangle<float>(static_cast<float>(x + 16), static_cast<float>(indicatorY),
                                                       16.0f, 16.0f);
            g.setColour(palette::kPanelRaised.brighter(0.08f));
            g.fillRoundedRectangle(ledBox, 3.0f);
            if (active)
            {
                g.setColour(palette::kAccent);
                g.fillRoundedRectangle(ledBox.reduced(5.0f), 1.0f);
            }
            g.setColour(palette::kTextDim);
            g.drawText(kEngineLabels[i], static_cast<float>(x + 16), static_cast<float>(indicatorY + 20), 15.0f, 10.0f,
                       juce::Justification::centred, true);
            x += layout::kDesktopPlayModeEngineIndicatorWidth + layout::kDesktopPlayModeEngineIndicatorGap;
        }
    }

    void VstBottomBar::resized()
    {
        if (designModeV2Layout_)
        {
            auto row = getLocalBounds().reduced(layout::kDesignModeV2StatusBarPaddingX, 11);
            panicButton_.setBounds(row.removeFromRight(73).withHeight(18));
            row.removeFromRight(16);

            auto modRow = row.withSizeKeepingCentre(212, 14);
            modRow = modRow.withX(row.getCentreX() - modRow.getWidth() / 2);
            modSourcesLabel_.setBounds(modRow.removeFromLeft(64));
            modRow.removeFromLeft(6);
            for (std::size_t i = 0; i < modChipBounds_.size(); ++i)
            {
                modChipBounds_[i] = modRow.removeFromLeft(30).withHeight(14).withY(modRow.getY());
                modRow.removeFromLeft(6);
            }

            auto cpuCluster = row.removeFromLeft(89);
            const int metricY = cpuCluster.getY() + 4;
            cpuBarBounds_ = juce::Rectangle<int>(cpuCluster.getX() + 21, metricY + 6, 40, 4);
            cpuLabel_.setBounds(cpuCluster.getX() + 67, metricY, 22, 10);
            row.removeFromLeft(16);

            auto fxCluster = row.removeFromLeft(81);
            const int fxMetricY = fxCluster.getY() + 4;
            fxLoadBarBounds_ = juce::Rectangle<int>(fxCluster.getX() + 16, fxMetricY + 6, 40, 4);
            fxLoadLabel_.setBounds(fxCluster.getX() + 59, fxMetricY, 22, 10);
            row.removeFromLeft(16);

            voicesLabel_.setBounds(row.removeFromLeft(65).withHeight(10).withY(row.getY() + 4));
            row.removeFromLeft(16);
            midiLabel_.setBounds(row.removeFromLeft(40).withHeight(10).withY(row.getY() + 4));
            return;
        }

        if (desktopPlayMode_)
        {
            auto row = getLocalBounds().reduced(layout::kDesktopPlayModeBottomBarPaddingX, 15);
            auto rightBlock = row.removeFromRight(235);
            routeLabel_.setBounds(rightBlock.removeFromRight(103).reduced(0, 0));
            rightBlock.removeFromRight(20);
            midiLabel_.setBounds(rightBlock.removeFromLeft(92).reduced(0, 6));

            auto actions = row.withSizeKeepingCentre(313, layout::kDesktopPlayModeStageActionHeight);
            actions = actions.withX(row.getCentreX() - actions.getWidth() / 2);
            latchButton_.setBounds(actions.removeFromLeft(layout::kDesktopPlayModeLatchButtonWidth));
            actions.removeFromLeft(layout::kDesktopPlayModeStageActionGap);
            panicButton_.setBounds(actions.removeFromLeft(layout::kDesktopPlayModePanicButtonWidth));
            actions.removeFromLeft(layout::kDesktopPlayModeStageActionGap);
            sustainLabel_.setBounds(actions);
            return;
        }

        auto row = getLocalBounds().reduced(layout::kVstBottomBarPaddingX, 4);
        if (!playBoardMode_)
        {
            panicButton_.setBounds(row.removeFromRight(52).reduced(1));
            row.removeFromRight(6);
            modChip_->setBounds(row.removeFromRight(layout::kLabChipWidth).reduced(1));
            row.removeFromRight(4);
        }

        const int chipW = layout::kLabChipWidth;
        lfoChip_->setBounds(row.removeFromRight(chipW + 4).reduced(1));
        row.removeFromRight(4);
        vocoderChip_->setBounds(row.removeFromRight(chipW + 8).reduced(1));

        voicesLabel_.setBounds(row.removeFromRight(96));
        row.removeFromRight(8);
        statusLabel_.setBounds(row);
    }

} // namespace pw8::plugin::ui
