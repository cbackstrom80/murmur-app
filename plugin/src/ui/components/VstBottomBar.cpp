#include "VstBottomBar.h"

#include <optional>

#include "../PerformanceMetricsUi.h"
#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "LabLauncherChip.h"
#include "state/PluginState.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    VstBottomBar::VstBottomBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        statusLabel_.setFont(fonts::label(layout::kIpadPlayCaptionSize));
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

        quasarChip_ = std::make_unique<LabLauncherChip>();
        quasarChip_->setLabel("QUASAR");
        quasarChip_->setAccentColour(juce::Colour(0xff7c4dff));
        quasarChip_->setHighlighted(true);
        quasarChip_->onClick = [this] {
            if (onQuasarLabRequested)
                onQuasarLabRequested();
        };
        addChildComponent(*quasarChip_);

        lfoChip_ = std::make_unique<LabLauncherChip>();
        lfoChip_->setLabel("LFO 1·2");
        lfoChip_->setAccentColour(juce::Colour(0xff9f80ff));
        lfoChip_->setIcon(LabLauncherIcon::Lfo);
        lfoChip_->onClick = [this] {
            if (onLfoLabRequested)
                onLfoLabRequested();
        };
        addAndMakeVisible(*lfoChip_);

        motionChip_ = std::make_unique<LabLauncherChip>();
        motionChip_->setLabel("MOTION");
        motionChip_->setAccentColour(palette::kFigmaTeal);
        motionChip_->setIcon(LabLauncherIcon::Lfo);
        motionChip_->onClick = [this] {
            if (onMotionLabRequested)
                onMotionLabRequested();
        };
        addChildComponent(*motionChip_);

        morphEditorChip_ = std::make_unique<LabLauncherChip>();
        morphEditorChip_->setLabel("MORPH");
        morphEditorChip_->setAccentColour(palette::kFigmaTeal);
        morphEditorChip_->setIcon(LabLauncherIcon::Lfo);
        morphEditorChip_->onClick = [this] {
            if (onMorphEditorRequested)
                onMorphEditorRequested();
        };
        addChildComponent(*morphEditorChip_);

        filterLabChip_ = std::make_unique<LabLauncherChip>();
        filterLabChip_->setLabel("FILTER");
        filterLabChip_->setAccentColour(palette::kAccent);
        filterLabChip_->setIcon(LabLauncherIcon::Lfo);
        filterLabChip_->onClick = [this] {
            if (onFilterLabRequested)
                onFilterLabRequested();
        };
        addChildComponent(*filterLabChip_);

        dynamicsLabChip_ = std::make_unique<LabLauncherChip>();
        dynamicsLabChip_->setLabel("DYN");
        dynamicsLabChip_->setAccentColour(palette::kMurmurViolet);
        dynamicsLabChip_->setIcon(LabLauncherIcon::Lfo);
        dynamicsLabChip_->onClick = [this] {
            if (onDynamicsLabRequested)
                onDynamicsLabRequested();
        };
        addChildComponent(*dynamicsLabChip_);

        generativeLabChip_ = std::make_unique<LabLauncherChip>();
        generativeLabChip_->setLabel("GEN");
        generativeLabChip_->setAccentColour(palette::kFigmaTeal);
        generativeLabChip_->setIcon(LabLauncherIcon::Mod);
        generativeLabChip_->onClick = [this] {
            if (onGenerativeLabRequested)
                onGenerativeLabRequested();
        };
        addChildComponent(*generativeLabChip_);

        utilityPeaksChip_ = std::make_unique<LabLauncherChip>();
        utilityPeaksChip_->setLabel("PEAKS");
        utilityPeaksChip_->setAccentColour(palette::kAccent);
        utilityPeaksChip_->setIcon(LabLauncherIcon::Lfo);
        utilityPeaksChip_->onClick = [this] {
            if (onUtilityPeaksRequested)
                onUtilityPeaksRequested();
        };
        addChildComponent(*utilityPeaksChip_);

        segmentsLabChip_ = std::make_unique<LabLauncherChip>();
        segmentsLabChip_->setLabel("SEG");
        segmentsLabChip_->setAccentColour(palette::kFigmaTeal);
        segmentsLabChip_->setIcon(LabLauncherIcon::Lfo);
        segmentsLabChip_->onClick = [this] {
            if (onEnvelopeSegmentsRequested)
                onEnvelopeSegmentsRequested();
        };
        addChildComponent(*segmentsLabChip_);

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

        latchButton_.setClickingTogglesState(true);
        latchButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        latchButton_.setColour(juce::TextButton::buttonOnColourId, palette::kAccent.withAlpha(0.22f));
        latchButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        latchButton_.setColour(juce::TextButton::textColourOnId, palette::kAccent);
        latchButton_.setVisible(false);
        addChildComponent(latchButton_);
        latchAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor_.apvts, juce::String(kArpIdPrefix) + "Latch", latchButton_);

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

        cpuLabel_.setFont(fonts::label(layout::kIpadPlayCaptionSize));
        cpuLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        cpuLabel_.setJustificationType(juce::Justification::centredLeft);
        cpuLabel_.setVisible(false);
        addChildComponent(cpuLabel_);

        bpmLabel_.setFont(fonts::label(layout::kIpadPlayLabelSize));
        bpmLabel_.setColour(juce::Label::textColourId, palette::kFigmaTeal);
        bpmLabel_.setJustificationType(juce::Justification::centredLeft);
        bpmLabel_.setVisible(false);
        addChildComponent(bpmLabel_);

        fxLoadLabel_.setFont(fonts::label(layout::kIpadPlayCaptionSize));
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
            ipadPlayMode_ = false;
            murmurBasicViewMode_ = false;
            // Secondary labs only — FILTER/DYN/GEN/PEAKS/SEG/MOD are in the header sub-nav.
            vocoderChip_->setVisible(true);
            lfoChip_->setVisible(true);
            lfoChip_->setLabel("LFO");
            morphEditorChip_->setVisible(true);
            filterLabChip_->setVisible(false);
            dynamicsLabChip_->setVisible(false);
            generativeLabChip_->setVisible(false);
            utilityPeaksChip_->setVisible(false);
            segmentsLabChip_->setVisible(false);
            modChip_->setVisible(false);
            motionChip_->setVisible(false);
        }

        statusLabel_.setVisible(!designMode);
        if (!designMode)
        {
            vocoderChip_->setVisible(true);
            lfoChip_->setVisible(true);
            lfoChip_->setLabel("LFO 1-2");
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

    void VstBottomBar::setMurmurBasicViewMode(bool murmurBasicViewMode)
    {
        murmurBasicViewMode_ = murmurBasicViewMode;
        if (murmurBasicViewMode)
        {
            playBoardMode_ = false;
            desktopPlayMode_ = false;
            ipadPlayMode_ = false;
            designModeV2Layout_ = false;
            murmurBasicViewMode_ = false;
        }

        vocoderChip_->setVisible(false);
        lfoChip_->setVisible(false);
        motionChip_->setVisible(false);
        morphEditorChip_->setVisible(false);
        filterLabChip_->setVisible(false);
        dynamicsLabChip_->setVisible(false);
        generativeLabChip_->setVisible(false);
        utilityPeaksChip_->setVisible(false);
        segmentsLabChip_->setVisible(false);
        modChip_->setVisible(false);
        latchButton_.setVisible(false);
        sustainLabel_.setVisible(false);
        midiLabel_.setVisible(false);
        routeLabel_.setVisible(false);
        modSourcesLabel_.setVisible(false);
        fxLoadLabel_.setVisible(false);
        bpmLabel_.setVisible(false);
        statusLabel_.setVisible(murmurBasicViewMode_);
        cpuLabel_.setVisible(murmurBasicViewMode_);
        voicesLabel_.setVisible(murmurBasicViewMode_);
        panicButton_.setVisible(murmurBasicViewMode_);
        if (murmurBasicViewMode_)
            panicButton_.setButtonText("PANIC RESET");
        resized();
    }

    void VstBottomBar::setPlayBoardMode(bool playBoardMode)
    {
        playBoardMode_ = playBoardMode;
        if (playBoardMode)
        {
            desktopPlayMode_ = false;
            ipadPlayMode_ = false;
            designModeV2Layout_ = false;
            murmurBasicViewMode_ = false;
        }
        modChip_->setVisible(!playBoardMode_ && !desktopPlayMode_ && !ipadPlayMode_);
        panicButton_.setVisible(!playBoardMode_ && !desktopPlayMode_ && !ipadPlayMode_);
        latchButton_.setVisible(desktopPlayMode_);
        sustainLabel_.setVisible(desktopPlayMode_);
        midiLabel_.setVisible(desktopPlayMode_);
        routeLabel_.setVisible(desktopPlayMode_);
        vocoderChip_->setVisible(!desktopPlayMode_ && !ipadPlayMode_);
        quasarChip_->setVisible(playBoardMode_);
        lfoChip_->setVisible(!desktopPlayMode_ && !ipadPlayMode_);
        motionChip_->setVisible(playBoardMode_);
        morphEditorChip_->setVisible(false);
        resized();
    }

    void VstBottomBar::setDesktopPlayMode(bool desktopPlayMode)
    {
        desktopPlayMode_ = desktopPlayMode;
        if (desktopPlayMode)
        {
            playBoardMode_ = false;
            ipadPlayMode_ = false;
            designModeV2Layout_ = false;
            murmurBasicViewMode_ = false;
        }
        modChip_->setVisible(!playBoardMode_ && !desktopPlayMode_ && !ipadPlayMode_);
        panicButton_.setVisible(desktopPlayMode_ || ipadPlayMode_ || (!playBoardMode_ && !desktopPlayMode_ && !ipadPlayMode_));
        latchButton_.setVisible(desktopPlayMode_);
        sustainLabel_.setVisible(desktopPlayMode_);
        midiLabel_.setVisible(desktopPlayMode_);
        routeLabel_.setVisible(desktopPlayMode_);
        vocoderChip_->setVisible(!desktopPlayMode_ && !ipadPlayMode_);
        quasarChip_->setVisible(playBoardMode_);
        lfoChip_->setVisible(!desktopPlayMode_ && !ipadPlayMode_);
        motionChip_->setVisible(playBoardMode_);
        morphEditorChip_->setVisible(false);
        statusLabel_.setVisible(ipadPlayMode_);
        voicesLabel_.setVisible(!desktopPlayMode_ && !ipadPlayMode_ && !designModeV2Layout_);
        cpuLabel_.setVisible(ipadPlayMode_ || designModeV2Layout_);
        bpmLabel_.setVisible(ipadPlayMode_);
        fxLoadLabel_.setVisible(designModeV2Layout_);
        if (desktopPlayMode_)
        {
            panicButton_.setButtonText("PANIC RESET");
        }
        else if (!ipadPlayMode_)
        {
            panicButton_.setButtonText("PANIC");
        }
        resized();
    }

    void VstBottomBar::setIpadFooterPillActive(layout::IpadFooterPill pill)
    {
        activeIpadFooterPill_ = pill;
        repaint();
    }

    void VstBottomBar::setIpadPlayMode(bool ipadPlayMode)
    {
        ipadPlayMode_ = ipadPlayMode;
        if (ipadPlayMode)
        {
            playBoardMode_ = false;
            desktopPlayMode_ = false;
            designModeV2Layout_ = false;
            murmurBasicViewMode_ = false;
        }
        modChip_->setVisible(!playBoardMode_ && !desktopPlayMode_ && !ipadPlayMode_);
        panicButton_.setVisible(desktopPlayMode_ || ipadPlayMode_ || (!playBoardMode_ && !desktopPlayMode_ && !ipadPlayMode_));
        panicButton_.setButtonText(ipadPlayMode_ ? "PANIC RESET" : (desktopPlayMode_ ? "PANIC RESET" : "PANIC"));
        latchButton_.setVisible(desktopPlayMode_);
        sustainLabel_.setVisible(desktopPlayMode_);
        midiLabel_.setVisible(desktopPlayMode_ || designModeV2Layout_);
        routeLabel_.setVisible(desktopPlayMode_);
        vocoderChip_->setVisible(!desktopPlayMode_ && !ipadPlayMode_);
        lfoChip_->setVisible(!desktopPlayMode_ && !ipadPlayMode_);
        motionChip_->setVisible(false);
        morphEditorChip_->setVisible(false);
        statusLabel_.setVisible(ipadPlayMode_);
        voicesLabel_.setVisible(false);
        cpuLabel_.setVisible(ipadPlayMode_ || designModeV2Layout_);
        bpmLabel_.setVisible(ipadPlayMode_);
        fxLoadLabel_.setVisible(designModeV2Layout_);
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

        if (ipadPlayMode_)
        {
            const bool sidechainActive = processor_.getSidechainActive();
            statusLabel_.setText(sidechainActive ? "SIDECHAIN: ON" : "SIDECHAIN: OFF", juce::dontSendNotification);
            cpuLabel_.setText(formatCpuPercent(metrics.cpuPercent), juce::dontSendNotification);
            const float bpm = processor_.getHostBpm();
            const juce::String playState = processor_.getHostIsPlaying() ? "PLAY" : "STOP";
            bpmLabel_.setText(juce::String(bpm, 1) + " BPM · HOST · " + playState, juce::dontSendNotification);
            repaint(cpuBarBounds_);
            repaint();
            return;
        }

        if (desktopPlayMode_)
        {
            const bool latched = latchButton_.getToggleState();
            latchButton_.setButtonText(latched ? "HOLD / LATCH [ON]" : "HOLD / LATCH [OFF]");

            const bool sustainHeld = processor_.isSustainPedalHeld(0);
            sustainLabel_.setText(sustainHeld ? "SUSTAIN [ON]" : "SUSTAIN", juce::dontSendNotification);
            sustainLabel_.setColour(juce::Label::textColourId,
                                    sustainHeld ? palette::kAccent : palette::kTextSecondary);

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

    void VstBottomBar::paintIpadFooterPills(juce::Graphics& g) const
    {
        static constexpr const char* kLabels[] = {"PLAY", "DESIGN", "VOC", "ARP", "LFO"};
        g.setFont(fonts::label(layout::kIpadPlayLabelSize));

        for (std::size_t i = 0; i < ipadFooterPillBounds_.size(); ++i)
        {
            const auto bounds = ipadFooterPillBounds_[i].toFloat();
            if (bounds.isEmpty())
                continue;

            const bool active = static_cast<int>(activeIpadFooterPill_) == static_cast<int>(i);
            g.setColour(active ? palette::kFigmaTeal.withAlpha(0.22f) : palette::kPanelRaised.withAlpha(0.95f));
            g.fillRoundedRectangle(bounds, 19.0f);
            g.setColour(active ? palette::kFigmaTeal.withAlpha(0.95f) : palette::kBorder.withAlpha(0.75f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), 19.0f, 1.0f);
            g.setColour(active ? palette::kFigmaTextPrimary : palette::kTextSecondary);
            g.drawText(kLabels[i], bounds, juce::Justification::centred, true);
        }
    }

    std::optional<layout::IpadFooterPill> VstBottomBar::footerPillAt(juce::Point<int> point) const
    {
        for (std::size_t i = 0; i < ipadFooterPillBounds_.size(); ++i)
        {
            if (ipadFooterPillBounds_[i].contains(point))
                return static_cast<layout::IpadFooterPill>(i);
        }
        return std::nullopt;
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

        g.setFont(fonts::micro(7.0f));
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
        if (ipadPlayMode_)
            paintIpadFooterPills(g);

        if (ipadPlayMode_ && !cpuBarBounds_.isEmpty())
        {
            g.setColour(juce::Colour(0xff0a0b0e));
            g.fillRoundedRectangle(cpuBarBounds_.toFloat(), 3.0f);
            auto fill = cpuBarBounds_.toFloat().reduced(1.0f);
            fill.setWidth(fill.getWidth() * juce::jlimit(0.0f, 1.0f, cpuLoadPercent_ / 100.0f));
            g.setColour(palette::kAccent.withAlpha(0.85f));
            g.fillRoundedRectangle(fill, 2.0f);
        }

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
        if (ipadPlayMode_)
        {
            if (const auto pill = footerPillAt(event.getPosition()))
            {
                activeIpadFooterPill_ = *pill;
                repaint();
                if (onIpadFooterPillSelected)
                    onIpadFooterPillSelected(*pill);
                return;
            }
        }

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
        g.setFont(fonts::micro(8.0f));
        g.setColour(palette::kTextDim);
        const int padX = layout::kDesignModeV2StatusBarPaddingX;
        const int y = static_cast<int>(bounds.getY()) + 15;
        g.drawText("CPU", padX, y, 18, 10, juce::Justification::centredLeft, true);
        g.drawText("FX", padX + 101, y, 14, 10, juce::Justification::centredLeft, true);
        g.drawText("VOICES", padX + 191, y, 40, 10, juce::Justification::centredLeft, true);
            return;
        }

        if (ipadPlayMode_)
            return;

        if (!desktopPlayMode_)
            return;

        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextSecondary);
        const int padX = layout::kDesktopPlayModeBottomBarPaddingX;
        const int indicatorY = static_cast<int>(bounds.getY()) + 17;
        g.drawText("ENGINES", padX, indicatorY + 9, 52, 12, juce::Justification::centredLeft, false);

        static constexpr const char* kEngineLabels[] = {"SIN", "SAW", "TRI", "SQR", "NOI", "PAD", "FMB", "ORG"};
        const int indicatorWidth = layout::kDesktopPlayModeEngineIndicatorWidth;
        int x = padX + 56;
        for (int i = 0; i < 8; ++i)
        {
            const auto& op = processor_.getCurrentPatch().layerA.operators[static_cast<std::size_t>(i)];
            const bool active = op.mixEnabled && !op.mixMute;

            const float ledX = static_cast<float>(x + (indicatorWidth - 16) / 2);
            const auto ledBox = juce::Rectangle<float>(ledX, static_cast<float>(indicatorY), 16.0f, 16.0f);
            g.setColour(palette::kPanelRaised.brighter(0.08f));
            g.fillRoundedRectangle(ledBox, 3.0f);
            if (active)
            {
                g.setColour(palette::kAccent);
                g.fillRoundedRectangle(ledBox.reduced(5.0f), 1.0f);
            }
            g.setColour(palette::kTextDim);
            g.drawText(kEngineLabels[i], static_cast<float>(x), static_cast<float>(indicatorY + 20),
                       static_cast<float>(indicatorWidth), 10.0f, juce::Justification::centred, false);
            x += indicatorWidth + layout::kDesktopPlayModeEngineIndicatorGap;
        }
    }

    void VstBottomBar::resized()
    {
        if (designModeV2Layout_)
        {
            auto bounds = getLocalBounds().reduced(layout::kDesignModeV2StatusBarPaddingX, 11);
            const int rowY = bounds.getY();
            const int chipH = 18;
            const int chipW = layout::kDesignStatusBarLabChipWidth;
            const int chipGap = layout::kDesignStatusBarLabChipGap;

            // --- Left: CPU / FX / voices / MIDI (fixed columns) ---
            int x = bounds.getX();
            const int metricY = rowY + 4;

            cpuBarBounds_ = {x + 21, metricY + 6, 40, 4};
            cpuLabel_.setBounds(x + 63, metricY, 26, 10);
            x += 89 + layout::kDesignStatusBarZoneGap;

            fxLoadBarBounds_ = {x + 14, metricY + 6, 40, 4};
            fxLoadLabel_.setBounds(x + 56, metricY, 26, 10);
            x += 78 + layout::kDesignStatusBarZoneGap;

            voicesLabel_.setBounds(x, metricY, 58, 10);
            x += 58 + layout::kDesignStatusBarZoneGap;

            midiLabel_.setBounds(x, metricY, 44, 10);
            const int leftEnd = x + 44;

            // --- Right: panic + secondary lab shortcuts ---
            int rightX = bounds.getRight();
            auto placeChip = [&](LabLauncherChip& chip) {
                rightX -= chipW;
                chip.setBounds(rightX, rowY, chipW, chipH);
                rightX -= chipGap;
            };

            if (morphEditorChip_->isVisible())
                placeChip(*morphEditorChip_);
            if (lfoChip_->isVisible())
                placeChip(*lfoChip_);
            if (vocoderChip_->isVisible())
                placeChip(*vocoderChip_);

            rightX -= layout::kDesignStatusBarZoneGap;
            rightX -= layout::kDesignStatusBarPanicWidth;
            panicButton_.setBounds(rightX, rowY, layout::kDesignStatusBarPanicWidth, chipH);
            const int rightStart = rightX;

            // --- Center: mod-source quick chips (fills space between left metrics and right actions) ---
            const int centerPad = 8;
            const int centerX = leftEnd + centerPad;
            const int centerW = juce::jmax(0, rightStart - centerPad - centerX);
            auto modRow = juce::Rectangle<int>(centerX, rowY + 2, centerW, 14);
            modSourcesLabel_.setBounds(modRow.removeFromLeft(juce::jmin(58, modRow.getWidth())));
            if (modRow.getWidth() > 4)
                modRow.removeFromLeft(4);

            const int modChipW = juce::jmax(24, (modRow.getWidth() - 3 * 4) / 4);
            for (std::size_t i = 0; i < modChipBounds_.size(); ++i)
            {
                if (modRow.getWidth() < modChipW)
                {
                    modChipBounds_[i] = {};
                    continue;
                }
                modChipBounds_[i] = modRow.removeFromLeft(modChipW).withHeight(14);
                if (i + 1 < modChipBounds_.size() && modRow.getWidth() > 4)
                    modRow.removeFromLeft(4);
            }
            return;
        }

        if (murmurBasicViewMode_)
        {
            auto row = getLocalBounds().reduced(layout::kMurmurBasicViewOuterMargin, 10);
            panicButton_.setBounds(row.removeFromRight(92).withSizeKeepingCentre(92, 20));
            row.removeFromRight(12);
            statusLabel_.setBounds(row.removeFromLeft(120).withHeight(12).withY(row.getY() + 4));
            row.removeFromLeft(12);
            voicesLabel_.setBounds(row.removeFromLeft(72).withHeight(12).withY(row.getY() + 4));
            row.removeFromLeft(12);
            cpuBarBounds_ = row.removeFromLeft(50).withSizeKeepingCentre(50, 6);
            cpuLabel_.setBounds(row.removeFromLeft(28).withHeight(11).withY(cpuBarBounds_.getY() - 1));
            return;
        }

        if (ipadPlayMode_)
        {
            auto row = getLocalBounds().reduced(layout::kDesktopPlayModeBottomBarPaddingX, 10);
            panicButton_.setBounds(row.removeFromRight(92).withSizeKeepingCentre(92, 27));
            row.removeFromRight(12);

            auto leftCluster = row.removeFromLeft(280);
            statusLabel_.setBounds(leftCluster.removeFromLeft(96).withSizeKeepingCentre(96, 23));
            leftCluster.removeFromLeft(8);
            bpmLabel_.setBounds(leftCluster.removeFromLeft(150).withHeight(14).withY(leftCluster.getY() + 4));
            leftCluster.removeFromLeft(8);
            cpuBarBounds_ = leftCluster.removeFromLeft(50).withSizeKeepingCentre(50, 6);
            cpuLabel_.setBounds(leftCluster.removeFromLeft(24).withHeight(11).withY(cpuBarBounds_.getY() - 1));

            const std::array<int, 5> pillWidths = {
                layout::kIpadPlayFooterPillPlayWidth,   layout::kIpadPlayFooterPillDesignWidth,
                layout::kIpadPlayFooterPillVocWidth,    layout::kIpadPlayFooterPillArpWidth,
                layout::kIpadPlayFooterPillLfoWidth,
            };
            const int stripWidth = layout::kIpadPlayFooterPillStripWidth;
            const int stripX = getLocalBounds().getCentreX() - stripWidth / 2;
            const int stripY = getLocalBounds().getCentreY() - layout::kIpadPlayFooterPillHeight / 2;
            int pillX = stripX;
            for (std::size_t i = 0; i < pillWidths.size(); ++i)
            {
                ipadFooterPillBounds_[i] = {pillX, stripY, pillWidths[i], layout::kIpadPlayFooterPillHeight};
                pillX += pillWidths[i] + layout::kIpadPlayFooterPillGap;
            }
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
        if (playBoardMode_)
        {
            motionChip_->setBounds(row.removeFromRight(chipW + 4).reduced(1));
            row.removeFromRight(4);
            quasarChip_->setBounds(row.removeFromRight(chipW + 4).reduced(1));
            row.removeFromRight(4);
        }
        lfoChip_->setBounds(row.removeFromRight(chipW + 4).reduced(1));
        row.removeFromRight(4);
        vocoderChip_->setBounds(row.removeFromRight(chipW + 8).reduced(1));

        voicesLabel_.setBounds(row.removeFromRight(96));
        row.removeFromRight(8);
        statusLabel_.setBounds(row);
    }

} // namespace pw8::plugin::ui
