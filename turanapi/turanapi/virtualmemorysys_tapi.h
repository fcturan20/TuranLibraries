#pragma once
#include "predefinitions_tapi.h"
#define VIRTUALMEMORY_TAPI_PLUGIN_NAME "tapi_virtualmemsys"
#define VIRTUALMEMORY_TAPI_PLUGIN_LOAD_TYPE struct tapi_virtualMemorySys_type*
#define VIRTUALMEMORY_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)

struct tapi_virtualMemorySys {
  // Reserve address space from virtual memory
  // size is in bytes
  void* (*virtual_reserve)(unsigned long long size);
  // Initialize the reserved memory with zeros.
  void (*virtual_commit)(void* reservedmem, unsigned long long commitsize);
  // Return back the committed memory to reserved state
  // This will help if you want to catch some bugs that points to memory you just freed.
  void (*virtual_decommit)(void* committedmem, unsigned long long size);
  // Return allocated address space back to OS
  void (*virtual_free)(void* ptr, unsigned long long size);
  // Get page size of the system
  unsigned int (*get_pagesize)();

  // Note: There may be additional functionality here to allocate memory that can be reached across
  // applications.
};

// Virtual Memory System
// System that allows reserve, commit and free operations across platforms
struct tapi_virtualMemorySys_type {
  // RegistrySys Documentation: Storing Functions for Systems
  struct tapi_virtualMemorySys* funcs;
  // RegistrySys Documentation: Storing Data for Systems
  struct tapi_virtualMemorySys_d* data;
};
