cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE tester_files "Tester.cpp")
tcmake_create_executable_target(TTester "${tester_files}" "TManager")