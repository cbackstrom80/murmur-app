#include "state/PluginState.h"

namespace pw8::plugin
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        params.reserve(kMacroParameterIds.size());

        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
        {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{kMacroParameterIds[i], 1}, kMacroParameterNames[i],
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        }

        return {params.begin(), params.end()};
    }

} // namespace pw8::plugin
