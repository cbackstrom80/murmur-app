include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

add_executable(pw8_tests
    dsp/ClassicOscillatorTests.cpp
    dsp/FmPmOperatorTests.cpp
    dsp/DahdsrEnvelopeTests.cpp
    dsp/VoiceAllocatorTests.cpp
    dsp/StateVariableFilterTests.cpp
    dsp/LfoTests.cpp
    dsp/NoiseChaosOperatorTests.cpp
    unit/AlgorithmGraphCompilerTests.cpp
    unit/DeterministicRngTests.cpp
    unit/ModMatrixTests.cpp
    unit/FftTests.cpp
    unit/WavetableTableTests.cpp
    unit/ArpeggiatorTests.cpp
    unit/EffectsTests.cpp
    unit/EngineMacroLiveUpdateTests.cpp
    unit/EngineLiveParamsTests.cpp
    serialization/PatchSerializerTests.cpp
    serialization/StandardMidiFileTests.cpp
    serialization/WavetableTableLoaderTests.cpp
    regression/RenderSanityTests.cpp
)

target_link_libraries(pw8_tests PRIVATE pw8::core Catch2::Catch2WithMain)
target_compile_features(pw8_tests PRIVATE cxx_std_20)
pw8_set_warnings(pw8_tests)
pw8_apply_sanitizers(pw8_tests)

include(CTest)
include(Catch)
catch_discover_tests(pw8_tests)
