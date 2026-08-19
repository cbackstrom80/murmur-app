#pragma once

namespace murmur8
{

/** GPU visualizers are disabled in plugin hosts until OpenGL compositing is verified (Logic AU). */
inline bool visualizerGpuEnabled() noexcept
{
#if defined(MURMUR_ENABLE_GPU_VISUALIZERS) && MURMUR_ENABLE_GPU_VISUALIZERS
    return true;
#else
    return false;
#endif
}

} // namespace murmur8
