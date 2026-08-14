#include "PatchFocusPanel.h"

#include <cmath>

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

        introLabel_.setText("This patch's main controls — turn these while you play.",
                            juce::dontSendNotification);
        introLabel_.setJustificationType(juce::Justification::centredLeft);
        introLabel_.setFont(fonts::value(fonts::kBodyLabelSize));
        introLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        panel_.addAndMakeVisible(introLabel_);

        panel_.addAndMakeVisible(subtitleLabel_);
        subtitleLabel_.setJustificationType(juce::Justification::centredLeft);
        subtitleLabel_.setFont(fonts::value(fonts::kBodyLabelSize));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);

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

        advancedButton_.setButtonText("Mod Matrix (M)");
        advancedButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        advancedButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        advancedButton_.onClick = [this] {
            if (onAdvancedClicked)
                onAdvancedClicked();
        };
        panel_.addAndMakeVisible(advancedButton_);
        advancedButton_.setVisible(false);

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
        introLabel_.setVisible(basicLayout && !compactLayout_);
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
            basicLayout_ = false;
        introLabel_.setVisible(basicLayout_ && !compactLayout_);
        panel_.setVisible(!compactLayout_);
        if (!compactLayout_)
            orbitHole_ = {};
        applyLayoutMode();
        resized();
    }

    void PatchFocusPanel::setOrbitHole(juce::Rectangle<int> centerHole)
    {
        orbitHole_ = centerHole;
        resized();
    }

    void PatchFocusPanel::applyLayoutMode()
    {
        const int dialCap = compactLayout_ ? 56 : (basicLayout_ ? 120 : 72);
        const auto deckedSize = compactLayout_ ? GlowKnob::DeckedKnobSize::Small
                                               : (basicLayout_ ? GlowKnob::DeckedKnobSize::Large
                                                               : GlowKnob::DeckedKnobSize::Medium);
        for (auto& knob : knobs_)
        {
            knob->setMaxDialDiameter(dialCap);
            knob->setDeckedStyle(true, deckedSize);
        }
    }

    void PatchFocusPanel::refreshFromPatch()
    {
        lastSpecs_.clear();
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
    }

    void PatchFocusPanel::paint(juce::Graphics& g)
    {
        if (!basicLayout_ || compactLayout_)
            return;

        const auto frame = panel_.getBounds().toFloat().expanded(3.0f);
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
        const std::size_t maxKnobs = compactLayout_ ? 4 : kStandardKoinCount;
        const auto specs = inferPatchFocusKnobs(patch, maxKnobs, &processor_.apvts);

        const auto patchName = patch.metadata.name.empty() ? juce::String("Init") : juce::String(patch.metadata.name);
        const bool authored = !patch.uiFocus.knobs.empty();
        if (compactLayout_)
        {
            subtitleLabel_.setText(authored ? "Patch controls" : "Focus controls", juce::dontSendNotification);
        }
        else
        {
            subtitleLabel_.setText("Playing " + patchName + " — " + juce::String(static_cast<int>(specs.size())) +
                                       (authored ? " patch-authored controls" : " focus controls"),
                                   juce::dontSendNotification);
        }

        if (specs != lastSpecs_)
        {
            lastSpecs_ = specs;
            rebuildKnobs(specs);
            applyLayoutMode();
            resized();
        }

        const auto wheelText = formatModWheelStatus(patch, processor_.getModWheelValue());
        modWheelBadge_.setText(wheelText, juce::dontSendNotification);
        modWheelBadge_.setVisible(!wheelText.isEmpty());

        const auto exprText = formatExpressionStatus(patch, processor_.getExpressionValue());
        expressionBadge_.setText(exprText, juce::dontSendNotification);
        expressionBadge_.setVisible(!exprText.isEmpty());

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

    void PatchFocusPanel::rebuildKnobs(const std::vector<PatchFocusKnobSpec>& specs)
    {
        knobs_.clear();
        if (!compactLayout_)
        {
            panel_.removeAllChildren();
            panel_.addAndMakeVisible(introLabel_);
            panel_.addAndMakeVisible(subtitleLabel_);
            panel_.addAndMakeVisible(modWheelBadge_);
            panel_.addAndMakeVisible(expressionBadge_);
            panel_.addAndMakeVisible(advancedButton_);
        }
        else
        {
            removeAllChildren();
            addAndMakeVisible(subtitleLabel_);
        }

        for (const auto& spec : specs)
        {
            std::unique_ptr<GlowKnob> knob;
            if (spec.kind == PatchFocusKnobKind::Macro)
            {
                knob = std::make_unique<GlowKnob>(processor_.apvts, kMacroParameterIds[spec.macroIndex], spec.label,
                                                  nullptr, palette::kAccentWarm);
            }
            else
            {
                if (processor_.apvts.getParameter(spec.paramId) == nullptr)
                    continue;
                knob = std::make_unique<GlowKnob>(processor_.apvts, spec.paramId, spec.label);
            }
            if (compactLayout_)
                addAndMakeVisible(*knob);
            else
                panel_.addAndMakeVisible(*knob);
            knobs_.push_back(std::move(knob));
        }
    }

    void PatchFocusPanel::resized()
    {
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

            auto bounds = getLocalBounds().reduced(2, 0);
            subtitleLabel_.setBounds(bounds.removeFromTop(16));
            bounds.removeFromTop(4);

            if (knobs_.empty())
                return;

            const int columns = static_cast<int>(knobs_.size()) <= 2 ? static_cast<int>(knobs_.size()) : 2;
            const int rows = static_cast<int>((knobs_.size() + static_cast<std::size_t>(columns) - 1) /
                                              static_cast<std::size_t>(columns));
            const int cellWidth = bounds.getWidth() / juce::jmax(1, columns);
            const int cellHeight = bounds.getHeight() / juce::jmax(1, rows);

            for (std::size_t i = 0; i < knobs_.size(); ++i)
            {
                const int col = static_cast<int>(i) % columns;
                const int row = static_cast<int>(i) / columns;
                knobs_[i]->setBounds(bounds.getX() + col * cellWidth, bounds.getY() + row * cellHeight, cellWidth,
                                     cellHeight);
            }
            return;
        }

        panel_.setBounds(getLocalBounds());

        auto bounds = panel_.getContentBounds().reduced(4, 0);
        if (introLabel_.isVisible())
        {
            introLabel_.setBounds(bounds.removeFromTop(20));
            bounds.removeFromTop(4);
        }

        auto header = bounds.removeFromTop(22);
        subtitleLabel_.setBounds(header);

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

        bounds.removeFromTop(basicLayout_ ? 8 : 4);

        if (knobs_.empty())
            return;

        constexpr int kMinKnobCellWidthBasic = 140;
        constexpr int kMinKnobCellWidthCompact = 108;
        const int minCellWidth = basicLayout_ ? kMinKnobCellWidthBasic : kMinKnobCellWidthCompact;

        int columns = static_cast<int>(knobs_.size());
        int rows = 1;
        if (columns > 1 && bounds.getWidth() / columns < minCellWidth)
        {
            columns = juce::jmax(1, bounds.getWidth() / minCellWidth);
            rows = static_cast<int>((knobs_.size() + static_cast<std::size_t>(columns) - 1) /
                                    static_cast<std::size_t>(columns));
        }

        const int cellWidth = bounds.getWidth() / juce::jmax(1, columns);
        const int cellHeight = bounds.getHeight() / juce::jmax(1, rows);

        for (std::size_t i = 0; i < knobs_.size(); ++i)
        {
            const int col = static_cast<int>(i) % columns;
            const int row = static_cast<int>(i) / columns;
            knobs_[i]->setBounds(bounds.getX() + col * cellWidth, bounds.getY() + row * cellHeight, cellWidth,
                                 cellHeight);
        }
    }

} // namespace pw8::plugin::ui
