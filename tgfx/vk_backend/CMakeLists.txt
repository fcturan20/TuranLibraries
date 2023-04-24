cmake_minimum_required (VERSION 3.8)

#file(GLOB_RECURSE VKbackendfiles "*.cpp" "*.h" "*.c")
#list(REMOVE_ITEM VKbackendfiles "${CMAKE_SOURCE_DIR}/tgfx/vk_backend/rendergraph_main.cpp" "${CMAKE_SOURCE_DIR}/tgfx/vk_backend/rendergraph_static.cpp" "${CMAKE_SOURCE_DIR}/tgfx/vk_backend/rendergraph_dynamic.cpp" "${CMAKE_SOURCE_DIR}/tgfx/vk_backend/rendergraph.h")
#add_library(TGFXVulkan SHARED ${VKbackendfiles})
add_library(TGFXVulkan SHARED "vk_core.h" "vk_core.cpp" "vk_predefinitions.h" "vk_predefinitions.cpp" "vk_extension.h" "vk_extension.cpp"
 "vkext_timelineSemaphore.h" "vkext_timelineSemaphore.cpp" "vk_queue.h" "vk_queue.cpp" "vk_helper.cpp" "vk_helper.h" "vkext_descIndexing.h" "vkext_descIndexing.cpp"
 "vkext_depthstencil.h" "vkext_depthstencil.cpp" "vk_includes.h" "vk_renderer.h" "vk_renderer.cpp" "vk_contentmanager.cpp" "vk_contentmanager.h" "vkext_dynamic_rendering.cpp" "vkext_dynamic_rendering.h"
 "vkext_dynamicStates.cpp" "vkext_dynamicStates.h" "vk_glslang.cpp")
target_include_directories(TGFXVulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../turanapi/turanapi/ ${CMAKE_CURRENT_SOURCE_DIR}/../core/)
message(${CMAKE_CURRENT_SOURCE_DIR}/../../turanapi/turanapi/)
set_target_properties(TGFXVulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(TGFXVulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(TGFXVulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(TGFXVulkan PROPERTIES FOLDER "TuranLibraries/tgfx")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVULKAN_DEBUGGING")

find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    target_include_directories(TGFXVulkan PUBLIC ${VULKAN_INCLUDE_DIR})
    message(${Vulkan_INCLUDE_DIR})
    target_link_libraries(TGFXVulkan PUBLIC Vulkan::Vulkan)
endif()

find_package(glfw3 CONFIG REQUIRED)
if(GLFW_FOUND OR glfw3_FOUND)
    target_include_directories(TGFXVulkan PUBLIC ${GLFW_INCLUDE_DIRS})
    target_link_libraries(TGFXVulkan PUBLIC glfw)
else()
    message ("GLFW isn't found, please be sure to include and link GLFW3 to the project!")
endif()


find_package(glm REQUIRED)
target_include_directories(TGFXVulkan PUBLIC ${glm_INCLUDE_DIRS})
target_link_libraries(TGFXVulkan PUBLIC glm::glm)

find_package(imgui CONFIG)
if(imgui_FOUND)
    target_link_libraries(TGFXVulkan PUBLIC imgui::imgui)
else()
    message("dear Imgui (by Ocornut) isn't installed on your system!")
endif()

# GLSLANG
find_package(glslang CONFIG REQUIRED)
target_link_libraries(TGFXVulkan PRIVATE glslang::OSDependent glslang::glslang glslang::MachineIndependent glslang::GenericCodeGen glslang::glslang-default-resource-limits glslang::OGLCompiler glslang::SPVRemapper glslang::SPIRV glslang::HLSL)