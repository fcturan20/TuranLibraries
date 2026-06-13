cmake_minimum_required (VERSION 3.8)

# Core system
file(GLOB_RECURSE tgfxcorefiles "Core/Source/*.cpp" "Core/Include/*.h" "Core/Source/*.c")
tcmake_create_dynamic_target(TGfxCore "${tgfxcorefiles}" "TGfx")
target_link_libraries(TGfxCore PRIVATE TCoreECS TCoreString TCoreBitset TCoreFilesys TCoreLogger TCoreProfiler TCoreThreadedJobSys TCoreUnitTest)

#RenderGraph
tcmake_add_subdirectory_if_exists("RenderGraph")

#Vulkan
tcmake_add_subdirectory_if_exists("Vk")

#D3D12
if (WIN32)
    tcmake_add_subdirectory_if_exists("Dx12")
endif()