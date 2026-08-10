#include "pw8/render/WavWriter.hpp"

#include <cstdint>
#include <cstring>
#include <fstream>

namespace pw8::render
{
    namespace
    {
        void writeU32(std::ofstream& f, std::uint32_t v)
        {
            f.write(reinterpret_cast<const char*>(&v), sizeof(v));
        }

        void writeU16(std::ofstream& f, std::uint16_t v)
        {
            f.write(reinterpret_cast<const char*>(&v), sizeof(v));
        }
    } // namespace

    bool writeWavFileFloat32(const std::string& path, const std::vector<float>& interleavedStereo,
                              double sampleRate) noexcept
    {
        try
        {
            std::ofstream f(path, std::ios::binary);
            if (!f.is_open())
                return false;

            constexpr std::uint16_t kNumChannels = 2;
            constexpr std::uint16_t kBitsPerSample = 32;
            constexpr std::uint16_t kFormatTagIeeeFloat = 3;

            const auto sr = static_cast<std::uint32_t>(sampleRate);
            const std::uint32_t byteRate = sr * kNumChannels * (kBitsPerSample / 8);
            const std::uint16_t blockAlign = kNumChannels * (kBitsPerSample / 8);
            const std::uint32_t dataBytes = static_cast<std::uint32_t>(interleavedStereo.size() * sizeof(float));

            f.write("RIFF", 4);
            writeU32(f, 36 + dataBytes);
            f.write("WAVE", 4);

            f.write("fmt ", 4);
            writeU32(f, 16); // PCM-float fmt chunk size
            writeU16(f, kFormatTagIeeeFloat);
            writeU16(f, kNumChannels);
            writeU32(f, sr);
            writeU32(f, byteRate);
            writeU16(f, blockAlign);
            writeU16(f, kBitsPerSample);

            f.write("data", 4);
            writeU32(f, dataBytes);
            if (!interleavedStereo.empty())
                f.write(reinterpret_cast<const char*>(interleavedStereo.data()), dataBytes);

            return f.good();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

} // namespace pw8::render
