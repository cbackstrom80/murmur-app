#pragma once

#include <array>
#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../PlayModeLayout.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-fx-card-browser` (152:4) — 2×5 card grid for design FX landing.
    class DesignFxCardBrowser : public juce::Component
    {
    public:
        explicit DesignFxCardBrowser(juce::AudioProcessorValueTreeState& apvts);

        std::function<void(int cardIndex)> onCardSelected;
        std::function<void(int cardIndex)> onCardToggle;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void resized() override;

        [[nodiscard]] int chipIndexForCard(int cardIndex) const noexcept;
        [[nodiscard]] bool isQuasarCard(int cardIndex) const noexcept;

    private:
        struct CardDef;

        [[nodiscard]] juce::Rectangle<int> cardBounds(int cardIndex) const;
        [[nodiscard]] juce::String cardParamPrefix(int cardIndex) const;
        [[nodiscard]] bool isCardEnabled(int cardIndex) const;
        void toggleCardEnabled(int cardIndex);

        void paintCard(juce::Graphics& g, int cardIndex, juce::Rectangle<int> bounds, bool enabled);

        juce::AudioProcessorValueTreeState& apvts_;
        std::array<juce::Rectangle<int>, layout::kDesignFxCardBrowserCount> toggleBounds_{};
    };

} // namespace pw8::plugin::ui
