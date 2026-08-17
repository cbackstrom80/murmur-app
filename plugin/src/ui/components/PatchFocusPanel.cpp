#include "PatchFocusPanel.h"

#include <cmath>

#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    PatchFocusPanel::PatchFocusPanel(PatchworkEightProcessor& processor) : processor_(processor)
    {
        addAndMakeVisible(panel_);
        panel_.setInterceptsMouseClicks(false, true);

        introLabel_.setText("Feature macros modulate this patch — standard knobs below always work the same way.",
                            juce::dontSendNotification);
        introLabel_.setJustificationType(juce::Justification::centredLeft);
        introLabel_.setFont(fonts::value(fonts::kBodyLabelSize));
        introLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        panel_.addAndMakeVisible(introLabel_);

        ringLegendLabel_.setText(
            juce::String(juce::CharPointer_UTF8("\xe2\x97\x8f")) + " value arc   "
                + juce::String(juce::CharPointer_UTF8("\xe2\x97\x8b")) + " mod route   "
                + juce::String(juce::CharPointer_UTF8("\xc2\xb7")) + " live mod",
            juce::dontSendNotification);
        ringLegendLabel_.setJustificationType(juce::Justification::centredLeft);
        ringLegendLabel_.setFont(fonts::value(fonts::kCaptionSize));
        ringLegendLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addAndMakeVisible(ringLegendLabel_);

        panel_.addAndMakeVisible(subtitleLabel_);
        subtitleLabel_.setJustificationType(juce::Justification::centredLeft);
        subtitleLabel_.setFont(fonts::value(fonts::kBodyLabelSize));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);

        panel_.addAndMakeVisible(macroHintsLabel_);
        macroHintsLabel_.setJustificationType(juce::Justification::centredLeft);
        macroHintsLabel_.setFont(fonts::value(fonts::kCaptionSize));
        macroHintsLabel_.setColour(juce::Label::textColourId, palette::kTextDim);

        standardSectionLabel_.setText("Standard controls", juce::dontSendNotification);

        modWheelBadge_.setJustificationType(juce::Justification::centredLeft);
        modWheelBadge_.setFont(fonts::label(11.0f));
        modWheelBadge_.setColour(juce::Label::textColourId, palette::kModModWheel);
        modWheelBadge_.setColour(juce::Label::backgroundColourId, palette::kModModWheel.withAlpha(0.12f));
        panel_.addAndMakeVisible(modWheelBadge_);

        expressionBadge_.setJustificationType(juce::Justification::centredLeft);
        expressionBadge_.setFont(fonts::label(11.0f));
        expressionBadge_.setColour(juce::Label::textColourId, palette::kModExpression);
        expressionBadge_.setColour(juce::Label::backgroundColourId, palette::kModExpression.withAlpha(0.12f));
        panel_.addAndMakeVisible(expressionBadge_);

        sidechainBadge_.setJustificationType(juce::Justification::centredLeft);
        sidechainBadge_.setFont(fonts::label(11.0f));
        sidechainBadge_.setColour(juce::Label::textColourId, palette::kAccent);
        sidechainBadge_.setColour(juce::Label::backgroundColourId, palette::kAccent.withAlpha(0.12f));
        panel_.addAndMakeVisible(sidechainBadge_);

        advancedButton_.setButtonText("Mod Matrix (M)");
        advancedButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        advancedButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        advancedButton_.onClick = [this] {
            if (onAdvancedClicked)
                onAdvancedClicked();
        };
        panel_.addAndMakeVisible(advancedButton_);
        advancedButton_.setVisible(false);

        desktopDeckTitleLabel_.setText("OBSIDIAN ACTIVE PERFORMANCE CONTROL DECK", juce::dontSendNotification);
        desktopDeckTitleLabel_.setJustificationType(juce::Justification::centredLeft);
        desktopDeckTitleLabel_.setFont(fonts::label(9.0f));
        desktopDeckTitleLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        desktopDeckTitleLabel_.setVisible(false);
        panel_.addChildComponent(desktopDeckTitleLabel_);

        desktopDeckStatusLabel_.setText("ASSIGNED MACRO MAPPINGS OK", juce::dontSendNotification);
        desktopDeckStatusLabel_.setJustificationType(juce::Justification::centredRight);
        desktopDeckStatusLabel_.setFont(fonts::label(8.0f));
        desktopDeckStatusLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        desktopDeckStatusLabel_.setVisible(false);
        panel_.addChildComponent(desktopDeckStatusLabel_);

        setBasicPerformanceLayout(true);
        startTimerHz(12);
        timerCallback();
    }

    PatchFocusPanel::~PatchFocusPanel()
    {
        stopTimer();
    }

    void PatchFocusPanel::setBasicPerformanceLayout(bool basicLayout)
    {
        basicLayout_ = basicLayout;
        if (basicLayout)
            compactLayout_ = false;
        introLabel_.setVisible(basicLayout && !compactLayout_ && !desktopPlayModeLayout_);
        ringLegendLabel_.setVisible(basicLayout && !compactLayout_ && !desktopPlayModeLayout_);
        if (!compactLayout_)
        {
            panel_.setVisible(true);
            if (panel_.getParentComponent() == nullptr)
                addAndMakeVisible(panel_);
        }
        applyLayoutMode();
        resized();
    }

    void PatchFocusPanel::setCompactLayout(bool compactLayout)
    {
        compactLayout_ = compactLayout;
        if (compactLayout)
        {
            basicLayout_ = false;
            desktopPlayModeLayout_ = false;
        }
        introLabel_.setVisible(basicLayout_ && !compactLayout_ && !desktopPlayModeLayout_);
        ringLegendLabel_.setVisible(basicLayout_ && !compactLayout_ && !desktopPlayModeLayout_);
        panel_.setVisible(!compactLayout_);
        if (!compactLayout_)
            orbitHole_ = {};
        applyLayoutMode();
        resized();
    }

    void PatchFocusPanel::setDesktopPlayModeLayout(bool desktopPlayMode)
    {
        desktopPlayModeLayout_ = desktopPlayMode;
        if (desktopPlayMode)
        {
            compactLayout_ = false;
            basicLayout_ = true;
        }
        introLabel_.setVisible(basicLayout_ && !compactLayout_ && !desktopPlayModeLayout_);
        ringLegendLabel_.setVisible(basicLayout_ && !compactLayout_ && !desktopPlayModeLayout_);
        desktopDeckTitleLabel_.setVisible(desktopPlayModeLayout_);
        desktopDeckStatusLabel_.setVisible(desktopPlayModeLayout_);
        modWheelBadge_.setVisible(!desktopPlayModeLayout_ && modWheelBadge_.isVisible());
        expressionBadge_.setVisible(!desktopPlayModeLayout_ && expressionBadge_.isVisible());
        sidechainBadge_.setVisible(!desktopPlayModeLayout_ && sidechainBadge_.isVisible());
        macroHintsLabel_.setVisible(!desktopPlayModeLayout_ && macroHintsLabel_.isVisible());
        standardSectionLabel_.setVisible(!desktopPlayModeLayout_ && standardSectionLabel_.isVisible());
        panel_.setVisible(true);
        if (panel_.getParentComponent() == nullptr)
            addAndMakeVisible(panel_);
        applyLayoutMode();
        refreshFromPatch();
        resized();
    }

    void PatchFocusPanel::setIpadPlayLayout(bool ipadPlayLayout)
    {
        ipadPlayLayout_ = ipadPlayLayout;
        if (ipadPlayLayout)
            desktopPlayModeLayout_ = true;
        applyLayoutMode();
        refreshFromPatch();
        resized();
    }

    void PatchFocusPanel::setOrbitHole(juce::Rectangle<int> centerHole)
    {
        orbitHole_ = centerHole;
        resized();
    }

    void PatchFocusPanel::applyLayoutMode()
    {
        const int featureDialCap = desktopPlayModeLayout_
                                       ? layout::kDesktopPlayModeMacroKnobTouchSize
                                       : (compactLayout_ ? 56 : (basicLayout_ ? 120 : 72));
        const int standardDialCap = compactLayout_ ? 52 : (basicLayout_ ? 96 : 68);
        const auto featureDeckedSize = desktopPlayModeLayout_
                                           ? GlowKnob::DeckedKnobSize::Large
                                           : (compactLayout_ ? GlowKnob::DeckedKnobSize::Small
                                                             : (basicLayout_ ? GlowKnob::DeckedKnobSize::Large
                                                                             : GlowKnob::DeckedKnobSize::Medium));
        const auto standardDeckedSize = compactLayout_ ? GlowKnob::DeckedKnobSize::Small
                                                       : GlowKnob::DeckedKnobSize::Medium;

        for (std::size_t i = 0; i < knobs_.size(); ++i)
        {
            const bool featured = i < featureKnobCount_;
            knobs_[i]->setMaxDialDiameter(featured ? featureDialCap : standardDialCap);
            knobs_[i]->setDeckedStyle(true, featured ? featureDeckedSize : standardDeckedSize);
            knobs_[i]->setFeaturedPerformanceMacro(featured);
        }
    }

    void PatchFocusPanel::refreshFromPatch()
    {
        lastLayout_ = {};
        timerCallback();
    }

    void PatchFocusPanel::updateBadgePulse()
    {
        const float wheel = processor_.getModWheelValue();
        const float expr = processor_.getExpressionValue();
        const bool wheelActive = wheel > 0.02f;
        const bool exprActive = expr > 0.02f;
        const float pulse = 0.5f + 0.5f * std::sin(badgePulsePhase_ * juce::MathConstants<float>::twoPi);

        if (modWheelBadge_.isVisible())
        {
            const auto base = palette::kModModWheel;
            modWheelBadge_.setColour(juce::Label::backgroundColourId,
                                     base.withAlpha(wheelActive ? 0.12f + 0.14f * pulse : 0.12f));
            modWheelBadge_.setColour(juce::Label::textColourId,
                                     wheelActive ? base.brighter(wheelActive ? 0.08f * pulse : 0.0f) : base);
        }

        if (expressionBadge_.isVisible())
        {
            const auto base = palette::kModExpression;
            expressionBadge_.setColour(juce::Label::backgroundColourId,
                                       base.withAlpha(exprActive ? 0.12f + 0.14f * pulse : 0.12f));
            expressionBadge_.setColour(juce::Label::textColourId,
                                       exprActive ? base.brighter(exprActive ? 0.08f * pulse : 0.0f) : base);
        }

        if (sidechainBadge_.isVisible())
        {
            const bool scActive = processor_.getSidechainActive();
            const auto base = palette::kAccent;
            sidechainBadge_.setColour(juce::Label::backgroundColourId,
                                      base.withAlpha(scActive ? 0.12f + 0.14f * pulse : 0.12f));
            sidechainBadge_.setColour(juce::Label::textColourId,
                                      scActive ? base.brighter(0.08f * pulse) : base);
        }
    }

    void PatchFocusPanel::paint(juce::Graphics& g)
    {
        if (desktopPlayModeLayout_)
        {
            auto bounds = getLocalBounds().toFloat();
            g.setColour(palette::kPanelRaised.withAlpha(0.92f));
            g.fillRoundedRectangle(bounds, 10.0f);
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);
            return;
        }

        if (!basicLayout_ || compactLayout_ || featureKnobCount_ == 0)
            return;

        const std::size_t featureEnd = juce::jmin(featureKnobCount_, knobs_.size());
        juce::Rectangle<int> featureBounds;
        for (std::size_t i = 0; i < featureEnd; ++i)
            featureBounds = featureBounds.isEmpty() ? knobs_[i]->getBounds() : featureBounds.getUnion(knobs_[i]->getBounds());

        if (featureBounds.isEmpty())
            return;

        const auto frame = featureBounds.toFloat().expanded(8.0f, 10.0f);
        juce::Path card;
        card.addRoundedRectangle(frame, 9.0f);

        g.setColour(palette::kAccentWarm.withAlpha(0.06f));
        g.fillPath(card);

        g.setColour(palette::kAccentWarm.withAlpha(0.32f));
        g.strokePath(card, juce::PathStrokeType(1.1f));

        constexpr float tick = 10.0f;
        const juce::Point<float> corners[] = {
            frame.getTopLeft(),
            frame.getTopRight(),
            frame.getBottomLeft(),
            frame.getBottomRight(),
        };
        for (const auto& c : corners)
        {
            g.setColour(palette::kAccentWarm.withAlpha(0.55f));
            if (c == frame.getTopLeft())
            {
                g.drawLine(c.x, c.y, c.x + tick, c.y, 1.4f);
                g.drawLine(c.x, c.y, c.x, c.y + tick, 1.4f);
            }
            else if (c == frame.getTopRight())
            {
                g.drawLine(c.x, c.y, c.x - tick, c.y, 1.4f);
                g.drawLine(c.x, c.y, c.x, c.y + tick, 1.4f);
            }
            else if (c == frame.getBottomLeft())
            {
                g.drawLine(c.x, c.y, c.x + tick, c.y, 1.4f);
                g.drawLine(c.x, c.y, c.x, c.y - tick, 1.4f);
            }
            else
            {
                g.drawLine(c.x, c.y, c.x - tick, c.y, 1.4f);
                g.drawLine(c.x, c.y, c.x, c.y - tick, 1.4f);
            }
        }
    }

    void PatchFocusPanel::timerCallback()
    {
        const auto& patch = processor_.getCurrentPatch();

        PatchFocusLayout layout;
        if (desktopPlayModeLayout_)
        {
            const std::size_t macroCount = ipadPlayLayout_ ? layout::kIpadPlayMacroCount : layout::kDesktopPlayModeMacroCount;
            layout.featureKnobs.reserve(macroCount);
            for (std::size_t i = 0; i < macroCount; ++i)
            {
                const auto& macro = patch.macros[i];
                const auto label = macro.name.empty() ? juce::String(kMacroParameterNames[i]) : juce::String(macro.name);
                layout.featureKnobs.push_back({PatchFocusKnobKind::Macro, i, {}, label});
            }
        }
        else if (compactLayout_)
        {
            layout = inferPatchFocusLayout(patch, 4, 0, &processor_.apvts);
        }
        else
        {
            const std::size_t maxFeature = kMaxFeatureKoinCount;
            const std::size_t maxStandard =
                compactLayout_ ? kCompactStandardParamKoinCount : kStandardParamKoinCount;
            layout = inferPatchFocusLayout(patch, maxFeature, maxStandard, &processor_.apvts);
        }

        const auto patchName = patch.metadata.name.empty() ? juce::String("Init") : juce::String(patch.metadata.name);
        const bool authored = !patch.uiFocus.knobs.empty();
        if (compactLayout_)
        {
            subtitleLabel_.setText(authored ? "Performance macros" : "Feature macros", juce::dontSendNotification);
        }
        else if (desktopPlayModeLayout_)
        {
            subtitleLabel_.setText({}, juce::dontSendNotification);
        }
        else
        {
            subtitleLabel_.setText("Playing " + patchName + " — " +
                                       juce::String(static_cast<int>(layout.featureKnobs.size())) +
                                       (authored ? " patch macros + " : " feature macros + ") +
                                       juce::String(static_cast<int>(layout.standardKnobs.size())) + " standard controls",
                                   juce::dontSendNotification);
        }

        standardSectionLabel_.setVisible(!compactLayout_ && !layout.standardKnobs.empty());

        const auto macroHints = formatFeatureMacroHints(patch, layout.featureKnobs);
        macroHintsLabel_.setText(macroHints, juce::dontSendNotification);
        macroHintsLabel_.setVisible(!compactLayout_ && macroHints.isNotEmpty());

        if (layout != lastLayout_)
        {
            lastLayout_ = layout;
            rebuildKnobs(layout);
            applyLayoutMode();
            resized();
        }

        const auto wheelText = formatModWheelStatus(patch, processor_.getModWheelValue());
        modWheelBadge_.setText(wheelText, juce::dontSendNotification);
        modWheelBadge_.setVisible(!wheelText.isEmpty());

        const auto exprText = formatExpressionStatus(patch, processor_.getExpressionValue());
        expressionBadge_.setText(exprText, juce::dontSendNotification);
        expressionBadge_.setVisible(!exprText.isEmpty());

        const auto scText = formatSidechainStatus(patch, processor_.getSidechainLevel(), processor_.getSidechainActive()
#if JucePlugin_Build_AU || JucePlugin_Build_VST3
                                                      ,
                                                  true
#else
                                                      ,
                                                  false
#endif
        );
        sidechainBadge_.setText(scText, juce::dontSendNotification);
        sidechainBadge_.setVisible(!scText.isEmpty());

        badgePulsePhase_ += 1.0f / 12.0f;
        if (badgePulsePhase_ > 1.0f)
            badgePulsePhase_ -= 1.0f;
        updateBadgePulse();

        const float wheel = processor_.getModWheelValue();
        const float expr = processor_.getExpressionValue();
        const bool midiMoved =
            (lastModWheel_ >= 0.0f && std::abs(wheel - lastModWheel_) > 0.005f) ||
            (lastExpression_ >= 0.0f && std::abs(expr - lastExpression_) > 0.005f);
        lastModWheel_ = wheel;
        lastExpression_ = expr;
        if (midiMoved && onPerformanceActivity)
            onPerformanceActivity(-1);
    }

    void PatchFocusPanel::rebuildKnobs(const PatchFocusLayout& layout)
    {
        knobs_.clear();
        featureKnobCount_ = layout.featureKnobs.size();
        if (!compactLayout_)
        {
            panel_.removeAllChildren();
            panel_.addAndMakeVisible(introLabel_);
            panel_.addAndMakeVisible(ringLegendLabel_);
            panel_.addAndMakeVisible(subtitleLabel_);
            panel_.addAndMakeVisible(macroHintsLabel_);
            panel_.addAndMakeVisible(standardSectionLabel_);
            panel_.addAndMakeVisible(modWheelBadge_);
            panel_.addAndMakeVisible(expressionBadge_);
            panel_.addAndMakeVisible(sidechainBadge_);
            panel_.addAndMakeVisible(advancedButton_);
            panel_.addAndMakeVisible(desktopDeckTitleLabel_);
            panel_.addAndMakeVisible(desktopDeckStatusLabel_);
        }
        else
        {
            removeAllChildren();
            addAndMakeVisible(subtitleLabel_);
            addAndMakeVisible(standardSectionLabel_);
        }

        auto addKnob = [&](const PatchFocusKnobSpec& spec, bool featured) {
            std::unique_ptr<GlowKnob> knob;
            if (spec.kind == PatchFocusKnobKind::Macro)
            {
                if (spec.macroIndex >= 8)
                    return;
                knob = std::make_unique<GlowKnob>(processor_.apvts, kMacroParameterIds[spec.macroIndex], spec.label,
                                                  nullptr, featured ? palette::kAccentWarm : juce::Colours::transparentBlack);
                knob->enableMacroActivityRing(processor_, spec.macroIndex);
            }
            else if (spec.kind == PatchFocusKnobKind::Morph)
            {
                knob = std::make_unique<GlowKnob>(processor_.apvts, kMorphPositionId, spec.label, nullptr,
                                                  palette::kAccent);
            }
            else
            {
                if (processor_.apvts.getParameter(spec.paramId) == nullptr)
                    return;
                knob = std::make_unique<GlowKnob>(processor_.apvts, spec.paramId, spec.label);
                if (const auto mapped = findModDestinationForApvtsParam(spec.paramId))
                    knob->enableModulationTarget(processor_, mapped->first, mapped->second);
            }

            knob->setFeaturedPerformanceMacro(featured);
            if (compactLayout_)
                addAndMakeVisible(*knob);
            else
                panel_.addAndMakeVisible(*knob);
            knobs_.push_back(std::move(knob));
        };

        for (const auto& spec : layout.featureKnobs)
            addKnob(spec, true);
        for (const auto& spec : layout.standardKnobs)
            addKnob(spec, false);
    }

    void PatchFocusPanel::layoutKnobGrid(juce::Rectangle<int> bounds, std::size_t startIndex, std::size_t count,
                                           int minCellWidth)
    {
        if (count == 0 || startIndex >= knobs_.size())
            return;

        int columns = static_cast<int>(count);
        int rows = 1;
        if (columns > 1 && bounds.getWidth() / columns < minCellWidth)
        {
            columns = juce::jmax(1, bounds.getWidth() / minCellWidth);
            rows = static_cast<int>((count + static_cast<std::size_t>(columns) - 1) / static_cast<std::size_t>(columns));
        }

        const int cellWidth = bounds.getWidth() / juce::jmax(1, columns);
        const int cellHeight = bounds.getHeight() / juce::jmax(1, rows);

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t knobIndex = startIndex + i;
            if (knobIndex >= knobs_.size())
                break;
            const int col = static_cast<int>(i) % columns;
            const int row = static_cast<int>(i) / columns;
            knobs_[knobIndex]->setBounds(bounds.getX() + col * cellWidth, bounds.getY() + row * cellHeight, cellWidth,
                                        cellHeight);
        }
    }

    void PatchFocusPanel::resized()
    {
        if (desktopPlayModeLayout_)
        {
            panel_.setBounds(getLocalBounds());
            auto bounds = panel_.getLocalBounds().reduced(layout::kDesktopPlayModePerformanceDeckPadding);

            auto header = bounds.removeFromTop(layout::kDesktopPlayModeMacrosHeaderHeight);
            desktopDeckTitleLabel_.setBounds(header.removeFromLeft(header.getWidth() - 160));
            desktopDeckStatusLabel_.setBounds(header);

            const int headerGap = ipadPlayLayout_ ? 12 : 20;
            bounds.removeFromTop(headerGap);
            auto knobRow = bounds.removeFromTop(ipadPlayLayout_ ? 148 : layout::kDesktopPlayModeKnobsRowHeight);

            const int knobWidth =
                ipadPlayLayout_ ? layout::kIpadPlayMacroKnobWidth : layout::kDesktopPlayModeMacroKnobWidth;
            const int knobGap = ipadPlayLayout_ ? layout::kIpadPlayMacroKnobGap : layout::kDesktopPlayModeMacroKnobGap;
            const std::size_t knobCount = juce::jmin(
                knobs_.size(),
                static_cast<std::size_t>(ipadPlayLayout_ ? layout::kIpadPlayMacroCount : layout::kDesktopPlayModeMacroCount));
            for (std::size_t i = 0; i < knobCount; ++i)
            {
                const int x = static_cast<int>(i) * (knobWidth + knobGap);
                knobs_[i]->setBounds(knobRow.getX() + x, knobRow.getY(), knobWidth, knobRow.getHeight());
            }
            return;
        }

        if (compactLayout_)
        {
            if (!orbitHole_.isEmpty() && !knobs_.empty())
            {
                subtitleLabel_.setBounds(0, 0, 0, 0);
                const auto centre = orbitHole_.getCentre();
                const int orbitRadius = juce::jmax(orbitHole_.getWidth(), orbitHole_.getHeight()) / 2 + 36;
                const int knobSize = 64;
                static constexpr float kAngles[] = {
                    -juce::MathConstants<float>::halfPi,
                    0.0f,
                    juce::MathConstants<float>::halfPi,
                    juce::MathConstants<float>::pi,
                };
                for (std::size_t i = 0; i < knobs_.size() && i < 4; ++i)
                {
                    const float a = kAngles[i];
                    const int cx = centre.x + static_cast<int>(std::cos(a) * static_cast<float>(orbitRadius));
                    const int cy = centre.y + static_cast<int>(std::sin(a) * static_cast<float>(orbitRadius));
                    knobs_[i]->setBounds(cx - knobSize / 2, cy - knobSize / 2, knobSize, knobSize);
                }
                return;
            }

            auto bounds = getLocalBounds().reduced(layout::kCompactMacroPanelPadding);
            subtitleLabel_.setBounds(bounds.removeFromTop(16));
            bounds.removeFromTop(layout::kCompactMacroRowGap);

            const std::size_t knobCount = knobs_.size();
            if (knobCount == 0)
                return;

            const int columns = 3;
            const int rows = static_cast<int>((knobCount + static_cast<std::size_t>(columns) - 1)
                                              / static_cast<std::size_t>(columns));
            const int cellWidth = (bounds.getWidth() - (columns - 1) * layout::kCompactMacroKnobGap) / columns;
            const int cellHeight = layout::kCompactMacroKnobSize + 18;

            for (std::size_t i = 0; i < knobCount; ++i)
            {
                const int col = static_cast<int>(i) % columns;
                const int row = static_cast<int>(i) / columns;
                const int x = bounds.getX() + col * (cellWidth + layout::kCompactMacroKnobGap);
                const int y = bounds.getY() + row * (cellHeight + layout::kCompactMacroRowGap);
                knobs_[i]->setBounds(x, y, cellWidth, cellHeight);
            }
            juce::ignoreUnused(rows);
            return;
        }

        panel_.setBounds(getLocalBounds());

        auto bounds = panel_.getContentBounds().reduced(4, 0);
        if (introLabel_.isVisible())
        {
            introLabel_.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(4);
        }

        if (ringLegendLabel_.isVisible())
        {
            ringLegendLabel_.setBounds(bounds.removeFromTop(14));
            bounds.removeFromTop(4);
        }

        auto header = bounds.removeFromTop(22);
        subtitleLabel_.setBounds(header);

        if (macroHintsLabel_.isVisible())
        {
            bounds.removeFromTop(4);
            macroHintsLabel_.setBounds(bounds.removeFromTop(32));
        }

        if (modWheelBadge_.isVisible())
        {
            bounds.removeFromTop(4);
            modWheelBadge_.setBounds(bounds.removeFromTop(18));
        }

        if (expressionBadge_.isVisible())
        {
            bounds.removeFromTop(4);
            expressionBadge_.setBounds(bounds.removeFromTop(18));
        }

        if (sidechainBadge_.isVisible())
        {
            bounds.removeFromTop(4);
            sidechainBadge_.setBounds(bounds.removeFromTop(18));
        }

        bounds.removeFromTop(basicLayout_ ? 8 : 4);

        if (knobs_.empty())
            return;

        constexpr int kMinFeatureCellWidthBasic = 140;
        constexpr int kMinStandardCellWidthBasic = 108;
        const int minFeatureCellWidth = basicLayout_ ? kMinFeatureCellWidthBasic : 108;
        const int minStandardCellWidth = basicLayout_ ? kMinStandardCellWidthBasic : 96;

        const std::size_t standardCount = knobs_.size() > featureKnobCount_ ? knobs_.size() - featureKnobCount_ : 0;

        if (featureKnobCount_ > 0)
        {
            const int featureRowHeight =
                juce::jmax(96, bounds.getHeight() / (standardCount > 0 ? 2 : 1));
            auto featureArea = bounds.removeFromTop(featureRowHeight);
            layoutKnobGrid(featureArea, 0, featureKnobCount_, minFeatureCellWidth);
            bounds.removeFromTop(6);
        }

        if (standardCount > 0)
        {
            standardSectionLabel_.setBounds(bounds.removeFromTop(16));
            bounds.removeFromTop(4);
            layoutKnobGrid(bounds, featureKnobCount_, standardCount, minStandardCellWidth);
        }
    }

} // namespace pw8::plugin::ui
