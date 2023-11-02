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


struct tlLogObject {
  std::wstring  logText;
  tlLogType logType;
};
const tlString*      stringSys = nullptr;
const tlIO* fileSys = nullptr;
struct tlLogPriv {
  tlLog*                   sys;
  std::wstring             mainFilePath;
  std::vector<tlLogObject> logList;
};
tlLogPriv* logSys;

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

void tlLogSave(tlLogType logType, stringReadArgument_tapi(path)) {
  if (logSys->logList.size() == 0 || !fileSys) {
    return;
  }
  std::wstring textData;
  for (unsigned int i = 0; i < logSys->logList.size(); i++) {
    const tlLogObject& log = logSys->logList[i];
    // If logType is INT32_MAX, this is the main log file save
    if (logType == INT32_MAX) {
      textData += log.logText;
    } else if (logType == log.logType) {
      textData += log.logText;
    }
  }
  fileSys->writeText(tlStringUTF16, textData.c_str(), pathType, pathData, false);
  logSys->logList.clear();
}

static constexpr uint32_t maxCharPerLog_tapi = 1 << 12;
static constexpr wchar_t* statusNames[]      = {L"Status", L"Warning", L"Error", L"Not coded",
                                                L"Crashing"};
static constexpr int      consoleColors[]    = {2, 6, 12, 14, 64};

HANDLE hConsole = nullptr;
void   tlLogLog(tlLogType type, unsigned char stopRunning, const wchar_t* format, ...) {
  va_list args;
  va_start(args, format);

  wchar_t* buf = nullptr;
  stringSys->vCreateString(tlStringUTF16, ( void** )&buf, format, args);
  va_end(args);

  if (!buf) {
    printf("String conversion has failed, please check your log format\n");
    return;
  }

  logSys->logList.push_back(tlLogObject());
  tlLogObject& log = logSys->logList[logSys->logList.size() - 1];
  log.logText    = buf;
  log.logType    = type;
  SetConsoleTextAttribute(hConsole, consoleColors[type]);
  wprintf_s(L"%s: %s\n", statusNames[type], buf);
  SetConsoleTextAttribute(hConsole, 7);
  tlLogSave(( tlLogType )INT32_MAX, tlStringUTF16, logSys->mainFilePath.c_str());

  if (stopRunning) {
    breakpoint();
  }
}

void tlLogInit(stringReadArgument_tapi(mainLogFile)) {
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  switch (mainLogFileType) {
    case tlStringUTF8: {
      typedef std::codecvt_utf8<wchar_t>          convert_type;
      std::wstring_convert<convert_type, wchar_t> converter;
      logSys->mainFilePath = converter.from_bytes(( const char* )mainLogFileData);
    } break;
    case tlStringUTF16: {
      logSys->mainFilePath = ( const wchar_t* )mainLogFileData;
    } break;
  }
  logSys->logList.clear();
}
void tlLogDestroy() {
  logSys->mainFilePath = {};
  logSys->logList.clear();
}

ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  STRINGSYS_TAPI_PLUGIN_LOAD_TYPE stringSysType =
    ( STRINGSYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(STRINGSYS_TAPI_PLUGIN_NAME);
  FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys_type =
    ( FILESYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(FILESYS_TAPI_PLUGIN_NAME);

  if (!stringSysType || !filesys_type) {
    printf("Log system needs %s and %s loaded. Loading logger has failed",
           STRINGSYS_TAPI_PLUGIN_NAME, FILESYS_TAPI_PLUGIN_NAME);
    return;
  }

  tlLog* type =
    ( tlLog* )malloc(sizeof(tlLog));
  {
    logSys = new tlLogPriv;
    logSys->sys = type;
    type->d     = logSys;


    type->init    = &tlLogInit;
    type->destroy = &tlLogDestroy;
    type->log     = &tlLogLog;
    type->save    = &tlLogSave;
  }

  stringSys = stringSysType;

  ecssys->addSystem(LOGGER_TAPI_PLUGIN_NAME, LOGGER_TAPI_PLUGIN_VERSION, type);
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) { printf("Not coded!"); }