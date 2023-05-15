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
#include "virtualmemorysys_tapi.h"

// FIX: This should be specified by the flag!
#define tapi_average_string_len 100

// TO-DO: Add debug build checks

virtualmemorysys_tapi*                     virmem_sys;
unsigned int                               pagesize;
stringSys_tapi_type*                       strSysType;
stringSys_tapi_d*                          aos_sys_d;
array_of_strings_standardallocator_tapi*   aos_standard;
array_of_strings_virtualallocatorsys_tapi* aos_virtual;
stringSys_tapi*                            stringSys;

typedef struct stringSys_tapi_d {
  stringSys_tapi_type* aos_sys;
} stringSys_tapi_d;

typedef struct standard_array_of_strings {
  char*               list_of_chars;
  unsigned long long* list_of_ptrs;
  unsigned long long  ptr_list_capacity, ptr_list_len,
    char_list_len; // char list is always filled and searched, so there is no need for capacity
} standard_array_of_strings;

array_of_strings_tapi* standard_create_array(unsigned long long initial_size,
                                             unsigned long long char_expand_size) {
  standard_array_of_strings* array = new standard_array_of_strings;
  array->ptr_list_len              = 0;
  array->ptr_list_capacity         = initial_size;
  if (array->ptr_list_capacity) {
    array->list_of_ptrs =
      ( unsigned long long* )malloc(array->ptr_list_capacity * sizeof(array->list_of_ptrs[0]));
  }
  array->list_of_chars = nullptr;

  return ( array_of_strings_tapi* )array;
}
const char* standard_read_string(const array_of_strings_tapi* array_o,
                                 unsigned long long           element_index) {
  const standard_array_of_strings* array = ( const standard_array_of_strings* )array_o;
  return &array->list_of_chars[array->list_of_ptrs[element_index]];
}

unsigned long long find_a_place_for_string(char* list_of_chars, unsigned long char_list_len,
                                           unsigned long len_str) {
  unsigned long long current_pointer = 0, found_size = 0;
  while (current_pointer < char_list_len) {
    if (list_of_chars[current_pointer] == -1) {
      found_size++;
      if (found_size == len_str) {
        return current_pointer - found_size;
      }
    } else {
      current_pointer += strlen(&list_of_chars[current_pointer]);
    }
  }
  return ULONG_MAX;
}
unsigned long long standard_expand_char_array(standard_array_of_strings* array) {
  char* newlist = ( char* )malloc(array->char_list_len * 2);
  memcpy(newlist, array->list_of_chars, array->char_list_len);
  free(array->list_of_chars);
  array->char_list_len *= 2;
  array->list_of_chars = newlist;
  return array->char_list_len / 2;
}
void standard_push_back(array_of_strings_tapi* array_o, const char* string) {
  standard_array_of_strings* array   = ( standard_array_of_strings* )array_o;
  unsigned long long         len_str = strlen(string) + 1;

  // Expand list of pointers if full
  if (array->ptr_list_len == array->ptr_list_capacity) {
    array->ptr_list_capacity *= 2;
    unsigned long long* new_list_of_ptrs =
      ( unsigned long long* )malloc(sizeof(array->list_of_ptrs[0]) * array->ptr_list_capacity);
    memcpy(new_list_of_ptrs, array->list_of_ptrs,
           array->ptr_list_len * sizeof(array->list_of_ptrs[0]));
    free(array->list_of_ptrs);
    array->list_of_ptrs = new_list_of_ptrs;
  }

  unsigned long found_place =
    find_a_place_for_string(array->list_of_chars, array->char_list_len, len_str);
  if (found_place == ULONG_MAX) {
    found_place = standard_expand_char_array(array);
  }
  memcpy(&array->list_of_chars[found_place], string, len_str);
  array->list_of_ptrs[array->ptr_list_len++] = found_place;
}
void standard_delete_string(array_of_strings_tapi* array_o, unsigned long long element_index) {
  standard_array_of_strings* array = ( standard_array_of_strings* )array_o;
  memset(&array->list_of_chars[array->list_of_ptrs[element_index]], -1,
         strlen(&array->list_of_chars[array->list_of_ptrs[element_index]]));
  for (unsigned long i = element_index + 1; i < array->ptr_list_capacity; i++) {
    array->list_of_ptrs[i - 1] = array->list_of_ptrs[i];
  }
  array->ptr_list_len--;
}
void standard_change_string(array_of_strings_tapi* array_o, unsigned long long element_index,
                            const char* string) {
  standard_array_of_strings* array   = ( standard_array_of_strings* )array_o;
  unsigned long              len_str = strlen(string) + 1;
  unsigned long              previous_str_len =
    strlen(&array->list_of_chars[array->list_of_ptrs[element_index]]) + 1;
  if (len_str <= previous_str_len) {
    memcpy(&array->list_of_chars[array->list_of_ptrs[element_index]], string, len_str);
    memset(&array->list_of_chars[array->list_of_ptrs[element_index] + len_str], -1,
           previous_str_len - len_str);
    return;
  }

  unsigned long found_place =
    find_a_place_for_string(array->list_of_chars, array->char_list_len, len_str);
  if (found_place == ULONG_MAX) {
    found_place = standard_expand_char_array(array);
  }
  memcpy(&array->list_of_chars[found_place], string, len_str);
  memset(&array->list_of_chars[array->list_of_ptrs[element_index]], -1, previous_str_len);
  array->list_of_ptrs[element_index] = found_place;
}

typedef struct virtual_array_of_strings {
  char*          list_of_chars;
  unsigned long* list_of_ptrs;
  unsigned int   all_memoryblock_len, committed_charpagecount, committed_ptrpagecount;
} virtual_array_of_strings;

array_of_strings_tapi* virtual_create_array(void* virtualmem_loc, unsigned long long memsize,
                                            unsigned long long allocator_flag) {
  // Commit to place object at the beginning
  virmem_sys->virtual_commit(virtualmem_loc, pagesize);
  // Place base object at the beginning
  virtual_array_of_strings* arry = ( virtual_array_of_strings* )virtualmem_loc;
  arry->all_memoryblock_len      = memsize;

  // Place ptr list at the end of base object
  arry->list_of_ptrs = ( unsigned long* )(( char* )arry + sizeof(virtual_array_of_strings));
  // Place char list far away from the ptr list's start
  // Placement is like this: page0[baseobj - ptrlist_beginning], page1-x[ptrlist_continue],
  // pagex-end[charlist] [x,end] range is "(tapi_average_string_len / sizeof(unsigned int))-1" times
  // bigger than [1-x] range
  arry->list_of_chars = ( char* )arry + (pagesize * (memsize / pagesize) /
                                         (tapi_average_string_len / sizeof(unsigned int)));
  virmem_sys->virtual_commit(( char* )arry + pagesize, pagesize);
  arry->committed_ptrpagecount = 1;

  // Commit to make array usable
  virmem_sys->virtual_commit(arry->list_of_chars, pagesize);
  arry->committed_charpagecount = 1;
  unsigned int i                = 0;
  while (i < pagesize) {
    arry->list_of_chars[i] = -1;
    i++;
  }

  return ( array_of_strings_tapi* )arry;
}
const char* virtual_read_string(const array_of_strings_tapi* array_o,
                                unsigned long long           element_index) {
  virtual_array_of_strings* array = ( virtual_array_of_strings* )array_o;
  if (element_index <
      ((array->committed_ptrpagecount * pagesize) + (pagesize - sizeof(virtual_array_of_strings))) /
        4) {
    return &array->list_of_chars[array->list_of_ptrs[element_index]];
  }
  return NULL;
}
unsigned long long virtual_change_string(array_of_strings_tapi* array_o,
                                         unsigned long long element_index, const char* string) {
  virtual_array_of_strings* array  = ( virtual_array_of_strings* )array_o;
  unsigned int              length = strlen(string) + 1;
  unsigned int              i      = 0;
  // Check if its time to expand ptr list with a commit
  if (element_index % (pagesize / sizeof(unsigned int)) == pagesize / sizeof(unsigned int) - 1) {
    virmem_sys->virtual_commit(( char* )array + (pagesize * array->committed_ptrpagecount),
                               array->committed_ptrpagecount * pagesize);
    array->committed_ptrpagecount *= 2;
  }
  while (i < (pagesize * array->committed_charpagecount)) {
    unsigned int current_length = 0;
    while (array->list_of_chars[i] == -1) {
      i++;
      current_length++;
    }
    if (current_length >= length) {
      memcpy(&array->list_of_chars[i - current_length], string, length);
      array->list_of_ptrs[element_index] = i - current_length;
      return ULONG_MAX;
    }
    i++;
  }
  virmem_sys->virtual_commit(array->list_of_chars + i, array->committed_charpagecount * pagesize);
  array->committed_charpagecount *= 2;
  memcpy(&array->list_of_chars[i], string, length);
  array->list_of_ptrs[element_index] = i;
  return ULONG_MAX;
}
// returned: status of the operation depending on the given allocator_flag while creating the array
unsigned long long virtual_delete_string(array_of_strings_tapi* array_o,
                                         unsigned long long     element_index) {
  virtual_array_of_strings* array = ( virtual_array_of_strings* )array_o;
  if ((element_index < ((array->committed_ptrpagecount * pagesize) +
                        (pagesize - sizeof(virtual_array_of_strings))) /
                         4)) {
    char* stringptr;
    stringptr = &(array->list_of_chars[array->list_of_ptrs[element_index]]);
    for (unsigned int i = 0; i < strlen(stringptr) + 1; i++) {
      stringptr[i] = -1;
    }
    array->list_of_ptrs[element_index] = UINT_MAX;
    return ULONG_MAX;
  }
}

/////////////////////////////////////////// STRING SYSTEM

void string_convertString(stringReadArgument_tapi(src), stringWriteArgument_tapi(dst), 
                          uint64_t maxLen) {
  switch (srcType) {
    case string_type_tapi_UTF8: {
      uint64_t wholeSrcStrLen = strlen(( const char* )srcData) + 1;
      uint64_t copyLen        = __min(wholeSrcStrLen, maxLen);
      switch (dstType) {
        case string_type_tapi_UTF8: {
          memcpy(dstData, srcData, copyLen);
        } break;
        case string_type_tapi_UTF16: {
          mbstowcs(( wchar_t* )dstData, ( const char* )srcData, wholeSrcStrLen);
        } break;
        default: assert(0 && "This string type isn't supported to be converted");
      }
    } break;
    case string_type_tapi_UTF16: {
      uint64_t wholeSrcStrLen = wcslen(( const wchar_t* )srcData) + 1;
      uint64_t copyLen        = __min(wholeSrcStrLen, maxLen);
      switch (dstType) {
        case string_type_tapi_UTF8: {
          wcstombs(( char* )dstData, ( const wchar_t* )srcData, copyLen);
        } break;
        case string_type_tapi_UTF16: {
          memcpy(( wchar_t* )dstData, ( const wchar_t* )srcData, sizeof(wchar_t) * copyLen);
        } break;
        default: assert(0 && "This string type isn't supported to be converted");
      }
    } break;
    default: assert(0 && "This string type isn't supported to be converted");
  }
}
bool logCheckFormatLetter(stringReadArgument_tapi(format), uint32_t letterIndx, char letter) {
  switch (formatType) {
    case string_type_tapi_UTF8: {
      const char* format = ( const char* )formatData;
      return format[letterIndx] == letter;
    } break;
    case string_type_tapi_UTF16: {
      wchar_t wchar = (( const wchar_t* )formatData)[letterIndx];
      return wchar == letter;
    } break;
    case string_type_tapi_UTF32: {
      char32_t uchar = (( const char32_t* )formatData)[letterIndx];
      return uchar == letter;
    } break;
    default: assert(0 && "Unsupported format for logCheckFormatLetter!");
  }
  return false;
}
#define tStrPrint(buf, bufIter, count, format, p)                   \
  if (targetType == string_type_tapi_UTF8) {                        \
    count = snprintf((( char* )##buf) + bufIter, count, format, p); \
  } else if (targetType == string_type_tapi_UTF16) {                \
    count = _snwprintf((( wchar_t* )##buf) + bufIter, count, L##format, p); \
  }
// All f32 variadic arguments are converted to f64 by compiler anyway
// So don't bother with supporting %llf or something like that
void tapi_vCreateString(string_type_tapi targetType, void** target, const wchar_t* format,
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
            long long int lld = va_arg(rArgs, long long int);
            uint32_t      maxLetterCount = 0;
            tStrPrint(nullptr, 0, maxLetterCount, "%lld", lld);
            bufSize += maxLetterCount;
          } else if (format[formatIter] == L'u') {
            unsigned long long int llu              = va_arg(rArgs, unsigned long long int);
            uint32_t               maxLetterCount = 0;
            tStrPrint(nullptr, 0, maxLetterCount, "%llu", llu);
            bufSize += maxLetterCount;
          }
        }
        else if (format[formatIter] == L'd') {
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
          double    f              = va_arg(rArgs, double);
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
      case string_type_tapi_UTF8: charSize = sizeof(char); break;
      case string_type_tapi_UTF16: charSize = sizeof(wchar_t); break;
      case string_type_tapi_UTF32: charSize = sizeof(char32_t); break;
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
            long long int lld              = va_arg(args, long long int);
            uint32_t      maxLetterCount = 32;
            tStrPrint(*target, bufIter, maxLetterCount, "%lld", lld);
            bufIter += 8;
          } else if (format[formatIter] == L'u') {
            unsigned long long int u              = va_arg(args, unsigned long long int);
            uint32_t               maxLetterCount = 32;
            tStrPrint(*target, bufIter, maxLetterCount, "%llu", u);
            bufIter += maxLetterCount;
          }
        }
        else if (format[formatIter] == L'd') {
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
                case string_type_tapi_UTF8:
                  memcpy((( char* )*target) + bufIter, s, strLength);
                  break;
                case string_type_tapi_UTF16:
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
                case string_type_tapi_UTF8:
                  wcstombs((( char* )*target) + bufIter, vs, strLength);
                  break;
                case string_type_tapi_UTF16:
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
          case string_type_tapi_UTF8:
            wcstombs((( char* )*target) + bufIter, &format[formatIter], 1);
            break;
          case string_type_tapi_UTF16: (( wchar_t* )*target)[bufIter] = format[formatIter]; break;
          default: assert(0 && "NOT SUPPORTED FORMAT TO CREATE STRING");
        }
        bufIter++;
      }

      formatIter++;
    }
    switch (targetType) {
      case string_type_tapi_UTF8: (( char* )*target)[bufIter] = '\0'; break;
      case string_type_tapi_UTF16: (( wchar_t* )*target)[bufIter] = L'\0'; break;
    }
  }
}
void string_createString(string_type_tapi targetType, void** target, const wchar_t* format, ...) {
  va_list args;
  va_start(args, format);
  tapi_vCreateString(targetType, target, format, args);
  va_end(args);
}

////////////////////////////////////////////////////////////////

// Unit Tests
unsigned char ut_0(const char** output_string, void* data) {
  *output_string = "Hello from ut_0!";
  return 255;
}

void set_unittests(ecs_tapi* ecsSYS) {
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
  virtualmemorysys_tapi_type* virmemtype =
    ( VIRTUALMEMORY_TAPI_PLUGIN_TYPE )ecsSys->getSystem(VIRTUALMEMORY_TAPI_PLUGIN_NAME);
  virmem_sys = virmemtype->funcs;
  pagesize   = virmem_sys->get_pagesize();
  set_unittests(ecsSys);

  strSysType                     = ( stringSys_tapi_type* )malloc(sizeof(stringSys_tapi_type));
  strSysType->standardString     = ( stringSys_tapi* )malloc(sizeof(stringSys_tapi));
  strSysType->standard_allocator = ( array_of_strings_standardallocator_tapi* )malloc(
    sizeof(array_of_strings_standardallocator_tapi));
  strSysType->virtual_allocator = ( array_of_strings_virtualallocatorsys_tapi* )malloc(
    sizeof(array_of_strings_virtualallocatorsys_tapi));
  strSysType->data = ( stringSys_tapi_d* )malloc(sizeof(stringSys_tapi_d));
  aos_sys_d        = strSysType->data;
  stringSys        = strSysType->standardString;
  aos_standard     = strSysType->standard_allocator;
  aos_virtual      = strSysType->virtual_allocator;

  stringSys->convertString = string_convertString;
  stringSys->createString  = string_createString;
  stringSys->vCreateString = tapi_vCreateString;

  aos_standard->change_string          = &standard_change_string;
  aos_standard->create_array_of_string = &standard_create_array;
  aos_standard->delete_string          = &standard_delete_string;
  aos_standard->push_back              = &standard_push_back;
  aos_standard->read_string            = &standard_read_string;

  aos_virtual->change_string          = &virtual_change_string;
  aos_virtual->create_array_of_string = &virtual_create_array;
  aos_virtual->delete_string          = &virtual_delete_string;
  aos_virtual->read_string            = &virtual_read_string;

  ecsSys->addSystem(STRINGSYS_TAPI_PLUGIN_NAME, STRINGSYS_TAPI_PLUGIN_VERSION, strSysType);
}

ECSPLUGIN_EXIT(ecsSys, reloadFlag) {
  free(aos_standard);
  free(aos_virtual);
  free(aos_sys_d);
  free(stringSys);
  free(strSysType);
}
