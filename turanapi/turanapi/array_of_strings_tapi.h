#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "predefinitions_tapi.h"

#define ARRAY_OF_STRINGS_TAPI_PLUGIN_NAME "tapi_array_of_strings_sys"
#define ARRAY_OF_STRINGS_TAPI_LOAD_TYPE array_of_strings_tapi_type*
#define ARRAY_OF_STRINGS_TAPI_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 1)
//Create allocator flag (but not coded for now)
#define ALLOCATOR_FLAG_TAPI() 0

typedef struct tapi_array_of_strings* array_of_strings_tapi;

//Uses classic malloc
//If you want to get all valid strings one after sorted by their element index, new buffer is created and returned. That means don't forget to delete it.
//Element indexes are dynamic, means that if an element is deleted every other element that comes after changes index (element index a is deleted, element index a+1 becomes a etc.)
typedef struct array_of_strings_standardallocator_tapi{
    //You can set both of the arguments to 0, it is just for optimization
    array_of_strings_tapi* (*create_array_of_string)(unsigned long initial_size, unsigned long char_expand_size);
    const char* (*read_string)(const array_of_strings_tapi* array, unsigned long element_index);
    void (*change_string)(array_of_strings_tapi* array, unsigned long element_index, const char* string);
    void (*push_back)(array_of_strings_tapi* array, const char* string);
    void (*delete_string)(array_of_strings_tapi* array, unsigned long element_index);
} array_of_strings_standardallocator_tapi;

//Uses virtual memory to allocate buffers
//Also there is no re-aligning an element if a previous element is deleted. To be more clear; each element is always has the same index, no 
typedef struct array_of_strings_virtualallocatorsys_tapi{
    //virtualmem_loc: location of the reserved (but not committed) address space
    //memsize: how much bytes are allowed to be used
    //allocator_flag (use with ALLOCATOR_FLAG_TAPI): specify virtual memory tricks
    array_of_strings_tapi* (*create_array_of_string)(void* virtualmem_loc, unsigned long memsize, unsigned long allocator_flag);
    const char* (*read_string)(const array_of_strings_tapi* array, unsigned long element_index);
    //returned: status of the operation depending on the given allocator_flag while creating the array
    unsigned long (*change_string)(array_of_strings_tapi* array, unsigned long element_index, const char* string);
    //returned: status of the operation depending on the given allocator_flag while creating the array
    unsigned long (*delete_string)(array_of_strings_tapi* array, unsigned long element_index);
} array_of_strings_virtualallocatorsys_tapi;

typedef struct array_of_strings_tapi_d array_of_strings_tapi_d;
//Array of Strings System
typedef struct array_of_strings_tapi_type{
    array_of_strings_tapi_d* data;
    array_of_strings_standardallocator_tapi* standard_allocator;
    array_of_strings_virtualallocatorsys_tapi* virtual_allocator;
} array_of_strings_tapi_type;

