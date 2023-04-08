#include "logger_tapi.h"

#include <assert.h>
#include <stdarg.h>
#include <wchar.h>

#include <codecvt>
#include <iostream>
#include <string>
#include <vector>

#include "ecs_tapi.h"
#include "filesys_tapi.h"
#include "string_tapi.h"
#include "virtualmemorysys_tapi.h"

struct tapi_log {
  std::wstring  logText;
  tapi_log_type logType;
};
struct Logger {
  std::wstring          mainFilePath;
  std::vector<tapi_log> logList;
};
static Logger*       logSys  = nullptr;
static filesys_tapi* fileSys = nullptr;

#define GET_printer(i) (*GET_LOGLISTTAPI())[i]
inline void breakpoint() {
  printf("This log is meant to stop the app: y to continue, n to exit application\n");
  char breakpoint_choice = 0;
  while (breakpoint_choice != 'y' || breakpoint_choice != 'n') {
    std::cin >> &breakpoint_choice;
  }
  if (breakpoint_choice == 'n') {
    exit(-1);
  }
}

void logSave_tapi(tapi_log_type logType, stringArgument_tapi(path)) {
  if (logSys->logList.size() == 0) {
    return;
  }
  std::wstring textData;
  for (unsigned int i = 0; i < logSys->logList.size(); i++) {
    const tapi_log& log = logSys->logList[i];
    // If logType is INT32_MAX, this is the main log file save
    if (logType == INT32_MAX) {
      textData += log.logText;
    } else if (logType == log.logType) {
      textData += log.logText;
    }
  }
  fileSys->write_textfile(string_type_tapi_UTF16, textData.c_str(), pathType, pathData, false);
  logSys->logList.clear();
}

static constexpr uint32_t maxCharPerLog_tapi = 1 << 12;
static constexpr wchar_t* statusNames[]      = {L"Status", L"Warning", L"Error", L"Not coded",
                                                L"Crashing"};
bool logCheckFormatLetter(stringArgument_tapi(format), uint32_t letterIndx, char letter) {
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
void logLog_tapi(tapi_log_type type, unsigned char stopRunning, stringArgument_tapi(format), ...) {
  wchar_t buf[maxCharPerLog_tapi] = {};

  va_list args;
  va_start(args, formatData);

  int      bufIter   = 0;
  uint32_t formatLen = 0, sizeOfEachChar = 0;
  switch (formatType) {
    case string_type_tapi_UTF8:
      formatLen      = strlen(( const char* )formatData);
      sizeOfEachChar = sizeof(char);
      break;
    case string_type_tapi_UTF16:
      formatLen      = wcslen(( const wchar_t* )formatData);
      sizeOfEachChar = sizeof(wchar_t);
      break;
    default: assert(0 && "Only UTF8 and UTF16 supported for now!");
  }
  for (uint32_t formatIter = 0; formatIter < formatLen && formatIter < maxCharPerLog_tapi;
       formatIter++) {
    if (logCheckFormatLetter(formatType, formatData, formatIter, '%') &&
        formatIter + 1 <= formatLen) {
      formatIter++;
      if (logCheckFormatLetter(formatType, formatData, formatIter, 'd')) {
        int i = va_arg(args, int);
        _snwprintf(&buf[bufIter], 4, L"%d", i);
        bufIter += 4;
      } else if (logCheckFormatLetter(formatType, formatData, formatIter, 'f')) {
        double d = va_arg(args, double);
        _snwprintf(&buf[bufIter], 8, L"%f", d);
        bufIter += 8;
      } else if (logCheckFormatLetter(formatType, formatData, formatIter, 's')) {
        const char* s      = va_arg(args, const char*);
        if (s) {
          int strLen = strlen(s);
          if (strLen <= maxCharPerLog_tapi - 1 - bufIter) {
            mbstowcs(&buf[bufIter], s, strLen);
            bufIter += strLen;
          } else {
            assert(0 && "maxCharPerLog_tapi is exceeded!");
          }
        }
      } else if (logCheckFormatLetter(formatType, formatData, formatIter, 'v')) {
        const wchar_t* vs     = va_arg(args, const wchar_t*);
        if (vs) {
          int strLen = wcslen(vs);
          if (strLen <= maxCharPerLog_tapi - 1 - bufIter) {
            memcpy(&buf[bufIter], vs, sizeof(wchar_t) * strLen);
            bufIter += strLen;
          } else {
            assert(0 && "maxCharPerLog_tapi is exceeded!");
          }
        }
      } else {
        assert(0 && "Unsupported type of log argument!");
      }
    } else {
      memcpy(buf + (sizeOfEachChar * bufIter),
             (( char* )formatData) + (sizeOfEachChar * formatIter), sizeOfEachChar);
    }
  }

  va_end(args);

  logSys->logList.push_back(tapi_log());
  tapi_log& log = logSys->logList[logSys->logList.size() - 1];
  log.logText   = buf;
  log.logType   = type;
  wprintf_s(L"%s: %s", statusNames[type], buf);
  logSave_tapi(( tapi_log_type )INT32_MAX, string_type_tapi_UTF16, logSys->mainFilePath.c_str());

  if (stopRunning) {
    breakpoint();
  }
}

void logInit_tapi(stringArgument_tapi(mainLogFile)) {
  switch (mainLogFileType) {
    case string_type_tapi_UTF8: {
      typedef std::codecvt_utf8<wchar_t>          convert_type;
      std::wstring_convert<convert_type, wchar_t> converter;
      logSys->mainFilePath = converter.from_bytes(( const char* )mainLogFileData);
    } break;
    case string_type_tapi_UTF16: {
      logSys->mainFilePath = ( const wchar_t* )mainLogFileData;
    } break;
  }
  logSys->logList.clear();
}
void logDestroy_tapi() {
  logSys->mainFilePath = {};
  logSys->logList.clear();
}

typedef struct logger_tapi_d {
  logger_tapi_type* type;
  Logger*           log_sys;
} logger_tapi_d;
ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  VIRTUALMEMORY_TAPI_PLUGIN_TYPE virmemsys_type =
    ( VIRTUALMEMORY_TAPI_PLUGIN_TYPE )ecssys->getSystem(VIRTUALMEMORY_TAPI_PLUGIN_NAME);
  ARRAY_OF_STRINGS_TAPI_LOAD_TYPE array_of_strings_sys_type =
    ( ARRAY_OF_STRINGS_TAPI_LOAD_TYPE )ecssys->getSystem(ARRAY_OF_STRINGS_TAPI_PLUGIN_NAME);
  FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys_type =
    ( FILESYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(FILESYS_TAPI_PLUGIN_NAME);

  if (!virmemsys_type || !array_of_strings_sys_type || !filesys_type) {
    printf(
      "Logger system needs virtual memory, array of strings and file systems loaded. So loading "
      "failed!");
    return;
  }

  LOGGER_TAPI_PLUGIN_LOAD_TYPE type =
    ( LOGGER_TAPI_PLUGIN_LOAD_TYPE )malloc(sizeof(logger_tapi_type));
  type->data       = ( logger_tapi_d* )malloc(sizeof(logger_tapi_d));
  type->funcs      = ( logger_tapi* )malloc(sizeof(logger_tapi));
  type->data->type = type;

  type->funcs->init    = &logInit_tapi;
  type->funcs->destroy = &logDestroy_tapi;
  type->funcs->log     = &logLog_tapi;
  type->funcs->save    = &logSave_tapi;

  ecssys->addSystem(LOGGER_TAPI_PLUGIN_NAME, LOGGER_TAPI_PLUGIN_VERSION, type);

  logSys              = new Logger;
  type->data->log_sys = logSys;
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) { printf("Not coded!"); }