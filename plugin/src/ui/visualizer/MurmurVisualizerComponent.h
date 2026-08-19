#pragma once

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

#include "AudioVisualizerBus.h"
#include "VisualizerGpu.h"
#include "VisualizerParams.h"

namespace murmur8
{

class SharedGlVisualizerRoot;

/** GPU preview renderer — one shader mode per visual module; motion on GPU via uTime. */
class MurmurVisualizerComponent : public juce::Component,
                                   private juce::OpenGLRenderer,
                                   private juce::Timer
{
public:
    enum class Mode
    {
        Waveform = 0,
        EnvelopeVu,
        FxPreview,
        QuasarBinaural,
        Spectral,
        Lfo,
        Filter,
        EnvelopeCurve,
        Wavetable,
        Granular,
        VuMeter,
    };

    explicit MurmurVisualizerComponent(AudioVisualizerBus& busToUse);
    ~MurmurVisualizerComponent() override;

    void setMode(Mode newMode);
    [[nodiscard]] Mode getMode() const noexcept { return mode_; }

    void setEnvelopeOnly(bool envelopeOnly);
    void setFxPreviewParams(const FxPreviewParams& params);
    void setQuasarFieldParams(const QuasarFieldParams& params);
    void setLfoPreviewParams(const LfoPreviewParams& params);
    void setFilterPreviewParams(const FilterPreviewParams& params);
    void setEnvelopeCurveParams(const EnvelopeCurveParams& params);
    void setWavetablePreviewParams(const WavetablePreviewParams& params);
    void setGranularPreviewParams(const GranularPreviewParams& params);
    void setVuMeterParams(const VuMeterParams& params);

    [[nodiscard]] static bool isGpuEnabled() noexcept { return visualizerGpuEnabled(); }

    void paint(juce::Graphics&) override;

    void renderSharedViewport(juce::Rectangle<int> boundsInRoot);
    void releaseGlResources();

private:
    friend class SharedGlVisualizerRoot;
    struct WaveformUniforms;
    struct EnvelopeVuUniforms;
    struct FxPreviewUniforms;
    struct QuasarUniforms;
    struct SpectralUniforms;
    struct LfoUniforms;
    struct FilterUniforms;
    struct EnvelopeCurveUniforms;
    struct WavetableUniforms;
    struct GranularUniforms;
    struct VuMeterUniforms;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    void timerCallback() override;

    [[nodiscard]] juce::OpenGLContext& getActiveContext();
    void ensureGlResources();
    void renderFrameImpl();
    bool linkAllPrograms();
    void uploadWaveformTextureIfNeeded();
    void uploadFftTextureIfNeeded();
    void uploadWavetableTextureIfNeeded();
    void rebuildInterleavedWaveformStaging();
    void drawFullscreenQuad(juce::OpenGLShaderProgram& program);

    AudioVisualizerBus& bus;
    bool cpuOnly_ = false;

    juce::OpenGLContext openGLContext;
    SharedGlVisualizerRoot* sharedRoot_ = nullptr;
    bool usesSharedContext_ = false;
    int frameWidth_ = 1;
    int frameHeight_ = 1;
    Mode mode_ = Mode::Waveform;
    bool envelopeOnly_ = false;

    FxPreviewParams fxPreviewParams_ {};
    QuasarFieldParams quasarParams_ {};
    LfoPreviewParams lfoParams_ {};
    FilterPreviewParams filterParams_ {};
    EnvelopeCurveParams envelopeCurveParams_ {};
    WavetablePreviewParams wavetableParams_ {};
    GranularPreviewParams granularParams_ {};
    VuMeterParams vuMeterParams_ {};

    std::unique_ptr<juce::OpenGLShaderProgram> waveformShader;
    std::unique_ptr<juce::OpenGLShaderProgram> envelopeShader;
    std::unique_ptr<juce::OpenGLShaderProgram> fxPreviewShader;
    std::unique_ptr<juce::OpenGLShaderProgram> quasarShader;
    std::unique_ptr<juce::OpenGLShaderProgram> spectralShader;
    std::unique_ptr<juce::OpenGLShaderProgram> lfoShader;
    std::unique_ptr<juce::OpenGLShaderProgram> filterShader;
    std::unique_ptr<juce::OpenGLShaderProgram> envelopeCurveShader;
    std::unique_ptr<juce::OpenGLShaderProgram> wavetableShader;
    std::unique_ptr<juce::OpenGLShaderProgram> granularShader;
    std::unique_ptr<juce::OpenGLShaderProgram> vuMeterShader;

    std::unique_ptr<WaveformUniforms> waveformUniforms;
    std::unique_ptr<EnvelopeVuUniforms> envelopeUniforms;
    std::unique_ptr<FxPreviewUniforms> fxPreviewUniforms;
    std::unique_ptr<QuasarUniforms> quasarUniforms;
    std::unique_ptr<SpectralUniforms> spectralUniforms;
    std::unique_ptr<LfoUniforms> lfoUniforms;
    std::unique_ptr<FilterUniforms> filterUniforms;
    std::unique_ptr<EnvelopeCurveUniforms> envelopeCurveUniforms;
    std::unique_ptr<WavetableUniforms> wavetableUniforms;
    std::unique_ptr<GranularUniforms> granularUniforms;
    std::unique_ptr<VuMeterUniforms> vuMeterUniforms;

    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> positionAttribute_;
    juce::OpenGLShaderProgram* activeProgram_ = nullptr;

    unsigned int vertexBuffer = 0;
    unsigned int waveformTextureId = 0;
    unsigned int fftTextureId = 0;
    unsigned int wavetableTextureId = 0;

    std::array<float, AudioVisualizerBus::waveformColumns * 2> waveformStaging {};
    std::array<float, AudioVisualizerBus::fftBinCount> fftStaging {};
    std::array<float, 256> wavetableStaging {};

    int lastWaveformWriteIndex = -1;
    int lastFftGeneration = -1;
    bool waveformTextureDirty = false;
    bool fftTextureDirty = false;
    bool wavetableTextureDirty = true;

    double startTimeMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MurmurVisualizerComponent)
};

} // namespace murmur8
