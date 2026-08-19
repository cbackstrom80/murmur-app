#include "TripleGlowKnob.h"

#include "../ModPreview.hpp"
#include "../theme/KnobRingDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../theme/RadialGlowDraw.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kDepthPopoverHeight = 22;

        [[nodiscard]] juce::Colour channelAccent(int channel) noexcept
        {
            switch (channel)
            {
                case 1: return juce::Colour(0xff9f80ff);
                case 2: return juce::Colour(0xffff9d00);
                default: return juce::Colour(0xff00ffd2);
            }
        }
    } // namespace

    TripleGlowKnob::TripleGlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& outerParamId,
                                   const juce::String& middleParamId, const juce::String& innerParamId,
                                   const juce::String& outerLabel, const juce::String& middleLabel,
                                   const juce::String& innerLabel,
                                   std::function<juce::String(float)> outerValueToText,
                                   std::function<juce::String(float)> middleValueToText,
                                   std::function<juce::String(float)> innerValueToText)
        : apvts_(apvts),
          outerParamId_(outerParamId),
          middleParamId_(middleParamId),
          innerParamId_(innerParamId),
          outerLabel_(outerLabel),
          middleLabel_(middleLabel),
          innerLabel_(innerLabel),
          outerValueToText_(std::move(outerValueToText)),
          middleValueToText_(std::move(middleValueToText)),
          innerValueToText_(std::move(innerValueToText))
    {
    }

    TripleGlowKnob::~TripleGlowKnob()
    {
        stopTimer();
    }

    void TripleGlowKnob::enableOuterModulationTarget(MurmurProcessor& processor,
                                                     modulation::ModDestination destination,
                                                     std::uint8_t targetIndex)
    {
        modProcessor_ = &processor;
        outerMod_.destination = destination;
        outerMod_.targetIndex = targetIndex;
        startTimerHz(8);
        timerCallback();
    }

    void TripleGlowKnob::enableMiddleModulationTarget(MurmurProcessor& processor,
                                                      modulation::ModDestination destination,
                                                      std::uint8_t targetIndex)
    {
        modProcessor_ = &processor;
        middleMod_.destination = destination;
        middleMod_.targetIndex = targetIndex;
        startTimerHz(8);
        timerCallback();
    }

    void TripleGlowKnob::enableInnerModulationTarget(MurmurProcessor& processor,
                                                     modulation::ModDestination destination,
                                                     std::uint8_t targetIndex)
    {
        modProcessor_ = &processor;
        innerMod_.destination = destination;
        innerMod_.targetIndex = targetIndex;
        startTimerHz(8);
        timerCallback();
    }

    void TripleGlowKnob::setModAssignmentController(ModAssignmentController* controller)
    {
        modAssignment_ = controller;
    }

    void TripleGlowKnob::setExpandedReadout(bool expanded)
    {
        expandedReadout_ = expanded;
        repaint();
    }

    void TripleGlowKnob::setMaxDialDiameter(int diameter)
    {
        maxDialDiameter_ = diameter;
        repaint();
    }

    void TripleGlowKnob::applyFigmaContext(figma::KnobContext context)
    {
        setMaxDialDiameter(figma::specFor(context).diameter);
    }

    juce::Rectangle<float> TripleGlowKnob::dialBounds() const
    {
        return getLocalBounds().toFloat().withSizeKeepingCentre(static_cast<float>(maxDialDiameter_),
                                                                static_cast<float>(maxDialDiameter_));
    }

    TripleGlowKnob::ModRingState& TripleGlowKnob::modStateForChannel(int channel)
    {
        if (channel == 1)
            return middleMod_;
        if (channel == 2)
            return innerMod_;
        return outerMod_;
    }

    TripleGlowKnob::ModRingState& TripleGlowKnob::modStateAtPoint(juce::Point<int> localPos)
    {
        const int channel = radialglow::hitTestTripleRingChannel(localPos.toFloat(), dialBounds());
        if (channel >= 0)
            return modStateForChannel(channel);
        return modStateForChannel(focusedChannel_);
    }

    float TripleGlowKnob::readProportional(int channel) const
    {
        const juce::String& id = channel == 0 ? outerParamId_ : (channel == 1 ? middleParamId_ : innerParamId_);
        if (auto* param = apvts_.getParameter(id))
            return param->getValue();
        return 0.0f;
    }

    juce::String TripleGlowKnob::formatValue(int channel) const
    {
        const juce::String& id = channel == 0 ? outerParamId_ : (channel == 1 ? middleParamId_ : innerParamId_);
        if (auto* raw = apvts_.getRawParameterValue(id))
        {
            const float v = raw->load();
            if (channel == 0 && outerValueToText_)
                return outerValueToText_(v);
            if (channel == 1 && middleValueToText_)
                return middleValueToText_(v);
            if (channel == 2 && innerValueToText_)
                return innerValueToText_(v);
            return juce::String(v, 2);
        }
        return "--";
    }

    void TripleGlowKnob::refreshModRingState(ModRingState& state, const juce::String& paramId)
    {
        juce::Colour next = juce::Colours::transparentBlack;
        auto nextSource = modulation::ModSource::None;
        float nextAmountNorm = 0.0f;
        float nextLiveMod = -1.0f;
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

            if (modProcessor_->hasModRouteTo(state.destination, state.targetIndex))
            {
                const auto sources = modProcessor_->buildModPreviewSources(modProcessor_->getHostBpm());
                const auto modOut =
                    modulation::ModMatrixExecutor::apply(modProcessor_->getCurrentPatch().layerA.modRoutes, sources);
                const float offset =
                    modOffsetForDestination(modOut, state.destination, state.targetIndex);
                if (auto* param = modProcessor_->apvts.getParameter(paramId))
                {
                    float base = 0.0f;
                    if (auto* raw = modProcessor_->apvts.getRawParameterValue(paramId))
                        base = raw->load();
                    nextLiveMod = normalizedModulatedValue(param, base, state.destination, offset);
                }
            }
        }
        if (next != state.colour || nextAmountNorm != state.amountNormalized || nextSource != state.source ||
            nextLiveMod != state.liveModNormalized)
        {
            state.colour = next;
            state.source = nextSource;
            state.amountNormalized = nextAmountNorm;
            state.liveModNormalized = nextLiveMod;
            repaint();
        }
    }

    void TripleGlowKnob::timerCallback()
    {
        refreshModRingState(outerMod_, outerParamId_);
        refreshModRingState(middleMod_, middleParamId_);
        refreshModRingState(innerMod_, innerParamId_);
    }

    void TripleGlowKnob::handleModMouseDown(const juce::MouseEvent& event, ModRingState& state)
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
        valueDragActive_ = false;
        repaint();
    }

    void TripleGlowKnob::handleModMouseDrag(const juce::MouseEvent& event, ModRingState& state)
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
        const juce::String& paramId =
            &state == &outerMod_ ? outerParamId_ : (&state == &middleMod_ ? middleParamId_ : innerParamId_);
        refreshModRingState(state, paramId);
    }

    void TripleGlowKnob::adjustFocusedChannel(float deltaY)
    {
        const juce::String& id =
            focusedChannel_ == 0 ? outerParamId_ : (focusedChannel_ == 1 ? middleParamId_ : innerParamId_);
        if (auto* param = apvts_.getParameter(id))
        {
            const float deltaNorm = (-deltaY / 140.0f);
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, dragStartValue_ + deltaNorm));
        }
        repaint();
    }

    void TripleGlowKnob::mouseDown(const juce::MouseEvent& event)
    {
        auto& modState = modStateAtPoint(event.getPosition());

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

        const int hit = radialglow::hitTestTripleRingChannel(event.position, dialBounds());
        if (hit >= 0)
            focusedChannel_ = hit;
        else
        {
            const float pillTop = static_cast<float>(getHeight()) - juce::jmax(18.0f, getHeight() * 0.18f);
            if (event.position.y >= pillTop)
            {
                const float pillW = getWidth() / 3.2f;
                focusedChannel_ = juce::jlimit(0, 2, static_cast<int>(event.position.x / pillW));
            }
        }

        handleModMouseDown(event, modStateForChannel(focusedChannel_));

        if (modAssignment_ != nullptr && modAssignment_->isArmed())
            return;

        const juce::String& id =
            focusedChannel_ == 0 ? outerParamId_ : (focusedChannel_ == 1 ? middleParamId_ : innerParamId_);
        if (auto* param = apvts_.getParameter(id))
        {
            dragStartValue_ = param->getValue();
            valueDragActive_ = true;
        }

        repaint();
    }

    void TripleGlowKnob::mouseDrag(const juce::MouseEvent& event)
    {
        auto& modState = modStateAtPoint(event.getPosition());
        if (modState.depthDragActive)
        {
            handleModMouseDrag(event, modState);
            return;
        }

        if (!valueDragActive_)
            return;

        adjustFocusedChannel(static_cast<float>(event.getDistanceFromDragStartY()));
    }

    void TripleGlowKnob::mouseUp(const juce::MouseEvent&)
    {
        outerMod_.depthDragActive = false;
        middleMod_.depthDragActive = false;
        innerMod_.depthDragActive = false;
        valueDragActive_ = false;
    }

    void TripleGlowKnob::mouseDoubleClick(const juce::MouseEvent& event)
    {
        juce::ignoreUnused(event);
        setExpandedReadout(!expandedReadout_);
    }

    bool TripleGlowKnob::isInterestedInDragSource(const SourceDetails& details)
    {
        return (outerMod_.destination != modulation::ModDestination::None ||
                middleMod_.destination != modulation::ModDestination::None ||
                innerMod_.destination != modulation::ModDestination::None) &&
               parseModSourceDragDescription(details.description.toString()).has_value();
    }

    void TripleGlowKnob::itemDragEnter(const SourceDetails& details)
    {
        auto& modState = modStateAtPoint(details.localPosition.toInt());
        modState.dragHover = true;
        repaint();
    }

    void TripleGlowKnob::itemDragExit(const SourceDetails&)
    {
        outerMod_.dragHover = false;
        middleMod_.dragHover = false;
        innerMod_.dragHover = false;
        repaint();
    }

    void TripleGlowKnob::itemDropped(const SourceDetails& details)
    {
        outerMod_.dragHover = false;
        middleMod_.dragHover = false;
        innerMod_.dragHover = false;
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

    void TripleGlowKnob::paintModRing(juce::Graphics& g, const ModRingState& state, float orbitRadius,
                                      float orbitScale) const
    {
        if (state.colour.isTransparent() && !state.dragHover)
            return;

        const auto bounds = dialBounds();
        const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const float dialRadius = diameter * 0.5f;
        const float modGap = juce::jmax(3.5f, dialRadius * 0.07f * orbitScale);
        const float modRingRadius = orbitRadius + modGap;
        const float modStrokeWidth = juce::jmax(3.2f, dialRadius * 0.068f * orbitScale);

        knobrings::Layout ringLayout{bounds.getCentre(), dialRadius, orbitRadius, modRingRadius, modStrokeWidth};

        if (state.dragHover)
            knobrings::drawDragHoverRing(g, ringLayout);

        if (!state.colour.isTransparent())
            knobrings::strokeModRouteArc(g, ringLayout, state.amountNormalized, state.colour, orbitScale);

        if (state.liveModNormalized >= 0.0f)
        {
            const auto ghostAccent = state.colour.isTransparent() ? palette::kAccent : state.colour;
            knobrings::drawLiveModGhost(g, ringLayout, state.liveModNormalized, ghostAccent);
        }
    }

    void TripleGlowKnob::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const auto dial = dialBounds();

        radialglow::drawTripleRadialFrame(g, dial, readProportional(0), readProportional(1), readProportional(2),
                                          focusedChannel_, expandedReadout_, formatValue(0), formatValue(1),
                                          formatValue(2), outerLabel_, middleLabel_, innerLabel_);

        auto pillRow = bounds.removeFromBottom(juce::jmax(18.0f, bounds.getHeight() * 0.18f));
        const float pillW = pillRow.getWidth() / 3.2f;
        const juce::String* labels[] = {&outerLabel_, &middleLabel_, &innerLabel_};

        for (int i = 0; i < 3; ++i)
        {
            const juce::Colour colour = channelAccent(i);
            auto pill = pillRow.removeFromLeft(pillW).reduced(2.0f);
            const bool focused = focusedChannel_ == i;
            g.setColour(focused ? colour.withAlpha(0.08f) : palette::kBackgroundTop);
            g.fillRoundedRectangle(pill, 6.0f);
            g.setColour(focused ? colour : palette::kBorder);
            g.drawRoundedRectangle(pill, 6.0f, focused ? 1.2f : 0.8f);

            const auto dot = pill.removeFromLeft(14.0f).withSizeKeepingCentre(6.0f, 6.0f);
            g.setColour(colour.withAlpha(focused ? 1.0f : 0.55f));
            g.fillEllipse(dot);

            g.setFont(fonts::label(8.0f));
            g.setColour(focused ? palette::kTextPrimary : palette::kTextDim);
            g.drawText(labels[i]->toUpperCase(), pill.removeFromLeft(pill.getWidth() * 0.42f),
                       juce::Justification::centredLeft);

            g.setFont(fonts::value(8.0f));
            g.setColour(colour.withAlpha(focused ? 1.0f : 0.75f));
            g.drawText(formatValue(i), pill, juce::Justification::centredRight);
        }
    }

    void TripleGlowKnob::paintOverChildren(juce::Graphics& g)
    {
        const auto layout = radialglow::computeTripleLayout(dialBounds());

        paintModRing(g, outerMod_, layout.outerRingRadius, 1.15f);
        paintModRing(g, middleMod_, layout.middleRingRadius, 1.0f);
        paintModRing(g, innerMod_, layout.innerRingRadius, 0.9f);

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
        drawDepth(outerMod_);
        drawDepth(middleMod_);
        drawDepth(innerMod_);
    }

    void TripleGlowKnob::resized()
    {
        auto bounds = getLocalBounds();
        if (outerMod_.showDepthPopover || middleMod_.showDepthPopover || innerMod_.showDepthPopover)
        {
            auto popoverArea = bounds.removeFromBottom(kDepthPopoverHeight).reduced(8, 2);
            outerMod_.depthPopoverArea_ = popoverArea;
            middleMod_.depthPopoverArea_ = popoverArea;
            innerMod_.depthPopoverArea_ = popoverArea;
        }
        repaint();
    }

} // namespace pw8::plugin::ui
