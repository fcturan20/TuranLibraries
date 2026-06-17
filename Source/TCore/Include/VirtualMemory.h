#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE
TCORE_PLUGIN_DEFINE(TCVirtualMemory, "tcVirtualMemory", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

struct TCVirtualMemoryServices {
  // Reserve address space from virtual memory
  // size is in bytes
  void* (*Reserve)(unsigned long long size);
  // Initialize the reserved memory with zeros.
  void (*Commit)(void* reservedmem, unsigned long long commitsize);
  // Return back the committed memory to reserved state
  // This will help if you want to catch some bugs that points to memory you just freed.
  void (*Decommit)(void* committedmem, unsigned long long size);
  // Return allocated address space back to OS
  void (*VirtualFree)(void* ptr, unsigned long long size);
  // Get page size of the system
  unsigned int (*GetPageSize)();
};

TCORE_END_C_LINKAGE