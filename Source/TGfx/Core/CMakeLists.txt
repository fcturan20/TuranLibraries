file(GLOB_RECURSE tgfxcorefiles "Source/*.cpp" "Include/*.h" "Source/*.c")
tcmake_create_dynamic_target(TGfxCore "${tgfxcorefiles}" "TGfx")
target_link_libraries(TGfxCore PRIVATE TCore TCoreECS TCoreString TCoreBitset TCoreFilesys TCoreLogger TCoreProfiler TCoreThreadedJobSys TCoreUnitTest)
