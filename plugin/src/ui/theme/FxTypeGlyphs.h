#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui::fxglyphs
{
    /// Figma `sprite-sheet-fx-chain` SEC_02 — one glyph per DESIGN FX chip index (BYP→VOC).
    void paintChipGlyph(juce::Graphics& g, juce::Rectangle<float> bounds, std::size_t chipIndex, bool active);

    /// Map live effect type ordinal to the closest Figma glyph (PLAY chain flow boxes).
    void paintEffectTypeGlyph(juce::Graphics& g, juce::Rectangle<float> bounds, int effectType, bool active);

} // namespace pw8::plugin::ui::fxglyphs
