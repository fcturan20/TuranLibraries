file(GLOB SOURCE_FILES "*.h" "*.cpp")

create_dynamic_target(tgfx_vulkan "${SOURCE_FILES}" "TuranLibraries/tgfx")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVULKAN_DEBUGGING")

find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    target_include_directories(tgfx_vulkan PUBLIC ${VULKAN_INCLUDE_DIR})
    message(STATUS "Vulkan include directory: ${Vulkan_INCLUDE_DIR}")
    target_link_libraries(tgfx_vulkan PUBLIC Vulkan::Vulkan)
else()
    error ("Vulkan SDK isn't found, please be sure to include and link Vulkan SDK to the project!")
endif()

add_vcpkg_dependency(tgfx_vulkan glfw3 glfw)
add_vcpkg_dependency(tgfx_vulkan glm glm::glm)
add_vcpkg_dependency(tgfx_vulkan imgui imgui::imgui)
add_vcpkg_dependency(tgfx_vulkan glslang glslang::glslang)