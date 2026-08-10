# pw8_plugin -- JUCE VST3/AU/Standalone wrapper around pw8_core.
#
# STATUS: SCAFFOLD / PARTIAL. This target is off by default (PW8_BUILD_PLUGIN=OFF)
# and is NOT exercised by CI in this pass -- see docs/PLUGIN_ARCHITECTURE.md and
# docs/ROADMAP.md Phase 16. It is included now so the directory structure, source
# layout, and dependency wiring are real and ready rather than an empty folder, but
# the processor code has not been compiled against an actual JUCE checkout yet.
#
# To attempt a build:
#   cmake --preset dev -DPW8_BUILD_PLUGIN=ON
# This will fetch JUCE (GPLv3/commercial-dual-licensed -- see THIRD_PARTY_LICENSES.md;
# a commercial JUCE license is required before this plugin can ship closed-source).

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(pw8_plugin
    PRODUCT_NAME "Patchwork Eight"
    COMPANY_NAME "Patchwork"
    BUNDLE_ID "com.patchwork.patchworkeight"
    PLUGIN_MANUFACTURER_CODE Pwei
    PLUGIN_CODE Pwe8
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
)

target_include_directories(pw8_plugin PRIVATE src)

target_link_libraries(pw8_plugin
    PRIVATE
        pw8::core
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
