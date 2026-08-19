#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "processor/PatchworkEightProcessor.h"
#include "../theme/FigmaKnobTokens.h"

// The Figma glow-ring-knob control (frame 21:4): decked rotary + mod rings.
// Code Connect: plugin/src/ui/figma-connect/GlowKnob.figma.ts
//
// Attached directly to an APVTS parameter, so automating it from a host visibly moves the
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

        /// UI-only knob (no APVTS) — same Figma decked rendering; for patch-local or UI-state values.
        GlowKnob(const juce::String& name, double minValue, double maxValue, double initialValue,
                 std::function<void(double)> onValueChange,
                 std::function<juce::String(float)> valueToText = nullptr,
                 juce::Colour accentColour = juce::Colours::transparentBlack);
        ~GlowKnob() override;

        void setManualValue(double value, juce::NotificationType notification = juce::dontSendNotification);
        [[nodiscard]] double getManualValue() const noexcept;

        /// Re-read the bound APVTS parameter into the slider (e.g. after preset load).
        void syncSliderFromParameter(const juce::RangedAudioParameter& param);

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

        /// Shows a warm activity ring when this macro fans out to active mod routes.
        void enableMacroActivityRing(PatchworkEightProcessor& processor, std::size_t macroIndex);

        void setModAssignmentController(ModAssignmentController* controller);

        /// Overrides the name label set at construction -- e.g. MacroStrip calling
        /// this with a patch-authored macro name ("Growl") once one's loaded,
        /// instead of the generic "Macro 3" it starts with. Uppercased the same
        /// way the constructor's `name` already is, for the same reason (see the
        /// constructor's own label styling).
        void setDisplayName(const juce::String& name) { nameLabel_.setText(name.toUpperCase(), juce::dontSendNotification); }

        /// Larger dial cap for BASIC performance layout (default 88 in Obsidian LAF).
        void setMaxDialDiameter(int diameter);

        /// Concentric decked knob rendering (outer recess, accent rim, inner cap).
        enum class DeckedKnobSize
        {
            Small,
            Medium,
            Large,
        };
        void setDeckedStyle(bool enabled, DeckedKnobSize size = DeckedKnobSize::Medium);

        void setFeaturedPerformanceMacro(bool featured);

        /// Compact header layout: smaller dial, hidden value readout.
        void setHeaderCompactMode(bool compact);

        /// Apply diameter, deck size, label font, and readout mode from Figma `21:4` context tokens.
        void applyFigmaContext(figma::KnobContext context);

        /// Hide mod-route overlay rings (desktop PLAY macros — value arc only).
        void setShowModRouteRing(bool show);

        void setShowNameLabel(bool show);

        [[nodiscard]] juce::String getValueDisplayText() const;

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
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

        FormattedSlider slider_;
        juce::Label nameLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

        ModAssignmentController* modAssignment_ = nullptr;
        PatchworkEightProcessor* modProcessor_ = nullptr;
        modulation::ModDestination modDestination_ = modulation::ModDestination::None;
        std::uint8_t modTargetIndex_ = 0;
        modulation::ModSource ringSource_ = modulation::ModSource::None; ///< The route currently shown by ringColour_, so right-click-remove can identify it exactly.
        juce::Colour ringColour_ = juce::Colours::transparentBlack; ///< transparent == no route assigned.
        float ringAmountNormalized_ = 0.0f;
        float liveModNormalized_ = -1.0f; ///< Ghost pointer: modulated effective value 0..1; <0 hides.
        juce::String paramId_;
        bool macroActivityMode_ = false;
        std::size_t macroActivityIndex_ = 0;
        bool dragHover_ = false;
        bool showDepthPopover_ = false;
        juce::Rectangle<int> depthPopoverArea_;
        bool depthDragActive_ = false;
        float depthDragStartAmount_ = 0.0f;
        float depthDragStartX_ = 0.0f;
        int maxDialDiameter_ = 88;
        bool headerCompactMode_ = false;
        bool deckedStyle_ = false;
        DeckedKnobSize deckedSize_ = DeckedKnobSize::Medium;
        bool featuredPerformanceMacro_ = false;
        bool hideValueBox_ = false;
        bool showModRouteRing_ = true;
        bool manualMode_ = false;
        std::function<void(double)> manualOnChange_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlowKnob)
    };

} // namespace pw8::plugin::ui
