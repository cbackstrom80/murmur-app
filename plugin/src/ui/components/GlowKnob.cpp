#include "GlowKnob.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../theme/ObsidianRotary.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNameLabelHeight = 14;
        constexpr int kTextBoxHeight = 16;
        constexpr int kDepthPopoverHeight = 22;

    } // namespace

    juce::String GlowKnob::FormattedSlider::getTextFromValue(double value)
    {
        if (valueToText)
            return valueToText(static_cast<float>(value));
        return juce::String(value, 2);
    }

    GlowKnob::GlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                        const juce::String& name, std::function<juce::String(float)> valueToText,
                        juce::Colour accentColour)
    {
        slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider_.setRotaryParameters(rotary::kStartAngle, rotary::kEndAngle, true);
        slider_.valueToText = std::move(valueToText);
        slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 76, 16);
        slider_.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        slider_.setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        slider_.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        if (!accentColour.isTransparent())
            slider_.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        addAndMakeVisible(slider_);
        slider_.addMouseListener(this, false);

        nameLabel_.setText(name.toUpperCase(), juce::dontSendNotification);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        nameLabel_.setFont(fonts::label(11.0f));
        addAndMakeVisible(nameLabel_);

        attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId,
                                                                                               slider_);
        slider_.getProperties().set("maxDialDiameter", maxDialDiameter_);
    }

    GlowKnob::~GlowKnob()
    {
        stopTimer();
    }

    void GlowKnob::enableModulationTarget(PatchworkEightProcessor& processor, modulation::ModDestination destination,
                                           std::uint8_t targetIndex)
    {
        modProcessor_ = &processor;
        modDestination_ = destination;
        modTargetIndex_ = targetIndex;
        setHelpText("Click after selecting a mod source, drag a source chip here, or right-click to remove.");
        startTimerHz(8);
        timerCallback();
    }

    void GlowKnob::setModAssignmentController(ModAssignmentController* controller)
    {
        modAssignment_ = controller;
    }

    void GlowKnob::setMaxDialDiameter(int diameter)
    {
        maxDialDiameter_ = diameter;
        slider_.getProperties().set("maxDialDiameter", diameter);
        resized();
    }

    void GlowKnob::setDeckedStyle(bool enabled, DeckedKnobSize size)
    {
        deckedStyle_ = enabled;
        deckedSize_ = size;
        if (enabled)
        {
            slider_.getProperties().set("knobStyle", "decked");
            juce::String sizeProp;
            switch (size)
            {
                case DeckedKnobSize::Large: sizeProp = "large"; break;
                case DeckedKnobSize::Small: sizeProp = "small"; break;
                default: sizeProp = "medium"; break;
            }
            slider_.getProperties().set("deckedSize", sizeProp);
        }
        else
        {
            slider_.getProperties().remove("knobStyle");
            slider_.getProperties().remove("deckedSize");
        }
        slider_.repaint();
    }

    void GlowKnob::setFeaturedPerformanceMacro(bool featured)
    {
        slider_.getProperties().set("featuredKoin", featured);
        slider_.repaint();
    }

    void GlowKnob::setHeaderCompactMode(bool compact)
    {
        headerCompactMode_ = compact;
        if (compact)
        {
            maxDialDiameter_ = 40;
            slider_.getProperties().set("maxDialDiameter", maxDialDiameter_);
            nameLabel_.setFont(fonts::label(fonts::kCaptionSize));
        }
        resized();
    }

    void GlowKnob::timerCallback()
    {
        juce::Colour next = juce::Colours::transparentBlack;
        auto nextSource = modulation::ModSource::None;
        float nextAmountNorm = 0.0f;
        if (modProcessor_ != nullptr)
        {
            for (const auto& route : modProcessor_->getCurrentPatch().layerA.modRoutes)
            {
                if (route.isActive() && route.destination == modDestination_ && route.targetIndex == modTargetIndex_)
                {
                    next = palette::modSourceColour(static_cast<int>(route.source));
                    nextSource = route.source;
                    const auto range = modAmountRangeFor(route.destination);
                    const float def = defaultModAmountFor(route.destination);
                    const float span = juce::jmax(0.001f, range.max - range.min);
                    nextAmountNorm = juce::jlimit(0.0f, 1.0f, std::abs(route.amount) / juce::jmax(0.001f, std::abs(def)));
                    break;
                }
            }
        }
        if (next != ringColour_ || nextAmountNorm != ringAmountNormalized_ || nextSource != ringSource_)
        {
            ringColour_ = next;
            ringSource_ = nextSource;
            ringAmountNormalized_ = nextAmountNorm;
            repaint();
        }
    }

    void GlowKnob::mouseDown(const juce::MouseEvent& event)
    {
        if (showDepthPopover_ && depthPopoverArea_.contains(event.getPosition()))
        {
            depthDragActive_ = true;
            depthDragStartX_ = static_cast<float>(event.position.x);
            for (const auto& route : modProcessor_->getCurrentPatch().layerA.modRoutes)
            {
                if (route.isActive() && route.destination == modDestination_ && route.targetIndex == modTargetIndex_)
                {
                    depthDragStartAmount_ = route.amount;
                    break;
                }
            }
            return;
        }

        if (modDestination_ == modulation::ModDestination::None || modProcessor_ == nullptr)
            return;

        if (event.mods.isLeftButtonDown() && modAssignment_ != nullptr && modAssignment_->isArmed())
        {
            const auto source = modAssignment_->armedSource();
            if (source.has_value())
            {
                assignModRoute(*modProcessor_, *source, modDestination_, modTargetIndex_);
                ringColour_ = palette::modSourceColour(static_cast<int>(*source));
                ringSource_ = *source;
                ringAmountNormalized_ = 1.0f;
                modAssignment_->disarm();
                showDepthPopover_ = true;
                repaint();
            }
            return;
        }

        if (!event.mods.isRightButtonDown())
            return;
        if (ringColour_.isTransparent())
            return;

        modProcessor_->removeModRouteLive(ringSource_, modDestination_, modTargetIndex_);
        ringColour_ = juce::Colours::transparentBlack;
        ringSource_ = modulation::ModSource::None;
        ringAmountNormalized_ = 0.0f;
        showDepthPopover_ = false;
        repaint();
    }

    void GlowKnob::mouseDrag(const juce::MouseEvent& event)
    {
        if (!depthDragActive_ || modProcessor_ == nullptr)
            return;

        const auto range = modAmountRangeFor(modDestination_);
        const float span = range.max - range.min;
        const float delta = (static_cast<float>(event.position.x) - depthDragStartX_) * (span / 100.0f);
        for (const auto& route : modProcessor_->getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive() && route.destination == modDestination_ && route.targetIndex == modTargetIndex_)
            {
                updateModRouteAmount(*modProcessor_, route, depthDragStartAmount_ + delta);
                break;
            }
        }
        timerCallback();
    }

    void GlowKnob::mouseUp(const juce::MouseEvent&)
    {
        depthDragActive_ = false;
    }

    bool GlowKnob::isInterestedInDragSource(const SourceDetails& details)
    {
        return modDestination_ != modulation::ModDestination::None &&
               parseModSourceDragDescription(details.description.toString()).has_value();
    }

    void GlowKnob::itemDragEnter(const SourceDetails&)
    {
        dragHover_ = true;
        repaint();
    }

    void GlowKnob::itemDragExit(const SourceDetails&)
    {
        dragHover_ = false;
        repaint();
    }

    void GlowKnob::itemDropped(const SourceDetails& details)
    {
        dragHover_ = false;
        const auto source = parseModSourceDragDescription(details.description.toString());
        if (source.has_value() && modProcessor_ != nullptr)
        {
            assignModRoute(*modProcessor_, *source, modDestination_, modTargetIndex_);
            ringColour_ = palette::modSourceColour(static_cast<int>(*source));
            ringSource_ = *source;
            ringAmountNormalized_ = 1.0f;
            showDepthPopover_ = true;
            if (modAssignment_ != nullptr)
                modAssignment_->disarm();
        }
        repaint();
    }

    void GlowKnob::resized()
    {
        auto bounds = getLocalBounds().reduced(headerCompactMode_ ? 1 : 4);
        const int nameHeight = headerCompactMode_ ? 11 : kNameLabelHeight;
        nameLabel_.setBounds(bounds.removeFromBottom(nameHeight));

        if (showDepthPopover_ && modProcessor_ != nullptr)
            depthPopoverArea_ = bounds.removeFromBottom(kDepthPopoverHeight).reduced(8, 2);

        if (headerCompactMode_)
        {
            slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            const int dialDiameter =
                juce::jmin(maxDialDiameter_, bounds.getWidth(), juce::jmax(28, bounds.getHeight()));
            slider_.setBounds(bounds.withSizeKeepingCentre(bounds.getWidth(), dialDiameter));
            return;
        }

        const int dialDiameter = juce::jmin(maxDialDiameter_, bounds.getWidth(),
                                           juce::jmax(32, bounds.getHeight() - kTextBoxHeight));
        const int textBoxWidth = juce::jmin(72, juce::jmax(44, bounds.getWidth() - 4));
        slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, kTextBoxHeight);
        slider_.setBounds(bounds.withSizeKeepingCentre(bounds.getWidth(), dialDiameter + kTextBoxHeight));
    }

    void GlowKnob::paintOverChildren(juce::Graphics& g)
    {
        if (showDepthPopover_ && !depthPopoverArea_.isEmpty())
        {
            auto pop = depthPopoverArea_.toFloat();
            g.setColour(palette::kPanelRaised);
            g.fillRoundedRectangle(pop, 3.0f);
            auto fill = pop.reduced(2.0f);
            fill.setWidth(fill.getWidth() * ringAmountNormalized_);
            g.setColour(ringColour_.isTransparent() ? palette::kAccent : ringColour_.withAlpha(0.75f));
            g.fillRoundedRectangle(fill, 2.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(fonts::kCaptionSize));
            g.drawText("DEPTH", depthPopoverArea_, juce::Justification::centred);
        }

        if (ringColour_.isTransparent() && !dragHover_)
            return;

        auto sliderBounds = slider_.getBounds().toFloat().reduced(4.0f);
        const float diameter = juce::jmax(16.0f, juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight()));
        const auto knobBounds =
            slider_.getBounds().toFloat().withSizeKeepingCentre(diameter, diameter).expanded(3.5f);

        if (dragHover_)
        {
            g.setColour(palette::kMurmurViolet.withAlpha(0.7f));
            g.drawEllipse(knobBounds.expanded(2.0f), 1.6f);
        }
        if (!ringColour_.isTransparent())
        {
            const float endAngle = rotary::kStartAngle + rotary::kSweep * juce::jlimit(0.0f, 1.0f, ringAmountNormalized_);
            juce::Path arc;
            arc.addCentredArc(knobBounds.getCentreX(), knobBounds.getCentreY(), knobBounds.getWidth() * 0.5f,
                              knobBounds.getHeight() * 0.5f, 0.0f, rotary::kStartAngle, endAngle, true);
            g.setColour(ringColour_.withAlpha(0.85f));
            g.strokePath(arc, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

} // namespace pw8::plugin::ui
