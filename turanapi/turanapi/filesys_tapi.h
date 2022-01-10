#include "predefinitions.h"
#define FILESYS_TAPI_PLUGIN_NAME "tapi_filesys"
#define FILESYS_TAPI_PLUGIN_VERSION  MAKE_PLUGIN_VERSION_TAPI(0,0,0)
#define FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys_tapi_type*



typedef struct filesys_tapi{
    void* (*read_binaryfile)(const char* path, unsigned long* size);
    void (*write_binaryfile)(const char* path, void* data, unsigned long size);
    void (*overwrite_binaryfile)(const char* path, void* data, unsigned long size);
    char* (*read_textfile)(const char* path);
    void (*write_textfile)(const char* text, const char* path, unsigned char write_to_end);
    void (*delete_file)(const char* path);
} filesys_tapi;

typedef struct filesys_tapi_d filesys_tapi_d;
typedef struct filesys_tapi_type{
    filesys_tapi_d* data;
    filesys_tapi* funcs;
} filesys_tapi_type;
