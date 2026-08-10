#pragma once

#include <complex>
#include <cstddef>
#include <vector>

// Small in-place radix-2 Cooley-Tukey FFT. Control-path only (wavetable
// preprocessing, tests) -- allocates via std::vector and is not used anywhere on
// the audio thread. `data.size()` must be a power of two; this is the standard
// textbook iterative FFT (bit-reversal permutation + butterfly passes), not sourced
// from or specific to any product -- see e.g. any DSP/algorithms reference for the
// same derivation.

namespace pw8::dsp
{
    [[nodiscard]] constexpr bool isPowerOfTwo(std::size_t n) noexcept
    {
        return n != 0 && (n & (n - 1)) == 0;
    }

    /// In-place FFT (invert=false) or IFFT (invert=true, includes the 1/N scaling).
    /// No-op (returns immediately) if data.size() is not a power of two.
    inline void fft(std::vector<std::complex<float>>& data, bool invert) noexcept
    {
        const std::size_t n = data.size();
        if (!isPowerOfTwo(n) || n < 2)
            return;

        for (std::size_t i = 1, j = 0; i < n; ++i)
        {
            std::size_t bit = n >> 1;
            for (; (j & bit) != 0; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(data[i], data[j]);
        }

        for (std::size_t len = 2; len <= n; len <<= 1)
        {
            const float angle = (invert ? 1.0f : -1.0f) * 2.0f * 3.14159265358979323846f / static_cast<float>(len);
            const std::complex<float> wlen(std::cos(angle), std::sin(angle));
            for (std::size_t i = 0; i < n; i += len)
            {
                std::complex<float> w(1.0f, 0.0f);
                for (std::size_t j = 0; j < len / 2; ++j)
                {
                    const std::complex<float> u = data[i + j];
                    const std::complex<float> v = data[i + j + len / 2] * w;
                    data[i + j] = u + v;
                    data[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }

        if (invert)
            for (auto& x : data)
                x /= static_cast<float>(n);
    }

} // namespace pw8::dsp
