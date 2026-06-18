cmake_minimum_required (VERSION 3.8)

# Core system headers
set(TCoreFiles "Source/TCore.cpp" "Include/TCore.h" "Include/TCoreMacros.h")
tcmake_create_dynamic_target(TCore "${TCoreFiles}" "TCore")
set_target_properties(TCore PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/Include")
target_link_libraries(TCore PRIVATE dynalo)

# Virtual Memory System
set(TCoreVIRTUALMEM_SOURCES "Source/VirtualMemory.cpp" "Include/VirtualMemory.h")
tcmake_create_dynamic_target(TCoreVirtualMemory "${TCoreVIRTUALMEM_SOURCES}" "TCore")
target_link_libraries(TCoreVirtualMemory PRIVATE TCore)

# Allocator System
set(TCoreALLOCATOR_SOURCES "Source/Allocator.cpp" "Include/Allocator.h")
tcmake_create_dynamic_target(TCoreAllocator "${TCoreALLOCATOR_SOURCES}" "TCore")
target_link_libraries(TCoreAllocator PRIVATE TCore)

#ECS System
set(TCoreECS_SOURCES "Source/ECS.cpp" "Include/ECS.h")
tcmake_create_dynamic_target(TCoreECS "${TCoreECS_SOURCES}" "TCore")
target_link_libraries(TCoreECS PRIVATE TCore)

#Unit Test System
set(TCoreUNITTEST_SOURCES "Source/UnitTestSystem.cpp" "Include/UnitTestSystem.h")
tcmake_create_dynamic_target(TCoreUnitTest "${TCoreUNITTEST_SOURCES}" "TCore")
target_link_libraries(TCoreUnitTest PRIVATE TCore)

#String System
set(TCoreSTRING_SOURCES "Source/TString.cpp" "Include/TString.h")
tcmake_create_dynamic_target(TCoreString "${TCoreSTRING_SOURCES}" "TCore")
target_link_libraries(TCoreString PRIVATE TCore)

#Threaded Job System
set(TCoreTHREADEDJOBSYS_SOURCES "Source/Threading.cpp" "Include/Threading.h")
tcmake_create_dynamic_target(TCoreThreadedJobSys "${TCoreTHREADEDJOBSYS_SOURCES}" "TCore")
target_link_libraries(TCoreThreadedJobSys PRIVATE TCore)

#File System
set(TCoreFILESYS_SOURCES "Source/FileSystem.cpp" "Include/FileSystem.h")
tcmake_create_dynamic_target(TCoreFileSystem "${TCoreFILESYS_SOURCES}" "TCore")
target_link_libraries(TCoreFileSystem PRIVATE TCore)

#Logger System
set(TCoreLOGGER_SOURCES "Source/Logger.cpp" "Include/Logger.h")
tcmake_create_dynamic_target(TCoreLogger "${TCoreLOGGER_SOURCES}" "TCore")
target_link_libraries(TCoreLogger PRIVATE TCore)
 
#Bitset System
set(TCoreBITSET_SOURCES "Source/Bitset.cpp" "Include/Bitset.h")
tcmake_create_dynamic_target(TCoreBitset "${TCoreBITSET_SOURCES}" "TCore")
target_link_libraries(TCoreBitset PRIVATE TCore)

#Profiler System
set(TCorePROFILER_SOURCES "Source/Profiler.cpp" "Include/Profiler.h")
tcmake_create_dynamic_target(TCoreProfiler "${TCorePROFILER_SOURCES}" "TCore")
target_link_libraries(TCoreProfiler PRIVATE TCore)

# C++ Wrapper for C API