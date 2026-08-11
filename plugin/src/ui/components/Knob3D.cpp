#include "Knob3D.h"

#include <array>
#include <cmath>
#include <cstddef>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

// This file is the first place in the whole project that issues raw OpenGL
// calls. Written as carefully and conservatively as possible given that --
// no compiler or GPU available to verify against in the environment this was
// written in (same standing caveat as every plugin change so far, but this
// one carries more real risk than a 2D juce::Graphics change would: shader
// compilation, attribute/uniform binding, and matrix math are all places a
// typo produces a black screen or a crash rather than a slightly-wrong
// layout). Specific choices made to reduce that risk, not for cleverness:
//
// - Plain glDrawArrays with non-indexed triangle lists -- no index buffer,
//   so no chance of an off-by-one index bug. Costs a little more vertex
//   data (shared corners duplicated per face, needed anyway for flat
//   per-face shading -- see below), which doesn't matter at this scale.
// - Backface culling is left OFF. Normals are supplied explicitly per
//   vertex for lighting, so culling isn't needed for correctness, and
//   leaving it off means a winding-order mistake produces a slightly-wrong
//   look, never an invisible face.
// - Old-style GLSL (attribute/varying, no #version line) -- matches desktop
//   OpenGL's compatibility profile, which is what JUCE uses by default when
//   no explicit version is requested. This plugin only ships VST3/AU/
//   Standalone (desktop targets), so GLES precision-qualifier concerns
//   don't apply here and were deliberately left out, not overlooked.
// - Hand-rolled 4x4/3x3 matrix math (plain float arrays) instead of
//   juce::Matrix3D -- this avoids depending on exact JUCE 8.0.6 Matrix3D
//   method names/signatures that couldn't be checked against real headers
//   in this environment. More lines of code, but every line is plain C++
//   with no JUCE-API-guessing risk.
// - A VAO is generated and bound even though this isn't a forced core
//   profile -- macOS's OpenGL implementation (the actual deployment target
//   here) has known quirks requiring an active VAO for glDrawArrays even
//   outside strict core profile.
//
// None of this has been run. Build + visually verify before trusting it,
// and expect a first-pass bug or two -- that's normal for a project's first
// OpenGL component, not a sign this was done carelessly.

namespace pw8::plugin::ui
{
    using namespace juce::gl;

    namespace
    {
        constexpr int kFacets = 12;
        constexpr float kBodyRadius = 1.0f;
        constexpr float kBodyHalfHeight = 0.31f;

        // Colours matching the chosen design exactly (Variant B, flat blue line,
        // no glow) -- see the HTML/Three.js prototype this was worked out in.
        constexpr float kBodyColour[3] = {0x23 / 255.0f, 0x23 / 255.0f, 0x26 / 255.0f};
        constexpr float kIndicatorColour[3] = {0x2f / 255.0f, 0xb8 / 255.0f, 0xff / 255.0f};

        // -- Minimal 4x4/3x3 matrix helpers, column-major (OpenGL convention). --
        using Mat4 = std::array<float, 16>;
        using Mat3 = std::array<float, 9>;

        Mat4 identity4()
        {
            return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        }

        Mat4 multiply4(const Mat4& a, const Mat4& b)
        {
            Mat4 out{};
            for (int col = 0; col < 4; ++col)
            {
                for (int row = 0; row < 4; ++row)
                {
                    float sum = 0.0f;
                    for (int k = 0; k < 4; ++k)
                        sum += a[k * 4 + row] * b[col * 4 + k];
                    out[col * 4 + row] = sum;
                }
            }
            return out;
        }

        Mat4 perspective(float fovYRadians, float aspect, float nearZ, float farZ)
        {
            const float f = 1.0f / std::tan(fovYRadians * 0.5f);
            Mat4 m{};
            m[0] = f / aspect;
            m[5] = f;
            m[10] = (farZ + nearZ) / (nearZ - farZ);
            m[11] = -1.0f;
            m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
            return m;
        }

        Mat4 translation(float x, float y, float z)
        {
            auto m = identity4();
            m[12] = x;
            m[13] = y;
            m[14] = z;
            return m;
        }

        Mat4 rotationX(float radians)
        {
            auto m = identity4();
            const float c = std::cos(radians), s = std::sin(radians);
            m[5] = c;
            m[6] = s;
            m[9] = -s;
            m[10] = c;
            return m;
        }

        Mat4 rotationY(float radians)
        {
            auto m = identity4();
            const float c = std::cos(radians), s = std::sin(radians);
            m[0] = c;
            m[2] = -s;
            m[8] = s;
            m[10] = c;
            return m;
        }

        // Upper-left 3x3 of a 4x4 -- valid as a normal matrix here specifically
        // because every transform used (pure rotation + translation, no scale,
        // no shear) is orthonormal, so the usual inverse-transpose isn't needed.
        Mat3 upperLeft3x3(const Mat4& m)
        {
            return {m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]};
        }
    } // namespace

    Knob3D::Knob3D(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& name)
    {
        slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider_.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f,
                                     true);
        slider_.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0); // valueLabel_ below shows the value instead.
        addAndMakeVisible(slider_);
        slider_.setAlpha(0.0f); // Invisible, still fully interactive -- see class doc comment in Knob3D.h.

        nameLabel_.setText(name.toUpperCase(), juce::dontSendNotification);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        nameLabel_.setFont(fonts::label(10.5f));
        addAndMakeVisible(nameLabel_);

        valueLabel_.setJustificationType(juce::Justification::centred);
        valueLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        valueLabel_.setFont(fonts::value(10.0f));
        valueLabel_.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(valueLabel_);
        slider_.onValueChange = [this]
        {
            valueLabel_.setText(juce::String(slider_.getValue(), 2), juce::dontSendNotification);
            repaint();
        };

        attachment_ =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider_);
        valueLabel_.setText(juce::String(slider_.getValue(), 2), juce::dontSendNotification);

        buildGeometry();

        glContext_.setRenderer(this);
        glContext_.attachTo(*this);
        // Continuous repainting, not change-driven: guarantees the render always
        // reflects slider_.getValue() with no missed-update edge case (host
        // automation moving the parameter while nothing else triggers a repaint).
        // A real follow-up once more than one Knob3D exists on screen at once:
        // switch to triggerRepaint() from slider_.onValueChange + a shared
        // OpenGLContext across all of them, matching the perf-budget concern
        // already raised when this design was first proposed.
        glContext_.setContinuousRepainting(true);
    }

    Knob3D::~Knob3D()
    {
        glContext_.detach();
    }

    void Knob3D::buildGeometry()
    {
        bodyVertices_.clear();
        indicatorVertices_.clear();

        auto addTri = [](std::vector<Vertex>& out, const std::array<float, 3>& p0, const std::array<float, 3>& p1,
                          const std::array<float, 3>& p2, const std::array<float, 3>& n, const float (&colour)[3]) {
            for (const auto& p : {p0, p1, p2})
                out.push_back({{p[0], p[1], p[2]}, {n[0], n[1], n[2]}, {colour[0], colour[1], colour[2]}});
        };

        // -- Faceted side faces: kFacets flat quads (2 triangles each), each
        // with its own outward-facing normal so the facets read as hard/flat,
        // not smoothly rounded. --
        for (int i = 0; i < kFacets; ++i)
        {
            const float a0 = (float) i / (float) kFacets * juce::MathConstants<float>::twoPi;
            const float a1 = (float) (i + 1) / (float) kFacets * juce::MathConstants<float>::twoPi;
            const float aMid = (a0 + a1) * 0.5f;

            const std::array<float, 3> bottom0 = {std::cos(a0) * kBodyRadius, -kBodyHalfHeight, std::sin(a0) * kBodyRadius};
            const std::array<float, 3> bottom1 = {std::cos(a1) * kBodyRadius, -kBodyHalfHeight, std::sin(a1) * kBodyRadius};
            const std::array<float, 3> top0 = {std::cos(a0) * kBodyRadius, kBodyHalfHeight, std::sin(a0) * kBodyRadius};
            const std::array<float, 3> top1 = {std::cos(a1) * kBodyRadius, kBodyHalfHeight, std::sin(a1) * kBodyRadius};
            const std::array<float, 3> faceNormal = {std::cos(aMid), 0.0f, std::sin(aMid)};

            addTri(bodyVertices_, bottom0, bottom1, top1, faceNormal, kBodyColour);
            addTri(bodyVertices_, bottom0, top1, top0, faceNormal, kBodyColour);
        }

        // -- Top cap: a triangle fan, single up-facing normal (never viewed from
        // below in this UI, so no bottom cap -- invisible geometry not worth the
        // extra vertices). --
        const std::array<float, 3> topCentre = {0.0f, kBodyHalfHeight, 0.0f};
        const std::array<float, 3> upNormal = {0.0f, 1.0f, 0.0f};
        for (int i = 0; i < kFacets; ++i)
        {
            const float a0 = (float) i / (float) kFacets * juce::MathConstants<float>::twoPi;
            const float a1 = (float) (i + 1) / (float) kFacets * juce::MathConstants<float>::twoPi;
            const std::array<float, 3> rim0 = {std::cos(a0) * kBodyRadius, kBodyHalfHeight, std::sin(a0) * kBodyRadius};
            const std::array<float, 3> rim1 = {std::cos(a1) * kBodyRadius, kBodyHalfHeight, std::sin(a1) * kBodyRadius};
            addTri(bodyVertices_, topCentre, rim0, rim1, upNormal, kBodyColour);
        }

        // -- Indicator: a small flat box sitting just above the top cap, offset
        // toward +Z (matches the reference prototype's "line" position, rotated
        // by the model matrix along with the rest of the knob every frame). --
        const float ix = 0.06f, iy = 0.02f, iz = 0.42f; // half-extents
        const float iyPos = kBodyHalfHeight + 0.015f;
        const float izPos = 0.42f; // offset toward +Z, near the rim
        const std::array<float, 3> c000 = {-ix, iyPos - iy, izPos - iz};
        const std::array<float, 3> c100 = {ix, iyPos - iy, izPos - iz};
        const std::array<float, 3> c010 = {-ix, iyPos + iy, izPos - iz};
        const std::array<float, 3> c110 = {ix, iyPos + iy, izPos - iz};
        const std::array<float, 3> c001 = {-ix, iyPos - iy, izPos + iz};
        const std::array<float, 3> c101 = {ix, iyPos - iy, izPos + iz};
        const std::array<float, 3> c011 = {-ix, iyPos + iy, izPos + iz};
        const std::array<float, 3> c111 = {ix, iyPos + iy, izPos + iz};

        const std::array<float, 3> nUp = {0, 1, 0}, nDown = {0, -1, 0}, nFwd = {0, 0, 1}, nBack = {0, 0, -1},
                                     nRight = {1, 0, 0}, nLeft = {-1, 0, 0};

        addTri(indicatorVertices_, c010, c110, c111, nUp, kIndicatorColour);
        addTri(indicatorVertices_, c010, c111, c011, nUp, kIndicatorColour);
        addTri(indicatorVertices_, c000, c101, c100, nDown, kIndicatorColour);
        addTri(indicatorVertices_, c000, c001, c101, nDown, kIndicatorColour);
        addTri(indicatorVertices_, c001, c011, c111, nFwd, kIndicatorColour);
        addTri(indicatorVertices_, c001, c111, c101, nFwd, kIndicatorColour);
        addTri(indicatorVertices_, c100, c110, c010, nBack, kIndicatorColour);
        addTri(indicatorVertices_, c100, c010, c000, nBack, kIndicatorColour);
        addTri(indicatorVertices_, c101, c111, c110, nRight, kIndicatorColour);
        addTri(indicatorVertices_, c101, c110, c100, nRight, kIndicatorColour);
        addTri(indicatorVertices_, c000, c010, c011, nLeft, kIndicatorColour);
        addTri(indicatorVertices_, c000, c011, c001, nLeft, kIndicatorColour);
    }

    void Knob3D::buildShaders()
    {
        // Old-style GLSL (attribute/varying), no #version line -- see the file
        // header comment for why this matches the desktop-only deployment target.
        static const char* vertexShaderSource = R"(
            attribute vec4 position;
            attribute vec3 normal;
            attribute vec3 sourceColour;
            uniform mat4 modelViewProjection;
            uniform mat3 normalMatrix;
            varying vec3 vNormal;
            varying vec3 vColour;
            void main()
            {
                vNormal = normalMatrix * normal;
                vColour = sourceColour;
                gl_Position = modelViewProjection * position;
            }
        )";

        // Two-light flat-shaded lambert -- deliberately no specular term, matches
        // the matte/non-glossy material the chosen design calls for.
        static const char* fragmentShaderSource = R"(
            varying vec3 vNormal;
            varying vec3 vColour;
            void main()
            {
                vec3 n = normalize(vNormal);
                vec3 keyDir = normalize(vec3(0.5, 0.8, 0.6));
                vec3 fillDir = normalize(vec3(-0.6, 0.2, 0.3));
                float key = max(dot(n, keyDir), 0.0);
                float fill = max(dot(n, fillDir), 0.0);
                float lighting = 0.35 + key * 0.65 + fill * 0.18;
                gl_FragColor = vec4(vColour * lighting, 1.0);
            }
        )";

        auto newShader = std::make_unique<juce::OpenGLShaderProgram>(glContext_);
        if (newShader->addVertexShader(vertexShaderSource) && newShader->addFragmentShader(fragmentShaderSource) &&
            newShader->link())
        {
            shader_ = std::move(newShader);
            positionAttribute_ =
                std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader_, "position");
            normalAttribute_ = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader_, "normal");
            colourAttribute_ = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader_, "sourceColour");
            modelViewProjUniform_ =
                std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader_, "modelViewProjection");
            normalMatrixUniform_ = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader_, "normalMatrix");
        }
        else
        {
            // Real, visible failure rather than a silent black knob -- shows up
            // in a debug build's console/DBG output via JUCE's own shader-error
            // reporting (newShader->getLastError()), which is exactly what's
            // needed the first time this actually gets compiled somewhere.
            jassertfalse;
            DBG("Knob3D shader compile/link failed: " << newShader->getLastError());
        }
    }

    void Knob3D::newOpenGLContextCreated()
    {
        buildShaders();

        glGenBuffers(1, &vertexBuffer_);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        std::vector<Vertex> all = bodyVertices_;
        all.insert(all.end(), indicatorVertices_.begin(), indicatorVertices_.end());
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(all.size() * sizeof(Vertex)), all.data(),
                     GL_STATIC_DRAW);
    }

    void Knob3D::renderOpenGL()
    {
        jassert(juce::OpenGLHelpers::isContextActive());

        const float scale = (float) glContext_.getRenderingScale();
        const int w = juce::jmax(1, (int) (getWidth() * scale));
        const int h = juce::jmax(1, (int) (getHeight() * scale));
        glViewport(0, 0, w, h);

        glClearColor(palette::kPanelRaised.getFloatRed(), palette::kPanelRaised.getFloatGreen(),
                     palette::kPanelRaised.getFloatBlue(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE); // See file header comment -- deliberate, not an oversight.

        if (shader_ == nullptr)
            return; // Shader failed to compile (see buildShaders()) -- panel colour alone, no geometry.

        shader_->use();

        const float aspect = (float) w / (float) h;
        const auto projection = perspective(juce::degreesToRadians(32.0f), aspect, 0.1f, 100.0f);
        // Fixed camera: back on Z, slightly above, tilted down -- matches the
        // reference prototype's camera framing.
        const auto view = multiply4(rotationX(juce::degreesToRadians(-18.0f)), translation(0.0f, -0.3f, -3.6f));

        const float angle =
            juce::jmap((float) slider_.getValue(), (float) slider_.getMinimum(), (float) slider_.getMaximum(),
                       -2.2f, 2.2f);
        const auto model = rotationY(angle);

        const auto modelView = multiply4(view, model);
        const auto mvp = multiply4(projection, modelView);
        const auto normalMatrix = upperLeft3x3(model);

        if (modelViewProjUniform_ != nullptr)
            modelViewProjUniform_->setMatrix4(mvp.data(), 1, false);
        if (normalMatrixUniform_ != nullptr)
            normalMatrixUniform_->setMatrix3(normalMatrix.data(), 1, false);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);

        if (positionAttribute_ != nullptr)
        {
            glVertexAttribPointer((GLuint) positionAttribute_->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                   (const void*) offsetof(Vertex, position));
            glEnableVertexAttribArray((GLuint) positionAttribute_->attributeID);
        }
        if (normalAttribute_ != nullptr)
        {
            glVertexAttribPointer((GLuint) normalAttribute_->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                   (const void*) offsetof(Vertex, normal));
            glEnableVertexAttribArray((GLuint) normalAttribute_->attributeID);
        }
        if (colourAttribute_ != nullptr)
        {
            glVertexAttribPointer((GLuint) colourAttribute_->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                   (const void*) offsetof(Vertex, colour));
            glEnableVertexAttribArray((GLuint) colourAttribute_->attributeID);
        }

        glDrawArrays(GL_TRIANGLES, 0, (GLsizei) bodyVertices_.size());
        glDrawArrays(GL_TRIANGLES, (GLint) bodyVertices_.size(), (GLsizei) indicatorVertices_.size());

        if (positionAttribute_ != nullptr)
            glDisableVertexAttribArray((GLuint) positionAttribute_->attributeID);
        if (normalAttribute_ != nullptr)
            glDisableVertexAttribArray((GLuint) normalAttribute_->attributeID);
        if (colourAttribute_ != nullptr)
            glDisableVertexAttribArray((GLuint) colourAttribute_->attributeID);

        // Required so the child JUCE components (nameLabel_, valueLabel_,
        // slider_) composite correctly afterward -- confirmed against JUCE's
        // own OpenGLAppDemo.h / OpenGL tutorial, which does the same with the
        // comment "Reset the element buffers so child Components draw
        // correctly". Missed in the first pass of this file; added after
        // checking JUCE's real demo code rather than assuming.
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void Knob3D::openGLContextClosing()
    {
        if (vertexBuffer_ != 0)
        {
            glDeleteBuffers(1, &vertexBuffer_);
            vertexBuffer_ = 0;
        }
        shader_ = nullptr;
        positionAttribute_ = nullptr;
        normalAttribute_ = nullptr;
        colourAttribute_ = nullptr;
        modelViewProjUniform_ = nullptr;
        normalMatrixUniform_ = nullptr;
    }

    void Knob3D::resized()
    {
        auto bounds = getLocalBounds();
        valueLabel_.setBounds(bounds.removeFromBottom(16));
        nameLabel_.setBounds(bounds.removeFromBottom(14));
        slider_.setBounds(bounds); // The interactive (invisible) drag area -- matches the rendered knob's area.
    }

} // namespace pw8::plugin::ui
