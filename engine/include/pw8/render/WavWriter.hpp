#pragma once

#include <string>
#include <vector>

// Minimal, dependency-free WAV writer. Writes 32-bit IEEE-float PCM (format tag 3),
// which sidesteps dithering/clipping decisions entirely and matches the renderer's
// internal float pipeline exactly -- appropriate for a native offline/QA renderer.
// A 16/24-bit integer PCM path can be added later for delivery formats without
// touching the render pipeline itself.

namespace pw8::render
{
    [[nodiscard]] bool writeWavFileFloat32(const std::string& path, const std::vector<float>& interleavedStereo,
                                            double sampleRate) noexcept;

} // namespace pw8::render
