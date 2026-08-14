#include "ConcentricGlowKnob.h"

#include "../theme/DeckedKnobDraw.h"
#include "../theme/KnobRingDraw.h"
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
        configureSlider(outerSlider_);
        configureSlider(innerSlider_);
        outerSlider_.valueToText = std::move(outerValueToText);
        innerSlider_.valueToText = std::move(innerValueToText);

        outerSlider_.setLookAndFeel(&outerLookAndFeel_);
        innerSlider_.setLookAndFeel(&innerLookAndFeel_);

        outerSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        innerSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 76, kTextBoxHeight);
        innerSlider_.setColour(juce::Slider::rotarySliderFillColourId, palette::kAccent);

        addAndMakeVisible(outerSlider_);
        addAndMakeVisible(innerSlider_);
        outerSlider_.addMouseListener(this, false);
        innerSlider_.addMouseListener(this, false);

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
        outerSlider_.removeMouseListener(this);
        innerSlider_.removeMouseListener(this);
        outerSlider_.setLookAndFeel(nullptr);
        innerSlider_.setLookAndFeel(nullptr);
    }

    void ConcentricGlowKnob::configureSlider(FormattedSlider& slider)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters(rotary::kStartAngle, rotary::kEndAngle, true);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        slider.setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        slider.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        slider.getProperties().set("maxDialDiameter", maxDialDiameter_);
    }

    void ConcentricGlowKnob::setInnerAccentColour(juce::Colour colour)
    {
        innerSlider_.setColour(juce::Slider::rotarySliderFillColourId, colour);
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

    ConcentricGlowKnob::ModRingState& ConcentricGlowKnob::modStateForEvent(const juce::MouseEvent& event)
    {
        if (event.eventComponent == &outerSlider_ || outerSlider_.isParentOf(event.eventComponent))
            return outerMod_;
        return innerMod_;
    }

    ConcentricGlowKnob::ModRingState& ConcentricGlowKnob::modStateAtPoint(juce::Point<int> localPos)
    {
        if (isOuterRingPoint(localPos))
            return outerMod_;
        return innerMod_;
    }

    bool ConcentricGlowKnob::isOuterRingPoint(juce::Point<int> localPos) const
    {
        if (!outerSlider_.getBounds().contains(localPos))
            return false;
        if (!innerSlider_.getBounds().contains(localPos))
            return true;

        const auto outerCentre = outerSlider_.getBounds().toFloat().getCentre();
        const float outerRadius =
            juce::jmin(static_cast<float>(outerSlider_.getWidth()), static_cast<float>(outerSlider_.getHeight())) * 0.5f;
        const float innerRadius =
            juce::jmin(static_cast<float>(innerSlider_.getWidth()), static_cast<float>(innerSlider_.getHeight())) * 0.5f;
        return localPos.toFloat().getDistanceFrom(outerCentre) >= (innerRadius + outerRadius) * 0.5f;
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
        auto& modState = modStateForEvent(event);

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
    }

    void ConcentricGlowKnob::mouseDrag(const juce::MouseEvent& event)
    {
        auto& modState = modStateForEvent(event);
        if (modState.depthDragActive)
            handleModMouseDrag(event, modState);
    }

    void ConcentricGlowKnob::mouseUp(const juce::MouseEvent&)
    {
        innerMod_.depthDragActive = false;
        outerMod_.depthDragActive = false;
    }

    bool ConcentricGlowKnob::isInterestedInDragSource(const SourceDetails& details)
    {
        return (innerMod_.destination != modulation::ModDestination::None ||
                outerMod_.destination != modulation::ModDestination::None) &&
               parseModSourceDragDescription(details.description.toString()).has_value();
    }

    void ConcentricGlowKnob::itemDragEnter(const SourceDetails& details)
    {
        auto& modState = modStateAtPoint(details.localPosition.toInt());
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
        auto& modState = modStateAtPoint(details.localPosition.toInt());
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

        const int outerDiameter =
            juce::jmin(maxDialDiameter_, bounds.getWidth(), juce::jmax(32, bounds.getHeight() - kTextBoxHeight));
        const int innerDiameter =
            juce::jmax(24, outerDiameter - static_cast<int>(decked::kDualKnobInnerInset * 2.0f));
        const int textBoxWidth = juce::jmin(72, juce::jmax(44, bounds.getWidth() - 4));

        const auto dialColumn = bounds.withSizeKeepingCentre(bounds.getWidth(), outerDiameter + kTextBoxHeight);
        outerSlider_.setBounds(dialColumn.withHeight(outerDiameter));

        innerSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, kTextBoxHeight);
        innerSlider_.setBounds(dialColumn.withSizeKeepingCentre(bounds.getWidth(), innerDiameter + kTextBoxHeight));

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

        const auto& refSlider = outerOrbit ? outerSlider_ : innerSlider_;
        const auto sliderBounds = refSlider.getBounds().toFloat().reduced(4.0f);
        const float diameter = juce::jmax(16.0f, juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight()));
        const auto knobBounds = refSlider.getBounds().toFloat().withSizeKeepingCentre(diameter, diameter);
        const float dialRadius = diameter * 0.5f;
        const float modGap = juce::jmax(3.5f, dialRadius * (outerOrbit ? 0.09f : 0.07f));
        const float modRingRadius = orbitRadius + modGap;
        const float modStrokeWidth = juce::jmax(3.6f, dialRadius * (outerOrbit ? 0.082f : 0.068f));

        knobrings::Layout ringLayout{knobBounds.getCentre(), dialRadius, orbitRadius, modRingRadius, modStrokeWidth};

        if (state.dragHover)
            knobrings::drawDragHoverRing(g, ringLayout);

        if (!state.colour.isTransparent())
            knobrings::strokeModRouteArc(g, ringLayout, state.amountNormalized, state.colour,
                                         outerOrbit ? 1.15f : 1.0f);
    }

    void ConcentricGlowKnob::paintOverChildren(juce::Graphics& g)
    {
        const auto outerBounds = outerSlider_.getBounds().toFloat().reduced(4.0f);
        const float outerDiameter = juce::jmax(16.0f, juce::jmin(outerBounds.getWidth(), outerBounds.getHeight()));
        const float outerOrbitRadius = outerDiameter * 0.45f;

        const auto innerBounds = innerSlider_.getBounds().toFloat().reduced(4.0f);
        const float innerDiameter = juce::jmax(16.0f, juce::jmin(innerBounds.getWidth(), innerBounds.getHeight()));
        const float innerOrbitRadius = innerDiameter * 0.42f;

        paintModRing(g, outerMod_, outerOrbitRadius, true);
        paintModRing(g, innerMod_, innerOrbitRadius, false);

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
