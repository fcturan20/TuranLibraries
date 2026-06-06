cmake_minimum_required (VERSION 3.8)

# Core system
file(GLOB_RECURSE tgfxcorefiles "core/*.cpp" "core/*.h" "core/*.c")
create_dynamic_target(tgfx_core
"${tgfxcorefiles}"
    "TuranLibraries/tgfx")
file(GLOB_RECURSE tgfxcorefiles "core/*.cpp" "core/*.h" "core/*.c")
target_link_libraries(tgfx_core PRIVATE tapi_ecs tapi_string tapi_bitset tapi_filesys tapi_logger tapi_profiler tapi_threadedjobsys tapi_unittest)

#RenderGraph
add_subdirectory("rendergraph")

#Vulkan
add_subdirectory("vk_backend")

#D3D12
if (WIN32)
    add_subdirectory("dx12")
endif()