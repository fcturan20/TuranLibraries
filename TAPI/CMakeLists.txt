cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE TAPIFiles "TAPI/*.cpp" "TAPI/*.h")
# Add source to this project's executable.
add_library (TAPI STATIC ${TAPIFiles})
include_directories(${CMAKE_SOURCE_DIR}/TAPI)
add_compile_definitions (TAPI_THREADING TAPI_BITSET TAPI_FILESYSTEM TAPI_LOGGER TAPI_PROFILER)
set_target_properties(TAPI PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(TAPI PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)

# TODO: Add tests and install targets if needed.
install(TARGETS TAPI RUNTIME DESTINATION bin)
