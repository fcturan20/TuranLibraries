message(STATUS "Project.cmake included")

function (create_dynamic_target target_name source_files target_folder)
    add_library(${target_name} SHARED ${source_files})
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
        PREFIX ""
    )
    if (target_folder)
        set_target_properties(${target_name} PROPERTIES FOLDER ${target_folder})
    endif()
endfunction()

function (create_executable_target target_name source_files target_folder)
    add_executable(${target_name} ${source_files})
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Binaries"
    )
    if (target_folder)
        set_target_properties(${target_name} PROPERTIES FOLDER ${target_folder})
    endif()
endfunction()