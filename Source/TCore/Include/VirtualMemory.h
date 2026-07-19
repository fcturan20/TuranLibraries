#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE
TCORE_PLUGIN_DEFINE(TCVirtualMemory, "tcVirtualMemory", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

typedef struct ITCVirtualMemory
{
	// Reserve address space from virtual memory
	// size is in bytes
	void* (*Reserve)(TU8 size);
	// Initialize the reserved memory with zeros.
	void (*Commit)(void* reservedmem, TU8 commitsize);
	// Return back the committed memory to reserved state
	// This will help if you want to catch some bugs that points to memory you just freed.
	void (*Decommit)(void* committedmem, TU8 size);
	// Return allocated address space back to OS
	void (*Free)(void* ptr, TU8 size);
	// Get page size of the system
	TU4 (*GetPageSize)();
} ITCVirtualMemory;

TCORE_END_C_LINKAGE