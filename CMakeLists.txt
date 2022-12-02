cmake_minimum_required (VERSION 3.8)
set(VCPKG_TARGET_TRIPLET x64-windows)
set(CMAKE_TOOLCHAIN_FILE "D:/dev/Extras/vcpkg/scripts/buildsystems/vcpkg.cmake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include("D:/dev/Extras/vcpkg/scripts/buildsystems/vcpkg.cmake")

project ("TuranLibraries")
set(CMAKE_CXX_STANDARD 17)

# Include sub-projects.
add_subdirectory ("turanapi")
add_subdirectory ("tgfx")
add_subdirectory ("Example")
add_subdirectory ("editor")