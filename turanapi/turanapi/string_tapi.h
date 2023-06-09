#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>

#include "predefinitions_tapi.h"

#define STRINGSYS_TAPI_PLUGIN_NAME "tapi_string"
#define STRINGSYS_TAPI_PLUGIN_LOAD_TYPE struct tapi_stringSys_type*
#define STRINGSYS_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 1)
// Create allocator flag (but not coded for now)
#define ALLOCATOR_FLAG_TAPI() 0

enum tapi_string_type { string_type_tapi_UTF8, string_type_tapi_UTF16, string_type_tapi_UTF32 };

struct tapi_stringSys {
  void (*convertString)(stringReadArgument_tapi(src), stringWriteArgument_tapi(dst),
                        unsigned long long maxLen);
  // Use these to create a single string from variadic input
  // Calling free() is enough to destroy
  // %v Wide string, %s Char string, %u uint32, %d int32, %f f32-64, %p pointer
  // %lld int64, %llu uint64
  void (*createString)(enum tapi_string_type targetType, void** target, const wchar_t* format, ...);
  void (*vCreateString)(enum tapi_string_type targetType, void** target, const wchar_t* format,
                        va_list arg);
};

/*
typedef struct tapi_array_of_strings* array_of_strings_tapi;

// Uses classic malloc
// If you want to get all valid strings one after sorted by their element index, new buffer is
// created and returned. That means don't forget to delete it. Element indexes are dynamic, means
// that if an element is deleted every other element that comes after changes index (element index a
// is deleted, element index a+1 becomes a etc.)
typedef struct array_of_strings_standardallocator_tapi {
  // You can set both of the arguments to 0, it is just for optimization
  array_of_strings_tapi* (*create_array_of_string)(unsigned long long initial_size,
                                                   unsigned long long char_expand_size);
  const char* (*read_string)(const array_of_strings_tapi* array, unsigned long long element_index);
  void (*change_string)(array_of_strings_tapi* array, unsigned long long element_index,
                        const char* string);
  void (*push_back)(array_of_strings_tapi* array, const char* string);
  void (*delete_string)(array_of_strings_tapi* array, unsigned long long element_index);
} array_of_strings_standardallocator_tapi;

// Uses virtual memory to allocate buffers
// Also there is no re-aligning an element if a previous element is deleted. To be more clear; each
// element is always has the same index, no
typedef struct array_of_strings_virtualallocatorsys_tapi {
  // virtualmem_loc: location of the reserved (but not committed) address space
  // memsize: how much bytes are allowed to be used
  // allocator_flag (use with ALLOCATOR_FLAG_TAPI): specify virtual memory tricks
  array_of_strings_tapi* (*create_array_of_string)(void* virtualmem_loc, unsigned long long memsize,
                                                   unsigned long long allocator_flag);
  const char* (*read_string)(const array_of_strings_tapi* array, unsigned long long element_index);
  // returned: status of the operation depending on the given allocator_flag while creating the
  // array
  unsigned long long (*change_string)(array_of_strings_tapi* array,
                                      unsigned long long element_index, const char* string);
  // returned: status of the operation depending on the given allocator_flag while creating the
  // array
  unsigned long long (*delete_string)(array_of_strings_tapi* array,
                                      unsigned long long     element_index);
} array_of_strings_virtualallocatorsys_tapi;
*/

// Array of Strings System
struct tapi_stringSys_type {
  struct tapi_stringSys_d* data;
  struct tapi_stringSys*   standardString;
  // array_of_strings_standardallocator_tapi*   standard_allocator;
  // array_of_strings_virtualallocatorsys_tapi* virtual_allocator;
};

#ifdef __cplusplus
}
#endif