#pragma once
#include "predefinitions_tapi.h"
#define LOGGER_TAPI_PLUGIN_NAME "tapi_logger"
#define LOGGER_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define LOGGER_TAPI_PLUGIN_LOAD_TYPE logger_tapi_type*

/*
 * You should init the log system. This will make every log to be saved in the main log file
 * If you want to save specific type of logs to a specific text file, you can query it with save().
 * All logs are saved as UTF32.
 * You can use all UTFs by defining them with proper format argument: 8 -> %s, 16 -> %v, 32 -> %w
 * There is a limit of characters per log. It is defined with maxCharPerLog_tapi constant.
 */

typedef enum tapi_log_type {
  log_type_tapi_STATUS,
  log_type_tapi_WARNING,
  log_type_tapi_ERROR,
  log_type_tapi_NOTCODED,
  log_type_tapi_CRASHING
} log_type_tapi;

typedef struct logger_tapi {
  void (*init)(stringReadArgument_tapi(mainLogFile));
  void (*destroy)();
  // You can set file paths different each time you write to file
  // Also you can set paths NULL if you don't want to change file path
  void (*save)(tapi_log_type logType, stringReadArgument_tapi(path));
  void (*log)(tapi_log_type type, unsigned char stopRunning, const wchar_t* format, ...);
} logger_tapi;

typedef struct logger_tapi_d logger_tapi_d;
typedef struct logger_tapi_type {
  logger_tapi_d* data;
  logger_tapi*   funcs;
} logger_tapi_type;
