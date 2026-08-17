#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    enum class LabLauncherIcon
    {
        None,
        Vocoder,
        Lfo,
        Mod,
    };

    /// Figma obsidian-play-board launcher chip (VOCODER / LFO 1·2 / MOD).
    class LabLauncherChip : public juce::Component
    {
    public:
        LabLauncherChip();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseUp(const juce::MouseEvent& event) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;

        void setLabel(const juce::String& label);
        void setAccentColour(juce::Colour colour);
        void setHighlighted(bool highlighted);
        void setIcon(LabLauncherIcon icon);

        [[nodiscard]] static juce::Rectangle<int> getPreferredSize() noexcept;

        std::function<void()> onClick;

    private:
        void drawMiniIcon(juce::Graphics& g, juce::Rectangle<float> area) const;

        juce::String label_;
        juce::Colour accent_{0xff00ffd2};
        bool highlighted_ = false;
        bool hovered_ = false;
        LabLauncherIcon icon_ = LabLauncherIcon::None;
    };

} // namespace pw8::plugin::ui
