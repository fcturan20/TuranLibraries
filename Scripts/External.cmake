message(STATUS "External.cmake included")

if (DEFINED VCPKG_TARGET_TRIPLET)
    message(STATUS "VCPKG_TARGET_TRIPLET is set to ${VCPKG_TARGET_TRIPLET}")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(VCPKG_TARGET_TRIPLET x64-windows)
    message(WARNING "VCPKG_TARGET_TRIPLET is not defined. Defaulting to ${VCPKG_TARGET_TRIPLET}.")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(VCPKG_TARGET_TRIPLET x64-osx)
    message(WARNING "VCPKG_TARGET_TRIPLET is not defined. Defaulting to ${VCPKG_TARGET_TRIPLET}.")
endif()

# Vcpkg integration
# Resolve vcpkg toolchain in this order:
# 1) VCPKG_TOOLCHAIN_FILE (if explicitly provided)
# 2) VCPKG_ROOT environment variable
# 3) vcpkg executable path
if (DEFINED VCPKG_TOOLCHAIN_FILE AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_TOOLCHAIN_FILE}" CACHE STRING "Vcpkg toolchain file")
endif()

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(_vcpkg_root "")

    if (DEFINED ENV{VCPKG_ROOT})
        set(_vcpkg_root "$ENV{VCPKG_ROOT}")
    endif()

    if (NOT _vcpkg_root)
        find_program(VCPKG_EXECUTABLE NAMES vcpkg vcpkg.exe
            HINTS "${CMAKE_SOURCE_DIR}/vcpkg" "${CMAKE_SOURCE_DIR}/../vcpkg" "${CMAKE_SOURCE_DIR}/../../vcpkg")

        if (NOT VCPKG_EXECUTABLE)
            find_program(VCPKG_EXECUTABLE NAMES vcpkg vcpkg.exe)
        endif()

        if (VCPKG_EXECUTABLE)
            get_filename_component(_vcpkg_root "${VCPKG_EXECUTABLE}" DIRECTORY)
        endif()
    endif()

    if (_vcpkg_root)
        set(_vcpkg_toolchain "${_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")

        if (EXISTS "${_vcpkg_toolchain}")
            set(CMAKE_TOOLCHAIN_FILE "${_vcpkg_toolchain}" CACHE STRING "Vcpkg toolchain file")
            message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        else()
            message(FATAL_ERROR "vcpkg was found, but toolchain file was not found at: ${_vcpkg_toolchain}")
        endif()
    else()
        message(WARNING "vcpkg toolchain file not found. Please set VCPKG_TOOLCHAIN_FILE or VCPKG_ROOT environment variable.")
    endif()
endif()

function(run_regen_script)
    message(STATUS "Running regen.py with arguments: ${ARGN}")
    execute_process(COMMAND python3 ${CMAKE_SOURCE_DIR}/Scripts/regen.py ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error_output
        ECHO_OUTPUT_VARIABLE   # prints AND captures stdout
        ECHO_ERROR_VARIABLE    # prints AND captures stderr
    )
    if (result EQUAL 0)
        message(STATUS "Successfully ran regen.py with arguments: ${ARGN}")
    else()
        message(FATAL_ERROR "Failed to run regen.py. Output: ${output} Error: ${error_output}")
    endif()
endfunction()

function(add_vcpkg_dependency target_name dependency target_dep_name)
    find_package(${dependency} CONFIG QUIET)

    if (NOT TARGET ${target_dep_name})
        message(STATUS "Dependency ${dependency} not found. Attempting to install via vcpkg...")
        run_regen_script(--install ${dependency})
        find_package(${dependency} CONFIG QUIET)
    endif()
    
    if (TARGET ${target_dep_name} OR (DEFINED ${target_dep_name} AND ${target_dep_name}))
        message(STATUS "Dependency ${dependency} found with target ${target_dep_name}. Linking to ${target_name}.")
        target_link_libraries(${target_name} PRIVATE ${target_dep_name})
    else()
        message(FATAL_ERROR "Dependency ${dependency} could not be linked: target ${target_dep_name} was not found and variable ${target_dep_name} is empty or undefined.")
    endif()
endfunction()
