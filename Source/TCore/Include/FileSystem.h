#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCFileSystem, "tcFileSystem", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

// All path and texts should be UTF-8 encoded with null terminator
typedef struct ITCFileSystem
{
	void* (*ReadBinaryFile)(const char* path, TU8* size);
	void (*WriteBinaryFile)(const char* path, void* data, TU8 size);
	void (*OverwriteBinaryFile)(const char* path, void* data, TU8 size);
	void* (*ReadTextFile)(const char* path, TU8* size);
	void (*WriteTextFile)(const char* text, const char* path, TBool writeToEnd);
	void (*DeleteFile)(const char* path);
} ITCFileSystem;

TCORE_END_C_LINKAGE