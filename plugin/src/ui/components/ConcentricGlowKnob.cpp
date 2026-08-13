#include "ConcentricGlowKnob.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../theme/ObsidianRotary.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kInnerLabelHeight = 12;
        constexpr int kOuterLabelHeight = 10;
        constexpr int kTextBoxHeight = 16;
        constexpr int kDepthPopoverHeight = 22;
    } // namespace

    juce::String ConcentricGlowKnob::FormattedSlider::getTextFromValue(double value)
    {
        if (valueToText)
            return valueToText(static_cast<float>(value));
        return juce::String(value, 2);
    }

    ConcentricGlowKnob::ConcentricGlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& innerParamId,
                                           const juce::String& outerParamId, const juce::String& innerLabel,
                                           const juce::String& outerLabel,
                                           std::function<juce::String(float)> innerValueToText,
                                           std::function<juce::String(float)> outerValueToText)
    {
        configureSlider(innerSlider_, "inner");
        configureSlider(outerSlider_, "outer");
        innerSlider_.valueToText = std::move(innerValueToText);
        outerSlider_.valueToText = std::move(outerValueToText);

        innerSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 76, kTextBoxHeight);
        outerSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        innerSlider_.setInterceptsMouseClicks(false, false);
        outerSlider_.setInterceptsMouseClicks(false, false);

        addAndMakeVisible(innerSlider_);
        addAndMakeVisible(outerSlider_);

        innerLabel_.setText(innerLabel.toUpperCase(), juce::dontSendNotification);
        innerLabel_.setJustificationType(juce::Justification::centred);
        innerLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        innerLabel_.setFont(fonts::label(10.5f));
        addAndMakeVisible(innerLabel_);

        outerLabel_.setText(outerLabel.toUpperCase(), juce::dontSendNotification);
        outerLabel_.setJustificationType(juce::Justification::centred);
        outerLabel_.setColour(juce::Label::textColourId, palette::kMurmurViolet);
        outerLabel_.setFont(fonts::label(9.0f));
        addAndMakeVisible(outerLabel_);

        innerAttachment_ =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, innerParamId, innerSlider_);
        outerAttachment_ =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, outerParamId, outerSlider_);
    }

    ConcentricGlowKnob::~ConcentricGlowKnob()
    {
        stopTimer();
    }

    void ConcentricGlowKnob::configureSlider(FormattedSlider& slider, const juce::String& ringRole)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters(rotary::kStartAngle, rotary::kEndAngle, true);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        slider.setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        slider.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        slider.setColour(juce::Slider::rotarySliderFillColourId, palette::kMurmurViolet);
        slider.getProperties().set("knobRingRole", ringRole);
        slider.getProperties().set("maxDialDiameter", maxDialDiameter_);
    }

    void ConcentricGlowKnob::enableInnerModulationTarget(PatchworkEightProcessor& processor,
                                                         modulation::ModDestination destination,
                                                         std::uint8_t targetIndex)
    {
        modProcessor_ = &processor;
        innerMod_.destination = destination;
        innerMod_.targetIndex = targetIndex;
        startTimerHz(8);
        timerCallback();
    }

    void ConcentricGlowKnob::enableOuterModulationTarget(PatchworkEightProcessor& processor,
                                                         modulation::ModDestination destination,
                                                         std::uint8_t targetIndex)
    {
        modProcessor_ = &processor;
        outerMod_.destination = destination;
        outerMod_.targetIndex = targetIndex;
        startTimerHz(8);
        timerCallback();
    }

    void ConcentricGlowKnob::setModAssignmentController(ModAssignmentController* controller)
    {
        modAssignment_ = controller;
    }

    void ConcentricGlowKnob::setMaxDialDiameter(int diameter)
    {
        maxDialDiameter_ = diameter;
        innerSlider_.getProperties().set("maxDialDiameter", diameter);
        outerSlider_.getProperties().set("maxDialDiameter", diameter);
        resized();
    }

    void ConcentricGlowKnob::refreshModRingState(ModRingState& state)
    {
        juce::Colour next = juce::Colours::transparentBlack;
        auto nextSource = modulation::ModSource::None;
        float nextAmountNorm = 0.0f;
        if (modProcessor_ != nullptr && state.destination != modulation::ModDestination::None)
        {
            for (const auto& route : modProcessor_->getCurrentPatch().layerA.modRoutes)
            {
                if (route.isActive() && route.destination == state.destination &&
                    route.targetIndex == state.targetIndex)
                {
                    next = palette::modSourceColour(static_cast<int>(route.source));
                    nextSource = route.source;
                    const float def = defaultModAmountFor(state.destination);
                    nextAmountNorm =
                        juce::jlimit(0.0f, 1.0f, std::abs(route.amount) / juce::jmax(0.001f, std::abs(def)));
                    break;
                }
            }
        }
        if (next != state.colour || nextAmountNorm != state.amountNormalized || nextSource != state.source)
        {
            state.colour = next;
            state.source = nextSource;
            state.amountNormalized = nextAmountNorm;
            repaint();
        }
    }

    void ConcentricGlowKnob::timerCallback()
    {
        refreshModRingState(innerMod_);
        refreshModRingState(outerMod_);
    }

    ConcentricGlowKnob::ActiveRing ConcentricGlowKnob::ringAtPoint(juce::Point<float> localPos) const
    {
        const auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        const float diameter =
            juce::jmin(static_cast<float>(maxDialDiameter_), juce::jmin(bounds.getWidth(), bounds.getHeight()));
        const float radius = diameter * 0.5f;
        const float bodyRadius = radius * 0.62f;
        const float orbitRadius = radius * 0.90f;
        const float threshold = (bodyRadius + orbitRadius) * 0.5f;
        const auto centre = bounds.withSizeKeepingCentre(diameter, diameter).getCentre();
        return localPos.getDistanceFrom(centre) >= threshold ? ActiveRing::Outer : ActiveRing::Inner;
    }

    juce::Slider& ConcentricGlowKnob::activeSlider() noexcept
    {
        return activeRing_ == ActiveRing::Outer ? static_cast<juce::Slider&>(outerSlider_) : innerSlider_;
    }

    void ConcentricGlowKnob::forwardMouseToActiveSlider(const juce::MouseEvent& event, bool isDown)
    {
        auto& slider = activeSlider();
        auto rel = event.getEventRelativeTo(&slider);
        if (isDown)
            slider.mouseDown(rel);
        else
            slider.mouseDrag(rel);
    }

    void ConcentricGlowKnob::handleModMouseDown(const juce::MouseEvent& event, ModRingState& state)
    {
        if (state.showDepthPopover && state.depthPopoverArea_.contains(event.getPosition()))
        {
            state.depthDragActive = true;
            state.depthDragStartX = static_cast<float>(event.position.x);
            if (modProcessor_ != nullptr)
            {
                for (const auto& route : modProcessor_->getCurrentPatch().layerA.modRoutes)
                {
                    if (route.isActive() && route.destination == state.destination &&
                        route.targetIndex == state.targetIndex)
                    {
                        state.depthDragStartAmount = route.amount;
                        break;
                    }
                }
            }
            return;
        }

        if (!event.mods.isLeftButtonDown() || modAssignment_ == nullptr || !modAssignment_->isArmed() ||
            modProcessor_ == nullptr || state.destination == modulation::ModDestination::None)
            return;

        const auto source = modAssignment_->armedSource();
        if (!source.has_value())
            return;

        assignModRoute(*modProcessor_, *source, state.destination, state.targetIndex);
        state.colour = palette::modSourceColour(static_cast<int>(*source));
        state.source = *source;
        state.amountNormalized = 1.0f;
        modAssignment_->disarm();
        state.showDepthPopover = true;
        repaint();
    }

    void ConcentricGlowKnob::handleModMouseDrag(const juce::MouseEvent& event, ModRingState& state)
    {
        if (!state.depthDragActive || modProcessor_ == nullptr)
            return;

        const auto range = modAmountRangeFor(state.destination);
        const float span = range.max - range.min;
        const float delta = (static_cast<float>(event.position.x) - state.depthDragStartX) * (span / 100.0f);
        for (const auto& route : modProcessor_->getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive() && route.destination == state.destination && route.targetIndex == state.targetIndex)
            {
                updateModRouteAmount(*modProcessor_, route, state.depthDragStartAmount + delta);
                break;
            }
        }
        refreshModRingState(state);
    }

    void ConcentricGlowKnob::mouseDown(const juce::MouseEvent& event)
    {
        activeRing_ = ringAtPoint(event.position);
        auto& modState = activeRing_ == ActiveRing::Outer ? outerMod_ : innerMod_;

        if (modState.showDepthPopover && modState.depthPopoverArea_.contains(event.getPosition()))
        {
            handleModMouseDown(event, modState);
            return;
        }

        if (event.mods.isRightButtonDown())
        {
            if (!modState.colour.isTransparent() && modProcessor_ != nullptr)
            {
                modProcessor_->removeModRouteLive(modState.source, modState.destination, modState.targetIndex);
                modState.colour = juce::Colours::transparentBlack;
                modState.source = modulation::ModSource::None;
                modState.amountNormalized = 0.0f;
                modState.showDepthPopover = false;
                repaint();
            }
            return;
        }

        handleModMouseDown(event, modState);
        if (modState.depthDragActive)
            return;

        forwardMouseToActiveSlider(event, true);
    }

    void ConcentricGlowKnob::mouseDrag(const juce::MouseEvent& event)
    {
        auto& modState = activeRing_ == ActiveRing::Outer ? outerMod_ : innerMod_;
        if (modState.depthDragActive)
        {
            handleModMouseDrag(event, modState);
            return;
        }
        forwardMouseToActiveSlider(event, false);
    }

    void ConcentricGlowKnob::mouseUp(const juce::MouseEvent& event)
    {
        innerMod_.depthDragActive = false;
        outerMod_.depthDragActive = false;
        activeSlider().mouseUp(event.getEventRelativeTo(&activeSlider()));
    }

    void ConcentricGlowKnob::mouseDoubleClick(const juce::MouseEvent& event)
    {
        activeRing_ = ringAtPoint(event.position);
        activeSlider().mouseDoubleClick(event.getEventRelativeTo(&activeSlider()));
    }

    void ConcentricGlowKnob::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
    {
        activeRing_ = ringAtPoint(event.position);
        activeSlider().mouseWheelMove(event.getEventRelativeTo(&activeSlider()), wheel);
    }

    bool ConcentricGlowKnob::isInterestedInDragSource(const SourceDetails& details)
    {
        const auto& modState = activeRing_ == ActiveRing::Outer ? outerMod_ : innerMod_;
        return modState.destination != modulation::ModDestination::None &&
               parseModSourceDragDescription(details.description.toString()).has_value();
    }

    void ConcentricGlowKnob::itemDragEnter(const SourceDetails&)
    {
        auto& modState = activeRing_ == ActiveRing::Outer ? outerMod_ : innerMod_;
        modState.dragHover = true;
        repaint();
    }

    void ConcentricGlowKnob::itemDragExit(const SourceDetails&)
    {
        innerMod_.dragHover = false;
        outerMod_.dragHover = false;
        repaint();
    }

    void ConcentricGlowKnob::itemDropped(const SourceDetails& details)
    {
        innerMod_.dragHover = false;
        outerMod_.dragHover = false;
        const auto source = parseModSourceDragDescription(details.description.toString());
        auto& modState = activeRing_ == ActiveRing::Outer ? outerMod_ : innerMod_;
        if (source.has_value() && modProcessor_ != nullptr && modState.destination != modulation::ModDestination::None)
        {
            assignModRoute(*modProcessor_, *source, modState.destination, modState.targetIndex);
            modState.colour = palette::modSourceColour(static_cast<int>(*source));
            modState.source = *source;
            modState.amountNormalized = 1.0f;
            modState.showDepthPopover = true;
            if (modAssignment_ != nullptr)
                modAssignment_->disarm();
        }
        repaint();
    }

    void ConcentricGlowKnob::resized()
    {
        auto bounds = getLocalBounds().reduced(4);
        outerLabel_.setBounds(bounds.removeFromTop(kOuterLabelHeight));
        bounds.removeFromTop(1);
        innerLabel_.setBounds(bounds.removeFromBottom(kInnerLabelHeight));

        const int dialDiameter =
            juce::jmin(maxDialDiameter_, bounds.getWidth(), juce::jmax(32, bounds.getHeight() - kTextBoxHeight));
        const int textBoxWidth = juce::jmin(72, juce::jmax(44, bounds.getWidth() - 4));
        innerSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, kTextBoxHeight);
        const auto dialBounds = bounds.withSizeKeepingCentre(bounds.getWidth(), dialDiameter + kTextBoxHeight);
        innerSlider_.setBounds(dialBounds);
        outerSlider_.setBounds(dialBounds.withHeight(dialDiameter));

        if (innerMod_.showDepthPopover || outerMod_.showDepthPopover)
        {
            auto popoverArea = bounds.removeFromBottom(kDepthPopoverHeight).reduced(8, 2);
            innerMod_.depthPopoverArea_ = popoverArea;
            outerMod_.depthPopoverArea_ = popoverArea;
        }
    }

    void ConcentricGlowKnob::paintModRing(juce::Graphics& g, const ModRingState& state, float orbitRadius,
                                          bool outerOrbit) const
    {
        if (state.colour.isTransparent() && !state.dragHover)
            return;

        const auto sliderBounds = innerSlider_.getBounds().toFloat().reduced(4.0f);
        const float diameter = juce::jmax(16.0f, juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight()));
        const auto knobBounds = innerSlider_.getBounds().toFloat().withSizeKeepingCentre(diameter, diameter);
        const float arcRadius = outerOrbit ? orbitRadius : diameter * 0.31f;

        if (state.dragHover)
        {
            g.setColour(palette::kMurmurViolet.withAlpha(0.7f));
            g.drawEllipse(knobBounds.expanded(outerOrbit ? 2.0f : 1.0f), 1.6f);
        }

        if (!state.colour.isTransparent())
        {
            const float endAngle =
                rotary::kStartAngle + rotary::kSweep * juce::jlimit(0.0f, 1.0f, state.amountNormalized);
            juce::Path arc;
            arc.addCentredArc(knobBounds.getCentreX(), knobBounds.getCentreY(), arcRadius, arcRadius, 0.0f,
                              rotary::kStartAngle, endAngle, true);
            g.setColour(state.colour.withAlpha(0.85f));
            g.strokePath(arc, juce::PathStrokeType(outerOrbit ? 2.4f : 2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }
    }

    void ConcentricGlowKnob::paintOverChildren(juce::Graphics& g)
    {
        const auto sliderBounds = innerSlider_.getBounds().toFloat().reduced(4.0f);
        const float diameter = juce::jmax(16.0f, juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight()));
        const float orbitRadius = diameter * 0.45f;

        paintModRing(g, innerMod_, orbitRadius, false);
        paintModRing(g, outerMod_, orbitRadius, true);

        const auto drawDepth = [&](const ModRingState& state) {
            if (!state.showDepthPopover || state.depthPopoverArea_.isEmpty())
                return;
            auto pop = state.depthPopoverArea_.toFloat();
            g.setColour(palette::kPanelRaised);
            g.fillRoundedRectangle(pop, 3.0f);
            auto fill = pop.reduced(2.0f);
            fill.setWidth(fill.getWidth() * state.amountNormalized);
            g.setColour(state.colour.isTransparent() ? palette::kAccent : state.colour.withAlpha(0.75f));
            g.fillRoundedRectangle(fill, 2.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(fonts::kCaptionSize));
            g.drawText("DEPTH", state.depthPopoverArea_, juce::Justification::centred);
        };
        drawDepth(innerMod_);
        drawDepth(outerMod_);
    }

} // namespace pw8::plugin::ui
