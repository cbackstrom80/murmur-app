#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

// The one control primitive PLAY mode is built from: a rotary knob (painted by
// ObsidianLookAndFeel), a name label underneath, and a value readout -- attached
// directly to an APVTS parameter, so automating it from a host visibly moves the
// same knob the user sees (the property docs/PLUGIN_ARCHITECTURE.md's whole
// "Automation" section exists to guarantee at the engine level; this is that
// guarantee's visible side).
//
// Optionally also a drag-to-modulate drop target (docs/UI.md) -- call
// enableModulationTarget() to opt a specific knob in. Not every knob accepts
// modulation (only the two real mod-matrix destinations PLAY mode currently
// surfaces, Filter Cutoff/Resonance -- see FilterLfoPanel), so this stays an
// explicit opt-in rather than every GlowKnob silently becoming a drop target.
namespace pw8::plugin::ui
{
    class GlowKnob : public juce::Component, public juce::DragAndDropTarget, private juce::Timer
    {
    public:
        /// `valueToText`, if given, formats the raw parameter value for the knob's
        /// text readout (e.g. mapping a discrete filter-mode float to "LOWPASS")
        /// instead of the raw number -- used for every enum-valued parameter this
        /// skin exposes, so PLAY mode never shows a bare "2.0" for a mode control.
        /// `accentColour`, if not transparent (the default), overrides
        /// ObsidianLookAndFeel's default cool-cyan value arc/pointer with a
        /// different color for this one knob -- the duotone mechanism (macros use
        /// the warm variant; see ObsidianPalette.h).
        GlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& name,
                  std::function<juce::String(float)> valueToText = nullptr,
                  juce::Colour accentColour = juce::Colours::transparentBlack);
        ~GlowKnob() override;

        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;

        /// Opts this knob into drag-to-modulate: dropping a ModSourceChip onto it
        /// calls `processor.setOrReplaceModRouteLive(source, destination, targetIndex,
        /// ...)` with a sensible default depth for `destination`; right-clicking a
        /// currently-modulated knob removes the route. Polls
        /// `processor.getCurrentPatch().layerA.modRoutes` at a modest rate to notice
        /// changes made this way (or, in principle, by anything else) and repaint the
        /// ring -- there's no push notification for a patch change yet, and 8Hz on a
        /// FixedVector of at most 64 small POD structs is cheap.
        void enableModulationTarget(PatchworkEightProcessor& processor, modulation::ModDestination destination,
                                     std::uint8_t targetIndex = 0);

        // -- juce::DragAndDropTarget --
        bool isInterestedInDragSource(const SourceDetails& details) override;
        void itemDragEnter(const SourceDetails& details) override;
        void itemDragExit(const SourceDetails& details) override;
        void itemDropped(const SourceDetails& details) override;

    private:
        /// A juce::Slider subclass rather than relying on `textFromValueFunction`
        /// directly: overriding the virtual `getTextFromValue()` is a guaranteed,
        /// directly-verifiable formatting hook, and it doubles as the one place a
        /// sane default decimal precision gets applied for every continuous
        /// (non-formatter) knob -- fixing the "0.3635937"-style raw-float readout
        /// JUCE's own default would otherwise show.
        class FormattedSlider : public juce::Slider
        {
        public:
            std::function<juce::String(float)> valueToText;
            juce::String getTextFromValue(double value) override;
        };

        void timerCallback() override;
        void mouseDown(const juce::MouseEvent& event) override;

        FormattedSlider slider_;
        juce::Label nameLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

        PatchworkEightProcessor* modProcessor_ = nullptr;
        modulation::ModDestination modDestination_ = modulation::ModDestination::None;
        std::uint8_t modTargetIndex_ = 0;
        modulation::ModSource ringSource_ = modulation::ModSource::None; ///< The route currently shown by ringColour_, so right-click-remove can identify it exactly.
        juce::Colour ringColour_ = juce::Colours::transparentBlack; ///< transparent == no route assigned.
        bool dragHover_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlowKnob)
    };

} // namespace pw8::plugin::ui
