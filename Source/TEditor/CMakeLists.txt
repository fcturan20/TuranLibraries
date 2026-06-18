cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE editorfiles "pecfManager/*.cpp" "pecfManager/*.h" "pecfManager/*.c" "main.cpp" "main.h")

tcmake_create_executable_target(TEditor "${editorfiles}" "TEditor")
target_include_directories(TEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../turanapi/turanapi ${CMAKE_CURRENT_SOURCE_DIR}/../tgfx/core ${CMAKE_CURRENT_SOURCE_DIR}/../editor)
target_compile_definitions(TEditor PUBLIC SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(TEditor PRIVATE TGfxCore TGfxVulkan TCoreECS TCoreString TCoreBitset TCoreFilesys TCoreLogger TCoreProfiler TCoreThreadedJobSys TCoreUnitTest)

tcmake_add_vcpkg_dependency(TEditor glm glm::glm)
tcmake_add_vcpkg_dependency(TEditor stb Stb_INCLUDE_DIR)
message(STATUS "Stb include dir: ${Stb_INCLUDE_DIR}")

add_subdirectory("scene")
add_subdirectory("UI")
add_subdirectory("gameLinker")