#include "string_tapi.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "ecs_tapi.h"
#include "predefinitions_tapi.h"
#include "unittestsys_tapi.h"
#include "allocator_tapi.h"

// FIX: This should be specified by the flag!
#define tapi_average_string_len 100

// TO-DO: Add debug build checks

const tlVM*                     virmem_sys;
unsigned int                               pagesize;
tlString*                            stringSys;


#define tStrPrint(buf, bufIter, count, format, p)                           \
  if (targetType == tlStringUTF8) {                                         \
    count = snprintf((( char* )##buf) + bufIter, count, format, p);         \
  } else if (targetType == tlStringUTF16) {                        \
    count = _snwprintf((( wchar_t* )##buf) + bufIter, count, L##format, p); \
  }
struct tlStringPriv {
  static void convertString(stringReadArgument_tapi(src), stringWriteArgument_tapi(dst),
                       uint64_t maxLen) {
    switch (srcType) {
      case tlStringUTF8: {
        uint64_t wholeSrcStrLen = strlen(( const char* )srcData) + 1;
        uint64_t copyLen        = __min(wholeSrcStrLen, maxLen);
        switch (dstType) {
          case tlStringUTF8: {
            memcpy(dstData, srcData, copyLen);
          } break;
          case tlStringUTF16: {
            mbstowcs(( wchar_t* )dstData, ( const char* )srcData, wholeSrcStrLen);
          } break;
          default: assert(0 && "This string type isn't supported to be converted");
        }
      } break;
      case tlStringUTF16: {
        uint64_t wholeSrcStrLen = wcslen(( const wchar_t* )srcData) + 1;
        uint64_t copyLen        = __min(wholeSrcStrLen, maxLen);
        switch (dstType) {
          case tlStringUTF8: {
            wcstombs(( char* )dstData, ( const wchar_t* )srcData, copyLen);
          } break;
          case tlStringUTF16: {
            memcpy(( wchar_t* )dstData, ( const wchar_t* )srcData, sizeof(wchar_t) * copyLen);
          } break;
          default: assert(0 && "This string type isn't supported to be converted");
        }
      } break;
      default: assert(0 && "This string type isn't supported to be converted");
    }
  }
  static bool logCheckFormatLetter(stringReadArgument_tapi(format), uint32_t letterIndx, char letter) {
    switch (formatType) {
      case tlStringUTF8: {
        const char* format = ( const char* )formatData;
        return format[letterIndx] == letter;
      } break;
      case tlStringUTF16: {
        wchar_t wchar = (( const wchar_t* )formatData)[letterIndx];
        return wchar == letter;
      } break;
      case tlStringUTF32: {
        char32_t uchar = (( const char32_t* )formatData)[letterIndx];
        return uchar == letter;
      } break;
      default: assert(0 && "Unsupported format for logCheckFormatLetter!");
    }
    return false;
  }
  // All f32 variadic arguments are converted to f64 by compiler anyway
  // So don't bother with supporting %llf or something like that
  static void vCreateString(string_type_tapi targetType, void** target, const wchar_t* format,
                     va_list args) {
    // Read format
    uint64_t bufSize = 0;
    {
      va_list rArgs;
      va_copy(rArgs, args);

      uint64_t formatIter = 0;
      while (format[formatIter] != L'\0') {
        if (format[formatIter] == L'%' && format[formatIter + 1] != L'\0') {
          formatIter++;
          if (format[formatIter] == L'l' && format[formatIter + 1] != L'\0') {
            formatIter++;
            if (format[formatIter] == L'd') {
              long long int lld            = va_arg(rArgs, long long int);
              uint32_t      maxLetterCount = 0;
              tStrPrint(nullptr, 0, maxLetterCount, "%lld", lld);
              bufSize += maxLetterCount;
            } else if (format[formatIter] == L'u') {
              unsigned long long int llu            = va_arg(rArgs, unsigned long long int);
              uint32_t               maxLetterCount = 0;
              tStrPrint(nullptr, 0, maxLetterCount, "%llu", llu);
              bufSize += maxLetterCount;
            }
          } else if (format[formatIter] == L'd') {
            int      d              = va_arg(rArgs, int);
            uint32_t maxLetterCount = 0;
            tStrPrint(nullptr, 0, maxLetterCount, "%d", d);
            bufSize += maxLetterCount;
          } else if (format[formatIter] == L'u') {
            unsigned int u              = va_arg(rArgs, unsigned int);
            uint32_t     maxLetterCount = 0;
            tStrPrint(nullptr, 0, maxLetterCount, "%u", u);
            bufSize += maxLetterCount;
          } else if (format[formatIter] == L'f') {
            double   f              = va_arg(rArgs, double);
            uint32_t maxLetterCount = 0;
            tStrPrint(nullptr, 0, maxLetterCount, "%f", f);
            bufSize += maxLetterCount;
          } else if (format[formatIter] == L'p') {
            void*    p              = va_arg(rArgs, void*);
            uint32_t maxLetterCount = 0;
            tStrPrint(nullptr, 0, maxLetterCount, "%p", p);
            bufSize += maxLetterCount;
          } else if (format[formatIter] == L's') {
            const char* s = va_arg(rArgs, const char*);
            if (s) {
              bufSize += strlen(s);
            }
          } else if (format[formatIter] == L'v') {
            const wchar_t* vs = va_arg(rArgs, const wchar_t*);
            if (vs) {
              bufSize += wcslen(vs);
            }
          } else {
            assert(0 && "Unsupported type of log argument!");
          }
        } else {
          bufSize++;
        }

        formatIter++;
      }

      va_end(rArgs);
    }

    // Allocate string buffer
    uint64_t charSize = 0;
    {
      switch (targetType) {
        case tlStringUTF8: charSize = sizeof(char); break;
        case tlStringUTF16: charSize = sizeof(wchar_t); break;
        case tlStringUTF32: charSize = sizeof(char32_t); break;
      }
      *target = malloc((bufSize + 1) * charSize);
    }

    // Fill string buffer
    {
      uint64_t bufIter = 0, formatIter = 0;
      while (format[formatIter] != L'\0') {
        if (format[formatIter] == L'%' && format[formatIter + 1] != L'\0') {
          formatIter++;
          if (format[formatIter] == L'l' && format[formatIter + 1] != L'\0') {
            formatIter++;
            if (format[formatIter] == L'd') {
              long long int lld            = va_arg(args, long long int);
              uint32_t      maxLetterCount = 32;
              tStrPrint(*target, bufIter, maxLetterCount, "%lld", lld);
              bufIter += 8;
            } else if (format[formatIter] == L'u') {
              unsigned long long int u              = va_arg(args, unsigned long long int);
              uint32_t               maxLetterCount = 32;
              tStrPrint(*target, bufIter, maxLetterCount, "%llu", u);
              bufIter += maxLetterCount;
            }
          } else if (format[formatIter] == L'd') {
            int      i              = va_arg(args, int);
            uint32_t maxLetterCount = 32;
            tStrPrint(*target, bufIter, maxLetterCount, "%d", i);
            bufIter += maxLetterCount;
          } else if (format[formatIter] == L'u') {
            unsigned int u              = va_arg(args, unsigned int);
            uint32_t     maxLetterCount = 32;
            tStrPrint(*target, bufIter, maxLetterCount, "%u", u);
            bufIter += maxLetterCount;
          } else if (format[formatIter] == L'f') {
            double   d              = va_arg(args, double);
            uint32_t maxLetterCount = 32;
            tStrPrint(*target, bufIter, maxLetterCount, "%f", d);
            bufIter += maxLetterCount;
          } else if (format[formatIter] == L'p') {
            void*    p              = va_arg(args, void*);
            uint32_t maxLetterCount = 32;
            tStrPrint(*target, bufIter, maxLetterCount, "%p", p);
            bufIter += maxLetterCount;
          } else if (format[formatIter] == L's') {
            const char* s = va_arg(args, const char*);
            if (s) {
              uint64_t strLength = strlen(s);
              if (strLength) {
                switch (targetType) {
                  case tlStringUTF8: memcpy((( char* )*target) + bufIter, s, strLength); break;
                  case tlStringUTF16:
                    mbstowcs((( wchar_t* )*target) + bufIter, s, strLength);
                    break;
                  default: assert(0 && "NOT SUPPORTED FORMAT TO CREATE STRING");
                }
                bufIter += strLength;
              }
            }
          } else if (format[formatIter] == L'v') {
            const wchar_t* vs = va_arg(args, const wchar_t*);
            if (vs) {
              uint64_t strLength = wcslen(vs);
              if (strLength) {
                switch (targetType) {
                  case tlStringUTF8: wcstombs((( char* )*target) + bufIter, vs, strLength); break;
                  case tlStringUTF16:
                    memcpy((( wchar_t* )*target) + bufIter, vs, sizeof(wchar_t) * strLength);
                    break;
                  default: assert(0 && "NOT SUPPORTED FORMAT TO CREATE STRING");
                }
                bufIter += strLength;
              }
            }
          } else {
            assert(0 && "Unsupported type of log argument!");
          }
        } else {
          switch (targetType) {
            case tlStringUTF8:
              wcstombs((( char* )*target) + bufIter, &format[formatIter], 1);
              break;
            case tlStringUTF16: (( wchar_t* )*target)[bufIter] = format[formatIter]; break;
            default: assert(0 && "NOT SUPPORTED FORMAT TO CREATE STRING");
          }
          bufIter++;
        }

        formatIter++;
      }
      switch (targetType) {
        case tlStringUTF8: (( char* )*target)[bufIter] = '\0'; break;
        case tlStringUTF16: (( wchar_t* )*target)[bufIter] = L'\0'; break;
      }
    }
  }
  static void createString(string_type_tapi targetType, void** target, const wchar_t* format, ...) {
    va_list args;
    va_start(args, format);
    vCreateString(targetType, target, format, args);
    va_end(args);
  }
};

////////////////////////////////////////////////////////////////

// Unit Tests
unsigned char ut_0(const char** output_string, void* data) {
  *output_string = "Hello from ut_0!";
  return 255;
}

void set_unittests(tlECS* ecsSYS) {
  UNITTEST_TAPI_PLUGIN_LOAD_TYPE ut_sys =
    ( UNITTEST_TAPI_PLUGIN_LOAD_TYPE )ecsSYS->getSystem(UNITTEST_TAPI_PLUGIN_NAME);
  // If unit test system is available, add unit tests to it
  if (ut_sys) {
    /*unittest_interface_tapi x;
    x.test = &ut_0;
    ut_sys->funcs->add_unittest("array_of_strings", 0, x);*/
  } else {
  }
}

ECSPLUGIN_ENTRY(ecsSys, reloadFlag) {
  virmem_sys =
    (( ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE )ecsSys->getSystem(ALLOCATOR_TAPI_PLUGIN_NAME))
      ->virtualMemory;
  pagesize   = virmem_sys->pageSize();
  set_unittests(ecsSys);

  stringSys                     = ( tlString* )malloc(sizeof(tlString));
  stringSys->d               = nullptr;


  stringSys->convertString = tlStringPriv::convertString;
  stringSys->createString  = tlStringPriv::createString;
  stringSys->vCreateString = tlStringPriv::vCreateString;

  ecsSys->addSystem(STRINGSYS_TAPI_PLUGIN_NAME, STRINGSYS_TAPI_PLUGIN_VERSION, stringSys);
}

ECSPLUGIN_EXIT(ecsSys, reloadFlag) {
  free(stringSys);
}
