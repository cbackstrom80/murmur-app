#include "pw8/oscillator/WavetableTableLoader.hpp"

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

// Untrusted-input hardening, same posture as PatchSerializer.cpp: hard size
// ceiling, bounded loop counts, explicit failure results rather than partial data
// presented as valid.

namespace pw8::oscillator
{
    namespace
    {
        using nlohmann::json;
        constexpr std::size_t kMaxJsonInputBytes = 64ull * 1024 * 1024; // wavetables are bigger than patches.
        constexpr int kMaxFrames = 4096;
        constexpr int kMaxSamplesPerFrame = 16384;
    } // namespace

    WavetableLoadResult loadWavetableFromJson(std::string_view jsonText) noexcept
    {
        WavetableLoadResult result;

        if (jsonText.size() > kMaxJsonInputBytes)
        {
            result.error = "Wavetable JSON exceeds maximum accepted size";
            return result;
        }

        json root;
        try
        {
            root = json::parse(jsonText, nullptr, /*allow_exceptions=*/true);
        }
        catch (const json::parse_error& e)
        {
            result.error = std::string("JSON parse error: ") + e.what();
            return result;
        }

        try
        {
            if (!root.is_object())
            {
                result.error = "Wavetable root is not a JSON object";
                return result;
            }

            const int numFrames = root.value("numFrames", 0);
            const int samplesPerFrame = root.value("samplesPerFrame", 0);
            if (numFrames <= 0 || numFrames > kMaxFrames || samplesPerFrame <= 1 || samplesPerFrame > kMaxSamplesPerFrame)
            {
                result.error = "numFrames/samplesPerFrame out of accepted range";
                return result;
            }

            WavetableTable table;
            table.numFrames = numFrames;
            table.samplesPerFrame = samplesPerFrame;

            if (!root.contains("mips") || !root.at("mips").is_array())
            {
                result.error = "Missing 'mips' array";
                return result;
            }

            const std::size_t expectedSamples = static_cast<std::size_t>(numFrames) * static_cast<std::size_t>(samplesPerFrame);

            for (const auto& jMip : root.at("mips"))
            {
                if (table.mips.size() >= kMaxWavetableMipLevels)
                    break;

                WavetableTable::MipLevel mip;
                mip.maxHarmonic = jMip.value("maxHarmonic", 0);

                if (!jMip.contains("frames") || !jMip.at("frames").is_array())
                    continue;

                mip.samples.reserve(expectedSamples);
                for (const auto& jFrame : jMip.at("frames"))
                {
                    if (!jFrame.is_array())
                        continue;
                    for (const auto& jSample : jFrame)
                    {
                        if (mip.samples.size() >= expectedSamples)
                            break;
                        mip.samples.push_back(jSample.is_number() ? jSample.get<float>() : 0.0f);
                    }
                }

                if (mip.samples.size() != expectedSamples)
                {
                    result.error = "Mip level sample count does not match numFrames * samplesPerFrame";
                    return result;
                }

                table.mips.push_back(std::move(mip));
            }

            if (table.mips.empty())
            {
                result.error = "No valid mip levels found";
                return result;
            }

            result.table = std::move(table);
            result.ok = true;
            return result;
        }
        catch (const std::exception& e)
        {
            result.error = std::string("Wavetable structure error: ") + e.what();
            return result;
        }
    }

    WavetableLoadResult loadWavetableFromFile(const std::string& path) noexcept
    {
        WavetableLoadResult result;
        try
        {
            std::ifstream f(path, std::ios::binary);
            if (!f.is_open())
            {
                result.error = "Could not open file: " + path;
                return result;
            }
            std::ostringstream ss;
            ss << f.rdbuf();
            return loadWavetableFromJson(ss.str());
        }
        catch (const std::exception& e)
        {
            result.error = std::string("Failed to read file: ") + e.what();
            return result;
        }
    }

} // namespace pw8::oscillator
