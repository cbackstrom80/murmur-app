# pw8_quasar_plugin -- standalone QUASAR binaural spatial effect (JUCE VST3/AU/Standalone).
#
# Off by default (PW8_BUILD_QUASAR_PLUGIN=OFF). Build with:
#   cmake --preset quasar-release && cmake --build --preset quasar-release

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

juce_add_binary_data(pw8_quasar_branding_data
    SOURCES
        ../plugin/resources/branding/murmur_mark_512.png
)

juce_add_plugin(pw8_quasar_plugin
    PRODUCT_NAME "QUASAR"
    COMPANY_NAME "Murmur"
    BUNDLE_ID "com.patchwork.quasar"
    VERSION 1.0.0
    PLUGIN_MANUFACTURER_CODE Murr
    PLUGIN_CODE Qsar
    ICON_BIG ../plugin/resources/branding/murmur_mark_512.png
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Spatial"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

target_sources(pw8_quasar_plugin PRIVATE
    src/QuasarProcessor.cpp
    src/QuasarParamLayout.cpp
    src/QuasarEditor.cpp
    src/QuasarPreset.cpp
    src/ui/theme/ObsidianLookAndFeel.cpp
    src/ui/components/GlowKnob.cpp
    src/ui/components/MetadataFacetRow.cpp
)

target_include_directories(pw8_quasar_plugin PRIVATE src)

target_link_libraries(pw8_quasar_plugin
    PRIVATE
        pw8::core
        pw8_quasar_branding_data
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_compile_definitions(pw8_quasar_plugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

pw8_set_warnings(pw8_quasar_plugin)

# Ship factory presets beside the plugin bundle (dev + install scripts).
add_custom_command(TARGET pw8_quasar_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:pw8_quasar_plugin>/../Resources/presets/quasar"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/content/presets/quasar"
            "$<TARGET_FILE_DIR:pw8_quasar_plugin>/../Resources/presets/quasar"
    COMMENT "Copy QUASAR factory presets into plugin bundle Resources"
)
