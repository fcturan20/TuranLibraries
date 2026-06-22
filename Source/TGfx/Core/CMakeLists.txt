file(GLOB_RECURSE tgfxcorefiles "Core/Source/*.cpp" "Core/Include/*.h" "Core/Source/*.c")
tcmake_create_dynamic_target(TGfxCore "${tgfxcorefiles}" "TGfx")
target_link_libraries(TGfxCore PRIVATE TCore TCoreECS TCoreString TCoreBitset TCoreFilesys TCoreLogger TCoreProfiler TCoreThreadedJobSys TCoreUnitTest)
