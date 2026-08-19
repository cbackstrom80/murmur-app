/*
    AudioVisualizerBus.h

    Single lock-free hub for everything the GUI's OpenGL renderer needs.
    Audio thread WRITES here (cheap, non-blocking, no allocation).
    GUI/render thread READS the latest snapshot once per frame.

    Nothing in here ever locks. The audio thread must never wait on the
    GUI thread under any circumstance, so every field is a plain atomic
    or a fixed-size lock-free FIFO. If you need more channels/grains,
    resize the constants below rather than switching to dynamic containers.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>

namespace murmur8
{

//==============================================================================
// ADSR / envelope state, pushed once per audio block (cheap - not per sample)
struct EnvelopeSnapshot
{
    enum class Stage : int { Idle = 0, Attack, Decay, Sustain, Release };

    std::atomic<Stage> stage { Stage::Idle };
    std::atomic<float> level { 0.0f };          // current envelope output, 0-1
    std::atomic<float> stageProgress { 0.0f };  // 0-1 through current stage
};

//==============================================================================
// VU / peak-RMS, pushed once per audio block. Ballistics (attack/decay smoothing)
// happen in the SHADER via uTime delta, not here - keep this raw.
struct LevelSnapshot
{
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };
    std::atomic<float> rmsL  { 0.0f }, rmsR  { 0.0f };
};

//==============================================================================
// One grain "event" - pushed when a grain fires, aged out by the shader using
// (uTime - onsetTime). GPU-instanced draw reads this whole buffer per frame;
// the audio thread just writes a slot and advances the index.
struct GrainEvent
{
    float onsetTime   = 0.0f;   // seconds, matches uTime timebase
    float duration    = 0.0f;   // seconds
    float pan         = 0.0f;   // -1..1
    float pitchRatio  = 1.0f;
    float amplitude    = 0.0f;
};

//==============================================================================
// Downsampled waveform ring buffer - ONE min/max pair per pixel-column, not
// per sample. This is the #1 place naive plugins blow their CPU/GPU budget:
// never push raw samples at audio rate into a GUI-rate buffer.
template <int NumColumns>
struct WaveformRing
{
    std::array<std::atomic<float>, NumColumns> mins {};
    std::array<std::atomic<float>, NumColumns> maxs {};
    std::atomic<int> writeIndex { 0 };

    void pushColumn (float minVal, float maxVal) noexcept
    {
        const int i = writeIndex.load (std::memory_order_relaxed);
        mins[(size_t) i].store (minVal, std::memory_order_relaxed);
        maxs[(size_t) i].store (maxVal, std::memory_order_relaxed);
        writeIndex.store ((i + 1) % NumColumns, std::memory_order_release);
    }
};

//==============================================================================
/** The whole bus. Own ONE instance of this per plugin editor, shared by the
    audio processor (writer) and every GL visual module (reader). */
class AudioVisualizerBus
{
public:
    static constexpr int maxGrainSlots      = 64;
    static constexpr int fftBinCount        = 128;   // texture width for spectral view
    static constexpr int waveformColumns    = 512;

    AudioVisualizerBus()
    {
        for (auto& magnitude : fftMagnitudes)
            magnitude.store(0.0f, std::memory_order_relaxed);
    }

    //== Audio-thread writers ==============================================
    void pushEnvelope (EnvelopeSnapshot::Stage stage, float level, float stageProgress) noexcept
    {
        envelope.stage.store (stage, std::memory_order_relaxed);
        envelope.level.store (level, std::memory_order_relaxed);
        envelope.stageProgress.store (stageProgress, std::memory_order_relaxed);
    }

    void pushLevels (float pkL, float pkR, float rL, float rR) noexcept
    {
        levels.peakL.store (pkL, std::memory_order_relaxed);
        levels.peakR.store (pkR, std::memory_order_relaxed);
        levels.rmsL.store (rL, std::memory_order_relaxed);
        levels.rmsR.store (rR, std::memory_order_relaxed);
    }

    // Called from the analysis thread (NOT the audio thread) after an FFT pass.
    // Keep this off the realtime audio thread - hop it via a FIFO if your FFT
    // runs inline, or run FFT on a dedicated timer/analysis thread entirely.
    void pushFFTFrame (const float* magnitudes, int count) noexcept
    {
        const int n = juce::jmin (count, fftBinCount);
        for (int i = 0; i < n; ++i)
            fftMagnitudes[(size_t) i].store (magnitudes[i], std::memory_order_relaxed);
        fftGeneration.fetch_add (1, std::memory_order_release);
    }

    void pushGrain (const GrainEvent& g) noexcept
    {
        const int i = grainWriteIndex.load (std::memory_order_relaxed);
        grains[(size_t) i] = g; // POD struct, plain store is fine here
        grainWriteIndex.store ((i + 1) % maxGrainSlots, std::memory_order_release);
    }

    void pushWaveformColumn (float minVal, float maxVal) noexcept
    {
        waveform.pushColumn (minVal, maxVal);
    }

    void setWavetablePosition (float pos01) noexcept
    {
        wavetablePosition.store (pos01, std::memory_order_relaxed);
    }

    //== GUI/render-thread readers ==========================================
    const EnvelopeSnapshot& getEnvelope() const noexcept { return envelope; }
    const LevelSnapshot&    getLevels()   const noexcept { return levels; }
    const WaveformRing<waveformColumns>& getWaveform() const noexcept { return waveform; }

    // Copies out the current FFT texture data - call at GUI frame rate (30-60Hz),
    // never faster than the audio-side pushFFTFrame rate.
    void readFFTInto (std::array<float, fftBinCount>& dest) const noexcept
    {
        for (int i = 0; i < fftBinCount; ++i)
            dest[(size_t) i] = fftMagnitudes[(size_t) i].load (std::memory_order_relaxed);
    }

    int getFFTGeneration() const noexcept { return fftGeneration.load (std::memory_order_acquire); }

    const std::array<GrainEvent, maxGrainSlots>& getGrains() const noexcept { return grains; }
    int getGrainWriteIndex() const noexcept { return grainWriteIndex.load (std::memory_order_acquire); }

    float getWavetablePosition() const noexcept { return wavetablePosition.load (std::memory_order_relaxed); }

private:
    EnvelopeSnapshot envelope;
    LevelSnapshot levels;
    WaveformRing<waveformColumns> waveform;

    std::array<std::atomic<float>, fftBinCount> fftMagnitudes;
    std::atomic<int> fftGeneration { 0 };

    std::array<GrainEvent, maxGrainSlots> grains {};
    std::atomic<int> grainWriteIndex { 0 };

    std::atomic<float> wavetablePosition { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioVisualizerBus)
};

} // namespace murmur8
