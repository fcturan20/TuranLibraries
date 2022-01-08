cmake_minimum_required (VERSION 3.8)
set(CMAKE_TOOLCHAIN_FILE "D:/dev/Extras/vcpkg/scripts/buildsystems/vcpkg.cmake")
include("D:/dev/Extras/vcpkg/scripts/buildsystems/vcpkg.cmake")

project ("TuranLibraries")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )


# Include sub-projects.
add_subdirectory ("turanapi")
#add_subdirectory ("TGFXCORE")
#add_subdirectory ("TGFXVulkan")
add_subdirectory ("Example")