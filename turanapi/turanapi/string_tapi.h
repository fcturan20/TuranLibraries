#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>

#include "predefinitions_tapi.h"

#define STRINGSYS_TAPI_PLUGIN_NAME "tapi_string"
#define STRINGSYS_TAPI_PLUGIN_LOAD_TYPE const struct tlString*
#define STRINGSYS_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 1)

enum tlStringType { tlStringUTF8, tlStringUTF16, tlStringUTF32 };

struct tlString {
  const struct tlStringPriv* d;
  void (*convertString)(stringReadArgument_tapi(src), stringWriteArgument_tapi(dst),
                        unsigned long long maxLen);
  // Use these to create a single string from variadic input
  // Calling free() is enough to destroy
  // %v Wide string, %s Char string, %u uint32, %d int32, %f f32-64, %p pointer
  // %lld int64, %llu uint64
  void (*createString)(enum tlStringType targetType, void** target, const wchar_t* format, ...);
  void (*vCreateString)(enum tlStringType targetType, void** target, const wchar_t* format,
                        va_list arg);
};

#ifdef __cplusplus
}
#endif