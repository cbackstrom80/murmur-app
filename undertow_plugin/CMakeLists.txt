# pw8_undertow_plugin -- Undertow, a real genuinely separate JUCE VST3/AU/
# Standalone plugin binary (own bundle ID, own splash, own installer per
# docs/UNDERTOW.md), NOT a mode inside MURMUR. Off by default
# (PW8_BUILD_UNDERTOW_PLUGIN=OFF).
#
# Reuses MURMUR's real engine (pw8::core) and its real, already-shipped UI
# component library by REFERENCING the same files under plugin/src/ by
# relative path (target_sources() below) -- not a fork. This is the lesson
# learned from QUASAR's 2026-08-14 standalone extraction
# (docs/QUASAR_STANDALONE_PLUGIN.md, docs/QUASAR_RETURN_PLAN.md): its UI/
# theme library was a full duplicated copy that drifted the moment it
# existed. Editing a file under plugin/src/ affects both binaries on the
# next rebuild here; there is exactly one real copy of each shared source
# file on disk.
#
# The only genuinely new/product-specific pieces are: this CMakeLists.txt's
# own juce_add_plugin() identity (PRODUCT_NAME/BUNDLE_ID/PLUGIN_CODE), the
# real Undertow branding assets (resources/branding/undertow_squid_512.png,
# pulled from the approved undertow-vst-splash Figma frame, node 277:918),
# and src/UndertowPluginMain.cpp (JUCE requires exactly one
# createPluginFilter() per binary).
#
# To attempt a build:
#   cmake -B build -DPW8_BUILD_UNDERTOW_PLUGIN=ON && cmake --build build --target pw8_undertow_plugin_Standalone

include(FetchContent)

if(NOT TARGET juce::juce_audio_utils)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(pw8_undertow_branding_data
    SOURCES
        ../plugin/resources/branding/murmur_mark_512.png
        ../plugin/resources/branding/murmur_logo_lockup.png
        ../plugin/resources/branding/murmur_splash_bg.jpg
        ../plugin/resources/branding/murmur_whale_512.png
        ../plugin/resources/branding/undertow_squid_512.png
        ../plugin/resources/fx-glyphs/fx_bypass.svg
        ../plugin/resources/fx-glyphs/fx_sat.svg
        ../plugin/resources/fx-glyphs/fx_chr.svg
        ../plugin/resources/fx-glyphs/fx_tape.svg
        ../plugin/resources/fx-glyphs/fx_mood.svg
        ../plugin/resources/fx-glyphs/fx_fshf.svg
        ../plugin/resources/fx-glyphs/fx_frac.svg
        ../plugin/resources/fx-glyphs/fx_rev.svg
        ../plugin/resources/fx-glyphs/fx_eq.svg
        ../plugin/resources/fx-glyphs/fx_comp.svg
        ../plugin/resources/fx-glyphs/fx_lim.svg
        ../plugin/resources/fx-glyphs/fx_voc.svg
        ../plugin/resources/fx_atlases/chorus_atlas.png
        ../plugin/resources/fx_atlases/tape_atlas.png
        ../plugin/resources/fx_atlases/reverb_atlas.png
        ../plugin/resources/fx_atlases/clouds_atlas.png
)

juce_add_plugin(pw8_undertow_plugin
    PRODUCT_NAME "Undertow"
    COMPANY_NAME "Murmur"
    BUNDLE_ID "com.patchwork.undertow"
    VERSION 1.0.0
    PLUGIN_MANUFACTURER_CODE Murr
    PLUGIN_CODE Undr
    ICON_BIG ../plugin/resources/branding/undertow_squid_512.png
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Instrument" "Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
)

# Same real reason as plugin/CMakeLists.txt's MURMUR Standalone: a distinct
# bundle ID keeps macOS pkg install from relocating the AU into
# Undertow.app, which would hide Undertow.component from the DAW.
if(TARGET pw8_undertow_plugin_Standalone)
    set_target_properties(pw8_undertow_plugin_Standalone PROPERTIES
        JUCE_BUNDLE_ID "com.patchwork.undertow.standalone"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.patchwork.undertow.standalone")
endif()

target_sources(pw8_undertow_plugin PRIVATE
    src/UndertowPluginMain.cpp
    ../plugin/src/processor/MurmurProcessor.cpp
    ../plugin/src/state/PluginState.cpp
    ../plugin/src/content/PresetIndex.cpp
    ../plugin/src/content/WavetableIndex.cpp
    ../plugin/src/content/FavoritesStore.cpp
    ../plugin/src/content/PresetRatingsStore.cpp
    ../plugin/src/content/LicenseStore.cpp
    ../plugin/src/net/MurmurApiClient.cpp
    ../plugin/src/net/UpdateChecker.cpp
    ../plugin/src/ui/MurmurRootEditor.cpp
    ../plugin/src/ui/DesignModeEditor.cpp
    ../plugin/src/ui/PlayModeEditor.cpp
    ../plugin/src/ui/theme/ObsidianLookAndFeel.cpp
    ../plugin/src/ui/theme/DualKnobLookAndFeel.cpp
    ../plugin/src/ui/theme/BrandingAssets.cpp
    ../plugin/src/ui/theme/FxTypeGlyphs.cpp
    ../plugin/src/ui/components/GlowKnob.cpp
    ../plugin/src/ui/components/ConcentricGlowKnob.cpp
    ../plugin/src/ui/components/LabLauncherChip.cpp
    ../plugin/src/ui/components/TripleGlowKnob.cpp
    ../plugin/src/ui/components/GlowRingButton.cpp
    ../plugin/src/ui/components/SectionPanel.cpp
    ../plugin/src/ui/components/SubAnchorMeter.cpp
    ../plugin/src/ui/components/WireframePanel.cpp
    ../plugin/src/ui/components/InterstellarHudDraw.cpp
    ../plugin/src/ui/components/AlgorithmGraphView.cpp
    ../plugin/src/ui/components/EngineIconGrid.cpp
    ../plugin/src/ui/components/EdgeIconGrid.cpp
    ../plugin/src/ui/components/EngineNodeStrip.cpp
    ../plugin/src/ui/components/EngineCard.cpp
    ../plugin/src/ui/components/EngineOscillatorPicker.cpp
    ../plugin/src/ui/components/EngineOscContextThumb.cpp
    ../plugin/src/ui/components/EngineWaveformSelector.cpp
    ../plugin/src/ui/components/EngineAdsrMini.cpp
    ../plugin/src/ui/components/EngineGridPanel.cpp
    ../plugin/src/ui/components/MurmurChromeBar.cpp
    ../plugin/src/ui/components/DashboardStrip.cpp
    ../plugin/src/ui/components/DesignFxPanel.cpp
    ../plugin/src/ui/components/DesignFxCardBrowser.cpp
    ../plugin/src/ui/components/DesignFxUiState.cpp
    ../plugin/src/ui/components/DesignFxPresetLibrary.cpp
    ../plugin/src/ui/components/DesignFxSignalChain.cpp
    ../plugin/src/ui/components/DesignModMatrixPanel.cpp
    ../plugin/src/ui/components/VstBottomBar.cpp
    ../plugin/src/ui/components/VstTopBar.cpp
    ../plugin/src/ui/components/VocoderLabPanel.cpp
    ../plugin/src/ui/components/DualLfoLabPanel.cpp
    ../plugin/src/ui/components/DesignMorphEditorPanel.cpp
    ../plugin/src/ui/components/DesignFilterLabPanel.cpp
    ../plugin/src/ui/components/DesignDynamicsLabPanel.cpp
    ../plugin/src/ui/components/DesignGenerativeLabPanel.cpp
    ../plugin/src/ui/components/DesignUtilityPeaksPanel.cpp
    ../plugin/src/ui/components/DesignEnvelopeSegmentsPanel.cpp
    ../plugin/src/ui/components/MorphTimelineStrip.cpp
    ../plugin/src/ui/components/EngineDetailOverlay.cpp
    ../plugin/src/ui/components/LiveTopologyStrip.cpp
    ../plugin/src/ui/components/TopologyGraphOverlay.cpp
    ../plugin/src/ui/components/ContextStrip.cpp
    ../plugin/src/ui/components/OperatorEditorPanel.cpp
    ../plugin/src/ui/components/MacroStrip.cpp
    ../plugin/src/ui/components/EngineSummingStrip.cpp
    ../plugin/src/ui/components/FilterLfoPanel.cpp
    ../plugin/src/ui/components/FxChainStrip.cpp
    ../plugin/src/ui/components/GlobalPanel.cpp
    ../plugin/src/ui/components/PatchBrowserBar.cpp
    ../plugin/src/ui/components/HeaderSpectrumScope.cpp
    ../plugin/src/ui/components/CircularSpectrumScope.cpp
    ../plugin/src/ui/components/ScopeModeToggle.cpp
    ../plugin/src/ui/components/CompactModeEditor.cpp
    ../plugin/src/ui/components/MetadataFacetRow.cpp
    ../plugin/src/ui/components/PresetBrowserOverlay.cpp
    ../plugin/src/ui/components/SplashOverlay.cpp
    ../plugin/src/ui/components/KeyActivationOverlay.cpp
    ../plugin/src/ui/components/CuratorReviewOverlay.cpp
    ../plugin/src/ui/components/UpdateBadge.cpp
    ../plugin/src/ui/components/AmpEnvelopePanel.cpp
    ../plugin/src/ui/components/ObsidianEnvelopeVisualizer.cpp
    ../plugin/src/ui/components/ModSourceChip.cpp
    ../plugin/src/ui/components/ModRoutingUi.cpp
    ../plugin/src/ui/components/ModSourcePalette.cpp
    ../plugin/src/ui/components/PatchFocusPanel.cpp
    ../plugin/src/ui/components/MasterEnvelopePanel.cpp
    ../plugin/src/ui/components/MasterMotionLabPanel.cpp
    ../plugin/src/ui/components/MasterQuasarPanel.cpp
    ../plugin/src/ui/components/quasar/QuasarBinauralFieldView.cpp
    ../plugin/src/ui/components/quasar/QuasarChainHeader.cpp
    ../plugin/src/ui/components/quasar/QuasarEngineCard.cpp
    ../plugin/src/ui/components/quasar/QuasarPrimaryKnobRow.cpp
    ../plugin/src/ui/components/quasar/QuasarSegmentedMeter.cpp
    ../plugin/src/ui/components/quasar/QuasarSpatialCard.cpp
    ../plugin/src/ui/components/quasar/QuasarTelemetryBar.cpp
    ../plugin/src/ui/components/IpadPlayMasterStrip.cpp
    ../plugin/src/ui/components/BasicPerformanceSidebar.cpp
    ../plugin/src/ui/components/MasterOutputDeck.cpp
    ../plugin/src/ui/components/ModLauncherPanel.cpp
    ../plugin/src/ui/components/ModRoutingOverlay.cpp
    ../plugin/src/ui/components/ModSourceStrip.cpp
    ../plugin/src/ui/components/OscilloscopeView.cpp
    ../plugin/src/ui/visualizer/MurmurVisualizerComponent.cpp
    ../plugin/src/ui/visualizer/SharedGlVisualizerRoot.cpp
    ../plugin/src/ui/visualizer/VisualPreviewCache.cpp
    ../plugin/src/ui/visualizer/FxCpuPreview.cpp
    ../plugin/src/ui/visualizer/PreviewSurface.cpp
    ../plugin/src/ui/visualizer/FxAnimationAtlas.cpp
    ../plugin/src/ui/visualizer/FxEmbeddedAtlases.cpp
    ../plugin/src/ui/components/FilterPanelScopeView.cpp
    ../plugin/src/ui/components/ArpStepStrip.cpp
    ../plugin/src/ui/components/ArpLauncherChip.cpp
    ../plugin/src/ui/components/ArpPanelOverlay.cpp
    ../plugin/src/ui/components/WavetableLabPanel.cpp
    ../plugin/src/ui/components/WavetableStackView.cpp
    ../plugin/src/ui/components/wireframe/WireframeCanvas.cpp
    ../plugin/src/ui/components/wireframe/OscPreviewSampler.cpp
    ../plugin/src/ui/components/wireframe/WavetableMeshPaint.cpp
    ../plugin/src/ui/components/wireframe/EngineWireframeViews.cpp
    ../plugin/src/ui/components/wireframe/OscWireframeHost.cpp
    ../plugin/src/ui/components/wireframe/EnvelopeCurveView.cpp
    ../plugin/src/ui/components/wireframe/FxWireframeView.cpp
    ../plugin/src/ui/components/wireframe/DesignFxHeroViz.cpp
    ../plugin/src/ui/components/wireframe/FxChainFlowView.cpp
    ../plugin/src/ui/components/wireframe/LfoWireframeView.cpp
    ../plugin/src/ui/components/wireframe/FilterWireframeView.cpp
    ../plugin/src/ui/components/wireframe/FilterRoutingWireframeView.cpp
    ../plugin/src/ui/components/wireframe/ModRoutingWireframeView.cpp
    ../plugin/src/standalone/StandaloneMcpBridge.cpp
)

target_include_directories(pw8_undertow_plugin PRIVATE ../plugin/src)

target_link_libraries(pw8_undertow_plugin
    PRIVATE
        pw8::core
        pw8_undertow_branding_data
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_compile_definitions(pw8_undertow_plugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    $<$<CONFIG:Debug>:PW8_DEV_CONTENT_ROOT="${CMAKE_SOURCE_DIR}">
)

pw8_set_warnings(pw8_undertow_plugin)
