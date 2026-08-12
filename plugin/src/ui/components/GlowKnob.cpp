#include "GlowKnob.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    namespace
    {
        /// The modulation depth a fresh drop is assigned -- destination-specific
        /// since the two real PLAY-mode destinations have very different natural
        /// units (see ModMatrixExecutor.hpp's destination-semantics doc comment).
        /// Deliberately generous enough to be obviously audible on drop (the whole
        /// point of drag-to-modulate is instant, undeniable feedback), not tuned to
        /// taste -- adjusting the exact depth afterward is the mod-matrix's job
        /// once it exists (docs/UI.md "What's PLANNED"), not this gesture's.
        float defaultModAmountFor(modulation::ModDestination destination) noexcept
        {
            switch (destination)
            {
                case modulation::ModDestination::FilterCutoff: return 36.0f;
                case modulation::ModDestination::FilterResonance: return 0.4f;
                case modulation::ModDestination::OperatorFilterCutoff: return 36.0f;
                case modulation::ModDestination::OperatorFilterResonance: return 0.4f;
                default: return 0.0f;
            }
        }
    } // namespace

    juce::String GlowKnob::FormattedSlider::getTextFromValue(double value)
    {
        if (valueToText)
            return valueToText(static_cast<float>(value));
        // Deliberately not delegating to juce::Slider::getTextFromValue() here --
        // its default decimal-place count is derived from the parameter's step
        // interval in a way that's easy to fight with (setNumDecimalPlacesToDisplay
        // needs to survive SliderAttachment's own range setup), and this is simple
        // enough to just own directly: fixed 2-decimal display for every continuous
        // (non-formatter) knob.
        return juce::String(value, 2);
    }

    GlowKnob::GlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                        const juce::String& name, std::function<juce::String(float)> valueToText,
                        juce::Colour accentColour)
    {
        slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider_.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f,
                                     true);
        slider_.valueToText = std::move(valueToText);
        slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 76, 16);
        slider_.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        slider_.setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        slider_.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        if (!accentColour.isTransparent())
            slider_.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        addAndMakeVisible(slider_);
        slider_.addMouseListener(this, false); // so this->mouseDown() also sees right-clicks landing on the knob.

        nameLabel_.setText(name.toUpperCase(), juce::dontSendNotification);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        nameLabel_.setFont(fonts::label(10.5f));
        addAndMakeVisible(nameLabel_);

        attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId,
                                                                                               slider_);
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
        startTimerHz(8);
        timerCallback();
    }

    void GlowKnob::timerCallback()
    {
        juce::Colour next = juce::Colours::transparentBlack;
        auto nextSource = modulation::ModSource::None;
        if (modProcessor_ != nullptr)
        {
            for (const auto& route : modProcessor_->getCurrentPatch().layerA.modRoutes)
            {
                if (route.isActive() && route.destination == modDestination_ && route.targetIndex == modTargetIndex_)
                {
                    next = palette::modSourceColour(static_cast<int>(route.source));
                    nextSource = route.source;
                    break;
                }
            }
        }
        if (next != ringColour_)
        {
            ringColour_ = next;
            ringSource_ = nextSource;
            repaint();
        }
    }

    void GlowKnob::mouseDown(const juce::MouseEvent& event)
    {
        if (modDestination_ == modulation::ModDestination::None || modProcessor_ == nullptr)
            return;
        if (!event.mods.isRightButtonDown())
            return;
        if (ringColour_.isTransparent())
            return; // Nothing assigned -- nothing to remove.

        modProcessor_->removeModRouteLive(ringSource_, modDestination_, modTargetIndex_);
        ringColour_ = juce::Colours::transparentBlack;
        ringSource_ = modulation::ModSource::None;
        repaint();
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
            modProcessor_->setOrReplaceModRouteLive(*source, modDestination_, modTargetIndex_,
                                                      defaultModAmountFor(modDestination_));
            ringColour_ = palette::modSourceColour(static_cast<int>(*source)); // Immediate, don't wait for the next poll.
        }
        repaint();
    }

    void GlowKnob::resized()
    {
        auto bounds = getLocalBounds();
        nameLabel_.setBounds(bounds.removeFromBottom(14));
        slider_.setBounds(bounds);
    }

    void GlowKnob::paintOverChildren(juce::Graphics& g)
    {
        if (ringColour_.isTransparent() && !dragHover_)
            return;

        // Matches ObsidianLookAndFeel::drawRotarySlider's own geometry (bounds
        // reduced by 4, diameter = min(w,h)) so this ring sits flush around the
        // knob body it belongs to regardless of this GlowKnob's exact cell size.
        auto sliderBounds = slider_.getBounds().toFloat().reduced(4.0f);
        const float diameter = juce::jmax(16.0f, juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight()));
        const auto knobBounds =
            slider_.getBounds().toFloat().withSizeKeepingCentre(diameter, diameter).expanded(3.5f);

        if (dragHover_)
        {
            g.setColour(palette::kAccent.withAlpha(0.7f));
            g.drawEllipse(knobBounds.expanded(2.0f), 1.6f);
        }
        if (!ringColour_.isTransparent())
        {
            g.setColour(ringColour_.withAlpha(0.85f));
            g.drawEllipse(knobBounds, 2.0f);
        }
    }

} // namespace pw8::plugin::ui
