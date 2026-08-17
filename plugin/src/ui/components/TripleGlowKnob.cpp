#include "TripleGlowKnob.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../theme/RadialGlowDraw.h"

namespace pw8::plugin::ui
{
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
        const auto dialBounds = getLocalBounds().toFloat().withSizeKeepingCentre(
            static_cast<float>(maxDialDiameter_), static_cast<float>(maxDialDiameter_));
        const int hit = radialglow::hitTestTripleRingChannel(event.position, dialBounds);
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

        const juce::String& id =
            focusedChannel_ == 0 ? outerParamId_ : (focusedChannel_ == 1 ? middleParamId_ : innerParamId_);
        if (auto* param = apvts_.getParameter(id))
            dragStartValue_ = param->getValue();

        repaint();
    }

    void TripleGlowKnob::mouseDrag(const juce::MouseEvent& event)
    {
        juce::ignoreUnused(event);
        adjustFocusedChannel(static_cast<float>(event.getDistanceFromDragStartY()));
    }

    void TripleGlowKnob::mouseUp(const juce::MouseEvent&) {}

    void TripleGlowKnob::mouseDoubleClick(const juce::MouseEvent& event)
    {
        juce::ignoreUnused(event);
        setExpandedReadout(!expandedReadout_);
    }

    void TripleGlowKnob::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        auto dialBounds = bounds.withSizeKeepingCentre(static_cast<float>(maxDialDiameter_),
                                                       static_cast<float>(maxDialDiameter_));

        radialglow::drawTripleRadialFrame(g, dialBounds, readProportional(0), readProportional(1), readProportional(2),
                                          focusedChannel_, expandedReadout_, formatValue(0), formatValue(1),
                                          formatValue(2), outerLabel_, middleLabel_, innerLabel_);

        auto pillRow = bounds.removeFromBottom(juce::jmax(18.0f, bounds.getHeight() * 0.18f));
        const float pillW = pillRow.getWidth() / 3.2f;
        const juce::Colour colours[] = {juce::Colour(0xff00ffd2), juce::Colour(0xff9f80ff), juce::Colour(0xffff9d00)};
        const juce::String* labels[] = {&outerLabel_, &middleLabel_, &innerLabel_};

        for (int i = 0; i < 3; ++i)
        {
            auto pill = pillRow.removeFromLeft(pillW).reduced(2.0f);
            const bool focused = focusedChannel_ == i;
            g.setColour(focused ? colours[i].withAlpha(0.08f) : palette::kBackgroundTop);
            g.fillRoundedRectangle(pill, 6.0f);
            g.setColour(focused ? colours[i] : palette::kBorder);
            g.drawRoundedRectangle(pill, 6.0f, focused ? 1.2f : 0.8f);

            const auto dot = pill.removeFromLeft(14.0f).withSizeKeepingCentre(6.0f, 6.0f);
            g.setColour(colours[i].withAlpha(focused ? 1.0f : 0.55f));
            g.fillEllipse(dot);

            g.setFont(fonts::label(8.0f));
            g.setColour(focused ? palette::kTextPrimary : palette::kTextDim);
            g.drawText(labels[i]->toUpperCase(), pill.removeFromLeft(pill.getWidth() * 0.42f), juce::Justification::centredLeft);

            g.setFont(fonts::value(8.0f));
            g.setColour(colours[i].withAlpha(focused ? 1.0f : 0.75f));
            g.drawText(formatValue(i), pill, juce::Justification::centredRight);
        }
    }

    void TripleGlowKnob::resized() { repaint(); }

} // namespace pw8::plugin::ui
