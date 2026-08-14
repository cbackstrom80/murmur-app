cmake_minimum_required(VERSION 3.24)

project(patchwork_eight
    VERSION 1.1.3
    DESCRIPTION "Patchwork Eight -- AI-native dual-layer 8-engine algorithmic software synthesizer"
    LANGUAGES CXX C)
# C is only needed for the optional JUCE plugin target (a handful of JUCE's
# helper/vendored bits are plain C); pw8_core and everything else here is pure C++.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --- Top-level options -------------------------------------------------------------
option(PW8_BUILD_TESTS "Build Catch2 unit/dsp/serialization tests" ON)
option(PW8_BUILD_TOOLS "Build developer CLI tools (render, info, graph inspector, ...)" ON)
option(PW8_BUILD_PYTHON_BINDINGS "Build pybind11 Python bindings (requires Python dev headers)" OFF)
option(PW8_BUILD_BENCHMARKS "Build Google Benchmark suite" OFF)
option(PW8_BUILD_PLUGIN "Build the JUCE VST3/AU/Standalone plugin (requires JUCE)" OFF)
option(PW8_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(PW8_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(PW8_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)

add_subdirectory(engine)

if(PW8_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(PW8_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PW8_BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings/python)
endif()

if(PW8_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(PW8_BUILD_PLUGIN)
    add_subdirectory(plugin)
endif()

message(STATUS "patchwork-eight configuration:")
message(STATUS "  PW8_BUILD_TESTS            = ${PW8_BUILD_TESTS}")
message(STATUS "  PW8_BUILD_TOOLS            = ${PW8_BUILD_TOOLS}")
message(STATUS "  PW8_BUILD_PYTHON_BINDINGS  = ${PW8_BUILD_PYTHON_BINDINGS}")
message(STATUS "  PW8_BUILD_BENCHMARKS       = ${PW8_BUILD_BENCHMARKS}")
message(STATUS "  PW8_BUILD_PLUGIN           = ${PW8_BUILD_PLUGIN}")
