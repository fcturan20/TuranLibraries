file(GLOB SOURCE_FILES "*.h" "*.cpp")

tcmake_create_dynamic_target(TGfxVulkan "${SOURCE_FILES}" "TGfx")
target_link_libraries(TGfxVulkan PRIVATE TGfxCore TCore TCoreString TCoreBitset TCoreFilesys TCoreLogger TCoreProfiler TCoreThreadedJobSys TCoreUnitTest)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVULKAN_DEBUGGING")

find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    target_include_directories(TGfxVulkan PUBLIC ${VULKAN_INCLUDE_DIR})
    message(STATUS "Vulkan include directory: ${Vulkan_INCLUDE_DIR}")
    target_link_libraries(TGfxVulkan PUBLIC Vulkan::Vulkan)
else()
    error ("Vulkan SDK isn't found, please be sure to include and link Vulkan SDK to the project!")
endif()

target_link_libraries(TGfxVulkan PRIVATE volk)

tcmake_add_vcpkg_dependency(TGfxVulkan glfw3 glfw)
tcmake_add_vcpkg_dependency(TGfxVulkan glm glm::glm)
tcmake_add_vcpkg_dependency(TGfxVulkan imgui imgui::imgui)
tcmake_add_vcpkg_dependency(TGfxVulkan glslang glslang::glslang)