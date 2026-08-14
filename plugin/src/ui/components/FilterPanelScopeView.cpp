#include "FilterPanelScopeView.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    FilterPanelScopeView::FilterPanelScopeView(PatchworkEightProcessor& processor)
        : processor_(processor), oscilloscope_(processor), spectrum_(processor)
    {
        addChildComponent(oscilloscope_);
        addChildComponent(spectrum_);
        setDisplayMode(FilterScopeDisplayMode::Waveform);
    }

    void FilterPanelScopeView::setDisplayMode(FilterScopeDisplayMode mode)
    {
        mode_ = mode;
        const bool waveform = mode_ == FilterScopeDisplayMode::Waveform;
        oscilloscope_.setVisible(waveform);
        spectrum_.setVisible(!waveform);
        resized();
        repaint();
    }

    juce::Rectangle<float> FilterPanelScopeView::toggleBounds() const
    {
        const auto bounds = getLocalBounds().toFloat();
        return juce::Rectangle<float>(bounds.getRight() - 72.0f, bounds.getY() + 4.0f, 68.0f, 16.0f);
    }

    int FilterPanelScopeView::toggleSegmentAt(juce::Point<float> pos) const
    {
        const auto toggle = toggleBounds();
        if (!toggle.contains(pos))
            return -1;
        return pos.x >= toggle.getCentreX() ? 1 : 0;
    }

    void FilterPanelScopeView::resized()
    {
        auto plot = getLocalBounds();
        plot.removeFromTop(20);
        oscilloscope_.setBounds(plot);
        spectrum_.setBounds(plot);
    }

    void FilterPanelScopeView::paint(juce::Graphics& g)
    {
        const auto toggle = toggleBounds();
        draw::fillRecessedRoundedRect(g, toggle, toggle.getHeight() * 0.5f);

        static constexpr const char* kLabels[] = {"WAVE", "FFT"};
        for (int i = 0; i < 2; ++i)
        {
            const float width = toggle.getWidth() * 0.5f;
            const auto seg = toggle.withWidth(width).withX(toggle.getX() + static_cast<float>(i) * width);
            const bool selected =
                (i == 0 && mode_ == FilterScopeDisplayMode::Waveform) ||
                (i == 1 && mode_ == FilterScopeDisplayMode::Spectrum);

            if (selected)
            {
                g.setColour(palette::kAccent.withAlpha(0.16f));
                g.fillRoundedRectangle(seg.reduced(1.0f), seg.getHeight() * 0.45f);
            }

            g.setFont(fonts::label(fonts::kCaptionSize));
            g.setColour(selected ? palette::kTextPrimary : palette::kTextSecondary);
            g.drawText(kLabels[i], seg, juce::Justification::centred, false);
        }

        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawVerticalLine(static_cast<int>(toggle.getCentreX()), toggle.getY() + 2.0f, toggle.getBottom() - 2.0f);
    }

    void FilterPanelScopeView::mouseDown(const juce::MouseEvent& event)
    {
        const int seg = toggleSegmentAt(event.position);
        if (seg < 0)
            return;

        setDisplayMode(seg == 0 ? FilterScopeDisplayMode::Waveform : FilterScopeDisplayMode::Spectrum);
    }

} // namespace pw8::plugin::ui
