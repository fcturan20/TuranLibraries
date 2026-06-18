cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE gameAttachingPluginFiles "*.cpp" "*.h" "*.c")
tcmake_create_dynamic_target(TEditorGameLinker "${gameAttachingPluginFiles}" "TEditor")