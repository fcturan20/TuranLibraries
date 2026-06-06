message(STATUS "Project.cmake included")

function (create_dynamic_target target_name source_files target_folder)
    add_library(${target_name} SHARED ${source_files})
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dynamic_libs"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dynamic_libs"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dynamic_libs"
    )
    if (target_folder)
        set_target_properties(${target_name} PROPERTIES FOLDER ${target_folder})
    endif()
    install(TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/dynamic_libs)
endfunction()