cmake_minimum_required (VERSION 3.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dynalo")
    add_subdirectory(dynalo)
endif()
