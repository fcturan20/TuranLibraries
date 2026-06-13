cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE scenePluginFiles "*.cpp" "*.h" "*.c")
tcmake_create_dynamic_target(editor_scene "${scenePluginFiles}" "TEditor")