cmake_minimum_required (VERSION 3.8)


#ECS System
set(TAPI_ECS_SOURCES "turanapi/ecs_tapi.cpp" "turanapi/ecs_tapi.h" "turanapi/allocator_tapi.cpp" "turanapi/allocator_tapi.h")
create_dynamic_target(tapi_ecs "${TAPI_ECS_SOURCES}" "TuranLibraries/tapi")

#Unit Test System
set(TAPI_UNITTEST_SOURCES "turanapi/unittestsys_tapi.cpp" "turanapi/unittestsys_tapi.h")
create_dynamic_target(tapi_unittest "${TAPI_UNITTEST_SOURCES}" "TuranLibraries/tapi")

#Array of Strings System
set(TAPI_STRING_SOURCES "turanapi/string_tapi.cpp" "turanapi/string_tapi.h")
create_dynamic_target(tapi_string "${TAPI_STRING_SOURCES}" "TuranLibraries/tapi")

#Threaded Job System
set(TAPI_THREADEDJOBSYS_SOURCES "turanapi/threadingsys_tapi.cpp" "turanapi/threadingsys_tapi.h")
create_dynamic_target(tapi_threadedjobsys "${TAPI_THREADEDJOBSYS_SOURCES}" "TuranLibraries/tapi")

#File System
set(TAPI_FILESYS_SOURCES "turanapi/filesys_tapi.cpp" "turanapi/filesys_tapi.h")
create_dynamic_target(tapi_filesys "${TAPI_FILESYS_SOURCES}" "TuranLibraries/tapi")

#Logger System
set(TAPI_LOGGER_SOURCES "turanapi/logger_tapi.cpp" "turanapi/logger_tapi.h")
create_dynamic_target(tapi_logger "${TAPI_LOGGER_SOURCES}" "TuranLibraries/tapi")

#Bitset System
set(TAPI_BITSET_SOURCES "turanapi/bitset_tapi.cpp" "turanapi/bitset_tapi.h")
create_dynamic_target(tapi_bitset "${TAPI_BITSET_SOURCES}" "TuranLibraries/tapi")

#Profiler System
set(TAPI_PROFILER_SOURCES "turanapi/profiler_tapi.cpp" "turanapi/profiler_tapi.h")
create_dynamic_target(tapi_profiler "${TAPI_PROFILER_SOURCES}" "TuranLibraries/tapi")

# C++ Wrapper for C API
