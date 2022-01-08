cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE DependentTuranLibs "${CMAKE_SOURCE_DIR}/TAPI/TAPI/*.cpp" "${CMAKE_SOURCE_DIR}/TGFXCORE/TGFX/*.cpp")
add_compile_definitions (TAPI_THREADING TAPI_BITSET TAPI_FILESYSTEM TAPI_LOGGER TAPI_PROFILER)
file(GLOB_RECURSE VKbackendfiles "*.cpp" "*.h")
include_directories(${CMAKE_SOURCE_DIR}/TAPI ${CMAKE_SOURCE_DIR}/TGFXCORE)

add_library(TGFXVulkan SHARED ${VKbackendfiles} ${DependentTuranLibs})
set_target_properties(TGFXVulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set_target_properties(TGFXVulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

target_compile_definitions(TGFXVulkan PRIVATE
    $<$<CONFIG:Debug>:VULKAN_DEBUGGING>
)


find_package(glfw3 CONFIG REQUIRED)
if(GLFW_FOUND OR glfw3_FOUND)
    include_directories(${GLFW_INCLUDE_DIRS})
    target_link_libraries(TGFXVulkan PRIVATE glfw)
else()
    message ("GLFW isn't found, please be sure to include and link GLFW3 to the project!")
endif()

find_package(glm)

find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    include_directories(${Vulkan_INCLUDE_DIRS})
    message(${Vulkan_INCLUDE_DIRS})
    target_link_libraries(TGFXVulkan PRIVATE Vulkan)
else()
    message("Vulkan isn't found, please be sure to install Vulkan SDK then include and link Vulkan to the project!")
endif()

