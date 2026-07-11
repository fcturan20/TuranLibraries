tcmake_add_subdirectory_if_exists("TCore")
tcmake_add_subdirectory_if_exists("TGfx")
#tcmake_add_subdirectory_if_exists("TEditor")

# Examples
tcmake_add_subdirectory_if_exists("TExamples")

# Executables to manage TuranLibraries components and dependencies
# Package manager, Tester etc.
tcmake_add_subdirectory_if_exists("TManager")