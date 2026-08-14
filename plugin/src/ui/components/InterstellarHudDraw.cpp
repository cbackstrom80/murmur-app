#include "InterstellarHudDraw.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui::interstellar
{
    bool isInterstellarCategory(const juce::String& category)
    {
        return category.trim().equalsIgnoreCase("Interstellar");
    }

    void paintHudBadge(juce::Graphics& g, juce::Rectangle<float> bounds, bool showCapsule)
    {
        constexpr float kTick = 6.0f;
        const auto accent = palette::kAccentWarm.withAlpha(0.45f);

        g.setColour(accent);
        const juce::Point<float> corners[] = {
            bounds.getTopLeft(),
            bounds.getTopRight(),
            bounds.getBottomLeft(),
            bounds.getBottomRight(),
        };
        for (const auto& c : corners)
        {
            if (c == bounds.getTopLeft())
            {
                g.drawLine(c.x, c.y, c.x + kTick, c.y, 1.0f);
                g.drawLine(c.x, c.y, c.x, c.y + kTick, 1.0f);
            }
            else if (c == bounds.getTopRight())
            {
                g.drawLine(c.x, c.y, c.x - kTick, c.y, 1.0f);
                g.drawLine(c.x, c.y, c.x, c.y + kTick, 1.0f);
            }
            else if (c == bounds.getBottomLeft())
            {
                g.drawLine(c.x, c.y, c.x + kTick, c.y, 1.0f);
                g.drawLine(c.x, c.y, c.x, c.y - kTick, 1.0f);
            }
            else
            {
                g.drawLine(c.x, c.y, c.x - kTick, c.y, 1.0f);
                g.drawLine(c.x, c.y, c.x, c.y - kTick, 1.0f);
            }
        }

        for (float tx = bounds.getX() + 12.0f; tx < bounds.getRight() - 12.0f; tx += 14.0f)
        {
            g.setColour(accent.withAlpha(0.22f));
            g.drawLine(tx, bounds.getY() + 1.0f, tx, bounds.getY() + 3.0f, 0.8f);
            g.drawLine(tx, bounds.getBottom() - 3.0f, tx, bounds.getBottom() - 1.0f, 0.8f);
        }

        if (!showCapsule)
            return;

        auto capsule = bounds.removeFromTop(14.0f).withSizeKeepingCentre(88.0f, 12.0f);
        g.setColour(palette::kBackgroundBottom.withAlpha(0.85f));
        g.fillRoundedRectangle(capsule, 6.0f);
        g.setColour(accent);
        g.drawRoundedRectangle(capsule, 6.0f, 1.0f);
        g.setFont(fonts::label(7.5f));
        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.drawText("INTERSTELLAR", capsule, juce::Justification::centred);
    }

} // namespace pw8::plugin::ui::interstellar
