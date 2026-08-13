# pw8_plugin -- JUCE VST3/AU/Standalone wrapper around pw8_core.
#
# STATUS: PARTIAL, build-verified. Off by default (PW8_BUILD_PLUGIN=OFF) and not
# built by the main CI job (see .github/workflows/ci.yml's separate macOS-only
# `plugin` job) -- see docs/PLUGIN_ARCHITECTURE.md and docs/ROADMAP.md Phase 16.
# VST3/AU/Standalone all build against real JUCE 8.0.6, and the AU target passes
# Apple's `auval` validation tool in full (format tests, render tests at 22.05k/
# 44.1k/48k/96k/192k Hz and multiple block sizes, MIDI test). createEditor() now
# returns a real custom editor (plugin/src/ui/PlayModeEditor -- the OBSIDIAN skin,
# docs/UI.md), not JUCE's generic parameter list. What's still missing: a real
# DAW host-matrix pass, and DESIGN/LAB modes (PLAY mode is the only mode today).
#
# To attempt a build:
#   cmake --preset plugin && cmake --build --preset plugin
# This fetches JUCE (GPLv3/commercial-dual-licensed -- see THIRD_PARTY_LICENSES.md;
# a commercial JUCE license is required before this plugin can ship closed-source).

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(pw8_branding_data
    SOURCES
        resources/branding/murmur_mark_512.png
)

juce_add_plugin(pw8_plugin
    PRODUCT_NAME "MURMUR"
    COMPANY_NAME "Murmur"
    BUNDLE_ID "com.patchwork.murmur"
    PLUGIN_MANUFACTURER_CODE Murr
    PLUGIN_CODE Murm
    ICON_BIG resources/branding/murmur_mark_512.png
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Instrument" "Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
)

target_sources(pw8_plugin PRIVATE
    src/processor/PatchworkEightProcessor.cpp
    src/state/PluginState.cpp
    src/content/PresetIndex.cpp
    src/content/WavetableIndex.cpp
    src/content/FavoritesStore.cpp
    src/ui/MurmurRootEditor.cpp
    src/ui/DesignModeEditor.cpp
    src/ui/PlayModeEditor.cpp
    src/ui/theme/ObsidianLookAndFeel.cpp
    src/ui/theme/BrandingAssets.cpp
    src/ui/components/GlowKnob.cpp
    src/ui/components/ConcentricGlowKnob.cpp
    src/ui/components/GlowRingButton.cpp
    src/ui/components/SectionPanel.cpp
    src/ui/components/AlgorithmGraphView.cpp
    src/ui/components/AlgorithmGraphEditor.cpp
    src/ui/components/NodeSelectorRow.cpp
    src/ui/components/ContextStrip.cpp
    src/ui/components/OperatorEditorPanel.cpp
    src/ui/components/MacroStrip.cpp
    src/ui/components/EngineSummingStrip.cpp
    src/ui/components/FilterLfoPanel.cpp
    src/ui/components/FxChainStrip.cpp
    src/ui/components/PatchBrowserBar.cpp
    src/ui/components/HeaderSpectrumScope.cpp
    src/ui/components/CircularSpectrumScope.cpp
    src/ui/components/ScopeModeToggle.cpp
    src/ui/components/CompactModeEditor.cpp
    src/ui/components/MetadataFacetRow.cpp
    src/ui/components/PresetBrowserOverlay.cpp
    src/ui/components/AmpEnvelopePanel.cpp
    src/ui/components/ObsidianEnvelopeVisualizer.cpp
    src/ui/components/ModSourceChip.cpp
    src/ui/components/ModRoutingUi.cpp
    src/ui/components/ModSourcePalette.cpp
    src/ui/components/PatchFocusPanel.cpp
    src/ui/components/ModLauncherPanel.cpp
    src/ui/components/ModRoutingOverlay.cpp
    src/ui/components/ModSourceStrip.cpp
    src/ui/components/OscilloscopeView.cpp
    src/ui/components/ArpStepStrip.cpp
    src/ui/components/ArpLauncherChip.cpp
    src/ui/components/ArpPanelOverlay.cpp
    src/ui/components/WavetableStackView.cpp
    src/ui/components/wireframe/WireframeCanvas.cpp
    src/ui/components/wireframe/OscPreviewSampler.cpp
    src/ui/components/wireframe/EngineWireframeViews.cpp
    src/ui/components/wireframe/OscWireframeHost.cpp
    src/ui/components/wireframe/EnvelopeCurveView.cpp
    src/ui/components/wireframe/FxWireframeView.cpp
    src/ui/components/wireframe/FxChainFlowView.cpp
    src/ui/components/wireframe/LfoWireframeView.cpp
    src/ui/components/wireframe/FilterWireframeView.cpp
    src/ui/components/wireframe/ModRoutingWireframeView.cpp
)

target_include_directories(pw8_plugin PRIVATE src)

target_link_libraries(pw8_plugin
    PRIVATE
        pw8::core
        pw8_branding_data
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_compile_definitions(pw8_plugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

pw8_set_warnings(pw8_plugin)
