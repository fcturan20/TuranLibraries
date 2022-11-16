cmake_minimum_required (VERSION 3.8)


file(GLOB_RECURSE rendergraphfiles "main.cpp")
add_library (tgfx_rendergraph SHARED ${rendergraphfiles})
target_include_directories(tgfx_rendergraph PRIVATE ${CMAKE_SOURCE_DIR}/turanapi/turanapi ${CMAKE_SOURCE_DIR}/tgfx/core)
set_target_properties(tgfx_rendergraph PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tgfx_rendergraph PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tgfx_rendergraph RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)