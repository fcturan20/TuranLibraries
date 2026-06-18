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

typedef enum TCLogLevel
{
	TC_LOG_LEVEL_STATUS,
	TC_LOG_LEVEL_WARNING,
	TC_LOG_LEVEL_ERROR,
	TC_LOG_LEVEL_NOT_CODED,
	TC_LOG_LEVEL_CRASHING
};

typedef struct TCLogServices
{
	void (*Initialize)(const char* logFilePath);
	void (*Destroy)();
	// You can set file paths different each time you write to file
	// Also you can set paths as TC_NULL, if you don't want to change file path
	// Formats are the same as string api's
	void (*Save)(enum TCLogLevel logType, const char* filePath);
	// If time_point is 0, current time will be used. Otherwise, the time_point will be used to log the log's time.
	// If owner is TC_NULL, the log's owner will be caller plugin's name.
	void (*Log)(enum TCLogLevel type, size_t time_point, const char* owner, const char* text, const char* details);
} TCLogServices;

TCORE_END_C_LINKAGE

#define TCORE_USE_CPP_WRAPPER
#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
// C++ wrapper
#include <TString.h>

namespace TCore
{

class tl
{
public:
	tl(TCLogLevel level, const char* owner) : Level(level)
	{
		TCORE_SOFT_CHECK(TCLog,
						 "Log system is not initialized. Please call "
						 "TCLog->Initialize() before using the log system.");
	}

	~tl()
	{
		if (!TCLog)
			return;
		TCLog->Log(Level, 0, TC_NULL, Message.C_str(), Details.C_str());
	}

	void StartDetailing() { IsStartedDetailing = true; }

	void Append(const char* text)
	{
		if (IsStartedDetailing)
			Details.Append(text);
		else
			Message.Append(text);
	}

	TCLogLevel Level;
	TCore::String Message;
	TCore::String Details;
	bool IsStartedDetailing = false;
};
} // namespace TCore
#endif