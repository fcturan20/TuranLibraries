#define TCORE_INCLUDE_PLATFORM_LIBS
#include "VirtualMemory.h"

#include <stdio.h>

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCVirtualMemory)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCVirtualMemory)
TCORE_PLUGIN_ENTRY_POINT_END()

#ifndef T_ENVWINDOWS
#error Virtual memory system isn't supported for your platform, virtualmemorysys_tapi.c for more information
#else

struct TCVirtualMemoryContext {
  // Reserve address space from virtual memory
  static void* Reserve(unsigned long long size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
  }

  // Initialize the reserved memory with zeros.
  static void Commit(void* ptr, unsigned long long commitsize) {
    if (VirtualAlloc(ptr, commitsize, MEM_COMMIT, PAGE_READWRITE) == NULL && commitsize) {
      LPVOID msgBuf;
      DWORD  dw = GetLastError();

      FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ( LPTSTR )&msgBuf, 0, NULL);

      printf("Virtual Alloc failed: %s", msgBuf);
    }
  }

  // Return back the committed memory to reserved state
  // This will help if you want to catch some bugs that points to memory you just freed.
  static void Decommit(void* ptr, unsigned long long size) { VirtualFree(ptr, size, MEM_DECOMMIT); }

  // Free the pages allocated
  static void VirtualFree(void* ptr, unsigned long long commit) {
    VirtualFree(ptr, commit, MEM_RELEASE);
  }

  static unsigned int GetPageSize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
  }
};

TCResult TCVirtualMemory_Initialize(const void** outPluginAPI) {
  TCVirtualMemoryServices* services = new TCVirtualMemoryServices;
  services->Reserve                 = TCVirtualMemoryContext::Reserve;
  services->Commit                  = TCVirtualMemoryContext::Commit;
  services->Decommit                = TCVirtualMemoryContext::Decommit;
  services->VirtualFree             = TCVirtualMemoryContext::VirtualFree;
  services->GetPageSize             = TCVirtualMemoryContext::GetPageSize;

  TCVirtualMemory = services;
  *outPluginAPI   = TCVirtualMemory;
  return TC_RESULT_SUCCESS;
}

TCResult TCVirtualMemory_OnPreShutdown() { return TC_RESULT_SUCCESS; }

TCResult TCVirtualMemory_Shutdown() {
  if (TCVirtualMemory) {
    delete TCVirtualMemory;
    TCVirtualMemory = nullptr;
  }
  return TC_RESULT_SUCCESS;
}

void TCVirtualMemory_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded) {
  // This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}

#endif