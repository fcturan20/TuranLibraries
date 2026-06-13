#pragma once
#ifdef __cplusplus
extern "C" {
#endif
  
#include "predefinitions_tapi.h"
#define LOGGER_TAPI_PLUGIN_NAME "tapi_logger"
#define LOGGER_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define LOGGER_TAPI_PLUGIN_LOAD_TYPE const struct tlLog*

/*
 * You should init the log system. This will make every log to be saved in the main log file
 * If you want to save specific type of logs to a specific text file, you can query it with save().
 * All logs are saved as UTF32.
 * You can use all UTFs by defining them with proper format argument: 8 -> %s, 16 -> %v, 32 -> %w
 * There is a limit of characters per log. It is defined with maxCharPerLog_tapi constant.
 */

enum tlLogType {
  tlLogStatus,
  tlLogWarning,
  tlLogError,
  tlLogNotCoded,
  tlLogCrashing
};

struct tlLog {
  const struct tlLogPriv* d;
  void (*init)(stringReadArgument_tapi(mainLogFile));
  void (*destroy)();
  // You can set file paths different each time you write to file
  // Also you can set paths NULL if you don't want to change file path
  // Formats are the same as string api's
  void (*save)(enum tlLogType logType, stringReadArgument_tapi(path));
  void (*log)(enum tlLogType type, unsigned char stopRunning, const wchar_t* format, ...);
};

#ifdef __cplusplus
}
#endif