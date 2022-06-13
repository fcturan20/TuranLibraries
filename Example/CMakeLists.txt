cmake_minimum_required (VERSION 3.8)

#Use VCPKG and force x64
set(CMAKE_TOOLCHAIN_FILE "C:/dev/Extras/vcpkg-master/scripts/buildsystems/vcpkg.cmake")
set(-DUSE64bit=true)

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


#Registry System Example
add_executable(cube_rendering_example "cube_rendering.cpp" "loading_libraries.h" "loading_shaders.h" "camera.h")
target_compile_definitions(cube_rendering_example PUBLIC CMAKE_SOURCE_FOLDER="${CMAKE_SOURCE_DIR}")
set_target_properties(cube_rendering_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug PREFIX "")
set_target_properties(cube_rendering_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
target_include_directories(cube_rendering_example PUBLIC ${CMAKE_SOURCE_DIR}/turanapi/turanapi ${CMAKE_SOURCE_DIR}/tgfx/core)
install(TARGETS cube_rendering_example RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/Example_runnables)


find_package(glm REQUIRED)
target_include_directories(cube_rendering_example PRIVATE ${glm_INCLUDE_DIRS})
target_link_libraries(cube_rendering_example PRIVATE glm::glm TGFXVulkan)



