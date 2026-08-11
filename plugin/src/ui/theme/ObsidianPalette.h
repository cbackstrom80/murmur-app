#pragma once

#include <juce_graphics/juce_graphics.h>

// OBSIDIAN -- the one launch skin (docs/UI.md), chosen deliberately as the safest
// path to "genuinely premium" (dark glass/hardware territory already proven by
// u-he/Arturia/Serum's own dark mode) rather than the boldest option, so the very
// first real Patchwork Eight screen reads as finished rather than experimental.
// Every color used anywhere in plugin/src/ui/ comes from this file -- no component
// hand-rolls its own juce::Colour literal, so the whole skin can be re-tuned (or a
// second skin added later) by editing one place.
namespace pw8::plugin::ui::palette
{
    // -- Structure: background/panel/border, darkest to lightest. --
    inline const juce::Colour kBackgroundTop{0xff0b0c0f};
    inline const juce::Colour kBackgroundBottom{0xff101216};
    inline const juce::Colour kPanel{0xff16181d};
    inline const juce::Colour kPanelRaised{0xff1c1f26};
    inline const juce::Colour kBorder{0xff232630};
    inline const juce::Colour kBorderBright{0xff30343f};

    // -- Text. --
    inline const juce::Colour kTextPrimary{0xffe6e8ec};
    inline const juce::Colour kTextSecondary{0xff8b909c};
    inline const juce::Colour kTextDim{0xff565a66};

    // -- A deliberate duotone, not a single accent: cool cyan for structural/signal
    // things (the algorithm graph, Filter, FX), warm amber for performance things
    // (the 8 macros -- the one surface a player's hands are actually on). Neither
    // reads as "extra" because each owns a distinct, consistent role rather than
    // competing for the same meaning -- restraint is still the point, just spent on
    // two colors instead of one. --
    inline const juce::Colour kAccent{0xff7fe7e0};
    inline const juce::Colour kAccentDim{0xff3d5c59};

    inline const juce::Colour kAccentWarm{0xffe8a33d};
    inline const juce::Colour kAccentWarmDim{0xff5c4a2c};

    // Card depth: a soft shadow beneath every SectionPanel, and a faint highlight
    // along its top edge -- the "milled panel set into a chassis" read depends on
    // both, not just a border.
    inline const juce::Colour kShadow{0x66000000};
    inline const juce::Colour kTopHighlight{0x14ffffff};

    // -- Semantic edge colors for the algorithm graph view, one per
    // algorithm::EdgeType. Deliberately all desaturated relative to kAccent -- an
    // *active* edge on any of these still reads via glow/animation, not by being a
    // louder color than its neighbors. --
    inline const juce::Colour kEdgeAudio{0xff6f7684};
    inline const juce::Colour kEdgePhaseMod{0xff8f7fe0};
    inline const juce::Colour kEdgeFrequencyMod{0xff7f9fe0};
    inline const juce::Colour kEdgeAmplitudeMod{0xffe0c17f};
    inline const juce::Colour kEdgeRingMod{0xffe07fb0};
    inline const juce::Colour kEdgeSync{0xff7fe0a0};
    inline const juce::Colour kEdgeFeedback{0xffe07f7f};

    [[nodiscard]] inline juce::Colour edgeColour(int edgeTypeOrdinal) noexcept
    {
        switch (edgeTypeOrdinal)
        {
            case 1: return kEdgePhaseMod;
            case 2: return kEdgeFrequencyMod;
            case 3: return kEdgeAmplitudeMod;
            case 4: return kEdgeRingMod;
            case 5: return kEdgeSync;
            case 6: return kEdgeFeedback;
            default: return kEdgeAudio;
        }
    }

    // -- Drag-to-modulate (docs/UI.md): one distinct color per mod source chip,
    // reused as the ring painted around any knob that source is currently assigned
    // to -- the "trace what's modulating what at a glance" property Serum/Vital's
    // own modulation UI is built around. Deliberately distinct from both the cool
    // structural accent and the warm macro accent, so a modulation ring never reads
    // as "just another knob's own value arc."
    inline const juce::Colour kModLfo{0xffb08fe8};
    inline const juce::Colour kModEnv{0xff8fd4e8};
    inline const juce::Colour kModVelocity{0xffe88f9e};

    /// `source` is a raw `modulation::ModSource` ordinal (kept as `int` here so this
    /// header doesn't need to include ModMatrixTypes.hpp just for one enum). Shared
    /// by ModSourceChip (assigns the color to the chip itself) and GlowKnob (assigns
    /// the color to a knob's modulation ring) so both always agree without either
    /// one hand-copying the mapping.
    [[nodiscard]] inline juce::Colour modSourceColour(int source) noexcept
    {
        if (source >= 1 && source <= 8) return kModLfo;   // Lfo1..Lfo8
        if (source >= 9 && source <= 16) return kModEnv;  // Env1..Env8
        if (source == 17) return kModVelocity;            // Velocity
        return kTextDim; // None, or a performance/macro source this UI doesn't offer as a drag chip yet.
    }

} // namespace pw8::plugin::ui::palette
