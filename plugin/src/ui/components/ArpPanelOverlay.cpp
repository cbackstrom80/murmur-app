#include "ArpPanelOverlay.h"

#include "../PerformanceMetricsUi.h"
#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianRotary.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ArpUiFormatters.h"
#include "pw8/core/Types.hpp"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"
#include "pw8/oscillator/ClassicOscillator.hpp"
#include "pw8/patch/Patch.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] float readParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                      float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        void setParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        }

        void styleModeButton(juce::TextButton& btn, const juce::String& text)
        {
            btn.setButtonText(text);
            btn.setClickingTogglesState(false);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        }

        void styleOctaveButton(juce::TextButton& btn, int octaves)
        {
            btn.setButtonText(juce::String(octaves) + " OCT");
            btn.setClickingTogglesState(false);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        }

        void styleSyncButton(juce::TextButton& btn, const juce::String& label)
        {
            btn.setButtonText(label);
            btn.setClickingTogglesState(false);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
        }

        void highlightChoiceButton(juce::TextButton& btn, bool active)
        {
            btn.setColour(juce::TextButton::buttonColourId, active ? palette::kAccent.withAlpha(0.28f) : palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, active ? palette::kAccent : palette::kTextSecondary);
        }

        [[nodiscard]] const char* classicWaveformName(oscillator::ClassicWaveform waveform) noexcept
        {
            switch (waveform)
            {
                case oscillator::ClassicWaveform::Sine: return "SINE";
                case oscillator::ClassicWaveform::Triangle: return "TRIANGLE";
                case oscillator::ClassicWaveform::Saw: return "SAW";
                case oscillator::ClassicWaveform::Square: return "SQUARE";
            }
            return "WAVE";
        }

        [[nodiscard]] const char* engineRoleSuffix(std::size_t index) noexcept
        {
            static constexpr const char* kRoles[] = {"LEAD", "SUB", "OSC", "REZZ", "PERC", "PAD", "BELL", "VIBE"};
            return kRoles[juce::jlimit<std::size_t>(0, 7, index)];
        }

        [[nodiscard]] juce::String enginePrimaryLabel(const patch::OperatorPatch& op)
        {
            switch (op.engine)
            {
                case algorithm::EngineType::Classic:
                    return classicWaveformName(op.classicWaveform);
                case algorithm::EngineType::Wavetable:
                    if (!op.wavetableId.empty())
                    {
                        auto fragment = juce::String(op.wavetableId).fromLastOccurrenceOf("/", false, false)
                                            .fromLastOccurrenceOf("\\", false, false);
                        if (fragment.isEmpty())
                            fragment = op.wavetableId;
                        fragment = fragment.upToFirstOccurrenceOf(".", false, false).replaceCharacter('_', ' ')
                                       .trim();
                        if (fragment.length() > 14)
                            fragment = fragment.substring(0, 14).trimEnd();
                        if (fragment.isNotEmpty())
                            return fragment.toUpperCase();
                    }
                    return "CUSTOM";
                case algorithm::EngineType::FmPm: return "FM";
                case algorithm::EngineType::Additive: return "ADDITIVE";
                case algorithm::EngineType::PhaseShape: return "PHASE";
                case algorithm::EngineType::Granular: return "GRANULAR";
                case algorithm::EngineType::NoiseChaos: return "NOISE";
                case algorithm::EngineType::Resonator: return "RESONATOR";
                case algorithm::EngineType::External: return "EXTERNAL";
            }
            return "ENGINE";
        }

        void drawLockIcon(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour)
        {
            const float w = bounds.getWidth();
            const float h = bounds.getHeight();
            const float shackleW = w * 0.62f;
            const float shackleH = h * 0.48f;
            const float bodyW = w * 0.72f;
            const float bodyH = h * 0.46f;
            const float stroke = 1.4f;

            juce::Path shackle;
            shackle.addArc(bounds.getCentreX() - shackleW * 0.5f, bounds.getY() + h * 0.04f, shackleW, shackleH,
                           juce::MathConstants<float>::pi, 0.0f, true);
            g.setColour(colour);
            g.strokePath(shackle, juce::PathStrokeType(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            const auto body = juce::Rectangle<float>(bounds.getCentreX() - bodyW * 0.5f,
                                                     bounds.getBottom() - bodyH - h * 0.02f, bodyW, bodyH);
            g.drawRoundedRectangle(body, 2.0f, stroke);
        }

        struct FooterModChipSpec
        {
            const char* label;
            juce::Colour colour;
            modulation::ModSource source;
        };
    } // namespace

    ArpPanelOverlay::ArpPanelOverlay(PatchworkEightProcessor& processor)
        : processor_(processor),
          apvts_(processor.apvts),
          enableButton_("ARP ON"),
          leftPanel_("ARP ENGINE SETTINGS"),
          latchButton_("HOLD"),
          stepPanel_(""),
          stepStrip_(processor),
          timingPanel_("TIMING & ENVELOPE"),
          curvePanel_("VELOCITY RESPONSE"),
          routingPanel_("ENGINE ROUTING")
    {
        setVisible(false);
        setWantsKeyboardFocus(true);

        addAndMakeVisible(backButton_);
        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] { dismiss(); };

        badgeLabel_.setText("UX-09", juce::dontSendNotification);
        badgeLabel_.setFont(fonts::label(9.0f));
        badgeLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        badgeLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(badgeLabel_);

        titleLabel_.setText("ARPEGGIATOR LAB", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        syncReadoutLabel_.setFont(fonts::label(10.0f));
        syncReadoutLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        syncReadoutLabel_.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(syncReadoutLabel_);

        enableButton_.setClickingTogglesState(true);
        enableButton_.setAccentColour(palette::kAccent);
        addAndMakeVisible(enableButton_);
        enableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, juce::String(kArpIdPrefix) + "Enabled", enableButton_);

        addAndMakeVisible(leftPanel_);
        playbackDirectionLabel_.setText("PLAYBACK DIRECTION", juce::dontSendNotification);
        playbackDirectionLabel_.setFont(fonts::label(8.0f));
        playbackDirectionLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        leftPanel_.addAndMakeVisible(playbackDirectionLabel_);

        static constexpr const char* kModeLabels[] = {"UP", "DOWN", "UP / DOWN", "DN-UP", "AS PLAYED", "RANDOM", "CHORD"};
        for (std::size_t i = 0; i < modeButtons_.size(); ++i)
        {
            styleModeButton(modeButtons_[i], kModeLabels[i]);
            modeButtons_[i].onClick = [this, i] { setArpMode(static_cast<int>(i)); };
            leftPanel_.addAndMakeVisible(modeButtons_[i]);
        }

        octaveRangeLabel_.setText("OCTAVE RANGE", juce::dontSendNotification);
        octaveRangeLabel_.setFont(fonts::label(8.0f));
        octaveRangeLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        leftPanel_.addAndMakeVisible(octaveRangeLabel_);
        for (std::size_t i = 0; i < octaveButtons_.size(); ++i)
        {
            styleOctaveButton(octaveButtons_[i], static_cast<int>(i + 1));
            octaveButtons_[i].onClick = [this, i] { setOctaveRange(static_cast<int>(i + 1)); };
            leftPanel_.addAndMakeVisible(octaveButtons_[i]);
        }

        latchButton_.setClickingTogglesState(true);
        latchButton_.setAccentColour(palette::kAccent);
        latchButton_.setButtonText({});
        leftPanel_.addAndMakeVisible(latchButton_);
        latchAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts_, juce::String(kArpIdPrefix) + "Latch", latchButton_);

        holdLabel_.setText("PATTERN HOLD", juce::dontSendNotification);
        holdLabel_.setFont(fonts::label(9.0f));
        holdLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        holdLabel_.setJustificationType(juce::Justification::centredLeft);
        leftPanel_.addAndMakeVisible(holdLabel_);

        addAndMakeVisible(stepPanel_);
        stepViewport_.setViewedComponent(&stepStrip_, false);
        stepViewport_.setScrollBarsShown(true, false);
        stepViewport_.setScrollBarThickness(8);
        stepPanel_.addAndMakeVisible(stepViewport_);

        addAndMakeVisible(timingPanel_);
        clockResolutionLabel_.setText("CLOCK RESOLUTION", juce::dontSendNotification);
        clockResolutionLabel_.setFont(fonts::label(8.0f));
        clockResolutionLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        timingPanel_.addAndMakeVisible(clockResolutionLabel_);

        static constexpr const char* kSyncLabels[] = {"1 / 4", "1 / 8", "1 / 16", "1 / 32"};
        for (std::size_t i = 0; i < syncButtons_.size(); ++i)
        {
            styleSyncButton(syncButtons_[i], kSyncLabels[i]);
            syncButtons_[i].onClick = [this, i] { setSyncDivisionQuick(static_cast<int>(i)); };
            timingPanel_.addAndMakeVisible(syncButtons_[i]);
        }

        gateSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        gateSlider_.setRotaryParameters(rotary::kStartAngle, rotary::kEndAngle, true);
        gateSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 76, 16);
        gateSlider_.setRange(0.05, 1.0, 0.01);
        gateSlider_.setValue(processor_.getCurrentPatch().arpeggiator.steps[0].gate, juce::dontSendNotification);
        gateSlider_.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        gateSlider_.setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        gateSlider_.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        gateSlider_.textFromValueFunction = [](double v) { return juce::String(static_cast<int>(v * 100.0)) + "%"; };
        gateSlider_.onValueChange = [this] {
            processor_.setAllArpStepsGate(static_cast<float>(gateSlider_.getValue()));
        };
        timingPanel_.addAndMakeVisible(gateSlider_);

        gateLabel_.setText("GATE", juce::dontSendNotification);
        gateLabel_.setFont(fonts::label(11.0f));
        gateLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        gateLabel_.setJustificationType(juce::Justification::centred);
        timingPanel_.addAndMakeVisible(gateLabel_);

        numStepsKnob_ = std::make_unique<GlowKnob>(
            apvts_, juce::String(kArpIdPrefix) + "NumSteps", "LENGTH",
            [](float v) { return juce::String(juce::roundToInt(v)); });
        numStepsKnob_->setMaxDialDiameter(layout::kArpTimingKnobDialSize);
        timingPanel_.addAndMakeVisible(*numStepsKnob_);

        swingKnob_ = std::make_unique<GlowKnob>(apvts_, juce::String(kArpIdPrefix) + "Swing", "SWING",
                                                [](float v) { return juce::String(static_cast<int>(v * 100.0f)) + "%"; });
        swingKnob_->setMaxDialDiameter(layout::kArpTimingKnobDialSize);
        timingPanel_.addAndMakeVisible(*swingKnob_);

        rateHzKnob_ = std::make_unique<GlowKnob>(apvts_, juce::String(kArpIdPrefix) + "RateHz", "RATE HZ");
        rateHzKnob_->setMaxDialDiameter(layout::kArpTimingKnobDialSize);
        timingPanel_.addAndMakeVisible(*rateHzKnob_);

        addAndMakeVisible(curvePanel_);
        curveTypeLabel_.setText("CURVE: S-CURVE", juce::dontSendNotification);
        curveTypeLabel_.setFont(fonts::label(8.0f));
        curveTypeLabel_.setColour(juce::Label::textColourId, palette::kAccentWarm);
        curveTypeLabel_.setJustificationType(juce::Justification::centredRight);
        curvePanel_.addAndMakeVisible(curveTypeLabel_);

        curveCaption_.setText("S-curve velocity shaping", juce::dontSendNotification);
        curveCaption_.setFont(fonts::label(9.0f));
        curveCaption_.setColour(juce::Label::textColourId, palette::kTextDim);
        curveCaption_.setJustificationType(juce::Justification::centred);
        curvePanel_.addAndMakeVisible(curveCaption_);

        addAndMakeVisible(routingPanel_);
        for (std::size_t i = 0; i < 8; ++i)
        {
            routeEngineLabels_[i].setFont(fonts::label(10.0f));
            routeEngineLabels_[i].setColour(juce::Label::textColourId, palette::kTextPrimary);
            routeEngineLabels_[i].setJustificationType(juce::Justification::centredLeft);
            routingPanel_.addAndMakeVisible(routeEngineLabels_[i]);

            routeSubLabels_[i].setFont(fonts::label(8.0f));
            routeSubLabels_[i].setColour(juce::Label::textColourId, palette::kTextDim);
            routeSubLabels_[i].setJustificationType(juce::Justification::centredLeft);
            routingPanel_.addAndMakeVisible(routeSubLabels_[i]);

            mixButtons_[i] = std::make_unique<GlowRingButton>();
            mixButtons_[i]->setClickingTogglesState(true);
            mixButtons_[i]->setAccentColour(palette::kAccent);
            routingPanel_.addAndMakeVisible(*mixButtons_[i]);
            mixAttachments_[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts_, operatorMixParamId(i, "MixEnabled"), *mixButtons_[i]);
        }

        footerCpuLabel_.setFont(fonts::label(8.0f));
        footerCpuLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerCpuLabel_.setJustificationType(juce::Justification::centredLeft);
        footerCpuLabel_.setText("CPU", juce::dontSendNotification);
        addAndMakeVisible(footerCpuLabel_);

        footerCpuPercentLabel_.setFont(fonts::label(8.0f));
        footerCpuPercentLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        footerCpuPercentLabel_.setJustificationType(juce::Justification::centredLeft);
        footerCpuPercentLabel_.setText("--%", juce::dontSendNotification);
        addAndMakeVisible(footerCpuPercentLabel_);

        footerVoicesLabel_.setFont(fonts::label(8.0f));
        footerVoicesLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerVoicesLabel_.setJustificationType(juce::Justification::centredLeft);
        footerVoicesLabel_.setText("VOICES:", juce::dontSendNotification);
        addAndMakeVisible(footerVoicesLabel_);

        footerVoicesCountLabel_.setFont(fonts::label(8.0f));
        footerVoicesCountLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        footerVoicesCountLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(footerVoicesCountLabel_);

        footerMidiLabel_.setFont(fonts::label(8.0f));
        footerMidiLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerMidiLabel_.setJustificationType(juce::Justification::centredLeft);
        footerMidiLabel_.setText("MIDI IN", juce::dontSendNotification);
        addAndMakeVisible(footerMidiLabel_);

        footerModSourcesLabel_.setFont(fonts::label(8.0f));
        footerModSourcesLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerModSourcesLabel_.setJustificationType(juce::Justification::centredLeft);
        footerModSourcesLabel_.setText("MOD SOURCES:", juce::dontSendNotification);
        addAndMakeVisible(footerModSourcesLabel_);

        panicButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3b1c1c));
        panicButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe07f7f));
        panicButton_.onClick = [this] { processor_.panicAllNotes(); };
        addAndMakeVisible(panicButton_);

        refreshEngineRouting();
        startTimerHz(12);
    }

    ArpPanelOverlay::~ArpPanelOverlay() { stopTimer(); }

    void ArpPanelOverlay::showDrawer()
    {
        setVisible(true);
        setInterceptsMouseClicks(true, true);
        resized();
        grabKeyboardFocus();
        toFront(true);
    }

    void ArpPanelOverlay::dismiss()
    {
        setVisible(false);
        setInterceptsMouseClicks(false, true);
        if (onClosed)
            onClosed();
    }

    void ArpPanelOverlay::setEmbeddedInDesignMode(bool embedded)
    {
        if (embeddedInDesignMode_ == embedded)
            return;

        embeddedInDesignMode_ = embedded;
        backButton_.setButtonText(embedded ? "← ENGINE" : "← PLAY BOARD");
        badgeLabel_.setVisible(!embedded);
        titleLabel_.setVisible(!embedded);
        syncReadoutLabel_.setVisible(!embedded);
        enableButton_.setVisible(true);
        resized();
    }

    void ArpPanelOverlay::timerCallback()
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        if (arp.numSteps > 0)
        {
            const float gate = arp.steps[0].gate;
            if (std::abs(gateSlider_.getValue() - gate) > 0.001)
                gateSlider_.setValue(gate, juce::dontSendNotification);
        }

        const bool tempoSync = readParam(apvts_, juce::String(kArpIdPrefix) + "RateMode", 1.0f) >= 0.5f;
        const float rateHz = readParam(apvts_, juce::String(kArpIdPrefix) + "RateHz", 8.0f);
        const int syncIdx = static_cast<int>(readParam(apvts_, juce::String(kArpIdPrefix) + "SyncDivisionIndex", 6.0f) + 0.5f);
        syncReadoutLabel_.setText(tempoSync ? ("SYNC · " + arpSyncDivisionLabel(syncIdx))
                                            : ("FREE · " + juce::String(rateHz, 1) + " Hz"),
                                  juce::dontSendNotification);

        const int mode = static_cast<int>(readParam(apvts_, juce::String(kArpIdPrefix) + "Mode", 0.0f) + 0.5f);
        for (std::size_t i = 0; i < modeButtons_.size(); ++i)
            highlightChoiceButton(modeButtons_[i], static_cast<int>(i) == mode);

        const int octaves = static_cast<int>(readParam(apvts_, juce::String(kArpIdPrefix) + "OctaveRange", 1.0f) + 0.5f);
        for (std::size_t i = 0; i < octaveButtons_.size(); ++i)
            highlightChoiceButton(octaveButtons_[i], static_cast<int>(i + 1) == octaves);

        static constexpr int kSyncIndices[] = {4, 5, 6, 6};
        for (std::size_t i = 0; i < syncButtons_.size(); ++i)
            highlightChoiceButton(syncButtons_[i], tempoSync && syncIdx == kSyncIndices[i]);

        rateHzKnob_->setVisible(!tempoSync);

        const auto metrics = readPerformanceMetrics(processor_);
        footerCpuLoadPercent_ = metrics.cpuPercent;
        footerCpuPercentLabel_.setText(formatCpuPercent(metrics.cpuPercent), juce::dontSendNotification);
        footerVoicesCountLabel_.setText(formatVoiceCount(metrics.activeVoices, metrics.maxVoices),
                                        juce::dontSendNotification);
        footerMidiLabel_.setColour(juce::Label::textColourId, palette::kTextDim);

        refreshEngineRouting();
        repaint();
    }

    void ArpPanelOverlay::refreshEngineRouting()
    {
        const auto& patch = processor_.getCurrentPatch();
        for (std::size_t i = 0; i < 8; ++i)
        {
            const auto& op = patch.layerA.operators[i];
            const bool active = op.mixEnabled && !op.mixMute;

            routeEngineLabels_[i].setText("ENGINE " + juce::String(static_cast<int>(i + 1)).paddedLeft('0', 2),
                                          juce::dontSendNotification);
            routeEngineLabels_[i].setColour(juce::Label::textColourId,
                                            active ? palette::kTextPrimary : palette::kTextDim);

            routeSubLabels_[i].setText(engineRoutingSubtitle(static_cast<int>(i)), juce::dontSendNotification);
            routeSubLabels_[i].setColour(juce::Label::textColourId,
                                         active ? palette::kAccent : palette::kTextDim.withAlpha(0.72f));
        }
    }

    void ArpPanelOverlay::setArpMode(int modeOrdinal)
    {
        setParam(apvts_, juce::String(kArpIdPrefix) + "Mode", static_cast<float>(juce::jlimit(0, 6, modeOrdinal)));
    }

    void ArpPanelOverlay::setOctaveRange(int octaves)
    {
        setParam(apvts_, juce::String(kArpIdPrefix) + "OctaveRange", static_cast<float>(juce::jlimit(1, 4, octaves)));
    }

    void ArpPanelOverlay::setSyncDivisionQuick(int buttonIndex)
    {
        static constexpr int kSyncIndices[] = {4, 5, 6, 6};
        const int idx = kSyncIndices[juce::jlimit(0, 3, buttonIndex)];
        setParam(apvts_, juce::String(kArpIdPrefix) + "RateMode", 1.0f);
        setParam(apvts_, juce::String(kArpIdPrefix) + "SyncDivisionIndex", static_cast<float>(idx));
    }

    juce::String ArpPanelOverlay::engineRoutingSubtitle(int engineIndex) const
    {
        const auto idx = static_cast<std::size_t>(juce::jlimit(0, 7, engineIndex));
        const auto& op = processor_.getCurrentPatch().layerA.operators[idx];
        return enginePrimaryLabel(op) + " " + engineRoleSuffix(idx);
    }

    bool ArpPanelOverlay::modSourceActive(modulation::ModSource source) const
    {
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive() && route.source == source)
                return true;
        }
        return false;
    }

    void ArpPanelOverlay::paintHoldRow(juce::Graphics& g) const
    {
        if (holdRowBounds_.isEmpty())
            return;

        auto row = holdRowBounds_.toFloat();
        g.setColour(juce::Colour(0xff1a1c23));
        g.fillRoundedRectangle(row, 6.0f);
        g.setColour(palette::kBorder);
        g.drawRoundedRectangle(row.reduced(0.5f), 6.0f, 1.0f);

        const auto lockBounds = juce::Rectangle<float>(row.getX() + 8.0f, row.getCentreY() - 7.0f, 14.0f, 14.0f);
        drawLockIcon(g, lockBounds, latchButton_.getToggleState() ? palette::kAccent : palette::kTextDim);
    }

    void ArpPanelOverlay::paintFooterBar(juce::Graphics& g) const
    {
        if (footerBounds_.isEmpty())
            return;

        auto bounds = footerBounds_.toFloat();
        g.setColour(juce::Colour(0xff13151d));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(palette::kBorder);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
    }

    void ArpPanelOverlay::paintFooterModChips(juce::Graphics& g) const
    {
        static const FooterModChipSpec kChips[] = {
            {"LFO1", juce::Colour(0xff7fe7e0), modulation::ModSource::Lfo1},
            {"LFO2", juce::Colour(0xff7f9fe0), modulation::ModSource::Lfo2},
            {"ENV1", juce::Colour(0xff8f7fe0), modulation::ModSource::Env1},
            {"ENV2", juce::Colour(0xffe0c17f), modulation::ModSource::Env2},
            {"RAND", juce::Colour(0xff7fe0a0), modulation::ModSource::None},
        };

        g.setFont(fonts::label(7.0f));
        for (std::size_t i = 0; i < footerModChipBounds_.size(); ++i)
        {
            const auto& spec = kChips[i];
            const auto chip = footerModChipBounds_[i].toFloat();
            if (chip.isEmpty())
                continue;

            const bool active = spec.source != modulation::ModSource::None && modSourceActive(spec.source);
            const float fillAlpha = active ? 0.16f : 0.08f;
            g.setColour(spec.colour.withAlpha(fillAlpha));
            g.fillRoundedRectangle(chip, 4.0f);
            g.setColour(spec.colour.withAlpha(active ? 1.0f : 0.82f));
            g.drawRoundedRectangle(chip.reduced(0.5f), 4.0f, 1.0f);
            g.drawText(spec.label, chip, juce::Justification::centred);
        }
    }

    void ArpPanelOverlay::paintVelocityCurve(juce::Graphics& g, juce::Rectangle<int> area) const
    {
        auto bounds = area.toFloat().reduced(8.0f);
        draw::fillRecessedRoundedRect(g, bounds, 6.0f);

        juce::Path curve;
        curve.startNewSubPath(bounds.getX(), bounds.getBottom());
        curve.cubicTo(bounds.getX() + bounds.getWidth() * 0.25f, bounds.getBottom(),
                      bounds.getX() + bounds.getWidth() * 0.35f, bounds.getY() + bounds.getHeight() * 0.55f,
                      bounds.getCentreX(), bounds.getCentreY());
        curve.cubicTo(bounds.getRight() - bounds.getWidth() * 0.35f, bounds.getY() + bounds.getHeight() * 0.15f,
                      bounds.getRight() - bounds.getWidth() * 0.2f, bounds.getY(),
                      bounds.getRight(), bounds.getY());

        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.strokePath(curve, juce::PathStrokeType(2.0f));

        g.setColour(palette::kBorder.withAlpha(0.35f));
        for (int i = 1; i < 4; ++i)
        {
            const float x = bounds.getX() + bounds.getWidth() * static_cast<float>(i) / 4.0f;
            g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
        }
    }

    void ArpPanelOverlay::paint(juce::Graphics& g)
    {
        if (embeddedInDesignMode_)
            g.fillAll(palette::kBackgroundTop);
        else
            g.fillAll(palette::kBackgroundTop.withAlpha(0.96f));

        if (embeddedInDesignMode_)
            return;

        auto badge = badgeLabel_.getBounds().toFloat().expanded(4.0f, 2.0f);
        g.setColour(palette::kAccent.withAlpha(0.13f));
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(palette::kAccent);
        g.drawRoundedRectangle(badge, 4.0f, 1.0f);
    }

    void ArpPanelOverlay::paintOverChildren(juce::Graphics& g)
    {
        paintHoldRow(g);
        paintFooterBar(g);
        paintVelocityCurve(g, curvePanel_.getContentBounds().reduced(12, 28));
        paintFooterModChips(g);

        if (!cpuBarBounds_.isEmpty())
        {
            auto barArea = cpuBarBounds_.toFloat();
            g.setColour(juce::Colour(0xff0d0f14));
            g.fillRoundedRectangle(barArea, 2.0f);
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(barArea.getX(), barArea.getY(),
                                   barArea.getWidth() * cpuBarFillRatio(footerCpuLoadPercent_), barArea.getHeight(),
                                   2.0f);
        }

        auto midiArea = footerMidiLabel_.getBounds();
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.fillEllipse(static_cast<float>(midiArea.getX() - 12), static_cast<float>(midiArea.getCentreY() - 3), 6.0f,
                      6.0f);

        const int mode = static_cast<int>(readParam(apvts_, juce::String(kArpIdPrefix) + "Mode", 0.0f) + 0.5f);
        for (std::size_t i = 0; i < modeButtons_.size(); ++i)
        {
            if (static_cast<int>(i) != mode)
                continue;
            auto btn = modeButtons_[i].getBounds().toFloat();
            g.setColour(palette::kAccent);
            g.fillEllipse(btn.getX() + 12.0f, btn.getCentreY() - 3.0f, 6.0f, 6.0f);
        }
    }

    void ArpPanelOverlay::resized()
    {
        auto bounds = getLocalBounds();
        if (!embeddedInDesignMode_)
            bounds = bounds.reduced(layout::kDesignArpPageOuterMargin);

        const int headerHeight =
            embeddedInDesignMode_ ? layout::kDesignLabPanelHeaderHeight : layout::kArpHeaderHeight;
        auto header = bounds.removeFromTop(headerHeight);
        backButton_.setBounds(header.removeFromLeft(120));
        if (embeddedInDesignMode_)
            enableButton_.setBounds(header.removeFromRight(80).reduced(2));
        else
        {
            header.removeFromLeft(8);
            badgeLabel_.setBounds(header.removeFromLeft(36).withSizeKeepingCentre(36, 18));
            header.removeFromLeft(8);
            titleLabel_.setBounds(header.removeFromLeft(header.getWidth() / 2));
            syncReadoutLabel_.setBounds(header.removeFromLeft(header.getWidth() - 88));
            enableButton_.setBounds(header.removeFromRight(80).reduced(2));
        }

        bounds.removeFromTop(embeddedInDesignMode_ ? layout::kDesignArpPageSectionGap
                                                   : layout::kDesignArpPageSectionGap);
        footerBounds_ = bounds.removeFromBottom(layout::kArpFooterHeight);
        auto footer = footerBounds_.reduced(16, 0);
        bounds.removeFromBottom(layout::kDesignArpPageSectionGap);

        panicButton_.setBounds(footer.removeFromRight(layout::kArpFooterPanicWidth)
                                   .withSizeKeepingCentre(layout::kArpFooterPanicWidth, layout::kArpFooterPanicHeight));
        footer.removeFromRight(16);

        auto modRow = footer.withSizeKeepingCentre(248, 14);
        modRow = modRow.withX(footer.getCentreX() - modRow.getWidth() / 2);
        footerModSourcesLabel_.setBounds(modRow.removeFromLeft(64).withHeight(10).withY(modRow.getY() + 2));
        modRow.removeFromLeft(6);
        static constexpr int kChipWidths[] = {30, 30, 30, 30, 34};
        for (std::size_t i = 0; i < footerModChipBounds_.size(); ++i)
        {
            footerModChipBounds_[i] = modRow.removeFromLeft(kChipWidths[i]).withHeight(14).withY(modRow.getY());
            modRow.removeFromLeft(6);
        }

        auto midiArea = footer.removeFromRight(52);
        footerMidiLabel_.setBounds(midiArea.withTrimmedLeft(12).withHeight(10).withY(midiArea.getY() + 4));
        footer.removeFromRight(16);
        footerVoicesCountLabel_.setBounds(footer.removeFromRight(40).withHeight(10).withY(footer.getY() + 4));
        footerVoicesLabel_.setBounds(footer.removeFromRight(34).withHeight(10).withY(footer.getY() + 4));
        footer.removeFromRight(16);
        footerCpuPercentLabel_.setBounds(footer.removeFromRight(22).withHeight(10).withY(footer.getY() + 4));
        cpuBarBounds_ = footer.removeFromLeft(40).withHeight(4).withY(footer.getY() + 6);
        footer.removeFromLeft(6);
        footerCpuLabel_.setBounds(footer.removeFromLeft(15).withHeight(10).withY(footer.getY() + 4));

        const int sideW = layout::kArpSidePanelWidth;
        const int colGap = layout::kDesignArpPageSectionGap;
        auto left = bounds.removeFromLeft(sideW);
        bounds.removeFromLeft(colGap);
        auto right = bounds.removeFromRight(sideW);
        bounds.removeFromRight(colGap);
        auto center = bounds;

        leftPanel_.setBounds(left);
        routingPanel_.setBounds(right);

        const int bottomH = layout::kArpBottomPanelHeight;
        auto centerBottom = center.removeFromBottom(bottomH);
        center.removeFromBottom(colGap);
        auto centerTop = center;

        stepPanel_.setBounds(centerTop);
        stepViewport_.setBounds(stepPanel_.getContentBounds().reduced(8, 4));
        stepStrip_.syncLayoutMetrics(stepViewport_.getWidth());
        stepViewport_.setViewPosition(0, 0);

        curvePanel_.setBounds(centerBottom.removeFromRight(layout::kArpVelocityCurveCardWidth));
        centerBottom.removeFromRight(colGap);
        timingPanel_.setBounds(centerBottom);

        // Left panel content — Figma `left-parameters` (4:1305)
        {
            auto content = leftPanel_.getContentBounds().reduced(8, 0);
            playbackDirectionLabel_.setBounds(content.removeFromTop(10));
            content.removeFromTop(4);

            for (auto& btn : modeButtons_)
            {
                btn.setBounds(content.removeFromTop(layout::kArpModeButtonHeight));
                content.removeFromTop(layout::kArpModeButtonGap);
            }

            content.removeFromTop(10);
            octaveRangeLabel_.setBounds(content.removeFromTop(10));
            content.removeFromTop(4);
            auto octRow = content.removeFromTop(layout::kArpOctaveStripHeight);
            const int octW = octRow.getWidth() / static_cast<int>(octaveButtons_.size());
            for (auto& btn : octaveButtons_)
                btn.setBounds(octRow.removeFromLeft(octW).reduced(2));

            content.removeFromTop(10);
            auto holdRow = content.removeFromTop(layout::kArpHoldRowHeight).reduced(0, 2);
            holdRowBounds_ = holdRow;
            latchButton_.setBounds(holdRow.removeFromRight(layout::kArpRoutingToggleWidth)
                                       .withSizeKeepingCentre(layout::kArpRoutingToggleWidth,
                                                              layout::kArpRoutingToggleHeight));
            holdLabel_.setBounds(holdRow.reduced(8, 0).withTrimmedLeft(20));
        }

        // Timing panel — Figma `pattern-controls` (4:1515)
        {
            auto content = timingPanel_.getContentBounds().reduced(12);
            clockResolutionLabel_.setBounds(content.removeFromTop(10));
            content.removeFromTop(4);

            auto syncGrid = content.removeFromTop(layout::kArpSyncGridHeight).withWidth(layout::kArpSyncGridWidth);
            const int syncGap = 2;
            for (std::size_t i = 0; i < syncButtons_.size(); ++i)
            {
                const int row = static_cast<int>(i / 2);
                const int col = static_cast<int>(i % 2);
                syncButtons_[i].setBounds(syncGrid.getX() + col * (layout::kArpSyncButtonWidth + syncGap),
                                          syncGrid.getY() + row * (layout::kArpSyncButtonHeight + syncGap),
                                          layout::kArpSyncButtonWidth, layout::kArpSyncButtonHeight);
            }

            content.removeFromTop(8);
            auto knobRow = content.removeFromTop(layout::kArpTimingKnobHeight);
            swingKnob_->setBounds(knobRow.removeFromLeft(layout::kArpTimingKnobWidth).reduced(0, 0));
            knobRow.removeFromLeft(42);
            auto gateArea = knobRow.removeFromLeft(layout::kArpTimingKnobWidth);
            gateSlider_.setBounds(gateArea.removeFromTop(layout::kArpTimingKnobDialSize).reduced(4, 0));
            gateLabel_.setBounds(gateArea.removeFromTop(10));
            knobRow.removeFromLeft(42);
            numStepsKnob_->setBounds(knobRow.removeFromLeft(layout::kArpTimingKnobWidth).reduced(0, 0));
            knobRow.removeFromLeft(42);
            rateHzKnob_->setBounds(knobRow.removeFromLeft(layout::kArpTimingKnobWidth).reduced(0, 0));
        }

        // Velocity curve card header
        {
            auto headerArea = curvePanel_.getContentBounds().reduced(12).removeFromTop(11);
            curveTypeLabel_.setBounds(headerArea);
            curveCaption_.setBounds({});
        }

        // Engine routing rows — Figma `switchboard` (4:1574)
        {
            auto content = routingPanel_.getContentBounds().reduced(12);
            for (std::size_t i = 0; i < 8; ++i)
            {
                auto row = content.removeFromTop(layout::kArpRoutingRowHeight).reduced(0, 2);
                mixButtons_[i]->setBounds(row.removeFromRight(layout::kArpRoutingToggleWidth)
                                              .withSizeKeepingCentre(layout::kArpRoutingToggleWidth,
                                                                     layout::kArpRoutingToggleHeight));
                row.removeFromRight(6);
                auto textCol = row.reduced(6, 0);
                routeEngineLabels_[i].setBounds(textCol.removeFromTop(11));
                routeSubLabels_[i].setBounds(textCol.removeFromTop(8));
            }
        }

        leftPanel_.toFront(false);
        routingPanel_.toFront(false);
    }

    bool ArpPanelOverlay::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
