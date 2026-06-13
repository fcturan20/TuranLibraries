#pragma once
#ifdef __cplusplus
extern "C" {
#endif
  
#include "predefinitions_tapi.h"
#define FILESYS_TAPI_PLUGIN_NAME "tapi_filesys"
#define FILESYS_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define FILESYS_TAPI_PLUGIN_LOAD_TYPE const struct tlIO*

struct tlIO {
  const struct tlIOPriv* d;
  void* (*readBinary)(stringReadArgument_tapi(path), unsigned long* size);
  void (*writeBinary)(stringReadArgument_tapi(path), void* data, unsigned long size);
  void (*overwriteBinary)(stringReadArgument_tapi(path), void* data, unsigned long size);
  void* (*readText)(stringReadArgument_tapi(path), string_type_tapi fileTextType);
  void (*writeText)(stringReadArgument_tapi(text), stringReadArgument_tapi(path),
                         unsigned char writeToEnd);
  void (*deleteFile)(stringReadArgument_tapi(path));
};

#ifdef __cplusplus
}
#endif