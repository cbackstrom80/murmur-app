# Google Benchmark suite. Off by default (PW8_BUILD_BENCHMARKS); see docs/BUILD.md
# "benchmarks" preset. Not run in CI (perf numbers are host-dependent and not a
# pass/fail signal) -- run locally with `cmake --preset benchmarks && cmake --build
# --preset benchmarks && ./build/benchmarks/benchmarks/pw8_benchmarks`.

include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(pw8_benchmarks
    OscillatorBenchmarks.cpp
    AlgorithmGraphBenchmarks.cpp
    VoiceBenchmarks.cpp
    RenderBenchmarks.cpp
)

target_link_libraries(pw8_benchmarks PRIVATE pw8::core benchmark::benchmark)
target_compile_features(pw8_benchmarks PRIVATE cxx_std_20)
pw8_set_warnings(pw8_benchmarks)

# Release-like codegen even under a Debug top-level build type, since benchmark
# numbers from an unoptimized build are meaningless.
if(NOT CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
    message(WARNING "pw8_benchmarks: CMAKE_BUILD_TYPE is '${CMAKE_BUILD_TYPE}', not Release/RelWithDebInfo -- benchmark numbers will not be meaningful.")
endif()
