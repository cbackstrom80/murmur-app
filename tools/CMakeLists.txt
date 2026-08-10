# Developer CLI tools. Each links only pw8::core (and, where noted, nlohmann_json
# directly for tool-local JSON needs) -- none of these depend on JUCE.

add_executable(pw8-render render/main.cpp)
target_link_libraries(pw8-render PRIVATE pw8::core)
pw8_set_warnings(pw8-render)

add_executable(pw8-info info/main.cpp)
target_link_libraries(pw8-info PRIVATE pw8::core)
pw8_set_warnings(pw8-info)

add_executable(pw8-graph graph_inspector/main.cpp)
target_link_libraries(pw8-graph PRIVATE pw8::core)
pw8_set_warnings(pw8-graph)

add_executable(pw8-wavetable-builder wavetable_builder/main.cpp)
target_link_libraries(pw8-wavetable-builder PRIVATE pw8::core nlohmann_json::nlohmann_json)
pw8_set_warnings(pw8-wavetable-builder)

# tools/algorithm_inspector, tools/preset_inspector, tools/fuzz_render, and
# tools/benchmark are architected (see docs/ROADMAP.md and docs/TESTING.md
# "Property / Fuzz Testing") but not yet implemented in this pass.
