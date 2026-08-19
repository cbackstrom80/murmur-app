#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "AudioVisualizerBus.h"
#include "MurmurVisualizerComponent.h"

namespace murmur8
{

/** Embeds a MurmurVisualizerComponent into any parent; call from panel code that has the bus. */
class GlVisualizerHost
{
public:
    void attach(juce::Component& parent, AudioVisualizerBus& bus)
    {
        if (plot_ != nullptr)
            return;

        bus_ = &bus;
        plot_ = std::make_unique<MurmurVisualizerComponent>(bus);
        parent.addAndMakeVisible(*plot_);
    }

    [[nodiscard]] bool isAttached() const noexcept { return plot_ != nullptr; }

    MurmurVisualizerComponent& plot()
    {
        jassert(plot_ != nullptr);
        return *plot_;
    }

    void setBounds(juce::Rectangle<int> bounds)
    {
        if (plot_ != nullptr)
            plot_->setBounds(bounds);
    }

    void setVisible(bool visible)
    {
        if (plot_ != nullptr)
            plot_->setVisible(visible);
    }

private:
    AudioVisualizerBus* bus_ = nullptr;
    std::unique_ptr<MurmurVisualizerComponent> plot_;
};

} // namespace murmur8
