#include "SplashOverlay.h"

#include "BinaryData.h"
#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        /// Manual letter-spacing draw — JUCE's Graphics::drawText has no
        /// native tracking parameter, and this is the only place in the UI
        /// that needs the Figma splash's wide-tracked uppercase labels.
        void drawTracked(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> bounds,
                         const juce::Font& font, float tracking, juce::Justification justification)
        {
            g.setFont(font);
            float totalWidth = 0.0f;
            for (int i = 0; i < text.length(); ++i)
                totalWidth += font.getStringWidthFloat(juce::String::charToString(text[i])) + tracking;
            totalWidth -= tracking;

            float x = bounds.getX();
            if (justification.testFlags(juce::Justification::horizontallyCentred))
                x = bounds.getCentreX() - totalWidth * 0.5f;
            else if (justification.testFlags(juce::Justification::right))
                x = bounds.getRight() - totalWidth;

            const float y = bounds.getCentreY() - font.getHeight() * 0.5f;
            for (int i = 0; i < text.length(); ++i)
            {
                const auto ch = juce::String::charToString(text[i]);
                g.drawText(ch, juce::Rectangle<float>(x, y, font.getStringWidthFloat(ch) + tracking, font.getHeight()),
                          juce::Justification::centredLeft, false);
                x += font.getStringWidthFloat(ch) + tracking;
            }
        }
    } // namespace

    SplashOverlay::SplashOverlay()
    {
        backgroundImage_ =
            juce::ImageCache::getFromMemory(BinaryData::murmur_splash_bg_jpg, BinaryData::murmur_splash_bg_jpgSize);
        markIcon_ = branding::getWhaleIcon();
        // false/true, not false/false: the overlay itself stays click-through
        // (splash isn't modal), but updateBadge_ -- a real child component --
        // still needs to receive its own click to open the release URL.
        setInterceptsMouseClicks(false, true);

        addChildComponent(updateBadge_);
        // Fired immediately, not on dismiss: this banner only exists for the
        // splash's own ~1.9s lifetime (matches the Figma frame's placement),
        // unlike the license-activation flow which persists after dismiss.
        updateBadge_.checkNow();

        startTimerHz(30);
    }

    SplashOverlay::~SplashOverlay() { stopTimer(); }

    void SplashOverlay::timerCallback()
    {
        elapsedMs_ += 1000.0 / 30.0;
        repaint();

        if (elapsedMs_ >= kDisplayMs + kFadeMs)
        {
            stopTimer();
            if (onDismissed)
                onDismissed();
        }
    }

    void SplashOverlay::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        const float fadeAlpha =
            elapsedMs_ <= kDisplayMs
                ? 1.0f
                : juce::jmax(0.0f, 1.0f - static_cast<float>((elapsedMs_ - kDisplayMs) / kFadeMs));

        g.setOpacity(fadeAlpha);
        updateBadge_.setAlpha(fadeAlpha); // child component's own paint() doesn't
                                            // inherit this Graphics context's opacity

        // Base fill, matching the Figma frame's own deep-water solid fill --
        // shows through at the crop edges since the source photo is square
        // (1024x1024) but the editor window is wide.
        g.fillAll(juce::Colour(0xff0d0f14));

        if (backgroundImage_.isValid())
        {
            g.drawImage(backgroundImage_, bounds, juce::RectanglePlacement::fillDestination);
            // Darker than the original pass -- the mark now carries the
            // brand alone (no wordmark line under it), so it needs more
            // contrast against the photo to read as the hero element.
            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.fillRect(bounds);
        }

        const auto centreY = bounds.getCentreY();

        // Whale mark alone, centred -- no separate "MURMUR" wordmark: the
        // window title bar already says MURMUR, so a second text lockup
        // here was redundant. Bigger than the original pass since it's now
        // the only header element.
        constexpr float markSize = 150.0f;
        const juce::Rectangle<float> markBounds(bounds.getCentreX() - markSize * 0.5f, centreY - 180.0f, markSize,
                                                 markSize);
        if (markIcon_.isValid())
            g.drawImageWithin(markIcon_, (int)markBounds.getX(), (int)markBounds.getY(), (int)markBounds.getWidth(),
                              (int)markBounds.getHeight(), juce::RectanglePlacement::centred);

        g.setColour(palette::kFigmaTeal);
        drawTracked(g, "8-ENGINE COGNITIVE SYNTHESIZER",
                   juce::Rectangle<float>(bounds.getX(), centreY - 22.0f, bounds.getWidth(), 20.0f),
                   fonts::label(12.0f), 3.0f, juce::Justification::centred);

        // Real fixed-duration fill, not a fabricated backend-progress
        // percentage -- see the class doc comment.
        constexpr float barWidth = 260.0f;
        constexpr float barHeight = 2.0f;
        const juce::Rectangle<float> barBounds(bounds.getCentreX() - barWidth * 0.5f, centreY + 36.0f, barWidth,
                                                barHeight);
        g.setColour(palette::kFigmaTextDim.withAlpha(0.3f));
        g.fillRect(barBounds);
        const float fillFraction = juce::jlimit(0.0f, 1.0f, static_cast<float>(elapsedMs_ / kDisplayMs));
        g.setColour(palette::kFigmaTeal);
        g.fillRect(barBounds.withWidth(barWidth * fillFraction));

        const juce::String statusText = fillFraction < 1.0f ? "INITIALIZING ENGINES" : "READY";
        g.setFont(fonts::micro(9.0f));
        g.setColour(palette::kFigmaTextDim);
        g.drawText(statusText, juce::Rectangle<float>(bounds.getX(), barBounds.getBottom() + 8.0f, bounds.getWidth(), 16.0f),
                  juce::Justification::centred, false);

        // Footer: real product version (JucePlugin_VersionString, same
        // macro VstBottomBar.cpp already uses), not a fabricated one.
        g.setFont(fonts::micro(9.0f));
        g.setColour(palette::kFigmaTextDim.withAlpha(0.7f));
        g.drawText("MURMUR AUDIO (C) 2026", juce::Rectangle<float>(16.0f, bounds.getBottom() - 26.0f, 240.0f, 16.0f),
                  juce::Justification::centredLeft, false);
        g.drawText("v" + juce::String(JucePlugin_VersionString),
                  juce::Rectangle<float>(bounds.getWidth() - 100.0f, bounds.getBottom() - 26.0f, 84.0f, 16.0f),
                  juce::Justification::centredRight, false);
    }

    void SplashOverlay::resized()
    {
        // Fixed 380x40 pill, 24px from the top, horizontally centred --
        // exact geometry from the Figma frame's update-notification-banner
        // node (absoluteBoundingBox relative to its 1280-wide splash frame).
        constexpr int kBannerWidth = 380;
        constexpr int kBannerHeight = 40;
        constexpr int kBannerTopMargin = 24;
        updateBadge_.setBounds(getWidth() / 2 - kBannerWidth / 2, kBannerTopMargin, kBannerWidth, kBannerHeight);
    }

} // namespace pw8::plugin::ui
