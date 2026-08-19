#include "UpdateBadge.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "net/UpdateChecker.h"

namespace pw8::plugin::ui
{
    namespace
    {
        // Same secondary teal the Figma frame uses for this element's
        // border/dot/text/chevron -- distinct from the primary accent
        // (kAccent) used everywhere else, already a named palette token
        // from the FX-toggle work, not a new one-off color.
        const juce::Colour& kBadgeTeal = palette::kFigmaFxToggleOnBorder;

        /// Same manual letter-spacing helper as SplashOverlay.cpp/
        /// KeyActivationOverlay.cpp used to have -- JUCE's Graphics::drawText
        /// has no native tracking parameter.
        void drawTracked(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> bounds,
                         const juce::Font& font, float tracking)
        {
            g.setFont(font);
            float x = bounds.getX();
            const float y = bounds.getCentreY() - font.getHeight() * 0.5f;
            for (int i = 0; i < text.length(); ++i)
            {
                const auto ch = juce::String::charToString(text[i]);
                const auto w = font.getStringWidthFloat(ch);
                g.drawText(ch, juce::Rectangle<float>(x, y, w + tracking, font.getHeight()),
                          juce::Justification::centredLeft, false);
                x += w + tracking;
            }
        }
    } // namespace

    UpdateBadge::UpdateBadge() { setVisible(false); }

    UpdateBadge::~UpdateBadge() = default;

    void UpdateBadge::checkNow()
    {
        net::UpdateChecker::checkForUpdate([this](net::UpdateCheckResult result) {
            if (!result.success || !result.updateAvailable)
                return; // no nag on a failed check or when already current

            releaseUrl_ = juce::URL(result.releaseUrl);
            // ASCII dash, not the Figma reference's em-dash -- this
            // codebase's fonts (Avenir Next at small sizes) garble certain
            // UTF-8 punctuation, see ObsidianFonts.h's kDash/kArrow/kSep.
            labelText_ = "NEW UPDATE AVAILABLE" + juce::String(fonts::kDash) + "V" + result.latestVersion;
            setVisible(true);
            repaint();
        });
    }

    void UpdateBadge::mouseUp(const juce::MouseEvent&)
    {
        if (releaseUrl_.isWellFormed())
            releaseUrl_.launchInDefaultBrowser();
    }

    void UpdateBadge::paint(juce::Graphics& g)
    {
        if (!isVisible() || labelText_.isEmpty())
            return;

        const auto bounds = getLocalBounds().toFloat();
        const float radius = bounds.getHeight() * 0.5f; // fully pill, matches Figma's cornerRadius 999

        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(bounds, radius);
        g.setColour(kBadgeTeal);
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);

        constexpr float kPadding = 16.0f;
        constexpr float kDotDiameter = 8.0f;
        constexpr float kDotToText = 10.0f;

        const juce::Rectangle<float> dotBounds(bounds.getX() + kPadding, bounds.getCentreY() - kDotDiameter * 0.5f,
                                               kDotDiameter, kDotDiameter);
        g.setColour(kBadgeTeal);
        g.fillEllipse(dotBounds);

        const float textX = dotBounds.getRight() + kDotToText;
        g.setColour(kBadgeTeal);
        drawTracked(g, labelText_, juce::Rectangle<float>(textX, bounds.getY(), bounds.getWidth() - textX, bounds.getHeight()),
                   fonts::denseBold(11.0f), 1.2f);

        // Small ">" chevron, right-aligned with the same padding as the dot's left inset.
        const float chevronCentreX = bounds.getRight() - kPadding - 8.0f;
        const float chevronCentreY = bounds.getCentreY();
        juce::Path chevron;
        chevron.startNewSubPath(chevronCentreX - 2.0f, chevronCentreY - 4.0f);
        chevron.lineTo(chevronCentreX + 2.0f, chevronCentreY);
        chevron.lineTo(chevronCentreX - 2.0f, chevronCentreY + 4.0f);
        g.setColour(kBadgeTeal);
        g.strokePath(chevron, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

} // namespace pw8::plugin::ui
