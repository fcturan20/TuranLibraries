cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE uiPluginFiles "*.cpp" "*.h" "*.c")
tcmake_create_dynamic_target(TEditorUi "${uiPluginFiles}" "TEditor")