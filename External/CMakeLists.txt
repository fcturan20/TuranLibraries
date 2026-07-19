cmake_minimum_required (VERSION 3.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# TODO: Find all subdirectories and add them automatically

tcmake_add_subdirectory_if_exists(dynalo)

if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(LINUX)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
elseif(MACOSX)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
endif()
tcmake_add_subdirectory_if_exists(volk)