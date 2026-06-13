#include "predefinitions_tapi.h"
#include "registrysys_tapi.h"
#include "virtualmemorysys_tapi.h"
#include "stdio.h"

#define tapi_first_allocation_size (1 << 28)
#define tapi_first_commit_size (1 << 12)
#define tapi_average_plugin_name_len (70)

registrysys_tapi_type* const regsys;
registrysys_tapi_d* regsys_d;
registrysys_tapi* regsys_f;
virtualmemorysys_tapi* virmem_sys;

unsigned int pagesize, ptr_list_commit_index;

typedef struct regsys_strings_array{
    char* list_of_chars;
    long* list_of_ptrs;
    unsigned int all_memoryblock_len, committed_charpagecount, committed_ptrpagecount;
} regsys_strings_array;

//Give virtual address space that is allocated but not committed
regsys_strings_array* create_array_of_string(void* reserved_virtualmemblock_ptr, long length_to_reserved_end){
    //Calculate how much commit to memory at initialization
    unsigned int initialelements_totalsize = tapi_average_plugin_name_len * 4;
    unsigned int firstcommitsize = max(pagesize, initialelements_totalsize - (initialelements_totalsize % pagesize) + pagesize);

    //Commit to place object at the beginning
    virmem_sys->virtual_commit(reserved_virtualmemblock_ptr, firstcommitsize);
    //Place base object at the beginning
    regsys_strings_array* array = (regsys_strings_array*)reserved_virtualmemblock_ptr;
    array->all_memoryblock_len = length_to_reserved_end;
    
    //Place ptr list at the end of base object
    array->list_of_ptrs = (unsigned int*)((char*)array + sizeof(regsys_strings_array));
    //Place char list far away from the ptr list's start
    //Placement is like this: page0[baseobj - ptrlist_beginning], page1-x[ptrlist_continue], pagex-end[charlist]
    //[x,end] range is "(tapi_average_plugin_name_len / sizeof(unsigned int))-1" times bigger than [1-x] range
    array->list_of_chars = (char*)array + (pagesize * (length_to_reserved_end / pagesize) / (tapi_average_plugin_name_len / sizeof(unsigned int)));
    virmem_sys->virtual_commit((char*)array + pagesize, pagesize);
    array->committed_ptrpagecount = 1;

    //Commit to make array usable
    virmem_sys->virtual_commit(array->list_of_chars, pagesize);
    array->committed_charpagecount = 1;
    unsigned int i = 0;
    while(i < pagesize){array->list_of_chars[i] = -1; i++;}

    return array;
}

void copy_string_to_array(unsigned int index, regsys_strings_array* array, const char* string){
    unsigned long length = strlen(string) + 1;
    unsigned int i = 0;
    //Check if its time to expand ptr list with a commit
    if(index % (pagesize / sizeof(unsigned int)) == ptr_list_commit_index){
        virmem_sys->virtual_commit((char*)array + (pagesize * array->committed_ptrpagecount), array->committed_ptrpagecount * pagesize);
        array->committed_ptrpagecount *= 2;
    }
    while(i < (pagesize * array->committed_charpagecount)){
        unsigned int current_length = 0;
        while(array->list_of_chars[i] == -1){
            i++; current_length++;
        }
        if(current_length >= length){
            memcpy(&array->list_of_chars[i - current_length], string, length);
            array->list_of_ptrs[index] = i - current_length;
            return;
        }
        i++;
    }
    virmem_sys->virtual_commit(array->list_of_chars + i, array->committed_charpagecount * pagesize);
    array->committed_charpagecount *= 2;
    memcpy(&array->list_of_chars[i], string, length);
    array->list_of_ptrs[index] = i;
}

const char* get_string_from_array(regsys_strings_array* array, unsigned int index){
    return (index < ((array->committed_ptrpagecount * pagesize) + (pagesize - sizeof(regsys_strings_array)))/ 4) ? (&array->list_of_chars[array->list_of_ptrs[index]]) : (NULL);
}

void remove_string_from_array(regsys_strings_array* array, unsigned int index){
    if((index < ((array->committed_ptrpagecount * pagesize) + (pagesize - sizeof(regsys_strings_array)))/ 4)){
        char* stringptr;
        stringptr = &(array->list_of_chars[array->list_of_ptrs[index]]);
        for(unsigned int i = 0; i < strlen(stringptr) + 1; i++){
            stringptr[i] = -1;
        }
        array->list_of_ptrs[index] = UINT_MAX;
    }
}



unsigned int pagesize, ptr_list_commit_index;

void* get_virtualmemory(long reserve_size, unsigned int first_commit_size);
void* load_virtualmemorysystem();

typedef struct plugin_tapi_d{
    void* plugin_ptr;
    unsigned int plugin_version;
} plugin_tapi_d;

typedef struct registrysys_tapi_d{
    registrysys_tapi_type* registry_sys;
    plugin_tapi_d* plugin_list;
    regsys_strings_array* plugin_names;
    unsigned int plugin_capacity;
} registrysys_tapi_d;

void add_plugin(const char* name, unsigned int version, void* plugin_ptr){
    for(unsigned int i = 0; i < regsys_d->plugin_capacity; i++){
        if(regsys_d->plugin_list[i].plugin_ptr == NULL){
            regsys_d->plugin_list[i].plugin_ptr = plugin_ptr;
            copy_string_to_array(i, regsys_d->plugin_names, name);
            regsys_d->plugin_list[i].plugin_version = version;
            return;
        }
    }


    virmem_sys->virtual_commit(&regsys_d->plugin_list[regsys_d->plugin_capacity], (regsys_d->plugin_capacity / pagesize) * pagesize);

    regsys_d->plugin_list[regsys_d->plugin_capacity].plugin_ptr = plugin_ptr;
    copy_string_to_array(regsys_d->plugin_capacity, regsys_d->plugin_names, name);
    regsys_d->plugin_list[regsys_d->plugin_capacity].plugin_version = version;
    regsys_d->plugin_capacity += (regsys_d->plugin_capacity / (pagesize / sizeof(plugin_tapi_d))) * (pagesize / sizeof(plugin_tapi_d));
}

void remove_plugin(void* plugin_ptr){
    for(unsigned int i = 0; i < regsys_d->plugin_capacity; i++){
        if(regsys_d->plugin_list[i].plugin_ptr == plugin_ptr){
            regsys_d->plugin_list[i].plugin_ptr = NULL;
            regsys_d->plugin_list[i].plugin_version = MAKE_PLUGIN_VERSION_TAPI(255, 255, 255);
            remove_string_from_array(regsys_d->plugin_names, i);
        }
    }
}

void* get_plugin(const char* name, unsigned int version, unsigned char search_flag){
    for(unsigned int i = 0; i < regsys_d->plugin_capacity; i++){
        if(regsys_d->plugin_list[i].plugin_ptr == NULL){continue;}
        unsigned int name_ptr = regsys_d->plugin_names->list_of_ptrs[i];
        if(name_ptr == UINT_MAX){continue;}
        if(strcmp(&regsys_d->plugin_names->list_of_chars[name_ptr], name) == 0){
            //printf("Plugin Major: %u, Input Major: %u", GET_PLUGIN_VERSION_TAPI_MAJOR(regsys_d->plugin_list[i].plugin_version),
            //GET_PLUGIN_VERSION_TAPI_MAJOR(version));
            if(GET_PLUGIN_VERSION_TAPI_MAJOR(regsys_d->plugin_list[i].plugin_version) == GET_PLUGIN_VERSION_TAPI_MAJOR(version) || GET_PLUGIN_VERSION_TAPI_MAJOR(version) == 255){
                return regsys_d->plugin_list[i].plugin_ptr;
            }
        }
    }
    return NULL;
}

char** list_plugin(const char* name){
    if(name == NULL){
        unsigned int plugin_count = 0;
        for(unsigned int i = 0; i < regsys_d->plugin_capacity; i++){
            if(regsys_d->plugin_list[i].plugin_ptr != NULL){
                plugin_count++;
            }
        }
        char** list = malloc(sizeof(char*) * (plugin_count + 1));
        unsigned int validindex = 0;
        for(unsigned int i = 0; i < regsys_d->plugin_capacity; i++){
            if(regsys_d->plugin_list[i].plugin_ptr != NULL){
                unsigned long len = strlen(&regsys_d->plugin_names->list_of_chars[regsys_d->plugin_names->list_of_ptrs[i]]) + 1;
                list[validindex] = malloc(len);
                memcpy(list[validindex], &regsys_d->plugin_names->list_of_chars[regsys_d->plugin_names->list_of_ptrs[i]], len);
                validindex++;
            }
        }
        list[plugin_count] = NULL;
        return list;
    }
}

//Loads both registrysys_tapi_type and virtualmemorysys_tapi_type (because plugin data storage is handled with virtual memory)
FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* unused, unsigned char reload){
    //256MB address space is allocated and first 4KB of it is committed so we are free to use 4KB
    void* registry_sys_all_mem = get_virtualmemory(tapi_first_allocation_size, tapi_first_commit_size);

    //Set first 16 bytes of the memory as registry system's general struct storage place.
    registrysys_tapi_type* registry_system = (registrysys_tapi_type*)registry_sys_all_mem;  
    registry_system->data = (registrysys_tapi_d*)((char*)registry_system + sizeof(registrysys_tapi_type));
    registry_system->funcs = (registrysys_tapi*)((char*)registry_system + sizeof(registrysys_tapi_type) + sizeof(registrysys_tapi_d));


    regsys_d = registry_system->data;
    regsys_f = registry_system->funcs;


    regsys_f->add = &add_plugin;
    regsys_f->get = &get_plugin;
    regsys_f->list = &list_plugin;
    regsys_f->remove = &remove_plugin;

 
    regsys_d->plugin_capacity = (tapi_first_commit_size - sizeof(registrysys_tapi_type) - sizeof(registrysys_tapi_d) - sizeof(registrysys_tapi)) / sizeof(plugin_tapi_d);
    regsys_d->registry_sys = registry_system;
    regsys_d->plugin_list = (plugin_tapi_d*)((char*)regsys_f + sizeof(registrysys_tapi));
    virtualmemorysys_tapi_type* virmem_type = (virtualmemorysys_tapi_type*)load_virtualmemorysystem();
    regsys_d->plugin_list[0].plugin_ptr = virmem_type;
    virmem_sys = virmem_type->funcs;
    regsys_d->plugin_list[0].plugin_version = MAKE_PLUGIN_VERSION_TAPI(0,0,0);
    pagesize = virmem_sys->get_pagesize();

    //printf("Length of plugin list array: %u", ((tapi_first_allocation_size - regsys_d->allocation_size_remaining) / 16));
    regsys_d->plugin_names = create_array_of_string((char*)regsys_d + (((tapi_first_allocation_size / pagesize) * 15) / 16), (((tapi_first_allocation_size / pagesize) * 15) / 16));
    copy_string_to_array(0, regsys_d->plugin_names, VIRTUALMEMORY_TAPI_PLUGIN_NAME);

    return registry_system;
}

FUNC_DLIB_EXPORT void unload_plugin(registrysys_tapi* unused, unsigned char reload){
    unsigned int i;
    i = 0;
    while(i < regsys_d->plugin_capacity){
        if(regsys_d->plugin_list[i].plugin_ptr != NULL){
            printf("Plugin named \" %s \" isn't unloaded!", &regsys_d->plugin_names->list_of_chars[regsys_d->plugin_names->list_of_ptrs[i]]);
        }
    }

    free(regsys->data);
    free(regsys->funcs);
    free(regsys);
}