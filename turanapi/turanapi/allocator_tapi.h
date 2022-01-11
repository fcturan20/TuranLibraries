#include "predefinitions.h"
#define ALLOCATOR_TAPI_PLUGIN_NAME "tapi_allocator"
#define ALLOCATOR_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0,0,0)
#define ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE allocator_tapi*

typedef struct standard_allocator_tapi{
    void* (*malloc)(unsigned long size);
    void (*free)(void* location);
}   standard_allocator_tapi;

typedef struct endofpage_allocator_tapi{
    //Allocation is rounded up to pagesize and an extra page is allocated
    void* (*malloc)(unsigned long size);
    void (*free)(void* location);
}   endofpage_allocator_tapi;

typedef struct allocator_tapi_d allocator_tapi_d;
typedef struct allocator_tapi{
    allocator_tapi_d* data;
    standard_allocator_tapi* standard;
    endofpage_allocator_tapi* end_of_page;
} allocator_tapi;