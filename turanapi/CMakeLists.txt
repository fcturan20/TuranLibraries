cmake_minimum_required (VERSION 3.8)


#ECS System
add_library (tapi_ecs SHARED
"turanapi/ecs_tapi.cpp"
"turanapi/ecs_tapi.h"
"turanapi/allocator_tapi.cpp" "turanapi/allocator_tapi.h")
set_target_properties(tapi_ecs PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_ecs PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_ecs PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_ecs PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_ecs RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Unit Test System
add_library (tapi_unittest SHARED "turanapi/unittestsys_tapi.cpp" "turanapi/unittestsys_tapi.h")
set_target_properties(tapi_unittest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_unittest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_unittest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_unittest PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_unittest RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Array of Strings System
add_library (tapi_string SHARED "turanapi/string_tapi.cpp" "turanapi/string_tapi.h")
set_target_properties(tapi_string PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_string PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_string PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_string PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_string RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Threaded Job System
add_library (tapi_threadedjobsys SHARED "turanapi/threadingsys_tapi.cpp" "turanapi/threadingsys_tapi.h")
set_target_properties(tapi_threadedjobsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_threadedjobsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_threadedjobsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_threadedjobsys PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_threadedjobsys RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#File System
add_library (tapi_filesys SHARED "turanapi/filesys_tapi.cpp" "turanapi/filesys_tapi.h")
set_target_properties(tapi_filesys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_filesys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_filesys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_filesys PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_filesys RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Logger System
add_library (tapi_logger SHARED "turanapi/logger_tapi.cpp" "turanapi/logger_tapi.h")
set_target_properties(tapi_logger PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_logger PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_logger PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_logger PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_logger RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Bitset System
add_library (tapi_bitset SHARED "turanapi/bitset_tapi.cpp" "turanapi/bitset_tapi.h")
set_target_properties(tapi_bitset PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_bitset PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_bitset PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_bitset PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_bitset RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Bitset System
add_library (tapi_profiler SHARED "turanapi/profiler_tapi.cpp" "turanapi/profiler_tapi.h")
set_target_properties(tapi_profiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_profiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
set_target_properties(tapi_profiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/ReleaseDbg PREFIX "")
set_target_properties(tapi_profiler PROPERTIES FOLDER "TuranLibraries/tapi")
install(TARGETS tapi_profiler RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)
