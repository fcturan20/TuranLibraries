cmake_minimum_required (VERSION 3.8)


file(GLOB_RECURSE editorfiles "pecfManager/*.cpp" "pecfManager/*.h" "pecfManager/*.c" "main.cpp" "main.h")
add_executable(editor ${editorfiles})
target_include_directories(editor PRIVATE ${CMAKE_SOURCE_DIR}/turanapi/turanapi ${CMAKE_SOURCE_DIR}/tgfx/core ${CMAKE_SOURCE_DIR}/editor)
set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS editor RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

add_subdirectory("scene")