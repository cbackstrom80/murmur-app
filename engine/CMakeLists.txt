# pw8_core -- the framework-independent DSP/engine library.
# MUST NOT depend on JUCE. See docs/ARCHITECTURE.md "Most Important Architectural Principle".

include(FetchContent)

# nlohmann/json -- MIT license, header-only. Used ONLY behind the
# pw8/patch/PatchSerializer.hpp and pw8/oscillator/WavetableTableLoader.hpp
# boundaries (patch/wavetable load+save), never in any header included by
# realtime/hot-loop code. See THIRD_PARTY_LICENSES.md.
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

add_library(pw8_core STATIC
    src/algorithm/AlgorithmGraphCompiler.cpp
    src/content/ContentPaths.cpp
    src/patch/PatchSerializer.cpp
    src/midi/StandardMidiFile.cpp
    src/oscillator/WavetableTableLoader.cpp
    src/render/Engine.cpp
    src/render/Renderer.cpp
    src/render/WavWriter.cpp
)
add_library(pw8::core ALIAS pw8_core)

target_include_directories(pw8_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pw8_core PRIVATE nlohmann_json::nlohmann_json)

target_compile_features(pw8_core PUBLIC cxx_std_20)

pw8_set_warnings(pw8_core)
pw8_apply_sanitizers(pw8_core)

set_target_properties(pw8_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
