cmake_minimum_required (VERSION 3.8)


find_package(directx-headers CONFIG REQUIRED)


file(GLOB_RECURSE d3d12backendfiles "*.cpp" "*.h" "*.c")
add_library(TGFXD3D12 SHARED ${d3d12backendfiles})
target_include_directories(TGFXD3D12 PRIVATE ${CMAKE_SOURCE_DIR}/turanapi/turanapi ${CMAKE_SOURCE_DIR}/tgfx/core)
set_target_properties(TGFXD3D12 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(TGFXD3D12 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTD3D12_DEBUGGING")

target_link_libraries(TGFXD3D12 PRIVATE Microsoft::DirectX-Guids Microsoft::DirectX-Headers)
target_link_libraries(TGFXD3D12 PRIVATE "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")

find_path(D3DX12_INCLUDE_DIRS "d3dx12.h")
target_include_directories(TGFXD3D12 PRIVATE ${D3DX12_INCLUDE_DIRS})