#include "logger_tapi.h"

#include <assert.h>
#include <stdarg.h>
#include <wchar.h>
#include <windows.h> // WinApi header

#include <codecvt>
#include <iostream>
#include <string>
#include <vector>

#include "ecs_tapi.h"
#include "filesys_tapi.h"
#include "string_tapi.h"
#include "virtualmemorysys_tapi.h"

tapi_stringSys* stringSys = nullptr;

struct logObject {
  std::wstring  logText;
  tapi_log_type logType;
};
struct Logger {
  std::wstring           mainFilePath;
  std::vector<logObject> logList;
};
static Logger*       logSys  = nullptr;
static tapi_fileSys* fileSys = nullptr;

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

void tapi_logSave(tapi_log_type logType, stringReadArgument_tapi(path)) {
  if (logSys->logList.size() == 0 || !fileSys) {
    return;
  }
  std::wstring textData;
  for (unsigned int i = 0; i < logSys->logList.size(); i++) {
    const logObject& log = logSys->logList[i];
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
static constexpr int      consoleColors[]    = {2, 6, 12, 14, 64};

HANDLE hConsole = nullptr;
void   tapi_logLog(tapi_log_type type, unsigned char stopRunning, const wchar_t* format, ...) {
  va_list args;
  va_start(args, format);

  wchar_t* buf = nullptr;
  stringSys->vCreateString(string_type_tapi_UTF16, ( void** )&buf, format, args);
  va_end(args);

  if (!buf) {
    printf("String conversion has failed, please check your log format\n");
    return;
  }

  logSys->logList.push_back(logObject());
  logObject& log = logSys->logList[logSys->logList.size() - 1];
  log.logText    = buf;
  log.logType    = type;
  SetConsoleTextAttribute(hConsole, consoleColors[type]);
  wprintf_s(L"%s: %s\n", statusNames[type], buf);
  SetConsoleTextAttribute(hConsole, 7);
  tapi_logSave(( tapi_log_type )INT32_MAX, string_type_tapi_UTF16, logSys->mainFilePath.c_str());

  if (stopRunning) {
    breakpoint();
  }
}

void tapi_logInit(stringReadArgument_tapi(mainLogFile)) {
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
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
void tapi_logDestroy() {
  logSys->mainFilePath = {};
  logSys->logList.clear();
}

typedef struct tapi_logger_d {
  tapi_logger_type* type;
  Logger*           log_sys;
} logger_tapi_d;
ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  VIRTUALMEMORY_TAPI_PLUGIN_LOAD_TYPE virmemsys_type =
    ( VIRTUALMEMORY_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(VIRTUALMEMORY_TAPI_PLUGIN_NAME);
  STRINGSYS_TAPI_PLUGIN_LOAD_TYPE stringSysType =
    ( STRINGSYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(STRINGSYS_TAPI_PLUGIN_NAME);
  FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys_type =
    ( FILESYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(FILESYS_TAPI_PLUGIN_NAME);

  if (!virmemsys_type || !stringSysType || !filesys_type) {
    printf("Logger system needs %s, %s and %s loaded. Loading logger has failed",
           VIRTUALMEMORY_TAPI_PLUGIN_NAME, STRINGSYS_TAPI_PLUGIN_NAME, FILESYS_TAPI_PLUGIN_NAME);
    return;
  }

  LOGGER_TAPI_PLUGIN_LOAD_TYPE type =
    ( LOGGER_TAPI_PLUGIN_LOAD_TYPE )malloc(sizeof(tapi_logger_type));
  {
    tapi_logger_d* d = ( tapi_logger_d* )malloc(sizeof(tapi_logger_d));
    tapi_logger*   f = ( tapi_logger* )malloc(sizeof(tapi_logger));

    d->type    = type;
    logSys     = new Logger;
    d->log_sys = logSys;

    f->init    = &tapi_logInit;
    f->destroy = &tapi_logDestroy;
    f->log     = &tapi_logLog;
    f->save    = &tapi_logSave;

    type->data  = d;
    type->funcs = f;
  }

  stringSys = stringSysType->standardString;

  ecssys->addSystem(LOGGER_TAPI_PLUGIN_NAME, LOGGER_TAPI_PLUGIN_VERSION, type);
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) { printf("Not coded!"); }