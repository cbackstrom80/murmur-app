#include "FilterLfoPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/BrandingAssets.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "LabLauncherChip.h"
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

        juce::String filterModeChipText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "LP4";
                case 1: return "HP4";
                case 2: return "BP4";
                case 3: return "NOTCH";
                case 4: return "PEAK";
                default: return filterModeToText(v);
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

    FilterLfoPanel::FilterLfoPanel(MurmurProcessor& processor, ModAssignmentController& assignmentController)
        : processor_(processor),
          assignmentController_(assignmentController),
          filterWireframe_(processor.apvts),
          lfoWireframe_(processor.apvts, 0),
          modSourcePalette_(assignmentController),
          filterScope_(processor),
          filterRoutingWireframe_(processor.apvts),
          subAnchorMeter_(processor)
    {
        addAndMakeVisible(filterPanel_);
        addAndMakeVisible(lfoPanel_);
        addAndMakeVisible(modSourcePalette_);
        modSourcePalette_.setCompactLayout(true);

        filterWireframe_.attachVisualizerBus(processor.getVisualizerBus());
        lfoWireframe_.attachVisualizerBus(processor.getVisualizerBus());

        filterEnabledButton_ = std::make_unique<GlowRingButton>("Filter Enable");
        filterEnabledLabel_.setText("FILTER", juce::dontSendNotification);
        filterEnabledLabel_.setFont(fonts::label(9.5f));
        filterEnabledLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        filterEnabledLabel_.setJustificationType(juce::Justification::centredLeft);

        lfoWaveform_ = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(0, "Waveform"), "Wave", lfoWaveformToText);
        lfoMode_ = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(0, "Mode"), "Mode", lfoModeToText);
        lfoMotionKnob_ = std::make_unique<ConcentricGlowKnob>(
            processor.apvts, lfoParamId(0, "PhaseOffset"), lfoParamId(0, "RateHz"), "Phase", "Rate");
        lfoMotionKnob_->setInnerAccentColour(palette::kAccentWarm);
        lfoPanel_.addAndMakeVisible(*lfoWaveform_);
        lfoPanel_.addAndMakeVisible(*lfoMode_);
        lfoPanel_.addAndMakeVisible(*lfoMotionKnob_);
        lfoPanel_.addAndMakeVisible(lfoWireframe_);

        filter2EnabledButton_ = std::make_unique<GlowRingButton>("Filter 2 Enable");
        filter2EnabledLabel_.setText("BLADES DUAL FILTER", juce::dontSendNotification);
        filter2EnabledLabel_.setFont(fonts::label(9.5f));
        filter2EnabledLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        filter2EnabledLabel_.setJustificationType(juce::Justification::centredLeft);
        filter2Panel_.addAndMakeVisible(*filter2EnabledButton_);
        filter2Panel_.addAndMakeVisible(filter2EnabledLabel_);

        addAndMakeVisible(filter2Panel_);
        addAndMakeVisible(scopeFrame_);
        scopeFrame_.addAndMakeVisible(filterScope_);

        subAnchorEnabledButton_ = std::make_unique<GlowRingButton>("Sub Anchor Enable");
        subAnchorEnabledButton_->setAccentColour(juce::Colour(0xffD4603A));
        subAnchorEnabledLabel_.setText("SUB ANCHOR", juce::dontSendNotification);
        subAnchorEnabledLabel_.setFont(fonts::label(9.5f));
        subAnchorEnabledLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        subAnchorEnabledLabel_.setJustificationType(juce::Justification::centredLeft);
        subAnchorPanel_.addAndMakeVisible(*subAnchorEnabledButton_);
        subAnchorPanel_.addAndMakeVisible(subAnchorEnabledLabel_);
        subAnchorPanel_.addAndMakeVisible(subAnchorMeter_);
        addAndMakeVisible(subAnchorPanel_);

        lfoLabChip_ = std::make_unique<LabLauncherChip>();
        lfoLabChip_->setLabel("LFO 1·2 →");
        lfoLabChip_->setAccentColour(juce::Colour(0xff9f80ff));
        lfoLabChip_->setIcon(LabLauncherIcon::Lfo);
        lfoLabChip_->onClick = [this] {
            if (onLfoLabRequested)
                onLfoLabRequested();
        };
        lfoLabChip_->setVisible(false);
        addAndMakeVisible(*lfoLabChip_);

        filterModeChipLabel_.setFont(fonts::label(8.0f));
        filterModeChipLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        filterModeChipLabel_.setJustificationType(juce::Justification::centredLeft);
        filterModeChipLabel_.setVisible(false);
        addAndMakeVisible(filterModeChipLabel_);

        filterSlopeChipLabel_.setFont(fonts::label(7.0f));
        filterSlopeChipLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        filterSlopeChipLabel_.setJustificationType(juce::Justification::centredRight);
        filterSlopeChipLabel_.setVisible(false);
        addAndMakeVisible(filterSlopeChipLabel_);

        setScope(FilterPanelScope::Global, 0);
    }

    void FilterLfoPanel::setDashboardMode(bool dashboardMode)
    {
        dashboardMode_ = dashboardMode;
        lfoLabChip_->setVisible(dashboardMode_ && scope_ == FilterPanelScope::Global);
        filterModeChipLabel_.setVisible(dashboardMode_ && scope_ == FilterPanelScope::Global);
        filterSlopeChipLabel_.setVisible(dashboardMode_ && scope_ == FilterPanelScope::Global);
        lfoPanel_.setVisible(!dashboardMode_ && scope_ == FilterPanelScope::Global);
        filter2Panel_.setVisible(!dashboardMode_ && scope_ == FilterPanelScope::Global);
        subAnchorPanel_.setVisible(!dashboardMode_ && scope_ == FilterPanelScope::Global);
        modSourcePalette_.setVisible(!dashboardMode_);
        scopeFrame_.setVisible(!dashboardMode_);
        filterWireframe_.setVisible(!dashboardMode_);
        resized();
    }

    void FilterLfoPanel::setScope(FilterPanelScope scope, int engineIndex)
    {
        scope_ = scope;
        engineIndex_ = juce::jlimit(0, 7, engineIndex);
        rebuildAttachments();
        lfoPanel_.setVisible(!dashboardMode_ && scope_ == FilterPanelScope::Global);
        filter2Panel_.setVisible(!dashboardMode_ && scope_ == FilterPanelScope::Global);
        subAnchorPanel_.setVisible(!dashboardMode_ && scope_ == FilterPanelScope::Global);
        filterPanel_.setTitle(scope_ == FilterPanelScope::Global
                                  ? (dashboardMode_ ? "Global Filter" : "Global Filter")
                                  : "Engine " + juce::String(engineIndex_) + " Filter");
        lfoLabChip_->setVisible(dashboardMode_ && scope_ == FilterPanelScope::Global);
        filterModeChipLabel_.setVisible(dashboardMode_ && scope_ == FilterPanelScope::Global);
        filterSlopeChipLabel_.setVisible(dashboardMode_ && scope_ == FilterPanelScope::Global);
        filterWireframe_.setVisible(!dashboardMode_);
        modSourcePalette_.setVisible(!dashboardMode_);
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
        filterToneKnob_ = std::make_unique<ConcentricGlowKnob>(apvts, resonanceId, cutoffId, "Reso", "Cutoff");
        filterKeyTrack_ = std::make_unique<GlowKnob>(apvts, keyTrackId, "Key Trk");
        filterToneKnob_->setInnerAccentColour(palette::kAccent);
        filterToneKnob_->enableInnerModulationTarget(processor_, resonanceDest, modTarget);
        filterToneKnob_->enableOuterModulationTarget(processor_, cutoffDest, modTarget);
        filterToneKnob_->setModAssignmentController(&assignmentController_);

        filterPanel_.removeAllChildren();
        filterPanel_.addAndMakeVisible(filterWireframe_);
        filterPanel_.addAndMakeVisible(*filterEnabledButton_);
        filterPanel_.addAndMakeVisible(filterEnabledLabel_);
        filterPanel_.addAndMakeVisible(*filterMode_);
        filterPanel_.addAndMakeVisible(*filterToneKnob_);
        filterPanel_.addAndMakeVisible(*filterKeyTrack_);

        filter2EnabledAttachment_.reset();
        filterRouting_.reset();
        filterModeMorph_.reset();
        filter2Cutoff_.reset();
        filter2Resonance_.reset();
        filter2Drive_.reset();
        filter2CutoffOffset_.reset();
        filter2KeyTrack_.reset();
        filter2ModeMorph_.reset();
        filter2Panel_.removeAllChildren();
        filter2Panel_.addAndMakeVisible(*filter2EnabledButton_);
        filter2Panel_.addAndMakeVisible(filter2EnabledLabel_);
        filter2Panel_.addAndMakeVisible(filterRoutingWireframe_);

        subAnchorEnabledAttachment_.reset();
        subAnchorCrossover_.reset();
        subAnchorMonoAmount_.reset();
        subAnchorPanel_.removeAllChildren();
        subAnchorPanel_.addAndMakeVisible(*subAnchorEnabledButton_);
        subAnchorPanel_.addAndMakeVisible(subAnchorEnabledLabel_);
        subAnchorPanel_.addAndMakeVisible(subAnchorMeter_);

        if (scope_ == FilterPanelScope::Global)
        {
            const juce::String f2Prefix = juce::String(kFilter2IdPrefix);
            filter2EnabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, f2Prefix + "Enabled", *filter2EnabledButton_);
            filterRouting_ = std::make_unique<GlowKnob>(apvts, kFilterRoutingId, "Route");
            filterModeMorph_ =
                std::make_unique<GlowKnob>(apvts, juce::String(kFilterIdPrefix) + "ModeMorph", "Morph");
            filter2Cutoff_ = std::make_unique<GlowKnob>(apvts, f2Prefix + "CutoffHz", "Cutoff");
            filter2Resonance_ = std::make_unique<GlowKnob>(apvts, f2Prefix + "Resonance", "Reso");
            filter2Drive_ = std::make_unique<GlowKnob>(apvts, f2Prefix + "Drive", "Drive");
            filter2CutoffOffset_ =
                std::make_unique<GlowKnob>(apvts, f2Prefix + "CutoffOffsetSemis", "F2 Off");
            filter2KeyTrack_ = std::make_unique<GlowKnob>(apvts, f2Prefix + "KeyTrack", "Key Trk");
            // F2's own LP->BP->HP morph (see CharacterFilter.hpp) -- gives Filter 2 real
            // response variety instead of the LP-only output it originally shipped with.
            // Deliberately not wired as a mod-matrix destination (unlike F1's morph) --
            // a static per-patch knob is enough to close the response-variety gap; live
            // modulation of it is a separate, optional follow-up.
            filter2ModeMorph_ = std::make_unique<GlowKnob>(apvts, f2Prefix + "ModeMorph", "F2 Morph");

            filterRouting_->enableModulationTarget(processor_, modulation::ModDestination::FilterRouting, 0);
            filterRouting_->setModAssignmentController(&assignmentController_);
            filterModeMorph_->enableModulationTarget(processor_, modulation::ModDestination::FilterModeMorph, 0);
            filterModeMorph_->setModAssignmentController(&assignmentController_);
            filter2Drive_->enableModulationTarget(processor_, modulation::ModDestination::FilterDrive, 0);
            filter2Drive_->setModAssignmentController(&assignmentController_);

            filterRouting_->applyFigmaContext(figma::KnobContext::PlayBlades);
            filterModeMorph_->applyFigmaContext(figma::KnobContext::PlayBlades);
            filter2Cutoff_->applyFigmaContext(figma::KnobContext::PlayBlades);
            filter2Resonance_->applyFigmaContext(figma::KnobContext::PlayBlades);
            filter2Drive_->applyFigmaContext(figma::KnobContext::PlayBlades);
            filter2Drive_->setDeckedStyle(true, GlowKnob::DeckedKnobSize::Medium);
            filter2CutoffOffset_->applyFigmaContext(figma::KnobContext::PlayBlades);
            filter2KeyTrack_->applyFigmaContext(figma::KnobContext::PlayBlades);
            filter2ModeMorph_->applyFigmaContext(figma::KnobContext::PlayBlades);

            for (auto* k : {filterRouting_.get(), filterModeMorph_.get(), filter2Cutoff_.get(), filter2Resonance_.get(),
                            filter2Drive_.get(), filter2CutoffOffset_.get(), filter2KeyTrack_.get(),
                            filter2ModeMorph_.get()})
                filter2Panel_.addAndMakeVisible(*k);

            // Sub Anchor -- Layer A only (see pw8/spatial/SubAnchor.hpp's own
            // doc comment on why it's scoped to Layer A), so like Filter 2
            // this only exists in Global scope, not per-operator.
            const juce::Colour copper(0xffD4603A);
            subAnchorEnabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, juce::String(kSubAnchorIdPrefix) + "Enabled", *subAnchorEnabledButton_);
            subAnchorCrossover_ = std::make_unique<GlowKnob>(
                apvts, juce::String(kSubAnchorIdPrefix) + "CrossoverHz", "Crossover", nullptr, copper);
            subAnchorMonoAmount_ = std::make_unique<GlowKnob>(
                apvts, juce::String(kSubAnchorIdPrefix) + "MonoAmount", "Mono Amt", nullptr, copper);
            subAnchorCrossover_->applyFigmaContext(figma::KnobContext::PlayBlades);
            subAnchorMonoAmount_->applyFigmaContext(figma::KnobContext::PlayBlades);
            // Deliberately no enableModulationTarget() call -- no mod-matrix
            // destination for Sub Anchor yet, same real scope decision as
            // Filter 2's own modeMorph (see that knob's own comment above).
            subAnchorPanel_.addAndMakeVisible(*subAnchorCrossover_);
            subAnchorPanel_.addAndMakeVisible(*subAnchorMonoAmount_);
        }
    }

    void FilterLfoPanel::paint(juce::Graphics& g)
    {
        if (scope_ != FilterPanelScope::Global)
            return;

        g.setColour(branding::glowColour().withAlpha(0.06f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

        if (dashboardMode_ && !modeChipBounds_.isEmpty())
        {
            for (const auto& chip : {modeChipBounds_, slopeChipBounds_})
            {
                if (chip.isEmpty())
                    continue;
                g.setColour(palette::kBackgroundTop);
                g.fillRoundedRectangle(chip.toFloat(), 4.0f);
                g.setColour(palette::kBorder);
                g.drawRoundedRectangle(chip.toFloat().reduced(0.5f), 4.0f, 0.9f);
            }
        }
    }

    void FilterLfoPanel::resized()
    {
        auto bounds = getLocalBounds();

        if (!dashboardMode_)
        {
            auto scopeRow = bounds.removeFromBottom(64);
            scopeFrame_.setBounds(scopeRow.reduced(4, 2));
            filterScope_.setBounds(scopeFrame_.getContentBounds());

            auto paletteRow = bounds.removeFromTop(30);
            modSourcePalette_.setBounds(paletteRow.reduced(4, 2));
            bounds.removeFromTop(4);
        }
        else
        {
            scopeFrame_.setBounds({});
            modSourcePalette_.setBounds({});
        }

        if (scope_ == FilterPanelScope::Global)
        {
            if (!dashboardMode_)
            {
                auto filter2Row = bounds.removeFromBottom(layout::kPlayBladesSectionHeight);
                filter2Panel_.setBounds(filter2Row.reduced(4, 0));
                {
                    auto content = filter2Panel_.getContentBounds().reduced(layout::kPlayBladesSectionPadding / 2, 0);
                    auto headerRow = content.removeFromTop(layout::kPlayBladesHeaderRowHeight);
                    const int ringSize = 22;
                    filter2EnabledButton_->setBounds(
                        headerRow.removeFromRight(ringSize + 4).withSizeKeepingCentre(ringSize, ringSize));
                    filter2EnabledLabel_.setBounds(headerRow);
                    content.removeFromTop(layout::kPlayBladesSectionGap);

                    filterRoutingWireframe_.setVisible(false);
                    const int knobSlotW = content.getWidth() / layout::kPlayBladesKnobCount;
                    if (filterRouting_)
                        filterRouting_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (filterModeMorph_)
                        filterModeMorph_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (filter2Cutoff_)
                        filter2Cutoff_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (filter2Resonance_)
                        filter2Resonance_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (filter2Drive_)
                        filter2Drive_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (filter2CutoffOffset_)
                        filter2CutoffOffset_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (filter2KeyTrack_)
                        filter2KeyTrack_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (filter2ModeMorph_)
                        filter2ModeMorph_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                }
                bounds.removeFromBottom(4);

                // Sub Anchor -- compact row (button + 2 knobs + meter), real
                // scope Filter 2 doesn't need since it's only 3 controls.
                constexpr int kSubAnchorRowHeight = 72;
                auto subAnchorRow = bounds.removeFromBottom(kSubAnchorRowHeight);
                subAnchorPanel_.setBounds(subAnchorRow.reduced(4, 0));
                {
                    auto content =
                        subAnchorPanel_.getContentBounds().reduced(layout::kPlayBladesSectionPadding / 2, 0);
                    auto headerRow = content.removeFromTop(layout::kPlayBladesHeaderRowHeight);
                    const int ringSize = 22;
                    subAnchorEnabledButton_->setBounds(
                        headerRow.removeFromRight(ringSize + 4).withSizeKeepingCentre(ringSize, ringSize));
                    subAnchorEnabledLabel_.setBounds(headerRow);
                    content.removeFromTop(layout::kPlayBladesSectionGap);

                    const int knobSlotW = content.getWidth() / 4;
                    if (subAnchorCrossover_)
                        subAnchorCrossover_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    if (subAnchorMonoAmount_)
                        subAnchorMonoAmount_->setBounds(content.removeFromLeft(knobSlotW).reduced(2));
                    subAnchorMeter_.setBounds(content.reduced(2));
                }
                bounds.removeFromBottom(4);
            }

            if (dashboardMode_)
            {
                auto chipCol = bounds.removeFromRight(juce::jmax(110, bounds.getWidth() / 3)).reduced(4, 0);
                lfoLabChip_->setBounds(chipCol.removeFromTop(layout::kLabChipHeight).reduced(0, 2));
                chipCol.removeFromTop(6);
                modeChipBounds_ = chipCol.removeFromTop(26).reduced(0, 2);
                slopeChipBounds_ = chipCol.removeFromTop(26).reduced(0, 2);
                filterModeChipLabel_.setBounds(modeChipBounds_.reduced(8, 0));
                filterSlopeChipLabel_.setBounds(slopeChipBounds_.reduced(8, 0));
                if (auto* modeRaw = processor_.apvts.getRawParameterValue(juce::String(kFilterIdPrefix) + "Mode"))
                {
                    filterModeChipLabel_.setText("MODE: " + filterModeChipText(modeRaw->load()), juce::dontSendNotification);
                    filterSlopeChipLabel_.setText("SLOPE: 24dB", juce::dontSendNotification);
                }
                filterPanel_.setBounds(bounds.reduced(4, 0));
            }
            else
            {
                const int half = bounds.getWidth() / 2;
                filterPanel_.setBounds(bounds.removeFromLeft(half).reduced(4, 0));
                lfoPanel_.setBounds(bounds.reduced(4, 0));
            }
        }
        else
        {
            filterPanel_.setBounds(bounds.reduced(4, 0));
        }

        {
            auto content = filterPanel_.getContentBounds();
            if (dashboardMode_ && scope_ == FilterPanelScope::Global)
            {
                content = content.reduced(4, 0);
                const int knobWidth = content.getWidth() / 3;
                filterToneKnob_->applyFigmaContext(figma::KnobContext::DashboardFilter);
                filterKeyTrack_->applyFigmaContext(figma::KnobContext::DashboardFilter);
                filterMode_->applyFigmaContext(figma::KnobContext::DashboardFilter);
                filterToneKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(4));
                filterKeyTrack_->setBounds(content.removeFromLeft(knobWidth).reduced(4));
                filterMode_->setBounds(content.removeFromLeft(knobWidth).reduced(4));
                filterEnabledButton_->setBounds({});
                filterEnabledLabel_.setBounds({});
                filterWireframe_.setBounds({});
            }
            else
            {
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
        }
        if (scope_ == FilterPanelScope::Global && !dashboardMode_)
        {
            auto content = lfoPanel_.getContentBounds();
            auto wireBounds = content.removeFromLeft(static_cast<int>(content.getWidth() * 0.42f)).reduced(0, 2);
            lfoWireframe_.setBounds(wireBounds);

            content = content.reduced(4, 0);
            const int knobWidth = content.getWidth() / 3;
            lfoWaveform_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            lfoMode_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
            lfoMotionKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(5));
        }
    }

} // namespace pw8::plugin::ui
