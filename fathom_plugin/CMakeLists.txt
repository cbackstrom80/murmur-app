# pw8_fathom_plugin -- Fathom, a real dedicated reverb effect plugin (JUCE
# VST3/AU/Standalone). Off by default (PW8_BUILD_FATHOM_PLUGIN=OFF).
#
# Real structural twin of quasar_plugin/CMakeLists.txt -- an IS_SYNTH FALSE
# effect plugin with its own small src/ tree (not a MURMUR-UI reuse like
# undertow_plugin; Fathom is an effect, not an instrument, so Quasar's
# shape is the right template here). Wraps the real, already-shipping
# pw8::effects::ReverbProcessor (engine/include/pw8/effects/Reverb.hpp,
# the real 8-line Householder FDN behind MURMUR's/Undertow's own master
# effect slot) directly -- no fork, no rewrite -- plus a real new
# convolution engine (juce::dsp::Convolution, already vendored/linked,
# previously unused anywhere in this codebase) loading real bundled
# impulse responses.
#
# To attempt a build:
#   cmake -B build -DPW8_BUILD_FATHOM_PLUGIN=ON && cmake --build build --target pw8_fathom_plugin_Standalone

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

juce_add_binary_data(pw8_fathom_branding_data
    SOURCES
        # Real placeholder icon -- Fathom doesn't have its own approved brand
        # asset yet (no Figma pass has happened for this product), so this
        # reuses the same generic mark Quasar's own real precedent used.
        # Replace once a real Fathom mark exists.
        ../plugin/resources/branding/murmur_mark_512.png
)

juce_add_plugin(pw8_fathom_plugin
    PRODUCT_NAME "Fathom"
    COMPANY_NAME "Murmur"
    BUNDLE_ID "com.patchwork.fathom"
    VERSION 1.0.0
    PLUGIN_MANUFACTURER_CODE Murr
    PLUGIN_CODE Fthm
    ICON_BIG ../plugin/resources/branding/murmur_mark_512.png
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Reverb"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

# Same real reason as MURMUR's/Undertow's own Standalone targets: a distinct
# bundle ID keeps macOS pkg install from relocating the AU into Fathom.app.
if(TARGET pw8_fathom_plugin_Standalone)
    set_target_properties(pw8_fathom_plugin_Standalone PROPERTIES
        JUCE_BUNDLE_ID "com.patchwork.fathom.standalone"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.patchwork.fathom.standalone")
endif()

target_sources(pw8_fathom_plugin PRIVATE
    src/FathomPluginMain.cpp
    src/FathomProcessor.cpp
    src/FathomParamLayout.cpp
    src/FathomPreset.cpp
    src/FathomIrLibrary.cpp
    src/FathomEditor.cpp
    ../plugin/src/ui/theme/ObsidianLookAndFeel.cpp
    ../plugin/src/ui/components/SectionPanel.cpp
)
# Deliberately NOT reusing plugin/src/ui/components/GlowKnob.cpp here --
# despite its generic-looking APVTS-bound constructor, its header
# unconditionally #includes processor/MurmurProcessor.h (a real instrument-
# only dependency Fathom has no reason to pull in). Fathom uses plain
# juce::Slider (rotary style) + ObsidianLookAndFeel's own real
# drawRotarySlider() instead -- same visual language, no coupling.

target_include_directories(pw8_fathom_plugin PRIVATE src ../plugin/src)

target_link_libraries(pw8_fathom_plugin
    PRIVATE
        pw8::core
        pw8_fathom_branding_data
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

target_compile_definitions(pw8_fathom_plugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

pw8_set_warnings(pw8_fathom_plugin)

# Real, throwaway smoke test for the new convolution engine -- see
# tools/convolution_smoke_test.cpp's own header comment. Not part of the
# real product; a direct-verification probe only.
add_executable(pw8_fathom_convolution_smoke_test tools/convolution_smoke_test.cpp)
target_link_libraries(pw8_fathom_convolution_smoke_test PRIVATE
    juce::juce_dsp
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
)
target_compile_definitions(pw8_fathom_convolution_smoke_test PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

# Real bundled impulse responses (38 real Voxengo Impulse Modeler IRs,
# royalty-free incl. commercial use -- VOXENGO_IR_LICENSE.txt travels with
# them per the real license's redistribution terms) -- loose files in the
# bundle's Resources/, not compiled-in binary data (14MB of WAV would bloat
# compile time/binary size for no benefit; loaded at runtime via
# juce::File, same real "copy factory content into the bundle" pattern
# quasar_plugin's own POST_BUILD step already uses for its presets).
add_custom_command(TARGET pw8_fathom_plugin_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:pw8_fathom_plugin_Standalone>/../Resources/impulse-responses"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/impulse-responses"
            "$<TARGET_FILE_DIR:pw8_fathom_plugin_Standalone>/../Resources/impulse-responses"
    COMMENT "Copy Fathom's real bundled impulse responses into the Standalone bundle"
)
if(TARGET pw8_fathom_plugin_AU)
    add_custom_command(TARGET pw8_fathom_plugin_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:pw8_fathom_plugin_AU>/../Resources/impulse-responses"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/impulse-responses"
                "$<TARGET_FILE_DIR:pw8_fathom_plugin_AU>/../Resources/impulse-responses"
        COMMENT "Copy Fathom's real bundled impulse responses into the AU bundle"
    )
endif()
if(TARGET pw8_fathom_plugin_VST3)
    add_custom_command(TARGET pw8_fathom_plugin_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:pw8_fathom_plugin_VST3>/../Resources/impulse-responses"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/impulse-responses"
                "$<TARGET_FILE_DIR:pw8_fathom_plugin_VST3>/../Resources/impulse-responses"
        COMMENT "Copy Fathom's real bundled impulse responses into the VST3 bundle"
    )
endif()
