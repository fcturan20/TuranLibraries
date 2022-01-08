#include "virtualmemorysys_tapi.h"
#include "registrysys_tapi.h"

#ifdef T_ENVWINDOWS
#include "windows.h"

//Reserve address space from virtual memory
void* virtual_reserve(long size){
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}

//Initialize the reserved memory with zeros.
void virtual_commit(void* ptr, long commitsize){
    VirtualAlloc(ptr, commitsize, MEM_COMMIT, PAGE_READWRITE);
}

//Return back the committed memory to reserved state
//This will help if you want to catch some bugs that points to memory you just freed.
void virtual_uncommit(void* ptr, long size){
    VirtualFree(ptr, size, MEM_DECOMMIT);
}

    //Free the pages allocated
void virtual_free(void* ptr, long commit){
    VirtualFree(ptr, commit, MEM_RELEASE);
}

unsigned int get_pagesize(){
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

void* get_virtualmemory(long reserve_size, unsigned int first_commit_size){
    void* ptr = VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_READWRITE);
    VirtualAlloc(ptr, first_commit_size, MEM_COMMIT, PAGE_READWRITE);
    return ptr;
}

void* load_virtualmemorysystem(){
    virtualmemorysys_tapi_type* memsys = malloc(sizeof(virtualmemorysys_tapi_type));
    memsys->funcs = malloc(sizeof(virtualmemorysys_tapi));
    memsys->data = NULL;
    memsys->funcs->get_pagesize = &get_pagesize;
    memsys->funcs->virtual_commit = &virtual_commit;
    memsys->funcs->virtual_free = &virtual_free;
    memsys->funcs->virtual_reserve = &virtual_reserve;
    memsys->funcs->virtual_uncommit = &virtual_uncommit;
    return memsys;
}

#else
#error Virtual memory system isn't supported for your platform, virtualmemorysys_tapi.c for more information
#endif


