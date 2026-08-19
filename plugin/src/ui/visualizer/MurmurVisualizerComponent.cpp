#include "MurmurVisualizerComponent.h"

#include "Shaders/EnvelopeVUShaders.h"
#include "Shaders/FxPreviewShader.h"
#include "Shaders/PreviewShaders.h"
#include "Shaders/QuasarBinauralShader.h"
#include "Shaders/WaveformShaders.h"
#include "SharedGlVisualizerRoot.h"
#include "VisualizerGpu.h"

namespace murmur8
{
    struct MurmurVisualizerComponent::WaveformUniforms
    {
        explicit WaveformUniforms(juce::OpenGLShaderProgram& program)
            : uWaveformTex(program, "uWaveformTex"),
              uWaveformColumns(program, "uWaveformColumns"),
              uResolution(program, "uResolution")
        {
        }

        juce::OpenGLShaderProgram::Uniform uWaveformTex, uWaveformColumns, uResolution;
    };

    struct MurmurVisualizerComponent::EnvelopeVuUniforms
    {
        explicit EnvelopeVuUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uResolution(program, "uResolution"),
              uEnvLevel(program, "uEnvLevel"),
              uEnvStage(program, "uEnvStage"),
              uEnvProgress(program, "uEnvProgress"),
              uPeakL(program, "uPeakL"),
              uPeakR(program, "uPeakR"),
              uRmsL(program, "uRmsL"),
              uRmsR(program, "uRmsR"),
              uEnvelopeOnly(program, "uEnvelopeOnly")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uResolution, uEnvLevel, uEnvStage, uEnvProgress, uPeakL, uPeakR,
            uRmsL, uRmsR, uEnvelopeOnly;
    };

    struct MurmurVisualizerComponent::FxPreviewUniforms
    {
        explicit FxPreviewUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uResolution(program, "uResolution"),
              uFxKind(program, "uFxKind"),
              uMix(program, "uMix"),
              uParamsA(program, "uParamsA"),
              uParamsB(program, "uParamsB"),
              uParamsC(program, "uParamsC")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uResolution, uFxKind, uMix, uParamsA, uParamsB, uParamsC;
    };

    struct MurmurVisualizerComponent::QuasarUniforms
    {
        explicit QuasarUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uResolution(program, "uResolution"),
              uQsr1(program, "uQsr1"),
              uQsr2(program, "uQsr2"),
              uCrossfeed(program, "uCrossfeed"),
              uWidth(program, "uWidth"),
              uMix(program, "uMix"),
              uPeakL(program, "uPeakL"),
              uPeakR(program, "uPeakR")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uResolution, uQsr1, uQsr2, uCrossfeed, uWidth, uMix, uPeakL,
            uPeakR;
    };

    struct MurmurVisualizerComponent::SpectralUniforms
    {
        explicit SpectralUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uResolution(program, "uResolution"),
              uFFTTex(program, "uFFTTex"),
              uBinCount(program, "uBinCount")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uResolution, uFFTTex, uBinCount;
    };

    struct MurmurVisualizerComponent::LfoUniforms
    {
        explicit LfoUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uRateHz(program, "uRateHz"),
              uWaveform(program, "uWaveform"),
              uPhase(program, "uPhase")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uRateHz, uWaveform, uPhase;
    };

    struct MurmurVisualizerComponent::FilterUniforms
    {
        explicit FilterUniforms(juce::OpenGLShaderProgram& program)
            : uMode(program, "uMode"),
              uCutoff(program, "uCutoff"),
              uResonance(program, "uResonance")
        {
        }

        juce::OpenGLShaderProgram::Uniform uMode, uCutoff, uResonance;
    };

    struct MurmurVisualizerComponent::EnvelopeCurveUniforms
    {
        explicit EnvelopeCurveUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uADSR(program, "uADSR"),
              uEnvLevel(program, "uEnvLevel"),
              uEnvProgress(program, "uEnvProgress")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uADSR, uEnvLevel, uEnvProgress;
    };

    struct MurmurVisualizerComponent::WavetableUniforms
    {
        explicit WavetableUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uMorph(program, "uMorph"),
              uWaveTex(program, "uWaveTex")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uMorph, uWaveTex;
    };

    struct MurmurVisualizerComponent::GranularUniforms
    {
        explicit GranularUniforms(juce::OpenGLShaderProgram& program)
            : uTime(program, "uTime"),
              uDensity(program, "uDensity"),
              uMix(program, "uMix")
        {
        }

        juce::OpenGLShaderProgram::Uniform uTime, uDensity, uMix;
    };

    struct MurmurVisualizerComponent::VuMeterUniforms
    {
        explicit VuMeterUniforms(juce::OpenGLShaderProgram& program)
            : uPeakL(program, "uPeakL"),
              uPeakR(program, "uPeakR"),
              uRmsL(program, "uRmsL"),
              uRmsR(program, "uRmsR"),
              uVertical(program, "uVertical")
        {
        }

        juce::OpenGLShaderProgram::Uniform uPeakL, uPeakR, uRmsL, uRmsR, uVertical;
    };

    MurmurVisualizerComponent::MurmurVisualizerComponent(AudioVisualizerBus& busToUse) : bus(busToUse)
    {
        setOpaque(false);

        if (!visualizerGpuEnabled())
        {
            cpuOnly_ = true;
        }
        else if (auto* root = SharedGlVisualizerRoot::getInstance())
        {
            usesSharedContext_ = true;
            sharedRoot_ = root;
            root->registerView(*this);
        }
        else
        {
            setOpaque(true);
            openGLContext.setRenderer(this);
            openGLContext.attachTo(*this);
            openGLContext.setContinuousRepainting(true);
        }

        startTimerHz(30);
    }

    MurmurVisualizerComponent::~MurmurVisualizerComponent()
    {
        if (usesSharedContext_ && sharedRoot_ != nullptr)
            sharedRoot_->unregisterView(*this);
        else
            openGLContext.detach();
    }

    juce::OpenGLContext& MurmurVisualizerComponent::getActiveContext()
    {
        if (usesSharedContext_ && sharedRoot_ != nullptr)
            return sharedRoot_->getContext();
        return openGLContext;
    }

    void MurmurVisualizerComponent::setMode(const Mode newMode)
    {
        if (mode_ == newMode)
            return;

        mode_ = newMode;
        positionAttribute_.reset();
        activeProgram_ = nullptr;
        repaint();
    }

    void MurmurVisualizerComponent::setEnvelopeOnly(const bool envelopeOnly)
    {
        envelopeOnly_ = envelopeOnly;
    }

    void MurmurVisualizerComponent::setFxPreviewParams(const FxPreviewParams& params)
    {
        fxPreviewParams_ = params;
    }

    void MurmurVisualizerComponent::setQuasarFieldParams(const QuasarFieldParams& params)
    {
        quasarParams_ = params;
    }

    void MurmurVisualizerComponent::setLfoPreviewParams(const LfoPreviewParams& params)
    {
        lfoParams_ = params;
    }

    void MurmurVisualizerComponent::setFilterPreviewParams(const FilterPreviewParams& params)
    {
        filterParams_ = params;
    }

    void MurmurVisualizerComponent::setEnvelopeCurveParams(const EnvelopeCurveParams& params)
    {
        envelopeCurveParams_ = params;
    }

    void MurmurVisualizerComponent::setWavetablePreviewParams(const WavetablePreviewParams& params)
    {
        wavetableParams_ = params;
        wavetableTextureDirty = true;
    }

    void MurmurVisualizerComponent::setGranularPreviewParams(const GranularPreviewParams& params)
    {
        granularParams_ = params;
    }

    void MurmurVisualizerComponent::setVuMeterParams(const VuMeterParams& params)
    {
        vuMeterParams_ = params;
    }

    void MurmurVisualizerComponent::paint(juce::Graphics&) {}

    bool MurmurVisualizerComponent::linkAllPrograms()
    {
        if (waveformShader != nullptr && envelopeShader != nullptr && fxPreviewShader != nullptr
            && quasarShader != nullptr && spectralShader != nullptr && lfoShader != nullptr
            && filterShader != nullptr && envelopeCurveShader != nullptr && wavetableShader != nullptr
            && granularShader != nullptr && vuMeterShader != nullptr)
            return true;

        const auto linkProgram = [this](std::unique_ptr<juce::OpenGLShaderProgram>& program,
                                         const char* fragmentSource) {
            program = std::make_unique<juce::OpenGLShaderProgram>(getActiveContext());
            return program->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(previewVertexShader))
                   && program->addFragmentShader(
                          juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentSource))
                   && program->link();
        };

        const bool waveformOk =
            (waveformShader = std::make_unique<juce::OpenGLShaderProgram>(getActiveContext()))
            && waveformShader->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(waveformVertexShader))
            && waveformShader->addFragmentShader(
                   juce::OpenGLHelpers::translateFragmentShaderToV3(waveformFragmentShader))
            && waveformShader->link();

        const bool envelopeOk =
            (envelopeShader = std::make_unique<juce::OpenGLShaderProgram>(getActiveContext()))
            && envelopeShader->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(envelopeVuVertexShader))
            && envelopeShader->addFragmentShader(
                   juce::OpenGLHelpers::translateFragmentShaderToV3(envelopeVuFragmentShader))
            && envelopeShader->link();

        const bool fxOk =
            (fxPreviewShader = std::make_unique<juce::OpenGLShaderProgram>(getActiveContext()))
            && fxPreviewShader->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(fxPreviewVertexShader))
            && fxPreviewShader->addFragmentShader(
                   juce::OpenGLHelpers::translateFragmentShaderToV3(fxPreviewFragmentShader))
            && fxPreviewShader->link();

        const bool quasarOk =
            (quasarShader = std::make_unique<juce::OpenGLShaderProgram>(getActiveContext()))
            && quasarShader->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(quasarBinauralVertexShader))
            && quasarShader->addFragmentShader(
                   juce::OpenGLHelpers::translateFragmentShaderToV3(quasarBinauralFragmentShader))
            && quasarShader->link();

        const bool spectralOk = linkProgram(spectralShader, spectralFragmentShader);
        const bool lfoOk = linkProgram(lfoShader, lfoFragmentShader);
        const bool filterOk = linkProgram(filterShader, filterFragmentShader);
        const bool envCurveOk = linkProgram(envelopeCurveShader, envelopeCurveFragmentShader);
        const bool wavetableOk = linkProgram(wavetableShader, wavetableFragmentShader);
        const bool granularOk = linkProgram(granularShader, granularFragmentShader);
        const bool vuOk = linkProgram(vuMeterShader, vuMeterFragmentShader);

        jassert(waveformOk && envelopeOk && fxOk && quasarOk && spectralOk && lfoOk && filterOk && envCurveOk
                && wavetableOk && granularOk && vuOk);

        if (waveformOk)
            waveformUniforms = std::make_unique<WaveformUniforms>(*waveformShader);
        if (envelopeOk)
            envelopeUniforms = std::make_unique<EnvelopeVuUniforms>(*envelopeShader);
        if (fxOk)
            fxPreviewUniforms = std::make_unique<FxPreviewUniforms>(*fxPreviewShader);
        if (quasarOk)
            quasarUniforms = std::make_unique<QuasarUniforms>(*quasarShader);
        if (spectralOk)
            spectralUniforms = std::make_unique<SpectralUniforms>(*spectralShader);
        if (lfoOk)
            lfoUniforms = std::make_unique<LfoUniforms>(*lfoShader);
        if (filterOk)
            filterUniforms = std::make_unique<FilterUniforms>(*filterShader);
        if (envCurveOk)
            envelopeCurveUniforms = std::make_unique<EnvelopeCurveUniforms>(*envelopeCurveShader);
        if (wavetableOk)
            wavetableUniforms = std::make_unique<WavetableUniforms>(*wavetableShader);
        if (granularOk)
            granularUniforms = std::make_unique<GranularUniforms>(*granularShader);
        if (vuOk)
            vuMeterUniforms = std::make_unique<VuMeterUniforms>(*vuMeterShader);

        return waveformOk && envelopeOk && fxOk && quasarOk && spectralOk && lfoOk && filterOk && envCurveOk
               && wavetableOk && granularOk && vuOk;
    }

    void MurmurVisualizerComponent::newOpenGLContextCreated()
    {
        ensureGlResources();
    }

    void MurmurVisualizerComponent::ensureGlResources()
    {
        if (vertexBuffer != 0)
            return;

        using namespace juce::gl;

        startTimeMs = juce::Time::getMillisecondCounterHiRes();

        static const float quad[] = {
            -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
            -1.0f,  1.0f, 1.0f, -1.0f,  1.0f, 1.0f,
        };

        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

        if (!linkAllPrograms())
            return;

        glGenTextures(1, &waveformTextureId);
        glBindTexture(GL_TEXTURE_2D, waveformTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, AudioVisualizerBus::waveformColumns, 1, 0, GL_RG, GL_FLOAT,
                     waveformStaging.data());

        glGenTextures(1, &fftTextureId);
        glBindTexture(GL_TEXTURE_2D, fftTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, AudioVisualizerBus::fftBinCount, 1, 0, GL_RED, GL_FLOAT,
                     fftStaging.data());

        glGenTextures(1, &wavetableTextureId);
        glBindTexture(GL_TEXTURE_2D, wavetableTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, static_cast<GLsizei>(wavetableStaging.size()), 1, 0, GL_RED,
                     GL_FLOAT, wavetableStaging.data());

        rebuildInterleavedWaveformStaging();
        waveformTextureDirty = true;
        fftTextureDirty = true;
        wavetableTextureDirty = true;
    }

    void MurmurVisualizerComponent::renderSharedViewport(const juce::Rectangle<int> boundsInRoot)
    {
        if (boundsInRoot.isEmpty())
            return;

        ensureGlResources();

        using namespace juce::gl;

        frameWidth_ = juce::jmax(1, boundsInRoot.getWidth());
        frameHeight_ = juce::jmax(1, boundsInRoot.getHeight());

        const int rootH = sharedRoot_ != nullptr ? sharedRoot_->getHostHeight() : getHeight();
        const int glY = rootH - boundsInRoot.getBottom();

        glEnable(GL_SCISSOR_TEST);
        glViewport(boundsInRoot.getX(), glY, boundsInRoot.getWidth(), boundsInRoot.getHeight());
        glScissor(boundsInRoot.getX(), glY, boundsInRoot.getWidth(), boundsInRoot.getHeight());

        renderFrameImpl();

        glDisable(GL_SCISSOR_TEST);
    }

    void MurmurVisualizerComponent::releaseGlResources()
    {
        openGLContextClosing();
    }

    void MurmurVisualizerComponent::rebuildInterleavedWaveformStaging()
    {
        const auto& ring = bus.getWaveform();
        for (int i = 0; i < AudioVisualizerBus::waveformColumns; ++i)
        {
            waveformStaging[static_cast<std::size_t>(i) * 2] =
                ring.mins[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
            waveformStaging[static_cast<std::size_t>(i) * 2 + 1] =
                ring.maxs[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        }
    }

    void MurmurVisualizerComponent::uploadWaveformTextureIfNeeded()
    {
        if (!waveformTextureDirty || waveformTextureId == 0)
            return;

        using namespace juce::gl;

        glBindTexture(GL_TEXTURE_2D, waveformTextureId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AudioVisualizerBus::waveformColumns, 1, GL_RG, GL_FLOAT,
                        waveformStaging.data());
        waveformTextureDirty = false;
    }

    void MurmurVisualizerComponent::uploadFftTextureIfNeeded()
    {
        if (!fftTextureDirty || fftTextureId == 0)
            return;

        using namespace juce::gl;

        glBindTexture(GL_TEXTURE_2D, fftTextureId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AudioVisualizerBus::fftBinCount, 1, GL_RED, GL_FLOAT,
                        fftStaging.data());
        fftTextureDirty = false;
    }

    void MurmurVisualizerComponent::uploadWavetableTextureIfNeeded()
    {
        if (!wavetableTextureDirty || wavetableTextureId == 0)
            return;

        const float morph = juce::jlimit(0.0f, 1.0f, wavetableParams_.morph);
        for (int i = 0; i < static_cast<int>(wavetableStaging.size()); ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(wavetableStaging.size() - 1);
            const float sine = std::sin(t * juce::MathConstants<float>::twoPi);
            const float tri = 1.0f - 4.0f * std::abs(std::fmod(t + 0.25f, 1.0f) - 0.5f);
            const float sample = sine * (1.0f - morph) + tri * morph;
            wavetableStaging[static_cast<std::size_t>(i)] = sample * 0.5f + 0.5f;
        }

        using namespace juce::gl;

        glBindTexture(GL_TEXTURE_2D, wavetableTextureId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(wavetableStaging.size()), 1, GL_RED, GL_FLOAT,
                        wavetableStaging.data());
        wavetableTextureDirty = false;
    }

    void MurmurVisualizerComponent::drawFullscreenQuad(juce::OpenGLShaderProgram& program)
    {
        using namespace juce::gl;

        if (activeProgram_ != &program)
        {
            positionAttribute_ = std::make_unique<juce::OpenGLShaderProgram::Attribute>(program, "position");
            activeProgram_ = &program;
        }

        if (positionAttribute_ == nullptr || positionAttribute_->attributeID == 0)
            return;

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(static_cast<GLuint>(positionAttribute_->attributeID), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(static_cast<GLuint>(positionAttribute_->attributeID));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(static_cast<GLuint>(positionAttribute_->attributeID));
    }

    void MurmurVisualizerComponent::renderOpenGL()
    {
        frameWidth_ = juce::jmax(1, getWidth());
        frameHeight_ = juce::jmax(1, getHeight());
        renderFrameImpl();
    }

    void MurmurVisualizerComponent::renderFrameImpl()
    {
        juce::OpenGLHelpers::clear(juce::Colour(0xff0a0b0e));

        if (vertexBuffer == 0 || !linkAllPrograms())
            return;

        const float t =
            static_cast<float>((juce::Time::getMillisecondCounterHiRes() - startTimeMs) * 0.001);

        const float resW = static_cast<float>(frameWidth_);
        const float resH = static_cast<float>(frameHeight_);

        if (mode_ == Mode::Waveform)
        {
            if (waveformShader == nullptr || waveformUniforms == nullptr)
                return;

            uploadWaveformTextureIfNeeded();
            waveformShader->use();
            waveformUniforms->uResolution.set(resW, resH);
            waveformUniforms->uWaveformColumns.set(static_cast<float>(AudioVisualizerBus::waveformColumns));

            using namespace juce::gl;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, waveformTextureId);
            waveformUniforms->uWaveformTex.set(0);

            drawFullscreenQuad(*waveformShader);
            return;
        }

        if (mode_ == Mode::EnvelopeVu)
        {
            if (envelopeShader == nullptr || envelopeUniforms == nullptr)
                return;

            envelopeShader->use();
            const auto& env = bus.getEnvelope();
            const auto& lv = bus.getLevels();

            envelopeUniforms->uTime.set(t);
            envelopeUniforms->uResolution.set(resW, resH);
            envelopeUniforms->uEnvLevel.set(env.level.load(std::memory_order_relaxed));
            envelopeUniforms->uEnvStage.set(
                static_cast<float>(static_cast<int>(env.stage.load(std::memory_order_relaxed))));
            envelopeUniforms->uEnvProgress.set(env.stageProgress.load(std::memory_order_relaxed));
            envelopeUniforms->uPeakL.set(lv.peakL.load(std::memory_order_relaxed));
            envelopeUniforms->uPeakR.set(lv.peakR.load(std::memory_order_relaxed));
            envelopeUniforms->uRmsL.set(lv.rmsL.load(std::memory_order_relaxed));
            envelopeUniforms->uRmsR.set(lv.rmsR.load(std::memory_order_relaxed));
            envelopeUniforms->uEnvelopeOnly.set(envelopeOnly_ ? 1.0f : 0.0f);

            drawFullscreenQuad(*envelopeShader);
            return;
        }

        if (mode_ == Mode::FxPreview)
        {
            if (fxPreviewShader == nullptr || fxPreviewUniforms == nullptr)
                return;

            fxPreviewShader->use();
            const auto& fx = fxPreviewParams_;

            fxPreviewUniforms->uTime.set(t);
            fxPreviewUniforms->uResolution.set(resW, resH);
            fxPreviewUniforms->uFxKind.set(static_cast<float>(fx.fxKind));
            fxPreviewUniforms->uMix.set(fx.mix);
            fxPreviewUniforms->uParamsA.set(fx.paramA0, fx.paramA1, fx.paramA2, fx.paramA3);
            fxPreviewUniforms->uParamsB.set(fx.paramB0, fx.paramB1, fx.paramB2, fx.paramB3);
            fxPreviewUniforms->uParamsC.set(fx.paramC0, fx.paramC1, fx.paramC2, fx.paramC3);

            drawFullscreenQuad(*fxPreviewShader);
            return;
        }

        if (mode_ == Mode::QuasarBinaural)
        {
            if (quasarShader == nullptr || quasarUniforms == nullptr)
                return;

            quasarShader->use();
            const auto& q = quasarParams_;
            const auto& lv = bus.getLevels();

            quasarUniforms->uTime.set(t);
            quasarUniforms->uResolution.set(resW, resH);
            quasarUniforms->uQsr1.set(q.qsr1AngleDeg, q.qsr1Distance, q.qsr1Height, q.qsr1Level);
            quasarUniforms->uQsr2.set(q.qsr2AngleDeg, q.qsr2Distance, q.qsr2Height, q.qsr2Level);
            quasarUniforms->uCrossfeed.set(q.crossfeed);
            quasarUniforms->uWidth.set(q.width);
            quasarUniforms->uMix.set(q.mix);
            quasarUniforms->uPeakL.set(lv.peakL.load(std::memory_order_relaxed));
            quasarUniforms->uPeakR.set(lv.peakR.load(std::memory_order_relaxed));

            drawFullscreenQuad(*quasarShader);
            return;
        }

        if (mode_ == Mode::Spectral)
        {
            if (spectralShader == nullptr || spectralUniforms == nullptr)
                return;

            uploadFftTextureIfNeeded();
            spectralShader->use();
            spectralUniforms->uTime.set(t);
            spectralUniforms->uResolution.set(resW, resH);
            spectralUniforms->uBinCount.set(static_cast<float>(AudioVisualizerBus::fftBinCount));

            using namespace juce::gl;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fftTextureId);
            spectralUniforms->uFFTTex.set(0);

            drawFullscreenQuad(*spectralShader);
            return;
        }

        if (mode_ == Mode::Lfo)
        {
            if (lfoShader == nullptr || lfoUniforms == nullptr)
                return;

            lfoShader->use();
            const auto& lfo = lfoParams_;
            lfoUniforms->uTime.set(t);
            lfoUniforms->uRateHz.set(lfo.rateHz);
            lfoUniforms->uWaveform.set(lfo.waveform);
            lfoUniforms->uPhase.set(lfo.phase);

            drawFullscreenQuad(*lfoShader);
            return;
        }

        if (mode_ == Mode::Filter)
        {
            if (filterShader == nullptr || filterUniforms == nullptr)
                return;

            filterShader->use();
            const auto& f = filterParams_;
            filterUniforms->uMode.set(static_cast<float>(f.mode));
            filterUniforms->uCutoff.set(f.cutoffNorm);
            filterUniforms->uResonance.set(f.resonance);

            drawFullscreenQuad(*filterShader);
            return;
        }

        if (mode_ == Mode::EnvelopeCurve)
        {
            if (envelopeCurveShader == nullptr || envelopeCurveUniforms == nullptr)
                return;

            envelopeCurveShader->use();
            const auto& e = envelopeCurveParams_;
            const auto& env = bus.getEnvelope();

            envelopeCurveUniforms->uTime.set(t);
            envelopeCurveUniforms->uADSR.set(e.attackNorm, e.decayNorm, e.sustain, e.releaseNorm);
            envelopeCurveUniforms->uEnvLevel.set(env.level.load(std::memory_order_relaxed));
            envelopeCurveUniforms->uEnvProgress.set(env.stageProgress.load(std::memory_order_relaxed));

            drawFullscreenQuad(*envelopeCurveShader);
            return;
        }

        if (mode_ == Mode::Wavetable)
        {
            if (wavetableShader == nullptr || wavetableUniforms == nullptr)
                return;

            uploadWavetableTextureIfNeeded();
            wavetableShader->use();
            wavetableUniforms->uTime.set(t);
            wavetableUniforms->uMorph.set(wavetableParams_.morph);

            using namespace juce::gl;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, wavetableTextureId);
            wavetableUniforms->uWaveTex.set(0);

            drawFullscreenQuad(*wavetableShader);
            return;
        }

        if (mode_ == Mode::Granular)
        {
            if (granularShader == nullptr || granularUniforms == nullptr)
                return;

            granularShader->use();
            granularUniforms->uTime.set(t);
            granularUniforms->uDensity.set(granularParams_.density);
            granularUniforms->uMix.set(granularParams_.mix);

            drawFullscreenQuad(*granularShader);
            return;
        }

        if (mode_ == Mode::VuMeter)
        {
            if (vuMeterShader == nullptr || vuMeterUniforms == nullptr)
                return;

            vuMeterShader->use();
            const auto& v = vuMeterParams_;
            vuMeterUniforms->uPeakL.set(v.peakL);
            vuMeterUniforms->uPeakR.set(v.peakR);
            vuMeterUniforms->uRmsL.set(v.rmsL);
            vuMeterUniforms->uRmsR.set(v.rmsR);
            vuMeterUniforms->uVertical.set(v.vertical ? 1.0f : 0.0f);

            drawFullscreenQuad(*vuMeterShader);
        }
    }

    void MurmurVisualizerComponent::openGLContextClosing()
    {
        using namespace juce::gl;

        if (waveformTextureId != 0)
        {
            glDeleteTextures(1, &waveformTextureId);
            waveformTextureId = 0;
        }

        if (fftTextureId != 0)
        {
            glDeleteTextures(1, &fftTextureId);
            fftTextureId = 0;
        }

        if (wavetableTextureId != 0)
        {
            glDeleteTextures(1, &wavetableTextureId);
            wavetableTextureId = 0;
        }

        if (vertexBuffer != 0)
        {
            glDeleteBuffers(1, &vertexBuffer);
            vertexBuffer = 0;
        }

        positionAttribute_.reset();
        activeProgram_ = nullptr;
        waveformUniforms.reset();
        envelopeUniforms.reset();
        fxPreviewUniforms.reset();
        quasarUniforms.reset();
        spectralUniforms.reset();
        lfoUniforms.reset();
        filterUniforms.reset();
        envelopeCurveUniforms.reset();
        wavetableUniforms.reset();
        granularUniforms.reset();
        vuMeterUniforms.reset();
        waveformShader.reset();
        envelopeShader.reset();
        fxPreviewShader.reset();
        quasarShader.reset();
        spectralShader.reset();
        lfoShader.reset();
        filterShader.reset();
        envelopeCurveShader.reset();
        wavetableShader.reset();
        granularShader.reset();
        vuMeterShader.reset();
    }

    void MurmurVisualizerComponent::timerCallback()
    {
        const int writeIndex = bus.getWaveform().writeIndex.load(std::memory_order_acquire);
        if (writeIndex != lastWaveformWriteIndex)
        {
            lastWaveformWriteIndex = writeIndex;
            rebuildInterleavedWaveformStaging();
            waveformTextureDirty = true;
        }

        const int fftGen = bus.getFFTGeneration();
        if (fftGen != lastFftGeneration)
        {
            lastFftGeneration = fftGen;
            bus.readFFTInto(fftStaging);
            fftTextureDirty = true;
        }

        const float wtPos = bus.getWavetablePosition();
        if (std::abs(wtPos - wavetableParams_.morph) > 0.001f)
        {
            wavetableParams_.morph = wtPos;
            wavetableTextureDirty = true;
        }
        if (usesSharedContext_ && sharedRoot_ != nullptr)
            sharedRoot_->requestRender();
        else if (!usesSharedContext_)
            repaint();
    }

} // namespace murmur8
