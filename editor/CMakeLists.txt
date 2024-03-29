cmake_minimum_required (VERSION 3.8)


file(GLOB_RECURSE editorfiles "pecfManager/*.cpp" "pecfManager/*.h" "pecfManager/*.c" "main.cpp" "main.h")
add_executable(editor ${editorfiles})
target_include_directories(editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../turanapi/turanapi ${CMAKE_CURRENT_SOURCE_DIR}/../tgfx/core ${CMAKE_CURRENT_SOURCE_DIR}/../editor)
set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(editor PROPERTIES FOLDER "TuranLibraries/editor")
install(TARGETS editor RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)
target_compile_definitions(editor PUBLIC SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(editor PRIVATE tgfx_core TGFXVulkan tapi_ecs tapi_string tapi_bitset tapi_filesys tapi_logger tapi_profiler tapi_threadedjobsys tapi_unittest)

find_package(glm REQUIRED)
target_include_directories(editor PUBLIC ${glm_INCLUDE_DIRS})
target_link_libraries(editor PUBLIC glm::glm)

find_package(Stb REQUIRED)
target_include_directories(editor PRIVATE ${Stb_INCLUDE_DIR})

add_subdirectory("scene")
add_subdirectory("UI")
add_subdirectory("gameLinker")