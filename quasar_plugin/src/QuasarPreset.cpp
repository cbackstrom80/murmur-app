#include "QuasarPreset.h"

#include "QuasarParamLayout.h"

namespace pw8::quasar
{
    namespace
    {
        juce::var paramsToVar(const juce::AudioProcessorValueTreeState& apvts)
        {
            juce::DynamicObject::Ptr root = new juce::DynamicObject();
            for (const auto& spec : kQuasarParamSpecs)
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
            for (const auto& spec : kQuasarParamSpecs)
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
        root->setProperty("schemaVersion", kQuasarPresetSchemaVersion);
        root->setProperty("params", paramsToVar(apvts));
        return juce::JSON::toString(juce::var(root.get()), true);
    }

    bool importPresetJson(juce::AudioProcessorValueTreeState& apvts, const juce::String& jsonText)
    {
        const auto parsed = juce::JSON::parse(jsonText);
        if (!parsed.isObject())
            return false;

        const int schema = static_cast<int>(parsed.getProperty("schemaVersion", 0));
        if (schema != kQuasarPresetSchemaVersion)
            return false;

        applyParamsFromVar(apvts, parsed.getProperty("params", juce::var()));
        return true;
    }

    juce::File factoryPresetsDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::currentApplicationFile)
            .getParentDirectory()
            .getChildFile("Resources")
            .getChildFile("presets")
            .getChildFile("quasar");
    }

    bool savePresetFile(const juce::File& file, const juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& name, const juce::String& author, const juce::String& description)
    {
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("schemaVersion", kQuasarPresetSchemaVersion);
        juce::DynamicObject::Ptr meta = new juce::DynamicObject();
        meta->setProperty("name", name);
        meta->setProperty("author", author);
        if (description.isNotEmpty())
            meta->setProperty("description", description);
        root->setProperty("metadata", juce::var(meta.get()));
        root->setProperty("params", paramsToVar(apvts));

        return file.getParentDirectory().createDirectory() &&
               file.replaceWithText(juce::JSON::toString(juce::var(root.get()), true));
    }

    bool loadPresetFile(const juce::File& file, juce::AudioProcessorValueTreeState& apvts)
    {
        if (!file.existsAsFile())
            return false;
        return importPresetJson(apvts, file.loadFileAsString());
    }

} // namespace pw8::quasar
