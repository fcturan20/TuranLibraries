cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE dx12backendfiles "*.cpp" "*.h" "*.c")
tcmake_create_dynamic_target(TGfxDx12 "${dx12backendfiles}" "TGfx")
tcmake_add_vcpkg_dependency(TGfxDx12 directx-headers Microsoft::DirectX-Headers)
tcmake_add_vcpkg_dependency(TGfxDx12 directxmath Microsoft::DirectXMath)
target_link_libraries(TGfxDx12 PRIVATE TGfxCore TCoreECS TCoreString TCoreBitset TCoreFilesys TCoreLogger TCoreProfiler TCoreThreadedJobSys TCoreUnitTest)