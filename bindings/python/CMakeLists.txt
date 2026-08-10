# Python bindings for pw8_core, via pybind11 (BSD-3-Clause, header-only C++ side).
# Off by default (PW8_BUILD_PYTHON_BINDINGS) since it requires Python development
# headers; enable with -DPW8_BUILD_PYTHON_BINDINGS=ON.

include(FetchContent)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(patchwork_eight MODULE PyBindings.cpp)
target_link_libraries(patchwork_eight PRIVATE pw8::core)
target_compile_features(patchwork_eight PRIVATE cxx_std_20)
pw8_set_warnings(patchwork_eight)

# The .so is placed directly under build/python/ (not nested in a same-named
# subdirectory) so `import patchwork_eight` resolves straight to the extension module
# rather than to an accidental namespace package. A proper pip-installable package
# layout (setup.py / pyproject.toml, __init__.py re-export, wheel building) is
# PLANNED -- see docs/PYTHON_API.md.
set_target_properties(patchwork_eight PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
)
