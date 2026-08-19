#include "PreviewSurface.h"

#include "VisualTheme.h"

namespace pw8::plugin::ui::preview
{
    void PreviewSurface::ensureThemeCurrent() noexcept
    {
        const auto current = visualCacheThemeKey();
        if (themeKey_ != 0 && themeKey_ != current)
            invalidateAll();
        themeKey_ = current;
    }

    void PreviewSurface::setPlotBounds(juce::Rectangle<float> plotBounds) noexcept
    {
        if (plotBounds_ != plotBounds)
        {
            plotBounds_ = plotBounds;
            invalidateAll();
        }
    }

    void PreviewSurface::setDataKey(uint64_t key) noexcept
    {
        if (dataKey_ != key)
        {
            dataKey_ = key;
            dataDirty_ = true;
        }
    }

    void PreviewSurface::ensureBackgroundImage()
    {
        if (!backgroundDirty_ && backgroundLayer_.isValid())
            return;

        backgroundLayer_ = renderPlotHiRes(
            plotBounds_,
            [](juce::Graphics& g, juce::Rectangle<float> plot)
            {
                g.setColour(juce::Colour(0xff0a0b0e).withAlpha(0.65f));
                g.fillRoundedRectangle(plot, 4.0f);
            },
            1.0f);
        backgroundDirty_ = false;
    }

    void PreviewSurface::ensureDataImage(const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw)
    {
        if (!dataDirty_ && dataLayer_.isValid())
            return;

        dataLayer_ = renderPlotHiRes(plotBounds_, draw, kSupersampleScale);
        dataDirty_ = false;
    }

    void PreviewSurface::paintBackground(juce::Graphics& g,
                                          const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw)
    {
        if (plotBounds_.isEmpty())
            return;

        ensureThemeCurrent();
        ensureBackgroundImage();
        g.drawImage(backgroundLayer_, plotBounds_);
        draw(g, plotBounds_);
    }

    void PreviewSurface::paintData(juce::Graphics& g, const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw)
    {
        if (plotBounds_.isEmpty())
            return;

        ensureThemeCurrent();
        ensureDataImage(draw);
        g.drawImage(dataLayer_, plotBounds_);
    }

    void PreviewSurface::paintOverlay(juce::Graphics& g,
                                       const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw) const
    {
        if (plotBounds_.isEmpty())
            return;

        draw(g, plotBounds_);
    }

    juce::Image renderPlotHiRes(juce::Rectangle<float> plotBounds,
                                 const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw, float scale)
    {
        if (plotBounds.isEmpty() || scale <= 0.0f)
            return {};

        const int w = juce::jmax(1, static_cast<int>(std::ceil(plotBounds.getWidth() * scale)));
        const int h = juce::jmax(1, static_cast<int>(std::ceil(plotBounds.getHeight() * scale)));

        juce::Image image(juce::Image::ARGB, w, h, true);
        juce::Graphics g(image);
        g.addTransform(juce::AffineTransform::scale(scale, scale).translated(-plotBounds.getX(), -plotBounds.getY()));
        draw(g, plotBounds);
        return image;
    }

} // namespace pw8::plugin::ui::preview
