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

        juce::Image buildMarkIcon()
        {
            auto mark = loadEmbedded(BinaryData::murmur_mark_512_png, BinaryData::murmur_mark_512_pngSize);
            return keyBlackTransparent(mark);
        }

        juce::Image buildLogoLockup()
        {
            auto logo = loadEmbedded(BinaryData::murmur_logo_lockup_png, BinaryData::murmur_logo_lockup_pngSize);
            if (!logo.isValid())
                return {};

            return logo.convertedToFormat(juce::Image::ARGB);
        }

        juce::Image buildWhaleIcon()
        {
            auto whale = loadEmbedded(BinaryData::murmur_whale_512_png, BinaryData::murmur_whale_512_pngSize);
            if (!whale.isValid())
                return {};

            // Source PNG already has a transparent background (verified at
            // export time) -- no black-keying needed, unlike the M mark.
            // It also already bakes in a small "MURMUR" wordmark under the
            // whale (verified visually) -- see the getWhaleIcon() doc comment.
            return whale.convertedToFormat(juce::Image::ARGB);
        }

        const juce::Image& cachedMarkIcon()
        {
            static const juce::Image icon = buildMarkIcon();
            return icon;
        }

        const juce::Image& cachedLogoLockup()
        {
            static const juce::Image logo = buildLogoLockup();
            return logo;
        }

        const juce::Image& cachedWhaleIcon()
        {
            static const juce::Image whale = buildWhaleIcon();
            return whale;
        }
    } // namespace

    juce::Colour glowColour() noexcept
    {
        return juce::Colour(0xffB9A8FF);
    }

    juce::Image getMarkIcon()
    {
        return cachedMarkIcon();
    }

    juce::Image getLogoLockup()
    {
        return cachedLogoLockup();
    }

    juce::Image getWhaleIcon()
    {
        return cachedWhaleIcon();
    }

    int logoLockupWidth() noexcept
    {
        return 85;
    }

    int logoLockupHeight() noexcept
    {
        return 43;
    }

    int compactLogoWidth() noexcept
    {
        return 35;
    }

    int compactLogoHeight() noexcept
    {
        return 18;
    }

    int wordmarkWidth() noexcept
    {
        return 228;
    }

    int headerBarHeight() noexcept
    {
        return 54;
    }

    void paintMarkGlow(juce::Graphics& g, const juce::Image& mark, juce::Rectangle<float> bounds) noexcept
    {
        if (!mark.isValid() || bounds.isEmpty())
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
        g.drawImageWithin(mark, static_cast<int>(bounds.getX()), static_cast<int>(bounds.getY()),
                          static_cast<int>(bounds.getWidth()), static_cast<int>(bounds.getHeight()),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }

    void paintLogoLockup(juce::Graphics& g, juce::Rectangle<float> bounds) noexcept
    {
        const auto& logo = cachedLogoLockup();
        if (!logo.isValid() || bounds.isEmpty())
            return;

        g.drawImageWithin(logo, static_cast<int>(bounds.getX()), static_cast<int>(bounds.getY()),
                          static_cast<int>(bounds.getWidth()), static_cast<int>(bounds.getHeight()),
                          juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid
                              | juce::RectanglePlacement::onlyReduceInSize);
    }

} // namespace pw8::plugin::ui::branding
