cmake_minimum_required (VERSION 3.8)


file(GLOB_RECURSE gameAttachingPluginFiles "*.cpp" "*.h" "*.c")
add_library(editor_gamelink SHARED ${gameAttachingPluginFiles})
target_include_directories(editor_gamelink PRIVATE ${CMAKE_SOURCE_DIR}/turanapi/turanapi ${CMAKE_SOURCE_DIR}/tgfx/core ${CMAKE_SOURCE_DIR}/editor)
set_target_properties(editor_gamelink PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(editor_gamelink PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")