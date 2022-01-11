#include "allocator_tapi.h"
#include "registrysys_tapi.h"
#include "virtualmemorysys_tapi.h"

#include <map>

class EndOfPageSystem{
    public:
    std::map<void*, unsigned long> pages;
};

typedef struct allocator_tapi_d{
    allocator_tapi* type;
    virtualmemorysys_tapi* virmem;
    unsigned int pagesize = 0;
    EndOfPageSystem* eop = nullptr;
} allocator_tapi_d;
static allocator_tapi_d* data = nullptr;

void* standard_malloc(unsigned long size){
    return malloc(size);
}
void standard_free(void* location){
    free(location);
}


//Based on The Machinery's end of page allocator
void* endofpage_malloc(unsigned long size){
        const unsigned long new_size_up = (size + data->pagesize - 1) / data->pagesize * data->pagesize;
        void* base = data->virmem->virtual_reserve(new_size_up);
        data->eop->pages.insert(std::make_pair(base, new_size_up));
        data->virmem->virtual_commit(base, new_size_up);
        const unsigned long offset = new_size_up - size;
        return (unsigned char*)base + offset;
}
void endofpage_free(void* location){
    auto pair = data->eop->pages.find(location);
    if(pair == data->eop->pages.end()){return;}
    data->virmem->virtual_free(location, pair->second);
}

void* novirmem_endofpage_malloc(unsigned long size){return nullptr;}
void novirmem_endofpage_free(void* location){}


extern "C" FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* regsys, unsigned char reload){
    allocator_tapi* type = (allocator_tapi*)malloc(sizeof(allocator_tapi));
    type->data = (allocator_tapi_d*)malloc(sizeof(allocator_tapi_d));
    type->end_of_page = (endofpage_allocator_tapi*)malloc(sizeof(endofpage_allocator_tapi));
    type->standard = (standard_allocator_tapi*)malloc(sizeof(standard_allocator_tapi));

    type->data->type = type;
    data = type->data;

    type->standard->malloc = &standard_malloc;
    type->standard->free = &standard_free;


    VIRTUALMEMORY_TAPI_PLUGIN_TYPE virmemtype = (VIRTUALMEMORY_TAPI_PLUGIN_TYPE)regsys->get(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, 0);
    if(virmemtype){
        type->end_of_page->malloc = endofpage_malloc;
        type->end_of_page->free = endofpage_free;
        type->data->eop = new EndOfPageSystem;
        type->data->pagesize = virmemtype->funcs->get_pagesize();
        type->data->virmem = virmemtype->funcs;
    }
    else{type->end_of_page->malloc = &novirmem_endofpage_malloc;     type->end_of_page->free = &novirmem_endofpage_free;}
    
    

    regsys->add(ALLOCATOR_TAPI_PLUGIN_NAME, ALLOCATOR_TAPI_PLUGIN_VERSION, type);

    return type;
}

extern "C" FUNC_DLIB_EXPORT void unload_plugin(registrysys_tapi* regsys, unsigned char reload){

} 