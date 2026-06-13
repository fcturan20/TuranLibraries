cmake_minimum_required (VERSION 3.8)

# Core system headers
file(GLOB_RECURSE TCoreHeaders "Include/*.h")
tcmake_create_interface_library(TCore "${TCoreHeaders}" "TCore")

#ECS System
set(TCoreECS_SOURCES "Source/ECS.cpp" "Source/Allocator.cpp")
tcmake_create_dynamic_target(TCoreECS "${TCoreECS_SOURCES}" "TCore")
target_link_libraries(TCoreECS PRIVATE dynalo TCore)

#Unit Test System
set(TCoreUNITTEST_SOURCES "Source/UnitTestSystem.cpp")
tcmake_create_dynamic_target(TCoreUnitTest "${TCoreUNITTEST_SOURCES}" "TCore")
target_link_libraries(TCoreUnitTest PRIVATE dynalo TCore)

#Array of Strings System
set(TCoreSTRING_SOURCES "Source/String.cpp")
tcmake_create_dynamic_target(TCoreString "${TCoreSTRING_SOURCES}" "TCore")
target_link_libraries(TCoreString PRIVATE dynalo TCore)

#Threaded Job System
set(TCoreTHREADEDJOBSYS_SOURCES "Source/Threading.cpp")
tcmake_create_dynamic_target(TCoreThreadedJobSys "${TCoreTHREADEDJOBSYS_SOURCES}" "TCore")
target_link_libraries(TCoreThreadedJobSys PRIVATE dynalo TCore)

#File System
set(TCoreFILESYS_SOURCES "Source/FileSystem.cpp")
tcmake_create_dynamic_target(TCoreFileSystem "${TCoreFILESYS_SOURCES}" "TCore")
target_link_libraries(TCoreFileSystem PRIVATE dynalo TCore)

#Logger System
set(TCoreLOGGER_SOURCES "Source/Logger.cpp")
tcmake_create_dynamic_target(TCoreLogger "${TCoreLOGGER_SOURCES}" "TCore")
target_link_libraries(TCoreLogger PRIVATE dynalo TCore)

#Bitset System
set(TCoreBITSET_SOURCES "Source/Bitset.cpp" "Include/Bitset.h")
tcmake_create_dynamic_target(TCoreBitset "${TCoreBITSET_SOURCES}" "TCore")
target_link_libraries(TCoreBitset PRIVATE dynalo TCore)

#Profiler System
set(TCorePROFILER_SOURCES "Source/Profiler.cpp")
tcmake_create_dynamic_target(TCoreProfiler "${TCorePROFILER_SOURCES}" "TCore")
target_link_libraries(TCoreProfiler PRIVATE TCore)

# C++ Wrapper for C API