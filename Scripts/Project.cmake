message(STATUS "Project.cmake included")

function(tcmake_add_subdirectory_if_exists dir)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        add_subdirectory(${dir})
    endif()
endfunction()

function (tcmake_create_dynamic_target target_name source_files target_folder)
    add_library(${target_name} SHARED ${source_files})

    # Derive include directories from header paths in the given source list.
    set(_target_include_dirs "")
    foreach(_src IN LISTS source_files) 
        if (_src MATCHES "\\.(h|hh|hpp|hxx)$")
            get_filename_component(_src_dir "${_src}" DIRECTORY)
            if (NOT IS_ABSOLUTE "${_src_dir}")
                set(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_src_dir}")
            endif()
            list(APPEND _target_include_dirs "${_src_dir}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _target_include_dirs)
    if (_target_include_dirs)
        target_include_directories(${target_name} PUBLIC ${_target_include_dirs})
    endif()
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
        PREFIX ""
    )
    if (target_folder)
        message(STATUS "Setting target folder for ${target_name} to ${target_folder}")
        set_target_properties(${target_name} PROPERTIES FOLDER ${target_folder})
    endif()
endfunction()

function (tcmake_create_executable_target target_name source_files target_folder)
    add_executable(${target_name} ${source_files})

    # Derive include directories from header paths in the given source list.
    set(_target_include_dirs "")
    foreach(_src IN LISTS source_files)
        if (_src MATCHES "\\.(h|hh|hpp|hxx)$")
            get_filename_component(_src_dir "${_src}" DIRECTORY)
            if (NOT IS_ABSOLUTE "${_src_dir}")
                set(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_src_dir}")
            endif()
            list(APPEND _target_include_dirs "${_src_dir}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _target_include_dirs)
    if (_target_include_dirs)
        target_include_directories(${target_name} PUBLIC ${_target_include_dirs})
    endif()
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
    )
    if (target_folder)
        set_target_properties(${target_name} PROPERTIES FOLDER ${target_folder})
    endif()
endfunction()

function (tcmake_create_interface_library target_name header_files target_folder)
    add_library(${target_name} INTERFACE ${header_files})
    if (target_folder)
        set_target_properties(${target_name} PROPERTIES FOLDER ${target_folder})
    endif()
endfunction()