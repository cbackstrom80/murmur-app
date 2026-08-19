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
# DAW host-matrix pass.
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
        resources/branding/murmur_logo_lockup.png
        resources/branding/murmur_splash_bg.jpg
        resources/branding/murmur_whale_512.png
        resources/fx-glyphs/fx_bypass.svg
        resources/fx-glyphs/fx_sat.svg
        resources/fx-glyphs/fx_chr.svg
        resources/fx-glyphs/fx_tape.svg
        resources/fx-glyphs/fx_mood.svg
        resources/fx-glyphs/fx_fshf.svg
        resources/fx-glyphs/fx_frac.svg
        resources/fx-glyphs/fx_rev.svg
        resources/fx-glyphs/fx_eq.svg
        resources/fx-glyphs/fx_comp.svg
        resources/fx-glyphs/fx_lim.svg
        resources/fx-glyphs/fx_voc.svg
        resources/fx_atlases/chorus_atlas.png
        resources/fx_atlases/tape_atlas.png
        resources/fx_atlases/reverb_atlas.png
        resources/fx_atlases/clouds_atlas.png
)

juce_add_plugin(pw8_plugin
    PRODUCT_NAME "MURMUR"
    COMPANY_NAME "Murmur"
    BUNDLE_ID "com.patchwork.murmur"
    VERSION ${PROJECT_VERSION}
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

# Standalone must use a distinct bundle ID — otherwise macOS pkg install relocates the AU
# into MURMUR.app and Logic never sees MURMUR.component (PackageKit collision).
if(TARGET pw8_plugin_Standalone)
    set_target_properties(pw8_plugin_Standalone PROPERTIES
        JUCE_BUNDLE_ID "com.patchwork.murmur.standalone"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.patchwork.murmur.standalone")
endif()

target_sources(pw8_plugin PRIVATE
    src/processor/MurmurProcessor.cpp
    src/state/PluginState.cpp
    src/content/PresetIndex.cpp
    src/content/WavetableIndex.cpp
    src/content/FavoritesStore.cpp
    src/content/PresetRatingsStore.cpp
    src/content/LicenseStore.cpp
    src/net/MurmurApiClient.cpp
    src/ui/MurmurRootEditor.cpp
    src/ui/DesignModeEditor.cpp
    src/ui/PlayModeEditor.cpp
    src/ui/theme/ObsidianLookAndFeel.cpp
    src/ui/theme/DualKnobLookAndFeel.cpp
    src/ui/theme/BrandingAssets.cpp
    src/ui/theme/FxTypeGlyphs.cpp
    src/ui/components/GlowKnob.cpp
    src/ui/components/ConcentricGlowKnob.cpp
    src/ui/components/LabLauncherChip.cpp
    src/ui/components/TripleGlowKnob.cpp
    src/ui/components/GlowRingButton.cpp
    src/ui/components/SectionPanel.cpp
    src/ui/components/WireframePanel.cpp
    src/ui/components/InterstellarHudDraw.cpp
    src/ui/components/AlgorithmGraphView.cpp
    src/ui/components/EngineIconGrid.cpp
    src/ui/components/EdgeIconGrid.cpp
    src/ui/components/EngineNodeStrip.cpp
    src/ui/components/EngineCard.cpp
    src/ui/components/EngineOscillatorPicker.cpp
    src/ui/components/EngineOscContextThumb.cpp
    src/ui/components/EngineWaveformSelector.cpp
    src/ui/components/EngineAdsrMini.cpp
    src/ui/components/EngineGridPanel.cpp
    src/ui/components/MurmurChromeBar.cpp
    src/ui/components/DashboardStrip.cpp
    src/ui/components/DesignFxPanel.cpp
    src/ui/components/DesignFxCardBrowser.cpp
    src/ui/components/DesignFxUiState.cpp
    src/ui/components/DesignFxPresetLibrary.cpp
    src/ui/components/DesignFxSignalChain.cpp
    src/ui/components/DesignModMatrixPanel.cpp
    src/ui/components/VstBottomBar.cpp
    src/ui/components/VstTopBar.cpp
    src/ui/components/VocoderLabPanel.cpp
    src/ui/components/DualLfoLabPanel.cpp
    src/ui/components/DesignMorphEditorPanel.cpp
    src/ui/components/DesignFilterLabPanel.cpp
    src/ui/components/DesignDynamicsLabPanel.cpp
    src/ui/components/DesignGenerativeLabPanel.cpp
    src/ui/components/DesignUtilityPeaksPanel.cpp
    src/ui/components/DesignEnvelopeSegmentsPanel.cpp
    src/ui/components/MorphTimelineStrip.cpp
    src/ui/components/EngineDetailOverlay.cpp
    src/ui/components/LiveTopologyStrip.cpp
    src/ui/components/TopologyGraphOverlay.cpp
    src/ui/components/ContextStrip.cpp
    src/ui/components/OperatorEditorPanel.cpp
    src/ui/components/MacroStrip.cpp
    src/ui/components/EngineSummingStrip.cpp
    src/ui/components/FilterLfoPanel.cpp
    src/ui/components/FxChainStrip.cpp
    src/ui/components/GlobalPanel.cpp
    src/ui/components/PatchBrowserBar.cpp
    src/ui/components/HeaderSpectrumScope.cpp
    src/ui/components/CircularSpectrumScope.cpp
    src/ui/components/ScopeModeToggle.cpp
    src/ui/components/CompactModeEditor.cpp
    src/ui/components/MetadataFacetRow.cpp
    src/ui/components/PresetBrowserOverlay.cpp
    src/ui/components/SplashOverlay.cpp
    src/ui/components/KeyActivationOverlay.cpp
    src/ui/components/AmpEnvelopePanel.cpp
    src/ui/components/ObsidianEnvelopeVisualizer.cpp
    src/ui/components/ModSourceChip.cpp
    src/ui/components/ModRoutingUi.cpp
    src/ui/components/ModSourcePalette.cpp
    src/ui/components/PatchFocusPanel.cpp
    src/ui/components/MasterEnvelopePanel.cpp
    src/ui/components/MasterMotionLabPanel.cpp
    src/ui/components/MasterQuasarPanel.cpp
    src/ui/components/quasar/QuasarBinauralFieldView.cpp
    src/ui/components/quasar/QuasarChainHeader.cpp
    src/ui/components/quasar/QuasarEngineCard.cpp
    src/ui/components/quasar/QuasarPrimaryKnobRow.cpp
    src/ui/components/quasar/QuasarSegmentedMeter.cpp
    src/ui/components/quasar/QuasarSpatialCard.cpp
    src/ui/components/quasar/QuasarTelemetryBar.cpp
    src/ui/components/IpadPlayMasterStrip.cpp
    src/ui/components/BasicPerformanceSidebar.cpp
    src/ui/components/MasterOutputDeck.cpp
    src/ui/components/ModLauncherPanel.cpp
    src/ui/components/ModRoutingOverlay.cpp
    src/ui/components/ModSourceStrip.cpp
    src/ui/components/OscilloscopeView.cpp
    src/ui/visualizer/MurmurVisualizerComponent.cpp
    src/ui/visualizer/SharedGlVisualizerRoot.cpp
    src/ui/visualizer/VisualPreviewCache.cpp
    src/ui/visualizer/FxCpuPreview.cpp
    src/ui/visualizer/PreviewSurface.cpp
    src/ui/visualizer/FxAnimationAtlas.cpp
    src/ui/visualizer/FxEmbeddedAtlases.cpp
    src/ui/components/FilterPanelScopeView.cpp
    src/ui/components/ArpStepStrip.cpp
    src/ui/components/ArpLauncherChip.cpp
    src/ui/components/ArpPanelOverlay.cpp
    src/ui/components/WavetableLabPanel.cpp
    src/ui/components/WavetableStackView.cpp
    src/ui/components/wireframe/WireframeCanvas.cpp
    src/ui/components/wireframe/OscPreviewSampler.cpp
    src/ui/components/wireframe/WavetableMeshPaint.cpp
    src/ui/components/wireframe/EngineWireframeViews.cpp
    src/ui/components/wireframe/OscWireframeHost.cpp
    src/ui/components/wireframe/EnvelopeCurveView.cpp
    src/ui/components/wireframe/FxWireframeView.cpp
    src/ui/components/wireframe/DesignFxHeroViz.cpp
    src/ui/components/wireframe/FxChainFlowView.cpp
    src/ui/components/wireframe/LfoWireframeView.cpp
    src/ui/components/wireframe/FilterWireframeView.cpp
    src/ui/components/wireframe/FilterRoutingWireframeView.cpp
    src/ui/components/wireframe/ModRoutingWireframeView.cpp
    src/standalone/StandaloneMcpBridge.cpp
)

target_include_directories(pw8_plugin PRIVATE src)

target_link_libraries(pw8_plugin
    PRIVATE
        pw8::core
        pw8_branding_data
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_compile_definitions(pw8_plugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    $<$<CONFIG:Debug>:PW8_DEV_CONTENT_ROOT="${CMAKE_SOURCE_DIR}">
)

pw8_set_warnings(pw8_plugin)
