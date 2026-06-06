file(GLOB SOURCE_FILES "*.h" "*.cpp")

create_dynamic_target(TGFXVulkan ${SOURCE_FILES} "TuranLibraries/tgfx")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVULKAN_DEBUGGING")

find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    target_include_directories(TGFXVulkan PUBLIC ${VULKAN_INCLUDE_DIR})
    message(${Vulkan_INCLUDE_DIR})
    target_link_libraries(TGFXVulkan PUBLIC Vulkan::Vulkan)
else()
    error ("Vulkan SDK isn't found, please be sure to include and link Vulkan SDK to the project!")
endif()

add_vcpkg_dependency(TGFXVulkan glfw3 glfw)
add_vcpkg_dependency(TGFXVulkan glm glm::glm)
add_vcpkg_dependency(TGFXVulkan imgui imgui::imgui)
add_vcpkg_dependency(TGFXVulkan glslang glslang::glslang)