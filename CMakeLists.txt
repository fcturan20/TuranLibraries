cmake_minimum_required (VERSION 3.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Check if executed by the regen.py script
if (NOT DEFINED TCMAKE_INVOKER)
    message(WARNING "CMakeLists.txt is being executed directly. It is recommended to use a supported CMake invoker.")
endif()

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(WARNING "CMAKE_TOOLCHAIN_FILE is not defined. Please run the regen.py script to set up the vcpkg toolchain file.")
endif()

# Support for CMake versions older than 3.5
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

# Include CMake helper functions
file(GLOB CMK_FILES "${CMAKE_CURRENT_LIST_DIR}/Scripts/*.cmake")
list(SORT CMK_FILES)
foreach(f IN LISTS CMK_FILES)
  include("${f}")
endforeach()

project ("TuranLibraries")
set(CMAKE_CXX_STANDARD 17)

# Include sub-projects.
add_subdirectory ("turanapi")
add_subdirectory ("tgfx")
add_subdirectory ("Example")
add_subdirectory ("editor")
add_subdirectory ("External")