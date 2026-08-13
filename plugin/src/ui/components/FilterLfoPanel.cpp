#include "FilterLfoPanel.h"

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::String filterModeToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "LOWPASS";
                case 1: return "HIGHPASS";
                case 2: return "BANDPASS";
                case 3: return "NOTCH";
                case 4: return "PEAK";
                default: return juce::String(v);
            }
        }

        juce::String lfoWaveformToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "SINE";
                case 1: return "TRIANGLE";
                case 2: return "SAW";
                case 3: return "SQUARE";
                case 4: return "S & H";
                case 5: return "S.RANDOM";
                default: return juce::String(v);
            }
        }

        juce::String lfoModeToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "FREE";
                case 1: return "RETRIGGER";
                case 2: return "ONE SHOT";
                case 3: return "TEMPO SYNC";
                default: return juce::String(v);
            }
        }
    } // namespace

    FilterLfoPanel::FilterLfoPanel(PatchworkEightProcessor& processor, ModAssignmentController& assignmentController)
        : processor_(processor),
          assignmentController_(assignmentController),
          filterWireframe_(processor.apvts),
          lfoWireframe_(processor.apvts, 0),
          modSourcePalette_(assignmentController),
          oscilloscope_(processor)
    {
        addAndMakeVisible(filterPanel_);
        addAndMakeVisible(lfoPanel_);
        addAndMakeVisible(modSourcePalette_);
        modSourcePalette_.setCompactLayout(true);

        filterEnabledButton_ = std::make_unique<GlowRingButton>("Filter Enable");
        filterEnabledLabel_.setText("FILTER", juce::dontSendNotification);
        filterEnabledLabel_.setFont(fonts::label(9.5f));
        filterEnabledLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        filterEnabledLabel_.setJustificationType(juce::Justification::centredLeft);

        lfoWaveform_ = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(0, "Waveform"), "Wave", lfoWaveformToText);
        lfoMode_ = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(0, "Mode"), "Mode", lfoModeToText);
        lfoRate_ = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(0, "RateHz"), "Rate");
        for (auto* k : {lfoWaveform_.get(), lfoMode_.get(), lfoRate_.get()})
            lfoPanel_.addAndMakeVisible(*k);
        lfoPanel_.addAndMakeVisible(lfoWireframe_);

        filter2EnabledButton_ = std::make_unique<GlowRingButton>("Filter 2 Enable");
        filter2EnabledLabel_.setText("FILTER 2", juce::dontSendNotification);
        filter2EnabledLabel_.setFont(fonts::label(9.5f));
        filter2EnabledLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        filter2EnabledLabel_.setJustificationType(juce::Justification::centredLeft);
        filter2Panel_.addAndMakeVisible(*filter2EnabledButton_);
        filter2Panel_.addAndMakeVisible(filter2EnabledLabel_);

        addAndMakeVisible(filter2Panel_);
        addAndMakeVisible(oscilloscope_);

        setScope(FilterPanelScope::Global, 0);
    }

    void FilterLfoPanel::setScope(FilterPanelScope scope, int engineIndex)
    {
        scope_ = scope;
        engineIndex_ = juce::jlimit(0, 7, engineIndex);
        rebuildAttachments();
        lfoPanel_.setVisible(scope_ == FilterPanelScope::Global);
        filter2Panel_.setVisible(scope_ == FilterPanelScope::Global);
        filterPanel_.setTitle(scope_ == FilterPanelScope::Global
                                  ? "Global Filter"
                                  : "Engine " + juce::String(engineIndex_) + " Filter");
        resized();
        repaint();
    }

    void FilterLfoPanel::repaintModAssignmentState()
    {
        modSourcePalette_.repaintAssignmentState();
    }

    void FilterLfoPanel::rebuildAttachments()
    {
        filterEnabledAttachment_.reset();
        filterMode_.reset();
        filterToneKnob_.reset();
        filterKeyTrack_.reset();

        auto& apvts = processor_.apvts;
        juce::String enabledId;
        juce::String modeId;
        juce::String cutoffId;
        juce::String resonanceId;
        juce::String keyTrackId;
        modulation::ModDestination cutoffDest = modulation::ModDestination::None;
        modulation::ModDestination resonanceDest = modulation::ModDestination::None;
        std::uint8_t modTarget = 0;

        if (scope_ == FilterPanelScope::Global)
        {
            enabledId = juce::String(kFilterIdPrefix) + "Enabled";
            modeId = juce::String(kFilterIdPrefix) + "Mode";
            cutoffId = juce::String(kFilterIdPrefix) + "CutoffHz";
            resonanceId = juce::String(kFilterIdPrefix) + "Resonance";
            keyTrackId = juce::String(kFilterIdPrefix) + "KeyTrack";
            cutoffDest = modulation::ModDestination::FilterCutoff;
            resonanceDest = modulation::ModDestination::FilterResonance;
        }
        else
        {
            enabledId = operatorFilterParamId(static_cast<std::size_t>(engineIndex_), "FilterEnabled");
            modeId = operatorFilterParamId(static_cast<std::size_t>(engineIndex_), "FilterMode");
            cutoffId = operatorFilterParamId(static_cast<std::size_t>(engineIndex_), "FilterCutoffHz");
            resonanceId = operatorFilterParamId(static_cast<std::size_t>(engineIndex_), "FilterResonance");
            keyTrackId = operatorFilterParamId(static_cast<std::size_t>(engineIndex_), "FilterKeyTrack");
            cutoffDest = modulation::ModDestination::OperatorFilterCutoff;
            resonanceDest = modulation::ModDestination::OperatorFilterResonance;
            modTarget = static_cast<std::uint8_t>(engineIndex_);
        }

        filterWireframe_.setParamIds(enabledId, modeId, cutoffId, resonanceId);

        filterEnabledAttachment_ =
            std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, enabledId, *filterEnabledButton_);

        filterMode_ = std::make_unique<GlowKnob>(apvts, modeId, "Mode", filterModeToText);
        filterToneKnob_ = std::make_unique<ConcentricGlowKnob>(apvts, cutoffId, resonanceId, "Cutoff", "Reso");
        filterKeyTrack_ = std::make_unique<GlowKnob>(apvts, keyTrackId, "Key Trk");
        filterToneKnob_->enableInnerModulationTarget(processor_, cutoffDest, modTarget);
        filterToneKnob_->enableOuterModulationTarget(processor_, resonanceDest, modTarget);
        filterToneKnob_->setModAssignmentController(&assignmentController_);

        filterPanel_.removeAllChildren();
        filterPanel_.addAndMakeVisible(filterWireframe_);
        filterPanel_.addAndMakeVisible(*filterEnabledButton_);
        filterPanel_.addAndMakeVisible(filterEnabledLabel_);
        filterPanel_.addAndMakeVisible(*filterMode_);
        filterPanel_.addAndMakeVisible(*filterToneKnob_);
        filterPanel_.addAndMakeVisible(*filterKeyTrack_);

        filter2EnabledAttachment_.reset();
        filter2Cutoff_.reset();
        filter2Resonance_.reset();
        filter2Drive_.reset();
        filter2KeyTrack_.reset();
        filter2Panel_.removeAllChildren();
        filter2Panel_.addAndMakeVisible(*filter2EnabledButton_);
        filter2Panel_.addAndMakeVisible(filter2EnabledLabel_);

        if (scope_ == FilterPanelScope::Global)
        {
            const juce::String prefix = juce::String(kFilter2IdPrefix);
            filter2EnabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, prefix + "Enabled", *filter2EnabledButton_);
            filter2Cutoff_ = std::make_unique<GlowKnob>(apvts, prefix + "CutoffHz", "Cutoff");
            filter2Resonance_ = std::make_unique<GlowKnob>(apvts, prefix + "Resonance", "Reso");
            filter2Drive_ = std::make_unique<GlowKnob>(apvts, prefix + "Drive", "Drive");
            filter2KeyTrack_ = std::make_unique<GlowKnob>(apvts, prefix + "KeyTrack", "Key Trk");
            for (auto* k : {filter2Cutoff_.get(), filter2Resonance_.get(), filter2Drive_.get(), filter2KeyTrack_.get()})
                filter2Panel_.addAndMakeVisible(*k);
        }
    }

    void FilterLfoPanel::paint(juce::Graphics& g)
    {
        if (scope_ != FilterPanelScope::Global)
            return;

        g.setColour(branding::glowColour().withAlpha(0.06f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    }

    void FilterLfoPanel::resized()
    {
        auto bounds = getLocalBounds();

        auto scopeRow = bounds.removeFromBottom(56);
        oscilloscope_.setBounds(scopeRow.reduced(4, 2));

        auto paletteRow = bounds.removeFromTop(30);
        modSourcePalette_.setBounds(paletteRow.reduced(4, 2));
        bounds.removeFromTop(4);

        if (scope_ == FilterPanelScope::Global)
        {
            auto filter2Row = bounds.removeFromBottom(88);
            filter2Panel_.setBounds(filter2Row.reduced(4, 0));
            {
                auto content = filter2Panel_.getContentBounds().reduced(4, 0);
                auto enableRow = content.removeFromTop(32);
                const int ringSize = 28;
                filter2EnabledButton_->setBounds(enableRow.removeFromLeft(ringSize + 6).withSizeKeepingCentre(ringSize, ringSize));
                filter2EnabledLabel_.setBounds(enableRow.removeFromLeft(72));
                const int knobWidth = content.getWidth() / 4;
                if (filter2Cutoff_)
                    filter2Cutoff_->setBounds(content.removeFromLeft(knobWidth).reduced(4));
                if (filter2Resonance_)
                    filter2Resonance_->setBounds(content.removeFromLeft(knobWidth).reduced(4));
                if (filter2Drive_)
                    filter2Drive_->setBounds(content.removeFromLeft(knobWidth).reduced(4));
                if (filter2KeyTrack_)
                    filter2KeyTrack_->setBounds(content.removeFromLeft(knobWidth).reduced(4));
            }
            bounds.removeFromBottom(4);

            const int half = bounds.getWidth() / 2;
            filterPanel_.setBounds(bounds.removeFromLeft(half).reduced(4, 0));
            lfoPanel_.setBounds(bounds.reduced(4, 0));
        }
        else
        {
            filterPanel_.setBounds(bounds.reduced(4, 0));
        }

        {
            auto content = filterPanel_.getContentBounds();
            auto wireBounds = content.removeFromLeft(static_cast<int>(content.getWidth() * 0.42f)).reduced(0, 2);
            filterWireframe_.setBounds(wireBounds);

            content = content.reduced(4, 0);
            auto enableRow = content.removeFromTop(36);
            const int ringSize = 30;
            filterEnabledButton_->setBounds(enableRow.removeFromLeft(ringSize + 6).withSizeKeepingCentre(ringSize, ringSize));
            filterEnabledLabel_.setBounds(enableRow.removeFromLeft(70));

            const int knobWidth = content.getWidth() / 3;
            filterMode_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            filterToneKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            filterKeyTrack_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
        if (scope_ == FilterPanelScope::Global)
        {
            auto content = lfoPanel_.getContentBounds();
            auto wireBounds = content.removeFromLeft(static_cast<int>(content.getWidth() * 0.42f)).reduced(0, 2);
            lfoWireframe_.setBounds(wireBounds);

            content = content.reduced(4, 0);
            const int knobWidth = content.getWidth() / 3;
            lfoWaveform_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            lfoMode_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            lfoRate_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
    }

} // namespace pw8::plugin::ui
