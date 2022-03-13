#include "predefinitions_tapi.h"
#include "array_of_strings_tapi.h"
#include "virtualmemorysys_tapi.h"
#include "registrysys_tapi.h"
#include "unittestsys_tapi.h"

//FIX: This should be specified by the flag!
#define tapi_average_string_len 100

//TO-DO: Add debug build checks

virtualmemorysys_tapi* virmem_sys;
unsigned int pagesize;
array_of_strings_tapi_type* aos;
array_of_strings_tapi_d* aos_sys_d;
array_of_strings_standardallocator_tapi* aos_standard;
array_of_strings_virtualallocatorsys_tapi* aos_virtual;

typedef struct array_of_strings_tapi_d{
    array_of_strings_tapi_type* aos_sys;
} array_of_strings_tapi_d;

typedef struct standard_array_of_strings{
    char* list_of_chars;
    unsigned long* list_of_ptrs;
    unsigned long ptr_list_capacity, ptr_list_len, 
        char_list_len;  //char list is always filled and searched, so there is no need for capacity
} standard_array_of_strings;

array_of_strings_tapi* standard_create_array(unsigned int initial_size, unsigned int char_expand_size){
    standard_array_of_strings* array = (standard_array_of_strings*)malloc(sizeof(standard_array_of_strings));
    array->ptr_list_len = 0;
    array->ptr_list_capacity = initial_size;
    if(array->ptr_list_capacity){
        array->list_of_ptrs = malloc(array->ptr_list_capacity * sizeof(array->list_of_ptrs[0]));
    }
    array->list_of_chars = NULL;
    
    return array;
}
const char* standard_read_string(const array_of_strings_tapi* array_o, unsigned int element_index){
    const standard_array_of_strings* array = (const standard_array_of_strings*)array_o;
    return array->list_of_chars[array->list_of_ptrs[element_index]];
}

unsigned long find_a_place_for_string(char* list_of_chars, unsigned long char_list_len, unsigned long len_str){
    unsigned long current_pointer = 0, found_size = 0;
    while(current_pointer < char_list_len){
        if(list_of_chars[current_pointer] == -1){
            found_size++;
            if(found_size == len_str){
                return current_pointer - found_size;
            }
        }
        else{
            current_pointer += strlen(&list_of_chars[current_pointer]);
        }
    }
    return ULONG_MAX;
}
unsigned long standard_expand_char_array(standard_array_of_strings* array){
    char* newlist = (char*)malloc(array->char_list_len * 2);
    memcpy(newlist, array->list_of_chars, array->char_list_len);
    free(array->list_of_chars);
    array->char_list_len *= 2;
    array->list_of_chars = newlist;
    return array->char_list_len / 2;
}
void standard_push_back(array_of_strings_tapi* array_o, const char* string){
    standard_array_of_strings* array = (standard_array_of_strings*)array_o;
    unsigned long len_str = strlen(string) + 1;
    
    //Expand list of pointers if full
    if(array->ptr_list_len == array->ptr_list_capacity){
        array->ptr_list_capacity *= 2;
        unsigned long* new_list_of_ptrs = (unsigned long*)malloc(sizeof(array->list_of_ptrs[0]) * array->ptr_list_capacity);
        memcpy(new_list_of_ptrs, array->list_of_ptrs, array->ptr_list_len * sizeof(array->list_of_ptrs[0]));
        free(array->list_of_ptrs);
        array->list_of_ptrs = new_list_of_ptrs;
    }

    unsigned long found_place = find_a_place_for_string(array->list_of_chars, array->char_list_len, len_str);
    if(found_place == ULONG_MAX){
        found_place = standard_expand_char_array(array);
    }
    memcpy(&array->list_of_chars[found_place], string, len_str);
    array->list_of_ptrs[array->ptr_list_len++] = found_place;
}
void standard_delete_string(array_of_strings_tapi* array_o, unsigned long element_index){
    standard_array_of_strings* array = (standard_array_of_strings*)array_o;
    memset(&array->list_of_chars[array->list_of_ptrs[element_index]], -1, strlen(&array->list_of_chars[array->list_of_ptrs[element_index]]));
    for(unsigned long i = element_index + 1; i < array->ptr_list_capacity; i++){
        array->list_of_ptrs[i - 1] = array->list_of_ptrs[i];
    }
    array->ptr_list_len--;
}
void standard_change_string(array_of_strings_tapi* array_o, unsigned long element_index, const char* string){
    standard_array_of_strings* array = (standard_array_of_strings*)array_o;
    unsigned long len_str = strlen(string) + 1;
    unsigned long previous_str_len = strlen(array->list_of_chars[array->list_of_ptrs[element_index]]) + 1;
    if(len_str <= previous_str_len){
        memcpy(&array->list_of_chars[array->list_of_ptrs[element_index]], string, len_str);
        memset(&array->list_of_chars[array->list_of_ptrs[element_index] + len_str], -1, previous_str_len - len_str);
        return;
    }
    
    unsigned long found_place = find_a_place_for_string(array->list_of_chars, array->char_list_len, len_str);
    if(found_place == ULONG_MAX){
        found_place = standard_expand_char_array(array);
    }
    memcpy(&array->list_of_chars[found_place], string, len_str);
    memset(&array->list_of_chars[array->list_of_ptrs[element_index]], -1, previous_str_len);
    array->list_of_ptrs[element_index] = found_place;
}


typedef struct virtual_array_of_strings{
    char* list_of_chars;
    unsigned long* list_of_ptrs;
    unsigned int all_memoryblock_len, committed_charpagecount, committed_ptrpagecount;
} virtual_array_of_strings;

array_of_strings_tapi* virtual_create_array(void* virtualmem_loc, unsigned long memsize, unsigned long allocator_flag){
    //Commit to place object at the beginning
    virmem_sys->virtual_commit(virtualmem_loc, pagesize);
    //Place base object at the beginning
    virtual_array_of_strings* array = (virtual_array_of_strings*)virtualmem_loc;
    array->all_memoryblock_len = memsize;
    
    //Place ptr list at the end of base object
    array->list_of_ptrs = (unsigned long*)((char*)array + sizeof(virtual_array_of_strings));
    //Place char list far away from the ptr list's start
    //Placement is like this: page0[baseobj - ptrlist_beginning], page1-x[ptrlist_continue], pagex-end[charlist]
    //[x,end] range is "(tapi_average_string_len / sizeof(unsigned int))-1" times bigger than [1-x] range
    array->list_of_chars = (char*)array + (pagesize * (memsize / pagesize) / (tapi_average_string_len / sizeof(unsigned int)));
    virmem_sys->virtual_commit((char*)array + pagesize, pagesize);
    array->committed_ptrpagecount = 1;

    //Commit to make array usable
    virmem_sys->virtual_commit(array->list_of_chars, pagesize);
    array->committed_charpagecount = 1;
    unsigned int i = 0;
    while(i < pagesize){array->list_of_chars[i] = -1; i++;}

    return array;
}
const char* virtual_read_string(const array_of_strings_tapi* array_o, unsigned int element_index){
    virtual_array_of_strings* array = (virtual_array_of_strings*)array_o;
    if(element_index < ((array->committed_ptrpagecount * pagesize) + (pagesize - sizeof(virtual_array_of_strings)))/ 4){
        return &array->list_of_chars[array->list_of_ptrs[element_index]];
    }
    return NULL;
}
unsigned long virtual_change_string(array_of_strings_tapi* array_o, unsigned int element_index, const char* string){
    virtual_array_of_strings* array = (virtual_array_of_strings*)array_o;
    unsigned int length = strlen(string) + 1;
    unsigned int i = 0;
    //Check if its time to expand ptr list with a commit
    if(element_index % (pagesize / sizeof(unsigned int)) == pagesize / sizeof(unsigned int) - 1){
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
            array->list_of_ptrs[element_index] = i - current_length;
            return;
        }
        i++;
    }
    virmem_sys->virtual_commit(array->list_of_chars + i, array->committed_charpagecount * pagesize);
    array->committed_charpagecount *= 2;
    memcpy(&array->list_of_chars[i], string, length);
    array->list_of_ptrs[element_index] = i;
    return ULONG_MAX;
}
//returned: status of the operation depending on the given allocator_flag while creating the array
unsigned long virtual_delete_string(array_of_strings_tapi* array_o, unsigned int element_index){
    virtual_array_of_strings* array = (virtual_array_of_strings*)array_o;
    if((element_index < ((array->committed_ptrpagecount * pagesize) + (pagesize - sizeof(virtual_array_of_strings)))/ 4)){
        char* stringptr;
        stringptr = &(array->list_of_chars[array->list_of_ptrs[element_index]]);
        for(unsigned int i = 0; i < strlen(stringptr) + 1; i++){
            stringptr[i] = -1;
        }
        array->list_of_ptrs[element_index] = UINT_MAX;
        return ULONG_MAX;
    }
}

//Unit Tests
unsigned char ut_0(const char** output_string, void* data){
    *output_string = "Hello from ut_0!";
    return 255;
}

void set_unittests(registrysys_tapi* reg_sys){
    UNITTEST_TAPI_LOAD_TYPE ut_sys = (UNITTEST_TAPI_LOAD_TYPE)reg_sys->get(UNITTEST_TAPI_PLUGIN_NAME, 0, 0);
    //If unit test system is available, add unit tests to it
    if(ut_sys){
        unittest_interface_tapi x;
        x.test = &ut_0;
        ut_sys->funcs->add_unittest("array_of_strings", 0, x);
    }
    else{
        printf("Array of Strings: Unit tests aren't implemented because unit test system isn't available!");
    }
}

FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* reg_sys, unsigned char reload){
    virtualmemorysys_tapi_type* virmemtype = (VIRTUALMEMORY_TAPI_PLUGIN_TYPE)reg_sys->get(VIRTUALMEMORY_TAPI_PLUGIN_NAME, MAKE_PLUGIN_VERSION_TAPI(0, 0, 0), 0);
    virmem_sys = virmemtype->funcs;
    pagesize = virmem_sys->get_pagesize();
    set_unittests(reg_sys);
    

    aos = (array_of_strings_tapi_type*)malloc(sizeof(array_of_strings_tapi_type));
    aos->standard_allocator = (array_of_strings_standardallocator_tapi*)malloc(sizeof(array_of_strings_standardallocator_tapi));
    aos->virtual_allocator = (array_of_strings_virtualallocatorsys_tapi*)malloc(sizeof(array_of_strings_virtualallocatorsys_tapi));
    aos->data = (array_of_strings_tapi_d*)malloc(sizeof(array_of_strings_tapi_d));
    aos_sys_d = aos->data;
    aos_standard = aos->standard_allocator;
    aos_virtual = aos->virtual_allocator;

    aos_standard->change_string = &standard_change_string;
    aos_standard->create_array_of_string = &standard_create_array; 
    aos_standard->delete_string = &standard_delete_string;
    aos_standard->push_back = &standard_push_back;
    aos_standard->read_string = &standard_read_string;


    aos_virtual->change_string = &virtual_change_string;
    aos_virtual->create_array_of_string = &virtual_create_array;
    aos_virtual->delete_string = &virtual_delete_string;
    aos_virtual->read_string = &virtual_read_string;

    reg_sys->add(ARRAY_OF_STRINGS_TAPI_PLUGIN_NAME, ARRAY_OF_STRINGS_TAPI_VERSION, virmemtype);

    return aos;
}

void unload_plugin(registrysys_tapi* reg_sys, unsigned char reload){
    free(aos_standard);
    free(aos_virtual);
    free(aos_sys_d);
    free(aos);
}
