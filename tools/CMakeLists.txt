# Developer CLI tools. Each links only pw8::core (and, where noted, nlohmann_json
# directly for tool-local JSON needs) -- none of these depend on JUCE.

add_executable(murmur-render render/main.cpp)
target_link_libraries(murmur-render PRIVATE pw8::core)
pw8_set_warnings(murmur-render)

add_executable(murmur-info info/main.cpp)
target_link_libraries(murmur-info PRIVATE pw8::core)
pw8_set_warnings(murmur-info)

add_executable(murmur-graph graph_inspector/main.cpp)
target_link_libraries(murmur-graph PRIVATE pw8::core)
pw8_set_warnings(murmur-graph)

add_executable(murmur-wavetable-builder wavetable_builder/main.cpp)
target_link_libraries(murmur-wavetable-builder PRIVATE pw8::core nlohmann_json::nlohmann_json)
pw8_set_warnings(murmur-wavetable-builder)

add_executable(murmur-fuzz-render fuzz_render/main.cpp)
target_link_libraries(murmur-fuzz-render PRIVATE pw8::core)
pw8_set_warnings(murmur-fuzz-render)

# tools/algorithm_inspector and tools/preset_inspector remain PLANNED -- their scope
# is already covered by murmur-graph (see their README.md files for the rationale).
