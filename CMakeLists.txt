cmake_minimum_required (VERSION 3.8)
set(VCPKG_TARGET_TRIPLET x64-windows)
set(CMAKE_TOOLCHAIN_FILE "D:/dev/Extras/vcpkg/scripts/buildsystems/vcpkg.cmake")
include("D:/dev/Extras/vcpkg/scripts/buildsystems/vcpkg.cmake")

project ("TuranLibraries")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )


# Include sub-projects.
add_subdirectory ("turanapi")
add_subdirectory ("tgfx")
add_subdirectory ("Example")