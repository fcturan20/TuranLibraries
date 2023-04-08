#pragma once
#include "predefinitions_tapi.h"
#define FILESYS_TAPI_PLUGIN_NAME "tapi_filesys"
#define FILESYS_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys_tapi_type*

typedef struct filesys_tapi {
  void* (*read_binaryfile)(stringArgument_tapi(path), unsigned long* size);
  void (*write_binaryfile)(stringArgument_tapi(path), void* data, unsigned long size);
  void (*overwrite_binaryfile)(stringArgument_tapi(path), void* data, unsigned long size);
  void* (*read_textfile)(stringArgument_tapi(path), string_type_tapi fileTextType);
  void (*write_textfile)(stringArgument_tapi(text), stringArgument_tapi(path),
                         unsigned char writeToEnd);
  void (*delete_file)(stringArgument_tapi(path));
} filesys_tapi;

typedef struct filesys_tapi_d filesys_tapi_d;
typedef struct filesys_tapi_type {
  filesys_tapi_d* data;
  filesys_tapi*   funcs;
} filesys_tapi_type;
