#include "WavetableLabPanel.h"

#include <cmath>
#include <limits>

#include "../PerformanceMetricsUi.h"
#include "../PlayModeLayout.h"
#include "../theme/DeckedKnobDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../theme/ObsidianRotary.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/oscillator/WavetableTable.hpp"
#include "pw8/patch/Patch.hpp"
#include "state/PluginState.h"
#include "wireframe/WavetableMeshPaint.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] float readParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        void setEngineType(juce::AudioProcessorValueTreeState& apvts, std::size_t opIndex, algorithm::EngineType type)
        {
            if (auto* param = apvts.getParameter(operatorParamId(opIndex, "Engine")))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(static_cast<int>(type))));
        }

        void styleWaveformButton(juce::TextButton& btn, const juce::String& text)
        {
            btn.setButtonText(text);
            btn.setClickingTogglesState(false);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        }

        void highlightWaveformButton(juce::TextButton& btn, bool active)
        {
            btn.setColour(juce::TextButton::buttonColourId, active ? palette::kAccent.withAlpha(0.13f) : palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, active ? palette::kTextPrimary : palette::kTextSecondary);
        }

        [[nodiscard]] juce::String ratioToSemitonesText(float ratio)
        {
            if (ratio <= 0.0f)
                return "+0 ST";
            const float semitones = 12.0f * std::log2(ratio);
            const int rounded = juce::roundToInt(semitones);
            return (rounded >= 0 ? "+" : "") + juce::String(rounded) + " ST";
        }

        [[nodiscard]] juce::String phaseBendToCentsText(float bend)
        {
            const int cents = juce::roundToInt(bend * 100.0f);
            return (cents >= 0 ? "+" : "") + juce::String(cents) + " CT";
        }

        [[nodiscard]] oscillator::WtWarpParams loadWavetableWarpParams(juce::AudioProcessorValueTreeState& apvts,
                                                                       std::size_t opIndex)
        {
            oscillator::WtWarpParams warpParams;
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtBend")))
                warpParams.bend = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtAsymmetry")))
                warpParams.asymmetry = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtSyncRatio")))
                warpParams.syncRatio = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtSyncAmount")))
                warpParams.syncAmount = raw->load();
            return warpParams;
        }
    } // namespace

    WavetableLabPanel::WavetableLabPanel(PatchworkEightProcessor& processor)
        : processor_(processor),
          apvts_(processor.apvts),
          meshPanel_("3D SPECTRUM PREVIEW"),
          morphPanel_("MORPH CONTROLS"),
          meshView_(processor),
          harmonicPanel_("HARMONIC PARTIALS (1-16)")
    {
        addAndMakeVisible(backButton_);
        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };

        badgeLabel_.setText("UX-09", juce::dontSendNotification);
        badgeLabel_.setFont(fonts::label(9.0f));
        badgeLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        badgeLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(badgeLabel_);

        titleLabel_.setText("WAVETABLE EDITOR", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        for (int i = 0; i < 8; ++i)
            engineCombo_.addItem("ENGINE " + juce::String(i + 1).paddedLeft('0', 2), i + 1);
        engineCombo_.onChange = [this] { bindEngine(engineCombo_.getSelectedId() - 1); };
        addAndMakeVisible(engineCombo_);

        engineHintLabel_.setFont(fonts::label(9.0f));
        engineHintLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        engineHintLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(engineHintLabel_);

        subtitleTitleLabel_.setFont(fonts::label(9.0f));
        subtitleTitleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(subtitleTitleLabel_);

        subtitleMorphLabel_.setFont(fonts::label(8.0f));
        subtitleMorphLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        subtitleMorphLabel_.setJustificationType(juce::Justification::centredRight);
        subtitleMorphLabel_.setText("MORPH MODE: SPECTRAL CROSSFADE", juce::dontSendNotification);
        addAndMakeVisible(subtitleMorphLabel_);

        footerCpuLabel_.setFont(fonts::label(8.0f));
        footerCpuLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerCpuLabel_.setText("CPU", juce::dontSendNotification);
        addAndMakeVisible(footerCpuLabel_);

        footerCpuPercentLabel_.setFont(fonts::label(8.0f));
        footerCpuPercentLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        addAndMakeVisible(footerCpuPercentLabel_);

        footerVoicesLabel_.setFont(fonts::label(8.0f));
        footerVoicesLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerVoicesLabel_.setText("VOICES:", juce::dontSendNotification);
        addAndMakeVisible(footerVoicesLabel_);

        footerVoicesCountLabel_.setFont(fonts::label(8.0f));
        footerVoicesCountLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(footerVoicesCountLabel_);

        footerMidiLabel_.setFont(fonts::label(8.0f));
        footerMidiLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerMidiLabel_.setText("MIDI IN", juce::dontSendNotification);
        addAndMakeVisible(footerMidiLabel_);

        footerVersionLabel_.setFont(fonts::label(8.0f));
        footerVersionLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        footerVersionLabel_.setJustificationType(juce::Justification::centredRight);
        footerVersionLabel_.setText("MURMUR DSP ENGINE VST3 · VERSION 1.4.2 · © 2026 MURMUR AUDIO",
                                    juce::dontSendNotification);
        addAndMakeVisible(footerVersionLabel_);

        addAndMakeVisible(oscConfigPanel_);
        addAndMakeVisible(meshPanel_);
        meshPanel_.addAndMakeVisible(meshView_);
        meshView_.setShowLoadButton(true);
        addAndMakeVisible(morphPanel_);
        addAndMakeVisible(harmonicPanel_);

        static constexpr const char* kWaveformLabels[] = {"SINE", "SAW", "SQUARE", "TRIANGLE", "NOISE", "WAVETABLE"};
        for (std::size_t i = 0; i < waveformButtons_.size(); ++i)
        {
            styleWaveformButton(waveformButtons_[i], kWaveformLabels[i]);
            waveformButtons_[i].onClick = [this, i] { selectWaveformSource(static_cast<int>(i)); };
            oscConfigPanel_.addAndMakeVisible(waveformButtons_[i]);
        }

        static constexpr const char* kMorphLabels[] = {"SPECTRAL", "FORMANT", "CROSSFADE"};
        for (std::size_t i = 0; i < morphTypeButtons_.size(); ++i)
        {
            morphTypeButtons_[i].setButtonText(kMorphLabels[i]);
            morphTypeButtons_[i].setClickingTogglesState(false);
            morphTypeButtons_[i].onClick = [this, i] { selectMorphType(static_cast<int>(i)); };
            morphPanel_.addAndMakeVisible(morphTypeButtons_[i]);
        }

        morphPositionLabel_.setFont(fonts::label(7.0f));
        morphPositionLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        morphPositionLabel_.setJustificationType(juce::Justification::centred);
        morphPanel_.addAndMakeVisible(morphPositionLabel_);

        frameStripTitleLabel_.setText("WAVETABLE SEQUENCE HIGHLIGHTS", juce::dontSendNotification);
        frameStripTitleLabel_.setFont(fonts::label(8.0f));
        frameStripTitleLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(frameStripTitleLabel_);

        phaseDispersionLabel_.setText("PHASE DISPERSION", juce::dontSendNotification);
        phaseDispersionLabel_.setFont(fonts::label(8.0f));
        phaseDispersionLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        phaseDispersionToggle_.setClickingTogglesState(true);
        phaseDispersionToggle_.onClick = [this] {
            if (auto* param = apvts_.getParameter(kUnisonPhaseRandomId))
            {
                const float on = phaseDispersionToggle_.getToggleState() ? 1.0f : 0.0f;
                param->setValueNotifyingHost(param->convertTo0to1(on));
            }
        };
        phaseDispersionToggle_.setColour(juce::TextButton::buttonColourId, palette::kBackgroundBottom);
        phaseDispersionToggle_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        oscConfigPanel_.addAndMakeVisible(phaseDispersionLabel_);
        oscConfigPanel_.addAndMakeVisible(phaseDispersionToggle_);

        static constexpr float kSeedHarmonics[] = {0.82f, 0.74f, 0.66f, 0.18f, 0.42f, 0.10f, 0.58f, 0.50f,
                                                 0.66f, 0.18f, 0.18f, 0.66f, 0.26f, 0.02f, 0.10f, 0.26f};
        for (std::size_t i = 0; i < harmonicHeights_.size(); ++i)
            harmonicHeights_[i] = kSeedHarmonics[i];

        refreshHarmonicHeights();

        static auto detuneLabel = [](float cents) {
            return juce::String(juce::roundToInt(juce::jlimit(0.0f, 100.0f, cents * 0.5f))) + "%";
        };
        static auto spreadLabel = [](float spread) {
            return juce::String(juce::roundToInt(spread * 100.0f)) + "%";
        };
        static auto voicesLabel = [](float voices) {
            return juce::String(juce::jmax(1, juce::roundToInt(voices))) + " V";
        };

        unisonKnobs_[0] = std::make_unique<GlowKnob>(apvts_, kUnisonVoicesId, "VOICES", voicesLabel, palette::kAccentWarm);
        unisonKnobs_[1] = std::make_unique<GlowKnob>(apvts_, kUnisonDetuneId, "DETUNE", detuneLabel, palette::kAccentWarm);
        unisonKnobs_[2] = std::make_unique<GlowKnob>(apvts_, kUnisonSpreadId, "WIDTH", spreadLabel, palette::kAccentWarm);
        for (auto& knob : unisonKnobs_)
        {
            if (knob == nullptr)
                continue;
            knob->setDeckedStyle(true, GlowKnob::DeckedKnobSize::Small);
            knob->setMaxDialDiameter(26);
            oscConfigPanel_.addAndMakeVisible(*knob);
        }

        bindEngine(0);
        refreshMorphTypeButtons();
        startTimerHz(8);
    }

    WavetableLabPanel::~WavetableLabPanel() { stopTimer(); }

    void WavetableLabPanel::ensureWavetableEngine()
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        const int engine = static_cast<int>(readParam(apvts_, operatorParamId(op, "Engine"), 0.0f) + 0.5f);
        const auto type = static_cast<algorithm::EngineType>(engine);
        if (type != algorithm::EngineType::Wavetable && type != algorithm::EngineType::Granular)
            setEngineType(apvts_, op, algorithm::EngineType::Wavetable);
        processor_.syncCurrentPatchFromApvts();
    }

    void WavetableLabPanel::bindEngine(int engineIndex)
    {
        engineIndex_ = juce::jlimit(0, 7, engineIndex);
        engineCombo_.setSelectedId(engineIndex_ + 1, juce::dontSendNotification);

        const auto op = static_cast<std::size_t>(engineIndex_);
        const int engine = static_cast<int>(readParam(apvts_, operatorParamId(op, "Engine"), 0.0f) + 0.5f);
        const bool granular = static_cast<algorithm::EngineType>(engine) == algorithm::EngineType::Granular;
        meshView_.setGranularOverlay(granular);
        meshView_.showNode(engineIndex_);
        meshView_.ensureDefaultWavetableLoaded();

        subtitleTitleLabel_.setText("ENGINE " + juce::String(engineIndex_ + 1).paddedLeft('0', 2)
                                        + " — WAVETABLE EDITOR",
                                    juce::dontSendNotification);
        engineHintLabel_.setText(granular ? "Granular engine — grain windows overlaid on mesh"
                                          : "Wavetable engine — browse factory tables or Load custom JSON",
                                 juce::dontSendNotification);

        wtPosKnob_.reset();
        phaseBendKnob_.reset();
        leftCoarseKnob_.reset();
        leftFineKnob_.reset();

        wtPosKnob_ = std::make_unique<GlowKnob>(apvts_, operatorParamId(op, "WavetablePos"), "POSITION");
        phaseBendKnob_ = std::make_unique<GlowKnob>(apvts_, operatorParamId(op, "PhaseBend"), "PHASE");
        leftCoarseKnob_ = std::make_unique<GlowKnob>(apvts_, operatorParamId(op, "FreqRatio"), "COARSE",
                                                    ratioToSemitonesText);
        leftFineKnob_ = std::make_unique<GlowKnob>(apvts_, operatorParamId(op, "PhaseBend"), "FINE", phaseBendToCentsText);
        for (auto* knob : {wtPosKnob_.get(), phaseBendKnob_.get()})
        {
            knob->setMaxDialDiameter(58);
            morphPanel_.addAndMakeVisible(*knob);
        }
        for (auto* knob : {leftCoarseKnob_.get(), leftFineKnob_.get()})
        {
            knob->setMaxDialDiameter(28);
            knob->setDeckedStyle(true, GlowKnob::DeckedKnobSize::Small);
            oscConfigPanel_.addAndMakeVisible(*knob);
        }

        refreshWaveformButtons();
        refreshMorphTypeButtons();
        refreshHarmonicHeights();
        layoutOscConfigPanel();
        resized();
    }

    int WavetableLabPanel::activeWaveformSourceIndex() const
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        const int engine = static_cast<int>(readParam(apvts_, operatorParamId(op, "Engine"), 0.0f) + 0.5f);
        const auto type = static_cast<algorithm::EngineType>(engine);
        if (type == algorithm::EngineType::Wavetable || type == algorithm::EngineType::Granular)
            return 5;
        if (type == algorithm::EngineType::NoiseChaos)
            return 4;
        if (type != algorithm::EngineType::Classic)
            return 5;

        const int waveform = static_cast<int>(readParam(apvts_, operatorParamId(op, "Waveform"), 2.0f) + 0.5f);
        switch (waveform)
        {
            case 0: return 0;
            case 1: return 3;
            case 2: return 1;
            case 3: return 2;
            default: return 0;
        }
    }

    void WavetableLabPanel::refreshWaveformButtons()
    {
        const int active = activeWaveformSourceIndex();
        for (std::size_t i = 0; i < waveformButtons_.size(); ++i)
            highlightWaveformButton(waveformButtons_[i], static_cast<int>(i) == active);
    }

    int WavetableLabPanel::wavetableFrameCount() const
    {
        if (const auto* table = processor_.getActiveWavetableTable(static_cast<std::size_t>(engineIndex_)))
            return juce::jmax(1, table->numFrames);
        return 64;
    }

    int WavetableLabPanel::currentWavetableFrameIndex() const
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        const float pos = readParam(apvts_, operatorParamId(op, "WavetablePos"), 0.0f);
        const int frames = wavetableFrameCount();
        return juce::jlimit(0, frames - 1, juce::roundToInt(pos * static_cast<float>(frames - 1)));
    }

    void WavetableLabPanel::selectMorphType(int morphIndex)
    {
        morphTypeIndex_ = juce::jlimit(0, 2, morphIndex);
        const auto op = static_cast<std::size_t>(engineIndex_);
        if (auto* param = apvts_.getParameter(operatorParamId(op, "WtMorphMode")))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(morphTypeIndex_)));
        refreshMorphTypeButtons();
        static constexpr const char* kModes[] = {"SPECTRAL", "FORMANT", "CROSSFADE"};
        subtitleMorphLabel_.setText("MORPH MODE: " + juce::String(kModes[static_cast<std::size_t>(morphTypeIndex_)]),
                                    juce::dontSendNotification);
    }

    void WavetableLabPanel::refreshMorphTypeButtons()
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        morphTypeIndex_ = static_cast<int>(readParam(apvts_, operatorParamId(op, "WtMorphMode"), 0.0f) + 0.5f);
        morphTypeIndex_ = juce::jlimit(0, 2, morphTypeIndex_);
        for (std::size_t i = 0; i < morphTypeButtons_.size(); ++i)
        {
            const bool active = static_cast<int>(i) == morphTypeIndex_;
            morphTypeButtons_[i].setColour(juce::TextButton::buttonColourId,
                                           active ? palette::kAccent.withAlpha(0.13f) : palette::kBackgroundBottom);
            morphTypeButtons_[i].setColour(juce::TextButton::textColourOffId,
                                           active ? palette::kAccent : palette::kTextSecondary);
        }
    }

    void WavetableLabPanel::selectFrameHighlight(int stripIndex)
    {
        selectedFrameStripIndex_ = juce::jlimit(0, layout::kDesignWavetableFrameStripCount - 1, stripIndex);
        const int frames = wavetableFrameCount();
        const int frameNumber =
            juce::jmax(1, juce::roundToInt(static_cast<float>(frames) * static_cast<float>(selectedFrameStripIndex_ + 1)
                                           / static_cast<float>(layout::kDesignWavetableFrameStripCount)));
        const float pos = frames > 1 ? static_cast<float>(frameNumber - 1) / static_cast<float>(frames - 1) : 0.0f;

        const auto op = static_cast<std::size_t>(engineIndex_);
        if (auto* param = apvts_.getParameter(operatorParamId(op, "WavetablePos")))
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, pos));

        morphPositionLabel_.setText("Frame " + juce::String(frameNumber), juce::dontSendNotification);
        refreshFrameStripSelection();
        repaint(frameStripBounds_);
    }

    void WavetableLabPanel::refreshFrameStripSelection()
    {
        const int currentFrame = currentWavetableFrameIndex() + 1;
        const int frames = wavetableFrameCount();
        int bestIndex = 0;
        int bestDistance = std::numeric_limits<int>::max();
        for (int i = 0; i < layout::kDesignWavetableFrameStripCount; ++i)
        {
            const int frameNumber =
                juce::jmax(1, juce::roundToInt(static_cast<float>(frames) * static_cast<float>(i + 1)
                                               / static_cast<float>(layout::kDesignWavetableFrameStripCount)));
            const int distance = std::abs(frameNumber - currentFrame);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }
        selectedFrameStripIndex_ = bestIndex;
        morphPositionLabel_.setText("Frame " + juce::String(currentFrame), juce::dontSendNotification);
    }

    void WavetableLabPanel::layoutFrameStrip()
    {
        if (frameStripBounds_.isEmpty())
            return;

        auto row = frameStripBounds_.reduced(10, 0);
        row.removeFromTop(18);
        const int miniW = juce::jmin(layout::kDesignWavetableFrameMiniWidth,
                                     (row.getWidth() - (layout::kDesignWavetableFrameStripCount - 1) * 4)
                                         / layout::kDesignWavetableFrameStripCount);
        for (int i = 0; i < layout::kDesignWavetableFrameStripCount; ++i)
        {
            frameMiniBounds_[static_cast<std::size_t>(i)] =
                row.removeFromLeft(miniW).withHeight(layout::kDesignWavetableFrameMiniHeight);
            row.removeFromLeft(4);
        }
    }

    void WavetableLabPanel::refreshHarmonicHeights()
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        if (const auto* table = processor_.getActiveWavetableTable(op); table != nullptr && table->isValid())
        {
            const float pos = readParam(apvts_, operatorParamId(op, "WavetablePos"), 0.0f);
            const auto warpParams = loadWavetableWarpParams(apvts_, op);
            wireframe::computeWavetableHarmonicMagnitudes(table, pos, warpParams, harmonicHeights_);
        }
    }

    void WavetableLabPanel::paintFrameStrip(juce::Graphics& g) const
    {
        if (frameStripBounds_.isEmpty())
            return;

        paintPanelCard(g, frameStripBounds_);

        const int frames = wavetableFrameCount();
        g.setFont(fonts::label(7.0f));
        for (int i = 0; i < layout::kDesignWavetableFrameStripCount; ++i)
        {
            const auto tile = frameMiniBounds_[static_cast<std::size_t>(i)];
            if (tile.isEmpty())
                continue;

            const bool selected = i == selectedFrameStripIndex_;
            const int frameNumber =
                juce::jmax(1, juce::roundToInt(static_cast<float>(frames) * static_cast<float>(i + 1)
                                               / static_cast<float>(layout::kDesignWavetableFrameStripCount)));

            g.setColour(selected ? palette::kAccent.withAlpha(0.13f) : palette::kPanelRaised);
            g.fillRoundedRectangle(tile.toFloat(), 4.0f);
            g.setColour(selected ? palette::kAccent : palette::kBorder.withAlpha(0.45f));
            g.drawRoundedRectangle(tile.toFloat().reduced(0.5f), 4.0f, 1.0f);

            auto labelArea = tile.reduced(4, 2);
            g.setColour(selected ? palette::kAccent : palette::kTextSecondary);
            g.drawText("F-" + juce::String(frameNumber), labelArea.removeFromTop(10), juce::Justification::centred);

            auto waveArea = labelArea.reduced(2, 0);
            g.setColour(palette::kBackgroundBottom);
            g.fillRoundedRectangle(waveArea.toFloat(), 2.0f);

            const auto op = static_cast<std::size_t>(engineIndex_);
            const float framePos =
                frames > 1 ? static_cast<float>(frameNumber - 1) / static_cast<float>(frames - 1) : 0.0f;
            if (const auto* table = processor_.getActiveWavetableTable(op); table != nullptr && table->isValid())
            {
                const auto warpParams = loadWavetableWarpParams(apvts_, op);
                wireframe::paintWavetableFrameWaveform(g, waveArea.toFloat(), table, framePos, warpParams, selected,
                                                       24);
            }
            else
            {
                g.setColour(selected ? palette::kAccent.withAlpha(0.85f) : palette::kAccent.withAlpha(0.45f));
                juce::Path miniWave;
                const float midY = static_cast<float>(waveArea.getCentreY());
                const float amp = static_cast<float>(waveArea.getHeight()) * 0.35f;
                miniWave.startNewSubPath(static_cast<float>(waveArea.getX()), midY);
                for (int s = 0; s <= 24; ++s)
                {
                    const float t = static_cast<float>(s) / 24.0f;
                    const float x = static_cast<float>(waveArea.getX()) + t * static_cast<float>(waveArea.getWidth());
                    const float y = midY - std::sin(t * juce::MathConstants<float>::twoPi * (1.0f + static_cast<float>(i) * 0.15f)) * amp;
                    miniWave.lineTo(x, y);
                }
                g.strokePath(miniWave, juce::PathStrokeType(1.0f));
            }
        }
    }

    void WavetableLabPanel::mouseDown(const juce::MouseEvent& event)
    {
        for (int i = 0; i < layout::kDesignWavetableFrameStripCount; ++i)
        {
            if (frameMiniBounds_[static_cast<std::size_t>(i)].contains(event.getPosition()))
            {
                selectFrameHighlight(i);
                return;
            }
        }
    }

    void WavetableLabPanel::selectWaveformSource(int sourceIndex)
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        switch (sourceIndex)
        {
            case 0:
                setEngineType(apvts_, op, algorithm::EngineType::Classic);
                if (auto* param = apvts_.getParameter(operatorParamId(op, "Waveform")))
                    param->setValueNotifyingHost(param->convertTo0to1(0.0f));
                break;
            case 1:
                setEngineType(apvts_, op, algorithm::EngineType::Classic);
                if (auto* param = apvts_.getParameter(operatorParamId(op, "Waveform")))
                    param->setValueNotifyingHost(param->convertTo0to1(2.0f));
                break;
            case 2:
                setEngineType(apvts_, op, algorithm::EngineType::Classic);
                if (auto* param = apvts_.getParameter(operatorParamId(op, "Waveform")))
                    param->setValueNotifyingHost(param->convertTo0to1(3.0f));
                break;
            case 3:
                setEngineType(apvts_, op, algorithm::EngineType::Classic);
                if (auto* param = apvts_.getParameter(operatorParamId(op, "Waveform")))
                    param->setValueNotifyingHost(param->convertTo0to1(1.0f));
                break;
            case 4:
                setEngineType(apvts_, op, algorithm::EngineType::NoiseChaos);
                break;
            case 5:
            default:
                setEngineType(apvts_, op, algorithm::EngineType::Wavetable);
                break;
        }

        processor_.syncCurrentPatchFromApvts();

        const int engine = static_cast<int>(readParam(apvts_, operatorParamId(op, "Engine"), 0.0f) + 0.5f);
        const bool granular = static_cast<algorithm::EngineType>(engine) == algorithm::EngineType::Granular;
        meshView_.setGranularOverlay(granular);
        meshView_.showNode(engineIndex_);
        refreshWaveformButtons();
        repaint();
    }

    void WavetableLabPanel::layoutOscConfigPanel()
    {
        auto content = oscConfigPanel_.getContentBounds().reduced(2);
        content.removeFromTop(4);

        auto waveformHeader = content.removeFromTop(10);
        juce::ignoreUnused(waveformHeader);
        content.removeFromTop(4);

        for (auto& btn : waveformButtons_)
        {
            btn.setBounds(content.removeFromTop(24).reduced(0, 1));
            content.removeFromTop(4);
        }

        content.removeFromTop(6);
        auto tuningCard = content.removeFromTop(88);
        tuningCard = tuningCard.reduced(0, 2);
        auto tuningKnobs = tuningCard.removeFromBottom(56);
        if (leftCoarseKnob_ != nullptr)
        {
            leftCoarseKnob_->setBounds(tuningKnobs.removeFromLeft(tuningKnobs.getWidth() / 2).reduced(6, 0));
            leftFineKnob_->setBounds(tuningKnobs.reduced(6, 0));
        }

        content.removeFromTop(8);
        auto unisonCard = content;
        auto unisonKnobs = unisonCard.removeFromTop(72);
        const int uniW = unisonKnobs.getWidth() / 3;
        for (int i = 0; i < 3; ++i)
        {
            unisonKnobBounds_[static_cast<std::size_t>(i)] = unisonKnobs.removeFromLeft(uniW).reduced(4, 0);
            if (unisonKnobs_[static_cast<std::size_t>(i)] != nullptr)
                unisonKnobs_[static_cast<std::size_t>(i)]->setBounds(unisonKnobBounds_[static_cast<std::size_t>(i)]);
        }

        phaseDispersionBounds_ = unisonCard.removeFromTop(22).reduced(0, 2);
        phaseDispersionLabel_.setBounds(phaseDispersionBounds_.removeFromLeft(phaseDispersionBounds_.getWidth() - 28));
        phaseDispersionToggle_.setBounds(phaseDispersionBounds_.removeFromRight(24));
    }

    void WavetableLabPanel::showForEngine(int engineIndex)
    {
        bindEngine(engineIndex);
        ensureWavetableEngine();
        setVisible(true);
        toFront(false);
    }

    void WavetableLabPanel::dismiss()
    {
        setVisible(false);
    }

    void WavetableLabPanel::setEmbeddedInDesignMode(bool embedded)
    {
        if (embeddedInDesignMode_ == embedded)
            return;

        embeddedInDesignMode_ = embedded;
        backButton_.setButtonText(embedded ? "← DESIGN" : "← PLAY BOARD");
        badgeLabel_.setVisible(!embedded);
        titleLabel_.setVisible(!embedded);
        engineCombo_.setVisible(!embedded);
        engineHintLabel_.setVisible(!embedded);
        resized();
    }

    void WavetableLabPanel::timerCallback()
    {
        const auto metrics = readPerformanceMetrics(processor_);
        footerCpuLoadPercent_ = metrics.cpuPercent;
        footerCpuPercentLabel_.setText(formatCpuPercent(metrics.cpuPercent), juce::dontSendNotification);
        footerVoicesCountLabel_.setText(formatVoiceCount(metrics.activeVoices, metrics.maxVoices),
                                        juce::dontSendNotification);

        const float phaseRandom = readParam(apvts_, kUnisonPhaseRandomId, 0.0f);
        phaseDispersionOn_ = phaseRandom >= 0.5f;
        phaseDispersionToggle_.setToggleState(phaseDispersionOn_, juce::dontSendNotification);
        phaseDispersionToggle_.setButtonText(phaseDispersionOn_ ? "ON" : "OFF");

        refreshMorphTypeButtons();
        phaseDispersionToggle_.setColour(juce::TextButton::textColourOffId,
                                         phaseDispersionOn_ ? palette::kAccent : palette::kTextDim);
        refreshWaveformButtons();
        refreshFrameStripSelection();
        refreshHarmonicHeights();
        repaint(oscConfigPanel_.getBounds());
        repaint(frameStripBounds_);
        repaint(morphPanel_.getBounds());
        if (!harmonicEditorBounds_.isEmpty())
            repaint(harmonicEditorBounds_);
    }

    void WavetableLabPanel::paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8.0f, 1.0f);
    }

    void WavetableLabPanel::paintHarmonicEditor(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);

        auto plot = bounds.reduced(10, 12);
        const float barGap = 4.0f;
        const float barW = (static_cast<float>(plot.getWidth()) - barGap * 15.0f) / 16.0f;

        for (std::size_t i = 0; i < harmonicHeights_.size(); ++i)
        {
            const float x = static_cast<float>(plot.getX()) + static_cast<float>(i) * (barW + barGap);
            const float h = harmonicHeights_[i] * static_cast<float>(plot.getHeight());
            const bool active = harmonicHeights_[i] > 0.2f;
            g.setColour(active ? palette::kAccent : palette::kTextDim);
            g.fillRect(x, static_cast<float>(plot.getBottom()) - h, barW, h);
        }

        g.setColour(palette::kAccent);
        g.setFont(fonts::label(7.0f));
        g.drawText("ADDITIVE", bounds.removeFromTop(12).removeFromRight(48), juce::Justification::centredRight);
    }

    void WavetableLabPanel::paintOscConfigExtras(juce::Graphics& g) const
    {
        if (!phaseDispersionBounds_.isEmpty())
        {
            g.setColour(palette::kBackgroundBottom);
            g.fillRoundedRectangle(phaseDispersionBounds_.toFloat(), 4.0f);
            g.setColour(palette::kBorder.withAlpha(0.45f));
            g.drawRoundedRectangle(phaseDispersionBounds_.toFloat().reduced(0.5f), 4.0f, 1.0f);
        }
    }

    void WavetableLabPanel::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop);

        if (!embeddedInDesignMode_)
        {
            auto badge = badgeLabel_.getBounds().toFloat().expanded(4.0f, 2.0f);
            g.setColour(palette::kAccent.withAlpha(0.13f));
            g.fillRoundedRectangle(badge, 4.0f);
            g.setColour(palette::kAccent);
            g.drawRoundedRectangle(badge, 4.0f, 1.0f);
        }

        if (!footerBounds_.isEmpty())
        {
            paintPanelCard(g, footerBounds_);

            if (!cpuBarBounds_.isEmpty())
            {
                g.setColour(palette::kBackgroundBottom);
                g.fillRoundedRectangle(cpuBarBounds_.toFloat(), 2.0f);
                g.setColour(palette::kAccent);
                const auto fill = cpuBarBounds_.withWidth(juce::roundToInt(static_cast<float>(cpuBarBounds_.getWidth())
                                                                         * cpuBarFillRatio(footerCpuLoadPercent_)));
                g.fillRoundedRectangle(fill.toFloat(), 2.0f);
            }
        }

        if (!harmonicEditorBounds_.isEmpty())
            paintHarmonicEditor(g, harmonicEditorBounds_);

        paintFrameStrip(g);
    }

    void WavetableLabPanel::paintOverChildren(juce::Graphics& g)
    {
        paintOscConfigExtras(g);
    }

    void WavetableLabPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDesignWavetablePageOuterMargin);

        const int headerHeight = embeddedInDesignMode_ ? layout::kDesignLabPanelHeaderHeight : 36;
        auto header = bounds.removeFromTop(headerHeight);
        backButton_.setBounds(header.removeFromLeft(120));
        if (!embeddedInDesignMode_)
        {
            header.removeFromLeft(8);
            badgeLabel_.setBounds(header.removeFromLeft(36).withSizeKeepingCentre(36, 18));
            header.removeFromLeft(8);
            titleLabel_.setBounds(header.removeFromLeft(180));
            engineCombo_.setBounds(header.removeFromRight(120).reduced(2));
        }

        if (!embeddedInDesignMode_)
        {
            bounds.removeFromTop(6);
            engineHintLabel_.setBounds(bounds.removeFromTop(18).reduced(2, 0));
            bounds.removeFromTop(8);
        }

        footerBounds_ = bounds.removeFromBottom(layout::kDesignWavetableFooterHeight);
        bounds.removeFromBottom(layout::kDesignWavetablePageSectionGap);

        auto footer = footerBounds_.reduced(16, 0);
        footerVersionLabel_.setBounds(footer.removeFromRight(footer.getWidth() / 2).withHeight(10).withY(footer.getY() + 4));
        auto leftFooter = footer;
        footerCpuLabel_.setBounds(leftFooter.removeFromLeft(15).withHeight(10).withY(leftFooter.getY() + 4));
        leftFooter.removeFromLeft(6);
        cpuBarBounds_ = leftFooter.removeFromLeft(40).withHeight(4).withY(leftFooter.getY() + 6);
        leftFooter.removeFromLeft(6);
        footerCpuPercentLabel_.setBounds(leftFooter.removeFromLeft(24).withHeight(10).withY(leftFooter.getY() + 4));
        leftFooter.removeFromLeft(16);
        footerVoicesLabel_.setBounds(leftFooter.removeFromLeft(34).withHeight(10).withY(leftFooter.getY() + 4));
        footerVoicesCountLabel_.setBounds(leftFooter.removeFromLeft(40).withHeight(10).withY(leftFooter.getY() + 4));
        leftFooter.removeFromLeft(16);
        footerMidiLabel_.setBounds(leftFooter.removeFromLeft(40).withHeight(10).withY(leftFooter.getY() + 4));

        auto subtitle = bounds.removeFromTop(layout::kDesignWavetableSubtitleBarHeight);
        subtitleTitleLabel_.setBounds(subtitle.removeFromLeft(subtitle.getWidth() / 2).reduced(16, 0));
        subtitleMorphLabel_.setBounds(subtitle.reduced(16, 0));

        bounds.removeFromTop(layout::kDesignWavetablePageSectionGap);

        auto right = bounds.removeFromRight(layout::kDesignWavetableRightColumnWidth);
        bounds.removeFromRight(layout::kDesignWavetableEditorGap);
        auto left = bounds.removeFromLeft(layout::kDesignWavetableLeftColumnWidth);
        bounds.removeFromLeft(layout::kDesignWavetableEditorGap);
        auto center = bounds;

        oscConfigPanel_.setBounds(left);
        layoutOscConfigPanel();

        frameStripBounds_ = center.removeFromBottom(layout::kDesignWavetableFrameStripHeight);
        center.removeFromBottom(layout::kDesignWavetablePageSectionGap);
        frameStripTitleLabel_.setBounds(frameStripBounds_.removeFromTop(14).reduced(10, 0));
        layoutFrameStrip();

        meshPanel_.setBounds(center);
        meshView_.setBounds(meshPanel_.getContentBounds().reduced(4));

        morphPanel_.setBounds(right.removeFromTop(right.getHeight() / 2));
        harmonicPanel_.setBounds(right);

        auto morphContent = morphPanel_.getContentBounds().reduced(8);
        const int knobH = 72;
        if (wtPosKnob_ != nullptr)
        {
            auto knobRow = morphContent.removeFromTop(knobH + 18);
            wtPosKnob_->setBounds(knobRow.removeFromLeft(72).reduced(4));
            auto pillCol = knobRow.reduced(4, 8);
            pillCol.removeFromTop(12);
            for (auto& pill : morphTypeButtons_)
            {
                pill.setBounds(pillCol.removeFromTop(18));
                pillCol.removeFromTop(4);
            }
            morphPositionLabel_.setBounds(knobRow.removeFromTop(14));
        }

        harmonicEditorBounds_ =
            harmonicPanel_.getContentBounds().withSizeKeepingCentre(layout::kDesignWavetableHarmonicEditorWidth,
                                                                    layout::kDesignWavetableHarmonicEditorHeight);
    }

    bool WavetableLabPanel::keyPressed(const juce::KeyPress& key)
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
