

typedef struct tapi_end_of_page_allocator{
    void* (*malloc)(unsigned int size);
} tapi_eop_allocator;

