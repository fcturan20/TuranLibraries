#include "virtualmemorysys_tapi.h"

#include <stdio.h>

#include "allocator_tapi.h"
#include "ecs_tapi.h"
#include "predefinitions_tapi.h"
#include "registrysys_tapi.h"

#ifdef T_ENVWINDOWS
#include "windows.h"

static VIRTUALMEMORY_TAPI_PLUGIN_TYPE  memsys    = NULL;
static ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE allocator = NULL;
// Reserve address space from virtual memory
void* virtual_reserve(unsigned long long size) {
  return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}

// Initialize the reserved memory with zeros.
void virtual_commit(void* ptr, unsigned long long commitsize) {
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
void virtual_decommit(void* ptr, unsigned long long size) { VirtualFree(ptr, size, MEM_DECOMMIT); }

// Free the pages allocated
void virtual_free(void* ptr, unsigned long long commit) { VirtualFree(ptr, commit, MEM_RELEASE); }

unsigned int get_pagesize() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwPageSize;
}

void* get_virtualmemory(unsigned long long reserve_size, unsigned long long first_commit_size) {
  void* ptr = VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_READWRITE);
  VirtualAlloc(ptr, first_commit_size, MEM_COMMIT, PAGE_READWRITE);
  return ptr;
}

extern unsigned int getSizeOfAllocatorSys();
extern void*        initializeAllocator(void* allocatorSys);
void                load_virtualmemorysystem(VIRTUALMEMORY_TAPI_PLUGIN_TYPE*  virmemsysPTR,
                                             ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE* allocatorsysPTR) {
                 unsigned int sizeOfAllocation =
                   sizeof(virtualmemorysys_tapi_type) + sizeof(virtualmemorysys_tapi) + getSizeOfAllocatorSys();
                 memsys                          = malloc(sizeOfAllocation);
                 memsys->funcs                   = ( virtualmemorysys_tapi* )(memsys + 1);
                 memsys->data                    = NULL;
                 memsys->funcs->get_pagesize     = &get_pagesize;
                 memsys->funcs->virtual_commit   = &virtual_commit;
                 memsys->funcs->virtual_free     = &virtual_free;
                 memsys->funcs->virtual_reserve  = &virtual_reserve;
                 memsys->funcs->virtual_decommit = &virtual_decommit;

                 allocator = ( ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE )initializeAllocator(memsys->funcs + 1);

                 *virmemsysPTR    = memsys;
                 *allocatorsysPTR = allocator;
}
void get_virtualMemory_and_allocator_systems() {}

#else
#error Virtual memory system isn't supported for your platform, virtualmemorysys_tapi.c for more information
#endif
