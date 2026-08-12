#include "BrandingAssets.h"

#include "BinaryData.h"

namespace pw8::plugin::ui::branding
{
    namespace
    {
        juce::Image loadEmbedded(const char* data, int size)
        {
            if (data == nullptr || size <= 0)
                return {};

            return juce::ImageCache::getFromMemory(data, static_cast<std::size_t>(size));
        }

        juce::Image keyBlackTransparent(juce::Image src, float threshold = 0.11f)
        {
            if (!src.isValid())
                return {};

            juce::Image out = src.convertedToFormat(juce::Image::ARGB);
            juce::Image::BitmapData pixels(out, juce::Image::BitmapData::readWrite);

            for (int y = 0; y < pixels.height; ++y)
            {
                for (int x = 0; x < pixels.width; ++x)
                {
                    auto colour = pixels.getPixelColour(x, y);
                    if (colour.getBrightness() <= threshold)
                        colour = colour.withAlpha(0.0f);
                    pixels.setPixelColour(x, y, colour);
                }
            }

            return out;
        }

        juce::Image buildShipIcon()
        {
            auto mark = loadEmbedded(BinaryData::starfighter_logo_mark_png, BinaryData::starfighter_logo_mark_pngSize);
            if (!mark.isValid())
                return {};

            const int cropHeight = juce::jmax(1, static_cast<int>(static_cast<float>(mark.getHeight()) * 0.56f));
            mark = mark.getClippedImage({0, 0, mark.getWidth(), cropHeight});
            return keyBlackTransparent(mark);
        }

        const juce::Image& cachedShipIcon()
        {
            static const juce::Image icon = buildShipIcon();
            return icon;
        }
    } // namespace

    juce::Colour glowColour() noexcept
    {
        return juce::Colour(0xff5ecfff);
    }

    juce::Image getShipIcon()
    {
        return cachedShipIcon();
    }

    int wordmarkWidth() noexcept
    {
        return 248;
    }

    int headerBarHeight() noexcept
    {
        return 64;
    }

    void paintShipGlow(juce::Graphics& g, const juce::Image& ship, juce::Rectangle<float> bounds) noexcept
    {
        if (!ship.isValid() || bounds.isEmpty())
            return;

        const auto centre = bounds.getCentre();

        for (int i = 3; i >= 1; --i)
        {
            const float expand = static_cast<float>(i) * 2.5f;
            const float alpha = 0.07f * static_cast<float>(4 - i);
            g.setColour(glowColour().withAlpha(alpha));
            g.fillEllipse(centre.x - bounds.getWidth() * 0.35f - expand, centre.y - bounds.getHeight() * 0.35f - expand,
                         bounds.getWidth() * 0.7f + expand * 2.0f, bounds.getHeight() * 0.7f + expand * 2.0f);
        }

        g.setOpacity(1.0f);
        g.drawImageWithin(ship, static_cast<int>(bounds.getX()), static_cast<int>(bounds.getY()),
                          static_cast<int>(bounds.getWidth()), static_cast<int>(bounds.getHeight()),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }

} // namespace pw8::plugin::ui::branding
