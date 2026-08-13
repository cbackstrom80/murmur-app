#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/ScopeViewMode.h"

namespace pw8::plugin::ui
{
    /// Recessed FFT | VU pill for spectrum scopes — overlays the scope corner.
    class ScopeModeToggle : public juce::Component
    {
    public:
        ScopeModeToggle();

        void setMode(ScopeViewMode mode);
        [[nodiscard]] ScopeViewMode getMode() const noexcept { return mode_; }

        std::function<void(ScopeViewMode)> onModeChanged;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        [[nodiscard]] juce::Rectangle<float> segmentBounds(int index) const;
        [[nodiscard]] int segmentAt(juce::Point<float> pos) const;

        ScopeViewMode mode_ = ScopeViewMode::Fft;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopeModeToggle)
    };

} // namespace pw8::plugin::ui
