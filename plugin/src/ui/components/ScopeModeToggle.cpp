#include "ScopeModeToggle.h"

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    ScopeModeToggle::ScopeModeToggle()
    {
        setInterceptsMouseClicks(true, false);
    }

    void ScopeModeToggle::setMode(ScopeViewMode mode)
    {
        if (mode_ == mode)
            return;

        mode_ = mode;
        repaint();
    }

    juce::Rectangle<float> ScopeModeToggle::segmentBounds(int index) const
    {
        const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const float width = bounds.getWidth() * 0.5f;
        return bounds.withWidth(width).withX(bounds.getX() + static_cast<float>(index) * width);
    }

    int ScopeModeToggle::segmentAt(juce::Point<float> pos) const
    {
        return pos.x >= getWidth() * 0.5f ? 1 : 0;
    }

    void ScopeModeToggle::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        if (bounds.isEmpty())
            return;

        draw::fillRecessedRoundedRect(g, bounds, bounds.getHeight() * 0.5f);

        static constexpr const char* kLabels[] = {"FFT", "VU"};
        const auto accent = branding::glowColour();

        for (int i = 0; i < 2; ++i)
        {
            const auto seg = segmentBounds(i);
            const bool selected = (i == 0 && mode_ == ScopeViewMode::Fft) || (i == 1 && mode_ == ScopeViewMode::Vu);

            if (selected)
            {
                g.setColour(accent.withAlpha(0.16f));
                g.fillRoundedRectangle(seg.reduced(1.0f), seg.getHeight() * 0.45f);

                auto outline = draw::roundedRectPath(seg.reduced(1.0f), seg.getHeight() * 0.45f);
                draw::strokeGlowPath(g, outline, 0.85f, 1.2f, true);
            }

            g.setFont(fonts::label(fonts::kCaptionSize));
            g.setColour(selected ? palette::kTextPrimary : palette::kTextSecondary);
            g.drawText(kLabels[i], seg, juce::Justification::centred, false);
        }

        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawVerticalLine(getWidth() / 2, bounds.getY() + 2.0f, bounds.getBottom() - 2.0f);
    }

    void ScopeModeToggle::mouseDown(const juce::MouseEvent& event)
    {
        const int seg = segmentAt(event.position);
        const auto next = seg == 0 ? ScopeViewMode::Fft : ScopeViewMode::Vu;
        if (next == mode_)
            return;

        mode_ = next;
        repaint();

        if (onModeChanged)
            onModeChanged(mode_);
    }

} // namespace pw8::plugin::ui
