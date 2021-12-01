cmake_minimum_required (VERSION 3.8)

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#Add dependent .cpp files to project, because I don't want to copy .lib files of the dependent projects
file(GLOB_RECURSE DependentTuranLibs "${CMAKE_SOURCE_DIR}/TAPI/TAPI/*.cpp" "${CMAKE_SOURCE_DIR}/TGFXCORE/TGFX/*.cpp")
add_compile_definitions (TAPI_THREADING TAPI_BITSET TAPI_FILESYSTEM TAPI_LOGGER TAPI_PROFILER)
file(GLOB_RECURSE ExampleFiles "*.cpp" "*.h")
add_executable(Example ${ExampleFiles} ${DependentTuranLibs} "main.cpp")
include_directories(${CMAKE_SOURCE_DIR}/TAPI)
include_directories(${CMAKE_SOURCE_DIR}/TGFXCORE)
message("Binary Dir: ${CMAKE_BINARY_DIR}")
message("Current Binary Dir: ${CMAKE_CURRENT_BINARY_DIR}")
message("Source Dir: ${CMAKE_SOURCE_DIR}")
target_link_libraries(Example TGFXVulkan)

set_target_properties(Example PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set_target_properties(Example PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

#Use VCPKG and force x64
set(CMAKE_TOOLCHAIN_FILE "C:/dev/Extras/vcpkg-master/scripts/buildsystems/vcpkg.cmake")
set(-DUSE64bit=true)