cmake_minimum_required (VERSION 3.8)

# Core system
tcmake_add_subdirectory_if_exists("Core")
#RenderGraph
#tcmake_add_subdirectory_if_exists("RenderGraph")

#Vulkan
tcmake_add_subdirectory_if_exists("Vk")

#D3D12
if (WIN32)
    #tcmake_add_subdirectory_if_exists("Dx12")
endif()