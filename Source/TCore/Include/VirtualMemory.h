#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE
TCORE_PLUGIN_DEFINE(TCVirtualMemory, "tcVirtualMemory", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

typedef struct ITCVirtualMemory
{
	// Reserve address space from virtual memory
	// size is in bytes
	void* (*Reserve)(TSize size);
	// Initialize the reserved memory with zeros.
	void (*Commit)(void* reservedmem, TSize commitsize);
	// Return back the committed memory to reserved state
	// This will help if you want to catch some bugs that points to memory you just freed.
	void (*Decommit)(void* committedmem, TSize size);
	// Return allocated address space back to OS
	void (*Free)(void* ptr, TSize size);
	// Get page size of the system
	TUint (*GetPageSize)();
} ITCVirtualMemory;

TCORE_END_C_LINKAGE