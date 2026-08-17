#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

#include "WireframeProjection.h"
#include "pw8/oscillator/WavetableTable.hpp"
#include "pw8/oscillator/WavetableWarp.hpp"

namespace pw8::plugin::ui::wireframe
{
    struct WavetableMeshPaintOptions
    {
        int meshRows = 15;
        int pointsPerRow = 48;
        int crossLineStride = 6;
        MeshLayout layout{};
    };

    struct GranularOverlayParams
    {
        float wavetablePos = 0.35f;
        float grainSizeMs = 60.0f;
    };

    /// Full-fidelity mesh from a loaded wavetable table (same interpolation as WavetableOscillator).
    void paintWavetableTableMesh(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const oscillator::WavetableTable* table,
                                 const oscillator::WtWarpParams& warpParams, float livePos01,
                                 const WavetableMeshPaintOptions& options);

    /// Grain-window rectangles drawn over the wavetable mesh (Granular engine).
    void paintGranularGrainOverlay(juce::Graphics& g, juce::Rectangle<float> bounds,
                                   const GranularOverlayParams& params);

    /// WavetableLabPanel center column — tall hero mesh (~400×300+).
    [[nodiscard]] WavetableMeshPaintOptions labHeroMeshOptions();

    /// EngineCard design-v2 context strip — wide, ~80px tall (279×80 usable).
    [[nodiscard]] WavetableMeshPaintOptions contextThumbMeshOptions();

    /// Sample wavetable amplitude at frame position + cycle phase (same path as mesh painter).
    [[nodiscard]] float sampleWavetableAt(const oscillator::WavetableTable* table, float framePos01, float phase01,
                                          const oscillator::WtWarpParams& warpParams);

    /// Flat waveform for one table frame — frame strip minis (70×50) and legacy 36×14 cells.
    void paintWavetableFrameWaveform(juce::Graphics& g, juce::Rectangle<float> bounds,
                                     const oscillator::WavetableTable* table, float framePos01,
                                     const oscillator::WtWarpParams& warpParams, bool liveGlow = false,
                                     int points = 32);

    /// First 16 harmonic magnitudes from the frame nearest `framePos01` (normalized 0–1).
    void computeWavetableHarmonicMagnitudes(const oscillator::WavetableTable* table, float framePos01,
                                            const oscillator::WtWarpParams& warpParams,
                                            std::array<float, 16>& magnitudesOut);

} // namespace pw8::plugin::ui::wireframe
