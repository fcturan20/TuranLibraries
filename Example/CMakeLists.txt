cmake_minimum_required (VERSION 3.8)

#Use VCPKG and force x64
set(CMAKE_TOOLCHAIN_FILE "C:/dev/Extras/vcpkg-master/scripts/buildsystems/vcpkg.cmake")
set(-DUSE64bit=true)

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

#Registry System Example
add_executable(registry_sys_example "registry_sys_example.cpp")
set_target_properties(registry_sys_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug PREFIX "")
set_target_properties(registry_sys_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release PREFIX "")
target_include_directories(registry_sys_example PUBLIC ${CMAKE_SOURCE_DIR}/turanapi)
target_include_directories(registry_sys_example PUBLIC ${CMAKE_SOURCE_DIR}/TGFXCORE)
install(TARGETS registry_sys_example RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/Example_runnables)




