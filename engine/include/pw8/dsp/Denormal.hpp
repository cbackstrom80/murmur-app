#pragma once

// Denormal (subnormal) handling.
//
// Strategy (documented per docs/DSP_ENGINE.md):
//   1. Flip the CPU's flush-to-zero / denormals-are-zero flags for the lifetime of the
//      audio callback via ScopedDenormalGuard (cheap, x86 SSE + AArch64 both supported).
//   2. In specific feedback paths that are known hotspots for denormal decay tails
//      (filter states, feedback delay lines), also add a tiny DC bias or use
//      flushIfNotFinite() from Math.hpp rather than relying on FTZ alone, since FTZ
//      alone does not protect against NaN/Inf propagation.
//
// We do NOT sprinkle per-sample denormal checks through every hot loop in release
// builds -- that defeats the point. ScopedDenormalGuard covers the common case for
// free at the callback boundary.

#include <cstdint>

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86_FP)
    #include <xmmintrin.h>
    #include <pmmintrin.h>
    #define PW8_HAS_SSE_DENORMAL_CONTROL 1
#elif defined(__aarch64__) || defined(__arm64__)
    #define PW8_HAS_ARM_DENORMAL_CONTROL 1
#endif

namespace pw8::dsp
{
    /// RAII guard that enables flush-to-zero / denormals-are-zero for its lifetime.
    /// Construct one at the top of the audio callback / render block.
    class ScopedDenormalGuard
    {
    public:
        ScopedDenormalGuard() noexcept
        {
#if defined(PW8_HAS_SSE_DENORMAL_CONTROL)
            previousMxcsr_ = _mm_getcsr();
            _mm_setcsr(previousMxcsr_ | 0x8040); // FTZ (bit 15) | DAZ (bit 6)
#elif defined(PW8_HAS_ARM_DENORMAL_CONTROL)
            std::uint64_t fpcr = 0;
            __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
            previousFpcr_ = fpcr;
            fpcr |= (1ULL << 24); // FZ bit
            __asm__ __volatile__("msr fpcr, %0" : : "r"(fpcr));
#endif
        }

        ~ScopedDenormalGuard() noexcept
        {
#if defined(PW8_HAS_SSE_DENORMAL_CONTROL)
            _mm_setcsr(previousMxcsr_);
#elif defined(PW8_HAS_ARM_DENORMAL_CONTROL)
            __asm__ __volatile__("msr fpcr, %0" : : "r"(previousFpcr_));
#endif
        }

        ScopedDenormalGuard(const ScopedDenormalGuard&) = delete;
        ScopedDenormalGuard& operator=(const ScopedDenormalGuard&) = delete;

    private:
#if defined(PW8_HAS_SSE_DENORMAL_CONTROL)
        unsigned int previousMxcsr_ = 0;
#elif defined(PW8_HAS_ARM_DENORMAL_CONTROL)
        std::uint64_t previousFpcr_ = 0;
#endif
    };

} // namespace pw8::dsp
