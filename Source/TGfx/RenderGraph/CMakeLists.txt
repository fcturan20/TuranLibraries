cmake_minimum_required (VERSION 3.8)


file(GLOB_RECURSE rendergraphfiles "main.cpp")
tcmake_create_dynamic_target(TGfxRenderGraph "${rendergraphfiles}" "TGfx")