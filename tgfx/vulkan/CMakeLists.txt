cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE VKbackendfiles  "*.cpp" "*.h" "*.c")
add_library(TGFXVulkan SHARED ${VKbackendfiles})
target_include_directories(TGFXVulkan PRIVATE ${CMAKE_SOURCE_DIR}/turanapi ${CMAKE_SOURCE_DIR}/TGFXCORE)
set_target_properties(TGFXVulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(TGFXVulkan PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")

find_package(glfw3 CONFIG REQUIRED)
if(GLFW_FOUND OR glfw3_FOUND)
    include_directories(${GLFW_INCLUDE_DIRS})
    target_link_libraries(TGFXVulkan PRIVATE glfw)
else()
    message ("GLFW isn't found, please be sure to include and link GLFW3 to the project!")
endif()


find_package(glm REQUIRED)
target_link_libraries(TGFXVulkan PRIVATE glm::glm)

find_package(Vulkan REQUIRED)
if(VULKAN_FOUND)
    include_directories(${Vulkan_INCLUDE_DIRS})
    message(${Vulkan_INCLUDE_DIRS})
    target_link_libraries(TGFXVulkan PRIVATE Vulkan)
else()
    message("Vulkan isn't found, please be sure to install Vulkan SDK then include and link Vulkan to the project!")
endif()

