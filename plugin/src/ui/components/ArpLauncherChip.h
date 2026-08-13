#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowRingButton.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Compact ARP launcher: on/off toggle, rate readout, opens the arp drawer on click.
    class ArpLauncherChip : public juce::Component, private juce::Timer
    {
    public:
        explicit ArpLauncherChip(PatchworkEightProcessor& processor);
        ~ArpLauncherChip() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent& event) override;

        std::function<void()> onOpenDrawer;

    private:
        void timerCallback() override;
        [[nodiscard]] juce::String rateReadout() const;

        PatchworkEightProcessor& processor_;
        GlowRingButton enableButton_{"ARP"};
        juce::Label rateLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment_;
        bool pulseOn_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpLauncherChip)
    };

} // namespace pw8::plugin::ui
