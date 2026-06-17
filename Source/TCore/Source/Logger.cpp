#define TCORE_INCLUDE_PLATFORM_LIBS
#include "Logger.h"

#include <assert.h>
#include <stdarg.h>
#include <wchar.h>

#include <codecvt>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "ECS.h"
#include "FileSystem.h"
#include "String.h"

TCORE_PLUGIN_INIT(TSLog, TCLog)

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TSLog)
TCORE_PLUGIN_ENTRY_POINT_END()

struct TCLogRecord {
  std::string Text;
  TCLogType   Type;
};

struct TCLogContext {
  std::filesystem::path    mainFilePath;
  std::vector<TCLogRecord> logList;
};

struct TCLogServices {
  static void Initialize(const char* logFilePath) {}
  static void Destroy() {}
  static void Save(enum TCLogType logType, const char* filePath) {}
  static void Log(enum TCLogType type, unsigned char stopRunning, const wchar_t* format, ...) {}
};
TCLogContext* Context = nullptr;

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
  if (Context->logList.size() == 0 || !fileSys) {
    return;
  }
  std::wstring textData;
  for (unsigned int i = 0; i < Context->logList.size(); i++) {
    const TCLogRecord& log = Context->logList[i];
    // If logType is INT32_MAX, this is the main log file save
    if (logType == INT32_MAX) {
      textData += log.logText;
    } else if (logType == log.logType) {
      textData += log.logText;
    }
  }
  fileSys->writeText(tlStringUTF16, textData.c_str(), pathType, pathData, false);
  Context->logList.clear();
}

static constexpr uint32_t maxCharPerLog_tapi = 1 << 12;
static constexpr wchar_t* statusNames[]      = {L"Status", L"Warning", L"Error", L"Not coded",
                                                L"Crashing"};
static constexpr int      consoleColors[]    = {2, 6, 12, 14, 64};

#if defined(_WIN32)
HANDLE hConsole = nullptr;
#endif
void tlLogLog(tlLogType type, unsigned char stopRunning, const wchar_t* format, ...) {
  va_list args;
  va_start(args, format);

  wchar_t* buf = nullptr;
  stringSys->vCreateString(tlStringUTF16, ( void** )&buf, format, args);
  va_end(args);

  if (!buf) {
    printf("String conversion has failed, please check your log format\n");
    return;
  }

  Context->logList.push_back(TCLogRecord());
  TCLogRecord& log = Context->logList[Context->logList.size() - 1];
  log.logText      = buf;
  log.logType      = type;
#if defined(_WIN32)
  SetConsoleTextAttribute(hConsole, consoleColors[type]);
  wprintf(L"%ls: %ls\n", statusNames[type], buf);
  SetConsoleTextAttribute(hConsole, 7);
#else
  wprintf(L"%ls: %ls\n", statusNames[type], buf);
#endif
  tlLogSave(( tlLogType )INT32_MAX, tlStringUTF16, Context->mainFilePath.c_str());

  if (stopRunning) {
    breakpoint();
  }
}

void tlLogInit(stringReadArgument_tapi(mainLogFile)) {
#if defined(_WIN32)
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
  switch (mainLogFileType) {
    case tlStringUTF8: {
      typedef std::codecvt_utf8<wchar_t>          convert_type;
      std::wstring_convert<convert_type, wchar_t> converter;
      Context->mainFilePath = converter.from_bytes(( const char* )mainLogFileData);
    } break;
    case tlStringUTF16: {
      Context->mainFilePath = ( const wchar_t* )mainLogFileData;
    } break;
  }
  Context->logList.clear();
}
void tlLogDestroy() {
  Context->mainFilePath = {};
  Context->logList.clear();
}

TCResult TSLog_Initialize(const void** outPluginAPI) {
  if (!outPluginAPI) {
    return TC_RESULT_FAILURE;
  }
  Context              = new TCLogContext();
  auto services        = new TCLog;
  services->Initialize = tlLogInit;
  services->Destroy    = tlLogDestroy;
  services->Save       = tlLogSave;
  services->Log        = tlLogLog;
  TSLog                = services;
  *outPluginAPI        = TSLog;
  return TC_RESULT_SUCCESS;
}

TCResult TSLog_OnPreShutdown() { return TC_RESULT_SUCCESS; }

TCResult TSLog_Shutdown() {
  if (Context) {
    delete Context;
    Context = nullptr;
  }
  return TC_RESULT_SUCCESS;
}
void TSLog_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded) {
  // This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}