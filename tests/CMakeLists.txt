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
    dsp/CharacterFilterTests.cpp
    dsp/LfoTests.cpp
    dsp/NoiseChaosOperatorTests.cpp
    dsp/PhaseShapeOscillatorTests.cpp
    dsp/WavetableWarpTests.cpp
    dsp/AdditiveOscillatorTests.cpp
    dsp/BiquadBandpassTests.cpp
    dsp/ResonatorOscillatorTests.cpp
    dsp/GranularOscillatorTests.cpp
    dsp/SyncEdgeTests.cpp
    unit/AlgorithmGraphCompilerTests.cpp
    unit/AlgorithmGraphCommitTests.cpp
    unit/ContentPathsTests.cpp
    unit/DeterministicRngTests.cpp
    unit/ModMatrixTests.cpp
    unit/FftTests.cpp
    unit/WavetableTableTests.cpp
    unit/ArpeggiatorTests.cpp
    unit/EffectsTests.cpp
    unit/EngineMacroLiveUpdateTests.cpp
    unit/EngineLiveParamsTests.cpp
    serialization/PatchSerializerTests.cpp
    serialization/FactoryPresetLoadTests.cpp
    serialization/StandardMidiFileTests.cpp
    serialization/WavetableTableLoaderTests.cpp
    regression/RenderSanityTests.cpp
    regression/EngineLayerUnisonTests.cpp
    regression/BlockMidiTests.cpp
    regression/GoldenPresetTests.cpp
    dsp/DahdsrEnvelopeLiveTests.cpp
    realtime/EngineProcessRealtimeTests.cpp
)

target_link_libraries(pw8_tests PRIVATE pw8::core nlohmann_json::nlohmann_json Catch2::Catch2WithMain)
target_compile_definitions(pw8_tests PRIVATE PW8_REPO_ROOT="${CMAKE_SOURCE_DIR}")
target_include_directories(pw8_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(pw8_tests PRIVATE cxx_std_20)
pw8_set_warnings(pw8_tests)
pw8_apply_sanitizers(pw8_tests)

include(CTest)
include(Catch)
catch_discover_tests(pw8_tests)
