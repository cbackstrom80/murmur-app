#pragma once

#include <juce_graphics/juce_graphics.h>

// A shared font, not JUCE's plain default sans -- the single biggest lever on
// "does this look premium" per typeface, cheaper than any other visual change.
// Every label/value/title in plugin/src/ui/ routes through here rather than
// constructing its own juce::Font, so the whole skin's typography is one
// decision, tunable in one place (the same discipline ObsidianPalette.h
// applies to color).
//
// Deliberately a curated system-font fallback chain rather than an embedded
// font file: this project builds/ships on macOS today (auval/pluginval are
// both macOS-only tools this codebase already depends on), so a geometric
// system face available on every Mac (Avenir Next) is a real typographic
// upgrade for zero licensing/binary-embedding cost. A licensed, embedded,
// truly cross-platform typeface is the honest PLANNED follow-up once Windows/
// Linux builds are a real target -- see docs/UI.md.
namespace pw8::plugin::ui::fonts
{
    // Minimum readable sizes for dark Obsidian panels (see docs/UI.md).
    inline constexpr float kCaptionSize = 10.0f;
    inline constexpr float kLabelMinSize = 10.5f;
    inline constexpr float kSectionTitleSize = 12.5f;
    inline constexpr float kBodyLabelSize = 11.0f;

    [[nodiscard]] inline const juce::String& preferredFamily() noexcept
    {
        static const juce::String family = [] {
            const auto available = juce::Font::findAllTypefaceNames();
            for (const auto* candidate : {"Avenir Next", "Futura", "Helvetica Neue"})
                if (available.contains(candidate))
                    return juce::String(candidate);
            return juce::Font::getDefaultSansSerifFontName();
        }();
        return family;
    }

    [[nodiscard]] inline juce::Font title(float size) noexcept
    {
        return juce::Font(juce::FontOptions(preferredFamily(), size, juce::Font::bold)).withExtraKerningFactor(0.09f);
    }

    [[nodiscard]] inline juce::Font label(float size) noexcept
    {
        return juce::Font(juce::FontOptions(preferredFamily(), juce::jmax(kLabelMinSize, size), juce::Font::plain))
            .withExtraKerningFactor(0.06f);
    }

    [[nodiscard]] inline juce::Font value(float size) noexcept
    {
        return juce::Font(juce::FontOptions(preferredFamily(), juce::jmax(kCaptionSize, size), juce::Font::plain));
    }

    [[nodiscard]] inline juce::Font caption(float size) noexcept
    {
        return value(juce::jmax(kCaptionSize, size));
    }

} // namespace pw8::plugin::ui::fonts
