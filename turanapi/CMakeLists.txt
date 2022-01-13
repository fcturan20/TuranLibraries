cmake_minimum_required (VERSION 3.8)


#Registry System
add_library (tapi_registrysys SHARED
"${CMAKE_SOURCE_DIR}/turanapi/turanapi/registrysys_tapi.c" "${CMAKE_SOURCE_DIR}/turanapi/turanapi/virtualmemorysys_tapi.c"
"${CMAKE_SOURCE_DIR}/turanapi/turanapi/registrysys_tapi.h" "${CMAKE_SOURCE_DIR}/turanapi/turanapi/virtualmemorysys_tapi.h")
set_target_properties(tapi_registrysys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_registrysys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_registrysys RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Unit Test System
add_library (tapi_unittest SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/unittestsys_tapi.c""${CMAKE_SOURCE_DIR}/turanapi/turanapi/unittestsys_tapi.h")
set_target_properties(tapi_unittest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_unittest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_unittest RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Array of Strings System
add_library (tapi_array_of_strings_sys SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/array_of_strings_tapi.c""${CMAKE_SOURCE_DIR}/turanapi/turanapi/array_of_strings_tapi.h")
set_target_properties(tapi_array_of_strings_sys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_array_of_strings_sys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_array_of_strings_sys RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Threaded Job System
add_library (tapi_threadedjobsys SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/threadingsys_tapi.cpp""${CMAKE_SOURCE_DIR}/turanapi/turanapi/threadingsys_tapi.h")
set_target_properties(tapi_threadedjobsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_threadedjobsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_threadedjobsys RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#File System
add_library (tapi_filesys SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/filesys_tapi.cpp""${CMAKE_SOURCE_DIR}/turanapi/turanapi/filesys_tapi.h")
set_target_properties(tapi_filesys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_filesys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_filesys RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Logger System
add_library (tapi_logger SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/logger_tapi.cpp""${CMAKE_SOURCE_DIR}/turanapi/turanapi/logger_tapi.h")
set_target_properties(tapi_logger PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_logger PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_logger RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Bitset System
add_library (tapi_bitset SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/bitset_tapi.cpp""${CMAKE_SOURCE_DIR}/turanapi/turanapi/bitset_tapi.h")
set_target_properties(tapi_bitset PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_bitset PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_bitset RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Bitset System
add_library (tapi_profiler SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/profiler_tapi.cpp""${CMAKE_SOURCE_DIR}/turanapi/turanapi/profiler_tapi.h")
set_target_properties(tapi_profiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_profiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_profiler RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)

#Allocator System
add_library (tapi_allocator SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/allocator_tapi.cpp""${CMAKE_SOURCE_DIR}/turanapi/turanapi/allocator_tapi.h")
set_target_properties(tapi_allocator PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_allocator PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_allocator RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)