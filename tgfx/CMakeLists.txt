cmake_minimum_required (VERSION 3.8)

#Core system
file(GLOB_RECURSE tgfxcorefiles "core/*.cpp" "core/*.h" "core/*.c")
add_library (tgfx_core SHARED ${tgfxcorefiles})
include_directories(${CMAKE_SOURCE_DIR}/turanapi)
set_target_properties(tgfx_core PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tgfx_core PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tgfx_core RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)


#Vulkan
add_subdirectory("vulkan")