#pragma once

#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

#include "processor/PatchworkEightProcessor.h"

// A genuinely GPU-rendered 3D knob -- faceted 12-sided body, flat blue
// indicator line, matte/flat-shaded (no glow, no chrome, no glass), the
// design chosen from the HTML/Three.js prototype worked out with Curtis
// before this was written. This is the FIRST real juce::OpenGLContext usage
// anywhere in this project -- docs/UI.md and docs/VISUALIZATION_UI_GATE5.md
// both document TWO earlier, deliberate decisions to stay in plain
// juce::Graphics 2D-path pseudo-3D rather than take this on (UI GATE 5's
// WavetableStackView, and again in UI GATE 6's wireframe-mesh revision of
// it). This component is the spike that decision was always going to need
// before scaling past it -- see this file's own header comment in the .cpp
// for what's still unverified.
//
// Deliberately mirrors GlowKnob's real, already-proven interaction pattern
// rather than inventing a new one: a real juce::Slider (RotaryHorizontalVerticalDrag,
// same angle range as every other knob in this skin) handles drag gestures,
// keyboard, double-click-reset, and the real juce::AudioProcessorValueTreeState::
// SliderAttachment -- exactly GlowKnob's own machinery, all of it already
// working and host-automation-safe. The ONLY thing this component replaces is
// the *visual* layer: slider_ is set fully transparent (setAlpha(0), which
// stops it drawing anything while leaving it fully interactive -- mouse and
// keyboard events don't depend on visible alpha in JUCE) and renderOpenGL()
// draws the real 3D geometry read from slider_.getValue() every frame instead.
//
// NOT wired in everywhere -- this replaces exactly ONE knob (MacroStrip's
// Macro 1) as a live, functional proof point, not a skin-wide swap. See
// MacroStrip.cpp for the integration and why it stays scoped to one knob.
namespace pw8::plugin::ui
{
    class Knob3D : public juce::Component, private juce::OpenGLRenderer
    {
    public:
        /// Same constructor shape as GlowKnob -- apvts + paramId + display
        /// name -- so this is a drop-in swap wherever a caller already knows
        /// GlowKnob's construction pattern. No accentColour parameter (the
        /// 3D material IS the accent here, not a themeable override) and no
        /// valueToText (the value readout label is plain 2-decimal, same
        /// default GlowKnob itself falls back to).
        Knob3D(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& name);
        ~Knob3D() override;

        void resized() override;

        void setDisplayName(const juce::String& name) { nameLabel_.setText(name.toUpperCase(), juce::dontSendNotification); }

    private:
        // -- juce::OpenGLRenderer --
        void newOpenGLContextCreated() override;
        void renderOpenGL() override;
        void openGLContextClosing() override;

        void buildGeometry();
        void buildShaders();

        struct Vertex
        {
            float position[3];
            float normal[3];
            float colour[3];
        };

        juce::OpenGLContext glContext_;
        std::unique_ptr<juce::OpenGLShaderProgram> shader_;
        std::unique_ptr<juce::OpenGLShaderProgram::Attribute> positionAttribute_;
        std::unique_ptr<juce::OpenGLShaderProgram::Attribute> normalAttribute_;
        std::unique_ptr<juce::OpenGLShaderProgram::Attribute> colourAttribute_;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> modelViewProjUniform_;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> normalMatrixUniform_;

        GLuint vertexBuffer_ = 0;
        std::vector<Vertex> bodyVertices_;
        std::vector<Vertex> indicatorVertices_;

        juce::Slider slider_; // Invisible (alpha 0) but fully interactive -- see class doc comment.
        juce::Label nameLabel_;
        juce::Label valueLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob3D)
    };

} // namespace pw8::plugin::ui
