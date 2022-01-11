#include "predefinitions.h"
#define VIRTUALMEMORY_TAPI_PLUGIN_NAME "tapi_virtualmemsys"
#define VIRTUALMEMORY_TAPI_PLUGIN_TYPE virtualmemorysys_tapi_type*
#define VIRTUALMEMORY_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0,0,0)


//RegistrySys Documentation: Storing Data for Systems
typedef struct virtualmemorysys_tapi_d virtualmemorysys_tapi_d;

typedef struct virtualmemorysys_tapi{
    //Reserve address space from virtual memory
    void* (*virtual_reserve)(unsigned long size);
    //Initialize the reserved memory with zeros.
    void (*virtual_commit)(void* reservedmem, unsigned long commitsize);
    //Return back the committed memory to reserved state
    //This will help if you want to catch some bugs that points to memory you just freed.
    void (*virtual_uncommit)(void* committedmem, unsigned long size);
    //Return allocated address space back to OS
    void (*virtual_free)(void* ptr, unsigned long size);
    //Get page size of the system
    unsigned int (*get_pagesize)();
    
    //Note: There may be additional functionality here to allocate memory that can be reached across applications.
} virtualmemorysys_tapi;

//Virtual Memory System
//System that allows reserve, commit and free operations across platforms
typedef struct virtualmemorysys_tapi_type{
    //RegistrySys Documentation: Storing Functions for Systems
    struct virtualmemorysys_tapi* funcs;
    //RegistrySys Documentation: Storing Data for Systems
    struct virtualmemorysys_tapi_d* data;
} virtualmemorysys_tapi_type;

