#include "FathomPreset.h"

#include "FathomParamLayout.h"

namespace pw8::fathom
{
    namespace
    {
        juce::var paramsToVar(const juce::AudioProcessorValueTreeState& apvts)
        {
            juce::DynamicObject::Ptr root = new juce::DynamicObject();
            for (const auto& spec : kFathomParamSpecs)
            {
                if (auto* param = apvts.getParameter(spec.id))
                    root->setProperty(spec.id, param->convertFrom0to1(param->getValue()));
            }
            return juce::var(root.get());
        }

        void applyParamsFromVar(juce::AudioProcessorValueTreeState& apvts, const juce::var& paramsVar)
        {
            if (!paramsVar.isObject())
                return;
            for (const auto& spec : kFathomParamSpecs)
            {
                if (auto* param = apvts.getParameter(spec.id))
                {
                    const auto value = paramsVar.getProperty(spec.id, juce::var());
                    if (!value.isVoid())
                        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(value)));
                }
            }
        }
    } // namespace

    std::optional<juce::String> exportPresetJson(const juce::AudioProcessorValueTreeState& apvts)
    {
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("schemaVersion", kFathomPresetSchemaVersion);
        root->setProperty("params", paramsToVar(apvts));
        return juce::JSON::toString(juce::var(root.get()), true);
    }

    bool importPresetJson(juce::AudioProcessorValueTreeState& apvts, const juce::String& jsonText)
    {
        const auto parsed = juce::JSON::parse(jsonText);
        if (!parsed.isObject())
            return false;

        const int schema = static_cast<int>(parsed.getProperty("schemaVersion", 0));
        if (schema != kFathomPresetSchemaVersion)
            return false;

        applyParamsFromVar(apvts, parsed.getProperty("params", juce::var()));
        return true;
    }

} // namespace pw8::fathom
