#pragma once
#include "TCore.h"

TCORE_BEGIN_C_LINKAGE
TCORE_PLUGIN_DEFINE(TCLog, "tcLog", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

/*
 * You should init the log system. This will make every log to be saved in the main log file
 * If you want to save specific type of logs to a specific text file, you can query it with save().
 * All logs are saved as UTF32.
 * You can use all UTFs by defining them with proper format argument: 8 -> %s, 16 -> %v, 32 -> %w
 * There is a limit of characters per log. It is defined with maxCharPerLog_tapi constant.
 */

typedef enum TCLogType {
  TC_LOG_STATUS,
  TC_LOG_WARNING,
  TC_LOG_ERROR,
  TC_LOG_NOT_CODED,
  TC_LOG_CRASHING
};

typedef struct TCLogServices {
  void (*Initialize)(const char* logFilePath);
  void (*Destroy)();
  // You can set file paths different each time you write to file
  // Also you can set paths NULL if you don't want to change file path
  // Formats are the same as string api's
  void (*Save)(enum TCLogType logType, const char* filePath);
  void (*Log)(enum TCLogType type, unsigned char stopRunning, const wchar_t* format, ...);
} TCLogServices;

TCORE_END_C_LINKAGE