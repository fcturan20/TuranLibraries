cmake_minimum_required (VERSION 3.8)


#Registry System
add_library (tapi_registrysys SHARED
"${CMAKE_SOURCE_DIR}/turanapi/turanapi/registrysys_tapi.c" "${CMAKE_SOURCE_DIR}/turanapi/turanapi/virtualmemorysys_tapi.c")
set_target_properties(tapi_registrysys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_registrysys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_registrysys RUNTIME DESTINATION bin)

#Unit Test System
add_library (tapi_unittest SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/unittestsys_tapi.c")
set_target_properties(tapi_unittest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_unittest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_unittest RUNTIME DESTINATION bin)

#Array of Strings System
add_library (tapi_array_of_strings_sys SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/array_of_strings_tapi.c")
set_target_properties(tapi_array_of_strings_sys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_array_of_strings_sys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_array_of_strings_sys RUNTIME DESTINATION bin)

#Threaded Job System
add_library (tapi_threadedjobsys SHARED "${CMAKE_SOURCE_DIR}/turanapi/turanapi/threadingsys_tapi.cpp")
set_target_properties(tapi_threadedjobsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set_target_properties(tapi_threadedjobsys PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
install(TARGETS tapi_threadedjobsys RUNTIME DESTINATION bin)