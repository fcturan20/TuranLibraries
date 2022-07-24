#pragma once
#include "predefinitions_tapi.h"
#define ALLOCATOR_TAPI_PLUGIN_NAME "tapi_allocator"
#define ALLOCATOR_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0,0,0)
#define ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE allocator_sys_tapi*

//        ALLOCATOR API
/////////////////////////////////////
//  This API is used to provide memory allocator functionality as simple as possible
//  There are 3 types of allocators: 
//    1) Classic buffer allocators (allocator_tapi)
//    2) Vector allocators (std::vector with custom functionality)
//    3) List allocators (OrderedList, Hash etc.)
//  SuperMemoryBlock: A very big address space possibly for a system
//   Allocators allocates for a system by using a SuperMemoryBlock pointer
//   AllocatorSystem has its own address space for storing internal structures of SuperMemoryBlocks

typedef struct supermemoryblock_tapi supermemoryblock_tapi;

typedef struct buffer_allocator_tapi{
  void* (*malloc)(supermemoryblock_tapi* memBlock, unsigned int size);
  // @param returnedAllocPTR: This should be the same pointer as returned by malloc()
  void (*free)(supermemoryblock_tapi* memBlock, void* returnedAllocPTR);
  void (*set_malloc_name)(void* allocation, const char* name);
} buffer_allocator_tapi;


typedef void (*vector_element_constructor_func_tapi)(void* elementPtr);
typedef void (*vector_element_copy_func_tapi)(const void* srcElement, void* dstElement);
typedef void (*vector_element_destructor_func_tapi)(void* elementPtr);

//         VECTOR ALLOCATOR API
/////////////////////////////////////
// There is no static vector type info, so user should define their vectors by defining them in run-time
// Vectors allocate an adress space of "maxSize * elementSize" bytes from the allocator
// Then manages commitment of 

typedef enum vector_flagbits_tapi {
  vector_flagbit_constructor_tapi = 1,
  vector_flagbit_copy_tapi = 2,
  vector_flagbit_destructor_tapi = 4,
  vector_flagbit_invalidator_tapi = 8
} vector_flagbits_tapi;
typedef int vector_flags_tapi;
typedef struct vector_allocator_tapi {
  // Use maxSize as much as you can because vector struct and data will be right back each other.
  // So overall faster access etc.
  // @param elementSize: Size of a single element
  // @param memblock: Super memory block to allocate address from
  // @param initialSize: Elements to push at initialization
  // @param maxSize: Define an upper limit for element count
  // @param vectorFlag: Defines how variadic arguments are interpreted
  // @return Array pointer, so cast it to your own type
  void* (*create_vector)(unsigned int elementSize, supermemoryblock_tapi* memblock, unsigned int initialSize, unsigned int maxSize, vector_flags_tapi vectorFlag, ...);
  unsigned int (*size)(const void* hnd);
  unsigned int (*capacity)(const void* hnd);
  // @param src: Source data to copy from
  // @param copyFunc: Used to copy the element
  // @return 0 if fails; 1 if succeeds. May fail if mem allocation fais or upper limit is reached.
  unsigned char (*push_back)(void* hnd, const void* src);
  // @return 0 if there is no such object, 1 if succeeds
  unsigned char (*erase)(void* hnd, unsigned int elementIndex);
  // @param constructor: if new items will be added, this is used to initialize
  // @param destructor: if old items will be destroyed, this is used to destroy
  // @return 0 if fails; 1 if succeeds. May fail if mem allocation fais or upper limit is reached.
  unsigned char (*resize)(void* hnd, unsigned int newItemCount);
} vector_allocator_tapi;
#define TAPI_CREATEVECTOR(vectorAllocator, type, memblock, maxSize) ((type*)vectorAllocator->create_vector(sizeof(type), memblock, 0, maxSize, 0))
//Macro of TAPI_CREATEVECTOR. Vector allocator should be named f_vector, Super Memory Block should be named mainMemBlock.
#define TAPI_VECTOR(type, maxSize) ((type*)f_vector->create_vector(sizeof(type), mainMemBlock, 0, maxSize, 0))


typedef struct allocator_tapi_d allocator_tapi_d;
typedef struct allocator_sys_tapi{
  allocator_tapi_d* d;
  supermemoryblock_tapi* (*createSuperMemoryBlock)(unsigned long long blockSize, const char* superMemBlockName);
  void (*freeSuperMemoryBlock)(supermemoryblock_tapi* superMemBlock);
  vector_allocator_tapi* vector_manager;
  buffer_allocator_tapi* standard;
  // Allocations are rounded up to pagesize and an extra page is allocated
  // Use for debugging (out of space accesses will be crashes etc.)
  buffer_allocator_tapi* end_of_page;
} allocator_sys_tapi;