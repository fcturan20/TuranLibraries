cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE editorfiles "pecfManager/*.cpp" "pecfManager/*.h" "pecfManager/*.c" "main.cpp" "main.h")

create_executable_target(editor "${editorfiles}" "TuranLibraries/editor")
target_include_directories(editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../turanapi/turanapi ${CMAKE_CURRENT_SOURCE_DIR}/../tgfx/core ${CMAKE_CURRENT_SOURCE_DIR}/../editor)
target_compile_definitions(editor PUBLIC SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(editor PRIVATE tgfx_core TGFXVulkan tapi_ecs tapi_string tapi_bitset tapi_filesys tapi_logger tapi_profiler tapi_threadedjobsys tapi_unittest)

add_vcpkg_dependency(editor glm glm::glm)
add_vcpkg_dependency(editor stb Stb_INCLUDE_DIR)
message(STATUS "Stb include dir: ${Stb_INCLUDE_DIR}")

add_subdirectory("scene")
add_subdirectory("UI")
add_subdirectory("gameLinker")